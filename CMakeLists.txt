cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(chicoDeJuego C CXX)

# Estandar C++20 obligatorio (sin fallback a versiones anteriores)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(MAIN_SOURCE chicoDeJuego.cpp)

# Tipo de build por defecto. Se pone solo si el usuario no lo dio ya en la
# linea de comandos (-DCMAKE_BUILD_TYPE=Release), asi no pisamos su eleccion.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Flags de compilacion ---
# Se usa gcc en todas las plataformas (MinGW en Windows, gcc en Linux), asi que
# estos flags de GNU valen para las cuatro combinaciones:
#   -O3                  optimizacion agresiva
#   -Wall                activa la mayoria de warnings utiles
#   -Wno-unknown-pragmas silencia los warnings por #pragma que gcc no conoce
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -Wall -Wno-unknown-pragmas")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -Wall -Wno-unknown-pragmas")

# --- Link Time Optimization (LTO) ---
# El LTO deja que el optimizador trabaje entre todos los .o a la vez (mejor
# inlining, elimina codigo muerto). No todos los toolchains lo soportan (algunos
# gcc de ARM vienen sin el plugin), asi que preguntamos primero con
# check_ipo_supported y solo lo activamos si esta disponible; si no, seguimos
# compilando sin LTO en vez de fallar.
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)  # activa LTO en la compilacion
    add_link_options(-flto=auto)                # LTO tambien al enlazar, en paralelo
else()
    message(STATUS "IPO/LTO no soportado: ${ipo_error}")
endif()

# --- SDL3 ---
if(WIN32)
    # En Windows las librerias vienen empaquetadas en el repo, una copia por
    # arquitectura. CMAKE_SIZEOF_VOID_P es el tamano de un puntero en bytes:
    # 8 => compilacion de 64 bits (x64), si no => 32 bits (x86).
    if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
        set(ARCH_DIR "x64")
    else()
        set(ARCH_DIR "x86")
    endif()

    # Cabeceras (.h) y libreria (.lib/.a) del pthread empaquetado para Windows.
    include_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/include")
    link_directories("${PROJECT_SOURCE_DIR}/windows_libs/pthread/lib/${ARCH_DIR}")

    # SDL3_DIR le dice a find_package donde esta el SDL3Config.cmake empaquetado
    # (en vez de buscarlo por el sistema). SDL3_BIN_DIR es donde esta la DLL, que
    # copiaremos junto al ejecutable mas abajo.
    set(SDL3_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${ARCH_DIR}/lib/cmake/SDL3")
    set(SDL3_BIN_DIR "${PROJECT_SOURCE_DIR}/libs/sdl3/${ARCH_DIR}/bin")

    find_package(SDL3 CONFIG REQUIRED)
else()
    # En Linux (x86/x64/ARM) usamos el SDL3 instalado en el sistema
    # (paquete de desarrollo tipo libsdl3-dev). find_package lo localiza solo,
    # sin importar la arquitectura: por eso ARM no necesita nada especial aqui.
    find_package(SDL3 CONFIG REQUIRED)
endif()

# --- Threads (pthread) ---
# El emulador crea un hilo con pthread_create (src/emu.cpp). MinGW ya trae
# pthread integrado (libwinpthread) y lo enlaza solo, pero en Linux hay que
# enlazar la libreria de hilos explicitamente o el enlace falla. find_package
# (Threads) detecta la forma correcta segun la plataforma y expone el target
# Threads::Threads; THREADS_PREFER_PTHREAD_FLAG pide usar -pthread cuando exista.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Directorios donde el compilador busca cabeceras propias del proyecto.
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/libs)

# Recolecta recursivamente todos los fuentes de src/ y libs/.
# (GLOB_RECURSE es comodo, pero recuerda: al anadir archivos nuevos hay que
#  volver a ejecutar cmake para que los detecte.)
file(GLOB_RECURSE SRC_FILES ${CMAKE_SOURCE_DIR}/src/*.c ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h ${CMAKE_SOURCE_DIR}/src/*.hpp ${CMAKE_SOURCE_DIR}/libs/*.c ${CMAKE_SOURCE_DIR}/libs/*.h)

# El ejecutable y las librerias contra las que se enlaza.
add_executable(chicoDeJuego ${MAIN_SOURCE} ${SRC_FILES})
target_link_libraries(chicoDeJuego SDL3::SDL3 Threads::Threads)

# --- Instalacion ---
# Donde van los artefactos con `cmake --install` / al empaquetar.
install(TARGETS chicoDeJuego
RUNTIME DESTINATION bin
LIBRARY DESTINATION lib
ARCHIVE DESTINATION lib)

# En Windows hay que distribuir tambien las DLLs junto al binario.
if (WIN32)
    install(FILES
        ${SDL3_BIN_DIR}/SDL3.dll
        DESTINATION bin)

    install(FILES
        ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/${ARCH_DIR}/pthreadVC2.dll
        DESTINATION bin)
endif ()

# Copia las DLLs al lado del .exe recien compilado (POST_BUILD) para poder
# ejecutarlo directamente desde el directorio de build sin instalar.
if (WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${SDL3_BIN_DIR}/SDL3.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)

    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/windows_libs/pthread/dll/${ARCH_DIR}/pthreadVC2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        VERBATIM)
endif()

# --- Empaquetado (CPack) ---
set(CPACK_PACKAGE_NAME "chicoDeJuego")
set(CPACK_PACKAGE_VENDOR "FerdinandoPH")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Emulador chicoDeJuego")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "chicoDeJuego")
set(CPACK_PACKAGE_EXECUTABLES "chicoDeJuego" "chicoDeJuego")

# Instalador NSIS en Windows; en Linux un simple .tar.gz.
if (WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "chicoDeJuego")
    set(CPACK_NSIS_PACKAGE_NAME "chicoDeJuego")
else()
    set(CPACK_GENERATOR "TGZ")
endif ()

include(CPack)
