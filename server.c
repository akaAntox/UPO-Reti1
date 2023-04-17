#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <ctype.h> // isspace(), isalpha()
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()

#define MAX 256
#define MAX_CONNECTIONS 5
#define WORD_LENGTH 5

#define LISTENING 1
#define CONNECTION_OPENED 1
#define CONNECTION_CLOSED 0

#define WELCOME_MESSAGE "Welcome on the server!\n"
#define ERROR_MALFORMED_MESSAGE "ERR Malformed command!\n"
#define ERROR_WRONG_LENGTH "ERR Word is not 5 letters!\n"
#define ERROR_CHAR_NOT_ALPHA "ERR Word is not alphabetic!\n"
#define ERROR_DOUBLE_SPACE "ERR Double space present!\n"
#define ERROR_WRONG_MESSAGE "ERR Wrong command!\n"
#define PERFECT_MESSAGE "OK PERFECT\n"

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

const char *words[] = {"fessa", "caldo", "mucca", "leale", "pasto", "adori"};

typedef enum enum_commands
{
    QUIT, // leave execution
    WORD  // client guesses word
} commands;

typedef struct sockaddr_in *sockaddr_t;

/// @brief prints red string
void print_error(const char *string)
{
    fprintf(stderr, COLOR_RED "%s\n" COLOR_RESET, string);
    fflush(stderr);
}

/// @brief prints yellow string
void print_warning(const char *string)
{
    fprintf(stderr, COLOR_YELLOW "%s\n" COLOR_RESET, string);
    fflush(stderr);
}

/// @brief prints green string
void print_success(const char *string)
{
    fprintf(stderr, COLOR_GREEN "%s\n" COLOR_RESET, string);
    fflush(stderr);
}

/// @brief if there's an incoming connection on the socket handle it
/// @return connection socket
int accept_connection(int mySocket, sockaddr_t myClientName, int attempts)
{
    int clientLength = sizeof(*myClientName);                                                               // client name length
    int myConnectionSocket = accept(mySocket, (struct sockaddr *)myClientName, (socklen_t *)&clientLength); // accept incoming connection on socket

    if (myConnectionSocket < 0) // if the connection is succesfully accepted
    {
        print_error("Server accept failed...");
        close(mySocket); // close socket
        exit(EXIT_FAILURE);
    }
    else
    {
        fprintf(stderr, COLOR_YELLOW "\nClient connected with address %s\n" COLOR_RESET, inet_ntoa((*myClientName).sin_addr));
        fflush(stderr);

        char welcomeMessage[MAX] = "";
        sprintf(welcomeMessage, "OK %d %s", attempts, WELCOME_MESSAGE);    // prepare welcome message
        write(myConnectionSocket, welcomeMessage, strlen(welcomeMessage)); // send message to client
    }

    return myConnectionSocket;
}

/// @brief search space position in given string
/// @return first space position
int search_space_pos(const char *string)
{
    size_t space_pos = 0;
    for (size_t i = 0; string[i] != ' ' && space_pos < strlen(string); i++)
        space_pos = i + 1;

    if (string[space_pos] == ' ')
        return space_pos;
    else
        return -1;
}

/// @brief compare command and value
/// @param string message to compare
/// @param cmd expected command
/// @param val given value
/// @return value if equal, -1 otherwise
commands cmdcmp(const char *string, const char *cmd, commands val)
{
    size_t size = strlen(cmd);
    int result = strncmp(string, cmd, size);
    if (!result)
        return val;
    return -1;
}

/// @brief retrieves command and message from the given string
int retrieve_message(const char *string, commands *cmd, char **msg)
{
    if (cmdcmp(string, "WORD", WORD) == WORD)
    {
        *cmd = WORD;

        int space = search_space_pos(string); // search first space char
        if (space != -1)
        {
            if (isspace(string[space]) && isspace(string[space + 1]))
                return 2; // double space

            int wordLength = 0;
            *msg = malloc(sizeof(strlen(string) - 4));
            for (int i = space + 1; *(string + i) != '\n' && i < MAX; i++) // retrieve world after "WORD "
                if (isalpha(string[i]))                                    // if alphabetic character
                {
                    *(*msg + (i - (space + 1))) = string[i]; // store it
                    wordLength++;
                }
                else
                    return 3; // word is not alphabetic

            if (wordLength != WORD_LENGTH)
                return 4; // wrong length
        }
        else
            return 0; // malformed message
    }
    else if (cmdcmp(string, "QUIT\n", QUIT) == QUIT)
        *cmd = QUIT;
    else
        return -1; // wrong command sent
    return 1;
}

