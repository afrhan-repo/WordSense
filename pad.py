import numpy as np

def pad_sequences(token_list, maxlen=None, padding='pre'):
    """
    Pads sequences to the same length.

    Args:
        token_list (list of lists): List of integer sequences.
        maxlen (int, optional): Maximum length of all sequences after padding.
                                If None, it will use the length of the longest sequence.
        padding (str): 'pre' or 'post', pad either before or after each sequence.

    Returns:
        numpy.ndarray: Padded sequences.
    """
    # Determine the length of each sequence
    lengths = [len(seq) for seq in token_list]
    
    # Set maxlen to the length of the longest sequence if it is not provided
    if maxlen is None:
        maxlen = max(lengths)

    # Create an array of zeros with shape (number of sequences, maxlen)
    padded_sequences = np.zeros((len(token_list), maxlen), dtype=int)
    
    for i, seq in enumerate(token_list):
        if len(seq) == 0:
            continue  # Skip empty sequences

        if padding == 'pre':
            # Truncate sequence if it's longer than maxlen
            seq = seq[-maxlen:]
            # Pad with zeros at the beginning
            padded_sequences[i, -len(seq):] = seq
        elif padding == 'post':
            # Truncate sequence if it's longer than maxlen
            seq = seq[:maxlen]
            # Pad with zeros at the end
            padded_sequences[i, :len(seq)] = seq
        else:
            raise ValueError("Padding type must be 'pre' or 'post'")
    
    return padded_sequences

