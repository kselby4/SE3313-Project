#include "kernel/types.h"
#include "user/user.h"

#define RUNTIME 120

int
main(void)
{
  uint t0;
  int wakes;

  t0 = uptime();
  wakes = 0;
  while(uptime() - t0 < RUNTIME){
    wakes++;
    pause(1);
  }

  printf("sensor_normal: wakes=%d in %d ticks (pause(1) per cycle)\n", wakes, RUNTIME);
  exit(0);
}
