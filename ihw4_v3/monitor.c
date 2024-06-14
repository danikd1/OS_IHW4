#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MULTICAST_IP "239.255.255.250"
#define MULTICAST_PORT 1900
#define BUFFER_SIZE 256

int main() {
    int sock;
    struct sockaddr_in addr;
    struct ip_mreq mreq;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MULTICAST_PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return 1;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Setsockopt IP_ADD_MEMBERSHIP failed");
        close(sock);
        return 1;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received from server: %s\n", buffer);
    }

    if (bytes_received < 0) {
        perror("Receive failed");
    }

    close(sock);
    return 0;
}
