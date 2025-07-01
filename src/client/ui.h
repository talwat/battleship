#ifndef UI_H
#define UI_H

#include "game.h"
#include <stdbool.h>

#define ctrl(x) ((x) & 0x1f)

struct UI {
  WINDOW *main_win;
  WINDOW *side_win;
  WINDOW *lower_win;

  enum Tile board_data[10][10];
  enum Tile alt_board_data[10][10];

  int board_x;
  int board_y;

  int cursor_x;
  int cursor_y;
};

enum CursorResult {
  CURSOR_CONTINUE,
  CURSOR_QUIT,
  CURSOR_R,
  CURSOR_SELECT
};

bool select_server(char *username, char *address);
bool place_ships(struct UI *ui, struct ship ships[5]);
void lower_status(struct UI *ui, const char *status);
void init_ui(struct UI *ui);

void render_board(struct UI *ui, WINDOW *window, enum Tile board[10][10]);
void render_active_board(struct UI *ui);

enum CursorResult cursor_input(struct UI *ui, int input);
void move_cursor(struct UI *ui, WINDOW *window, int x, int y);

#endif
