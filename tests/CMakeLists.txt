# 
# This file contains the specifications to run the unit tests
# 

# Fetch unit testing library
include(../cmake/cmocka.cmake)

# Include directory where to find tinymodbus.h
include_directories(PRIVATE ${CMAKE_SOURCE_DIR}/)

# Link unit testing library
link_libraries(cmocka)

# Build test execusables
add_executable(example_test example_test.c)

# Add tests to be run with `ctest`
enable_testing()
add_test(NAME example_test COMMAND example_test)
