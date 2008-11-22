#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include <sched.h>
#include "/usr/src/linux-2.6.11.12.hmwk3/include/asm/unistd.h"
#include "/usr/include/errno.h"

#define MAX_CHILDREN 15
#define __NR_eventopen  289
_syscall0(int ,eventopen)

#define __NR_eventclose 290
_syscall1(int,eventclose,int, eventID)

#define __NR_eventwait 291
_syscall1(int, eventwait,int, eventIID)

#define __NR_eventsig 292
_syscall1(int, eventsig,int,eventID)

static int event[3];
static int returned[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void wait_for_child(void);
int child(int ci);
void sig_handle(int sig);


int child(int ci)
{

/* I can run 3 different tests @ once.  they are BORN! in this order. */
  if (ci<5){
    switch(ci)
      {
      case 0: sleep(1); fprintf(stdout,"Child 0 Returned: %d on wait\n",eventwait(event[0])); exit(1);
      case 1: sleep(1);	fprintf(stdout,"Child 1 Returned: %d on wait\n",eventwait(event[0])); exit(2);
      case 2: sleep(1);fprintf(stdout,"Child 2 Returned: %d on wait \n",eventwait(event[0]));  exit(3);
      case 3: sleep(1);	fprintf(stdout,"Child 3 Returned: %d on wait\n",eventwait(event[0])); exit(4);
      case 4: sleep(1); fprintf(stdout,"Child 4 Returned: %d on close \n",eventclose(event[0])); exit(5);
      }	
  }
  /* second set of children */
  else if(ci<10){
    switch(ci)
      {
	/* let the scheduler decide who fights who */
      case 5: sleep(1); fprintf(stdout,"Child: 5 Returned: %d on wait\n",eventwait(event[1])); exit(5);
      case 8:	sleep(1); fprintf(stdout,"Child: 8 Returned: %d on wait\n",eventwait(event[1])); exit(8);
      case 7:	sleep(1);fprintf(stdout,"Child: 7 Returned: %d on wait \n",eventwait(event[1]));  exit(7);
      case 6:	sleep(1); fprintf(stdout,"Child: 6 Returned: %d on wait\n",eventwait(event[1]));exit(6);
      case 9: sleep(1); fprintf(stdout,"Child: 9, closed an event with four waiting Returned: %d\n",eventclose(event[1])); exit(9);
      }	
  }
  /* third set of children */
  else{
    switch(ci)
      {
      case 10:  sleep(1);fprintf(stdout,"Child: 10 Returned: %d on dummy wait\n",eventwait(event[2]));  exit(10);
      case 11:  sleep(1);fprintf(stdout,"Child: 11 Returned: %d on dummy sig\n",eventsig(event[2]));    exit(11);
      case 12:  sleep(1);fprintf(stdout,"Child: 12 Returned: %d on dummy close\n",eventclose(event[2])); exit(12);
      case 13:  sleep(1);event[2] = eventopen(); fprintf(stdout,"Child: 13 Returned: %d on open\n",event[2]);  	; exit(13);
      case 14:  sleep(1);fprintf(stdout,"Child: 14 Returned: %d on close of event %d\n",eventclose(event[1]+1),event[1]+1);   		 exit(14);
      }	
  }
  return 0;
  
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! we must add this to all even calls, no need to check anything if id is not valid !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
/* if(eventID < 1 || eventID > global_eventID)return -EINVAL;*/
int main()
{
  int i,rv;
 
 pid_t pid;
 struct sigaction timer_sig;
 sigset_t sigset;

 sigemptyset(&sigset);
 timer_sig.sa_mask = sigset;
 timer_sig.sa_handler = sig_handle;
 timer_sig.sa_flags = 0;
 sigaction(SIGALRM,&timer_sig,NULL);
 
 event[0] = 	eventopen();
 event[1] =      eventopen();
 event[2] = 	0;
 fprintf(stdout,"parent: I have created events: %d and %d\n",event[0],event[1]);
 
 for (i=0;i<MAX_CHILDREN;i++)
   {
     pid=fork();
     if (pid == -1) fprintf(stdout,"Child #%d died at birth, sorry.\n",i); 
     else if (pid == 0)  {
    
       child(i); /*congrats, its a child*/
     }
     /*give children some space*/
     sleep(1);
   }	
 
 /* we now have MAX_CHILDREN out there. they are labled for our convinience from 0 - max... */
 
 /*make sure no chile left behind! - as wait can get stuck.*/
 for (i=0;i<MAX_CHILDREN;i++)
   {     
     wait(&rv); returned[WEXITSTATUS(rv)] = 1;
   }    
  
 return 0;
 
}

void sig_handle(int sig){}
