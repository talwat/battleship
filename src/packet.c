#include "packet.h"
#include "game.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

const bool packet_debug = false;

// clang-format off
// Contains a list of packet definitions.
const struct {
  uint8_t length;
  const char *name;
} PACKETS[9] = {
    {.length = 0,  .name = "NONE"},          // 0x0
    {.length = 16, .name = "LOGIN"},         // 0x1
    {.length = 1,  .name = "LOGIN_CONFIRM"}, // 0x2
    {.length = 16, .name = "SETUP"},         // 0x3
    {.length = 10, .name = "PLACE"},         // 0x4
    {.length = 1,  .name = "TURN"},          // 0x5
    {.length = 1,  .name = "SELECT"},        // 0x6
    {.length = 3,  .name = "TURN_RESULT"},   // 0x7
    {.length = 0,  .name = "QUIT"},          // 0x8
};

bool validate_ship(struct ship *ship, int index, enum Tile board[10][10]) {
  if (ship->x < 0 || ship->x >= 10 || ship->y < 0 || ship->y >= 10) {
    return false;
  }

  if (ship->orientation != HORIZONTAL && ship->orientation != VERTICAL) {
    return false;
  }

  for (int i = 0; i < SHIP_LENGTHS[index]; i++) {
    enum Tile tile = (ship->orientation == HORIZONTAL) ? board[ship->x + i][ship->y] : board[ship->x][ship->y + i];
    if (tile != TILE_EMPTY) {
      return false;
    }
  }

  if (ship->sunk) {
    return false;
  }

  return true;
}

bool parse_placements(unsigned char *data, struct ship placements[5]) {
  for (int i = 0; i < PACKETS[PLACE].length; i+=2) {
    unsigned char position = data[i+1];

    uint8_t high = (position >> 4) & 0x0F;
    uint8_t low  = position & 0x0F;

    struct ship *ship = &placements[i/2];

    ship->orientation = data[i];
    ship->x = low;
    ship->y = high;
    ship->defined = true;
    ship->sunk = false;
  }

  return true;
}

struct packet read_packet(int fd) {
  PacketType type;
  read(fd, &type, 1);

  if (type == NONE) {
    exit(EXIT_FAILURE);
  }

  if (packet_debug)
    printf("packet: received packet type %d (%s) with length %d\n", type, PACKETS[type].name, PACKETS[type].length);

  unsigned char *data = NULL;
  if (PACKETS[type].length > 0) {
    data = malloc(PACKETS[type].length);
    read(fd, data, PACKETS[type].length);
  }

  struct packet new = new_packet(type, data);
  
  if (packet_debug) {
    printf("        ");
    for (int i = 0; i < new.length; i++) {
      printf("%02x ", new.data[i]);
    }
    printf("\n");
  }

  return new;
}

struct packet new_packet(int type, unsigned char *data) {
  struct packet result = {
    .type = type,
    .length = PACKETS[type].length,
    .name = PACKETS[type].name,
    .data = data
  };

  return result;
}

void free_packet(struct packet *packet) {
  if (packet->data != NULL) {
    free(packet->data);
    packet->data = NULL;
  }

  packet->length = 0;
  packet->type = NONE;
  packet->name = NULL;
}

void write_packet(int fd, struct packet *packet) {
  if (packet_debug) {
    printf("packet: sending packet type %d (%s) with length %d to %d\n", packet->type, packet->name, packet->length, fd);
    printf("        ");
    for (int i = 0; i < packet->length; i++) {
      printf("%02x ", packet->data[i]);
    }
    printf("\n");
  }
  write(fd, &packet->type, sizeof(packet->type));
  write(fd, packet->data, packet->length);
}