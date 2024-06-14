#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFFER_SIZE 256
#define CONTROL_PORT 9090
#define MULTICAST_IP "239.255.255.250"
#define MULTICAST_PORT 1900

volatile sig_atomic_t client_running = 1;
volatile sig_atomic_t client_paused = 0;

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

void handle_commands(int control_sock, int multicast_sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);

    int bytes_received = recvfrom(control_sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr *)&from, &from_len);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate the received string
        if (strcmp(buffer, "stop") == 0) {
            client_paused = 1;
            printf("Client paused\n");
        } else if (strcmp(buffer, "start") == 0) {
            client_paused = 0;
            printf("Client resumed\n");
        }
    }

    bytes_received = recvfrom(multicast_sock, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT, (struct sockaddr *)&from, &from_len);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate the received string
        if (strcmp(buffer, "shutdown") == 0) {
            client_running = 0;
            printf("Client received shutdown message\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <PORT> [reader|writer]\n", argv[0]);
        return 1;
    }

    const char *ip_address = argv[1];
    int port = atoi(argv[2]);
    int sock, control_sock, multicast_sock;
    struct sockaddr_in server, control_addr, multicast_addr;
    struct ip_mreq mreq;

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

    control_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&control_addr, 0, sizeof(control_addr));
    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if (bind(control_sock, (struct sockaddr *)&control_addr, sizeof(control_addr)) < 0) {
        perror("Control bind failed");
        return 1;
    }

    multicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    multicast_addr.sin_port = htons(MULTICAST_PORT);

    if (bind(multicast_sock, (struct sockaddr *)&multicast_addr, sizeof(multicast_addr)) < 0) {
        perror("Multicast bind failed");
        return 1;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt failed");
        return 1;
    }

    if (strcmp(argv[3], "reader") == 0) {
        while (client_running) {
            if (!client_paused) {
                printf("Sending read request...\n");
                send_request(sock, &server, "read");
            }
            handle_commands(control_sock, multicast_sock);
            sleep(1);
        }
    } else if (strcmp(argv[3], "writer") == 0) {
        while (client_running) {
            if (!client_paused) {
                printf("Sending write request...\n");
                send_request(sock, &server, "write");
            }
            handle_commands(control_sock, multicast_sock);
            sleep(1);
        }
    } else {
        fprintf(stderr, "Invalid argument. Use 'reader' or 'writer'.\n");
        close(sock);
        return 1;
    }

    close(sock);
    close(control_sock);
    close(multicast_sock);
    printf("Client shut down gracefully.\n");
    return 0;
}
