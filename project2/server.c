/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool isStopped[2];

void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct listener_struct {
    int sockfd;
    int speaking_process_idx;
};

void *listenToProcess(void* thread_params) {

    struct listener_struct *ls = thread_params;
    char buffer[256];
    int n;
    while(true) {
        bzero(buffer,256);
        n = read(ls->sockfd, buffer, 255);
        if (n < 0) 
             error("ERROR reading from socket");
        if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
             printf("process %d has stopped\n", ls->speaking_process_idx);
             //close(newsockfd); moving to close all at the end
             //process 2 is no longer open for comms
             isStopped[ls->speaking_process_idx] = true;
             pthread_exit(NULL);
             return NULL;
        }
        printf("process %d: %s", ls->speaking_process_idx, buffer);
     }       
}

int main(int argc, char *argv[])
{
    //arrray of sockfd. 
    //array of portno
     int sockfd, portno, clilen;
     char buffer[256];
     int this_process_idx;

     //array of thread_id
     pthread_t thread_id;

     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     if(argc == 2) {
         //we know we are process 1, so p1 is not stopped
         isStopped[0] = false;
         this_process_idx = 0;


         //make a connection to process 1

         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno = atoi(argv[1]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno);
         if (bind(sockfd, (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding");
         listen(sockfd,5);
         clilen = sizeof(cli_addr);
         sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd < 0) 
              error("ERROR on accept");
         //if we got here, then the connection to process 1 is running
         isStopped[1] =  false;
         struct listener_struct params;
         params.sockfd = sockfd;
         params.speaking_process_idx = 1;
         pthread_create(&thread_id, NULL, &listenToProcess, (void *)&params);
        //successful connection to process 1
     }
     
    while(!isStopped[0] || !isStopped[1]) { 
        if(isStopped[this_process_idx]) continue; //if we are stopped, but not everyone else is, just wait until they are.
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) error("ERROR writing to socket");
        if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
             printf("process %d (me) stopped\n", this_process_idx);
             //close(newsockfd); moving to close all at the end
             //we stopped, mark ourselves as ready to exit
             isStopped[this_process_idx] = true;
        }
     }
    printf("everyone stopped\n");
    pthread_join(thread_id, NULL);
    printf("threads done\n");
    close(sockfd);
    return 0; 
}
