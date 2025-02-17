import libtmux
import os
from keystroke_monitor import monitor_keystrokes 
import warnings

# Suppress warnings
warnings.filterwarnings("ignore")

# Get the tmux session name from the environment, or default to 0
tmux_session_name = os.environ.get('TMUX_SESSION', '0')

def main():
    """
    Main function to run the word prediction loop.
    """
    server = libtmux.Server()
    session = server.find_where({"session_name": "ws-1"})
    #Set config for this session
    if session:
        session.cmd("source-file","./tmux.conf")
    else:
        exit("Session not found")
    monitor_keystrokes()


if __name__ == '__main__':
    main()
