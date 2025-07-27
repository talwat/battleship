#include "game.h"
#include "packet.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const uint8_t SHIP_LENGTHS[5] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};

void empty_board(enum Tile board[10][10]) {
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
      board[x][y] = TILE_EMPTY;
    }
  }
}

bool render_placements(struct ship ships[5], enum Tile board[10][10]) {
  empty_board(board);

  for (int i = 0; i < 5; i++) {
    if (!ships[i].defined) {
      break;
    }

    if (!validate_ship(&ships[i], i, board)) {
      return false;
    }

    uint8_t length = SHIP_LENGTHS[i];
    uint8_t x = ships[i].x;
    uint8_t y = ships[i].y;

    for (int j = 0; j < length; j++) {
      if (ships[i].orientation == HORIZONTAL) {
        board[x + j][y] = TILE_SHIP_HORIZONTAL;
      } else {
        board[x][y + j] = TILE_SHIP_VERTICAL;
      }
    }
  }

  return true;
}
