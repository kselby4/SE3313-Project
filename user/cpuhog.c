#include "kernel/types.h"
#include "user/user.h"

//inifinte busy loop process that burns CPU all the time 
int
main(void)
{
  volatile uint64 x = 0;

  while(1){
    for(int i = 0; i < 200000; i++){
      x += (uint64)i;
    }
  }

  exit(0);
}
