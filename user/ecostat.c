#include "kernel/types.h"
#include "user/user.h"

#define NSTATS 64

int
main(int argc, char *argv[])
{
  struct ecostat buf[NSTATS];
  int n, i;

  (void)argc;
  (void)argv;

  n = getecostats(buf, NSTATS);
  if(n < 0){
    printf("getecostats: error\n");
    exit(1);
  }

  printf("PID | NAME | CPU | RUNBL | SLEEP | CSW\n");
  printf("----+------+-----+-------+-------+----\n");
  for(i = 0; i < n; i++){
    printf("%d | %s | %d | %d | %d | %d\n",
           buf[i].pid, buf[i].name,
           buf[i].running_ticks, buf[i].runnable_ticks,
           buf[i].sleeping_ticks, buf[i].context_switches);
  }
  exit(0);
}
