#ifndef TYPING_H
#define TYPING_H

// Initialize the typing module with the socket file descriptor.
void init_typing_module(int sock_fd);

// Called on every key change to handle typing notifications.
void start_typing();

#endif
