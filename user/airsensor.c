#include "kernel/types.h"
#include "user/user.h"

#define NUM_ITER 8
#define PAUSE_TICKS 3

int
main(int argc, char **argv)
{
  int i;
  int v;

  (void)argc;
  (void)argv;

  for(i = 0; i < NUM_ITER; i++){
    /* Deterministic values roughly in 40–80 (AQI-style) */
    v = 45 + ((i * 11 + 5) % 36);
    if(v < 40)
      v = 40;
    if(v > 80)
      v = 80;
    printf("AIR %d\n", v);
    pause(PAUSE_TICKS);
  }
  exit(0);
}
