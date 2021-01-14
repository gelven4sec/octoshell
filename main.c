#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// code de Stephen Brennan (temp in the meantime to do better)
// https://brennan.io/2015/01/16/write-a-shell-in-c/
char* read_input(){
    int bufsize = 1024;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "octoshell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += 1024;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "octoshell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void saisie(char** input){
    if (!fgets(*input,sizeof(*input)-1,stdin)) {
        //^D pour quitter (fin) askip
        printf("\n");
        exit(0);
    }

    //Removing line breaks
    if (strchr(*input,'\n')) *strchr(*input,'\n') = 0;
}

void shell_loop(){
    char *username;
    char* input = malloc(sizeof(char) * 1024);
    int state = 1;

    username = getlogin();
    if (username == NULL) {
        printf("Failed to get username");
        exit(EXIT_FAILURE);
    }

    do {
        printf("%s/~ > ", username);
        saisie(&input);
        printf("%s\n", input);

        if (strcmp(input, "quit") == 0) state = 0;

        free(input);
    } while (state);

}

int main() {
    shell_loop();
    return 0;
}
