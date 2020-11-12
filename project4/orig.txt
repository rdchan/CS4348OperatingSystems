//Rishi Chandna
//RDC180001

#define _BSD_SOURCE;

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kthread.h> //for kernel level threads
#include <linux/semaphore.h> //for kernel level semaphores. they are a struct.
#include <linux/delay.h> //for usleep_range
#include <linux/random.h> //for get_random_bytes

bool all_threads_running; //used to signal that philosophers can start thinking/eating

//mutex for bowl, to ensure that count decreases accordingly when there are multiple modifying threads
static struct semaphore bowl;
static struct semaphore forks[7]; //mutex for acquiring forks
static struct task_struct *philosophers[7]; //task_structs to handle kthreads
static int food_remaining = 30;

/* This function is what all philosopher threads run */
int philosopher(void* args) {
    int my_num = (int)args;
    int sleep_time;
    int eat_time;
    int delete_me;
    int notfinal;
    printk(KERN_INFO "Starting a philosopher\n");

    if(my_num == 0) {
        //usleep_range(1000000, 10000001);
        msleep(3000);
    }
    int primary_fork, secondary_fork; //using an asymmetrical solution
    if(my_num % 2 == 1) {
        primary_fork = my_num;
        secondary_fork = (my_num + 1) % 7;
    } else {
        primary_fork = (my_num + 1) % 7;
        secondary_fork = my_num;
    }
    //printk(KERN_INFO "Hi I am philosopher %d\n", my_num);

    for(delete_me = 0; delete_me < my_num+1; delete_me++) {
        //Think for a little bit
        get_random_bytes(&sleep_time, sizeof(sleep_time));
        sleep_time %= 2000; //range of size 2 seconds
        sleep_time = (sleep_time < 0 ? sleep_time*-1 : sleep_time); //positive
        sleep_time += 2000; //starting at 2 seconds (2-4)
        msleep(sleep_time);

        ////////////////////////
        //  I am hungry       //
        ////////////////////////

        up(&forks[primary_fork]); //acquire primary fork lock
        up(&forks[secondary_fork]); //acquire secondary fork lock

        //acquire lock to read and eat from the bowl
        up(&bowl);
        if(food_remaining > 0) {
            food_remaining--; 
            notfinal= food_remaining;
            down(&bowl); //after reading and writing to the bowl, release it

            //eat for a little bit
            get_random_bytes(&eat_time, sizeof(eat_time));
            eat_time %= 2000; //range of size 2 seconds
            eat_time = (eat_time < 0 ? eat_time*-1 : eat_time); //positive
            eat_time += 1000; //starting at 1 seconds (1-3)
            msleep(eat_time);
            printk(KERN_INFO "Phil %d slept for %d and ate for %d. \t%d food remains\n", my_num, sleep_time, eat_time, notfinal);
            //release the forks
            down(&forks[primary_fork]); //release primary fork lock
            down(&forks[secondary_fork]); //release secondary fork lock
        } else {
            printk(KERN_INFO "Phil %d slept for %d and woke up hungry\n", my_num, sleep_time);
        }

        
    }
    return 0;

}
/* This function is called when the module is loaded. */
int entry_point(void)
{
        int i;
        printk(KERN_INFO "Loading Dining Philosopher Module 1.1\n");

        bool all_threads_running = false;

        //create the 7 philosopher threads
        char our_thread[8] = "thread1";
        for(i = 0; i < 7; i++) {
            philosophers[i] = kthread_create(philosopher, (void*)i,our_thread);
            if(philosophers[i]) {
                wake_up_process(philosophers[i]);
            }

        }



        //now that they are all running, allow them to eat!
        all_threads_running = true;
        
        //wait for them to finish one-by-one in any order
        for(i = 0; i < 7; i++) {
             //collect any terminated thread and print which philosopher finished
        }

        return 0;
} 

/* This function is called when the module is removed. */
void exit_point(void) {

    printk(KERN_INFO "Removing Dining Philosopher Module\n");

    /*
    for(i = 0; i < 7; i++) {
        if(philosophers[i]->state != -1) {
        kthread_stop(philosophers[i]);
        } else {
            printk(KERN_INFO "philosopher %d already stopped\n", i);
        }
    }
    */
    //no dynamically allocated memory to clear

}

/* Macros for registering module entry and exit points. */
module_init( entry_point );
module_exit( exit_point );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Rishi's Module");
MODULE_AUTHOR("SGG");

