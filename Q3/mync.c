#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>


// Signal handler for SIGCHLD
void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);  // Clean up all zombie processes
    errno = saved_errno;
}

// Helper function to run commands
void RUN(char *command) {
    char *args[10]; 
    int argc = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }
    args[argc] = NULL; // Null terminate the array

    pid_t pid = fork();
    if (pid == 0) { // Child process
        execvp(args[0], args);
        fprintf(stderr, "Execution failed: %s\n", strerror(errno));
        exit(1);
    } else if (pid > 0) { // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    } else {
        perror("Fork failed");
        exit(1);
    }
}

// Function to setup a TCP server
int setup_server(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error on binding");
        close(sockfd);
        exit(1);
    }

    listen(sockfd, 1);
    socklen_t clilen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr;

    // Prompt user to press Enter to start accepting connections
    printf("Press Enter to start accepting connections...");
    getchar();  // Wait for user input before continuing to accept connections

    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0) {
        perror("Error on accept");
        close(sockfd);
        exit(1);
    }

    return newsockfd;
}

// Function to set up a TCP client
int setup_client(const char *hostname, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}

// Main function enhanced to handle sockets correctly and run external commands
int main(int argc, char *argv[]) {
    signal(SIGCHLD, handle_sigchld);  // Set signal handler for SIGCHLD

    int sock_input = STDIN_FILENO;
    int sock_output = STDOUT_FILENO;
    char *command = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "e:b:i:o:")) != -1) {
        switch (opt) {
            case 'e':
                command = optarg;
                break;
            case 'i':
                if (strncmp(optarg, "TCPS", 4) == 0) {
                    int port = atoi(optarg + 4);
                    sock_input = setup_server(port);
                }
                break;
            case 'o':
                if (strncmp(optarg, "TCPC", 4) == 0) {
                    char *hostname = strtok(optarg + 4, ",");
                    int port = atoi(strtok(NULL, ","));
                    sock_output = setup_client(hostname, port);
                }
                break;
            case 'b':
                if (strncmp(optarg, "TCPS", 4) == 0) {
                    int port = atoi(optarg + 4);
                    int sockfd = setup_server(port);
                    sock_input = sock_output = sockfd;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s -e <command> [-i TCPS<port>] [-o TCPC<host>,<port>] [-b TCPS<port>]\n", argv[0]);
                exit(1);
        }
    }

    if (command == NULL) {
        fprintf(stderr, "Error: No command specified\n");
        exit(1);
    }

    if (sock_input != STDIN_FILENO) {
        if (dup2(sock_input, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(sock_input);
            exit(1);
        }
        close(sock_input);
    }

    if (sock_output != STDOUT_FILENO) {
        if (dup2(sock_output, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(sock_output);
            exit(1);
        }
        close(sock_output);
    }

    RUN(command);

    return 0;
}
