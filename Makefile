CC = gcc
CFLAGS = -pthread -Wall -I./include
LDFLAGS = -lrt -pthread
TARGET = server
SRCS = src/server.c src/shared_mem.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Build successful"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	rm -f /dev/shm/*bj* 2>/dev/null || true

run: $(TARGET)
	./$(TARGET)

test:
	timeout 8 ./$(TARGET) || true

.PHONY: all clean run test
