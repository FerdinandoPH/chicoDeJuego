cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(chicoDeJuego C CXX)

# C++20 is mandatory (no fallback to earlier versions)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generates build/compile_commands.json with the exact flags for every file.
# VSCode's IntelliSense uses it so it doesn't have to guess the include paths
# (see .vscode/c_cpp_properties.json).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# On Windows CMake puts the -I flags into a response file (@file.rsp) to stay
# under the command line length limit. The catch is that cpptools does not
# expand those @file arguments, so compile_commands.json would end up with no
# include paths at all. There are only 4 directories, no risk of overflowing:
# pass them inline.
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

# Default build type. Only set if the user did not already pass it on the
# command line (-DCMAKE_BUILD_TYPE=Release), so we don't override their choice.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Compilation flags ---
# gcc is used on every platform (MinGW on Windows, gcc on Linux), so these GNU
# flags work for all four combinations.
#
# Warnings apply to every build type:
#   -Wall                enables most of the useful warnings
#   -Wno-unknown-pragmas silences warnings about #pragmas gcc doesn't know
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unknown-pragmas")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-unknown-pragmas")

# Optimization is per build type, not global:
#   Debug   -> -Og, which keeps the code debuggable but is still fast enough to
#              run the emulator at full speed (plain -O0 is not). CMake's Debug
#              default is only "-g", so -Og is added here.
#   Release -> -O3, already CMake's default ("-O3 -DNDEBUG"), nothing to add.
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -Og")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")

# --- Link Time Optimization (LTO) ---
# LTO lets the optimizer work across all the .o files at once (better inlining,
# dead code elimination). Not every toolchain supports it (some ARM gcc builds
# ship without the plugin), so we ask first with check_ipo_supported and only
# enable it when available; otherwise we keep compiling without LTO instead of
# failing.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)  # enables LTO at compile time
    add_link_options(-flto=auto)                # LTO at link time too, in parallel
else()
    message(STATUS "IPO/LTO not supported: ${ipo_error}")
endif()

# --- SDL3 ---
if(WIN32)
    # On Windows the libraries are vendored in the repo, one copy per
    # architecture. CMAKE_SIZEOF_VOID_P is the size of a pointer in bytes:
    # 8 => 64-bit build (x64), otherwise => 32-bit (x86).
    if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
        set(ARCH_DIR "x64")
    else()
        set(ARCH_DIR "x86")
    endif()

    # SDL3_DIR tells find_package where the vendored SDL3Config.cmake lives
    # (instead of looking for it system-wide). SDL3_BIN_DIR is where the DLL is,
    # which we copy next to the executable further down.
    set(SDL3_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${ARCH_DIR}/lib/cmake/SDL3")
    set(SDL3_BIN_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${ARCH_DIR}/bin")

    find_package(SDL3 CONFIG REQUIRED)
else()
    # On Linux (x86/x64/ARM) we use the system-installed SDL3 (a development
    # package such as libsdl3-dev). find_package finds it on its own regardless
    # of the architecture: that's why ARM needs nothing special here.
    find_package(SDL3 CONFIG REQUIRED)
endif()

# --- Threads ---
# The emulator spawns a thread with std::thread (src/core/emu.cpp). On Linux the
# standard library implementation sits on top of pthread and has to be linked
# explicitly or the link fails; on MinGW it is already built in. find_package
# (Threads) detects the right way for each platform and exposes the target
# Threads::Threads; THREADS_PREFER_PTHREAD_FLAG asks for -pthread where it exists.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --- Sources ---
# One glob per directory instead of a recursive one, so the boundary between
# core and platform is explicit. (You still need to re-run cmake after adding
# new files.)
file(GLOB CORE_SRC     ${CMAKE_SOURCE_DIR}/src/core/*.cpp ${CMAKE_SOURCE_DIR}/libs/*.c)
file(GLOB PLATFORM_SRC ${CMAKE_SOURCE_DIR}/src/platform/sdl/*.cpp)

# --- The core: the emulator, with no platform dependency whatsoever ---
# Mind the target_link_libraries below: it does NOT link SDL3, and that is
# deliberate. If anyone sneaks SDL back into src/core/ the build breaks, which is
# exactly what we want. The factories from ports/backend.h are left unresolved
# here; the backend satisfies them when the executable is linked.
add_library(cdj_core STATIC ${CORE_SRC})
target_include_directories(cdj_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ports
    ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(cdj_core PUBLIC Threads::Threads)

# --- The executable: core + one concrete backend ---
# The entry point (main.cpp) is part of the backend: each one has its own
# (libretro doesn't even have a main), which is why the PLATFORM_SRC glob picks
# it up.
add_executable(chicoDeJuego ${PLATFORM_SRC})
target_link_libraries(chicoDeJuego PRIVATE cdj_core SDL3::SDL3)

# --- Installation ---
# Where the artifacts go with `cmake --install` / when packaging.
install(TARGETS chicoDeJuego
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)

# On Windows the DLLs have to be shipped alongside the binary too.
if (WIN32)
    install(FILES
        ${SDL3_BIN_DIR}/SDL3.dll
        DESTINATION bin)
endif ()

# Copy the DLLs next to the freshly built .exe (POST_BUILD) so it can be run
# straight from the build directory without installing.
if (WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${SDL3_BIN_DIR}/SDL3.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
endif()

# --- Packaging (CPack) ---
set(CPACK_PACKAGE_NAME "chicoDeJuego")
set(CPACK_PACKAGE_VENDOR "FerdinandoPH")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "chicoDeJuego emulator")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "chicoDeJuego")
set(CPACK_PACKAGE_EXECUTABLES "chicoDeJuego" "chicoDeJuego")

# NSIS installer on Windows; a plain .tar.gz on Linux.
if (WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "chicoDeJuego")
    set(CPACK_NSIS_PACKAGE_NAME "chicoDeJuego")
else()
    set(CPACK_GENERATOR "TGZ")
endif ()

include(CPack)
