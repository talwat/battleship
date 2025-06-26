#include <stdint.h>

#ifndef GAME_H
#define GAME_H

#define CARRIER 5
#define BATTLESHIP 4
#define CRUISER 3
#define SUBMARINE 3
#define DESTROYER 2

extern const uint8_t SHIP_LENGTHS[5];
void render_placements(uint8_t placements[5][3], uint8_t board[10][10]);

#endif