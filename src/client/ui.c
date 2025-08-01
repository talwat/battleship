#include <form.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "client/ui.h"
#include "game.h"
#include "sam.h"

// "Cruiser" and "submarine" were too hard for SAM to say...
const char *PHONETIC_SHIP_NAMES[5] = {"CARRIER", "BATTLE SHIP", "CRUZER", "SUBMUHREEN", "DESTROYER"};
const char *SHIP_NAMES[5] = {"Carrier", "Battleship", "Cruiser", "Submarine", "Destroyer"};

void remove_trailing_spaces(char *s, int len) {
  for (int i = len - 1; i >= 0; i--) {
    if (s[i] == ' ' || s[i] == '\0') {
      s[i] = '\0';
    } else {
      break;
    }
  }
}

bool select_server(char *username, char *address) {
  int width, height;
  getmaxyx(stdscr, height, width);

  int win_height = 10;
  int win_width = 32;
  int win_y = (height - win_height) / 2;
  int win_x = (width - win_width) / 2;
  WINDOW *win = newwin(win_height, win_width, win_y, win_x);
  keypad(win, TRUE);
  box(win, 0, 0);

  // Create fields
  FIELD *field[3];
  FORM *setup;
  int ch;

  field[0] = new_field(1, 16, 1, 12, 0, 0);
  field[1] = new_field(1, 14, 2, 14, 0, 0);
  set_field_buffer(field[1], 0, "127.0.0.1");

  field[2] = NULL;

  field_buffer(field[0], 0);

  set_field_back(field[0], A_UNDERLINE);
  field_opts_off(field[0], O_AUTOSKIP);

  set_field_back(field[1], A_UNDERLINE);
  field_opts_off(field[1], O_AUTOSKIP);

  setup = new_form(field);
  set_form_win(setup, win);
  set_form_sub(setup, derwin(win, win_height - 4, win_width - 4, 2, 1));

  post_form(setup);
  mvwprintw(win, 1, 2, "battleship client v0.1");
  mvwprintw(win, 3, 3, "Username:");
  mvwprintw(win, 4, 3, "IP Address:");
  mvwprintw(win, 6, 3, "CTRL+C to exit");
  mvwprintw(win, 7, 3, "ENTER to submit");

  wmove(win, 3, 13);
  wrefresh(win);

  while ((ch = wgetch(win)) != ctrl('c')) {
    switch (ch) {
    case KEY_DOWN:
      form_driver(setup, REQ_NEXT_FIELD);
      form_driver(setup, REQ_END_LINE);
      break;
    case KEY_ENTER:
    case 10:
      form_driver(setup, REQ_NEXT_FIELD);
      form_driver(setup, REQ_END_LINE);

      strncpy(username, field_buffer(field[0], 0), 16);
      remove_trailing_spaces(username, 16);
      strncpy(address, field_buffer(field[1], 0), 16);
      remove_trailing_spaces(address, 16);

      unpost_form(setup);
      free_form(setup);
      free_field(field[0]);
      free_field(field[1]);
      delwin(win);
      return true;
    case KEY_UP:
      form_driver(setup, REQ_PREV_FIELD);
      form_driver(setup, REQ_END_LINE);
      break;
    case KEY_BACKSPACE:
    case 127:
      form_driver(setup, REQ_DEL_PREV);
      break;
    case KEY_LEFT:
      form_driver(setup, REQ_PREV_CHAR);
      break;
    case KEY_RIGHT:
      form_driver(setup, REQ_NEXT_CHAR);
      break;
    case ctrl('c'):
      exit(0);
      return false;
    default:
      form_driver(setup, ch);
      break;
    }
    wrefresh(win);
  }

  unpost_form(setup);
  free_form(setup);
  free_field(field[0]);
  free_field(field[1]);
  delwin(win);

  return true;
}

void lower_status(struct UI *ui, const char *status) {
  werase(ui->lower_win);
  box(ui->lower_win, 0, 0);

  int y = 1;

  const char *line = status;
  const char *next;
  while (line && *line) {
    next = strchr(line, '\n');
    if (next) {
      mvwprintw(ui->lower_win, y++, 2, "%.*s", (int)(next - line), line);
      line = next + 1;
    } else {
      mvwprintw(ui->lower_win, y++, 2, "%s", line);
      break;
    }
  }

  wrefresh(ui->lower_win);
}

