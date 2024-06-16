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
    int saved_errno = errno;  // Save current errno, to restore it after the handler
    while (waitpid(-1, NULL, WNOHANG) > 0);  // Reap all zombie processes
    errno = saved_errno;  // Restore errno
}

// Function to execute a command in a separate process
void RUN(char *command) {
    char *args[10]; // Array to store command and arguments
    int argc = 0;  // Argument counter
    char *token = strtok(command, " ");  // Tokenize the command string
    while (token != NULL) {  // Loop through all tokens
        args[argc++] = token;  // Store each argument
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;  // Null-terminate the array for execvp

    pid_t pid = fork();  // Create a child process
    if (pid == 0) {  // Child process
        execvp(args[0], args);  // Replace child process with new program
        fprintf(stderr, "Execution failed: %s\n", strerror(errno));  // Error handling
        exit(1);
    } else if (pid > 0) {  // Parent process
        int status;
        waitpid(pid, &status, 0);  // Wait for child process to finish
        if (WIFEXITED(status)) {  // Check if child exited normally
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {  // Check if child was killed by a signal
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    } else {
        perror("Fork failed");  // Fork failed
        exit(1);
    }
}

// Function to set up a TCP server socket
int setup_server(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    int optval = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {  // Set socket options
        perror("setsockopt");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  // Clear structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any IP
    serv_addr.sin_port = htons(port);  // Listen on the specified port

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {  // Bind address to socket
        perror("Error on binding");
        close(sockfd);
        exit(1);
    }

    listen(sockfd, 1);  // Listen for connections, max backlog queue size set to 1
    socklen_t clilen = sizeof(struct sockaddr_in);
    struct sockaddr_in cli_addr;

    printf("Press Enter to start accepting connections...");  // Prompt user
    getchar();  // Wait for user input

    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);  // Accept a connection
    if (newsockfd < 0) {
        perror("Error on accept");
        close(sockfd);
        exit(1);
    }

    return newsockfd;
}

// Function to set up a TCP client socket
int setup_client(const char *hostname, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    struct hostent *server = gethostbyname(hostname);  // Resolve hostname
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
        close(sockfd);
        exit(1);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));  // Clear structure
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);  // Set IP address
    serv_addr.sin_port = htons(port);  // Set port

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {  // Connect to server
        perror("Error connecting");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}

// Main function: Process command-line arguments and run the program
int main(int argc, char *argv[]) {
    signal(SIGCHLD, handle_sigchld);  // Install signal handler for SIGCHLD

    int sock_input = STDIN_FILENO;  // Default standard input
    int sock_output = STDOUT_FILENO;  // Default standard output
    char *command = NULL;  // Command to run

    int opt;
    while ((opt = getopt(argc, argv, "e:b:i:o:")) != -1) {  // Parse command-line options
        switch (opt) {
            case 'e':  // Command to execute
                command = optarg;
                break;
            case 'i':  // Input redirection
                if (strncmp(optarg, "TCPS", 4) == 0) {
                    int port = atoi(optarg + 4);
                    sock_input = setup_server(port);  // Set up server for input
                }
                break;
            case 'o':  // Output redirection
                if (strncmp(optarg, "TCPC", 4) == 0) {
                    char *hostname = strtok(optarg + 4, ",");
                    int port = atoi(strtok(NULL, ","));
                    sock_output = setup_client(hostname, port);  // Set up client for output
                }
                break;
            case 'b':  // Bidirectional communication
                if (strncmp(optarg, "TCPS", 4) == 0) {
                    int port = atoi(optarg + 4);
                    int sockfd = setup_server(port);
                    sock_input = sock_output = sockfd;  // Use the same socket for input and output
                }
                break;
            default:  // Invalid option
                fprintf(stderr, "Usage: %s -e <command> [-i TCPS<port>] [-o TCPC<host>,<port>] [-b TCPS<port>]\n", argv[0]);
                exit(1);
        }
    }

    if (command == NULL) {  // Ensure a command was specified
        fprintf(stderr, "Error: No command specified\n");
        exit(1);
    }

    // Redirect input and output if necessary
    if (sock_input != STDIN_FILENO) {
        if (dup2(sock_input, STDIN_FILENO) < 0) {  // Redirect standard input
            perror("dup2 input");
            close(sock_input);
            exit(1);
        }
        close(sock_input);  // Close the original socket descriptor
    }

    if (sock_output != STDOUT_FILENO) {
        if (dup2(sock_output, STDOUT_FILENO) < 0) {  // Redirect standard output
            perror("dup2 output");
            close(sock_output);
            exit(1);
        }
        close(sock_output);  // Close the original socket descriptor
    }

    RUN(command);  // Run the specified command

    return 0;
}
