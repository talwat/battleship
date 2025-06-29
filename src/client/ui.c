#include <form.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

#include "client/ui.h"
#include "game.h"

void remove_trailing_spaces(char *s, int len) {
  for (int i = len; i > 0; i--) {
    if (s[i] == '\0' || s[i] == ' ') {
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

void move_cursor(struct UI *ui, int x, int y) {
  wmove(ui->board_win, y + ui->board_y, (x * 2) + ui->board_x);
}

void init_ui(struct UI *ui) {
  empty_board(ui->board_data);
  ui->cursor_x = 0;
  ui->cursor_y = 0;

  int width, height;
  getmaxyx(stdscr, height, width);
  box(stdscr, 0, 0);

  const int sidebar_width = 31;
  ui->sidebar_win = subwin(stdscr, height, sidebar_width, 0, 0);
  box(ui->sidebar_win, 0, 0);

  int board_window_height = height - (height >> 1) + 1;
  int board_window_width = width - sidebar_width;
  int lower_window_height = (height >> 1) - 1;

  ui->board_win = newwin(board_window_height, board_window_width, 0, sidebar_width);
  ui->board_y = (board_window_height - 10) / 2;
  ui->board_x = (board_window_width - 20) / 2 + 1;
  box(ui->board_win, 0, 0);
  render_board_ui(ui);

  ui->lower_win = newwin(lower_window_height, width - sidebar_width, board_window_height, sidebar_width);
  box(ui->lower_win, 0, 0);

  refresh();

  move_cursor(ui, 0, 0);

  wrefresh(ui->board_win);
  wrefresh(ui->lower_win);
}

void render_board_ui(struct UI *ui) {
  for (int y = 0; y < 10; y++) {
    for (int x = 0; x < 10; x++) {
      move_cursor(ui, x, y);

      char c;
      switch (ui->board_data[x][y]) {
      case TILE_EMPTY:
        c = '.';
        break;
      case TILE_SHIP_HORIZONTAL:
        c = '-';
        break;
      case TILE_SHIP_VERTICAL:
        c = '|';
        break;
      case TILE_HIT:
        c = 'H';
        break;
      case TILE_MISS:
        c = 'M';
        break;
      }

      waddch(ui->board_win, c);
    }
  }
}

int cursor_input(struct UI *ui, int input) {
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
  case ctrl('c'):
    return 1;
  }

  wmove(ui->board_win, ui->cursor_y + ui->board_y, (ui->cursor_x * 2) + ui->board_x);
  return 0;
}
