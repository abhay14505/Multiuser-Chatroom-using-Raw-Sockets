#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <glib.h>
#include "gui.h"
#include "typing.h"

// Forward declarations
static void scroll_to_bottom();
static gboolean display_message_safe(gpointer data);

// Main window elements
static GtkWidget *window;
static GtkWidget *header_label;
static GtkWidget *text_view;
static GtkWidget *entry;
static GtkWidget *typing_label;
static int sock_fd;

// Hash table of typing sockets
static GHashTable *typing_sockets = NULL;

// -------------------------------------------------------------------------------------
// Auto-scroll after new text
static void scroll_to_bottom() {
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(
        GTK_SCROLLED_WINDOW(gtk_widget_get_parent(text_view))
    );
    if (adj) {
        gtk_adjustment_set_value(adj, gtk_adjustment_get_upper(adj));
    }
}

// -------------------------------------------------------------------------------------
// Build the label like "Socket 4 is typing..." or "Socket 4 and Socket 5 are typing..."
static void update_typing_label() {
    GList *keys = g_hash_table_get_keys(typing_sockets);
    int count = g_list_length(keys);

    if (count == 0) {
        gtk_label_set_text(GTK_LABEL(typing_label), "");
        g_list_free(keys);
        return;
    }

    char label[256] = "";
    GList *l = keys;
    while (l) {
        int sock_id = GPOINTER_TO_INT(l->data);
        char sock_str[32];
        snprintf(sock_str, sizeof(sock_str), "Socket %d", sock_id);
        strcat(label, sock_str);

        if (l->next) strcat(label, " and ");
        l = l->next;
    }

    if (count == 1) {
        strcat(label, " is typing...");
    } else {
        strcat(label, " are typing...");
    }
    gtk_label_set_text(GTK_LABEL(typing_label), label);

    g_list_free(keys);
}

// -------------------------------------------------------------------------------------
// parse_and_display_chunk: recognized "TYPING:<fd>", "STOP_TYPING:<fd>", "MSG:..."
static void parse_and_display_chunk(char *chunk) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end_iter;

    // TYPING:
    if (g_str_has_prefix(chunk, "TYPING:")) {
        int sock_id = atoi(chunk + 7); // after "TYPING:"
        g_hash_table_add(typing_sockets, GINT_TO_POINTER(sock_id));
        update_typing_label();
        return;
    }
    // STOP_TYPING:
    else if (g_str_has_prefix(chunk, "STOP_TYPING:")) {
        int sock_id = atoi(chunk + 12);
        g_hash_table_remove(typing_sockets, GINT_TO_POINTER(sock_id));
        update_typing_label();
        return;
    }
    // MSG:
    else if (g_str_has_prefix(chunk, "MSG:")) {
        char *msg_body = chunk + 4;
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert(buffer, &end_iter, msg_body, -1);
        gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
    }
    // leftover text
    else {
        gtk_text_buffer_get_end_iter(buffer, &end_iter);
        gtk_text_buffer_insert(buffer, &end_iter, chunk, -1);
        gtk_text_buffer_insert(buffer, &end_iter, "\n", -1);
    }
    scroll_to_bottom();
}

// -------------------------------------------------------------------------------------
// display_message_safe: scheduled via g_idle_add() to run in main thread
static gboolean display_message_safe(gpointer data) {
    char *full_chunk = (char *)data;
    parse_and_display_chunk(full_chunk);
    g_free(full_chunk);
    return G_SOURCE_REMOVE;
}

// -------------------------------------------------------------------------------------
// Thread-safe entry point for messages
void display_message(const char *msg) {
    g_idle_add(display_message_safe, g_strdup(msg));
}

void send_message(const char *msg) {
    if (msg && strlen(msg) > 0) {
        send(sock_fd, msg, strlen(msg), 0);
    }
}

// Callback: user sends a message
void on_send_clicked(GtkButton *button, gpointer user_data) {
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if (g_strcmp0(text, "") != 0) {
        char msg[1024];
        snprintf(msg, sizeof(msg), "MSG:%s", text);
        send_message(msg);
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

// Callback: user typed text (trigger typing logic)
void on_entry_changed(GtkEditable *editable, gpointer user_data) {
    start_typing();
}

// GUI init
void init_gui(int argc, char *argv[], int sock) {
    sock_fd = sock;
    init_typing_module(sock_fd);

    gtk_init(&argc, &argv);

    // Initialize hash table for typing sockets
    typing_sockets = g_hash_table_new(g_direct_hash, g_direct_equal);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chatroom");
    gtk_window_set_default_size(GTK_WINDOW(window), 500, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    header_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(header_label),
        "<span size='xx-large' weight='bold'>Chatroom</span>");
    gtk_box_pack_start(GTK_BOX(vbox), header_label, FALSE, FALSE, 5);

    GtkWidget *frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 5);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

    typing_label = gtk_label_new("");
    gtk_label_set_markup(GTK_LABEL(typing_label),
        "<span style='italic' size='small'></span>");
    gtk_box_pack_start(GTK_BOX(vbox), typing_label, FALSE, FALSE, 5);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 5);
    g_signal_connect(entry, "activate", G_CALLBACK(on_send_clicked), NULL);
    g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), NULL);

    GtkWidget *send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(hbox), send_button, FALSE, FALSE, 5);
    g_signal_connect(send_button, "clicked", G_CALLBACK(on_send_clicked), NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
}

