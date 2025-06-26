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

enum PacketType {
  LOGIN = 0x0,
  LOGIN_CONFIRM = 0x1,
  SETUP = 0x2,
  PLACE = 0x3,
  TURN = 0x4,
  SELECT = 0x5,
  TURN_RESULT = 0x6,
  END = 0x7,
  QUIT = 0x8,
};

struct packet new_packet(int type, unsigned char *data);
struct packet read_packet(int fd);
void write_packet(int fd, struct packet *packet);
bool parse_placements(unsigned char *data, uint8_t placements[5][3]);

#endif