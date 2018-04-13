#include <stdio.h>
#include <stdlib.h>



int main(void) {
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
