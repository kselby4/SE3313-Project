#include "kernel/types.h"
#include "user/user.h"

//process that spends most time sleeping 
int
main(void)
{
  while(1){
    //Simulate a low-duty-cycle sensor task that mostly sleeps.
    pause(5);
  }

  exit(0);
}
