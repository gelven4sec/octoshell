// https://brennan.io/2015/01/16/write-a-shell-in-c/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    FILE* fd;

    args[redirect] = NULL;

    if ((fd = fopen(args[redirect+1], "w")) == NULL){
        printf("\nFile not found !\n");
        return;
    }

    pid = fork();
    if (pid == 0) {
        close(1);
        dup2(fd->_fileno, 1);
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("\rOctoshell");
        }
        //exit(EXIT_FAILURE); // jsp pourquoi je l'ai mis je verrais après
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &state, 0);
        fclose(fd);
    } else {
        // Error forking
        perror("\rOctoshell");
    }

}

void execute_redirect2(char** args, int redirect){
    pid_t pid;
    int state = 0;
    FILE* fd;

    args[redirect] = NULL;

    if ((fd = fopen(args[redirect+1], "r")) == NULL){
        printf("\nFile not found !\n");
        return;
    }

    pid = fork();
    if (pid == 0) {
        //close(1);
        dup2(fd->_fileno, 0);
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("\rOctoshell");
        }
        //exit(EXIT_FAILURE); // jsp pourquoi je l'ai mis je verrais après
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &state, 0);
        fclose(fd);
    } else {
        // Error forking
        perror("\rOctoshell");
    }

}

int count_args(char** args, int pipe_index){
    int i;
    for (i = pipe_index+1; args[i] != NULL ; ++i);
    return i;
}

char** get_pipe_args(char** args, int pipe_index){
    int size = count_args(args, pipe_index);
    char** list = malloc(sizeof(char*) * size);
    for (int i = 0; i < size; ++i) {
        list[i] = args[pipe_index+1+i];
    }
    return list;
}

void execute_pipe(char** args, int pipe_index){
    pid_t pid1;
    pid_t pid2;
    int fd[2];
    char** args_pipe;

    // for the first command to be executed normally
    args[pipe_index] = NULL;

    if (args[pipe_index+1] == NULL){
        fprintf(stderr, "\rNo argument after pipe !");
        return;
    }

    args_pipe = get_pipe_args(args, pipe_index);

    if (pipe(fd) == -1){
        perror("\rOctoshell");
        return;
    }

    pid1 = fork();
    if (pid1 == 0){
        // child process 1
        close(fd[0]);
        dup2(fd[1], 1);
        execvp(args[0], args);
    } else {
        pid2 = fork();
        if (pid2 == 0){
            // child process 1
            close(fd[1]);
            dup2(fd[0], 0);
            execvp(args_pipe[0], args_pipe);
        } else {
            // parent, i guess
            waitpid(pid1, NULL, 0);
            close(fd[1]);
            waitpid(pid2, NULL, 0);
        }
    }

}

int check_char(char** args, char* sep){
    int bool = 0;
    for (int i = 0; args[i] != NULL; ++i) {
        if (strcmp(args[i], sep) == 0){
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

    } else if (strcmp(args[0], "help") == 0) {

        printf("\r#########################"
               "\nWelcome to the octoshell, made with love by some students."
               "\nHere are the commands available :\n"
               "\n\t- cd : Navigate through directories"
               "\n\t- exit: To leave the shell"
               "\n\t- | : to use pipe functionality"
               "\n\n#########################\n");

    } else if (check_char(args, "|") > 0) {

        execute_pipe(args, check_char(args, "|"));

    } else if (check_char(args, ">") > 0) {

        execute_redirect(args, check_char(args, ">"));

    } else if (check_char(args, "<") > 0) {

        execute_redirect2(args, check_char(args, "<"));

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
