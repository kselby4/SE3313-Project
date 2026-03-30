#include "kernel/types.h"
#include "user/user.h"

#define RUNTIME 120
#define ECO_PERIOD 10

int
main(void)
{
  uint t0;
  int wakes;

  if(setecoperiod(ECO_PERIOD) < 0){
    printf("sensor_eco: setecoperiod failed\n");
    exit(1);
  }

  t0 = uptime();
  wakes = 0;
  while(uptime() - t0 < RUNTIME){
    wakes++;
    pause(1);
  }

  printf("sensor_eco: wakes=%d in %d ticks (eco period=%d, pause(1) stretched by kernel)\n",
         wakes, RUNTIME, ECO_PERIOD);
  exit(0);
}
