#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    //initialize pipe file descriptors. one to for child a to write to child b, and one for opposite
    int pipeatob[2];
    int pipebtoa[2];
    /*
     * child a writes to child b using pipeatob. CHILDA_WRITE is pipeatob[1]
     *                                           CHILDB_READ is pipeatob[0]
     *
     * child b writes to child a using pipebtoa. CHILDB_WRITE is pipebtoa[1]
     *                                           CHILDA_READ is pipebtoa[0]
     *
     */
    //pid objects to check return value from fork calls
    pid_t pida;
    pid_t pidb;


    char directory_one[100] = "dir1"; 
    char directory_two[100] = "dir2"; 
    //attempt to create the two pipes
    if(pipe(pipeatob) < 0 || pipe(pipebtoa) < 0) {
        //couldn't make pipes.
        printf("ERROR: Failed to create an essential pipe");
        return -1;
    }

    //try to fork and create a section of code that only child a will run
    pida = fork();

    if(pida < 0) {
        printf("ERROR: fork one failed\n");
        return -1;
    } else if(pida == 0) {
        //child a exclusive
        printf("hello, I am child a, pid is %d\n", getpid());
        
        //don't need to mess with child b sides of the pipes, so close them.
        close(pipeatob[0]); //child b read side
        close(pipebtoa[1]); //child b write side
        
        //now use dup2 to tie child a's typical stdout and stdin to the pipes
        dup2(pipebtoa[0], 0); //tie child a read side of the pipes to child a stdin
        dup2(pipeatob[1], 1); //tie child a write side of the pipes to child a stdout

        //close the file descriptors of the pipe as they have just been copied into child a's fd table
        close(pipebtoa[0]);
        close(pipeatob[1]);

        //now send child b the contents of this directory. The real magic is in the clever tail -vn +1 *.
            //for every file in the directory, it runs tail with -n +1. this flag says to print the file starting from line 1. the -v flag forces it to print the file name as a header before the contents of the file
            //this means that if a file contains ==> as a line, then it'll create a new file. helps for testing.

        chdir(directory_one);
        system("tail -vn +1 *");

        fprintf(stderr, "child a closing the pipe");
        close(1); //nothing left to write, let child b know we are done
        fprintf(stderr, "child a pipe closed");
        //now we've done our work of sending the information. time to read from child b and make its files.
        
        char *line = NULL;
        size_t size;
        const char header[5] = "==> ";
        FILE *fp = NULL;

        bool check_false_empty_line = false;
        while(getline(&line, &size, stdin) != -1) {
            fprintf(stderr, "from child b >> :%s:", line);

            char *isfile = strstr(line, header);
            if(isfile != NULL) { 
                check_false_empty_line = false;
                char filename[BUFSIZ];
                strncpy(filename, isfile+4, (strrchr(isfile, '<')-isfile-5));
                //close any existing file before trying to open a new one
                if(fp != NULL) {
                    //fprintf(stderr, "<<child a is closing a file\n");
                    fclose(fp);
                }
                //open a file for writing with the filename we parsed from child b
                fp = fopen(filename, "w");
            } else {
                if(check_false_empty_line) {
                    fputs("\n", fp);
                    check_false_empty_line = false;
                }
                if(fp != NULL) {
                    if((int)*line == 10) {
                        //fprintf(stderr, "empty line trigger");
                        check_false_empty_line = true;
                    } else {
                        fputs(line, fp);
                    }
                    //fprintf(stderr, "<<child a wrote :%s:", line);
                } else {
                    fprintf(stderr, "file does not exist");
                }
            }
        }
       
        //when there's nothing left to read from the pipe, try to close any open file
        if(fp != NULL) {
            fclose(fp);
            //fprintf(stderr, "<< nothing left in pipe, child a is closing file\n");
        }       

        fprintf(stderr, "child a exiting");
        exit(0);

    } else {
        //parent process. make child b.
        pidb = fork();

        if(pidb < 0) {
            printf("ERROR: fork two failed\n");
            return -1;
        } else if(pidb == 0) {
            //child b exclusive
            printf("hello, I am child b, pid is %d\n", getpid());

            //don't need to mess with child a sides of the pipes, so close them.
            close(pipebtoa[0]); //child a read side
            close(pipeatob[1]); //child a write side
            
            //now use dup2 to tie child b's typical stdout and stdin to the pipes
            dup2(pipeatob[0], 0); //tie child b read side of the pipes to child b stdin
            dup2(pipebtoa[1], 1); //tie child b write side of the pipes to child b stdout

            //close the file descriptors of the pipe as they have just been copied into child b's fd table
            close(pipeatob[0]);
            close(pipebtoa[1]);

            chdir(directory_two);
            system("tail -vn +1 *");
            
            fprintf(stderr, "child b closing the pipe");
            close(1); //nothing left to write, let child a know that we are done
            fprintf(stderr, "child b pipe closed");

            //now we've done our work of sending the information. time to read from child b and make its files.
            char *line = NULL;
            size_t size;
            const char header[5] = "==> ";
            FILE *fp = NULL;

            bool check_false_empty_line = false;
            while(getline(&line, &size, stdin) != -1) {
                fprintf(stderr, "from child a >> %s", line);
                
                char *isfile = strstr(line, header);
                if(isfile != NULL) { 
                    check_false_empty_line =  false;
                    char filename[BUFSIZ];
                    strncpy(filename, isfile+4, (strrchr(isfile, '<')-isfile-5));
                    //close any existing file before trying to open a new one
                    if(fp != NULL) {
                        //fprintf(stderr, ">>child b is closing a file");
                        fclose(fp);
                    }
                    //open a file for writing with the filename we parsed from child b
                    fp = fopen(filename, "w");
                } else {
                    if(check_false_empty_line) {
                        fputs("\n", fp);
                        check_false_empty_line = true;
                    }
                    if(fp != NULL) {
                        if((int)*line == 10) {
                            //fprintf(stderr, "empty line trigger");
                            check_false_empty_line = true;
                        } else {
                            fputs(line, fp);
                        }
                        //fprintf(stderr, ">>child b wrote :%s:", line);
                    } else {
                        fprintf(stderr, "file does not exist");
                    }
                }
            }

            //when there's nothing left to read from the pipe, try to close any open file
            if(fp != NULL) {
                fclose(fp);
            }       

            fprintf(stderr, "child b exiting");
            exit(0);
        } else {
            //finally just the parent.
            int returnStatus;
            printf("hello, I am parent!, pid is %d\n", getpid());
            waitpid(pida, &returnStatus, 0);
            if(returnStatus == 0) { printf("child a terminated as expected\n"); } else {
                printf("child a terminated with an error\n"); }
            waitpid(pidb, &returnStatus, 0);
            if(returnStatus == 0) { printf("child b terminated as expected\n"); } else {
                printf("child b terminated with an error\n"); }
            
        }
    }

    return 0;
}
