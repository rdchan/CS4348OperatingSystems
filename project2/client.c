#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

bool isStopped[2];

void error(char *msg)
{
    perror(msg);
    exit(0);
}

struct listener_struct {
    int sockfd;
    int speaking_process_idx;
};

void *listenToServer(void* thread_params) {
    struct listener_struct *ls = thread_params;

    char buffer[256];
    int n;
    while(true) {
        bzero(buffer,256);
        n = read(ls->sockfd,buffer,255);
        if (n < 0) 
             error("ERROR reading from socket");
        if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
             printf("server stopped (process %d)\n", ls->speaking_process_idx);
             //close(sockfd); moving to close sockets at exit
             //process 1 no longer open for comms
             isStopped[0] = true;
             pthread_exit(NULL);
             return NULL;
        }
        printf("Server said: %s",buffer);
     }       
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    pthread_t thread_id;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    //we know we are process 2, and we are running
    isStopped[1] = false;
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    while (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) ;
  //  if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
    //    error("ERROR connecting");
    //by now we've connected to process 1 and it must be running
    isStopped[0] = false;
    struct listener_struct params;
    params.sockfd = sockfd;
    params.speaking_process_idx = 0;
    pthread_create(&thread_id, NULL, &listenToServer, (void *)&params);
    while(!isStopped[0] || !isStopped[1]) {
        if(isStopped[1]) continue; //if we are stopped, but not everyone is, then wait for them to stop
        bzero(buffer,256);
        fgets(buffer,255,stdin);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) 
             error("ERROR writing to socket");
        
        if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
             printf("client stopped\n");
             //close(sockfd); moving to close all at the end
             //we have stopped. mark ourselves as ready to exit
             isStopped[1] = true;

         }
    }
    printf("everyone stopped\n");
    pthread_join(thread_id, NULL);
    printf("threads done\n");
    close(sockfd);
    return 0;
}
