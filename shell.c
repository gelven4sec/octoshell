#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/stat.h>

char   entry_line[5000];    //standard entry line stdin

//cut out the words of entry_line
#define word 32
char* tab[word];

//Prompt display
void prompt()
{
    printf("lea@lrze> ");
    fflush(stdout);
}

//Saisie
void saisie()
{
    if (!fgets(entry_line,sizeof(entry_line)-1,stdin)) {
        printf("\n");
        exit(0);
    }
}


//waitpid = wait for the child to change status
void attent(pid_t pid)
{
    while (1) {
        int status;
        int process_wait = waitpid(pid,&status,0); //wait child processus
        if (process_wait<0) {
            if (errno==EINTR) continue;
            printf("error of waitpid (%s)\n",strerror(errno));
            break;
        }
        if (WIFEXITED(status)) //Return true if status exited via the exit
            printf("normal termination, status %i\n",WEXITSTATUS(status));//Return exit code of child
        if (WIFSIGNALED(status)) //Return true if the child finished because of signal
            printf("termination by signal %i\n",WTERMSIG(status));//Return signal number
        break;
    }
}

void execute()
{
    pid_t pid;
    struct sigaction signal; //Change the action taken by a process of a specific signal

    if (!tab[0]) return;

    //deactivate interruption by CTRL^C
    signal.sa_flags = 0; //set atributes that modify the behavior of the signal
    signal.sa_handler = SIG_IGN; //ignore signal
    sigaction(SIGINT, &signal, NULL);

    pid = fork();
    if (pid < 0) {
        printf("fork is failed (%s)\n",strerror(errno));
        return;
    }

    if (pid==0) {
        //child
        int i;

        //activate interruption by CTRL^D
        signal.sa_handler = SIG_DFL;
        sigaction(SIGINT, &signal, NULL);


        //Finding the last argument
        for (i=0;tab[i+1];i++);

        if (tab[i][0]=='>') {
            //Open the destination file
            int file = open(&tab[i][1], O_WRONLY | O_CREAT | O_TRUNC);//Read, write, create
            if (file==-1) {
                printf("can't create file \"%s\" (%s)\n",
                       &tab[i][1],strerror(errno));
                exit(1);
            }

            //Redirection stdout
            close(1);
            dup2(file,1);

            //Delete last argument
            tab[i] = NULL;
        }

        execvp(tab[0], //execute programme
               tab     //execute argv programme
        );
        printf("execution not possible \"%s\" (%s)\n",tab[0],strerror(errno));
        exit(1);
    }
    else {
        //parent
        attent(pid);

        //active interruption by CTRL^C
        signal.sa_handler = SIG_DFL;
        sigaction(SIGINT, &signal, NULL);
    }
}

//To have several argument
void cutout()
{
    char* line_start = entry_line;
    int i;
    for (i=0; i<word-1; i++) {

        //skip the spaces
        while (*line_start && isspace(*line_start)) line_start++; //testing space characters
        if (!*line_start) break;

        //start of word
        tab[i] = line_start;

        //finding the end of the word and skip the word
        while (*line_start && !isspace(*line_start)) line_start++;

        // finish the word and pass to the next
        if (*line_start) { *line_start = 0; line_start++; }
    }
    //finish by pointer NULL
    tab[i] = NULL;
}

int main()
{
    while (1){
        prompt();
        saisie();
        cutout();
        execute();

    }
    return 0;
}