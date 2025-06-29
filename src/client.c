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

#include "client/sam.h"
#include "client/ui.h"
#include "game.h"
#include "packet.h"
#include "shared.h"

// "Cruiser" and "submarine" were too hard for SAM to say...
const char* SHIP_NAMES[5] = {"CARRIER", "BATTLE SHIP", "CRUZER", "SUBMUHREEN", "DESTROYER"};

uint8_t merge(uint8_t lower, uint8_t upper) {
  return ((upper & 0x0F) << 4) | (lower & 0x0F);
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

int login(int fd, char *username, struct packet *data) {
  struct packet login = new_packet(LOGIN, (unsigned char *)username);
  write_packet(fd, &login);

  struct packet confirm = read_packet(fd);
  printf("client: received confirmation for player %d\n", confirm.data[0]);
  uint8_t player_id = confirm.data[0];

  printf("client: waiting for other player...\n");
  *data = read_packet(fd);
  return player_id;
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
  SpeakSAM(48, "SELECT A SERVER.");
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

  struct UI ui;
  init_ui(&ui);
  wrefresh(ui.board_win);

  SpeakSAM(48, "PLACE YOUR VESSELS.");

  enum Orientation orientation = HORIZONTAL;

  struct ship ships[5];
  for (int i = 0; i < 5; i++) {
    ships[i].defined = 0;
    while (!(ships[i].defined)) {
      render_board_ui(&ui);

      for (int j = 0; j < SHIP_LENGTHS[i]; j++) {
        if (orientation == HORIZONTAL) {
          move_cursor(&ui, ui.cursor_x + j, ui.cursor_y);
          waddch(ui.board_win, 'S');
        } else {
          move_cursor(&ui, ui.cursor_x, ui.cursor_y + j);
          waddch(ui.board_win, 'S');
        }
      }

      move_cursor(&ui, ui.cursor_x, ui.cursor_y);
      wrefresh(ui.board_win);

      enum CursorResult result = cursor_input(&ui, getch(), &orientation);
      switch (result) {
      case CURSOR_QUIT:
        exit(EXIT_SUCCESS);
        return 0;
      case CURSOR_SELECT:
        ships[i] = (struct ship){
            .coordinates = {},
            .defined = true,
            .orientation = HORIZONTAL,
            .sunk = false,
            .x = ui.cursor_x,
            .y = ui.cursor_y,
        };

        render_placements(ships, ui.board_data);
        wrefresh(ui.board_win);

        SpeakSAM(48, SHIP_NAMES[i]);
        break;
      }

      if (orientation == HORIZONTAL && ui.cursor_x > 10 - SHIP_LENGTHS[i])
        ui.cursor_x = 10 - SHIP_LENGTHS[i];
      
      if (orientation == VERTICAL && ui.cursor_y > 10 - SHIP_LENGTHS[i])
        ui.cursor_y = 10 - SHIP_LENGTHS[i];
    }

    render_board_ui(&ui);
    move_cursor(&ui, ui.cursor_x, ui.cursor_y);
    wrefresh(ui.board_win);
  }

  return 0;
}