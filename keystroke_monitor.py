import os
import time
import hashlib

# Import your modules (make sure these files are in your current working directory)
from filter_text import filter_line
from word_prediction import get_prediction
from even_spacing import create_status_line

def calculate_file_hash(file_path):
    """
    Calculates the SHA1 hash of the given file.
    Returns the hash as a hexadecimal string.
    """
    try:
        with open(file_path, "rb") as f:
            file_data = f.read()
            return hashlib.sha1(file_data).hexdigest()
    except Exception as e:
        print(f"Error calculating hash for {file_path}: {e}")
        return None

def monitor_keystrokes():
    """
    Monitors the keystroke log file for changes by checking its hash.
    When the hash changes, it reads the file and:
      - Extracts the current (incomplete) line.
      - If the incomplete line has changed, calls filter_line, then get_prediction,
        and finally create_status_line.
      - If the file ends with a newline, calls remove_gb_patterns.
    """
    # Determine the log file path. Use TMPDIR if set; otherwise, default to /tmp.
    temp_dir = os.environ.get('TMPDIR', '/tmp')
    keystroke_log = os.path.join(temp_dir, "keystrokes.log")

    # Ensure the directory exists and create the file if it doesn't exist.
    os.makedirs(os.path.dirname(keystroke_log), exist_ok=True)
    if not os.path.exists(keystroke_log):
        open(keystroke_log, "w").close()

    # Calculate the initial hash and initialize the last incomplete line.
    while True:
        current_hash = calculate_file_hash(keystroke_log)
        time.sleep(0.2)
        new_hash = calculate_file_hash(keystroke_log)
        if new_hash != current_hash:
            current_hash = new_hash
            with open(keystroke_log, "r") as f:
                last_line = f.read().splitlines()[-1]
                filtered_line = filter_line(last_line,keystroke_log)
                prediction = get_prediction(filtered_line)
                create_status_line(prediction)
                os.system("tmux refresh -S")

                

if __name__ == "__main__":
    monitor_keystrokes()
