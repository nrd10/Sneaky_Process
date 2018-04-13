#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

int main(void) {
  //get process ID
  int PID;
  PID = getpid();
  printf("Hello Process PID from getpid() is:%d!\n", PID);
  //add number to num
  char num[20];
  sprintf(num, "%d", PID);
  printf("The PID after sprintf is:%s\n", num);
  char str[100];
  strcpy(str, "sudo insmod hello.ko");
  strcat(str, " pid=");
  strcat(str, num);
  printf("My string is:%s\n", str);
  //attempt to load kernel module using insmod
  system("sudo insmod hello.ko");

  //print out log of Kernel Module
  system("dmesg | tail -1");


  //attempt to release kernel module use
  system("sudo rmmod hello.ko");

  //print out log after release of module
  system("dmesg | tail -1");



  return EXIT_SUCCESS;


}
