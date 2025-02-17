#!/usr/bin/env python3
import libtmux
import os

def create_status_line(suggested_words):
    # Ensure the list always has exactly 3 elements.
    # If there are fewer than 3 words, pad with empty strings.
    # If there are more than 3 words, take only the first three.
    if len(suggested_words) < 3:
        suggested_words = suggested_words + [""] * (3 - len(suggested_words))
    else:
        suggested_words = suggested_words[:3]

    # ---------------------------
    # Step 1. Define prefix and words
    # ---------------------------
    prefix = ""

    # Get tmux window width using libtmux.
    server = libtmux.Server()
    session = server.sessions[0]  # Get the first session.
    window = session.attached_window  # Get the currently attached window.
    try:
        total_width = int(window.width)  # Convert width to an integer.
    except AttributeError:
        total_width = 80  # Fallback to 80 if width is not available.

    # ---------------------------
    # Step 2. Calculate available space
    # ---------------------------
    prefix_len = len(prefix)
    words_len = sum(len(word) for word in suggested_words)
    available_space = total_width - prefix_len - words_len

    if available_space < 6:
        gap0 = gap1 = gap2 = gap3 = 1
    else:
        x = available_space // 6
        gap0 = gap3 = x
        gap1 = gap2 = 2 * x

        assigned = gap0 + gap1 + gap2 + gap3
        remainder = available_space - assigned

        # Distribute any extra space into the four gaps.
        for index in [1, 2, 0, 3]:
            if remainder <= 0:
                break
            if index == 1:
                gap1 += 1
            elif index == 2:
                gap2 += 1
            elif index == 0:
                gap0 += 1
            elif index == 3:
                gap3 += 1
            remainder -= 1

        # Adjust gaps for better visual spacing.
        if gap1 % 2 == 0:
            if gap0 > 1:
                gap0 -= 1
                gap1 += 1
            elif gap3 > 1:
                gap3 -= 1
                gap1 += 1
        if gap2 % 2 == 0:
            if gap3 > 1:
                gap3 -= 1
                gap2 += 1
            elif gap0 > 1:
                gap0 -= 1
                gap2 += 1

    def create_inner_gap(gap_size):
        spaces = gap_size - 1
        left_spaces = (spaces + 1) // 2
        right_spaces = spaces // 2
        return " " * left_spaces + "|" + " " * right_spaces

    gap_str0 = " " * gap0
    gap_str3 = " " * gap3
    gap_str1 = create_inner_gap(gap1)
    gap_str2 = create_inner_gap(gap2)

    # Construct the final status line using the three words and four gap segments.
    output = (
        prefix +
        gap_str0 + suggested_words[0] +
        gap_str1 + suggested_words[1] +
        gap_str2 + suggested_words[2] +
        gap_str3
    )

    # Write the output to ~/.status_line.
    with open(os.path.expanduser("~/.status_line"), "w") as f:
        f.write(output)

    return output  # Return the output for further use or debugging.

if __name__ == '__main__':
    # Try with a list of two words. The function will pad this list to three elements.
    print(create_status_line(["word1", "word2"]))

