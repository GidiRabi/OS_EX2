#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>

#define BUFFER_SIZE 1024

/** Execute a command provided as a string.
 * @param command_line: A command string including the program and its arguments.
 */
void execute_command(char *command_line);

/* Close open socket descriptors unless they are standard input/output.
 * @param fds: An array of socket file descriptors.
 */
void close_descriptors(int *fds);

/** Handle the SIGALRM signal for timeouts.
 * @param signal: The signal number received.
 */
void handle_timeout(int signal);

/** Set up a TCP server that listens on a specified port.
 * @param fds: An array to store the client socket descriptor.
 * @param port: The port number on which to listen for incoming connections.
 * @param dual_flag: Indicates if both input and output should be handled by the same socket.
 */
void initialize_tcp_server(int *fds, int port, char *dual_flag);

/** Establish a TCP client connection to a specified IP and port.
 * @param fds: An array to store the client socket descriptor.
 * @param ip: The IP address of the server to connect to.
 * @param port: The port number on which the server is listening.
 */
void create_tcp_client(int *fds, char *ip, int port);

/** Initialize a UDP server that listens on a specified port.
 * @param fds: An array to store the server socket descriptor.
 * @param port: The port number on which the server will listen.
 * @param timeout_duration: The duration in seconds after which the server should timeout.
 */
void setup_udp_server(int *fds, int port, int timeout_duration);

/** Configure a UDP client to connect to a specified IP and port.
 * @param fds: An array to store the client socket descriptor.
 * @param ip: The IP address of the UDP server to connect to.
 * @param port: The port number on which the server is listening.
 */
void create_udp_client(int *fds, char *ip, int port);

/** Handle data transfer between sockets.
 * @param fds: An array of socket file descriptors.
 */
void manage_data_transfer(int *fds);

/**
 * Execute external commands using fork and execvp.
 * @param command_line: A command string including the program and its arguments.
 */
