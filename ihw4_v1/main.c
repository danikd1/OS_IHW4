#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define DB_SIZE 10
#define BUFFER_SIZE 256

typedef struct {
    int data[DB_SIZE];
    pthread_mutex_t mutex;
} SharedMemory;

int server_fd;
SharedMemory *shm;
volatile sig_atomic_t server_running = 1;

void handle_signal(int signum) {
    server_running = 0;
    printf("Server shutting down...\n");
    close(server_fd);
    shm_unlink("/shm_database");
    exit(0);
}

// Функция вычисления факториала
unsigned long long factorial(int n) {
    if (n > 20) return 0;  // Предотвращение переполнения
    unsigned long long result = 1;
    for (int i = 1; i <= n; ++i) {
        result *= i;
    }
    return result;
}

// Обработчик запросов клиента
void handle_client(int sock, struct sockaddr_in client_addr, socklen_t client_len, char *buffer) {
    srand(getpid());  // Уникальное зерно для каждого процесса
    char response[BUFFER_SIZE];
    int idx = rand() % DB_SIZE;  // Генерация случайного индекса

    pthread_mutex_lock(&shm->mutex);
    int value = shm->data[idx];
    pthread_mutex_unlock(&shm->mutex);

    if (strncmp(buffer, "read", 4) == 0) {
        unsigned long long fact = factorial(value);
        snprintf(response, sizeof(response), "Reader %d: index %d, value %d, factorial %llu", getpid(), idx, value, fact);
        sendto(sock, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len);
    } else if (strncmp(buffer, "write", 5) == 0) {
        int old_value = value;
        int new_value = rand() % 20 + 1;  // Новое значение не больше 20

        pthread_mutex_lock(&shm->mutex);
        shm->data[idx] = new_value;
        pthread_mutex_unlock(&shm->mutex);

        snprintf(response, sizeof(response), "Writer %d: index %d, old value %d, new value %d", getpid(), idx, old_value, new_value);
        sendto(sock, response, strlen(response), 0, (struct sockaddr *)&client_addr, client_len);
    }

    // Debugging information
    pthread_mutex_lock(&shm->mutex);
    pthread_mutex_unlock(&shm->mutex);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP_ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip_address = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(struct sockaddr_in);
    char buffer[BUFFER_SIZE];

    // Setup signal handler for cleanup
    signal(SIGINT, handle_signal);

    // Setup shared memory
    int shm_fd = shm_open("/shm_database", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(SharedMemory));
    shm = mmap(NULL, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initialize mutex
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shm->mutex, &attr);

    // Initialize shared data
    srand(time(NULL));  // Initialize random seed
    for (int i = 0; i < DB_SIZE; i++) {
        shm->data[i] = rand() % 20 + 1;
    }

    // Print initial values for debugging
    printf("Initial values in shared memory:\n");
    for (int i = 0; i < DB_SIZE; i++) {
        printf("Index %d: %d\n", i, shm->data[i]);
    }

    // Setup socket
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        perror("Could not create socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is waiting for incoming connections on %s:%d...\n", ip_address, port);

    while (server_running) {
        int n = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client, &client_len);
        if (n < 0) {
            if (!server_running) {
                break;
            }
            perror("recvfrom failed");
            continue;
        }
        buffer[n] = '\0';

        pid_t pid = fork();
        if (pid == 0) {
            handle_client(server_fd, client, client_len, buffer);
            exit(0);
        } else if (pid > 0) {
            // Non-blocking waitpid to clean up finished child processes
            while (waitpid(-1, NULL, WNOHANG) > 0);
        } else {
            perror("fork failed");
        }
    }

    // Wait for all child processes to terminate
    while (wait(NULL) > 0);

    // Cleanup
    close(server_fd);
    shm_unlink("/shm_database");
    return 0;
}





