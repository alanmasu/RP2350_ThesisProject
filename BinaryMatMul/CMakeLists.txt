add_library(BinaryMatMul STATIC
    src/BinaryMatMul.c
)

target_include_directories(BinaryMatMul PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
