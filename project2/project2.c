#include <pthread.h> //for creating threads
#include <unistd.h> //syscall wrappers (read, write etc)
#include <stdio.h> //stream model of i/o. used in tcp sockets and fprintf
#include <sys/types.h> //socket libraries
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h> //for using "true" and "false" instead of 1 and 0
#include <stdlib.h> //numeric conversions, atoi
#include <string.h> //because I'm sloppy with char arrays

//contains the state of each of the 4 processes. 
//once a connection is established, they are set to false (running)
//upon hearing a Stop, the state is set to Stopped
bool isStopped[4];

//clean/readable error messages
void error(char *msg)
{
    perror(msg);
    exit(1);
}

//passing data through thread function
struct listener_struct {
    int sockfd;
    int speaking_process_idx;
};

//takes sockfd and speaking process idx
//thread function, called once a connection is established to a process
//there will be 3 running concurrently
void *listenToProcess(void* thread_params) {

    struct listener_struct *ls = thread_params;
    char buffer[256];
    int n;

    while(true) {
        //zero out buffer for null terminataing character, memset works too
        bzero(buffer,256);
        n = read(ls->sockfd, buffer, 255); //reads from socket, n is number of bytes/characters
        if (n < 0) 
             error("ERROR reading from socket");

        //check if we were sent Stop, the stop message
        //if(buffer[0] == 'S' && buffer[1] == 't' && buffer[2] == 'o' && buffer[3] == 'p') {
        if(strncmp(buffer, "Stop", 4) == 0) {
             fprintf(stderr, "process %d has stopped\n", ls->speaking_process_idx + 1);
             //process we are listening to is no longer open for comms
             isStopped[ls->speaking_process_idx] = true;
             //cleanly exit the calling thread and the function
             pthread_exit(NULL);
             return NULL;
        }
        //if it was any message sent to us other than Stop, print it out
        fprintf(stdout, "process %d: %s", ls->speaking_process_idx + 1, buffer);
     }       
}

