#include "kernel/types.h"
#include "user/user.h"

#define PHASE_TICKS 130

static int
spawn(char *path, char *name)
{
  int pid = fork();
  if(pid < 0)
    return -1;
  if(pid == 0){
    char *argv[] = { name, 0 };
    exec(path, argv);
    printf("sedemo: exec %s failed\n", path);
    exit(1);
  }
  return pid;
}

static void
drain_waits(void)
{
  int st;
  while(wait(&st) > 0)
    ;
}

static void
run_phase(int mode, char *label)
{
  int hog;
  int sensor;
  int aggr;
  int st;

  if(setschedmode(mode) < 0){
    printf("sedemo: failed to set scheduler mode %d\n", mode);
    exit(1);
  }

  printf("\n=== %s (mode=%d) ===\n", label, mode);
  hog = spawn("cpuhog", "cpuhog");
  sensor = spawn("sensor_normal", "sensor_normal");
  aggr = spawn("aggregator", "aggregator");
  if(hog < 0 || sensor < 0 || aggr < 0){
    printf("sedemo: spawn failed\n");
    if(hog > 0) kill(hog);
    drain_waits();
    exit(1);
  }

  pause(PHASE_TICKS);
  printf("%s snapshot after %d ticks:\n", label, PHASE_TICKS);
  kps("-l");

  kill(hog);
  wait(&st);
  wait(&st);
  wait(&st);
}

int
main(void)
{
  printf("Energy-aware scheduler demo (recommended: run xv6 with CPUS=1 for clearer contrast)\n");
  printf("Workload: cpuhog (infinite), sensor_normal (periodic), aggregator (periodic compute)\n");

  run_phase(0, "Round-robin baseline");
  run_phase(1, "Energy-aware scheduler");

  printf("\nDemo complete. Compare sensor/aggregator responsiveness and cpuhog waste metrics above.\n");
  exit(0);
}
