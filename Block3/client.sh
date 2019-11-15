gcc -o client clientController.c client.c
./client localhost 4446 SET a 123
./client localhost 4446 GET a
