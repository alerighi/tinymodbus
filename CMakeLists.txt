cmake_minimum_required(VERSION 3.25)

project(tinymodbus 
    VERSION 1.0.0
    DESCRIPTION "A tiny, header-only, portable and compliant Modbus implementation, suitable for embedded systems"
    LANGUAGES C
)

enable_testing()

add_subdirectory(tests)
add_subdirectory(examples)
