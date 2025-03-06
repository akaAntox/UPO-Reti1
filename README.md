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
```

## How to Run

1. **Start the Server:**  
   Run the server with a specified port and an optional maximum attempts parameter (default is 6 if not specified).

   ```bash
   ./server <port> [<max-attempts>]
   ```
   
   _Example:_
   ```bash
   ./server 8080 6
   ```

2. **Start the Client:**  
   Run the client by providing the server's IP address and the port number.

   ```bash
   ./client <server-ip> <port>
   ```
   
   _Example:_
   ```bash
   ./client 127.0.0.1 8080
   ```

## How to Play

- **Welcome:** Once connected, the client receives a welcome message.
- **Gameplay:**  
  - Choose option **1** to guess the word.
  - After each guess, the server returns feedback so you can adjust your next try.
- **Quit Anytime:** Select option **2** to exit the game.

## Project Structure

- **server.c:** Contains all the server-side code for handling connections, generating a random word, and processing guesses.
- **client.c:** Contains the client-side code that manages the connection and handles user input.
