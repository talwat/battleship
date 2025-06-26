#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "packet.h"

#define PORT 8080

void serialize_placements(unsigned char *data, uint8_t placements[5][3]) {
  for (int i = 0; i < 5; i++) {
    uint8_t direction = placements[i][0];
    uint8_t x = placements[i][1];
    uint8_t y = placements[i][2];

    uint8_t position = ((y & 0x0F) << 4) | (x & 0x0F);

    data[i * 2] = direction;
    data[i * 2 + 1] = position;
  }
}

void handle(int connection) {
  unsigned char name[16] = "bobo";

  struct packet login = new_packet(0x0, name);
  write_packet(connection, &login);

  struct packet confirm = read_packet(connection);
  printf("client: received confirmation for player %d\n", confirm.data[0]);

  printf("client: waiting for other player...\n");
  struct packet setup = read_packet(connection);

  unsigned char *opponent_name = setup.data;
  printf("client: opponent name is %s\n", opponent_name);

  uint8_t placements[5][3] = {
      {1, 0, 0}, // CARRIER
      {1, 2, 0}, // BATTLESHIP
      {1, 4, 0}, // CRUISER
      {1, 6, 0}, // SUBMARINE
      {1, 8, 0}  // DESTROYER
  };

  unsigned char data[10] = {0};
  serialize_placements(data, placements);

  struct packet place = new_packet(0x3, data);
  write_packet(connection, &place);
}

int main() {
  int fd;
  struct sockaddr_in address;
  socklen_t address_len = sizeof(address);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("error: socket failed\n");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(PORT);

  if (connect(fd, (struct sockaddr *)&address, address_len) < 0) {
    perror("error: connection failed\n");
    exit(EXIT_FAILURE);
  } else {
    printf("client: connected to server on port %d\n", PORT);
  }

  handle(fd);
  close(fd);

  return 0;
}