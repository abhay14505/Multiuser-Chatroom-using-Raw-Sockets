#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <glib.h>  // For g_timeout_add() and g_source_remove()
#include "typing.h"

#define TYPING_TIMEOUT_MS 2000  // 2 seconds

static gboolean typing_in_progress = FALSE;
static guint typing_timeout_id = 0;
static int saved_sock_fd = -1;

// Timeout callback to send STOP_TYPING when no keystrokes occur.
static gboolean on_typing_timeout(gpointer data) {
    // Send bare prefix; server will append the socket FD
    const char *stop_msg = "STOP_TYPING:";
    send(saved_sock_fd, stop_msg, strlen(stop_msg), 0);
    typing_in_progress = FALSE;
    typing_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void init_typing_module(int sock_fd) {
    saved_sock_fd = sock_fd;
    typing_in_progress = FALSE;
    typing_timeout_id = 0;
}

void start_typing() {
    if (!typing_in_progress) {
        typing_in_progress = TRUE;
        const char *typing_msg = "TYPING:";
        send(saved_sock_fd, typing_msg, strlen(typing_msg), 0);
        // Start the timeout
        typing_timeout_id = g_timeout_add(TYPING_TIMEOUT_MS, on_typing_timeout, NULL);
    } else {
        // Reset the timeout for non-spamming logic
        if (typing_timeout_id != 0) {
            g_source_remove(typing_timeout_id);
        }
        typing_timeout_id = g_timeout_add(TYPING_TIMEOUT_MS, on_typing_timeout, NULL);
    }
}

