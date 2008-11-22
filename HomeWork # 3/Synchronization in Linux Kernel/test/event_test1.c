#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sched.h>
#include <signal.h>
#include "/usr/src/linux-2.6.11.12.hmwk3/include/asm/unistd.h"
#include "/usr/include/errno.h"

#define MAX_CHILDREN 5

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
 
  switch(ci)
    {
    case 0: fprintf(stdout,"Child: 1 Returned: %d on wait for event # 2\n",eventwait(event[1])); sleep(1); exit(0);
    case 1: fprintf(stdout,"Child: 2 Returned: %d on wait for event # 2\n",eventwait(event[1])); sleep(1);exit(1);
    case 2: fprintf(stdout,"Child: 3 Returned: %d on wait for event # 3\n",eventwait(event[2]));sleep(1); exit(2);
    case 3: fprintf(stdout,"Child: 4 Returned: %d on sig for event # 2\n",(eventsig(event[1]))); sleep(1);exit(3);
    case 4: fprintf(stdout,"Child: 5 Returned: %d on wait for event # 3 \n",eventwait(event[2]));sleep(1); exit(4);

    }

  return 0;  
}

int main()
{
 
 int i,rv;
 pid_t pid;
 struct sigaction sigact;
 sigset_t sigset;

 sigemptyset(&sigset);
 sigact.sa_handler = sig_handle;
 sigact.sa_mask = sigset;
 sigact.sa_flags = 0;
 sigaction(SIGALRM,&sigact,NULL);

 /*open three events*/
 event[0] = eventopen();
 event[1] = eventopen();
 event[2] = eventopen();

 fprintf(stdout,"parent: I have created events: %d and %d and %d\n",event[0],event[1],event[2]);
	
 for (i=0;i<MAX_CHILDREN;i++)
   {
     pid=fork();
     if (pid == -1)
       fprintf(stdout,"Child #%d died at birth, sorry.\n",i);
     else if (pid == 0)
       {
	 fprintf(stdout,"Child #%d here!\n",i+1);
	 child(i); /*congrats, its a child*/
       }
     sleep(1); /*give children some space*/
   }	
 /* we now have MAX_CHILDREN out there. they are labled from 0 - max*/
		
 alarm(6);

 int kk =eventclose(event[2]);fprintf(stdout,"\nParent closed event # 3: it returned : %d \n\n",kk);

 for (i=0;i<MAX_CHILDREN;i++)
   {

     wait(&rv); returned[WEXITSTATUS(rv)] = 1;
     fprintf(stdout,"parent here to tell you child %d succeded\n",WEXITSTATUS(rv)+1);
   }
 fprintf(stdout,"\nevent sig: WHEN NO ONE IS WAITING: %d\n",eventsig(event[0]));
 fprintf(stdout,"parent: loop done, child returned. closing the remaining events\n");
 if(eventclose(event[1])>-1)
   fprintf(stdout,"\nClose event # 2\n");
 if(eventclose(event[0])>-1)
   fprintf(stdout,"Close event # 1\n");

 return 0;

}

void sig_handle(int sig){}
