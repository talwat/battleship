#include "game.h"
#include <stdbool.h>
#include <stdint.h>

const uint8_t SHIP_LENGTHS[5] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};

void render_placements(struct ship placements[5], bool board[10][10]) {
  for (int i = 0; i < 5; i++) {
    uint8_t length = SHIP_LENGTHS[i];
    uint8_t x = placements[i].x;
    uint8_t y = placements[i].y;

    for (int j = 0; j < length; j++) {
      if (placements[i].orientation == HORIZONTAL) {
        board[x + j][y] = true;
      } else {
        board[x][y + j] = true;
      }
    }
  }
}