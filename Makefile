TARGETS = server client
CC = gcc
CFLAGS = -g -Wall
LIBS = -lm

SOURCES = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)

SERVER_MAIN = src/server.c
CLIENT_MAIN = src/client.c

SHARED_SOURCES = $(filter-out $(SERVER_MAIN) $(CLIENT_MAIN), $(SOURCES))

SERVER_OBJECTS = $(patsubst src/%.c, obj/%.o, $(SERVER_MAIN) $(SHARED_SOURCES))
CLIENT_OBJECTS = $(patsubst src/%.c, obj/%.o, $(CLIENT_MAIN) $(SHARED_SOURCES))

.PHONY: all clean

all: $(TARGETS)

obj/%.o: src/%.c $(HEADERS)
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) $(SERVER_OBJECTS) -o $@ $(LIBS)

client: $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) $(CLIENT_OBJECTS) -o $@ $(LIBS)

clean:
	rm -rf obj *.o $(TARGETS)

format:
	clang-format -i -- src/*.c src/*.h