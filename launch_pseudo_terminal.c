#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

// Global variables for terminal handling
struct termios orig_termios;
int master_fd;

// Structure to maintain escape sequence state.
typedef struct {
  int state; // 0: normal; 1: ESC received (waiting for more); 2: accumulating
             // escape
  unsigned char buffer[256];
  int len;
} EscState;

// Function prototypes
void restore_terminal();
void sig_handler(int sig);
void handle_sigwinch(int sig);
int is_allowed_char(unsigned char c);
int setup_pty(char **slave_name);
void launch_tmux_and_start_monitoring(const char *slave_name);
void setup_signals();
void configure_terminal();
FILE *open_logfile();
void handle_keystrokes(int master_fd, FILE *logfile, pid_t child_pid);
void process_keyboard_buffer(EscState *esc, char *buf, ssize_t n, int master_fd,
                             FILE *logfile);
void process_normal_char(unsigned char c, int master_fd, FILE *logfile,
                         EscState *esc);
void process_escape_char(EscState *esc, unsigned char c, int master_fd,
                         FILE *logfile);
void flush_escape(EscState *esc, int master_fd, FILE *logfile);
void process_master_output(int master_fd);
void cleanup(FILE *logfile, pid_t child_pid);

// Restore original terminal settings
void restore_terminal() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

// Signal handler for termination signals
void sig_handler(int sig) {
  restore_terminal();
  exit(1);
}

// Dummy handler for SIGWINCH (window resize)
void handle_sigwinch(int sig) {
  // No extra action needed besides interrupting select()
}

// Helper: return 1 if character c is allowed to be logged.
int is_allowed_char(unsigned char c) {
  if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t' || c == 8 ||
      c == 127)
    return 1;
  return 0;
}

// Setup PTY and get the slave name.
int setup_pty(char **slave_name) {
  master_fd = posix_openpt(O_RDWR | O_NOCTTY);
  if (master_fd == -1) {
    perror("posix_openpt");
    return -1;
  }
  if (grantpt(master_fd) == -1) {
    perror("grantpt");
    close(master_fd);
    return -1;
  }
  if (unlockpt(master_fd) == -1) {
    perror("unlockpt");
    close(master_fd);
    return -1;
  }
  *slave_name = ptsname(master_fd);
  if (!*slave_name) {
    perror("ptsname");
    close(master_fd);
    return -1;
  }
  return 0;
}

// Launch tmux process in child and fork a Python process that will be killed on
// tmux exit
void launch_tmux_and_start_monitoring(const char *slave_name) 
{
    close(master_fd);
    if (setsid() == -1) { perror("setsid"); exit(1); }
    int slave_fd = open(slave_name, O_RDWR);
    if (slave_fd == -1) { perror("open slave pty"); exit(1); }
    if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) { perror("ioctl TIOCSCTTY"); exit(1); }
    if (dup2(slave_fd, STDIN_FILENO) == -1 ||
        dup2(slave_fd, STDOUT_FILENO) == -1 ||
        dup2(slave_fd, STDERR_FILENO) == -1) { perror("dup2 failed"); exit(1); }
    if (slave_fd > STDERR_FILENO) { close(slave_fd); }

    char sessionName[16];
    int created = 0;
    for (int i = 1; i <= 9; i++) {
        snprintf(sessionName, sizeof(sessionName), "ws-%d", i);
        char cmd[64];
        snprintf(cmd, sizeof(cmd), "tmux has-session -t %s 2>/dev/null", sessionName);
        if (system(cmd) != 0) { created = 1; break; }
    }
    if (!created) {
        fprintf(stderr, "All session names ws-1 to ws-9 are taken.\n");
        exit(1);
    }

    pid_t tmux_pid = fork();
    if (tmux_pid < 0) { perror("fork for tmux"); exit(1); }
    if (tmux_pid == 0) {
        execlp("tmux", "tmux", "new-session", "-A", "-s", sessionName, NULL);
        perror("execlp tmux");
        exit(1);
    }

    pid_t python_pid = fork();
    if (python_pid < 0) { perror("fork for python"); exit(1); }
    if (python_pid == 0) {
        usleep(2000000);
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) { perror("prctl"); exit(1); }
        if (getppid() == 1) exit(1);
        if (setenv("TMUX_SESSION", sessionName, 1) == -1) { perror("setenv"); exit(1); }
        execlp("python3", "python3", "start_word_prediction.py", NULL);
        perror("execlp python3");
        exit(1);
    }

    int status;
    waitpid(tmux_pid, &status, 0);
    kill(python_pid, SIGTERM);
    waitpid(python_pid, NULL, 0);
    exit(0);
}
// Setup signal handlers.
void setup_signals() {
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  struct sigaction sa_winch;
  sa_winch.sa_handler = handle_sigwinch;
  sigemptyset(&sa_winch.sa_mask);
  sa_winch.sa_flags = 0;
  sigaction(SIGWINCH, &sa_winch, NULL);
}

