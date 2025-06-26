#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"

#define PORT 8080

uint8_t merge(uint8_t lower, uint8_t upper) {
  return ((upper & 0x0F) << 4) | (lower & 0x0F);
}

void serialize_placements(unsigned char *data, uint8_t placements[5][3]) {
  for (int i = 0; i < 5; i++) {
    uint8_t direction = placements[i][0];
    uint8_t x = placements[i][1];
    uint8_t y = placements[i][2];

    uint8_t position = merge(x, y);

    data[i * 2] = direction;
    data[i * 2 + 1] = position;
  }
}

void handle(int connection) {
  unsigned char name[16] = "bobo";

  struct packet login = new_packet(LOGIN, name);
  write_packet(connection, &login);

  struct packet confirm = read_packet(connection);
  printf("client: received confirmation for player %d\n", confirm.data[0]);
  uint8_t player_id = confirm.data[0];

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

  struct packet place = new_packet(PLACE, data);
  write_packet(connection, &place);

  while (true) {
    struct packet turn = read_packet(connection);
    if (turn.type == END) {
      printf("client: game over\n");
      break;
    }

    if (*turn.data == player_id) {
      printf("client: it's my turn!\n");

      int x;
      printf("client: select a target x (1-10): ");
      scanf("%d", &x);

      char y;
      printf("client: select a target y (A-J): ");
      scanf(" %c", &y);

      unsigned char position = merge(x - 1, toupper(y) - 'A');
      struct packet select = new_packet(SELECT, &position);
      write_packet(connection, &select);
    } else {
      printf("client: waiting for opponent's turn...\n");
    }

    struct packet turn_result = read_packet(connection);
    enum TurnResult result = turn_result.data[2];

    if (result == TURN_HIT) {
      printf("client: hit!\n");
    } else if (result == TURN_MISS) {
      printf("client: miss!\n");
    } else if (result == TURN_SINK) {
      printf("client: sink!\n");
    } else if (result == TURN_WIN) {
      printf("client: win!\n");
      break;
    } else {
      printf("client: unknown turn result %d\n", result);
    }
  }
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