#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packet.h"
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
  close(player->fd);
}