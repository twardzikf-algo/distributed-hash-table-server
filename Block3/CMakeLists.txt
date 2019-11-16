project(Block3 C)

set(CMAKE_C_STANDARD 99)

add_executable(server serverController.c server.c item.c)
add_executable(client clientController.c client.c )