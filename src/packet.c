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
/* Contains a list of packet definitions. */
const struct {
  uint8_t length;
  const char *name;
} PACKETS[8] = {
    {.length = 16, .name = "LOGIN"},        // 0x0
    {.length = 1, .name = "LOGIN_CONFIRM"}, // 0x1
    {.length = 16, .name = "SETUP"},        // 0x2
    {.length = 10, .name = "PLACE"},        // 0x3
    {.length = 1, .name = "TURN"},          // 0x4
    {.length = 1, .name = "SELECT"},        // 0x5
    {.length = 3, .name = "TURN_RESULT"},   // 0x6
    {.length = 0, .name = "QUIT"},          // 0x7
};

bool parse_placements(unsigned char *data, struct ship placements[5]) {
  for (int i = 0; i < PACKETS[3].length; i+=2) {
      unsigned char position = data[i+1];

      uint8_t high = (position >> 4) & 0x0F;
      uint8_t low  = position & 0x0F;

      struct ship *ship = &placements[i/2];

      ship->orientation = data[i];
      ship->x = low;
      ship->y = high;
      
      uint8_t ship_length = SHIP_LENGTHS[i/2];

      for (int j = 0; j < 5; j++) {
          if (j >= ship_length) {
              ship->coordinates[j] = -1;
              continue;
          }

          if (ship->orientation == HORIZONTAL) {
              ship->coordinates[j] = ship->x + j;
          } else {
              ship->coordinates[j] = ship->y + j;
          }
      }
  }

  return true;
}

struct packet read_packet(int fd) {
  enum PacketType type;
  read(fd, &type, 1);

  // TODO: Make this implementation less terrible.
  if (type == QUIT) {
    exit(EXIT_SUCCESS);
  }

  // TODO: memory leak not good.
  unsigned char *data = malloc(PACKETS[type].length);
  read(fd, data, PACKETS[type].length);

  struct packet new = new_packet(type, data);
  
  if (packet_debug) {
    printf("packet: received packet type %d\n", new.type);
    printf("packet: %d\n", new.data[0]);
    printf("packet: received packet type %d (%s) with length %d\n", new.type, new.name, new.length);
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

void write_packet(int fd, struct packet *packet) {
  if (packet_debug) {
    printf("packet: sending packet type %d (%s) with length %d\n", packet->type, packet->name, packet->length);
  }
  write(fd, &packet->type, sizeof(packet->type));
  write(fd, packet->data, packet->length);
}