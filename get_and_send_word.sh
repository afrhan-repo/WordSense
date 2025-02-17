
#!/bin/bash
# Afrhan AI Assistant's Combined Script
#
# Functionality:
# 1. Find the word at the current cursor position in the tmux pane.
# 2. Delete that word by sending the backspace key (^?) as many times as there are characters.
# 3. Read the ~/.status_line file which contains a line like:
#      "              criminal             |             made             |             on"
# 4. Based on the provided argument (1, 2, or 3), pick the corresponding field (word),
#    trim any extra spaces, and then send that word into the tmux pane.
#
# Usage: ./combined_script.sh <field_number>
# Example: ./combined_script.sh 2

#-------------------------------------------------------------------
# Check if exactly one argument is provided (for the status_line field)
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <field_number>"
    exit 1
fi

#-------------------------------------------------------------------
# Part 1: Delete the word under the cursor in the tmux pane.
# Get the current cursor coordinates (tmux uses 0-based indexing for x and y)
cursor_x=$(tmux display-message -p '#{cursor_x}')
cursor_y=$(tmux display-message -p '#{cursor_y}')

# Capture the content of the pane at the current cursor row (line)
line_content=$(tmux capture-pane -S "$cursor_y" -E "$cursor_y" -p)

# We use a 1-indexed counter (current_pos) to map the characters in the line.
current_pos=1
word_found=0

# Loop over each word in the captured line (bash splits on whitespace)
for word in $line_content; do
    # Get the length of the current word
    word_length=${#word}
    
    # Calculate the start and end positions of this word.
    # The word spans from current_pos to (current_pos + word_length - 1).
    start=$current_pos
    end=$(( current_pos + word_length - 1 ))
    
    # If the tmux cursor's x coordinate falls within this word, delete it.
    if [ "$cursor_x" -ge "$start" ] && [ "$cursor_x" -le "$end" ]; then
         # Send a backspace (^?) for each character of the word.
         for ((i = 0; i < word_length; i++)); do
              tmux send-keys "^?"
         done
         word_found=1
         break  # Exit the loop once the word is deleted.
    fi
    
    # Update current_pos for the next word.
    # We assume one space between words so add word_length + 1.
    current_pos=$(( end + 2 ))
done
# (If no word was found, we simply continue without deletion.)

#-------------------------------------------------------------------
# Part 2: Read the replacement word from ~/.status_line and insert it.
# The file is expected to contain a line like:
# "              criminal             |             made             |             on"

# Read the content from the file.
status_line=$(cat ~/.status_line)

# Split the line into an array using the pipe '|' as the delimiter.
IFS='|' read -ra fields <<< "$status_line"

# A helper function to trim leading and trailing spaces from a string.
trim() {
    local var="$*"
    # Remove leading whitespace.
    var="${var#"${var%%[![:space:]]*}"}"
    # Remove trailing whitespace.
    var="${var%"${var##*[![:space:]]}"}"
    echo "$var"
}

# Convert the provided argument (e.g., 1) to the correct array index (0-indexed).
index=$(($1 - 1))

# Check that the requested index is within range.
if [ "$index" -ge 0 ] && [ "$index" -lt "${#fields[@]}" ]; then
    replacement_word=$(trim "${fields[$index]}")
    # Use tmux send-keys with the -l option to send the literal replacement word.
    tmux send-keys -l "$replacement_word"
else
    echo "Error: Argument out of range. Please provide a number between 1 and ${#fields[@]}."
    exit 1
fi
