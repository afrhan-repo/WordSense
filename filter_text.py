import re

# Define the gb pattern as a global constant.
GB_PATTERN = re.compile(r'gb:[0-9a-fA-F]{4}/[0-9a-fA-F]{4}/[0-9a-fA-F]{4}')

def remove_gb_patterns_from_text(text):
    """
    Removes all occurrences of the gb pattern from the given text.
    """
    return re.sub(GB_PATTERN, '', text)

def remove_gb_patterns(file_path):
    """
    Reads a file, removes all gb patterns from its content, and writes the cleaned content back.
    """
    try:
        # Read the entire file (as binary) and decode as UTF-8.
        with open(file_path, 'rb') as f:
            content = f.read().decode('utf-8', errors='ignore')
        
        # Remove gb patterns from the content.
        cleaned_content = remove_gb_patterns_from_text(content)
        
        # Write the cleaned content back to the file (encoded as UTF-8).
        with open(file_path, 'wb') as f:
            f.write(cleaned_content.encode('utf-8'))
        
    except Exception as e:
        print("Error:", e)

def filter_line(line,file_path):
    """
    Processes a line by handling commands (like [BACKSPACE]) and then removes gb patterns.
    """
    result = []
    # Split the line into tokens: commands (like [BACKSPACE]) or individual characters.
    tokens = re.findall(r'\[.*?\]|.', line)
    for token in tokens:
        if token.startswith('[') and token.endswith(']'):
            # Extract the command inside the brackets.
            command = token[1:-1]
            if command == 'BACKSPACE':
                if result:
                    result.pop()
            # Other commands (like ENTER, Tab, etc.) are ignored.
        else:
            # Add regular characters to the result.
            result.append(token)
    
    # Join the tokens to form the processed line.
    filtered = ''.join(result)
    # Remove any gb patterns from the filtered line.
    filtered = remove_gb_patterns_from_text(filtered)
    # Write the filtered line back to the file.
    remove_gb_patterns(file_path)
    return filtered
