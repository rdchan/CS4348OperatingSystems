## Programming Assignment 3

# Precursor
This is an individual project and sharing of code is strictly prohibited. You (yes you, reading the README) are not permitted to copy this code with the intent to submit it as your own work. 

# Introduction
In Part 1 of this project I create a kernel module, then load and unload it. The kernel module will use `printk()` statements to add messages to the kernel log buffer that cana be checked with `dmesg`

In Part 2 of this project I  modify the entry and exit points of a kernel module to use elements of the linux header  `list.h` that implements a simple doubly linked list. 

* In the entry point,  I create a linked list with five `struct birthday` type elements, and traverse the list while outputting its contents to tthe kernel log buffer. This can be checked with `dmesg` once the module is loaded.  
* In the exit point, I delete the elements from the linked list and return the free memory back to the kernel. This can be checked with `dmesg` after the kernel module is unloaded

Below is still project 2...
# Running
Use the Makefile, this will generate a `project2` exectuble at `bin/project2`. From here run 4 instances of the program, either use 4 terminal emulators or a program like `tmux`. For running on 4 `netXX` machines, here is an example of how to run the program

Process 1  `you@netAA > ./project2 5050 5051 5052`

Process 2 `you@netBB > ./project2 netAA.utdallas.edu 5050 5053 5054`

Process 3 `you@netCC > ./project2 netAA.utdallas.edu 5051 netBB.utdallas.edu 5053 5055`

Process 4 `you@netCC > ./project2 netAA.utdallas.edu 5052 netBB.utdallas.edu 5054 netCC.utdallas.edu 5055`

To run on a local machine, replace all `netXX.utdallas.edu` addresses with `localhost`. This is what the connections look like:

![connection layout](images/socketmap.png)
# Video?
I don't know if I'll upload a video, but here is a screenshot of the program running on 4 `netXX` machines. The order may be a little hard to figure out, lines preceded by `process #:` are printing what was sent to the running process, and lines with `send` and `Stop` were typed into that terminal and sent out.

![net run](images/screenshot.png)edu 5052 netBB.utdallas.edu 5054 netCC.utdallas.edu 5055`

To run on a local machine, replace all `netXX.utdallas.edu` addresses with `localhost`. This is what the connections look like:

![connection layout](images/socketmap.png)
# Video?
I don't know if I'll upload a video, but here is a screenshot of the program running on 4 `netXX` machines. The order may be a little hard to figure out, lines preceded by `process #:` are printing what was sent to the running process, and lines with `send` and `Stop` were typed into that terminal and sent out.

![net run](images/screenshot.png)