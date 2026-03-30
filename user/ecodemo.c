#include "kernel/types.h"
#include "user/user.h"

static int
spawn(char *path, char *name)
{
  int pid;

  pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    char *argv[] = { name, 0 };
    exec(path, argv);
    printf("ecodemo: exec %s failed\n", path);
    exit(1);
  }
  return pid;
}

int
main(void)
{
  int status;

  printf("Eco sleep demo: normal vs eco sensor (same pause(1) loop, fixed runtime)\n\n");

  printf("--- Normal sensor ---\n");
  if(spawn("sensor_normal", "sensor_normal") < 0){
    printf("ecodemo: could not start sensor_normal\n");
    exit(1);
  }
  wait(&status);

  printf("\n--- Eco sensor (setecoperiod in sensor_eco) ---\n");
  if(spawn("sensor_eco", "sensor_eco") < 0){
    printf("ecodemo: could not start sensor_eco\n");
    exit(1);
  }
  wait(&status);

  printf("\nProcess list (ps -l): compare RUNNING / CSW / SLEEPING and DUTY column.\n");
  kps("-l");

  exit(0);
}
