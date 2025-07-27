#include "server/logic.h"
#include "game.h"
#include "packet.h"
#include "server/socket.h"
#include "stdio.h"
#include <stdlib.h>

void quit(struct client *quit, struct client *other) {
  struct packet packet = new_packet(QUIT, NULL);

  printf("server: player %s quit\n", quit->name);
  write_packet(quit->fd, &packet); // This will probably not be read unless the server initiates the quit.
  close_player(quit);

  write_packet(other->fd, &packet);
  close_player(other);

  exit(EXIT_SUCCESS);
}

void render_board(enum Tile board[10][10]) {
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      switch (board[x][y]) {
      case TILE_EMPTY:
        printf(". ");
        break;
      case TILE_SHIP_HORIZONTAL:
        printf("- ");
        break;
      case TILE_SHIP_VERTICAL:
        printf("| ");
        break;
      case TILE_HIT:
        printf("H ");
        break;
      case TILE_MISS:
        printf("M ");
        break;
      }
    }
    printf("\n");
  }
}

enum TurnResult process_turn(struct ship ships[5], enum Tile board[10][10], int x, int y) {
  if (board[x][y] == TILE_SHIP_HORIZONTAL || board[x][y] == TILE_SHIP_VERTICAL) {
    board[x][y] = TILE_HIT;

    bool sunk = false;  // Whether the player sunk a ship this turn.
    int sink_count = 0; // Number of ships sunk this turn.

    for (int i = 0; i < 5; i++) {
      if (ships[i].sunk) {
        sink_count++;
        continue;
      }

      int hit_count = 0;
      for (int j = 0; j < SHIP_LENGTHS[i]; j++) {
        int x_pos = ships[i].x;
        int y_pos = ships[i].y;

        if (ships[i].orientation == HORIZONTAL) {
          x_pos += j;
        } else {
          y_pos += j;
        }

        if (board[x_pos][y_pos] == TILE_HIT) {
          hit_count++;
        }
      }

      if (hit_count == SHIP_LENGTHS[i]) {
        sink_count++;
        sunk = true;
        ships[i].sunk = true;
      } else {
        ships[i].sunk = false;
      }
    }

    if (sink_count == 5) {
      return TURN_WIN;
    }

    if (sunk) {
      return TURN_SINK;
    }

    return TURN_HIT;
  }

  if (board[x][y] == TILE_EMPTY) {
    board[x][y] = TILE_MISS;
    return TURN_MISS;
  }

  return TURN_MISS;
}

enum Player loop(struct game_instance *game) {
  enum Player winner = PLAYER_NONE;

  while (winner == PLAYER_NONE) {
    printf("player 1's board:\n");
    render_board(game->board1);
    printf("\nplayer 2's board:\n");
    render_board(game->board2);

    struct packet turn = new_packet(TURN, (unsigned char *)&game->current_player);
    write_packet(game->player1.fd, &turn);
    write_packet(game->player2.fd, &turn);

    struct packet select;
    printf("server: waiting for player %d to select a target\n", game->current_player);
    if (game->current_player == PLAYER1) {
      select = read_packet(game->player1.fd);
      if (select.type == QUIT)
        quit(&game->player1, &game->player2);
    } else {
      select = read_packet(game->player2.fd);
      if (select.type == QUIT)
        quit(&game->player2, &game->player1);
    }

    if (select.type != SELECT) {
      perror("error: received unexpected packet type\n");
      printf("expected SELECT (%d), got %d (%s)\n", SELECT, select.type, select.name);
      exit(EXIT_FAILURE);
      break;
    }

    uint8_t target = select.data[0];
    uint8_t x = target & 0x0F;
    uint8_t y = (target >> 4) & 0x0F;

    if (x >= 10 || y >= 10) {
      perror("error: invalid target selected\n");
      continue;
    }

    enum TurnResult result = TURN_MISS;

    if (game->current_player == PLAYER1) {
      result = process_turn(game->ships2, game->board2, x, y);
    } else {
      result = process_turn(game->ships1, game->board1, x, y);
    }

    const char *result_str;
    switch (result) {
    case TURN_MISS:
      result_str = "miss";
      break;
    case TURN_HIT:
      result_str = "hit";
      break;
    case TURN_SINK:
      result_str = "sink";
      break;
    case TURN_WIN:
      result_str = "win";
      break;
    }

    printf("server: player %d struck target (%d, %c) with result %d (%s)\n", game->current_player, x + 1, 'A' + y, result, result_str);

    uint8_t data[3] = {game->current_player, target, result};
    struct packet turn_result = new_packet(TURN_RESULT, data);
    write_packet(game->player1.fd, &turn_result);
    write_packet(game->player2.fd, &turn_result);

    if (result == TURN_WIN) {
      winner = game->current_player;
      break;
    }

    if (result != TURN_HIT && result != TURN_SINK) {
      if (game->current_player == PLAYER1) {
        game->current_player = PLAYER2;
      } else {
        game->current_player = PLAYER1;
      }
    }

    free_packet(&select);
  }

  return winner;
}

void get_placements(struct game_instance *game) {
  struct packet setup1 = new_packet(SETUP, (unsigned char *)game->player2.name);
  write_packet(game->player1.fd, &setup1);

  struct packet setup2 = new_packet(SETUP, (unsigned char *)game->player1.name);
  write_packet(game->player2.fd, &setup2);

  printf("server: waiting for placements\n");
  struct packet place1 = read_packet(game->player1.fd);
  if (place1.type == QUIT) {
    quit(&game->player1, &game->player2);
  }

  struct packet place2 = read_packet(game->player2.fd);
  if (place2.type == QUIT) {
    quit(&game->player2, &game->player1);
  }

  parse_placements(place1.data, game->ships1);
  parse_placements(place2.data, game->ships2);

  if (!render_placements(game->ships1, game->board1)) {
    printf("error: invalid ship placement for player 1\n");
    quit(&game->player1, &game->player2);
  }

  if (!render_placements(game->ships2, game->board2)) {
    printf("error: invalid ship placement for player 1\n");
    quit(&game->player2, &game->player1);
  }

  free_packet(&place1);
  free_packet(&place2);
}