// Configure STDIN to raw mode.
void configure_terminal() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(restore_terminal);
  struct termios raw = orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

FILE *open_logfile() {
  const char *tmpdir = getenv("TMPDIR"); // Check if TMPDIR is set
  if (!tmpdir) {
    tmpdir = "/tmp"; // Default to /tmp if TMPDIR is not set
  }

  char filepath[256];
  snprintf(filepath, sizeof(filepath), "%s/keystrokes.log",
           tmpdir); // Construct full path

  FILE *logfile = fopen(filepath, "ab"); // Open file in append-binary mode
  if (!logfile) {
    perror("fopen");
  }
  return logfile;
}

// Flush a lone ESC when timeout occurs.
void flush_escape(EscState *esc, int master_fd, FILE *logfile) {
  if (esc->len > 0 && is_allowed_char(esc->buffer[0])) {
    fwrite(&esc->buffer[0], 1, 1, logfile);
  }
  write(master_fd, esc->buffer, 1);
  esc->state = 0;
  esc->len = 0;
}

// Process a normal (non-escape) character.
void process_normal_char(unsigned char c, int master_fd, FILE *logfile,
                         EscState *esc) {
  if (c == 27) { // Start escape sequence.
    esc->state = 1;
    esc->buffer[0] = c;
    esc->len = 1;
  } else if (c == '\n') {
    fwrite("[ENTER]\n", 1, 8, logfile);
    char cr = '\r';
    write(master_fd, &cr, 1);
  } else if (c == 127 || c == 8) {
    fwrite("[BACKSPACE]", 1, 11, logfile);
    write(master_fd, &c, 1);
  } else if (c == '\t') {
    fwrite("[TAB]", 1, 5, logfile);
    write(master_fd, &c, 1);
  } else {
    if (is_allowed_char(c))
      fwrite(&c, 1, 1, logfile);
    write(master_fd, &c, 1);
  }
}

// Process a character when already in an escape sequence.
void process_escape_char(EscState *esc, unsigned char c, int master_fd,
                         FILE *logfile) {
  if (esc->len < (int)sizeof(esc->buffer))
    esc->buffer[esc->len++] = c;

  if (esc->state == 1) {
    // Expect next character: if it is '[' or 'O' or ']', move to state 2.
    if (c == '[' || c == 'O' || c == ']')
      esc->state = 2;
    else {
      // Unrecognized sequence; forward it immediately.
      write(master_fd, esc->buffer, esc->len);
      esc->state = 0;
      esc->len = 0;
    }
  } else if (esc->state == 2) {
    // For OSC sequences: if sequence starts with ESC ']' and terminates with
    // BEL (0x07)
    if (esc->buffer[1] == ']' && c == 0x07) {
      // Ignore OSC sequence.
      esc->state = 0;
      esc->len = 0;
      return;
    }
    // For CSI/SS3 sequences, the final byte is in 0x40–0x7E.
    if (c >= 0x40 && c <= 0x7E) {
      int recognized = 0;
      char friendly[32] = "";
      // 3-byte sequences like ESC [ A or ESC O A.
      if (esc->len == 3) {
        switch (esc->buffer[2]) {
        case 'A':
          strcpy(friendly, "[UP]");
          recognized = 1;
          break;
        case 'B':
          strcpy(friendly, "[DOWN]");
          recognized = 1;
          break;
        case 'C':
          strcpy(friendly, "[RIGHT]");
          recognized = 1;
          break;
        case 'D':
          strcpy(friendly, "[LEFT]");
          recognized = 1;
          break;
        case 'H':
          strcpy(friendly, "[HOME]");
          recognized = 1;
          break;
        case 'F':
          strcpy(friendly, "[END]");
          recognized = 1;
          break;
        default:
          break;
        }
      }
      // Sequences ending with '~'
      else if (esc->len >= 4 && esc->buffer[esc->len - 1] == '~') {
        char param[16] = {0};
        int param_len = esc->len - 3;
        if (param_len > 0 && param_len < (int)sizeof(param)) {
          memcpy(param, &esc->buffer[2], param_len);
          param[param_len] = '\0';
          int val = atoi(param);
          if (val == 1) {
            strcpy(friendly, "[HOME]");
            recognized = 1;
          } else if (val == 4) {
            strcpy(friendly, "[END]");
            recognized = 1;
          } else if (val == 5) {
            strcpy(friendly, "[PAGE_UP]");
            recognized = 1;
          } else if (val == 6) {
            strcpy(friendly, "[PAGE_DOWN]");
            recognized = 1;
          }
        }
      }
      // Also check for ESC O sequences.
      if (!recognized && esc->len == 3 && esc->buffer[1] == 'O') {
        switch (esc->buffer[2]) {
        case 'A':
          strcpy(friendly, "[UP]");
          recognized = 1;
          break;
        case 'B':
          strcpy(friendly, "[DOWN]");
          recognized = 1;
          break;
        case 'C':
          strcpy(friendly, "[RIGHT]");
          recognized = 1;
          break;
        case 'D':
          strcpy(friendly, "[LEFT]");
          recognized = 1;
          break;
        case 'H':
          strcpy(friendly, "[HOME]");
          recognized = 1;
          break;
        case 'F':
          strcpy(friendly, "[END]");
          recognized = 1;
          break;
        default:
          break;
        }
      }
      // If the sequence starts with ESC [ ? or ESC ], ignore logging.
      if (esc->len >= 2 && (esc->buffer[1] == '?' || esc->buffer[1] == ']')) {
        recognized = 1;
        friendly[0] = '\0';
      }
      if (recognized && strlen(friendly) > 0) {
        fwrite(friendly, 1, strlen(friendly), logfile);
      }
      // Forward the complete escape sequence to the PTY.
      write(master_fd, esc->buffer, esc->len);
      esc->state = 0;
      esc->len = 0;
    }
  }
}

