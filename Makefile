TARGETS = server client
CC = gcc
CFLAGS = -g -Wall
LIBS = -lm

SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)

SERVER_MAIN = server.c
CLIENT_MAIN = client.c

SHARED_SOURCES = $(filter-out $(SERVER_MAIN) $(CLIENT_MAIN), $(SOURCES))

SERVER_OBJECTS = $(patsubst %.c, %.o, $(SERVER_MAIN) $(SHARED_SOURCES))
CLIENT_OBJECTS = $(patsubst %.c, %.o, $(CLIENT_MAIN) $(SHARED_SOURCES))

.PHONY: all clean

all: $(TARGETS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) $(SERVER_OBJECTS) -o $@ $(LIBS)

client: $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) $(CLIENT_OBJECTS) -o $@ $(LIBS)

clean:
	rm -f *.o $(TARGETS)

format:
	clang-format -i -- *.c *.h