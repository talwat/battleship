#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080

void handle(int connection) {
    char buffer[1024] = {0};

    const char *response = "hello!";
    write(connection, response, strlen(response));

    int bytes_read = read(connection, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        perror("error: reading from socket failed\n");
        exit(EXIT_FAILURE);
    }

    printf("client: received message: %s\n", buffer);
}

int main() {
    int fd;
    struct sockaddr_in address;
    socklen_t address_len = sizeof(address);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("error: socket failed\n");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);

    if (connect(fd, (struct sockaddr *)&address, address_len) < 0) {
        perror("error: connection failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("client: connected to server on port %d\n", PORT);
    }

    handle(fd);
    close(fd);
    
    return 0;
}