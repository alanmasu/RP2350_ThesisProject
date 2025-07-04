set(HEADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/include/BinaryMatMulConfig.h")

# Controlla se esiste, se no lo crea durante la configurazione
if(NOT EXISTS "${HEADER_FILE}")
    message(STATUS "Il file ${HEADER_FILE} non esiste. Lo creo automaticamente.")
    file(WRITE "${HEADER_FILE}" "#pragma once\n\n// Header creato da CMake\n#define MATH_COMPUTE\n#define TRANSPOSE_COMPUTE\n")
endif()

add_library(BinaryMatMul STATIC
    src/BinaryMatMul.c
)

target_include_directories(BinaryMatMul PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(BinaryMatMul PUBLIC pico_stdlib)