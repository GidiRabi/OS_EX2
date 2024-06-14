#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Execute a command by creating a new process.
 * 
 * This function takes a command string, tokenizes it into arguments,
 * forks a new process, and executes the command in the child process.
 * 
 * @param args_as_string Command string to be executed.
 */
void RUN(char *args_as_string) {
    // Split the command string into tokens
    char *token = strtok(args_as_string, " ");
    if (token == NULL) {
        fprintf(stderr, "No command provided\n");
        exit(EXIT_FAILURE);
    }

    // Create an array of arguments
    char **args = (char **)malloc(sizeof(char *));
    int n = 0;
    args[n++] = token;

    // Continue tokenizing the command string
    while (token != NULL) {
        token = strtok(NULL, " ");
        args = (char **)realloc(args, (n + 1) * sizeof(char *));
        args[n++] = token;
    }

    // Fork a new process to execute the command
    int pid = fork();  // Forking to create a new process
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    // In the child process, execute the command
    if (pid == 0) {
        execvp(args[0], args);  // Execute the command in the child process
        perror("Execution failed");  // If execvp returns, it must have failed
        free(args);  // Free the allocated memory
        exit(EXIT_FAILURE);
    } else {
        // In the parent process, wait for the child to complete
        wait(NULL);  // Wait for the child process to finish
        free(args);  // Free the allocated memory
    }
}

/**
 * @brief Close input and output descriptors if they are not standard streams.
 * 
 * This function closes the descriptors provided if they are not the standard input
 * or output descriptors.
 * 
 * @param descriptors Array containing the descriptors to close.
 */
void close_descriptors(int *descriptors) {
    if (descriptors[0] != STDIN_FILENO) {
        close(descriptors[0]);  // Close the input descriptor if it's not standard input
    }
    if (descriptors[1] != STDOUT_FILENO) {
        close(descriptors[1]);  // Close the output descriptor if it's not standard output
    }
}

/**
 * @brief Signal handler for alarm signal.
 * 
 * This function handles the SIGALRM signal to terminate the process.
 * 
 * @param sig The signal number.
 */
void handle_alarm(int sig) {
    exit(EXIT_FAILURE);  // Exit the process when the alarm signal is received
}

/**
 * @brief Setup a TCP server socket and accept a client connection.
 * 
 * This function creates a TCP server socket, binds it to a specified port,
 * listens for incoming connections, and accepts a client connection.
 * The client socket is stored in the provided descriptors array.
 * 
 * @param descriptors Array to store the input and output descriptors.
 * @param port Port number to bind the server socket.
 * @param b_flag Binding flag (can be NULL).
 * @param flag Indicates whether to set the input or output descriptor.
 */
void TCP_SERVER(int *descriptors, int port, char *b_flag, int flag) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }
    printf("TCP server socket created\n");

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Set socket option failed");
        close(server_fd);
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (flag == 0) {
        descriptors[0] = client_fd;  // Set the input descriptor to the client socket
    } else {
        descriptors[1] = client_fd;  // Set the output descriptor to the client socket
    }
    if (b_flag != NULL) {
        descriptors[1] = client_fd;  // Set the output descriptor if binding flag is set
    }
    close(server_fd);  // Close the server socket as it is no longer needed
}

/**
 * @brief Setup a TCP client socket and connect to a server.
 * 
 * This function creates a TCP client socket, connects to a specified server
 * IP address and port, and stores the socket in the provided descriptors array.
 * 
 * @param descriptors Array to store the input and output descriptors.
 * @param ip Server IP address to connect to.
 * @param port Server port number to connect to.
 * @param bvalue Binding value (can be NULL).
 * @param flag Indicates whether to set the input or output descriptor.
 */
void TCP_client(int *descriptors, char *ip, int port, char *bvalue, int flag) {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    printf("TCP client socket created\n");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("Set socket option failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (strncmp(ip, "localhost", 9) == 0) {
        ip = "127.0.0.1";  // Convert "localhost" to "127.0.0.1"
    }

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (flag == 0) {
        descriptors[1] = client_fd;  // Set the output descriptor to the client socket
    } else {
        descriptors[0] = client_fd;  // Set the input descriptor to the client socket
    }

    if (bvalue != NULL) {
        descriptors[0] = client_fd;  // Set the input descriptor if binding value is set
    }
}

