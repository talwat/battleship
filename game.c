#include "game.h"
#include <stdbool.h>
#include <stdint.h>

enum Orientation {
  HORIZONTAL = 0,
  VERTICAL = 1
};

const uint8_t SHIP_LENGTHS[5] = {CARRIER, BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};

void render_placements(uint8_t placements[5][3], bool board[10][10]) {
  for (int i = 0; i < 5; i++) {
    enum Orientation direction = placements[i][0];
    uint8_t x = placements[i][1];
    uint8_t y = placements[i][2];
    uint8_t length = SHIP_LENGTHS[i];

    for (int j = 0; j < length; j++) {
      if (direction == HORIZONTAL) {
        board[x + j][y] = true;
      } else {
        board[x][y + j] = true;
      }
    }
  }
}