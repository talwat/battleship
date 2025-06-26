#include <stdbool.h>
#include <stdint.h>

#ifndef GAME_H
#define GAME_H

#define CARRIER 5
#define BATTLESHIP 4
#define CRUISER 3
#define SUBMARINE 3
#define DESTROYER 2

/**
 * @enum Player
 * @brief Represents the players in the Battleship game.
 */
enum Player {
  PLAYER_NONE = -1,
  PLAYER1 = 1,
  PLAYER2 = 2,
};

/**
 * @enum TurnResult
 * @brief Represents the possible outcomes of a player's turn in the Battleship game.
 */
enum TurnResult {
  MISS = 0,
  HIT = 1,
  SINK = 2,
};

extern const uint8_t SHIP_LENGTHS[5];

/**
 * @enum Orientation
 * @brief Represents the orientation of a ship on the board.
 */
enum Orientation {
  HORIZONTAL = 0,
  VERTICAL = 1
};

/**
 * @struct ship
 * @brief Represents a ship's position and orientation on the Battleship game board.
 *
 * Holds the orientation and starting coordinates of a ship.
 * Coordinates are based on the leftmost or topmost cell of the ship's placement.
 */
struct ship {
  enum Orientation orientation;
  uint8_t x;
  uint8_t y;
};

/**
 * @brief Renders ship placements onto the game board.
 *
 * Takes an array of ship placements and marks their positions on the provided game board.
 * Each placement specifies the orientation, starting coordinates, and the ship's length is determined
 * by the `SHIP_LENGTHS` array. Ships are placed either horizontally or vertically on the board.
 *
 * @param placements An array of ship structures representing the placements of ships.
 * @param board A 10x10 boolean array representing the game board. Cells occupied by ships are set to true.
 */
void render_placements(struct ship placements[5], bool board[10][10]);

#endif