/// @brief compares word with reference and substitutes the letter with specific symbol
/// @param serverWord reference word
/// @param userWord compared word
/// @return processed word
char *check_word(const char *serverWord, char *userWord)
{
    char *result = malloc(WORD_LENGTH);

    for (size_t i = 0; i < WORD_LENGTH; i++)
        if (serverWord[i] == userWord[i]) // correct letter
            result[i] = '*';
        else if (strchr(serverWord, userWord[i]) != NULL) // correct letter wrong position
            result[i] = '+';
        else // incorrect letter
            result[i] = '-';

    return result;
}

/// @brief generate random word for client to guess and return it
const char *generate_random_word()
{
    int n = sizeof(words) / sizeof(words[0]);     // get number of words in the array
    srand(time(NULL));                            // initialize random number generator
    int randomIndex = rand() % n;                 // generate randon number between 0 and n-1
    const char *wordToGuess = words[randomIndex]; // store the random word

    return wordToGuess;
}

/// @brief chat between client and server
/// @param maxAttempts number of max-attempts
void chat(int maxAttempts, int mySocket)
{
    int actual_state = LISTENING;

    // infinite loop for chat
    while (actual_state == LISTENING)
    {
        struct sockaddr_in clientName = {0};

        int myConnectionSocket = accept_connection(mySocket, &clientName, maxAttempts); // accept incoming connection
        const char *wordToGuess = generate_random_word();
        int attempts = 1;

        int connectionStatus = CONNECTION_OPENED;
        while (connectionStatus == CONNECTION_OPENED)
        {
            char buffer[MAX];
            bzero(buffer, MAX);                                                    // clear buffer
            int responseStatus = read(myConnectionSocket, buffer, sizeof(buffer)); // read the message from client

            if (responseStatus > 0) // if you read a message
            {
                fprintf(stderr, "From client: %s\n", buffer); // print client message
                fflush(stderr);

                // store message
                commands cmd;               // command received
                char *clientMessage = NULL; // message sent by client
                int messageCorrect = retrieve_message(buffer, &cmd, &clientMessage);
                bzero(buffer, MAX); // clear buffer

                if (messageCorrect == 1) // message correct
                {
                    if (cmd == QUIT)
                        connectionStatus = CONNECTION_CLOSED;
                    else // handle client WORD message
                    {
                        char *guessWord = check_word(wordToGuess, clientMessage); // process word attempt by client
                        if (strcmp(guessWord, "*****") == 0)                      // if string is correct
                        {
                            sprintf(buffer, PERFECT_MESSAGE); // prepare message for client
                            connectionStatus = CONNECTION_CLOSED;
                        }
                        else
                            sprintf(buffer, "OK %d %s\n", attempts, guessWord); // prepare message for client

                        free(clientMessage);
                        free(guessWord);

                        write(myConnectionSocket, buffer, sizeof(buffer)); // write word to server
                        fprintf(stderr, "From server: %s", buffer);        // print server message
                        fflush(stderr);
                        bzero(buffer, MAX); // clear buffer

                        if (attempts == maxAttempts) // if last attempt
                        {
                            sprintf(buffer, "END %d %s\n", attempts, wordToGuess); // prepare message for client
                            write(myConnectionSocket, buffer, sizeof(buffer));     // write word to server
                            fprintf(stderr, "From server: %s", buffer);            // print server message
                            fflush(stderr);
                            bzero(buffer, MAX); // clear buffer
                            connectionStatus = CONNECTION_CLOSED;
                        }
                        else
                            attempts++;
                    }

                    if (connectionStatus == CONNECTION_CLOSED) // if word is correct or no more attempts
                    {
                        sprintf(buffer, "QUIT Succesfully disconnected.. The word was \'%s\' \n", wordToGuess); // prepare message for client
                        write(myConnectionSocket, buffer, sizeof(buffer));                                      // write word to server
                        fprintf(stderr, "From server: %s", buffer);                                             // print server message
                        fflush(stderr);
                        bzero(buffer, MAX); // clear buffer
                    }
                }
                else // handle errors
                {
                    switch (messageCorrect)
                    {
                    case -1:                                  // wrong command sent
                        sprintf(buffer, ERROR_WRONG_MESSAGE); // prepare error for client
                        break;
                    case 2:                                  // double space in received string
                        sprintf(buffer, ERROR_DOUBLE_SPACE); // prepare error for client
                        break;
                    case 3:                                    // word not alphabetic
                        sprintf(buffer, ERROR_CHAR_NOT_ALPHA); // prepare error for client
                        break;
                    case 4:                                  // wrong length
                        sprintf(buffer, ERROR_WRONG_LENGTH); // prepare error for client
                        break;
                    default:
                        sprintf(buffer, ERROR_MALFORMED_MESSAGE); // prepare error for client
                        break;
                    }

                    write(myConnectionSocket, buffer, sizeof(buffer)); // send error to client
                    fprintf(stderr, "From server: ");                  // print server message
                    print_error(buffer);
                    bzero(buffer, MAX); // clear buffer
                    connectionStatus = CONNECTION_CLOSED;
                }

                if (connectionStatus == CONNECTION_CLOSED)
                    close(myConnectionSocket); // close connection
            }
        }
    }
}

