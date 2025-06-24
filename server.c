#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080

void handle(int connection) {
    char buffer[1024] = {0};

    int bytes_read = read(connection, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        perror("error: reading from socket failed\n");
        exit(EXIT_FAILURE);
    }
    printf("server: received message: %s\n", buffer);
    
    const char *response = "shalom!";
    write(connection, response, strlen(response));
}

int main(int argc, char const *argv[])
{
    int fd;
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("error: socket failed\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    if ((bind(fd, (struct sockaddr *)&address, address_len)) != 0)
    {
        perror("error: socket bind failed.\n");
        exit(EXIT_FAILURE);
    }

    if ((listen(fd, 5)) != 0)
    {
        perror("error: listening failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("server: listening on port %d\n", PORT);
    }

    int connection = accept(fd, (struct sockaddr *)&address, &address_len);
    if (connection < 0)
    {
        perror("error: failed to accept connection\n");
        exit(EXIT_FAILURE);
    } else {
        printf("server: connection accepted\n");
    }

    handle(connection);

    close(connection);
    close(fd);
}