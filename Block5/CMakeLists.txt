cmake_minimum_required(VERSION 3.10)
project(Block4 C)

set(CMAKE_C_STANDARD 11)

set(GCC_ANONYMOUS_ELEMENTS_FLAG	"-fms-extensions")
set(GCC_DEBUG_INFORMATION "-g")
add_definitions(${GCC_ANONYMOUS_ELEMENTS_FLAG} ${GCC_DEBUG_INFORMATION})

add_executable(peer peer.c protocol.c table.c net.c)
