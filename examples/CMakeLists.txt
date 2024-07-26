#
# This file is used to compile the examples to be run
#

include_directories(PRIVATE ${CMAKE_SOURCE_DIR}/)

add_executable(example_posix_tcpip example_posix_tcpip.c)
