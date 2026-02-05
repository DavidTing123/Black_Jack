# add other code here ->>>>

client: src/client.c src/network.c
	gcc -o client src/client.c src/network.c -Iinclude

.PHONY: client
