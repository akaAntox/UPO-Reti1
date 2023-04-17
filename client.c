#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // isspace()
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()

#define MAX 256
#define TOLLERANCE 6

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

#define PLAY_GAME 1
#define QUIT_GAME 2

#define CLOSE_EXECUTION 0
#define CONTINUE_EXECUTION 1

typedef enum enum_commands
{
    OK,      // received message
    QUIT,    // leave execution
    PERFECT, // guessed word
    END,     // no more attempts
    ERR      // error
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

/// @brief print menu
void print_menu(int attempts, int max_attempts)
{
    fprintf(stderr, "Choose an option:\n");
    fprintf(stderr, "1. Try to guess the word. %d attempts remaining.\n", max_attempts - attempts);
    fprintf(stderr, "2. Leave execution.\n");
    fprintf(stderr, "Choice > ");
    fflush(stderr);
}

/// @brief create server and assign domain, address and port
/// @return resulting server
sockaddr_t init_server(int domain, in_addr_t address, int port)
{
    sockaddr_t myServer = malloc(sizeof(struct sockaddr_in));
    bzero(myServer, sizeof(*myServer));

    (*myServer).sin_family = domain;       // domain assignment
    (*myServer).sin_addr.s_addr = address; // ip_address assignment
    (*myServer).sin_port = port;           // port assignment

    return myServer; // resulting server
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

/// @brief connect client socket to server socket
void connect_socket_to_server(int clientSocket, struct sockaddr *serverSocket)
{
    int responseStatus = connect(clientSocket, serverSocket, sizeof(*serverSocket));

    if (responseStatus < 0)
    {
        print_error("Connection with the server failed...");
        close(clientSocket);
        free(serverSocket);
        exit(EXIT_FAILURE);
    }
    else
        print_success("Connected to the server..");
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

/// @brief checks given string for errors and retrieves message
/// @return 1 if no errors, 0 otherwise
int get_message(const char *string, char **msg)
{
    int space_pos = search_space_pos(string);
    int needle = space_pos;
    int err = 1;

    if (isspace(string[space_pos]))
        needle++;
    else
        err &= 0;

    // get message
    *msg = malloc(strlen(string + needle + 1) + 1);
    strcpy(*msg, string + needle);
    if (string[strlen(string) - 1] != '\n')
        err &= 0;

    if (!err) // if there's an error
        print_error("Malformed message...");

    return err;
}

/// @brief checks given string for errors and retrieves attempts and message
/// @return 1 if no errors, 0 otherwise
int get_ok_message(const char *string, int *attempts, char **msg)
{
    int space_pos = search_space_pos(string);
    int needle = space_pos;
    int err = 1;

    if (isspace(string[space_pos]))
        needle++;
    else
        err &= 0;
    if (isdigit(string[needle]))
        sscanf(string + needle, "%d", attempts);
    else
        err &= 0;
    if (*attempts < 9)
        needle++;
    return get_message(string + needle, msg);
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

/// @brief retrieves command, attempts and message from the given string
int retrieve_message(const char *string, commands *cmd, int *attempts, char **msg)
{
    if (cmdcmp(string, "OK", OK) == OK)
    {
        if (cmdcmp(string + 3, "PERFECT\n", PERFECT) == PERFECT)
        {
            *cmd = PERFECT;
            return 1;
        }
        else if (strcmp(string + (strlen(string) - 1), "\n") != 0)
        {
            print_error("Malformed string...\n");
            return 0;
        }

        *cmd = OK;
        get_ok_message(string, attempts, msg);
        return 1;
    }

    if (cmdcmp(string, "END", END) == END)
    {
        *cmd = END;
        get_ok_message(string, attempts, msg);
        return 1;
    }
    else if (cmdcmp(string, "QUIT", QUIT) == QUIT)
        *cmd = QUIT;
    else if (cmdcmp(string, "ERR", ERR) == ERR)
        *cmd = ERR;

    get_message(string, msg);
    return 1;
}

/// @brief checks if the word is admitted
/// @return 1 if the word is admitted, 0 otherwise
int check_word(char *word)
{
    if (strlen(word) == 0 || word == NULL) // no word
    {
        print_warning("No word inserted!");
        return 0;
    }

    for (size_t i = 0; i < strlen(word); i++) // no alphabetic char
        if (!isalpha(word[i]))
        {
            print_warning("The word should be composed of alphabetic characters!");
            return 0;
        }

    if (strlen(word) != 5) // word length wrong
    {
        print_warning("The word isn't 5 letters!");
        return 0;
    }

    return 1; // word correct format
}

/// @brief asks choice to user
/// @return 1 if no error, 0 if error
int ask_choice(int *ch, int attempts, int max_attempts)
{
    char choice[MAX] = "";
    print_menu(attempts, max_attempts);
    fscanf(stdin, " %s", choice); // get user choice
    int n = atoi(choice);         // convert choice to integer

    if (isdigit(choice[0]) && n > 0 && n < 3 && strlen(choice) == 1) // check if valid choice
    {
        *ch = n;
        return 1;
    }
    else
        return 0;
}

/// @brief exchange messages between client and server
/// @param mySocket given socket
void socket_chat(int mySocket)
{
    char buffer[MAX] = "";
    commands cmd;                          // command received
    int attempts = 0, max_attempts = 0;    // number of attempts
    char *message = NULL;                  // received message
    int actual_state = CONTINUE_EXECUTION; // execution state

    bzero(buffer, sizeof(buffer));                               // set buffer to 0
    int responseStatus = read(mySocket, buffer, sizeof(buffer)); // read socket

    if (responseStatus > 0 && retrieve_message(buffer, &cmd, &attempts, &message)) // if server responds
    {
        bzero(buffer, sizeof(buffer)); // set buffer to 0
        if (cmd == OK)
        {
            max_attempts = attempts; // set max attempts to server set attempts
            attempts = 0;
            print_warning(message); // print welcome message
        }
        else
            print_error(message); // print error message
    }
    else
        return;

    while (actual_state)
    {
        int choice = 0;
        char clientString[MAX] = ""; // string to send to server
        bzero(clientString, MAX);

        while (!ask_choice(&choice, attempts, max_attempts))
            ;

        if (choice == PLAY_GAME) // if user wants to guess the word
        {
            char parola[MAX - TOLLERANCE];
            bzero(parola, sizeof(parola));

            do // ask word to user until correctly formatted
            {
                fprintf(stderr, "\nGuess the 5 letter word: ");
                fflush(stderr);
                fscanf(stdin, " %s", parola); // get word from user
            } while ((!check_word(parola) && strlen(parola) == 5) || strlen(parola) != 5);

            sprintf(clientString, "WORD %s\n", parola);              // prepare command to send to server
            write(mySocket, clientString, strlen(clientString));     // send command to server
            bzero(buffer, sizeof(buffer));                           // set buffer to 0 (already got its values)
            responseStatus = read(mySocket, buffer, sizeof(buffer)); // wait for server response

            if (responseStatus > 0 && retrieve_message(buffer, &cmd, &attempts, &message))
            {
                bzero(buffer, sizeof(buffer)); // set buffer to 0 (already got its values)
                switch (cmd)
                {
                case PERFECT:
                    actual_state = CLOSE_EXECUTION;
                    print_success("Good job, you guessed the word! Thanks for playing!\n");
                    fflush(stderr);
                    break;
                case OK:
                    actual_state = CONTINUE_EXECUTION;
                    fprintf(stderr, "Wrong word, try again! %s\n", message);
                    fflush(stderr);
                    break;
                case END:
                    actual_state = CLOSE_EXECUTION;
                    fprintf(stderr, "I'm sorry! No more attempts left. Thanks for playing! The word was: \"%s\"", message);
                    fprintf(stderr, "\n");
                    fflush(stderr);
                    break;
                case ERR:
                    actual_state = CLOSE_EXECUTION;
                    print_error(message);
                    break;

                default:
                    actual_state = CLOSE_EXECUTION;
                    print_error("Malformed message...\n");
                    break;
                }
            }
            else
                return;
        }
        else if (choice == QUIT_GAME) // if user wants to quit game
        {
            sprintf(clientString, "QUIT\n");                         // prepare command to send to server
            write(mySocket, clientString, strlen(clientString));     // send command to server
            responseStatus = read(mySocket, buffer, sizeof(buffer)); // wait for server response

            if (responseStatus > 0 && retrieve_message(buffer, &cmd, &attempts, &message))
            {
                bzero(buffer, sizeof(buffer)); // set buffer to 0 (already got its values)
                actual_state = CLOSE_EXECUTION;
                switch (cmd)
                {
                case QUIT:
                    print_success(message);
                    break;
                case ERR:
                    print_error(message);
                    break;

                default:
                    break;
                }
            }
        }
    };
}

int main(int argc, char *argv[])
{
    // check arguments
    if (argc < 3)
    {
        fprintf(stderr, COLOR_RED "Incorrect arguments. Usage: %s <server> <port>\n" COLOR_RESET, argv[0]);
        exit(EXIT_FAILURE);
    }

    // verify if server address is valid
    if (inet_addr(argv[1]) == INADDR_NONE)
    {
        print_error("Server address is invalid!\n");
        exit(EXIT_FAILURE);
    }

    // create and verify streaming socket
    int mySocket = create_socket();

    // server setup and assign
    sockaddr_t myServer = init_server(AF_INET, inet_addr(argv[1]), htons(atoi(argv[2])));

    // connect the client socket to server socket
    connect_socket_to_server(mySocket, (struct sockaddr *)myServer);

    // function for chat
    socket_chat(mySocket);

    // close the socket
    close(mySocket);
    free(myServer);
    return 0;
}