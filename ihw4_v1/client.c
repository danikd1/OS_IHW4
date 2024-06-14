#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 256

volatile sig_atomic_t client_running = 1;

void handle_signal(int signum) {
    client_running = 0;
}

void send_request(int sock, struct sockaddr_in *server, const char *request) {
    if (sendto(sock, request, strlen(request), 0, (struct sockaddr *)server, sizeof(*server)) < 0) {
        puts("Send failed");
        exit(1);
    }

    char server_reply[BUFFER_SIZE];
    memset(server_reply, 0, BUFFER_SIZE);

    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    int bytes_received = recvfrom(sock, server_reply, sizeof(server_reply) - 1, 0, (struct sockaddr *)&from, &from_len);
    if (bytes_received < 0) {
        puts("recvfrom failed");
        exit(1);
    }

    server_reply[bytes_received] = '\0';
    printf("Server reply: %s\n", server_reply);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <PORT> [reader|writer]\n", argv[0]);
        return 1;
    }

    const char *ip_address = argv[1];
    int port = atoi(argv[2]);

    int sock;
    struct sockaddr_in server;

    signal(SIGINT, handle_signal);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return 1;
    }
    printf("Socket created\n");

    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    // Читатель или писатель
    if (strcmp(argv[3], "reader") == 0) {
        while (client_running) {
            printf("Sending read request...\n");  // Отладка
            send_request(sock, &server, "read");
            sleep(1);  // Эмуляция работы читателя
        }
    } else if (strcmp(argv[3], "writer") == 0) {
        while (client_running) {
            printf("Sending write request...\n");  // Отладка
            send_request(sock, &server, "write");
            sleep(1);  // Эмуляция работы писателя
        }
    } else {
        fprintf(stderr, "Invalid argument. Use 'reader' or 'writer'.\n");
        close(sock);
        return 1;
    }

    close(sock);
    printf("Client shut down gracefully.\n");
    return 0;
}


