#include <arpa/inet.h>
#include <stdint.h>

/**
 * @struct client
 * @brief Represents a connected client in the Battleship server.
 */
struct client {
  uint8_t id;
  char *name;
  int fd;
};

/**
 * Waits for a client to connect to the server, performs login handshake, and returns a populated client structure.
 *
 * @param id         The unique identifier to assign to the connecting client.
 * @param fd         The file descriptor of the listening socket.
 * @param address    Pointer to a sockaddr structure to receive the client's address.
 * @param address_len Pointer to a socklen_t variable containing the size of the address structure.
 * @return           A struct client containing the client's id, connection file descriptor, and name.
 *
 * This function blocks until a client connects, reads the login packet to obtain the client's name,
 * sends a login confirmation packet, and prints a message indicating the connection.
 * On failure to accept a connection, the function prints an error and exits the program.
 */
struct client wait_for_client(uint8_t id, int fd, struct sockaddr *address, socklen_t *address_len);

/**
 * Closes the connection for a given player.
 *
 * @param player Pointer to the client structure representing the player whose connection should be closed.
 */
void close_player(struct client *player);

int init(int *fd, struct sockaddr_in *address, socklen_t address_len);