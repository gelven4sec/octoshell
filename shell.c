
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

char ligne[5000];       //array entry

//Prompt display
void aff_invite()
{
    printf("user@name > ");
    fflush(stdout);
}

// Saisie
void saisie()
{
    if (!fgets(ligne,sizeof(ligne)-1,stdin)) {
        //^D pour quitter (fin)
        printf("\n");
        exit(0);
    }
}


void execute()
{
    pid_t pid;

    //Removing line breaks
    if (strchr(ligne,'\n')) *strchr(ligne,'\n') = 0;

    //Skip empty lines
    if (!strcmp(ligne,"")) return;

    pid = fork();
    if (pid < 0) {
        printf("fork failed (%s)\n",strerror(errno));
        return;
    }

    if (pid==0) {

        execlp(ligne, //execution
               ligne,
               NULL
        );

        //Exec failed
        printf("processus failed \"%s\" (%s)\n",ligne,strerror(errno));
        exit(1);
    }
    else {
    }
}

int main()
{

    while (1) {
        aff_invite();
        saisie();
        execute();
    }
    return 0;
}

