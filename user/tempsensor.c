#include "kernel/types.h"
#include "user/user.h"

/* Must match aggregator and sibling sensors for synchronized reads. */
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
    /* Deterministic wobble in ~20–30 °C, small span so summary can read "stable". */
    v = 23 + (i % 5);
    if(v < 20)
      v = 20;
    if(v > 30)
      v = 30;
    printf("TEMP %d\n", v);
    pause(PAUSE_TICKS);
  }
  exit(0);
}
