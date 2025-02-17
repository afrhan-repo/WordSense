import os 
import onnxruntime as ort
import numpy as np
from pad import pad_sequences
import pickle

# Load the tokenizer from pickle.
with open('tokenizer.pkl', 'rb') as handle:
    tokenizer = pickle.load(handle, fix_imports=True)

# Create an inverse mapping: index -> word.
# This lets us convert model prediction indices back into actual words.
index_to_word = {index: word for word, index in tokenizer.word_index.items()}

# Load the ONNX model.
onnx_model_path = "lstm_model.onnx"
session = ort.InferenceSession(onnx_model_path)

def get_model_predictions(seed_text):
    """
    Given a seed text, convert it to a padded sequence and run the model.
    Returns the prediction probabilities (assumed shape: [1, vocab_size]).
    """
    # Convert the seed text into a sequence of integers.
    token_list = tokenizer.texts_to_sequences([seed_text])[0]
    # Pad the sequence (adjust maxlen as needed; here we use 17).
    token_list = pad_sequences([token_list], maxlen=17, padding='pre')
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name
    predicted = session.run([output_name], {input_name: token_list.astype(np.float32)})[0]
    # Return the probability vector (we assume shape (1, vocab_size)).
    return predicted[0]

def get_top_three(predictions, candidate_filter=None):
    """
    Given the prediction probabilities (a numpy array of shape (vocab_size,)),
    find the indices of the top 3 words.
    Optionally, if candidate_filter is provided, only consider words that satisfy
    candidate_filter(word) == True.
    
    The returned list is sorted so that probabilities increase left to right.
    """
    # Create an array of all vocabulary indices.
    indices = np.arange(len(predictions))
    
    # If a filter is provided, only keep indices whose corresponding words pass the filter.
    if candidate_filter:
        filtered = []
        for idx in indices:
            word = index_to_word.get(idx)
            if word is not None and candidate_filter(word):
                filtered.append(idx)
        indices = np.array(filtered)
        # If no candidate passes the filter, return an empty list.
        if len(indices) == 0:
            return []
    
    # Get the probabilities for the (filtered) indices.
    probs = predictions[indices]
    # Use argsort to sort by probability (lowest to highest).
    sorted_idx = np.argsort(probs)
    # Take the last three indices (which have the highest probabilities).
    top3_indices = indices[sorted_idx][-3:]
    # Sort these three indices so that the leftmost is the lowest probability.
    top3_indices = sorted(top3_indices, key=lambda idx: predictions[idx])
    # Convert indices back into words.
    top3_words = [index_to_word.get(idx, "Unknown") for idx in top3_indices]
    return top3_words

def find_non_tokenizable_words(text):
    """
    Split the input text into words and return a list of words that are not found
    in the tokenizer's vocabulary.
    """
    words = text.split()
    # We check each word in lowercase.
    non_tokenizable = [word for word in words if word.lower() not in tokenizer.word_index]
    return non_tokenizable

def get_prediction(text) -> list:
    """
    Generate prediction suggestions for the given text.
    
    - If the input text is empty, return an empty list.
    - For the first word (with no prior context) that is incomplete,
      directly search the vocabulary for words starting with the given partial word.
    - Otherwise, use the model predictions to suggest the next word.
    """
    # Remove trailing whitespace.
    seed_text = text.rstrip()
    
    # Case 1: If the input is empty, return an empty list.
    if not seed_text:
        return []
    
    # Split the text into words.
    words = seed_text.split()
    
    # Case 2: If there is only one word and the text does NOT end with a space,
    # it means the first word is still being typed.
    if len(words) == 1 and not text.endswith(" "):
        partial = words[0].lower()
        # Search the entire vocabulary for words starting with the partial string.
        candidates = [word for word in tokenizer.word_index.keys() if word.startswith(partial)]
        candidates.sort()
        return candidates[:3]
    
    # (Optional) Identify words that are not in the tokenizer's vocabulary.
    non_tokenizable = find_non_tokenizable_words(seed_text)
    
    last_word = words[-1]
    
    if not text.endswith(" "):
        # If the last word is incomplete, use all words except the last one as context.
        seed_text_without_last = " ".join(words[:-1])
        # Use model predictions if there is context; if not, use an empty string.
        predictions = get_model_predictions(seed_text_without_last) if seed_text_without_last else get_model_predictions("")
        # Define a filter to only keep words that start with the partial last word.
        partial = last_word.lower()
        candidate_filter = lambda word: word.startswith(partial)
        top3_words = get_top_three(predictions, candidate_filter=candidate_filter)
        # If no candidates are found among the model predictions,
        # fall back to directly searching the vocabulary.
        if not top3_words:
            candidates = [word for word in tokenizer.word_index.keys() if word.startswith(partial)]
            candidates.sort()
            top3_words = candidates[:3]
    else:
        # If the input ends with a space, the last word is complete.
        predictions = get_model_predictions(seed_text)
        top3_words = get_top_three(predictions)
    
    return top3_words

def store_vocabulary():
    """
    Store the vocabulary words from the tokenizer into a file named '.dict'
    in the user's home directory.
    """
    dictionary_file = ".dict"
    home_dir = os.path.expanduser("~")
    file_path = os.path.join(home_dir, dictionary_file)
    if not os.path.exists(file_path):
        with open(file_path, 'w') as f:
            for word in tokenizer.word_index.keys():
                f.write(word + "\n")

