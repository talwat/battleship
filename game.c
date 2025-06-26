#include "game.h"

const uint8_t SHIP_LENGTHS[5] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};

void render_placements(uint8_t placements[5][3], uint8_t board[10][10]) {
  for (int i = 0; i < 5; i++) {
    uint8_t direction = placements[i][0];
    uint8_t x = placements[i][1];
    uint8_t y = placements[i][2];
    uint8_t length = SHIP_LENGTHS[i];

    for (int j = 0; j < length; j++) {
      if (direction == 0) {
        board[x + j][y] = 1;
      } else {
        board[x][y + j] = 1;
      }
    }
  }
}