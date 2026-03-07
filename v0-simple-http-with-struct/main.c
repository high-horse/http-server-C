#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "lib/server.h"

#define BUFFER_SIZE 2048

void launch(struct Server *server)
{
    char buffer[BUFFER_SIZE];
    size_t address_size = sizeof(server->address);
    int new_socket;

    char *return_message =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<html><body><h1>Za, Worldo!</h1></body></html>";

    printf("======WAITING FOR CONNECTION======\n");

    while (true)
    {
        new_socket = accept(server->socket, (struct sockaddr *)&server->address, (socklen_t *)&address_size);
        printf("------NEW CONNECTION---------\n");
        ssize_t read_size = read(new_socket, buffer, BUFFER_SIZE);
        if (read_size < 0)
        {
            perror("FAILED TO READ BYTES:");
            exit(EXIT_FAILURE);
        }
        printf("READ :%s\n", buffer);
        write(new_socket, return_message, strlen(return_message));
        close(new_socket);
    }
}

int main()
{
    struct Server server = server_constructor(AF_INET, SOCK_STREAM, 0, INADDR_ANY, 8888, 10, launch);
    server.launch(&server);
}
