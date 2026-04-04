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
    /* Deterministic values roughly in 100–500 W */
    v = 100 + ((i * 47 + 19) % 401);
    printf("POWER %d\n", v);
    pause(PAUSE_TICKS);
  }
  exit(0);
}
