# C Word Guessing Game

A simple client-server word guessing game written in C using TCP sockets. The server selects a random 5-letter word from a predefined list, and the client gets a set number of attempts to guess it correctly. After each guess, the server provides feedback using symbols to indicate letter correctness.

## Features

- **Client-Server Architecture:** Uses TCP sockets for communication.
- **Word Guessing:** Guess the secret 5-letter word chosen by the server.
- **Feedback System:**  
  - `*` indicates a letter is correct and in the right position.  
  - `+` indicates the letter exists but in a different spot.  
  - `-` indicates the letter isn’t in the word.
- **Colored Output:** Terminal messages are color-coded for errors, warnings, and success.
- **Robust Error Handling:** Both client and server validate messages to ensure smooth gameplay.

## How to Compile

Compile the server and client using `gcc`:

```bash
gcc -o server server.c
gcc -o client client.c
