cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(chicoDeJuego C CXX)
set(CMAKE_CXX_STANDARD 20)
set(MAIN_SOURCE chicoDeJuego.cpp)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -Wall -Wno-unknown-pragmas")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -Wall -Wno-unknown-pragmas")

if(WIN32)
    include_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/include")

    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
        set(SDL3_ARCH_DIR "x64")
        link_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/lib/x64")
    else ()
        set(SDL3_ARCH_DIR "x86")
        link_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/lib/x86")
    endif ()
    set(SDL3_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${SDL3_ARCH_DIR}/lib/cmake/SDL3")
    set(SDL3_BIN_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${SDL3_ARCH_DIR}/bin")

    find_package(SDL3 CONFIG REQUIRED)
else()
    find_package(SDL3 CONFIG REQUIRED)
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/libs)
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/src/*.hpp ${CMAKE_SOURCE_DIR}/libs/*.c ${CMAKE_SOURCE_DIR}/libs/*.h)

add_executable(chicoDeJuego ${MAIN_SOURCE} ${SRC_FILES})
target_link_libraries(chicoDeJuego SDL3::SDL3)

install(TARGETS chicoDeJuego
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)

if (WIN32)
    install(FILES
        ${SDL3_BIN_DIR}/SDL3.dll
        DESTINATION bin)

    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
        install(FILES
            ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/x64/pthreadVC2.dll
            DESTINATION bin)
    else ()
        install(FILES
            ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/x86/pthreadVC2.dll
            DESTINATION bin)
    endif ()
endif ()

if (WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${SDL3_BIN_DIR}/SDL3.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)

    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/x64/pthreadVC2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    else ()
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/x86/pthreadVC2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    endif()
endif()

set(CPACK_PACKAGE_NAME "chicoDeJuego")
set(CPACK_PACKAGE_VENDOR "FerdinandoPH")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Emulador chicoDeJuego")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "chicoDeJuego")
set(CPACK_PACKAGE_EXECUTABLES "chicoDeJuego" "chicoDeJuego")

if (WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "chicoDeJuego")
    set(CPACK_NSIS_PACKAGE_NAME "chicoDeJuego")
endif ()

include(CPack)