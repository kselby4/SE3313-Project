#include "kernel/types.h"
#include "user/user.h"

//helper function to make a child process
static int
spawn(char *path, char *name)
{
  int pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    char *argv[] = { name, 0 };
    exec(path, argv);
    exit(1);
  }
  return pid;
}

int
main(void)
{
  //creates a child process that ineffiecent 
  int hog = spawn("cpuhog", "cpuhog");

  //creates a child process that runs sensor
  int sensor = spawn("sensor", "sensor");
  int status;

  if(hog < 0 || sensor < 0){
    printf("energydemo: failed to start demo processes\n");
    if(hog > 0)
      kill(hog);
    if(sensor > 0)
      kill(sensor);
    exit(1);
  }

  //Let both processes run long enough for score differences to emerge.
  pause(120);
  printf("\nEnergy snapshot after 120 ticks:\n");
  kps("-l");

  kill(hog);
  kill(sensor);
  wait(&status);
  wait(&status);
  exit(0);
}
