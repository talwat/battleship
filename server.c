#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"

#define PORT 8080

struct player {
  uint8_t id;
  char *name;
  int fd;
};

struct player wait_for_player(uint8_t id, int fd, struct sockaddr *address, socklen_t *address_len) {
  int connection = accept(fd, address, address_len);
  if (connection < 0) {
    perror("error: failed to accept connection\n");
    exit(EXIT_FAILURE);
  } else {
    printf("server: connection accepted for player %d\n", id);
  }

  struct packet login = read_packet(connection);
  char *name = (char *)login.data;

  struct packet confirm = new_packet(1, &id);
  write_packet(connection, &confirm);

  struct player result = {
      .id = id,
      .fd = connection,
      .name = name,
  };

  printf("server: player %d (%s) connected\n", result.id, result.name);

  return result;
}

void close_player(struct player *player) {
  printf("server: player %d connection closed\n", player->id);
  close(player->fd);
}

int main(int argc, char const *argv[]) {
  int fd;
  struct sockaddr_in address;
  socklen_t address_len = sizeof(address);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("error: socket failed\n");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(PORT);

  int opt = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("error: setsockopt failed\n");
    exit(EXIT_FAILURE);
  }

  if ((bind(fd, (struct sockaddr *)&address, address_len)) != 0) {
    perror("error: socket bind failed.\n");
    exit(EXIT_FAILURE);
  }

  if ((listen(fd, 5)) != 0) {
    perror("error: listening failed\n");
    exit(EXIT_FAILURE);
  } else {
    printf("server: listening on port %d\n", PORT);
  }

  struct player player1 =
      wait_for_player(1, fd, (struct sockaddr *)&address, &address_len);
  struct player player2 =
      wait_for_player(2, fd, (struct sockaddr *)&address, &address_len);

  printf("server: all players connected, sending setup packets\n");
  struct packet setup1 = new_packet(2, player2.name);
  write_packet(player1.fd, &setup1);
  struct packet setup2 = new_packet(2, player1.name);
  write_packet(player2.fd, &setup2);

  printf("server: waiting for placements\n");
  struct packet place1 = read_packet(player1.fd);
  struct packet place2 = read_packet(player2.fd);

  uint8_t ships1[5][3] = {0};
  uint8_t ships2[5][3] = {0};
  parse_placements(place1.data, ships1);
  parse_placements(place2.data, ships2);

  uint8_t board[10][10] = {0};
  render_placements(ships1, board);

  printf("server: player 1 placements:\n");
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      printf("%d ", board[j][i]);
    }
    printf("\n");
  }

  close_player(&player1);
  close_player(&player2);
  close(fd);
}