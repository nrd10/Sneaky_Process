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
  strcpy(str, "sudo insmod hello.ko");
  strcat(str, " pid=");
  strcat(str, num);
  //attempt to load kernel module using insmod
  system(str);
  
  //print out log of Kernel Module
  system("dmesg | tail -1");


  //attempt to release kernel module use
  system("sudo rmmod hello.ko");

  //print out log after release of module
  system("dmesg | tail -1");



  return EXIT_SUCCESS;


}
