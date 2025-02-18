# WordSense

WordSense is a word prediction tool that suggests words based on the user's input. It uses a combination of machine learning models and custom algorithms to provide accurate word predictions.

## Installation

To install WordSense, follow these steps:

1. Clone the repository:
   ```
   git clone https://github.com/afrhan-repo/WordSense.git
   ```
2. Navigate to the project directory:
   ```
   cd WordSense
   ```
3. Install the required dependencies:
   ```
   pip install -r requirements.txt
   ```

## Usage

To use WordSense, follow these steps:

1. Start the keystroke monitor:
   ```
   python keystroke_monitor.py
   ```
2. Open a new terminal and start a tmux session:
   ```
   tmux new-session -s wordsense
   ```
3. In the tmux session, run the word prediction script:
   ```
   python start_word_prediction.py
   ```

## Contributing

We welcome contributions to WordSense! If you would like to contribute, please follow these steps:

1. Fork the repository.
2. Create a new branch for your feature or bugfix:
   ```
   git checkout -b my-feature-branch
   ```
3. Make your changes and commit them:
   ```
   git commit -m "Add new feature"
   ```
4. Push your changes to your fork:
   ```
   git push origin my-feature-branch
   ```
5. Open a pull request on GitHub.

