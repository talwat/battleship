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

int speak_text(const char *text) {
  char input[256] = {0};
  strncpy(input, text, 255);

  if (!TextToPhonemes((unsigned char *)input))
    return 1;

  SetInput(input);
  SAMMain();
  OutputSound();

  return 0;
}

void serialize_placements(unsigned char *data, uint8_t placements[5][3]) {
  for (int i = 0; i < 5; i++) {
    uint8_t direction = placements[i][0];
    uint8_t x = placements[i][1];
    uint8_t y = placements[i][2];

    uint8_t position = merge(x, y);

    data[i * 2] = direction;
    data[i * 2 + 1] = position;
  }
}

void handle(int connection) {
  unsigned char name[16] = "bobo";

  struct packet login = new_packet(LOGIN, name);
  write_packet(connection, &login);

  struct packet confirm = read_packet(connection);
  printf("client: received confirmation for player %d\n", confirm.data[0]);
  uint8_t player_id = confirm.data[0];

  printf("client: waiting for other player...\n");
  struct packet setup = read_packet(connection);

  unsigned char *opponent_name = setup.data;
  printf("client: opponent name is %s\n", opponent_name);
  speak_text("BEGIN.");

  uint8_t placements[5][3] = {
      {1, 0, 0}, // CARRIER
      {1, 2, 0}, // BATTLESHIP
      {1, 4, 0}, // CRUISER
      {1, 6, 0}, // SUBMARINE
      {1, 8, 0}  // DESTROYER
  };

  unsigned char data[10] = {0};
  serialize_placements(data, placements);

  struct packet place = new_packet(PLACE, data);
  write_packet(connection, &place);

  while (true) {
    struct packet turn = read_packet(connection);
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
      write_packet(connection, &select);
    } else {
      printf("client: waiting for opponent's turn...\n");
    }

    struct packet turn_result = read_packet(connection);
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
}

void init_connection(int *socket_fd, int *connection_fd) {
  struct sockaddr_in address;
  socklen_t address_len = sizeof(address);

  if ((*socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("error: socket failed\n");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = htons(PORT);

  *connection_fd = connect(*socket_fd, (struct sockaddr *)&address, address_len);

  if (*connection_fd < 0) {
    perror("error: connection failed\n");
    exit(EXIT_FAILURE);
  } else {
    printf("client: connected to server on port %d\n", PORT);
  }
}

void endwin_void() {
  endwin();
}

void form(char *username, char *address) {
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
      username[16] = '\0'; // Ensure null termination
      strncpy(address, field_buffer(field[1], 0), 16);
      address[16] = '\0'; // Ensure null termination

      unpost_form(setup);
      free_form(setup);
      free_field(field[0]);
      free_field(field[1]);
      delwin(win);
      return;
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

int main() {
  if (SDL_Init(SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "error: SDL_Init failed: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }

  atexit(SDL_Quit);
  atexit(endwin_void);

  // Initialize ncurses
  initscr();
  raw();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(1);

  char username[17];
  char address[17];
  form(username, address);

  endwin_void();

  printf("client: username is %s\n", username);
  printf("client: address is %s\n", address);

  // int fd, connection;
  // init_connection(&fd, &connection);

  return 0;
}