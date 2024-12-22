cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(chicoDeJuego CXX)
set(MAIN_SOURCE chicoDeJuego.cpp)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wno-unknown-pragmas")
if(WIN32)
    include_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/include")
    set(SDL2_DIR "${PROJECT_SOURCE_DIR}/windows_libs/sdl2")
    set(SDL2_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/windows_libs/sdl2/include;${PROJECT_SOURCE_DIR}/windows_libs/sdl2/include/SDL2")
    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
        link_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/lib/x64")
        set(SDL2_LIBRARIES "${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x64/SDL2.lib;${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x64/SDL2main.lib")
    else ()
        link_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/lib/x86")
        set(SDL2_LIBRARIES "${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x86/SDL2.lib;${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x86/SDL2main.lib")
    endif ()
    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
    set(SDL2_TTF_DIR "${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf")
    set(SDL2_TTF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/include")

    # Support both 32 and 64 bit builds
    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    set(SDL2_TTF_LIBRARIES "${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x64/SDL2_ttf.lib")
    else ()
    set(SDL2_TTF_LIBRARIES "${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x86/SDL2_ttf.lib")
    endif ()

    string(STRIP "${SDL2_TTF_LIBRARIES}" SDL2_TTF_LIBRARIES)
else()
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)
    find_package(SDL2 REQUIRED)
    find_package(SDL2_ttf REQUIRED)
endif()
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
add_executable(chicoDeJuego ${MAIN_SOURCE} ${SRC_FILES})
target_link_libraries(chicoDeJuego ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES})
install(TARGETS chicoDeJuego
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)
if (WIN32)

    if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x64/SDL2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x64/SDL2_ttf.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x64/zlib1.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x64/libfreetype-6.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
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
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2/lib/x86/SDL2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x86/SDL2_ttf.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x86/zlib1.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/sdl2_ttf/lib/x86/libfreetype-6.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/x86/pthreadVC2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
    endif()
endif()