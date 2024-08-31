#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENT 10
#define BUFFER_SIZE 1024

int client_sockets[MAX_CLIENT];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("Client %d: %s\n", client_sock, buffer);

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (client_sockets[i] != 0 && client_sockets[i] != client_sock) {
                send(client_sockets[i], buffer, strlen(buffer), 0);
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }

    if (read_size == 0) {
        printf("Client %d disconnected\n", client_sock);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(client_sock);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENT; i++) {
        if (client_sockets[i] == client_sock) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    free(socket_desc);
    return 0;
}

int main() {
    int server_sock, client_sock, c, *new_sock;
    struct sockaddr_in server, client;

    // Initialize client sockets array
    for (int i = 0; i < MAX_CLIENT; i++) {
        client_sockets[i] = 0;
    }

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }
    printf("Bind done\n");

    // Listen for incoming connections
    if (listen(server_sock, 3) < 0) {
        perror("Listen failed");
        return 1;
    }
    printf("Waiting for incoming connections...\n");

    c = sizeof(struct sockaddr_in);
    while ((client_sock = accept(server_sock, (struct sockaddr *)&client, (socklen_t*)&c))) {
        printf("Connection accepted\n");

        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENT; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_sock;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        pthread_t client_thread;
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&client_thread, NULL, handle_client, (void*)new_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
        printf("Handler assigned\n");
    }

    if (client_sock < 0) {
        perror("Accept failed");
        return 1;
    }

    return 0;
}
