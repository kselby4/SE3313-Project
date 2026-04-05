// Shared user/kernel layout for getecostats(2).

struct ecostat {
  int pid;
  int state;
  char name[16];
  int running_ticks;
  int runnable_ticks;
  int sleeping_ticks;
  int context_switches;
  int eco_score;
};
