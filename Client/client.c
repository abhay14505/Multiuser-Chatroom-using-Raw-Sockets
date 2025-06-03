#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "../common/network.h"
#include "gui.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int sock; // Global socket descriptor used by both the GUI and network thread

// Network listener thread: receives messages from the server.
void *network_listener(void *arg) {
    char buffer[1024];
    int bytes;
    while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        // Display the incoming message in the GUI.
        display_message(buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t net_thread;
    
    // Connect to the chat server.
    sock = connect_to_server(SERVER_IP, PORT);
    
    // Initialize the GUI, passing the socket descriptor.
    init_gui(argc, argv, sock);
    
    // Create a thread to continuously listen for incoming messages.
    pthread_create(&net_thread, NULL, network_listener, NULL);
    pthread_detach(net_thread);
    
    // Enter the GTK main loop.
    gtk_main();
    
    close(sock);
    return 0;
}