void execute_command(char *command_line) {
    char *token = strtok(command_line, " ");
    if (!token) {
        fprintf(stderr, "Error: No command provided\n");
        exit(EXIT_FAILURE);
    }

    char **args = malloc(sizeof(char *));
    int argc = 0;
    while (token != NULL) {
        args[argc++] = token;
        args = realloc(args, (argc + 1) * sizeof(char *));
        token = strtok(NULL, " ");
    }
    args[argc] = NULL;

    int pid = fork();
    if (pid == 0) { // Child process
        execvp(args[0], args);
        fprintf(stderr, "Exec failed\n");
        free(args);
        exit(EXIT_FAILURE);
    } else if (pid > 0) { // Parent process
        wait(NULL);
        free(args);
    } else {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * Close any open sockets if they are not standard input/output.
 * @param fds: An array of socket file descriptors.
 */
void close_descriptors(int *fds) {
    if (fds[0] != STDIN_FILENO) close(fds[0]);
    if (fds[1] != STDOUT_FILENO) close(fds[1]);
}

/**
 * Signal handler for timeout events.
 * @param signal: The signal number received.
 */
void handle_timeout(int signal) {
    fprintf(stderr, "Timeout triggered, exiting now.\n");
    exit(EXIT_SUCCESS);
}

/**
 * Sets up a TCP server and accepts a client connection.
 * @param fds: An array to store the client socket descriptor.
 * @param port: The port number on which to listen for incoming connections.
 * @param dual_flag: Indicates if both input and output should be handled by the same socket.
 */
void initialize_tcp_server(int *fds, int port, char *dual_flag) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create TCP server socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Socket binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Socket listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("Failed to accept connection");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    fds[0] = client_socket;
    if (dual_flag != NULL) {
        fds[1] = client_socket;
    }
}

/**
 * Establish a TCP client connection to a specific IP and port.
 * @param fds: An array to store the client socket descriptor.
 * @param ip: The IP address of the server to connect to.
 * @param port: The port number on which the server is listening.
 */
void create_tcp_client(int *fds, char *ip, int port) {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("TCP client socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP client connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    fds[1] = client_socket;
}

/**
 * Initialize a UDP server that listens on a specified port.
 * @param fds: An array to store the server socket descriptor.
 * @param port: The port number on which the server will listen.
 * @param timeout_duration: The duration in seconds after which the server should timeout.
 */
void setup_udp_server(int *fds, int port, int timeout_duration) {
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("UDP server socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP server binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int bytes_received = recvfrom(server_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_len);
    if (bytes_received < 0) {
        perror("Receiving data failed on UDP server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Connecting to client failed on UDP server");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    sendto(server_socket, "ACK", 3, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));

    fds[0] = server_socket;
    if (timeout_duration > 0) {
        alarm(timeout_duration);
    }
}

/**
 * Configure a UDP client to connect to a specified IP and port.
 * @param fds: An array to store the client socket descriptor.
 * @param ip: The IP address of the UDP server to connect to.
 * @param port: The port number on which the server is listening.
 */
void create_udp_client(int *fds, char *ip, int port) {
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("UDP client socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP client connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    fds[1] = client_socket;
}

/**
 * Handle data transfer between sockets, utilizing the poll system call.
 * Monitors two sockets for incoming data and forwards data between them.
 * @param fds: An array of socket file descriptors.
 */
void manage_data_transfer(int *fds) {
    struct pollfd pfds[2];
    pfds[0].fd = fds[0];
    pfds[0].events = POLLIN;
    pfds[1].fd = fds[1];
    pfds[1].events = POLLIN;

    while (1) {
        int ret = poll(pfds, 2, -1);
        if (ret < 0) {
            perror("Polling failed");
            break;
        }

        if (pfds[0].revents & POLLIN) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = read(pfds[0].fd, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) break;
            write(pfds[1].fd, buffer, bytes_read);
        }

        if (pfds[1].revents & POLLIN) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = read(pfds[1].fd, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) break;
            write(pfds[0].fd, buffer, bytes_read);
        }
    }
}

/*
 * Main function orchestrates the setup of TCP and UDP servers and clients,
 * executes commands, and handles socket-based communication.
 */
int main(int argc, char *argv[]) {
    int fds[2] = {STDIN_FILENO, STDOUT_FILENO};

    char *exec_cmd = NULL, *bind_cmd = NULL, *input_cmd = NULL, *output_cmd = NULL, *timeout_cmd = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "e:b:i:o:t:")) != -1) {
        switch (opt) {
            case 'e':
                exec_cmd = optarg;
                break;
            case 'b':
                bind_cmd = optarg;
                break;
            case 'i':
                input_cmd = optarg;
                break;
            case 'o':
                output_cmd = optarg;
                break;
            case 't':
                timeout_cmd = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -e <command> -b <bind> -i <input> -o <output> -t <timeout>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (timeout_cmd) {
        signal(SIGALRM, handle_timeout);
        alarm(atoi(timeout_cmd));
    }

    if (input_cmd) {
        if (strncmp(input_cmd, "TCPS", 4) == 0) {
            initialize_tcp_server(fds, atoi(input_cmd + 4), bind_cmd);
        } else if (strncmp(input_cmd, "UDPS", 4) == 0) {
            setup_udp_server(fds, atoi(input_cmd + 4), timeout_cmd ? atoi(timeout_cmd) : 0);
        }
    }

    if (output_cmd) {
        if (strncmp(output_cmd, "TCPC", 4) == 0) {
            create_tcp_client(fds, strtok(output_cmd + 4, ","), atoi(strtok(NULL, ",")));
        } else if (strncmp(output_cmd, "UDPC", 4) == 0) {
            create_udp_client(fds, strtok(output_cmd + 4, ","), atoi(strtok(NULL, ",")));
        }
    }

    if (bind_cmd) {
        if (strncmp(bind_cmd, "TCPS", 4) == 0) {
            initialize_tcp_server(fds, atoi(bind_cmd + 4), bind_cmd);
        } else if (strncmp(bind_cmd, "UDPS", 4) == 0) {
            setup_udp_server(fds, atoi(bind_cmd + 4), 0);
            fds[1] = fds[0];
        }
    }

    if (exec_cmd) {
        if (fds[0] != STDIN_FILENO) {
            dup2(fds[0], STDIN_FILENO);
        }
        if (fds[1] != STDOUT_FILENO) {
            dup2(fds[1], STDOUT_FILENO);
        }
        execute_command(exec_cmd);
    } else {
        manage_data_transfer(fds);
    }

    close_descriptors(fds);
    return 0;
}
