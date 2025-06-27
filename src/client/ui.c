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

void init_main_ui(WINDOW **sidebar, WINDOW **board, WINDOW **lower, int *board_x, int *board_y) {
  int width, height;
  getmaxyx(stdscr, height, width);
  box(stdscr, 0, 0);

  const int sidebar_width = 31;
  *sidebar = subwin(stdscr, height, sidebar_width, 0, 0);
  box(*sidebar, 0, 0);

  int board_window_height = height - (height >> 1) + 1;
  int board_window_width = width - sidebar_width;
  int lower_window_height = (height >> 1) - 1;

  *board = newwin(board_window_height, board_window_width, 0, sidebar_width);
  *board_y = (board_window_height - 10) / 2;
  *board_x = (board_window_width - 20) / 2 + 1;
  box(*board, 0, 0);

  *lower = newwin(lower_window_height, width - sidebar_width, board_window_height, sidebar_width);
  box(*lower, 0, 0);

  refresh();
}

void select_ship_placement(int *select_x, int *select_y, int index, WINDOW **board, int board_x, int board_y, struct ship *ships) {
  int orientation = HORIZONTAL;
  int length = SHIP_LENGTHS[index];

  do {
    // Draw plain board.
    for (int y = 0; y < 10; y++) {
      for (int x = 0; x < 10; x++) {
        wmove(*board, y + board_y, (x * 2) + board_x);
        waddch(*board, '.');
      }
    }

    // Draw on top already placed ships.
    for (int k = 0; k < index; k++) {
      uint8_t length_k = SHIP_LENGTHS[k];
      uint8_t x = ships[k].x;
      uint8_t y = ships[k].y;
      int orientation_k = ships[k].orientation;

      for (int l = 0; l < length_k; l++) {
        int draw_x = x + (orientation_k == HORIZONTAL ? l : 0);
        int draw_y = y + (orientation_k == VERTICAL ? l : 0);
        wmove(*board, board_y + draw_y, board_x + (draw_x * 2));
        waddch(*board, 'S');
      }
    }

    // Draw ship preview
    for (int i = 0; i < length; i++) {
      int draw_x = *select_x + (orientation == HORIZONTAL ? i : 0);
      int draw_y = *select_y + (orientation == VERTICAL ? i : 0);
      wmove(*board, board_y + draw_y, board_x + (draw_x * 2));

      waddch(*board, 'S' | A_BOLD);
    }

    wmove(*board, board_y + *select_y, board_x + (*select_x * 2));
    wrefresh(*board);

    int ch = getch();
    switch (ch) {
    case KEY_UP:
      if (orientation == HORIZONTAL) {
        if (*select_y > 0)
          (*select_y)--;
      } else {
        if (*select_y > 0)
          (*select_y)--;
      }
      break;
    case KEY_DOWN:
      if (orientation == HORIZONTAL) {
        if (*select_y < 9)
          (*select_y)++;
      } else {
        if (*select_y < 10 - length)
          (*select_y)++;
      }
      break;
    case KEY_LEFT:
      if (orientation == HORIZONTAL) {
        if (*select_x > 0)
          (*select_x)--;
      } else {
        if (*select_x > 0)
          (*select_x)--;
      }
      break;
    case KEY_RIGHT:
      if (orientation == HORIZONTAL) {
        if (*select_x < 10 - length)
          (*select_x)++;
      } else {
        if (*select_x < 9)
          (*select_x)++;
      }
      break;
    case 'r':
    case 'R':
      orientation = (orientation == HORIZONTAL) ? VERTICAL : HORIZONTAL;
      // Clamp position if out of bounds after rotation
      if (orientation == HORIZONTAL) {
        if (*select_x > 10 - length)
          *select_x = 10 - length;
        if (*select_y > 9)
          *select_y = 9;
      } else {
        if (*select_y > 10 - length)
          *select_y = 10 - length;
        if (*select_x > 9)
          *select_x = 9;
      }
      break;
    case KEY_ENTER:
    case 10:
    case 27:
      ships[index] = (struct ship){
          .x = *select_x,
          .y = *select_y,
          .orientation = orientation,
          .sunk = false,
      };
      return;
    case ctrl('c'):
      exit(0);
      break;
    }
  } while (1);
}