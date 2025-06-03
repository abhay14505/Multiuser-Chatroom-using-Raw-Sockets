#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "../common/network.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(const char *message, int sender_sock) {
    char final_msg[BUFFER_SIZE + 50];

    //If it's a chat message, prefix with "Socket <fd>:".
    if (strncmp(message, "MSG:", 4) == 0) {
        const char *msg_content = message + 4;
        snprintf(final_msg, sizeof(final_msg), "Socket %d: %s", sender_sock, msg_content);
    }
    //If it's TYPING or STOP_TYPING, attach the sender fd after the prefix
    else if (strncmp(message, "TYPING:", 7) == 0) {
        snprintf(final_msg, sizeof(final_msg), "TYPING:%d", sender_sock);
    }
    else if (strncmp(message, "STOP_TYPING:", 12) == 0) {
        snprintf(final_msg, sizeof(final_msg), "STOP_TYPING:%d", sender_sock);
    }
    //else forward as is
    else {
        strncpy(final_msg, message, sizeof(final_msg));
        final_msg[sizeof(final_msg) - 1] = '\0';
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int sock = client_sockets[i];
        if (sock != 0) {
            send(sock, final_msg, strlen(final_msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == sock) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("Received from socket %d: %s\n", sock, buffer);
        broadcast_message(buffer, sock);
    }
    close(sock);
    remove_client(sock);
    return NULL;
}

int main() {
    int server_sock, new_sock;
    struct sockaddr_in client_addr;
    pthread_t tid;
    
    server_sock = create_server_socket(PORT);
    printf("Server started on port %d\n", PORT);
    
    while (1) {
        new_sock = accept_client(server_sock, &client_addr);
        if (new_sock < 0)
            continue;
        
        pthread_mutex_lock(&clients_mutex);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = new_sock;
                added = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
        
        if (!added) {
            printf("Max clients reached. Rejecting client.\n");
            close(new_sock);
            continue;
        }
        
        int *pclient = malloc(sizeof(int));
        *pclient = new_sock;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }
    
    close(server_sock);
    return 0;
}
