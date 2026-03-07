// #include "Server.h"
#include "../lib/server.h" 
#include <stdio.h>
#include <stdlib.h>

struct Server server_constructor(
    int domain, int service, int protocol, unsigned long interface,
    int port, int backlog, void (*launch)(struct Server *server)
)
{
    struct Server server;

    server.domain = domain;
    server.service = service;
    server.protocol = protocol;
    server.interface = interface;
    server.port = port;
    server.backlog = backlog;


    server.address.sin_family = domain;
    server.address.sin_port = htons(port);
    server.address.sin_addr.s_addr = htonl(interface);

    server.socket = socket(domain, service, protocol);
    if(server.socket < 0) {
        perror("FAILED TO INITIATE SOCKET :");
        exit(EXIT_FAILURE);
    }

    if(bind(server.socket, (struct sockaddr *)&server.address, sizeof(server.address)) < 0) {
        perror("FAILED TO BIND SOCKET :");
        exit(EXIT_FAILURE);
    }

    if(listen(server.socket, server.backlog) < 0) {
        perror("FAILED TO LISTEN ON SOCKET :");
        exit(EXIT_FAILURE);
    }

    server.launch = launch;
    
    return server;
}