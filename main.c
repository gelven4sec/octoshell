#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

// from Stephen Brennan (temp in the meantime to do better)
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

/*void saisie(char** input){
    if (!fgets(*input,sizeof(*input)-1,stdin)) {
        //^D pour quitter (fin) askip
        printf("\n");
        exit(0);
    }

    //Removing line breaks
    if (strchr(*input,'\n')) *strchr(*input,'\n') = 0;
}*/

// return the nb of arguments
int nb_words(const char* line){
    int count = 0;

    for (int i = 0; line[i] != '\0'; ++i) {
        if (' ' == line[i])
            ++count;
    }
    return count+1;
}

// return list of arguments
char** parse_input(char* line, int* counter){
    char* tok;
    int words = nb_words(line);
    char** list = malloc(sizeof(char *) * words);
    *counter = 0;

    for (tok = strtok(line, " "); tok && *tok; tok = strtok(NULL, " \n")){
        char* token = strdup(tok);
        list[*counter] = token;
        (*counter)++;
        if (*counter >= words){
            break;
        }
    }

    list[*counter] = NULL;

    return list;
}
void free_args(char** args, int nb){
    for (int i = 0; i < nb; ++i) {
        free(args[i]);
    }
}

void execute(char** args, int argc_c){
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("octoshell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("octoshell");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

}

void process_input(char** args, int argc_c){
    if (strcmp(args[0], "exit") == 0){
        printf("\nExiting Octoshell...");
        exit(EXIT_SUCCESS);
    } else {
        execute(args, argc_c);
    }

}

void shell_loop(){
    char *username;
    int state = 1;
    char* input; // raw prompt input
    char** args; // list of arguments
    int args_c; // number of arguments

    username = getlogin();
    if (username == NULL) {
        printf("\nFailed to get username");
        exit(EXIT_FAILURE);
    }

    do {
        printf("OSH:%s/~ > ", username);

        input = read_input();
        if (strcmp(input, "") == 0) continue;

        args = parse_input(input, &args_c);

        process_input(args, args_c);

        free_args(args, args_c);
        free(args);
        free(input);
    } while (state);

}

int main() {
    shell_loop();
    return 0;
}
