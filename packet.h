#include <stdbool.h>
#include <stdint.h>

#ifndef PACKET_H
#define PACKET_H

struct packet {
  uint8_t type;
  uint8_t length;
  const char *name;
  unsigned char *data;
};

struct packet new_packet(int type, unsigned char *data);
struct packet read_packet(int fd);
void write_packet(int fd, struct packet *packet);
bool parse_placements(unsigned char *data, uint8_t placements[5][3]);

#endif