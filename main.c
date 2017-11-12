/* 
 *
 */
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "queue.h"

// Port to bind socket to
#define PORT          (8008)
// Maximum queue length of pending connections
#define BACKLOG       (5)
// Receive/transmit buffer size
#define RX_BUF_SIZE   (2048)
#define TX_BUF_SIZE   (2048)
// The number of threads (child servers)
#define N_THREADS     (5)
// Maximum thread pool task queue size
#define MAX_TASK_SIZE (10)
// Debug printing macro (var arguments)
#ifdef DEBUG
#define DPRINT(...) {fprintf(stderr, __VA_ARGS__);}
#else
#define DPRINT(...)
#endif

// Indicates desired run status of program (1: run, 0: try to stop)
static volatile int run_status = 1;
// Thread pool queue (thread-safe)
queue_t s_queue;
// HTTP Response template
const char* http_tmplt = "HTTP/1.0 %s\nContent-Type: text/html\nContent-Length: %u\n\n%s";
char* empty = "";


// Ctrl-c handler
void intHandler(int dummy)
{
    DPRINT("Interrupt received.\n");
    run_status = 0;
}


void* serve_http(void* unused)
{
    char rx_buffer[RX_BUF_SIZE],
         tx_buffer[TX_BUF_SIZE];
    int client_socket_fd, rx_size, tx_size;
    char *pcha, *pchb, *req_typ, *req_file, *req_prot, *body;
    FILE *f;
    long f_size;

    DPRINT("serve_http thread running.\n");

    while (run_status)
    {
        // Check if work to do (requests available to process)
        if ((client_socket_fd = queue_pop(&s_queue)) == -1)
            continue;

        DPRINT("Processing HTTP request.\n");

        /* Receive socket message
         *   sockfd client_socket_fd: Client socket fd
         *   buf    rx_buffer:        The receive buffer
         *   len    RX_BUF_SIZE:      Size of receive buffer
         *   flags  0:                No flags
         */
        if ((rx_size = recv(client_socket_fd, rx_buffer, RX_BUF_SIZE, 0)) < 0)
        {
            perror("Failed to read from client");
            exit(EXIT_FAILURE);
        }
        DPRINT("Successfully received client's message:\n%s\n", rx_buffer);

        // Parse message
        req_typ = empty;
        req_file = empty;
        req_prot = empty;
        pcha = strtok(rx_buffer, "\r\n");

        DPRINT("pcha: %s\n", pcha);
        // Retrieve request type, file URI, and protocol
        if (pcha != NULL) {
            if ((pchb = strtok(pcha, " ")) != NULL) {
                req_typ = pchb;
                if ((pchb = strtok(NULL, " ")) != NULL) {
                    req_file = pchb;
                    if ((pchb = strtok(NULL, " ")) != NULL)
                        req_prot = pchb;
                }
            }
        }
        DPRINT("Parsed contents:%s %s %s\n", req_typ, req_file, req_prot);

        // Treat empty string as request for index page
        if (strcmp(req_file, "") == 0)
            req_file = "html/index.html";
        else
            req_file = strcat("html/", req_file);

        if (strcmp(req_typ, "GET") != 0 ||
            (strcmp(req_prot, "HTTP/1.0") != 0 &&
             strcmp(req_prot, "HTTP/1.1") != 0))
        {
            DPRINT("405.\n");
            // Respond to non-GET messages with 405 Method Not Allowed
            body = "<html><h1>405 Method Not Allowed</h1></html>";
            sprintf(tx_buffer, http_tmplt, "405 Method Not Allowed\nAllow: GET",
                    strlen(body), body);
        } else if (access(req_file, F_OK) == -1)
        {
            DPRINT("404.\n");
            // Respond to unknown pages/files with 404 Not Found
            body = "<html><h1>404 Not Found</h1></html>";
            sprintf(tx_buffer, http_tmplt, "404 Not Found", strlen(body), body);
        }
            // Respond to private pages/files with 403 Forbidden
        else
        {
            DPRINT("200.\n");
            // Otherwise respond with 200 OK w/ HTML file contents
            f = fopen(req_file, "rb");
            fseek(f, 0, SEEK_END);
            f_size = ftell(f);
            rewind(f);
            if ((body = malloc(f_size+1)) == NULL)
            {
                perror("Failed to malloc body");
                exit(EXIT_FAILURE);
            }
            fread(body, f_size, 1, f);
            fclose(f);
            body[f_size] = '\0';
            sprintf(tx_buffer, http_tmplt, "200 OK", strlen(body), body);
            free(body);
        }

        /* Send message to socket
         *   sockfd client_socket_fd: Client socket fd
         *   buf    tx_buffer:        Transmit buffer
         *   len    TX_BUF_SIZE:      Size of transmit buffer
         *   flags  0:                No flags
         */
        if ((tx_size = send(client_socket_fd, tx_buffer, TX_BUF_SIZE, 0)) < 0)
        {
            perror("Failed to send to client");
            exit(EXIT_FAILURE);
        }
        DPRINT("Successfully replied to client:\n%s\n", tx_buffer);

        // Attempt to close client socket fd
        if (close(client_socket_fd) < 0)
        {
            perror("Failed to close client socket fd");
            exit(EXIT_FAILURE);
        }
        DPRINT("Successfully closed connection to client.\n");
    }

    DPRINT("Exiting thread.\n");
    pthread_exit(NULL);
}


