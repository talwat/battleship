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

  WINDOW *sidebar, *board, *lower;
  int board_x, board_y;
  init_main_ui(&sidebar, &board, &lower, &board_x, &board_y);
  SpeakSAM(48, "PLACE YOUR VESSELS.");
  sleep(2);

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

  // while (true) {
  //   struct packet turn = read_packet(fd);
  //   if (turn.type == QUIT) {
  //     printf("client: quit\n");
  //     break;
  //   }

  //   if (*turn.data == player_id) {
  //     printf("client: it's my turn!\n");

  //     int x;
  //     printf("client: select a target x (1-10): ");
  //     scanf("%d", &x);

  //     char y;
  //     printf("client: select a target y (A-J): ");
  //     scanf(" %c", &y);

  //     unsigned char position = merge(x - 1, toupper(y) - 'A');
  //     struct packet select = new_packet(SELECT, &position);
  //     write_packet(fd, &select);
  //   } else {
  //     printf("client: waiting for opponent's turn...\n");
  //   }

  //   struct packet turn_result = read_packet(fd);
  //   enum TurnResult result = turn_result.data[2];

  //   if (result == TURN_HIT) {
  //     SpeakSAM(48, "HIT.");
  //     printf("client: hit!\n");
  //   } else if (result == TURN_MISS) {
  //     SpeakSAM(48, "MISS.");
  //     printf("client: miss!\n");
  //   } else if (result == TURN_SINK) {
  //     SpeakSAM(48, "SINK.");
  //     printf("client: sink!\n");
  //   } else if (result == TURN_WIN) {
  //     SpeakSAM(48, "WIN.");
  //     printf("client: win!\n");
  //     break;
  //   } else {
  //     printf("client: unknown turn result %d\n", result);
  //   }
  // }

  return 0;
}