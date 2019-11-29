CC = gcc
CFLAGS_D = -Wall -g -fsanitize=address

compile:
peer:
	@$(CC) $(CFLAGS_D) -o peer peer.c
	@./peer 15 127.0.0.1 4711 245 127.0.0.1 4710 112 127.0.0.1 4712
	@rm -f peer

client: client.c
	@$(CC) $(CFLAGS_D) -o client client.c
	./client 127.0.0.1 4711 SET analfabetyzm siekurwaszerzy

test: client.c
	@$(CC) $(CFLAGS_D) -o client client.c
	@echo "\033[31mtest: GET invalid key\033[0m"
	@./client 127.0.0.1 4711 GET a
	@echo "\033[31mtest: SET valid key-value\033[0m"
	@./client 127.0.0.1 4711 SET a aValue
	@echo "\033[31mtest: GET valid key\033[0m"
	@./client 127.0.0.1 4711 GET a
	@echo "\033[31mtest: DEL valid key\033[0m"
	@./client 127.0.0.1 4711 DEL a
	@echo "\033[31mtest: DEL invalid key\033[0m"
	@./client 127.0.0.1 4711 DEL a
	@rm -f client

test-lookup: client.c
	@$(CC) $(CFLAGS_D) -o client client.c
	@./client 127.0.0.1 4711 LOOKUP 10
	@rm -f client

test-reply: client.c
	@$(CC) $(CFLAGS_D) -o client client.c
	@./client 127.0.0.1 4711 REPLY 10
	@rm -f client
clean:
	rm -f client server
