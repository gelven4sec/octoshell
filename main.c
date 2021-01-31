// https://brennan.io/2015/01/16/write-a-shell-in-c/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

char* read_input(){
    int counter = 0;
    char *input = malloc(sizeof(char) * 1024);
    int c;

    if (!input) {
        fprintf(stderr, "\nOctoshell: could not allocate more memory");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == '\n') {
            input[counter] = '\0';
            return input;
        } else {
            input[counter] = c;
        }
        counter++;

        if (counter >= 1024) {
            fprintf(stderr, "\nOctoshell: Exceed maximum input capacity");
            return "";
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

char** parse_input(char* input){
    int counter = 0;
    char **tokens = malloc(sizeof(char*) * 64);
    char *token;

    if (!tokens) {
        fprintf(stderr, "\nOctoshell: could not allocate more memory");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, " \n");
    while (token != NULL) {
        tokens[counter] = token;
        counter++;

        token = strtok(NULL, " \n");
    }

    tokens[counter] = NULL;
    return tokens;
}

void execute(char** args){
    pid_t pid;
    int state = 0;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("\rOctoshell");
        }
        //exit(EXIT_FAILURE); // jsp pourquoi je l'ai mis je verrais après
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &state, 0);
    } else {
        // Error forking
        perror("\rOctoshell");
    }

}

void execute_redirect(char** args, int redirect){
    pid_t pid;
    int state = 0;
    int fd;

    args[redirect] = NULL;

    if((fd = open(args[redirect+1], O_RDWR | O_CREAT))==-1){ /*open the file */
        perror("open");
        return;
    }

    //close(STDOUT_FILENO);
    dup2(fd, STDOUT_FILENO);
    close(fd);

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("\rOctoshell");
        }
        //exit(EXIT_FAILURE); // jsp pourquoi je l'ai mis je verrais après
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &state, 0);
    } else {
        // Error forking
        perror("\rOctoshell");
    }

}

void execute_pipe(char** args, int pipe_index){
    pid_t pid;
    pid_t pid2;
    int state = 0;
    int fd[2];
    char buffer[255];

    // for the first command to be executed normally
    args[pipe_index] = NULL;

    if (args[pipe_index+1] == NULL){
        fprintf(stderr, "\rNo argument after pipe !");
        return;
    }

    if (pipe(fd) == -1){
        perror("\rOctoshell");
        return;
    }

    // second process
    pid = fork();
    if (pid == 0) {
        // Child process
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        if (execvp(args[pipe_index+1], (char*[]){args[pipe_index+1], NULL}) == -1) {
            perror("\rOctoshell");
        }
        exit(EXIT_FAILURE);

    } else if (pid > 0) {
        // Parent process
        // if we reach here, we are in parent process
        close(fd[0]);                 // file descriptor unused in parent
        dup2(fd[1], 1);

        // first process
        pid2 = fork();
        if (pid2 == 0) {
            // Child process
            if (execvp(args[0], args) == -1) {
                perror("\rOctoshell");
            }
            exit(EXIT_FAILURE);
        } else if (pid2 > 0) {
            // Parent process
            wait(&state);
        } else {
            // Error forking
            perror("\rOctoshell");
        }

        // send EOF so child can continue (child blocks until all input has been processed):
        close(fd[1]);
        close(fd[0]);
        close(1); // looks like a bad idea but it's the only way to get the sort result
        wait(&state);

    } else {
        // Error forking
        perror("\rOctoshell");
    }

}

int check_pipe(char** args){
    int bool = 0;
    for (int i = 0; args[i] != NULL; ++i) {
        if (strcmp(args[i], "|") == 0){
            return i;
        }
    }
    return bool;
}

int check_redirect(char** args){
    int bool = 0;
    for (int i = 0; args[i] != NULL; ++i) {
        if (strcmp(args[i], ">") == 0){
            return i;
        }
    }
    return bool;
}

int process_input(char** args){

    if (strcmp(args[0], "exit") == 0){

        printf("\nExiting Octoshell...");
        free(args);
        return 0;

    } else if (strcmp(args[0], "cd") == 0){

        if (args[1] == NULL) {
            fprintf(stderr, "\rOctoshell: expected argument to \"cd\"\r");
        } else {
            if (chdir(args[1]) != 0) {
                perror("\rOctoshell");
            }
        }

    } else if (check_pipe(args) > 0) {

        execute_pipe(args, check_pipe(args));

    } else if (check_redirect(args) > 0) {

        execute_redirect(args, check_redirect(args));

    } else {
        execute(args);
    }


    free(args);
    return 1;
}

void shell_loop(){
    char* username;
    char dir_output[1024];
    int state = 1;
    char* input; // raw prompt input
    char** args; // list of arguments

    username = getlogin();
    if (username == NULL) {
        fprintf(stderr, "\nOctoshell: Failed to get username");
        exit(EXIT_FAILURE);
    }

    do {
        getcwd(dir_output, 1024);
        sleep(1);
        printf("\r[OSH] %s:%s > ", username, dir_output);

        input = read_input();
        if (strcmp(input, "") == 0) continue;

        args = parse_input(input); // free input here

        state = process_input(args); // free args here

    } while (state);

}

int main() {
    shell_loop();
    return 0;
}
