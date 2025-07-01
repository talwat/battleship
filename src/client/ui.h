#ifndef UI_H
#define UI_H

#include "game.h"
#include <stdbool.h>

#define ctrl(x) ((x) & 0x1f)

/**
 * @struct UI
 * @brief Holds all of the UI state, including windows, board data and the cursor position.
 */
struct UI {
  WINDOW *main_win;
  WINDOW *side_win;
  WINDOW *lower_win;

  enum Tile board_data[10][10];
  enum Tile alt_board_data[10][10];

  // The x coordinates of the board relative to the main window
  int board_x;

  // The y coordinates of the board relative to the main window
  int board_y;

  // The x coordinate of the cursor in the main board, from 0 to 9.
  int cursor_x;

  // The y coordinate of the cursor in the main board, from 0 to 9.
  int cursor_y;
};

/**
 * @enum CursorResult
 * @brief Represents the possible results of cursor input handling.
 */
enum CursorResult {
  CURSOR_CONTINUE,
  CURSOR_QUIT,
  CURSOR_R,
  CURSOR_SELECT
};

/**
 * @brief Prompts the user to select a server by entering their username and the server address.
 *
 * @param username A pointer to a character array where the username will be stored.
 * @param address A pointer to a character array where the server address will be stored.
 * @return true if the user successfully selected a server, false otherwise.
 */
bool select_server(char *username, char *address);

/**
 * @brief Places ships on the board by allowing the user to select positions and orientations.
 *
 * @param ui A pointer to the UI structure containing the game state.
 * @param ships An array of ships to be placed on the board.
 * @return true if all ships were successfully placed, false if the user cancelled.
 */
bool place_ships(struct UI *ui, struct ship ships[5]);

/**
 * @brief Clears the status line at the bottom of the UI and displays a new status message.
 *
 * @param ui A pointer to the UI structure containing the game state.
 * @param status The status message to display.
 */
void lower_status(struct UI *ui, const char *status);

/**
 * @brief Initializes the UI by setting up the main, side, and lower windows,
 *        and rendering the initial game board.
 *
 * @param ui A pointer to the UI structure to be initialized.
 */
void init_ui(struct UI *ui);

/**
 * @brief Renders the game board in the specified window.
 *
 * @param ui A pointer to the UI structure containing the game state.
 * @param window The window where the board will be rendered.
 * @param board The 10x10 array representing the game board to be rendered.
 */
void render_board(struct UI *ui, WINDOW *window, enum Tile board[10][10]);

/**
 * @brief Renders the active game board in the main window of the UI.
 *
 * This function is just a shorthand for rendering the main game board with main_win and board_data.
 *
 * @param ui A pointer to the UI structure containing the game state.
 */
void render_active_board(struct UI *ui);

/**
 * @brief Handles cursor input for navigating the game board.
 *
 * @param ui A pointer to the UI structure containing the game state.
 * @param input The input character from the user.
 * @return An enum CursorResult indicating the result of the input handling.
 */
enum CursorResult cursor_input(struct UI *ui, int input);

/**
 * @brief Moves the cursor to the specified coordinates in the given window.
 *
 * This function adjusts the cursor position based on the board's x and y offsets.
 *
 * @param ui A pointer to the UI structure containing the game state.
 * @param window The window where the cursor will be moved.
 * @param x The desired x coordinate of the cursor (0-9).
 * @param y The desired y coordinate of the cursor (0-9).
 */
void move_cursor(struct UI *ui, WINDOW *window, int x, int y);

#endif
