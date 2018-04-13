#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lakshmanan");
MODULE_DESCRIPTION("A Simple Hello World module");


//Global Variables for insmod: just need PID
static int   pid = -1;
//obtain PID from outer program
module_param(pid, int, 0);



static int __init hello_init(void)
{
  printk(KERN_INFO "Hello world!\n");

  printk(KERN_ALERT "Hello process ID is: %i!\n", pid);
  
  return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit hello_cleanup(void)
{
  printk(KERN_INFO "Cleaning up module.\n");
}

module_init(hello_init);
module_exit(hello_cleanup);
