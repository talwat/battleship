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

bool select_tile(struct UI *ui) {
  while (true) {
    render_board_ui(ui);

    move_cursor(ui, ui->cursor_x, ui->cursor_y);
    wrefresh(ui->board_win);

    enum CursorResult result = cursor_input(ui, getch());
    switch (result) {
    case CURSOR_QUIT:
      return false;
    case CURSOR_SELECT:
      wrefresh(ui->board_win);
      return true;
    }
  }
}

void quit(int fd) {
  struct packet quit = new_packet(QUIT, NULL);
  write_packet(fd, &quit);
  endwin();

  printf("client: quitting...\n");
  exit(EXIT_SUCCESS);
}

bool turn(struct UI *ui, int fd, int player_id) {
  render_board_ui(ui);
  wrefresh(ui->board_win);

  struct packet turn = read_packet(fd);
  if (turn.type == QUIT) {
    endwin();
    printf("client: requested quit, exiting...\n");
    return false;
  }

  if (turn.type != TURN) {
    endwin();
    printf("client: received unexpected packet type %d (%s)\n", turn.type, turn.name);
    printf("client: expected %d (TURN)\n", TURN);
    return false;
  }

  if (turn.data[0] == player_id) {
    curs_set(1);
    lower_status(ui, "Your turn! Select a target.");
    SpeakSAM(48, "SELECT A TARGET.");

    if (!select_tile(ui)) {
      quit(fd);
    };

    uint8_t target = merge(ui->cursor_x, ui->cursor_y);
    struct packet turn = new_packet(SELECT, &target);
    write_packet(fd, &turn);
  } else {
    curs_set(0);
    lower_status(ui, "Waiting for opponent to select a target.");
  }

  struct packet result = read_packet(fd);
  if (result.type != TURN_RESULT) {
    endwin();
    printf("client: received unexpected packet type %d (%s)\n", result.type, result.name);
    printf("client: expected %d (TURN_RESULT)\n", TURN_RESULT);
    return false;
  }

  enum Player turn_player = result.data[0];
  uint8_t target = result.data[1];
  uint8_t x = target & 0x0F;
  uint8_t y = (target >> 4) & 0x0F;

  if (turn_player == player_id) {
    switch (result.data[2]) {
    case TURN_WIN:
      ui->board_data[x][y] = TILE_HIT;
      render_board_ui(ui);
      move_cursor(ui, x, y);
      wrefresh(ui->board_win);
      SpeakSAM(48, "YOU WIN.");

      lower_status(ui, "You win! Press any key to continue.");
      getch();
      exit(EXIT_SUCCESS);
      return false;
    case TURN_SINK:
      SpeakSAM(48, "SINK.");
      ui->board_data[x][y] = TILE_HIT;
      break;
    case TURN_MISS:
      SpeakSAM(48, "MISS.");
      ui->board_data[x][y] = TILE_MISS;
      break;
    case TURN_HIT:
      SpeakSAM(48, "HIT.");
      ui->board_data[x][y] = TILE_HIT;
      break;
    }
  } else {
    switch (result.data[2]) {
    case TURN_WIN:
      SpeakSAM(48, "YOU LOSE.");
      lower_status(ui, "You lose! Press any key to continue.");
      getch();
      exit(EXIT_SUCCESS);
      return false;
    }
  }

  free_packet(&turn);
  free_packet(&result);

  return true;
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

  struct ship ships[5];
  if (!place_ships(&ui, ships)) {
    quit(fd);
  };

  curs_set(0);
  unsigned char data[10];
  serialize_placements(data, ships);

  struct packet place = new_packet(PLACE, data);
  write_packet(fd, &place);

  empty_board(ui.board_data);

  // clang-format off
  while (turn(&ui, fd, player_id));
}