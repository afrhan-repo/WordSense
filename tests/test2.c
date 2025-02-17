#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);  // Return 1 if fork fails
    } else if (pid == 0) {
        // Child process: execute tmux
        execlp("tmux", "tmux",NULL);
        
        // If execlp fails, print error and exit with code 2
        perror("execlp failed");
        exit(2);
    } else {
        // Parent process: wait for child and check exit status
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                printf("Child process failed with exit code %d\n", exit_code);
                return exit_code;  // Return child's exit code if it failed
            }
        } else {
            printf("Child process terminated abnormally\n");
            return 3;  // Return 3 if child was terminated abnormally
        }
    }

    printf("Child executed successfully\n");
    return 0;  // Return 0 if everything was successful
}

