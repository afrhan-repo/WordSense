#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

int main() {
  pid_t pid = fork();
  if (pid == -1) {
    perror("Fork failed");
    exit(1);
  } else if (pid == 0) {              // Child process
    prctl(PR_SET_PDEATHSIG, SIGTERM); // Kill child when parent dies
    execlp("python3", "pythbbon3", "script.py", NULL);
    perror("execlp failed");
    exit(-1);
  } else { // Parent process
    printf("Parent PID: %d, Child PID: %d\n", getpid(), pid);
    sleep(10); // Simulate work
    printf("Parent exiting...\n");
  }

  return 0;
}
