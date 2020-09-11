#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int main() {
    const char header[5] = "==> ";
    const char exitstring[20] = "ENDOFFILES";
    //initialize pipe file descriptors. one to for child a to write to child b, and one for opposite
    int pipeatob[2];
    int pipebtoa[2];
    /*
     * child a writes to child b using pipeatob. CHILDA_WRITE is pipeatob[1]
     *                                           CHILDB_READ is pipeatob[0]
     *
     * child b writes to child a using pipebtoa. CHILDB_WRITE is pipebtoa[1]
     *                                           CHILDA_READ is pipebtoa[0]
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
    
        int status;
        status = chdir(directory_one); //returns 0 on success, -1 on fail. errno is set 
        if(status == -1) { 
            fprintf(stderr, "[FATAL]: child a cannot change directory to %s", directory_one); 
            exit(-1);
        }

        status = system("tail -vn +1 *");
        if(status == -1) {
            fprintf(stderr, "[FATAL]: child a system call tail failed");
            exit(-1);
        }

        //send clear message that there is nothing left in the pipe. otherwise getline may hang
        //an alternative solution is to make the pipes nonblocking and wait for a timeout
        //this runs more reliably on all machines though
        status = system("echo ENDOFFILES");
        if(status == -1) {
            fprintf(stderr, "[FATAL]: child a system call echo failed");
            exit(-1);
        }

        //nothing left to write, let child b know we are done
        status = close(1); 
        if(status == -1) {
            fprintf(stderr, "[FATAL]: child a close file descriptor 1 (stdout) failed");
            exit(-1);
        } 

        //now we've done our work of sending the information. 
        //time to read from child b and make its files.
        char *line = NULL; //next line from the pipe
        size_t size; //size of message from getline
        FILE *fp = NULL; //pointer to the open file. changes value upon getting new headers

        //used to avoid extra line feed on all files except the last one listed
        bool check_false_empty_line = false; 

        //check that the size of the message is nonnegative
        while(getline(&line, &size, stdin) >= 0) {
            
            //strstr looks for an occurrence of the substring in the string.
            //here we check if the line contains the header for a filename or the exitstring
            char *isfile = strstr(line, header);
            char *isdone = strstr(line, exitstring);

            //once we are done, we can break out of the while loop, close the last file, and terminate
            if(isdone != NULL) {
                break;
            }

            //if it is a filename, extract the filename, close the previous file, and open this new file
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
            } else { // otherwise, its either data or an extra linefeed.
                //if it's data and the previous line was a linefeed, append the linefeed
                if(check_false_empty_line) {
                    fputs("\n", fp);
                    check_false_empty_line = false;
                }
                //if a file is open, either set the linefeed flag or write the data
                if(fp != NULL) {
                    if((int)*line == 10) {
                        check_false_empty_line = true;
                    } else {
                        fputs(line, fp);
                    }
                } else {
                    fprintf(stderr, "file does not exist, ignoring contents\n]");
                }
            }
        }
       
        //when there's nothing left to read from the pipe, close the last file.
        //this should always run, but check anyways
        if(fp != NULL) {
            fclose(fp);
        }       
    
        //child a terminates
        exit(0);
    } else {
        //parent process of child a. make child b.
        pidb = fork();

        if(pidb < 0) {
            printf("ERROR: fork two failed\n");
            return -1;
        } else if(pidb == 0) {
            //child b exclusive. near duplicate of child a, but with different pipe and directory. Check above for more information

            //don't need to mess with child a sides of the pipes, so close them.
            close(pipebtoa[0]); //child a read side
            close(pipeatob[1]); //child a write side
            
            //now use dup2 to tie child b's typical stdout and stdin to the pipes
            dup2(pipeatob[0], 0); //tie child b read side of the pipes to child b stdin
            dup2(pipebtoa[1], 1); //tie child b write side of the pipes to child b stdout

            //close the file descriptors of the pipe as they have just been copied into child b's fd table
            close(pipeatob[0]);
            close(pipebtoa[1]);

            int status;
            status = chdir(directory_two); //returns 0 on success, -1 on fail. errno is set 
            if(status == -1) { 
                fprintf(stderr, "[FATAL]: child b cannot change directory to %s", directory_two); 
                exit(-1);
            }

            status = system("tail -vn +1 *");
            if(status == -1) {
                fprintf(stderr, "[FATAL]: child b system call tail failed");
                exit(-1);
            }

            //send clear message that there is nothing left in the pipe. otherwise getline may hang
            //an alternative solution is to make the pipes nonblocking and wait for a timeout
            //this runs more reliably on all machines though
            status = system("echo ENDOFFILES");
            if(status == -1) {
                fprintf(stderr, "[FATAL]: child b system call echo failed");
                exit(-1);
            } 

            status = close(1); //nothing left to write, let child a know that we are done
            if(status == -1) {
                fprintf(stderr, "[FATAL]: child b close file descriptor 1 (stdout) failed");
                exit(-1);
            } 
    
            //now we've done our work of sending the information. time to read from child b and make its files.
            char *line = NULL;
            size_t size;
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
            //just the parent.

            //wait for children to terminate, and then exit cleanly
            //you could wait for any two children of the same group id to exit, that way order doesn't matter. 
            //here order is checked, but both are expected to terminate at similar times so it doesn't really matter.
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
