#include <stdbool.h>
#include <stdint.h>

#ifndef GAME_H
#define GAME_H

#define CARRIER 5
#define BATTLESHIP 4
#define CRUISER 3
#define SUBMARINE 3
#define DESTROYER 2

enum Player {
  PLAYER_NONE = -1,
  PLAYER1 = 1,
  PLAYER2 = 2,
};

enum TurnResult {
  MISS = 0,
  HIT = 1,
  SINK = 2,
};

extern const uint8_t SHIP_LENGTHS[5];
void render_placements(uint8_t placements[5][3], bool board[10][10]);

#endif