//Rishi Chandna
//RDC180001

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/slab.h>

struct birthday {
    int day;
    int month;
    int year;
    struct list_head list;
};

//define and initialize the variable birthday_list, which is of type struct list_head
static LIST_HEAD(birthday_list); 

/* This function is called when the module is loaded. */
int entry_point(void)
{
       //create and intialize 5 instances of struct birthday and add them to the tail
       struct birthday *ptr;
       int i = 0;
       int counter = 1;

       printk(KERN_INFO "Loading Rishi's Module\n");

       for(i = 0; i < 5; i++) {
           struct birthday *person_i;
           person_i = kmalloc(sizeof(*person_i), GFP_KERNEL);
           person_i->day = i+1;
           person_i->month = i+2;
           person_i->year = 2020;
           INIT_LIST_HEAD(&person_i->list);
           list_add_tail(&person_i->list, &birthday_list);
       }
       
       //traverse the list and print the birthdays
       //on each iteration of the linux list.h provided list_for_each_entry, ptr
       // will point to the next birthday struct
       list_for_each_entry(ptr, &birthday_list, list) {
        printk(KERN_INFO "person %d has been added with birthday %d/%d/%d\n", counter, ptr->day, ptr->month, ptr->year);
        counter++;
       }
       

       return 0;
}

/* This function is called when the module is removed. */
void exit_point(void) {
	printk(KERN_INFO "Removing Rishi's Module\n");

    struct birthday *ptr, *next;
    
    int counter = 1;

    struct birthday *ptr2, *next2;
    int counter2 = 1;

    list_for_each_entry_safe(ptr, next, &birthday_list, list) {
        //on each iteration, ptr points to the next birthdady struct
        printk(KERN_INFO "person %d exists whose birthday was %d/%d/%d", counter, ptr->day, ptr->month, ptr->year);
       // list_del(&ptr->list);
       // kfree(ptr);
        counter++;
    }


    list_for_each_entry_safe(ptr2, next2, &birthday_list, list) {
        printk(KERN_INFO "removed person %d whose birthday was %d/%d/%d", counter2, ptr2->day, ptr2->month, ptr2->year);
        list_del(&ptr2->list);
        kfree(ptr2);
        counter2++;
    }

}

/* Macros for registering module entry and exit points. */
module_init( entry_point );
module_exit( exit_point );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Rishi's Module");
MODULE_AUTHOR("SGG");

