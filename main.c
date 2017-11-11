/* 
 *
 */
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

// Port to bind socket to
#define PORT        (8008)
// Maximum queue length of pending connections
#define BACKLOG     (5)
// Receive buffer size
#define RX_BUF_SIZE (2048)


int main(int argc, char const *argv[])
{
    int socket_fd, client_socket_fd, valread;
    struct sockaddr_in address;
    socklen_t addrlen = (socklen_t)sizeof(address);
    char buffer[RX_BUF_SIZE] = {0};

    /* Create socket file descriptor
     *   Domain   AF_INET:     IPv4 address family
     *   Type     SOCK_STREAM: TCP connection
     *   Protocol 0:           Default protocol
     */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Failed to create socket fd.");
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
        perror("Failed to bind host address to socket.");
        exit(EXIT_FAILURE);
    }

    /* Mark socket as passive
     *   sockfd  socket_fd: Socket fd
     *   backlog BACKLOG:   Maximum queue length of pending connections
     */
    if (listen(socket_fd, BACKLOG) < 0)
    {
        perror("Failed to listen on socket.");
        exit(EXIT_FAILURE);
    }

    // Main loop
    while(1)
    {
        /* Accept a socket connection from pending connections queue.
         *   sockfd    socket_fd: Socket fd
         *   addr      &address:  Host address struct
         *   socklen_t &addrlen:  Size of address struct
         */
        if ((client_socket_fd = accept(socket_fd, (struct sockaddr*)&address,
                                       &addrlen) < 0))
        {
            perror("Failed to accept connection.");
            exit(EXIT_FAILURE);
        }

        valread = read(client_socket_fd, buffer, RX_BUF_SIZE);
        printf("%s\n", buffer);
        //send(client_socket_fd, hello, strlen(hello), 0);
        printf("Hello message sent\n");

        // Attempt to close client socket fd
        if(close(client_socket_fd) < 0)
        {
            perror("Failed to close client socket fd.");
            exit(EXIT_FAILURE);
        }
    }

    // Attempt to close server socket fd
    if (close(socket_fd) < 0)
    {
        perror("Failed to close server socket fd.");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