// Process the entire keyboard input buffer.
void process_keyboard_buffer(EscState *esc, char *buf, ssize_t n, int master_fd,
                             FILE *logfile) {
  for (ssize_t i = 0; i < n; i++) {
    unsigned char c = buf[i];
    if (esc->state == 0)
      process_normal_char(c, master_fd, logfile, esc);
    else
      process_escape_char(esc, c, master_fd, logfile);
  }
  fflush(logfile);
}

// Process output from master PTY to STDOUT.
void process_master_output(int master_fd) {
  char buf[256];
  ssize_t n = read(master_fd, buf, sizeof(buf));
  if (n > 0)
    write(STDOUT_FILENO, buf, n);
}

// Main loop for handling keystrokes.
void handle_keystrokes(int master_fd, FILE *logfile, pid_t child_pid) {

  fd_set readfds;
  int max_fd = (master_fd > STDIN_FILENO) ? master_fd : STDIN_FILENO;
  EscState esc = {0}; // Initialize escape state

  while (1) {
    FD_ZERO(&readfds);
    FD_SET(master_fd, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv;
    int activity;

    // Check if the child process has terminated
    int status;
    pid_t result = waitpid(child_pid, &status, WNOHANG);
    if (result > 0) {
      if (WIFEXITED(status) || WIFSIGNALED(status)) {
        // Child process has terminated
        break;
      }
    }

    // If waiting for more bytes in an escape sequence, use a short timeout.
    if (esc.state == 1) {
      tv.tv_sec = 0;
      tv.tv_usec = 1000; // 1ms timeout
      activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
      if (activity == 0) {
        // Timeout: no extra byte arrived; flush the lone ESC.
        flush_escape(&esc, master_fd, logfile);
        continue;
      }
    } else {
      activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
    }
    if (activity < 0) {
      if (errno == EINTR) {
        struct winsize ws;
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
          ioctl(master_fd, TIOCSWINSZ, &ws);
        continue;
      } else {
        perror("select");
        break;
      }
    }

    if (FD_ISSET(STDIN_FILENO, &readfds)) {
      char buf[256];
      ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
      if (n > 0) {
        process_keyboard_buffer(&esc, buf, n, master_fd, logfile);
      } else if (n == 0) {
        break; // EOF on STDIN
      } else {
        perror("read STDIN");
        break;
      }
    }
    if (FD_ISSET(master_fd, &readfds))
      process_master_output(master_fd);
  }
  cleanup(logfile, child_pid);
}

// Cleanup resources.
void cleanup(FILE *logfile, pid_t child_pid) {
  fclose(logfile);
  close(master_fd);
  waitpid(child_pid, NULL, 0);
}

// Main entry point.
int main() {
  char *slave_name;
  if (setup_pty(&slave_name) < 0)
    return 1;

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    close(master_fd);
    return 1;
  }
  if (pid == 0) {
    launch_tmux_and_start_monitoring(slave_name);
  } else {
    setup_signals();
    configure_terminal();

    FILE *logfile = open_logfile();
    if (!logfile) {
      restore_terminal();
      close(master_fd);
      return 1;
    }

    // Set initial window size.
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
      ioctl(master_fd, TIOCSWINSZ, &ws);

    handle_keystrokes(master_fd, logfile, pid);
  }
  return 0;
}
