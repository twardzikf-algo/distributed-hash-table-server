cmake_minimum_required(VERSION 3.15)
project(Block2 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_BINARY_DIR build EXECUTABLE_OUTPUT_PATH build)

set( LEARN_CMAKE_SOURCES
        server.c
        client.c
        )

add_executable(Block2 server.c)
add_executable(Block2 client.c)