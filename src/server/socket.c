#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packet.h"
#include "shared.h"
#include "socket.h"

struct client wait_for_client(uint8_t id, int fd, struct sockaddr *address, socklen_t *address_len) {
  int connection = accept(fd, address, address_len);
  if (connection < 0) {
    perror("error: failed to accept connection\n");
    exit(EXIT_FAILURE);
  }

  struct packet login = read_packet(connection);
  char *name = (char *)login.data;

  struct packet confirm = new_packet(LOGIN_CONFIRM, &id);
  write_packet(connection, &confirm);

  struct client result = {
      .id = id,
      .fd = connection,
      .name = name,
  };

  printf("server: player %d (%s) connected\n", result.id, result.name);

  return result;
}

void close_player(struct client *player) {
  printf("server: player %d connection closed\n", player->id);

  free(player->name);
  close(player->fd);
}

int init(int *fd, struct sockaddr_in *address, socklen_t address_len) {
  if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("error: socket failed\n");
    return -1;
  }

  address->sin_family = AF_INET;
  address->sin_addr.s_addr = htonl(INADDR_ANY);
  address->sin_port = htons(PORT);

  int opt = 1;
  if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("error: setsockopt failed\n");
    return -1;
  }

  if ((bind(*fd, (struct sockaddr *)address, address_len)) != 0) {
    perror("error: socket bind failed.\n");
    return -1;
  }

  if ((listen(*fd, 5)) != 0) {
    perror("error: listening failed\n");
    return -1;
  }

  printf("server: listening on port %d\n", PORT);
  return 0;
}