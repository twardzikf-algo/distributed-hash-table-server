cmake_minimum_required(VERSION 3.15)
project(Block2 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../build)


add_executable(server server.c)
add_executable(client client.c)