int main(int argc, char *argv[])
{
     //sockfd's hold file descriptors for socket connections to the other 3 processes
     //portno holds the port number, read from args, used to establish socket
     //clilen is just used to establish socket
     //buffer is used for capturing user input and n is for number of bytes read/written
     int sockfd[3], portno[3], clilen, n;
     char buffer[256];

     //is this process 0, 1, 2, 3? determined from number of args passed in
     int this_process_idx;

     //holds thread id for the 3 listening threads and argument structs for making them
     pthread_t thread_id[3];
     struct listener_struct params[3];

     //used for server connections
     struct sockaddr_in serv_addr;
     //used for server and client connections
     struct sockaddr_in cli_addr;
     struct hostent *server;

     if (argc < 4) {
         fprintf(stderr,"usage ./project2 port port port or \n./project2 machine port port port or \n./project2 machine port machine port port or \n./project2 machine port machine port machine port");
         exit(1);
     }

     //if we have enough args, start preparing the sockets
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
         //format: ./project2 portno1 portno2 portno3
         //we are going to have 3 server type sockets
         //we know we are process 0, so p0 is not stopped
         isStopped[0] = false;
         this_process_idx = 0;
        
         //***block and make a connection to process 2***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[0] = atoi(argv[1]); 
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[0]);
         if (bind(sockfd[0], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p2");
         n = listen(sockfd[0],5); 
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[0] = accept(sockfd[0], (struct sockaddr *) &cli_addr, &clilen); //this is a blocking call
         if (sockfd[0] < 0) 
              error("ERROR on accepting p2");
         //if we got here, then the connection to process 2 is good.
         //it isn't stopped, so create a thread to listen to it
         isStopped[1] =  false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 1;
         n = pthread_create(&thread_id[0], NULL, &listenToProcess, (void *)&params[0]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 1 connected to process 2\n");
        //***successful connection to process 2***

        //***block and make a connection to process 3***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[1] = atoi(argv[2]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[1]);
         if (bind(sockfd[1], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p2");
         n = listen(sockfd[1],5);
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[1] = accept(sockfd[1], (struct sockaddr *) &cli_addr, &clilen); //this is a blocking call
         if (sockfd[1] < 0) 
              error("ERROR on accepting p2");
         //if we got here, then the connection to process 3 is running
         isStopped[2] =  false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 2;
         n = pthread_create(&thread_id[1], NULL, &listenToProcess, (void *)&params[1]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 1 connected to process 3\n");
        //***successful connection to process 3***

        //***block and make a connection to process 4***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[3]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p4");
         n = listen(sockfd[2],5);
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p4");
         //if we got here, then the connection to process 4 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         n = pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 1 connected to process 4\n");
        //***successful connection to process 4***

     } else if(argc == 5) {

         //we know we are process 2, so p2 is not stopped
         isStopped[1] = false;
         this_process_idx = 1;

         //order is ./project2 Machine1 portno1 portno2 portno3
         //where machine is something like net03.utdallas.edu

         //***block and bind to process 1 as a client***
         portno[0] = atoi(argv[2]);          //get portnumber and machine from args
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
         serv_addr.sin_port = htons(portno[0]); //host endianness to network endianess?
         //blocking call
         while(connect(sockfd[0],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         //from here on is successful connection
         isStopped[0] = false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 0;
         n = pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 2 connected to process 1\n");
         //***successful connection to process 1***

        //***block and make a connection to process 3***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[1] = atoi(argv[3]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[1]);
         if (bind(sockfd[1], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p3");
         n = listen(sockfd[1],5);
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[1] = accept(sockfd[1], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[1] < 0) 
              error("ERROR on accepting p3");
         //if we got here, then the connection to process 3 is running
         isStopped[2] =  false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 2;
         n = pthread_create(&thread_id[1], NULL, &listenToProcess, (void *)&params[1]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 2 connected to process 3\n");
        //***successful connection to process 3***


        //***block and make a connection to process 4***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[4]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p3");
         n = listen(sockfd[2],5);
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p3");
         //if we got here, then the connection to process 4 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         n = pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 2 connected to process 4\n");
        //***successful connection to process 4***
        
     } else if (argc == 6) {

         //we know we are process 3, so p3 is not stopped
         isStopped[2] = false;
         this_process_idx = 2;

         //order is ./project2 Machine1 portno1 Machine2 portno2 portno3
         //where machine is something like net03.utdallas.edu
         
         //***block and bind to process 1 as a client***
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
         //blocking portion.
         while(connect(sockfd[0],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[0] = false;
         params[0].sockfd = sockfd[0];
         params[0].speaking_process_idx = 0;
         n = pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 3 connected to process 1\n");
         //***successful connection to processs 1***

         //***block and bind to process 2 as a client***
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
         //blocking portion. 
         while(connect(sockfd[1],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[1] = false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 1;
         n = pthread_create(&thread_id[1], NULL, &listenToProcess, (void  *)&params[1]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 3 connected to process 2\n");
         //***sucessful connection to process 2***

        //***block and make a connection to process 4***
         bzero((char *) &serv_addr, sizeof(serv_addr));
         portno[2] = atoi(argv[5]);
         serv_addr.sin_family = AF_INET;
         serv_addr.sin_addr.s_addr = INADDR_ANY;
         serv_addr.sin_port = htons(portno[2]);
         if (bind(sockfd[2], (struct sockaddr *) &serv_addr,
                  sizeof(serv_addr)) < 0) 
                  error("ERROR on binding to p4");
         n = listen(sockfd[2],5);
         if(n < 0)
             error("ERROR on listen");
         clilen = sizeof(cli_addr);
         sockfd[2] = accept(sockfd[2], (struct sockaddr *) &cli_addr, &clilen);
         if (sockfd[2] < 0) 
              error("ERROR on accepting p4");
         //if we got here, then the connection to process 4 is running
         isStopped[3] =  false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 3;
         n = pthread_create(&thread_id[2], NULL, &listenToProcess, (void *)&params[2]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 3 connected to process 4\n");
        //***successful connection to process 4***
        
     } else if(argc == 7) {

         //we know we are process 4, so p4 is not stopped
         isStopped[3] = false;
         this_process_idx = 3;

         //order is ./project2 Machine1 portno1 Machine2 portno2 Machine3 portno3
         //where machine is something like net03.utdallas.edu
         
         //***block and bind to process 1 as a client***
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
         n = pthread_create(&thread_id[0], NULL, &listenToProcess, (void  *)&params[0]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 4 connected to process 1\n");
         //***successful connection to processs 1***


         //***block and bind to process 2 as a client***
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
         //blocking portion. 
         while(connect(sockfd[1],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[1] = false;
         params[1].sockfd = sockfd[1];
         params[1].speaking_process_idx = 1;
         n = pthread_create(&thread_id[1], NULL, &listenToProcess, (void  *)&params[1]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 4 connected to process 2\n");
         //***sucessful connection to process 2***

         //***block and bind to process 3 as a client***
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
         //blocking portion. 
         while(connect(sockfd[2],(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0);
         isStopped[2] = false;
         params[2].sockfd = sockfd[2];
         params[2].speaking_process_idx = 2;
         n = pthread_create(&thread_id[2], NULL, &listenToProcess, (void  *)&params[2]);
         if(n != 0)
             error("ERROR on pthread");
         fprintf(stderr, "process 4 connected to process 3\n");
         //***sucessful connection to process 3***

     } else {
         fprintf(stderr, "usage %s port port port or \n%s machine port port port or \n%s machine port machine port port or \n%s machine port machine port machine port", argv[0]);
         exit(0);
     }
     
     //at this point all the socket connections have been made, and listening threads have been created
     //the next execution is only for user input
     //continue while not all processes are Stopped
    while(!isStopped[0] || !isStopped[1] || !isStopped[2] || !isStopped[3]) { 
        if(isStopped[this_process_idx]) continue; //if we are stopped, but not everyone else is, just wait until they are. don't send any more messages
        //read input
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
       // if(buffer[0] == 's' && buffer[1] == 'e' && buffer[2] == 'n' && buffer[3] == 'd') {
        if(strncmp(buffer, "send", 4) == 0 && strlen(buffer) > 7) {
            char *message = buffer;
            message += 7; 
            int receiver_id = buffer[5] - '0'; //pull number from character ascii subtraction
            if(strncmp(message, "Stop", 4) == 0) {
                fprintf(stderr, "Stop is a reserved message, not sending it\n");
                continue;
            }
            if(receiver_id == 0) {
                //send to all and error check
                for(int i = 0; i < 3; i++) {
                    n = write(sockfd[i], message, strlen(message));
                    if (n < 0) error("ERROR writing to socket");
                }
            } else if (receiver_id < 5) {
                receiver_id--; //go down one to match idx
                //if we are processs 1, then the other process are 0, 2, 3. but they are sockfd idx 0, 1, 2. 
                //anything larger than this idx should go down by 1
                if(receiver_id > this_process_idx) receiver_id--; 
                n = write(sockfd[receiver_id], message, strlen(message));
                if (n < 0) error("ERROR writing to socket");
            }
        } else if(strncmp(buffer, "Stop", 4) == 0) {
            for(int i = 0; i < 3; i++) {
                n = write(sockfd[i],buffer,strlen(buffer));
                if (n < 0) error("ERROR writing to socket");
            }
            fprintf(stderr, "process %d (me) has stopped\n", this_process_idx+1);
            //we stopped, mark ourselves as ready to exit
            isStopped[this_process_idx] = true;
        } else {
            fprintf(stderr, "Usage: send d message or Stop\n");
        }
     }
    
    //once all processes are marked as stopped (exited while loop) collect all threads
    for(int i = 0; i < 3; i++) {
        n = pthread_join(thread_id[i], NULL);
        if(n != 0)
            error("ERROR on pthread");
    }
    //then close all socket connectitons
    for(int i = 0; i < 3; i++) {
        n = close(sockfd[i]);
        if(n != 0)
            error("ERROR on close");
    }

    return 0; 
}
