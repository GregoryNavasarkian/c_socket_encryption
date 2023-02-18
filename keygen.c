#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <err.h>
#include <errno.h>

int main(int argc, char *argv[]) {

    srand(time(NULL));
    int random;
    if (argc != 2) {
        err(errno, "invalid argument");
    }
    for (int i = 0; i < atoi(argv[1]); i++) {
        random = rand() % 27 + 65;
        if (random == 91) {
            random = 32;
        }
        printf("%c", random);
    }
    printf("\n");
    return 0;
}