/**
 * @brief Setup a UDP server socket and wait for a client message.
 * 
 * This function creates a UDP server socket, binds it to a specified port,
 * waits for an incoming message from a client, and stores the client socket
 * in the provided descriptors array.
 * 
 * @param descriptors Array to store the input and output descriptors.
 * @param port Port number to bind the server socket.
 * @param timeout Timeout value in seconds.
 * @param flag Indicates whether to set the input or output descriptor.
 */
void UDP_SERVER(int *descriptors, int port, int timeout, int flag) {
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }
    printf("UDP server socket created\n");

    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Set socket option failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int numbytes = recvfrom(server_fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    if (numbytes == -1) {
        perror("Receive failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (connect(server_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        perror("Connect to client failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    if (sendto(server_fd, "ACK", 3, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Send ACK failed");
        exit(EXIT_FAILURE);
    }

    if (flag == 0) {
        descriptors[0] = server_fd;  // Set the input descriptor to the server socket
    } else {
        descriptors[1] = server_fd;  // Set the output descriptor to the server socket
    }
    alarm(timeout);  // Set an alarm to handle timeout
}

/**
 * @brief Setup a UDP client socket and connect to a server.
 * 
 * This function creates a UDP client socket, connects to a specified server
 * IP address and port, and stores the socket in the provided descriptors array.
 * 
 * @param descriptors Array to store the input and output descriptors.
 * @param ip Server IP address to connect to.
 * @param port Server port number to connect to.
 * @param flag Indicates whether to set the input or output descriptor.
 */
void UDP_CLIENT(int *descriptors, char *ip, int port, int flag) {
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("UDP client socket created\n");

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect to server failed");
        exit(EXIT_FAILURE);
    }

    char *message = "Let's play!\n";
    if (sendto(client_fd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Send message failed");
        exit(EXIT_FAILURE);
    }

    if (flag == 0) {
        descriptors[1] = client_fd;  // Set the output descriptor to the client socket
    } else {
        descriptors[0] = client_fd;  // Set the input descriptor to the client socket
    }
}

/**
 * @brief Setup a Unix Domain Socket (UDS) server for stream communication.
 * 
 * This function creates a Unix Domain Socket (UDS) server, binds it to a specified
 * file path, listens for incoming connections, and accepts a client connection.
 * The client socket is stored in the provided descriptors array.
 * 
 * @param path File path to bind the UDS server.
 * @param descriptors Array to store the input and output descriptors.
 */
void UDS_SERVER_STREAM(char *path, int *descriptors) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        close_descriptors(descriptors);
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("UDS server socket created\n");

    struct sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    unlink(path);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    printf("UDS server socket bound\n");

    if (listen(server_fd, 1) == -1) {
        perror("Listen failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    printf("UDS server listening\n");

    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    printf("UDS server accepted connection\n");

    descriptors[0] = client_fd;  // Set the input descriptor to the client socket
}

/**
 * @brief Setup a Unix Domain Socket (UDS) client for stream communication.
 * 
 * This function creates a Unix Domain Socket (UDS) client, connects to a specified
 * server file path, and stores the socket in the provided descriptors array.
 * 
 * @param path File path of the UDS server to connect to.
 * @param descriptors Array to store the input and output descriptors.
 */
void UDS_CLIENT_STREAM(char *path, int *descriptors) {
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Socket creation failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    printf("UDS client socket created\n");

    struct sockaddr_un server_addr = {0};
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, path, sizeof(server_addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect to server failed");
        close_descriptors(descriptors);
        exit(EXIT_FAILURE);
    }

    descriptors[1] = client_fd;  // Set the output descriptor to the client socket
}

/**
 * @brief Main function to handle command-line arguments and execute corresponding actions.
 * 
 * This function processes command-line arguments, sets up necessary sockets (TCP, UDP, UDS),
 * handles optional timeout, and manages input/output redirection for executing commands.
 * 
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int opt;
    char *evalue = NULL;
    char *bvalue = NULL;
    char *ivalue = NULL;
    char *ovalue = NULL;
    char *tvalue = NULL;

    while ((opt = getopt(argc, argv, "e:b:i:o:t:")) != -1) {
        switch (opt) {
            case 'e':
                evalue = optarg;
                break;
            case 'b':
                bvalue = optarg;
                break;
            case 'i':
                ivalue = optarg;
                break;
            case 'o':
                ovalue = optarg;
                break;
            case 't':
                tvalue = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s <port>\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (tvalue != NULL) {
        signal(SIGALRM, handle_alarm);  // Set the alarm signal handler
        alarm(atoi(tvalue));  // Set the alarm with the given timeout value
    }

    int descriptors[2] = {STDIN_FILENO, STDOUT_FILENO};

    if (bvalue != NULL && (ivalue != NULL || ovalue != NULL)) {
        fprintf(stderr, "Option -b cannot be used with -i or -o\n");
        fprintf(stderr, "Usage: %s -e <value> [-b <value>] [-i <value>] [-o <value>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (ivalue != NULL) {
        printf("Processing -i option: %s\n", ivalue);
        if (strncmp(ivalue, "TCPS", 4) == 0) {
            ivalue += 4;
            int port = atoi(ivalue);
            TCP_SERVER(descriptors, port, NULL, 0);  // Setup TCP server
        } else if (strncmp(ivalue, "UDPS", 4) == 0) {
            ivalue += 4;
            int port = atoi(ivalue);
            if (tvalue != NULL) {
                UDP_SERVER(descriptors, port, atoi(tvalue), 0);  // Setup UDP server with timeout
            } else {
                UDP_SERVER(descriptors, port, 0, 0);  // Setup UDP server without timeout
            }
        } else if (strncmp(ivalue, "UDSSS", 5) == 0) {
            ivalue += 5;
            printf("Unix Domain Socket Server Path: %s\n", ivalue);
            UDS_SERVER_STREAM(ivalue, descriptors);  // Setup UDS server
        } else if (strncmp(ivalue, "UDSCS", 5) == 0) {
            ivalue += 5;
            UDS_CLIENT_STREAM(ivalue, descriptors);  // Setup UDS client
            descriptors[0] = descriptors[1];
            descriptors[1] = STDOUT_FILENO;
        } else if (strncmp(ivalue, "TCPC", 4) == 0) {
            ivalue += 4;
            char *ip_server = strtok(ivalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            TCP_client(descriptors, ip_server, port, NULL, 1);  // Setup TCP client
        } else if (strncmp(ivalue, "UDPC", 4) == 0) {
            ivalue += 4;
            char *ip_server = strtok(ivalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            UDP_CLIENT(descriptors, ip_server, port, 1);  // Setup UDP client
        } else {
            fprintf(stderr, "Invalid -i value\n");
            exit(EXIT_FAILURE);
        }
    }

    if (ovalue != NULL) {
        printf("Processing -o option: %s\n", ovalue);
        if (strncmp(ovalue, "TCPC", 4) == 0) {
            ovalue += 4;
            char *ip_server = strtok(ovalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            TCP_client(descriptors, ip_server, port, NULL, 0);  // Setup TCP client
        } else if (strncmp(ovalue, "UDPC", 4) == 0) {
            ovalue += 4;
            char *ip_server = strtok(ovalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            UDP_CLIENT(descriptors, ip_server, port, 0);  // Setup UDP client
        } else if (strncmp(ovalue, "UDSCS", 5) == 0) {
            ovalue += 5;
            UDS_CLIENT_STREAM(ovalue, descriptors);  // Setup UDS client
        } else if (strncmp(ovalue, "TCPS", 4) == 0) {
            ovalue += 4;
            int port = atoi(ovalue);
            TCP_SERVER(descriptors, port, NULL, 1);  // Setup TCP server
        } else if (strncmp(ovalue, "UDPS", 4) == 0) {
            ovalue += 4;
            int port = atoi(ovalue);
            if (tvalue != NULL) {
                UDP_SERVER(descriptors, port, atoi(tvalue), 1);  // Setup UDP server with timeout
            } else {
                UDP_SERVER(descriptors, port, 0, 1);  // Setup UDP server without timeout
            }
        } else if (strncmp(ovalue, "UDSSS", 5) == 0) {
            ovalue += 5;
            UDS_SERVER_STREAM(ovalue, descriptors);  // Setup UDS server
            descriptors[1] = descriptors[0];
            descriptors[0] = STDIN_FILENO;
        } else {
            fprintf(stderr, "Invalid -o value\n");
            close_descriptors(descriptors);
            exit(EXIT_FAILURE);
        }
    }

    if (bvalue != NULL) {
        printf("Processing -b option: %s\n", bvalue);
        if (strncmp(bvalue, "TCPS", 4) == 0) {
            bvalue += 4;
            int port = atoi(bvalue);
            TCP_SERVER(descriptors, port, bvalue, 0);  // Setup TCP server
        } else if (strncmp(bvalue, "TCPC", 4) == 0) {
            bvalue += 4;
            char *ip_server = strtok(bvalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            TCP_client(descriptors, ip_server, port, bvalue, 0);  // Setup TCP client
        } else if (strncmp(bvalue, "UDPC", 4) == 0) {
            bvalue += 4;
            char *ip_server = strtok(bvalue, ",");
            if (ip_server == NULL) {
                fprintf(stderr, "Invalid server IP\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            char *port_server = strtok(NULL, ",");
            if (port_server == NULL) {
                fprintf(stderr, "Invalid server port\n");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
            int port = atoi(port_server);
            UDP_CLIENT(descriptors, ip_server, port, 0);  // Setup UDP client
            descriptors[0] = descriptors[1];
        } else if (strncmp(bvalue, "UDPS", 4) == 0) {
            bvalue += 4;
            int port = atoi(bvalue);
            UDP_SERVER(descriptors, port, 0, 0);  // Setup UDP server
            descriptors[1] = descriptors[0];
        } else if (strncmp(bvalue, "UDSSS", 5) == 0) {
            bvalue += 5;
            UDS_SERVER_STREAM(bvalue, descriptors);  // Setup UDS server
            descriptors[1] = descriptors[0];
        } else if (strncmp(bvalue, "UDSCS", 5) == 0) {
            bvalue += 5;
            UDS_CLIENT_STREAM(bvalue, descriptors);  // Setup UDS client
            descriptors[0] = descriptors[1];
        } else {
            fprintf(stderr, "Invalid -b value\n");
            close_descriptors(descriptors);
            exit(EXIT_FAILURE);
        }
    }

    if (evalue != NULL) {
        printf("Executing command: %s\n", evalue);
        if (descriptors[0] != STDIN_FILENO) {
            if (dup2(descriptors[0], STDIN_FILENO) == -1) {
                close(descriptors[0]);
                if (descriptors[1] != STDOUT_FILENO) {
                    close(descriptors[1]);
                }
                perror("dup2 input failed");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
        }
        if (descriptors[1] != STDOUT_FILENO) {
            if (dup2(descriptors[1], STDOUT_FILENO) == -1) {
                close(descriptors[1]);
                if (descriptors[0] != STDIN_FILENO) {
                    close(descriptors[0]);
                }
                perror("dup2 output failed");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }
        }
        RUN(evalue);  // Execute the command
    } else {
        printf("No command provided for execution\n");
        struct pollfd fds[4] = {
            {descriptors[0], POLLIN, 0},
            {descriptors[1], POLLIN, 0},
            {STDIN_FILENO, POLLIN, 0},
            {STDOUT_FILENO, POLLIN, 0}
        };

        while (1) {
            int ret = poll(fds, 4, -1);
            if (ret == -1) {
                perror("Poll failed");
                close_descriptors(descriptors);
                exit(EXIT_FAILURE);
            }

            if (bvalue == NULL && (fds[0].revents & POLLIN)) {
                char buffer[1024];
                int bytes_read = read(fds[0].fd, buffer, sizeof(buffer));
                if (bytes_read == -1) {
                    perror("Read failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
                if (bytes_read == 0) {
                    break;
                }
                if (write(fds[1].fd, buffer, bytes_read) == -1) {
                    perror("Write failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
            }

            if (bvalue != NULL && (fds[1].revents & POLLIN)) {
                char buffer[1024];
                int bytes_read = read(fds[1].fd, buffer, sizeof(buffer));
                if (bytes_read == -1) {
                    perror("Read failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
                if (bytes_read == 0) {
                    break;
                }
                if (write(fds[3].fd, buffer, bytes_read) == -1) {
                    perror("Write failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
            }

            if (bvalue != NULL && (fds[2].revents & POLLIN)) {
                char buffer[1024];
                int bytes_read = read(fds[2].fd, buffer, sizeof(buffer));
                if (bytes_read == -1) {
                    perror("Read failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
                if (bytes_read == 0) {
                    break;
                }
                if (write(descriptors[1], buffer, bytes_read) == -1) {
                    perror("Write failed");
                    close_descriptors(descriptors);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(descriptors[0]);
    close(descriptors[1]);

    return 0;
}
