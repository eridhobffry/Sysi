#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
 
#define WAIT_SEC 70

/*neue Name definieren*/
typedef void (*PV)(void);

/*
 
 
 4  -- SIGILL  -- illegal Instruction
 11 -- SIGSEGV -- egmentation violation
 8  -- SIGFPE  -- floating-point exception
 
 
 */
void signalhandler(int sigid) {
        printf("signalid: %d\n", sigid);
    if (sigid == 4 || sigid == 11 || sigid == 8) {
      exit(1);
    }
}

/*
 
                     Main

 
 --------------       Es wird die Signale 26(SIGVTALRM - setitimer) -- 6(SIGABRT - Abort programm) -- 14 (SIGALRM - real-time timer expired) gefangen    ---------------------
 
 Sonst wenn man das Programm zu beenden bedroht (mit strg+c), bekommt man 2(SIGINT - interrupted programm)
 
 
 
 */
int main(int argc, char* argv[]) {
    volatile int i;
    struct sigaction sigact;
    sigset_t sigmask; /*Signal sets are data objects that let a process keep track of groups of signals. Es wird in sigfillset verwendet */
    struct itimerval timerval;
    struct timeval tval[2]; /*The struct timeval structure represents an elapsed time.*/
    unsigned int sec = WAIT_SEC;
    
    /*char buf[] = {0x66, 0x0F, 0x3A, 0x16};
    volatile int a = 0;
    volatile int b = 1;*/
 
    sigfillset(&sigmask); /*initializes set to full, including all signals.*/
 
    sigact.sa_handler = signalhandler;
    sigact.sa_mask = sigmask;
    sigact.sa_flags = 0;
     
    for (i=1; i<=32; i++) {
        /*
         
         
         int sigaction(int sig, const struct sigaction *restrict act, struct sigaction *restrict oact);
         sigaction -- Analysieren und bearbeiten die Signale
         
         
         */
      sigaction(i, &sigact, NULL);
    }
 
    
    /*
    //((PV)buf)(); // illegal instruction
 
    //((PV)NULL)(); // segmentation fault   
 
    //b /= a; // floating point exception   
     */
    
    /*
     
     tv_sev  -- second
     tv_usec -- microsecond
     
     */
    
    
    tval[0].tv_sec = 0;
    tval[0].tv_usec = 0;
     
    tval[1].tv_sec = 1;
    tval[1].tv_usec = 0;
    
   
    
    /*
     struct timespec it_interval;  ------  Timer interval
     struct timespec it_value;     ------  Initial expiration
     */
     
    timerval.it_interval = tval[0];
    timerval.it_value = tval[1];
    
    /*
     
     int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
     
     -The setitimer() system call sets a timer to the specified value.
     -The setitimer system call is a generalization of the alarm call. It schedules the delivery of a signal at some point in the future after a fixed amount of time has elapsed and interrupted itself
     
     */
     
    if (setitimer(ITIMER_REAL, &timerval, NULL) < 0) {
      perror("setitimer");
      return 1;
    }
    
    
    tval[1].tv_sec = 0;
    tval[1].tv_usec = 1;
     
    timerval.it_interval = tval[0];
    timerval.it_value = tval[1];
     
    if (setitimer(ITIMER_VIRTUAL, &timerval, NULL) < 0) {
      perror("setitimer");
          return 1;
    }
     
    /*tval[1].tv_sec = 0;
    tval[1].tv_usec = 1;
     
    timerval.it_interval = tval[0];
    timerval.it_value = tval[1];
     
    if (setitimer(ITIMER_PROF, &timerval, NULL) < 0) {
      perror("setitimer");
          return 1;
    }*/
     
/*for (i=1; i<=0xFFFFFF; i++);*/
     
    sleep(sec);
     
    abort();
     
    return 0;
}
