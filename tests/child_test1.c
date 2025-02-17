#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Fork failed");
        exit(1);
    } else if (pid == 0) {  // Child process
	printf("Hey");
        setpgid(0, 0);  // Set child to a new process group
        execlp("python3", "python3", "script.py", NULL);
        perror("execlp failed");
        exit(1);
    } else {  // Parent process
        printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);
        sleep(10);  // Simulate work
        printf("Parent exiting... Killing child...\n");
        kill(-pid, SIGTERM);  // Kill all in the process group
    }

    return 0;
}

