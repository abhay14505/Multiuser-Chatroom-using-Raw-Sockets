#ifndef GUI_H
#define GUI_H

#include <gtk/gtk.h>

// Initialize the GUI, given command-line arguments and the socket descriptor.
void init_gui(int argc, char *argv[], int sock_fd);

// Display a received message in the chat text view.
void display_message(const char *msg);

#endif

