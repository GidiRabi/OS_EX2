#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // Check if the number of arguments is correct and if the first argument is "-e"
    if (argc != 3 || strcmp(argv[1], "-e") != 0) {
        // If not, print usage information and exit with status 1
        fprintf(stderr, "Usage: %s -e command\n", argv[0]);
        return 1;
    }

    // Create a new process by forking
    pid_t pid = fork();
    // If fork failed, print an error message and exit
    if (pid < 0) {
        perror("Error");
        exit(1);
    } else if (pid == 0) {
        // Child process
        // Redirect standard input and output to the current process
        dup2(0, STDIN_FILENO);
        dup2(1, STDOUT_FILENO);

        // Split the command and its arguments into separate strings
        char *cmd = strtok(argv[2], " ");
        char *arg = strtok(NULL, " ");
        // Create an array of arguments for execvp
        char *args[] = {cmd, arg, NULL};
        // Replace the current process image with a new process image
        execvp(cmd, args);

        // If execvp returns, there was an error
        perror("exec failed");
        return 1;
    } else {
        // Parent process
        // Wait for the child process to finish
        int status;
        waitpid(pid, &status, 0);
        // Check how the child process exited and print the exit status or the signal that killed it
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        }
    }

    // Return 0 to indicate successful execution
    return 0;
}