#include "packet.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// clang-format off
/* Contains a list of packet definitions. */
const struct {
  uint8_t length;
  const char *name;
} PACKETS[9] = {
    {.length = 16, .name = "LOGIN"},
    {.length = 1, .name = "LOGIN_CONFIRM"},
    {.length = 16, .name = "SETUP"},
    {.length = 10, .name = "PLACE"},
    {.length = 1, .name = "TURN"},
    {.length = 1, .name = "SELECT"},
    {.length = 3, .name = "TURN_RESULT"},
    {.length = 1, .name = "END"},
    {.length = 0, .name = "QUIT"},
};

bool parse_placements(unsigned char *data, uint8_t placements[5][3]) {
  for (int i = 0; i < PACKETS[3].length; i+=2) {
      unsigned char position = data[i+1];

      uint8_t high = (position >> 4) & 0x0F;
      uint8_t low  = position & 0x0F;

      placements[i/2][0] = data[i];
      placements[i/2][1] = low;
      placements[i/2][2] = high;
  }

  return true;
}

/* Reads a packet from the file descriptor. */
struct packet read_packet(int fd) {
  uint8_t type;
  read(fd, &type, sizeof(type));

  // TODO: memory leak not good.
  unsigned char *data = malloc(PACKETS[type].length);
  read(fd, data, PACKETS[type].length);

  printf("packet: received data: ");
  for (int i = 0; i < PACKETS[type].length; i++) {
    printf("%02x ", data[i]);
  }
  printf("\n");

  struct packet new = new_packet(type, data);
  printf("packet: received packet type %d (%s) with length %d\n", new.type, new.name, new.length);

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
  printf("packet: sending data: ");
  for (int i = 0; i < packet->length; i++) {
    printf("%02x ", packet->data[i]);
  }
  printf("\n");
  printf("packet: sending packet type %d (%s) with length %d\n", packet->type, packet->name, packet->length);

  write(fd, &packet->type, sizeof(packet->type));
  write(fd, packet->data, packet->length);
}