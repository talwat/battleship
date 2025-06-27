#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"
#include "server/logic.h"
#include "server/socket.h"
#include "shared.h"

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

  struct client player1 =
      wait_for_client(1, fd, (struct sockaddr *)&address, &address_len);
  struct client player2 =
      wait_for_client(2, fd, (struct sockaddr *)&address, &address_len);

  printf("server: all players connected, sending setup packets\n");

  struct game_instance game = {
      .current_player = PLAYER1,
      .ships1 = {},
      .ships2 = {},
      .player1 = player1,
      .player2 = player2,
      .board1 = {{0}},
      .board2 = {{0}},
  };

  get_placements(&game);

  enum Player winner = PLAYER_NONE;

  while (winner == PLAYER_NONE) {
    winner = loop(game);
  }

  printf("server: player %d wins!\n", winner);
  struct packet end_packet = new_packet(END, (unsigned char *)&winner);
  write_packet(player1.fd, &end_packet);
  write_packet(player2.fd, &end_packet);

  close_player(&player1);
  close_player(&player2);
  close(fd);
}