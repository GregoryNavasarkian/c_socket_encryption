#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>

#define SIZE 150000

/*
 * Sends all data in buffer to server.
 */
int sendall(int s, char *buf, int *len) {
    int total = 0;
    int bytesLeft = *len;
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesLeft, 0);
        if (n == -1 ) break;
        total += n;
        bytesLeft -= n;
    }

    *len = total;
    return n == -1 ? -1 : 0;
}

/*
 * Check if file contains valid characters.
 */
void checkFile(const char* name) {
    FILE* file = fopen(name, "r");
    while (1) {
        int c = fgetc(file);
        if (c == EOF || c == '\n') break;
        if (!isupper(c) && c != ' ') {
            fprintf(stderr, "CLIENT: ERROR file contains bad characters\n");
            exit(0);
        }
    }
    fclose(file);
}

int main(int argc, char *argv[])
{
    // initialize variables
    int sockFD, ret, charsRead, charsSent;
    struct sockaddr_in serverAddress;
    FILE* fd;

    // initialize buffers
    char buffer[SIZE];
    char key[SIZE];

    // check usage and args
    if (argc != 4) {
        fprintf(stderr, "USAGE: %s filename key port\n", argv[0]);
        exit(0);
    }

    // create socket
    sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        perror("CLIENT: ERROR connecting");
        exit(0);
    }

    // setsockopt for concurrency
    int optVal = 1;
    setsockopt(sockFD, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

    // initialize server connection
    memset(&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[3]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // connect to server
    ret = connect(sockFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (ret < 0) {
        perror("CLIENT: ERROR connecting");
    }

    // check if file has incorrect characters
    checkFile(argv[1]);

    // check if connection is valid
    char verify[3] = "ec";
    charsSent = send(sockFD, verify, sizeof(verify), 0);
    if (charsSent < 0) {
        perror("Failed to send");
        exit(0);
    }
    memset(verify, '\0', sizeof(verify));
    charsRead = recv(sockFD, verify, sizeof(verify), 0);
    if (charsRead < 0) {
        perror("Failed to receive");
        exit(0);
    }

    // check if connected to enc server
    if (strcmp(verify, "y") != 0) {
        perror("ERROR: enc_client cannot user dec_server");
        exit(2);
    }

    // open text file and save to buffer
    fd = fopen(argv[1], "r");
    memset(buffer, '\0', sizeof(buffer));
    // save data
    fread(buffer, sizeof(char), SIZE, fd);
    fclose(fd);


    // open key and save to buffer
    fd = fopen(argv[2], "r");
    memset(key, '\0', sizeof(key));
    // save data
    fread(key, sizeof(char), SIZE, fd);
    fclose(fd);

    // check if key is long enough
    if (strlen(buffer) > strlen(key)) {
        fprintf(stderr, "ERROR: key '%s' is too short\n", argv[2]);
        exit(0);
    }

    // combine buffer and key strings
    strcat(buffer, key);

    // send buffer to server to be encrypted
    int len = sizeof(buffer);
    sendall(sockFD, buffer, &len);

    // get encrypted data from server
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(sockFD, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0) {
        perror("CLIENT: ERROR receive from server");
        exit(0);
    }

    for(int i = 0; i < sizeof(buffer); i++){
        if(buffer[i] == '\0' ) {
            if (buffer[i + 1] != '\0'){
                buffer[i] = ' ';
            }
        }
    }

    // print encrypted output
    fprintf(stdout, "%s\n", buffer);

    close(sockFD);
    return EXIT_SUCCESS;
}