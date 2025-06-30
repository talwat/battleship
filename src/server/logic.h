#ifndef LOGIC_H
#define LOGIC_H

#include "game.h"
#include "server/socket.h"

struct game_instance {
  enum Player current_player;
  struct ship ships1[5];
  struct ship ships2[5];
  struct client player1;
  struct client player2;
  enum Tile board1[10][10];
  enum Tile board2[10][10];
};

/**
 * Processes a player's turn by checking if the specified coordinates (x, y)
 * on the given board result in a hit or a miss.
 *
 * @param board A 10x10 boolean array representing the game board, where true indicates a ship is present.
 * @param x The x-coordinate (column) of the attack (0-based index).
 * @param y The y-coordinate (row) of the attack (0-based index).
 * @return HIT if a ship is present at the specified coordinates, otherwise MISS.
 */
enum TurnResult process_turn(struct ship ships[5], enum Tile board[10][10], int x, int y);

/**
 * @brief Main game loop for the Battleship server, handling player turns and game state.
 *
 * Specifically, this function alternates turns between the two players,
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
enum Player loop(struct game_instance *game);

/**
 * Sends setup packets to both players, waits for their ship placements,
 * parses the received placement data, and renders the ships onto the provided boards.
 *
 * @param player1 The first client/player structure.
 * @param player2 The second client/player structure.
 * @param board1  The 10x10 board for player 1, to be filled with ship placements.
 * @param board2  The 10x10 board for player 2, to be filled with ship placements.
 */
void get_placements(struct game_instance *game);

#endif