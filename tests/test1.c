#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char sessionName[16];   // to store the session name (e.g. "ws-1")
    char command[128];      // to build the command string for tmux
    int i;
    int ret;
    int created = 0;

    // Loop through 1 to 9 to try and create a session.
    for (i = 1; i <= 9; i++) {
        // Format the session name, e.g., "ws-1", "ws-2", ...
        snprintf(sessionName, sizeof(sessionName), "ws-%d", i);
        // Build the command to create a new detached session.
        // The "2>/dev/null" part silences any error messages.
        snprintf(command, sizeof(command), "tmux new-session -s %s 2>/dev/null", sessionName);
        
        // Call system() and check its return value.
        ret = system(command);
        if (ret == 0) {
            // If return value is 0, the session was created successfully.
            created = 1;
            break;
        }
    }

    // If no session could be created (all names already exist), report an error.
    if (!created) {
        fprintf(stderr, "Error: Could not create any tmux session (ws-1 to ws-9 already exist).\n");
        return EXIT_FAILURE;
    }

    // At this point, sessionName contains the name of the session that was created.
    // You can now pass it to another program as needed.
    printf("Created tmux session: %s\n", sessionName);
    return EXIT_SUCCESS;
}
