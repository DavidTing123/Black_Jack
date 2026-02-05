# add other code here ->>>>

client: src/client.c src/network.c
	gcc -o client src/client.c src/network.c -Iinclude

server: src/server.c src/shared_mem.c src/game_logic.c src/network.c
	gcc -o server src/server.c src/shared_mem.c src/game_logic.c src/network.c -Iinclude -lpthread -lrt

all: client server

.PHONY: client server all
