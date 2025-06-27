#include "game.h"

#define ctrl(x) ((x) & 0x1f)

bool select_server(char *username, char *address);
void init_main_ui(WINDOW **sidebar, WINDOW **board, WINDOW **lower, int *board_x, int *board_y);
void select_ship_placement(int *select_x, int *select_y, int index, WINDOW **board, int board_x, int board_y, struct ship *ships);