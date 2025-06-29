#include "game.h"
#include <stdbool.h>

#define ctrl(x) ((x) & 0x1f)

struct UI {
    WINDOW *sidebar_win;
    WINDOW *board_win;
    WINDOW *lower_win;

    enum Tile board_data[10][10];

    int board_x;
    int board_y;

    int cursor_x;
    int cursor_y;
};

bool select_server(char *username, char *address);
void init_ui(struct UI *ui);
void render_board_ui(struct UI *ui);
int cursor_input(struct UI *ui, int input);
void move_cursor(struct UI *ui, int x, int y);
