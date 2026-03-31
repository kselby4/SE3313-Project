#include "kernel/types.h"
#include "user/user.h"

#define RUNTIME 120
#define PERIOD 4

int
main(void)
{
  uint start;
  uint last;
  int cycles;
  int late_cycles;
  volatile uint64 sum;

  start = uptime();
  last = start;
  cycles = 0;
  late_cycles = 0;
  sum = 0;

  while(uptime() - start < RUNTIME){
    uint now;

    pause(PERIOD);
    now = uptime();
    if(now - last > PERIOD + 2)
      late_cycles++;
    last = now;

    for(int i = 0; i < 50000; i++)
      sum += (uint64)(i ^ (cycles + 1));
    cycles++;
  }

  // Prevent compiler from optimizing away the loop.
  if(sum == 0xdeadbeefULL)
    printf("aggregator: impossible value\n");

  printf("aggregator: cycles=%d late=%d runtime=%d ticks\n", cycles, late_cycles, RUNTIME);
  exit(0);
}
