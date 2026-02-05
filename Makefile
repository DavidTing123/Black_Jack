CC = gcc
CFLAGS = -Wall -Iinclude -g
LDFLAGS = -lrt -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = .

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# Exclude client from server build objects if needed, 
# typically we want specific targets.
# Existing layout seems to imply all src/*.c might be compiled.
# But server.c has main, client.c has main.
# We need to separate them.

SERVER_SRCS = src/server.c src/game_logic.c src/scheduler.c src/shared_mem.c
SERVER_OBJS = $(server_srcs:.c=.o)

CLIENT_SRCS = src/client.c src/network.c
CLIENT_OBJS = $(client_srcs:.c=.o)

all: server client

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o server $(SERVER_SRCS) $(LDFLAGS)

client: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o client $(CLIENT_SRCS) $(LDFLAGS)

clean:
	rm -f server client *.o
