project(Block3 C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c item.c)
add_executable(client client.c )
