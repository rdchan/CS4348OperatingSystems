#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

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

    if(fcntl(pipeatob[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("ERROR: Failed to set reading file descriptors to nonblocking");
        return -1;
    }

    if(fcntl(pipebtoa[0], F_SETFL, O_NONBLOCK) < 0) {
        printf("ERROR: Failed to set reading file descriptors to nonblocking");
        return -1;
    }

    //try to fork and create a section of code that only child a will run
    pida = fork();

    if(pida < 0) {
        printf("ERROR: fork one failed\n");
        return -1;
    } else if(pida == 0) {
        //child a exclusive
        
        //don't need to mess with child b sides of the pipes, so close them.
        close(pipeatob[0]); //child b read side
        close(pipebtoa[1]); //child b write side
        
        //now use dup2 to tie child a's typical stdout and stdin to the pipes
        dup2(pipebtoa[0], 0); //tie child a read side of the pipes to child a stdin
        dup2(pipeatob[1], 1); //tie child a write side of the pipes to child a stdout

        //close the file descriptors of the pipe as they have just been copied into child a's fd table
        close(pipebtoa[0]);
        close(pipeatob[1]);

        //now send child b the contents of this directory. This is done with the tail -vn +1 *.
            //for every file in the directory, it runs tail with -n +1. this flag says to print the file starting from line 1. the -v flag forces it to print the file name as a header before the contents of the file
            //this means that if a file contains ==> as a line, then it'll create a new file. helps for testing.

        chdir(directory_one); 
        system("tail -vn +1 *");
        system("echo ENDOFFILES");

        close(1); //nothing left to write, let child b know we are done

        //now we've done our work of sending the information. time to read from child b and make its files.
        char *line = NULL;
        size_t size;
        const char header[5] = "==> ";
        const char exitstring[20] = "ENDOFFILES";
        FILE *fp = NULL;

        bool check_false_empty_line = false;
        //bool got_first_message = false;
        while(getline(&line, &size, stdin) >= 0) {
        /*while(1) {

            int abc = getline(&line, &size, stdin);
            fprintf(stderr, "   [abc] = %d\n", abc);
            //if no message yet, spinlock until first message
            //if we have gotten a first message and there's none left, exit
            if(abc < 0) {

            got_first_message = true;
        */
            char *isfile = strstr(line, header);
            char *isdone = strstr(line, exitstring);
            if(isdone != NULL) {
                break;
            }
            //fprintf(stderr, "from b >> %s", line);
            if(isfile != NULL) { 
                check_false_empty_line = false;
                char filename[BUFSIZ];
                strncpy(filename, isfile+4, (strrchr(isfile, '<')-isfile-5));
                //close any existing file before trying to open a new one
                if(fp != NULL) {
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
                        check_false_empty_line = true;
                    } else {
                        fputs(line, fp);
                    }
                } else {
                    fprintf(stderr, "file does not exist\n]");
                }
            }
        }
       
        //when there's nothing left to read from the pipe, try to close any open file
        if(fp != NULL) {
            fclose(fp);
        }       

        exit(0);
    } else {
        //parent process. make child b.
        pidb = fork();

        if(pidb < 0) {
            printf("ERROR: fork two failed\n");
            return -1;
        } else if(pidb == 0) {
            //child b exclusive

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
            system("echo ENDOFFILES");
            
            close(1); //nothing left to write, let child a know that we are done
    
            //now we've done our work of sending the information. time to read from child b and make its files.
            char *line = NULL;
            size_t size;
            const char header[5] = "==> ";
            const char exitstring[20] = "ENDOFFILES";
            FILE *fp = NULL;

            bool check_false_empty_line = false;
            while(getline(&line, &size, stdin) >= 0) {
                char *isdone = strstr(line, exitstring);
                if(isdone != NULL) {
                    break;
                }

                char *isfile = strstr(line, header);
                if(isfile != NULL) { 
                    check_false_empty_line =  false;
                    char filename[BUFSIZ];
                    strncpy(filename, isfile+4, (strrchr(isfile, '<')-isfile-5));
                    //close any existing file before trying to open a new one
                    if(fp != NULL) {
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
                            check_false_empty_line = true;
                        } else {
                            fputs(line, fp);
                        }
                    } else {
                        fprintf(stderr, "file does not exist\n");
                    }
                }
            }

            //when there's nothing left to read from the pipe, try to close any open file
            if(fp != NULL) {
                fclose(fp);
            }       

            exit(0);
        } else {
            //finally just the parent.
            int returnStatus;

            waitpid(pida, &returnStatus, 0);
            if(returnStatus == 0) { 
                printf("child a terminated as expected\n"); 
            } else {
                printf("child a terminated with an error\n");
            }

            waitpid(pidb, &returnStatus, 0);
            if(returnStatus == 0) { 
                printf("child b terminated as expected\n"); 
            } else {
                printf("child b terminated with an error\n");
            }
        }
    }
    return 0;
}
