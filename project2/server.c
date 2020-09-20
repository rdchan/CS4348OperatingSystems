/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool isStopped[4];

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
             fprintf(stderr, "process %d has stopped\n", ls->speaking_process_idx);
             //close(newsockfd); moving to close all at the end
             //process we are listening to is no longer open for comms
             isStopped[ls->speaking_process_idx] = true;
             pthread_exit(NULL);
             return NULL;
        }
        fprintf(stderr, "process %d: %s", ls->speaking_process_idx + 1, buffer);
     }       
}

int main(int argc, char *argv[])
{
    //arrray of sockfd. 
    //array of portno
    //for now use sockfd[i] to communicate with process i
    //this wastes 2 spots, but is easier to read
    //TODO: something like sockfd[(i-this_p_idx-1)%4] to access process i (i from 1 to 4)
     int sockfd[3], portno[3], clilen;
     char buffer[256];
     int this_process_idx;

     //array of thread_id
     pthread_t thread_id[4];

     struct listener_struct params[3];
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     sockfd[0] = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd[0] < 0) 
        error("ERROR opening socket");

     sockfd[1] = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd[1] < 0) 
        error("ERROR opening socket");

     sockfd[2] = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd[2] < 0) 
        error("ERROR opening socket");

     if(argc == 4) {
         //we know we are process 0, so p0 is not stopped
         isStopped[0] = false;
         this_process_idx = 0;

         //order is ./project2 portno1 portno2 portno3

         //block and make a connection to process 1
         struct sockaddr_in serv_addr, cli_addr;

         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[0] = atoi(argv[1]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[0]);
         if (bind(sockfd[0], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p1");
         listen(sockfd[0],5);
         clilen = sizeof(cli_addr);
         sockfd[0] = accept(sockfd[0], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[0] < 0) 
              error("ERROR on accepting p1");
         //if we got here, then the connection to process 1 is running
         isStopped[1] =  false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 1;
         pthread_create(&thread_id[0], NULL, &listenToProcess, (void *)&params[0]);
         fprintf(stderr, "process 0 connected to process 1\n");
        //successful connection to process 1

        //block and make a connection to process 2
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[1] = atoi(argv[2]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[1]);
         if (bind(sockfd[1], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p2");
         listen(sockfd[1],5);
         clilen = sizeof(cli_addr);
         sockfd[1] = accept(sockfd[1], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[1] < 0) 
              error("ERROR on accepting p2");
         //if we got here, then the connection to process 2 is running
         isStopped[2] =  false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 2;
         pthread_create(&thread_id[1], NULL, &listenToProcess, (void *)&params[1]);
         fprintf(stderr, "process 0 connected to process 2\n");
        //successful connection to process 2

        //block and make a connection to process 3
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[3]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p3");
         listen(sockfd[2],5);
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p3");
         //if we got here, then the connection to process 3 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         fprintf(stderr, "process 0 connected to process 3\n");
        //successful connection to process 3

     } else if(argc == 5) {

         //we know we are process 1, so p1 is not stopped
         isStopped[1] = false;
         this_process_idx = 1;

         //order is ./project2 Machine1 portno1 portno2 portno3
         //where machine is something like dc03
         
         struct sockaddr_in serv_addr, cli_addr;
         struct hostent *server;

         //block and bind to process 0 as a client
         portno[0] = atoi(argv[2]);
         server = gethostbyname(argv[1]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[0]);

         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[0],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[0] = false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 0;
         pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         fprintf(stderr, "process 1 connected to process 0\n");

        //block and make a connection to process 2
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[1] = atoi(argv[3]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[1]);
         if (bind(sockfd[1], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p2");
         listen(sockfd[1],5);
         clilen = sizeof(cli_addr);
         sockfd[1] = accept(sockfd[1], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[1] < 0) 
              error("ERROR on accepting p2");
         //if we got here, then the connection to process 2 is running
         isStopped[2] =  false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 2;
         pthread_create(&thread_id[1], NULL, &listenToProcess, (void *)&params[1]);
         fprintf(stderr, "process 1 connected to process 2\n");
        //successful connection to process 2

        //block and make a connection to process 3
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[4]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p3");
         listen(sockfd[2],5);
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p3");
         //if we got here, then the connection to process 3 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         fprintf(stderr, "process 1 connected to process 3\n");
        //successful connection to process 3
        
     } else if (argc == 6) {

         //we know we are process 2, so p2 is not stopped
         isStopped[2] = false;
         this_process_idx = 2;

         //order is ./project2 Machine1 portno1 Machine2 portno2 portno3
         //where machine is something like dc03
         
         struct sockaddr_in serv_addr, cli_addr;
         struct hostent *server;

         //block and bind to process 0 as a client
         portno[0] = atoi(argv[2]);
         server = gethostbyname(argv[1]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[0]);
         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[0],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[0] = false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 0;
         pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         fprintf(stderr, "process 2 connected to process 0\n");
         //successful connection to processs 0

         //block and bind to process 1 as a client
         portno[1] = atoi(argv[4]);
         server = gethostbyname(argv[3]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[1]);
         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[1],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[1] = false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 1;
         pthread_create(&thread_id[1], NULL, &listenToProcess, (void  *)&params[1]);
         fprintf(stderr, "process 2 connected to process 1\n");
         //sucessful connection to process 1

        //block and make a connection to process 3
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[5]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p3");
         listen(sockfd[2],5);
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p3");
         //if we got here, then the connection to process 3 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         fprintf(stderr, "process 2 connected to process 3\n");
        //successful connection to process 3
        
     } else if(argc == 7) {

         //we know we are process 3, so p3 is not stopped
         isStopped[3] = false;
         this_process_idx = 3;

         //order is ./project2 Machine1 portno1 Machine2 portno2 Machine3 portno3
         //where machine is something like dc03
         
         struct sockaddr_in serv_addr, cli_addr;
         struct hostent *server;

         //block and bind to process 0 as a client
         portno[0] = atoi(argv[2]);
         server = gethostbyname(argv[1]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[0]);
         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[0],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[0] = false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 0;
         pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         fprintf(stderr, "process 3 connected to process 0\n");
         //successful connection to processs 0

         //block and bind to process 1 as a client
         portno[1] = atoi(argv[4]);
         server = gethostbyname(argv[3]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[1]);
         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[1],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[1] = false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 1;
         pthread_create(&thread_id[1], NULL, &listenToProcess, (void  *)&params[1]);
         fprintf(stderr, "process 3 connected to process 1\n");
         //sucessful connection to process 1

         //block and bind to process 2 as a client
         portno[2] = atoi(argv[6]);
         server = gethostbyname(argv[5]);
         if(server == NULL) {
             fprintf(stderr, "ERROR, no such host\n");
             exit(0);
         }
         bzero((char *) &serv_addr, sizeof(serv_addr));
         serv_addr.sin_family = AF_INET;
         bcopy((char *)server->h_addr,
                 (char *)&serv_addr.sin_addr.s_addr,
                 server->h_length);
         serv_addr.sin_port = htons(portno[2]);
         //blocking portion. if we run into a deadlock, consider an array of serv_addr and a non-shortcircuiting while loop
         while(connect(sockfd[2],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[2] = false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 2;
         pthread_create(&thread_id[2], NULL, &listenToProcess, (void  *)&params[2]);
         fprintf(stderr, "process 3 connected to process 2\n");
         //sucessful connection to process 2

     } else {
         fprintf(stderr, "usage %s port port port or \n%s machine port port port or \n%s machine port machine port port or \n%s machine port machine port machine port", argv[0]);
         exit(0);
     }
     
    while(!isStopped[0] || !isStopped[1] || !isStopped[2] || !isStopped[3]) { 
        if(isStopped[this_process_idx]) continue; //if we are stopped, but not everyone else is, just wait until they are.
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        if(buffer[0] == 's' && buffer[1] == 'e' && buffer[2] == 'n' && buffer[3] == 'd') {
            
            char *message = buffer;
            message += 7;
            int receiver_id = buffer[5] - '0';
            if(receiver_id == 0) {
                //send to all and error check
                for(int i = 0; i < 3; i++) {
                    n = write(sockfd[i], message, strlen(message));
                    if (n < 0) error("ERROR writing to socket");
                }
            } else if (receiver_id < 5) {
                receiver_id--;
                if(receiver_id > this_process_idx) receiver_id--;
                n = write(sockfd[receiver_id], message, strlen(message));
                if (n < 0) error("ERROR writing to socket");
            }
        } else if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
            for(int i = 0; i < 3; i++) {
                n = write(sockfd[i],buffer,strlen(buffer));
                if (n < 0) error("ERROR writing to socket");
            }
            fprintf(stderr, "process %d (me) stopped\n", this_process_idx);
            //close(newsockfd); moving to close all at the end
            //we stopped, mark ourselves as ready to exit
            isStopped[this_process_idx] = true;
        } else {
            fprintf(stderr, "Usage: send d message or Stop\n");
        }
     }
    for(int i = 0; i < 3; i++)
        pthread_join(thread_id[i], NULL);
    for(int i = 0; i < 3; i++)
        close(sockfd[i]);

    return 0; 
}