int main(int argc, char const *argv[])
{
    int socket_fd, client_socket_fd, result;
    struct sockaddr_in address;
    socklen_t addrlen = (socklen_t)sizeof(address);
    pthread_t threads[N_THREADS];
    fd_set readfds;

    // Register Ctrl-c handler
    signal(SIGINT, intHandler);

    // Init s_queue
    queue_create(&s_queue, MAX_TASK_SIZE);

    /* Create socket file descriptor
     *   Domain   AF_INET:     IPv4 address family
     *   Type     SOCK_STREAM: TCP connection
     *   Protocol 0:           Default protocol
     */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket fd");
        exit(EXIT_FAILURE);
    }

    /* Set host address
     *   sin_family      AF_INET:    Address family (required)
     *   sin_addr.s_addr INADDR_ANY: Host IP (any interface) in network byte
     *                               order
     *   sin_port        PORT:       Host port in network byte order
     */
    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port        = htons(PORT);

    /* Bind socket to PORT
     *   sockfd    socket_fd: Socket fd
     *   addr      &address:  Host address struct
     *   socklen_t addrlen:   Size of the address struct
     */
    if (bind(socket_fd, (struct sockaddr*)&address, addrlen) < 0)
    {
        perror("Failed to bind host address to socket");
        exit(EXIT_FAILURE);
    }

    /* Mark socket as passive
     *   sockfd  socket_fd: Socket fd
     *   backlog BACKLOG:   Maximum queue length of pending connections
     */
    if (listen(socket_fd, BACKLOG) < 0)
    {
        perror("Failed to listen on socket");
        exit(EXIT_FAILURE);
    }

    // Create threads for pool
    DPRINT("Creating threads.\n");
    for (int i=0; i<N_THREADS; ++i)
    {
        if (pthread_create(&threads[i], NULL, serve_http, NULL))
        {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }
    DPRINT("Threads created.\n");

    // Setup var to hold fds to be watched
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);

    // Main loop
    while (run_status)
    {
        /* Check if I/O is available in readfds (client_socket_fd...)
         *   nfds    socket_fd+1: Highest # fd + 1 (required)
         *   readfds &readfds:    List of fds watched to see if chars available
         *   ...     NULL:        Unused options
         */
        result = select(socket_fd+1, &readfds, NULL, NULL, NULL);
        if (result < 0)
        {
            perror("Failed to select client_socket_fd");
            exit(EXIT_FAILURE);
        } else if (result == 0)
        {
            DPRINT("No data available.\n");
            // No data
            continue;
        }

        /* Accept a socket connection from pending connections queue.
         *   sockfd    socket_fd: Socket fd
         *   addr      &address:  Host address struct
         *   socklen_t &addrlen:  Size of address struct
         */
        if ((client_socket_fd = accept(socket_fd, (struct sockaddr*)&address,
                                       &addrlen)) < 0)
        {
            perror("Failed to accept connection");
            exit(EXIT_FAILURE);
        }
        DPRINT("Successfully connected to client.\n");

        // Add connection fd to queue (if queue full connections lost)
        queue_push(&s_queue, client_socket_fd);
        DPRINT("Pushed connection to queue (size %u).\n", s_queue.size);
    }

    // Join all threads
    DPRINT("Joining all threads.\n");
    for (int i=0; i<N_THREADS; ++i)
    {
        if(pthread_join(threads[i], NULL))
        {
            perror("Failed to join thread");
            exit(EXIT_FAILURE);
        }
    }
    DPRINT("Threads joined.\n");

    // Attempt to close server socket fd
    if (close(socket_fd) < 0)
    {
        perror("Failed to close server socket fd");
        exit(EXIT_FAILURE);
    }

    // Destroy queue
    queue_destroy(&s_queue);

    return EXIT_SUCCESS;
}

