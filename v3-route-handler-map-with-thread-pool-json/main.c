#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include "libs/uthash.h"
#include "libs/cJSON.h"

#define PORT 9990
#define BACKLOG 10
#define BUFFER_SIZE 1024
#define THREAD_POOL_SIZE 30

typedef void *(*handler_fn)(void *ctx);
const char *html_heading = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n";

typedef struct
{
    int client_fd;
    char method[10];
    char route[256];
    char version[10];
} Client;

typedef struct Node
{
    Client *data;
    struct Node *next;
} Node;

typedef struct
{
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadQueue;

void init_queue(ThreadQueue *q)
{
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&(q->mutex), NULL);
    pthread_cond_init(&(q->cond), NULL);
}

void enque(ThreadQueue *q, Client *client)
{
    Node *new_node = malloc(sizeof(Node));
    if (!new_node)
        return;

    new_node->data = client;
    new_node->next = NULL;
    pthread_mutex_lock(&q->mutex);

    if (q->tail == NULL)
    {
        q->head = new_node;
        q->tail = new_node;
    }
    else
    {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

Client *deque(ThreadQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL)
    {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    Node *temp = q->head;
    Client *data = temp->data;

    q->head = temp->next;
    if (q->head == NULL)
    {
        q->tail = NULL;
    }
    free(temp);
    pthread_mutex_unlock(&q->mutex);
    return data;
}

typedef struct
{
    char *route;
    handler_fn handler;
    UT_hash_handle hh;
} RouteHandlerMap;

RouteHandlerMap *handlerMap = NULL;

void add_handler(RouteHandlerMap **map, const char *name, handler_fn fn)
{
    RouteHandlerMap *e = malloc(sizeof(RouteHandlerMap));
    e->route = strdup(name);
    e->handler = fn;

    HASH_ADD_KEYPTR(hh, *map, e->route, strlen(e->route), e);
}

handler_fn get_handler(RouteHandlerMap *map, const char *name)
{
    RouteHandlerMap *e;
    HASH_FIND_STR(map, name, e);
    if (e)
        return e->handler;

    return NULL;
}

void success_response(Client *client, const char *msg)
{
    send(client->client_fd, html_heading, strlen(html_heading), 0);
    if (send(client->client_fd, msg, strlen(msg), 0) < 0)
    {
        perror("FAILED TO SEND MESSAGE:");
        close(client->client_fd);
        free(client);
        return;
    }
    shutdown(client->client_fd, SHUT_WR);
    close(client->client_fd);
    free(client);
}

void error_response(Client *client, const char *header, const char *msg)
{
    send(client->client_fd, header, strlen(header), 0);
    if (send(client->client_fd, msg, strlen(msg), 0) < 0)
    {
        perror("FAILED TO SEND MESSAGE:");
        close(client->client_fd);
        free(client);
        return;
    }
    shutdown(client->client_fd, SHUT_WR);
    close(client->client_fd);
    free(client);
}

void *home_request(void *ctx)
{
    // sleep(1); // simulate sleep
    Client *client = (Client *)ctx;
    char filename[255] = {0};
    if (
        strcmp(client->route, "/") == EXIT_SUCCESS ||
        strcmp(client->route, "/home") == EXIT_SUCCESS ||
        strcmp(client->route, "/index") == EXIT_SUCCESS)
    {
        strcpy(filename, "static/index.html");
    }
    else if (strcmp(client->route, "/about") == EXIT_SUCCESS)
    {
        strcpy(filename, "static/about.html");
    }
    else if (strcmp(client->route, "/contact") == EXIT_SUCCESS)
    {
        strcpy(filename, "static/contact.html");
    }
    else if (strcmp(client->route, "/projects") == EXIT_SUCCESS)
    {
        strcpy(filename, "static/projects.html");
    }
    else if (strcmp(client->route, "/favicon.ico") == EXIT_SUCCESS)
    {
        strcpy(filename, "static/favicon.ico");
    }
    FILE *html = fopen(filename, "r");
    if (html == NULL)
    {
        printf("FAILED TO OPEN REQUESTED FILE :%s\n", filename);
        close(client->client_fd);
        free(client);
        return NULL;
    }

    char read_buffer[BUFFER_SIZE];
    int read_size = 0;
    send(client->client_fd, html_heading, strlen(html_heading), 0);
    while ((read_size = fread(read_buffer, 1, BUFFER_SIZE, html)) > 0)
    {
        send(client->client_fd, read_buffer, read_size, 0);
    }
    shutdown(client->client_fd, SHUT_WR);
    close(client->client_fd);
    free(client);
    return NULL;
}

void *hello_handler(void *ctx)
{
    Client *client = (Client *)ctx;
    if(strcmp(client->method, "POST") == EXIT_SUCCESS) {
        char *message = "Hello Hello\n"
                "POST Request sent \n"
                "This is POST response.\n";
        success_response(client, message);
    } else if(strcmp(client->method, "GET") == EXIT_SUCCESS) {
        char *message = "Hello Hello\n"
                "GET Request sent \n"
                "This is GET response.\n";
        success_response(client, message);
    } else {
        char *message = "Hello Hello\n";
        success_response(client, message);
    }
    return NULL;
}

void *bye_handler(void *ctx)
{
    Client *client = (Client *)ctx;

    char *message = "goodbye\n";
    success_response(client, message);
    return NULL;
}

void *json_handler(void *ctx) 
{
    Client *client = (Client *)ctx;
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddStringToObject(json, "name", "John doe");
    cJSON_AddStringToObject(json, "message", "HELLO WORLD");
    cJSON_AddNumberToObject(json, "id", 100);
    char *json_str = cJSON_Print(json);
    success_response(client, json_str);
    
    cJSON_free(json_str);
    cJSON_Delete(json);
    
    return NULL;
}

void *not_found_handler(void *ctx)
{
    Client *client = (Client *)ctx;
    const char *not_found_header = "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n";

    FILE *html = fopen("static/404.html", "r");
    char read_buffer[BUFFER_SIZE];
    int read_size = 0;
    send(client->client_fd, html_heading, strlen(html_heading), 0);
    while ((read_size = fread(read_buffer, 1, BUFFER_SIZE, html)) > 0)
    {
        send(client->client_fd, read_buffer, read_size, 0);
    }

    shutdown(client->client_fd, SHUT_WR);
    close(client->client_fd);
    free(client);
    return NULL;

    error_response(client, not_found_header, "route not found");
    return NULL;
}

void *dispatch_handler(void *ctx)
{
    Client *client = (Client *)ctx;
    char request_buffer[BUFFER_SIZE];
    int n = recv(client->client_fd, request_buffer, BUFFER_SIZE, 0);
    if (n < 0)
    {
        perror("FAILED TO RECV:");
        close(client->client_fd);
        free(client);
        return NULL;
    }
    // null terminate the request buffer
    request_buffer[n] = '\0';
    sscanf(request_buffer, "%s %s %s", client->method, client->route,
        client->version);

    handler_fn h = get_handler(handlerMap, client->route);
    if (h)
    {
        h(client);
    }
    else
    {
        printf("ERROR:: HANDLER NOT FOUND FOR ROUTE %s\n", client->route);
        not_found_handler(client);
    }
    return NULL;
}

void *worker(void *ctx)
{
    ThreadQueue *q = (ThreadQueue *)ctx;
    while (true)
    {
        Client *client = deque(q);
        dispatch_handler((void *)client);
    }
    return NULL;
}

int register_routes()
{
    add_handler(&handlerMap, "/", home_request);
    add_handler(&handlerMap, "/home", home_request);
    add_handler(&handlerMap, "/index", home_request);
    add_handler(&handlerMap, "/about", home_request);
    add_handler(&handlerMap, "/contact", home_request);
    add_handler(&handlerMap, "/projects", home_request);
    add_handler(&handlerMap, "/facicon.ico", home_request);

    add_handler(&handlerMap, "/hello", hello_handler);
    add_handler(&handlerMap, "/bye", bye_handler);
    add_handler(&handlerMap, "/json", json_handler);
    

    return EXIT_SUCCESS;
}

int main()
{
    printf("process is running on pid %d\n", getpid());
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("FAILED TO CREATE SOCKET:");
        exit(EXIT_FAILURE);
    }

    int err = bind(server_fd, (struct sockaddr *)&server_addr,
        (socklen_t)sizeof(server_addr));
    if (err != EXIT_SUCCESS)
    {
        perror("FAILED TO BIND:");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) != EXIT_SUCCESS)
    {
        perror("FAILED TO LISTEN ON THE SOCKET:");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (register_routes() != EXIT_SUCCESS)
    {
        perror("FAILED TO REGISTER ROUTES:");
        exit(EXIT_FAILURE);
    }

    // create thread pool
    ThreadQueue queue;
    init_queue(&queue);
    pthread_t thread_pool[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&thread_pool[i], NULL, worker, (void *)&queue);
    }
    printf("THREAD POOL CREATED OF SIZE %d\n", THREAD_POOL_SIZE);

    printf("LISTENING FOR CONN IN PORT %d...\n", PORT);

    while (true)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);
        int client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_fd < 0)
        {
            perror("FAILED TO CREATE CLIENT SOCKET:");
            continue;
        }

        printf("ACCEPTED CONN FROM %s:%d\n", inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

        Client *client = calloc(1, sizeof(Client));
        client->client_fd = client_fd;

        enque(&queue, client);
        // pthread_t new_conn;
        // pthread_create(&new_conn, NULL, dispatch_handler, (void *)client);
        // pthread_detach(new_conn);
    }

    close(server_fd);
    return EXIT_SUCCESS;
}