bool place_ships(struct UI *ui, struct ship ships[5]) {
  lower_status(ui, "Place your vessels.\n\nUse arrow keys to move, R to rotate, and ENTER to place a vessel.");
  enum Orientation orientation = HORIZONTAL;

  for (int i = 0; i < 5; i++) {
    wmove(ui->side_win, 1, 2);
    wprintw(ui->side_win, "Ships:");
    for (int j = 0; j < 5; j++) {
      if (i == j) {
        wattron(ui->side_win, A_BOLD);
      }

      wmove(ui->side_win, j + 2, 4);
      wprintw(ui->side_win, "%s: %d", SHIP_NAMES[j], SHIP_LENGTHS[j]);

      if (i == j) {
        wattroff(ui->side_win, A_BOLD);
      }
    }
    wrefresh(ui->side_win);

    ships[i].defined = 0;
    while (!(ships[i].defined)) {
      render_active_board(ui);

      for (int j = 0; j < SHIP_LENGTHS[i]; j++) {
        if (orientation == HORIZONTAL) {
          move_cursor(ui, ui->main_win, ui->cursor_x + j, ui->cursor_y);
          waddch(ui->main_win, 'S');
        } else {
          move_cursor(ui, ui->main_win, ui->cursor_x, ui->cursor_y + j);
          waddch(ui->main_win, 'S');
        }
      }

      move_cursor(ui, ui->main_win, ui->cursor_x, ui->cursor_y);
      wrefresh(ui->main_win);

      enum CursorResult result = cursor_input(ui, getch());
      switch (result) {
      case CURSOR_QUIT:
        return false;
      case CURSOR_R:
        if (orientation == HORIZONTAL)
          orientation = VERTICAL;
        else
          orientation = HORIZONTAL;
        break;
      case CURSOR_SELECT:
        bool valid = true;
        for (int j = 0; j < SHIP_LENGTHS[i]; j++) {
          if ((orientation == HORIZONTAL && ui->board_data[ui->cursor_x + j][ui->cursor_y] != TILE_EMPTY) ||
              (orientation == VERTICAL && ui->board_data[ui->cursor_x][ui->cursor_y + j] != TILE_EMPTY)) {
            valid = false;
            break;
          }
        }

        if (valid == false) {
          continue;
        }

        ships[i] = (struct ship){
            .defined = true,
            .orientation = orientation,
            .sunk = false,
            .x = ui->cursor_x,
            .y = ui->cursor_y,
        };

        render_placements(ships, ui->board_data);
        wrefresh(ui->main_win);

        SpeakSAM(48, (char *)PHONETIC_SHIP_NAMES[i]);
        break;
      case CURSOR_CONTINUE:
        break;
      }

      if (orientation == HORIZONTAL && ui->cursor_x > 10 - SHIP_LENGTHS[i])
        ui->cursor_x = 10 - SHIP_LENGTHS[i];

      if (orientation == VERTICAL && ui->cursor_y > 10 - SHIP_LENGTHS[i])
        ui->cursor_y = 10 - SHIP_LENGTHS[i];
    }

    render_active_board(ui);
    move_cursor(ui, ui->main_win, ui->cursor_x, ui->cursor_y);
    wrefresh(ui->main_win);
  }

  lower_status(ui, "Waiting for the opponent to place their vessels.");

  return true;
}

void move_cursor(struct UI *ui, WINDOW *window, int x, int y) {
  wmove(window, y + ui->board_y, (x * 2) + ui->board_x);
}

void init_ui(struct UI *ui) {
  empty_board(ui->board_data);
  ui->cursor_x = 0;
  ui->cursor_y = 0;

  int width, height;
  getmaxyx(stdscr, height, width);

  int lower_window_height = (height >> 1) - 4;
  int board_window_height = height - lower_window_height;
  int board_window_width = width >> 1;

  ui->main_win = newwin(board_window_height, board_window_width, 0, width - board_window_width);
  ui->board_y = (board_window_height - 10) / 2;
  ui->board_x = (board_window_width - 20) / 2 + 1;

  wattron(ui->main_win, COLOR_PAIR(1));
  box(ui->main_win, 0, 0);
  render_active_board(ui);

  ui->side_win = newwin(board_window_height, board_window_width, 0, 0);
  wattron(ui->side_win, COLOR_PAIR(1));
  box(ui->side_win, 0, 0);

  ui->lower_win = newwin(lower_window_height, width, board_window_height, 0);
  wattron(ui->lower_win, COLOR_PAIR(1));
  box(ui->lower_win, 0, 0);

  refresh();

  move_cursor(ui, ui->main_win, ui->cursor_x, ui->cursor_y);

  wrefresh(ui->main_win);
  wrefresh(ui->lower_win);
  wrefresh(ui->side_win);
}

void render_active_board(struct UI *ui) {
  render_board(ui, ui->main_win, ui->board_data);
}

void render_board(struct UI *ui, WINDOW *window, enum Tile board[10][10]) {
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      move_cursor(ui, window, x, y);

      int c;
      switch (board[x][y]) {
      case TILE_EMPTY:
        c = '.';
        break;
      case TILE_SHIP_HORIZONTAL:
        c = '-' | A_BOLD;
        break;
      case TILE_SHIP_VERTICAL:
        c = '|' | A_BOLD;
        break;
      case TILE_HIT:
        c = 'H';
        break;
      case TILE_MISS:
        c = 'M';
        break;
      }

      waddch(window, c);
    }
  }
}

enum CursorResult cursor_input(struct UI *ui, int input) {
  switch (input) {
  case KEY_UP:
    if (ui->cursor_y > 0)
      ui->cursor_y--;
    break;
  case KEY_DOWN:
    if (ui->cursor_y < 9)
      ui->cursor_y++;
    break;
  case KEY_LEFT:
    if (ui->cursor_x > 0)
      ui->cursor_x--;
    break;
  case KEY_RIGHT:
    if (ui->cursor_x < 9)
      ui->cursor_x++;
    break;
  case 'r':
  case 'R':
    return CURSOR_R;
  case ctrl('c'):
    return CURSOR_QUIT;
  case KEY_ENTER:
  case 10:
    return CURSOR_SELECT;
  }

  return CURSOR_CONTINUE;
}
