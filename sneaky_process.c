#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(void) {
  //get process ID
  int PID;
  PID = getpid();
   //add number to num
  char num[20];
  //get PID into string
  sprintf(num, "%d", PID);
  char str[100];
  strcpy(str, "sudo insmod sneaky_mod.ko");
  strcat(str, " pid=");
  strcat(str, num);
  //attempt to load kernel module using insmod
  system(str);
  
  //attempt to release kernel module use
  system("sudo rmmod sneaky_mod.ko");

  return EXIT_SUCCESS;

}
