# Compiler and Flags
CC = gcc
CFLAGS = -pthread -Wall -Wextra -g -I./include
LDFLAGS = -pthread -lrt

# Directories
SRC_DIR = src
OBJ_DIR = src
INC_DIR = include

# Target Binaries
SERVER = server
CLIENT = client

# Object Files
SERVER_OBJS = $(OBJ_DIR)/server.o $(OBJ_DIR)/shared_mem.o $(OBJ_DIR)/game_logic.o $(OBJ_DIR)/logger.o
CLIENT_OBJS = $(OBJ_DIR)/client.o

# --- Build Rules ---

all: $(SERVER) $(CLIENT)

# Link Server
$(SERVER): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $(SERVER) $(LDFLAGS)

# Link Client
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o $(CLIENT) $(LDFLAGS)

# Compile Source Files to Object Files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Utility Rules ---

# Remove binaries and object files
clean:
	rm -f $(SERVER) $(CLIENT) $(OBJ_DIR)/*.o game.log
	@echo "Cleanup complete."

# Rebuild from scratch
rebuild: clean all

.PHONY: all clean rebuild