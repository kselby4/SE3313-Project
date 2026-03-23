#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// ==== CONFIGURATION ====
// INCREASING_BURSTS: Controls which test case to run
//   1 = Case 1: child1 has shortest burst, child5 has longest (short job runs first)
//   0 = Case 2: child1 has longest burst, child5 has shortest (long job runs first)
#define INCREASING_BURSTS 0

// BURST_MULTIPLIER: Makes bursts longer so waiting_tick values are higher
// Increase this number if your waiting_tick values are too small
#define BURST_MULTIPLIER 30

// Simulates CPU work by busy-waiting for a certain number of ticks
// ticks_to_burn = how many timer ticks to wait (1 tick is about 0.1 seconds)
void cpu_burst(int ticks_to_burn) {
    int start = uptime();                         // record start time
    while (uptime() - start < ticks_to_burn) {    // keep looping until time passes
        // do nothing - just burn CPU time
    }
}

// Each child calls this function to simulate doing work
// child_id = which child this is (1 through 5)
void child_process(int child_id) {
    int burst_time;
    
    #if INCREASING_BURSTS
        // Case 1: child1=30 ticks, child2=60, child3=90, child4=120, child5=150
        // Short jobs run first because child1 (smallest PID) has shortest burst
        burst_time = child_id * BURST_MULTIPLIER;
    #else
        // Case 2: child1=150 ticks, child2=120, child3=90, child4=60, child5=30
        // Long job runs first because child1 (smallest PID) has longest burst
        burst_time = (6 - child_id) * BURST_MULTIPLIER;
    #endif
    
    cpu_burst(burst_time);  // simulate doing work for burst_time ticks
}

int main(void) {
    int i;
    
    // Print which test case we're running
    #if INCREASING_BURSTS
        printf("Running Case 1: INCREASING bursts (child1=shortest, child5=longest)\n");
    #else
        printf("Running Case 2: DECREASING bursts (child1=longest, child5=shortest)\n");
    #endif
    
    // Create 5 child processes
    for (i = 0; i < 5; i++) {
        int pid = fork();  // fork() creates a copy of this process
        
        if (pid < 0) {
            printf("Fork failed for child %d\n", i);
            exit(1);
        } 
        else if (pid == 0) {
            // CHILD: fork() returns 0 to the child process
            child_process(i + 1);  // child does its work
            exit(0);               // child exits when done
        } 
        else {
            // PARENT: fork() returns child's PID to the parent
            printf("Parent: Forked child %d with PID %d\n", i + 1, pid);
        }
    }
    
    // Parent waits for all 5 children to finish
    // When each child exits, the kernel prints its schedstats
    for (i = 0; i < 5; i++) {
        wait(0);
    }
    
    printf("All children completed.\n");
    exit(0);
}
