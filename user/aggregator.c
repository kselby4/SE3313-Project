#include "kernel/types.h"
#include "user/user.h"

#define NUM_ITER 8

struct sstat {
  int count;
  int latest;
  int min;
  int max;
  int sum;
};

static void
st_init(struct sstat *s)
{
  s->count = 0;
  s->latest = 0;
  s->min = 0;
  s->max = 0;
  s->sum = 0;
}

static void
st_add(struct sstat *s, int v)
{
  if(s->count == 0){
    s->min = v;
    s->max = v;
  } else {
    if(v < s->min)
      s->min = v;
    if(v > s->max)
      s->max = v;
  }
  s->latest = v;
  s->sum += v;
  s->count++;
}

static int
readline(int fd, char *buf, int n)
{
  int i;
  char c;

  for(i = 0; i < n - 1; i++){
    if(read(fd, &c, 1) != 1){
      buf[i] = '\0';
      return i;
    }
    buf[i] = c;
    if(c == '\n'){
      i++;
      break;
    }
  }
  buf[i] = '\0';
  return i;
}

static int
parse_line(char *buf, struct sstat *st_temp, struct sstat *st_air, struct sstat *st_power)
{
  int v;

  if(strlen(buf) >= 5 && memcmp(buf, "TEMP ", 5) == 0){
    v = atoi(buf + 5);
    st_add(st_temp, v);
    return 0;
  }
  if(strlen(buf) >= 4 && memcmp(buf, "AIR ", 4) == 0){
    v = atoi(buf + 4);
    st_add(st_air, v);
    return 0;
  }
  if(strlen(buf) >= 6 && memcmp(buf, "POWER ", 6) == 0){
    v = atoi(buf + 6);
    st_add(st_power, v);
    return 0;
  }
  return -1;
}

static void
print_row(char *label, struct sstat *s)
{
  int avg;

  if(s->count == 0){
    printf("%s   count=0\n", label);
    return;
  }
  avg = s->sum / s->count;
  printf("%s   count=%d latest=%d min=%d max=%d avg=%d\n",
         label, s->count, s->latest, s->min, s->max, avg);
}

static void
print_env_summary(struct sstat *t, struct sstat *a, struct sstat *p)
{
  int trange, aavg, pavg;

  printf("\nEnvironment summary:\n");

  if(t->count > 0){
    trange = t->max - t->min;
    printf("- temperature %s\n",
           trange <= 4 ? "stable" : "variable");
  } else {
    printf("- temperature (no data)\n");
  }

  if(a->count > 0){
    aavg = a->sum / a->count;
    if(aavg < 52)
      printf("- air quality low\n");
    else if(aavg < 68)
      printf("- air quality moderate\n");
    else
      printf("- air quality elevated\n");
  } else {
    printf("- air quality (no data)\n");
  }

  if(p->count > 0){
    pavg = p->sum / p->count;
    if(pavg < 200)
      printf("- power usage low\n");
    else if(pavg < 282)
      printf("- power usage moderate\n");
    else
      printf("- power usage elevated\n");
  } else {
    printf("- power usage (no data)\n");
  }
}

int
main(void)
{
  int pt[2], pa[2], pp[2];
  char buf[64];
  int r, round, i;
  struct sstat st_temp, st_air, st_power;

  st_init(&st_temp);
  st_init(&st_air);
  st_init(&st_power);

  if(pipe(pt) < 0 || pipe(pa) < 0 || pipe(pp) < 0){
    printf("aggregator: pipe failed\n");
    exit(1);
  }

  if(fork() == 0){
    close(pt[0]);
    close(pa[0]);
    close(pa[1]);
    close(pp[0]);
    close(pp[1]);
    close(1);
    if(dup(pt[1]) != 1){
      printf("aggregator: dup failed (temp)\n");
      exit(1);
    }
    close(pt[1]);
    char *argv[] = { "tempsensor", 0 };
    exec("tempsensor", argv);
    printf("aggregator: exec tempsensor failed\n");
    exit(1);
  }

  if(fork() == 0){
    close(pt[0]);
    close(pt[1]);
    close(pa[0]);
    close(pp[0]);
    close(pp[1]);
    close(1);
    if(dup(pa[1]) != 1){
      printf("aggregator: dup failed (air)\n");
      exit(1);
    }
    close(pa[1]);
    char *argv[] = { "airsensor", 0 };
    exec("airsensor", argv);
    printf("aggregator: exec airsensor failed\n");
    exit(1);
  }

  if(fork() == 0){
    close(pt[0]);
    close(pt[1]);
    close(pa[0]);
    close(pa[1]);
    close(pp[0]);
    close(1);
    if(dup(pp[1]) != 1){
      printf("aggregator: dup failed (power)\n");
      exit(1);
    }
    close(pp[1]);
    char *argv[] = { "powersensor", 0 };
    exec("powersensor", argv);
    printf("aggregator: exec powersensor failed\n");
    exit(1);
  }

  close(pt[1]);
  close(pa[1]);
  close(pp[1]);

  for(round = 0; round < NUM_ITER; round++){
    int fds[3] = { pt[0], pa[0], pp[0] };
    for(i = 0; i < 3; i++){
      r = readline(fds[i], buf, (int)sizeof(buf));
      if(r == 0){
        printf("aggregator: unexpected EOF on sensor stream\n");
        exit(1);
      }
      if(parse_line(buf, &st_temp, &st_air, &st_power) < 0)
        printf("aggregator: bad line: %s", buf);
    }
  }

  close(pt[0]);
  close(pa[0]);
  close(pp[0]);

  for(i = 0; i < 3; i++){
    if(wait((int *)0) < 0){
      printf("aggregator: wait failed\n");
      exit(1);
    }
  }

  printf("\n----- Sensor Summary -----\n");
  print_row("TEMP ", &st_temp);
  print_row("AIR  ", &st_air);
  print_row("POWER", &st_power);
  print_env_summary(&st_temp, &st_air, &st_power);

  exit(0);
}
