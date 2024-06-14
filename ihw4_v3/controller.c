#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CONTROL_PORT 9090

void send_command(const char *ip, int port, const char *command) {
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (sendto(sock, command, strlen(command), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("sendto failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    close(sock);
    printf("Command sent: %s\n", command);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <COMMAND>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_address = argv[1];
    const char *command = argv[2];

    if (strcmp(command, "start") != 0 && strcmp(command, "stop") != 0) {
        fprintf(stderr, "Invalid command. Use 'start' or 'stop'.\n");
        exit(EXIT_FAILURE);
    }

    send_command(ip_address, CONTROL_PORT, command);

    return 0;
}
