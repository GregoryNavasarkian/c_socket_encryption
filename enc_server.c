#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SIZE 150000

/*
 * Checks if the client is enc_client.
 * If not will reject connection and print error.
 */
void checkConnection(int clientFD) {
    char check[3];
    int charsRead;
    // receive verification string
    memset(check, '\0', sizeof(check));
    charsRead = recv(clientFD, check, 3, 0);
    if (charsRead < 0) {
        perror("ERROR reading from socket (check connection)");
        exit(0);
    }
    // if client sends back correct string
    if (strcmp(check, "ec") == 0) {
        char verified[2] = "y";
        charsRead = send(clientFD, verified, 1, 0);
        if (charsRead < 0) {
            perror("ERROR writing to socket (check connection)");
            exit(1);
        }
    }
    else {
        char notVerified[2] = "n";
        charsRead = send(clientFD, notVerified, 1, 0);
        if (charsRead < 0) {
            perror("ERROR writing to socket (check connection)");
            exit(1);
        }
    }
}

/*
 * Converts char value to int
 */
int charToInt(char c) {
    const char charLib[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    for (int i = 0; i < strlen(charLib); i++)
        if (charLib[i] == c) return i;
    return -1;
}

/*
 * Converts int value to char
 */
char intToChar(int i) {
    if (i == 26) return ' ';
    else return (i + 65);
}

/*
 * Encrypts text with key.
 * Saves encrypted to var.
 */
void encrypt(char text[], char key[], char encrypted[]) {
    for (int i = 0; i < strlen(text) - 1; i++) {
        int encryptNum = (charToInt(text[i]) + charToInt(key[i])) % 27;
        encrypted[i] = intToChar(encryptNum);
    }
}

int main(int argc, char *argv[]) {
    // initialize variables
    int sockFD, charsRead;
    int ret;
    struct sockaddr_in serverAddress, clientAddress;
    int clientSocket;
    socklen_t addr_size;
    pid_t pid;

    // initialize buffers for holding strings
    char textBuffer[SIZE];
    char buffer[SIZE];
    char keyBuffer[SIZE];
    char encrypted[SIZE];

    // check usage and args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(0);
    }

    // create socket
    sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        perror("SERVER: ERROR opening socket");
        exit(0);
    }
    // setsockopt for concurrency
    int optVal = 1;
    setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    // clear address structure
    memset(&serverAddress, '\0', sizeof(serverAddress));
    // setup server socket
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[1]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // bind to socket
    ret = bind(sockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (ret < 0) {
        perror("ERROR on binding");
        exit(0);
    }
    // listen for connection
    if (listen(sockFD, 5) != 0) {
        perror("ERROR on listen");
        exit(0);
    }

    while (1) {
        // accept incoming connections
        clientSocket = accept(sockFD, (struct sockaddr*)&clientAddress, &addr_size);
        if (clientSocket < 0) {
            exit(1);
        }
        // fork process
        pid = fork();
        if (pid == -1) {
            perror("fork() failed");
            exit(1);
        }
        else if (pid == 0) {
            close(sockFD);
            // check if connected to correct program
            checkConnection(clientSocket);
            // get text string from client
            memset(buffer, '\0', sizeof(buffer));
            charsRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (charsRead < 0) {
                perror("ERROR reading text from socket");
                exit(0);
            }

            // clear buffers
            memset(textBuffer, '\0', sizeof(textBuffer));
            memset(keyBuffer, '\0', sizeof(keyBuffer));

            // separate buffer into separate text and string parts
            int write = 1;
            for (int i = 0, j = 0; i < strlen(buffer); i++) {
                if (write == 1) {
                    textBuffer[i] = buffer[i];
                    if (buffer[i] == 10) {
                        write = 0;
                    }
                }
                if(write == 0) {
                    keyBuffer[j] = buffer[i];
                    j++;
                }
            }

            // encrypt string
            memset(encrypted, '\0', sizeof(encrypted));
            encrypt(textBuffer, keyBuffer, encrypted);

            // send encrypted to client
            charsRead = send(clientSocket, encrypted, sizeof(encrypted), 0);
            if (charsRead < 0) {
                perror("ERROR sending encrypted text to client");
                exit(0);
            }
        }
        close(clientSocket);
    }
}
