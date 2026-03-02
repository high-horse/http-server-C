#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include "lib/map.h"

#define PORT 9990
#define BACKLOG 100
#define BUFFER_SIZE 1024
hashmap * route_file;

typedef struct{
    char filename[256];
    int client_fd;
    char method[10];
    char route[255] ;
    char version[16];
}ThreadData;


void * handle_client_old(int * client_fd, const char *filename) {
    // int *client_fd = (int *)arg;
    // printf("serving file %s\n", filename);
    FILE *html = fopen(filename, "r");
    if(html == NULL) {
        perror("could not open file");
        return NULL;
    }
    char read_buffer[BUFFER_SIZE] = {0};
    size_t read_size = 0;
    char *header =  "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Connection: close\r\n"
           "\r\n";
    
    send(*client_fd, header, strlen(header), 0);
    
    while ((read_size = fread(read_buffer, 1, BUFFER_SIZE, html)) > 0) {
        send(*client_fd, read_buffer, read_size, 0);
    }
    return NULL;
}

char *parse_route(const char * route) {
    printf(" route %s\n", route);
    if(strcmp(route, "/") == EXIT_SUCCESS || strcmp(route, "/home") == EXIT_SUCCESS || strcmp(route, "/index") == EXIT_SUCCESS) return "static/index.html";
    else if(strcmp(route, "/about") == EXIT_SUCCESS) return "static/about.html";
    else if (strcmp(route, "/contact") == EXIT_SUCCESS) return "static/contact.html";
    else if (strcmp(route, "/projects") == EXIT_SUCCESS) return "static/projects.html";
    else if (strcmp(route, "/facicon.ico") == EXIT_SUCCESS) return "static/facicon.ico";
    
    return "static/404.html";
}

void * handle_client(void *arg) {
    sleep(1);
    ThreadData *thread_data = (ThreadData *)arg;
    strcpy(thread_data->filename, parse_route(thread_data->route));
    printf("parsed filename %s\n", thread_data->filename);
    FILE *html = fopen(thread_data->filename, "r");
    if(html == NULL) {
        printf("FAILED TO OPEN REQUESTED FILE :%s\n", thread_data->filename);
        perror("FAILED TO OPEN FILE:");
        close(thread_data->client_fd);
        free(thread_data);
        return NULL;
    }
    
    char read_buffer[BUFFER_SIZE] = {0};
    int read_size = 0;
    char *header =  "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Connection: close\r\n"
           "\r\n";
    send(thread_data->client_fd, header, strlen(header), 0);
    while((read_size = fread(read_buffer, 1, BUFFER_SIZE, html)) > 0) { // fread(dest, unit_size, total_read_size, *fp)
        send(thread_data->client_fd, read_buffer, read_size, 0);
    }
    shutdown(thread_data->client_fd, SHUT_WR);
    close(thread_data->client_fd);
    free(thread_data);
    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) {
        perror("Failed to creae server socket fd:");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };
    
    if(bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) != EXIT_SUCCESS){
        perror("Failed to bind server");
        exit(EXIT_FAILURE);
    }
    
    if(listen(server_fd, BACKLOG) != EXIT_SUCCESS) {
        perror("Failed to listen:");
        exit(EXIT_FAILURE);
    }
    printf("Server running on %d. Waiting for connection...\n", PORT);
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_length = sizeof(client_address);
        char request_buffer[BUFFER_SIZE] = {0};
        int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &client_length);
        if(client_fd < 0) {
            perror("Failed to accept client");
            continue;
        }
        printf("connected ");
        int bytes = recv(client_fd, request_buffer, BUFFER_SIZE, 0);
        if (bytes < 0) {
            close(client_fd);continue;
        }
        request_buffer[bytes] = '\0'; // null terminate string at the end of buffer.
        
        ThreadData *thread_data = calloc(1, sizeof(ThreadData));
        thread_data->client_fd = client_fd;
 
        sscanf(request_buffer, "%s %s %s", thread_data->method, thread_data->route, thread_data->version);
        
        pthread_t new_conn;
        pthread_create(&new_conn, NULL, handle_client, (void *)thread_data);
        // handle_client(thread_data);
    }
    
    return EXIT_SUCCESS;
}