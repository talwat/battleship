#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"

#define PORT 8080

struct client {
  uint8_t id;
  char *name;
  int fd;
};

struct client wait_for_client(uint8_t id, int fd, struct sockaddr *address, socklen_t *address_len) {
  int connection = accept(fd, address, address_len);
  if (connection < 0) {
    perror("error: failed to accept connection\n");
    exit(EXIT_FAILURE);
  }

  struct packet login = read_packet(connection);
  char *name = (char *)login.data;

  struct packet confirm = new_packet(LOGIN_CONFIRM, &id);
  write_packet(connection, &confirm);

  struct client result = {
      .id = id,
      .fd = connection,
      .name = name,
  };

  printf("server: player %d (%s) connected\n", result.id, result.name);

  return result;
}

enum TurnResult process_turn(bool board[10][10], int x, int y) {
  if (board[y][x]) {
    return HIT;
  }
  return MISS;
}

enum Player loop(enum Player *current_player, struct client player1, struct client player2, bool board1[10][10], bool board2[10][10]) {
  enum Player winner = PLAYER_NONE;

  while (winner == PLAYER_NONE) {
    struct packet turn = new_packet(TURN, (unsigned char *)current_player);
    write_packet(player1.fd, &turn);
    write_packet(player2.fd, &turn);

    struct packet select;
    printf("server: waiting for player %d to select a target\n", *current_player);
    if (*current_player == PLAYER1) {
      select = read_packet(player1.fd);
    } else {
      select = read_packet(player2.fd);
    }

    uint8_t target = select.data[0];
    uint8_t x = target & 0x0F;
    uint8_t y = (target >> 4) & 0x0F;

    if (x >= 10 || y >= 10) {
      perror("error: invalid target selected\n");
      continue;
    }

    enum TurnResult result = MISS;

    printf("server: player %d selected target (%d, %d)\n", *current_player, x, y);
    if (*current_player == PLAYER1) {
      result = process_turn(board2, x, y);
    } else {
      result = process_turn(board1, x, y);
    }

    uint8_t data[3] = {*current_player, target, result};
    struct packet turn_result = new_packet(TURN_RESULT, data);
    write_packet(player1.fd, &turn_result);
    write_packet(player2.fd, &turn_result);

    if (result != HIT) {
      if (*current_player == PLAYER1) {
        *current_player = PLAYER2;
      } else {
        *current_player = PLAYER1;
      }
    }
  }

  return winner;
}

void get_placements(struct client player1, struct client player2, bool board1[10][10], bool board2[10][10]) {
  struct packet setup1 = new_packet(SETUP, (unsigned char *)player2.name);
  write_packet(player1.fd, &setup1);
  struct packet setup2 = new_packet(SETUP, (unsigned char *)player1.name);
  write_packet(player2.fd, &setup2);

  printf("server: waiting for placements\n");
  struct packet place1 = read_packet(player1.fd);
  struct packet place2 = read_packet(player2.fd);

  uint8_t ships1[5][3] = {0};
  uint8_t ships2[5][3] = {0};
  parse_placements(place1.data, ships1);
  parse_placements(place2.data, ships2);

  render_placements(ships1, board1);
  render_placements(ships1, board2);
}

void close_player(struct client *player) {
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

  struct client player1 =
      wait_for_client(1, fd, (struct sockaddr *)&address, &address_len);
  struct client player2 =
      wait_for_client(2, fd, (struct sockaddr *)&address, &address_len);

  printf("server: all players connected, sending setup packets\n");
  bool board1[10][10] = {0};
  bool board2[10][10] = {0};
  get_placements(player1, player2, board1, board2);

  enum Player current_player = PLAYER1;
  enum Player winner = PLAYER_NONE;

  while (winner == PLAYER_NONE) {
    winner = loop(&current_player, player1, player2, board1, board2);
  }

  printf("server: player %d wins!\n", winner);
  struct packet end_packet = new_packet(END, (unsigned char *)&winner);
  write_packet(player1.fd, &end_packet);
  write_packet(player2.fd, &end_packet);

  close_player(&player1);
  close_player(&player2);
  close(fd);
}