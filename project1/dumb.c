#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    pid_t pida;
    pid_t pidb;

    int pipeatob[2];
    int pipebtoa[2];

    char directory_one[100] = "dir1"; 
    char directory_two[100] = "dir2"; 
    //attempt to create the two pipes
    if(pipe(pipeatob) < 0 || pipe(pipebtoa) < 0) {
        //couldn't make pipes.
        printf("ERROR: Failed to create an essential pipe");
        return -1;
    }


    pida = fork();
    if(pida < 0) {
        printf("something went wrong\n");
    } else if(pida == 0) {
        fprintf(stderr, "child a sleeping..\n");
        sleep(3);
        fprintf(stderr, "child a done..\n");
    } else {
        pidb = fork();
        if(pidb < 0) {
            fprintf(stderr, "did bad");
        } else if(pidb == 0) {
            fprintf(stderr, "child b sleeping for longer..\n");
            sleep(5);
            fprintf(stderr, "child b done..\n");
        } else {
            fprintf(stderr,"parent waiting for a child\n");
            wait(NULL);
            fprintf(stderr, "parent done waiting\n");
            fprintf(stderr, "parent waiting again");
            wait(NULL);
            fprintf(stderr, "parent gong home");
        }
    }
    return 0;
}