/// @brief create socket
/// @return resulting socket
int create_socket()
{
    int mySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (mySocket < 0)
    {
        print_error("Socket creation failed...");
        exit(EXIT_FAILURE);
    }
    else
        print_success("Socket succesfully created..");

    return mySocket;
}

/// @brief bind socket to sever address
void bind_socket(int mySocket, struct sockaddr *serverAddress)
{
    int responseStatus = bind(mySocket, serverAddress, sizeof(*serverAddress));

    if (responseStatus < 0)
    {
        print_error("Errore nel bind del socket");
        exit(EXIT_FAILURE);
    }
    else
        print_success("Socket successfully binded..");
}

/// @brief create server and assign domain, address and port
/// @return resulting server
sockaddr_t init_server(int domain, in_addr_t address, int port)
{
    sockaddr_t myServer = malloc(sizeof(struct sockaddr_in));
    bzero(myServer, sizeof(*myServer));

    // assign IP, PORT
    (*myServer).sin_family = domain;
    (*myServer).sin_addr.s_addr = address;
    (*myServer).sin_port = htons(port);

    return myServer; // resulting server
}

/// @brief prepare the socket to listen to a max number of connections
void server_listen(int mySocket)
{
    int responseStatus = listen(mySocket, MAX_CONNECTIONS);

    if (responseStatus != 0)
    {
        print_error("Listen failed...");
        exit(EXIT_FAILURE);
    }
    else
        print_success("Server listening..");
}

int main(int argc, char *argv[])
{
    int attempts = 6;

    // check arguments
    if (argc < 3)
    {
        if (argc == 2)
        {
            fprintf(stderr, "Attempts number not specified, set to 6.");
            fflush(stderr);
        }
        else
        {
            fprintf(stderr, "Incorrect arguments. Usage: %s <port> [<max-attempts>]\n", argv[0]);
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
    }
    else if (atoi(argv[2]) >= 6 && atoi(argv[2]) <= 10) // se i tentativi massimi sono compresi tra 6 e 10
        attempts = atoi(argv[2]);
    else
    {
        fprintf(stderr, "Attempts should be between 6 and 10. Usage: %s %s [<max-attempts>]\n", argv[0], argv[1]);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    // create and verify streaming socket
    int mySocket = create_socket();

    // server setup and assign
    sockaddr_t myServer = init_server(AF_INET, INADDR_ANY, atoi(argv[1]));

    // Binding socket to IP and verification
    bind_socket(mySocket, (struct sockaddr *)myServer);

    // Now server is ready to listen and verification
    server_listen(mySocket);

    // Function for chatting between client and server
    chat(attempts, mySocket);

    // Close the socket
    close(mySocket);
    free(myServer);
    return 0;
}