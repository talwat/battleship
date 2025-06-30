TARGETS = server client
CC = gcc
CFLAGS = -g -Wall -DUSESDL `sdl2-config --cflags` -Ilib -Isrc
LIBS = -Llib -lsam -lncurses -lform -lmenu -lm `sdl2-config --libs`

SOURCES = $(wildcard src/*.c) \
          $(wildcard src/server/*.c) \
          $(wildcard src/client/*.c)

HEADERS = $(wildcard src/*.h) \
          $(wildcard src/server/*.h) \
          $(wildcard src/client/*.h)

SERVER_MAIN = src/server.c
CLIENT_MAIN = src/client.c

SHARED_SOURCES = $(filter-out $(SERVER_MAIN) $(CLIENT_MAIN), $(wildcard src/*.c))

SERVER_OBJECTS = \
  obj/server.o \
  $(patsubst src/%.c,obj/%.o,$(SHARED_SOURCES)) \
  $(patsubst src/server/%.c,obj/server/%.o,$(wildcard src/server/*.c))

CLIENT_OBJECTS = \
  obj/client.o \
  $(patsubst src/%.c,obj/%.o,$(SHARED_SOURCES)) \
  $(patsubst src/client/%.c,obj/client/%.o,$(wildcard src/client/*.c))

.PHONY: all clean format

all: $(TARGETS)

lib/libsam.a:
	mkdir -p lib
	git clone https://github.com/talwat/libsam.git libsam
	$(MAKE) -C libsam
	mv libsam/libsam.a lib/libsam.a
	rm -rf libsam

# Main files
obj/server.o: src/server.c $(HEADERS)
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/client.o: src/client.c $(HEADERS)
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# Shared sources
obj/%.o: src/%.c $(HEADERS)
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# Server-specific
obj/server/%.o: src/server/%.c $(HEADERS)
	mkdir -p obj/server
	$(CC) $(CFLAGS) -c $< -o $@

# Client-specific
obj/client/%.o: src/client/%.c $(HEADERS)
	mkdir -p obj/client
	$(CC) $(CFLAGS) -c $< -o $@

client: lib/libsam.a $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) $(CLIENT_OBJECTS) -o $@ $(LIBS)

server: lib/libsam.a $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) $(SERVER_OBJECTS) -o $@ $(LIBS)

clean:
	rm -rf obj lib *.o $(TARGETS)

format:
	clang-format -i src/*.c src/*.h src/server/*.c src/server/*.h src/client/*.c src/client/*.h