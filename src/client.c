#include <SDL.h>
#include <SDL_audio.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <curses.h>
#include <form.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "game.h"
#include "packet.h"
#include "sam.h"
#include "shared.h"

#define ctrl(x) ((x) & 0x1f)

uint8_t merge(uint8_t lower, uint8_t upper) {
  return ((upper & 0x0F) << 4) | (lower & 0x0F);
}

void remove_trailing_spaces(char *s, int len) {
  for (int i = len; i > 0; i--) {
    if (s[i] == '\0' || s[i] == ' ') {
      s[i] = '\0';
    } else {
      break;
    }
  }
}

void serialize_placements(unsigned char *data, struct ship *ships) {
  for (int i = 0; i < 5; i++) {
    uint8_t direction = ships[i].orientation;
    uint8_t x = ships[i].x;
    uint8_t y = ships[i].y;

    uint8_t position = merge(x, y);

    data[i * 2] = direction;
    data[i * 2 + 1] = position;
  }
}

int init_connection(char *ip) {
  int fd;
  struct sockaddr_in address;
  socklen_t address_len = sizeof(address);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("error: socket failed\n");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(PORT);

  if (connect(fd, (struct sockaddr *)&address, address_len) < 0) {
    perror("error: connection failed\n");
    exit(EXIT_FAILURE);
  } else {
    printf("client: connected to server on port %d\n", PORT);
  }

  return fd;
}

void endwin_void() {
  endwin();
}

// Whether to continue running the program.
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
}

int login(int fd, char *username, struct packet *data) {
  struct packet login = new_packet(LOGIN, username);
  write_packet(fd, &login);

  struct packet confirm = read_packet(fd);
  printf("client: received confirmation for player %d\n", confirm.data[0]);
  uint8_t player_id = confirm.data[0];

  printf("client: waiting for other player...\n");
  *data = read_packet(fd);
  return player_id;
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

int main() {
  InitSAMAudio();
  atexit(CloseSAMAudio);
  atexit(endwin_void);

  // Initialize ncurses
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(1);
  
  char username[16];
  char address[16];
  SpeakSAM(48, "HI.");
  if (!select_server(username, address)) {
    exit(0);
  }
  SpeakSAM(48, "SERVER SELECTED.");
  def_prog_mode();
  endwin();

  int fd = init_connection(address);
  struct packet setup;
  int player_id = login(fd, username, &setup);

  unsigned char *opponent_name = setup.data;
  printf("client: opponent name is %s\n", opponent_name);
  
  reset_prog_mode();
  
  WINDOW *sidebar, *board, *lower;
  int board_x, board_y;
  init_main_ui(&sidebar, &board, &lower, &board_x, &board_y);
  SpeakSAM(48, "PLACE YOUR SHIPS.");
  
  // Move cursor to the top-left of the board area
  wmove(board, board_y, board_x);
  wrefresh(lower);
  wrefresh(board);

  int select_x = 0, select_y = 0;
  struct ship ships[5] = {};

  for (int i = 0; i < 5; i++) {
    select_ship_placement(&select_x, &select_y, i, &board, board_x, board_y, ships);
  }

  unsigned char data[10] = {0};
  serialize_placements(data, ships);

  def_prog_mode();
  endwin();

  struct packet place = new_packet(PLACE, data);
  write_packet(fd, &place);

  while (true) {
    struct packet turn = read_packet(fd);
    if (turn.type == END) {
      printf("client: game over\n");
      break;
    }

    if (*turn.data == player_id) {
      printf("client: it's my turn!\n");

      int x;
      printf("client: select a target x (1-10): ");
      scanf("%d", &x);

      char y;
      printf("client: select a target y (A-J): ");
      scanf(" %c", &y);

      unsigned char position = merge(x - 1, toupper(y) - 'A');
      struct packet select = new_packet(SELECT, &position);
      write_packet(fd, &select);
    } else {
      printf("client: waiting for opponent's turn...\n");
    }

    struct packet turn_result = read_packet(fd);
    enum TurnResult result = turn_result.data[2];

    if (result == TURN_HIT) {
      printf("client: hit!\n");
    } else if (result == TURN_MISS) {
      printf("client: miss!\n");
    } else if (result == TURN_SINK) {
      printf("client: sink!\n");
    } else if (result == TURN_WIN) {
      printf("client: win!\n");
      break;
    } else {
      printf("client: unknown turn result %d\n", result);
    }
  }

  return 0;
}