#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

GtkWidget *text_view;
GtkTextBuffer *text_buffer;
int sock;
pthread_t recv_thread;

void *receive_messages(void *socket_desc)
{
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        buffer[read_size] = '\0';

        GtkTextIter end;
        gtk_text_buffer_get_end_iter(text_buffer, &end);
        gtk_text_buffer_insert(text_buffer, &end, buffer, -1);
        gtk_text_buffer_insert(text_buffer, &end, "\n", -1);
    }

    if (read_size == 0)
    {
        printf("Server disconnected\n");
    }
    else if (read_size == -1)
    {
        perror("recv failed");
    }

    return 0;
}

void send_message(GtkWidget *widget, gpointer entry)
{
    const char *message = gtk_entry_get_text(GTK_ENTRY(entry));
    if (send(sock, message, strlen(message), 0) < 0)
    {
        printf("Send failed\n");
    }
    gtk_entry_set_text(GTK_ENTRY(entry), "");
}

int main(int argc, char *argv[])
{
    struct sockaddr_in server;

    gtk_init(&argc, &argv);

    // Socket setup
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        printf("Could not create socket\n");
        return 1;
    }
    printf("Socket created\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connect failed");
        return 1;
    }
    printf("Connected\n");

    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) < 0)
    {
        perror("Could not create thread");
        return 1;
    }

    // GTK setup
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *entry;
    GtkWidget *send_button;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    text_view = gtk_text_view_new();
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

    send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), entry);
    gtk_box_pack_start(GTK_BOX(hbox), send_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();

    close(sock);
    return 0;
}