//Rishi Chandna
//RDC180001

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/kthread.h> //for kernel level threads
#include <linux/semaphore.h> //for kernel level semaphores. they are a struct.
#include <linux/delay.h> //for usleep_range
#include <linux/random.h> //for get_random_bytes to create a random integer

//mutex for bowl, to ensure that count decreases accordingly when there are multiple modifying threads
static struct semaphore bowl;
static struct semaphore forks[7]; //mutex for acquiring forks
static struct task_struct *philosophers[7]; //task_structs to handle kthreads
static int food_remaining = 30;

/* This function is what all philosopher threads run */
int philosopher(void* args) {
    int my_num = (int)args; //parse the input to know what this philosophers seat is.
    int sleep_time; //used as temporary calc for get_random_bytes.
    int eat_time; //same as above, just for readability.
    int sleep_times[30]; //record all sleep times. 30 is max if this phil eats all 30.
    int eat_times[30]; //same as above but for eating times 29 is actually max, but symmetry.
    int cycle = 0; //used to index logging arrays
    int print_counter; //used to iterate through times at the end for logging

    int primary_fork, secondary_fork; //using an asymmetrical solution
    if(my_num % 2 == 1) { //odd numbered philosophers take their right fork first
        primary_fork = my_num;
        secondary_fork = (my_num + 1) % 7;
    } else { //even numbered philosophers take their left fork first
        primary_fork = (my_num + 1) % 7;
        secondary_fork = my_num;
    }

    while(true) {
        //Think for a little bit
        get_random_bytes(&sleep_time, sizeof(sleep_time));
        sleep_time %= 2000; //range of size 2 seconds
        sleep_time = (sleep_time < 0 ? sleep_time*-1 : sleep_time); //positive
        sleep_time += 2000; //starting at 2 seconds (2-4)
        sleep_times[cycle] = sleep_time;
        msleep(sleep_time);

        ////////////////////////
        //  I am hungry !!!   //
        ////////////////////////

        //get my forks! 
        down(&forks[primary_fork]); //acquire primary fork lock
        down(&forks[secondary_fork]); //acquire secondary fork lock

        //acquire lock to read and eat from the bowl
        down(&bowl);
        if(food_remaining > 0) {
            //////////////////
            // eating time! //
            //////////////////

            food_remaining--; 
            printk(KERN_INFO "Phil %d ate.\t%d food remains", my_num, food_remaining);
            up(&bowl); //after reading and writing to the bowl, release it

            //eat for a little bit
            get_random_bytes(&eat_time, sizeof(eat_time));
            eat_time %= 2000; //range of size 2 seconds
            eat_time = (eat_time < 0 ? eat_time*-1 : eat_time); //positive
            eat_time += 1000; //starting at 1 seconds (1-3)
            eat_times[cycle] = eat_time;
            msleep(eat_time);
            //release the forks
            up(&forks[primary_fork]); //release primary fork lock
            up(&forks[secondary_fork]); //release secondary fork lock

        } else {
            /////////////////////////
            //     no more food    //
            // print logs and exit //
            /////////////////////////

            up(&bowl);
            up(&forks[primary_fork]); //release primary fork lock
            up(&forks[secondary_fork]); //release secondary fork lock
            printk(KERN_INFO "Phil %d on cycle %d woke up to no food after sleeping for %d\n", my_num, cycle, sleep_times[cycle]);
            for(print_counter = 0; print_counter < cycle; print_counter++) {
            printk(KERN_INFO "\t%d) Phil %d slept for %dms and ate for %dms\n", print_counter+1, my_num, sleep_times[print_counter], eat_times[print_counter]);
            }
            return 0;
        }
        cycle++; //just for indexing the records to log at the end
    }
    return 0;

}
/* This function is called when the module is loaded. */
int entry_point(void)
{
        int i;
        printk(KERN_INFO "Loading Dining Philosopher Module 2.1\n");

        //initialize all semaphores (operataing as mutex, so start value is 1)
        sema_init(&bowl, 1);

        for(i = 0; i < 7; i++) {
            sema_init(&forks[i], 1);
        }

        //don't let any philosophers enter eating until all threads are made
        down(&bowl);
        //create the 7 philosopher threads
        char kthreadfmt[5] = "empty";
        for(i = 0; i < 7; i++) {
            philosophers[i] = kthread_create(philosopher, (void*)i,kthreadfmt);
            get_task_struct(philosophers[i]);
            if(philosophers[i]) {
                wake_up_process(philosophers[i]);
            }
        }

        //now that they are all running, allow them to eat!
        printk(KERN_INFO "All threads running, let the eating begin!\n");
        up(&bowl);
        

        return 0;
} 

/* This function is called when the module is removed. */
void exit_point(void) {


    int i; //counter for the loop below

    //make sure that threads have stopped before finishing
    //if a thread is still going, let it finish, then stop it.
    for(i = 0; i < 7; i++) {
        kthread_stop(philosophers[i]);
        put_task_struct(philosophers[i]);
    }

    //no dynamically allocated memory to clear, so we're good to exit
    printk(KERN_INFO "Removing Dining Philosopher Module\n");

}

/* Macros for registering module entry and exit points. */
module_init( entry_point );
module_exit( exit_point );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Rishi's Module for Project 4");
MODULE_AUTHOR("SGG");

