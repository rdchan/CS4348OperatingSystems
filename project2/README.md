## Programming Assignment 2

# Precursor
This is an individual project and sharing of code is strictly prohibited. You (yes you, reading the README) are not permitted to copy this code with the intent to submit it as your own work. 

# Introduction
In this project I used sockets and threads to implement an inter-process communication scheme for message passing between 4 processes. There are four processes in the network, and each process is run on a separate machine. The goal is to create reliable socket connections (TCP) between each pair of processes, where each process listens for incoming messages from other nodes and sends input from the user to the other processes. I used threads to guarantee responsiveness with respect to user input. A process has separate threads for listening to each peer, and a fourth thread for user input. There are 3 types of inputs from the user
  
* **Send message to another process** The following command should send a message to the specified process: `send recvr_id MESSAGE` this will send `MESSAGE` to process `recvr_id`(bounded from 1 to 4)
* **Send message to all processes** The following command should send a message to all the other three processes: `send 0 MESSAGE`this will send `MESSAGE` to the other 3 processes
* **Stop** sends a `Stop` message to all other processes and marks own state as *stopped*. A *stopped* process can receive messages (and print them to the screen) but cannot send any more messages. When a processing has received `Stop` from the 3 other processes and is also `stopped`, then the process closes all of its socket connections, collects threads and exits gracefully

# Running
Use the Makefile. I'll have more instructions once it works.

# Video?
I don't know if I'll upload a video or some screenshots. We will see what makes sense.
