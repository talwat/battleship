#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"

#define PORT 8080

/**
 * @struct client
 * @brief Represents a connected client in the Battleship server.
 */
struct client {
  uint8_t id;
  char *name;
  int fd;
};

/**
 * Waits for a client to connect to the server, performs login handshake, and returns a populated client structure.
 *
 * @param id         The unique identifier to assign to the connecting client.
 * @param fd         The file descriptor of the listening socket.
 * @param address    Pointer to a sockaddr structure to receive the client's address.
 * @param address_len Pointer to a socklen_t variable containing the size of the address structure.
 * @return           A struct client containing the client's id, connection file descriptor, and name.
 *
 * This function blocks until a client connects, reads the login packet to obtain the client's name,
 * sends a login confirmation packet, and prints a message indicating the connection.
 * On failure to accept a connection, the function prints an error and exits the program.
 */
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

/**
 * Processes a player's turn by checking if the specified coordinates (x, y)
 * on the given board result in a hit or a miss.
 *
 * @param board A 10x10 boolean array representing the game board, where true indicates a ship is present.
 * @param x The x-coordinate (column) of the attack (0-based index).
 * @param y The y-coordinate (row) of the attack (0-based index).
 * @return HIT if a ship is present at the specified coordinates, otherwise MISS.
 */
enum TurnResult process_turn(bool board[10][10], int x, int y) {
  if (board[y][x]) {
    return HIT;
  }
  return MISS;
}

/**
 * @brief Main game loop for the Battleship server, handling player turns and game state.
 *
 * Specifically, this function alternates turns between two players,
 * sending and receiving packets to/from clients, processing their moves,
 * updating the game boards, and broadcasting turn results. The loop continues
 * until a winner is determined.
 *
 * @param current_player Pointer to the enum indicating the current player (PLAYER1 or PLAYER2).
 * @param player1        Struct representing the first client/player.
 * @param player2        Struct representing the second client/player.
 * @param board1         10x10 boolean array representing player 1's board.
 * @param board2         10x10 boolean array representing player 2's board.
 * @return enum Player   The winner of the game (PLAYER1 or PLAYER2).
 */
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

/**
 * Sends setup packets to both players, waits for their ship placements,
 * parses the received placement data, and renders the ships onto the provided boards.
 *
 * @param player1 The first client/player structure.
 * @param player2 The second client/player structure.
 * @param board1  The 10x10 board for player 1, to be filled with ship placements.
 * @param board2  The 10x10 board for player 2, to be filled with ship placements.
 */
void get_placements(struct client player1, struct client player2, bool board1[10][10], bool board2[10][10]) {
  struct packet setup1 = new_packet(SETUP, (unsigned char *)player2.name);
  write_packet(player1.fd, &setup1);
  struct packet setup2 = new_packet(SETUP, (unsigned char *)player1.name);
  write_packet(player2.fd, &setup2);

  printf("server: waiting for placements\n");
  struct packet place1 = read_packet(player1.fd);
  struct packet place2 = read_packet(player2.fd);

  struct ship ships1[5] = {0};
  struct ship ships2[5] = {0};
  parse_placements(place1.data, ships1);
  parse_placements(place2.data, ships2);

  render_placements(ships1, board1);
  render_placements(ships1, board2);
}

/**
 * Closes the connection for a given player.
 *
 * @param player Pointer to the client structure representing the player whose connection should be closed.
 */
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