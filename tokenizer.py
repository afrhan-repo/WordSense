import re
from collections import defaultdict

class CustomTokenizer:
    def __init__(self):
        self.word_index = {}
        self.index_word = {}
        self.word_count = defaultdict(int)
        self.num_words = 0
    
    def fit_on_texts(self, texts):
        """
        Updates internal vocabulary based on the list of texts.
        """
        for text in texts:
            words = self._text_to_word_list(text)
            for word in words:
                self.word_count[word] += 1
                if word not in self.word_index:
                    self.num_words += 1
                    self.word_index[word] = self.num_words
                    self.index_word[self.num_words] = word
    
    def texts_to_sequences(self, texts):
        """
        Transforms each text in texts to a sequence of integers.
        """
        sequences = []
        for text in texts:
            words = self._text_to_word_list(text)
            sequence = [self.word_index[word] for word in words if word in self.word_index]
            sequences.append(sequence)
        return sequences
    
    def _text_to_word_list(self, text):
        """
        Converts a text into a list of words.
        """
        # Convert text to lowercase and use regex to keep only words
        text = text.lower()
        text = re.sub(r"[^a-zA-Z0-9\s]", "", text)  # Remove punctuation
        words = text.split()
        return words



