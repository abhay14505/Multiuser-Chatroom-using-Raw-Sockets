#ifndef NETWORK_H
#define NETWORK_H

#include <netinet/in.h>

// Create and bind a TCP server socket on the given port.
int create_server_socket(int port);

// Accept a new client connection.
int accept_client(int server_sock, struct sockaddr_in *client_addr);

// Connect to a server given its IP address and port.
int connect_to_server(const char *ip, int port);

#endif

