#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>	
#include <asm/unistd.h> 
#include <sched.h>
#include <errno.h>

#define __NR_getcweight 223
_syscall1(int,getcweight,int, cid);
#define __NR_setcweight  251
_syscall2(int,setcweight,int, cid, int, weight);

#define SCHED_CWRR 4
#define MAX_CHILDREN 10

int pid_in_test = 0;
int child_pid[10];
int child_block_sig = 0;
int child_blocked[3];


void print_result(int res)
{
    if (res < 0)   
        printf( "Error : %s\n", strerror( errno ) );
    else
        fprintf(stdout,"Sucess : %d\n",res);       
}


int child(int ci)
{
	/*sotre my pid so i can be killed */
	child_pid[ci] = getpid();
	char ch;
	/* i can run 3 different tests @ once.  they are BORN! in this order. */
	if (ci<5){
		switch(ci)
		{
			case 0: while(1) 
				{	
					if (child_block_sig)
					{
						fprintf(stdout,"child blocking");
						child_blocked[0] = 1;
						scanf("%c",&ch); 
						fprintf(stdout,"child up");
					}
				}
			case 1:	 while(1) 
				{	
					if (child_block_sig)
					{
						fprintf(stdout,"child blocking");
						child_blocked[1] = 1;
						scanf("%c",&ch); 
						fprintf(stdout,"child up");
					}
					
				}
			case 2:	 while(1) 
				{	
					if (child_block_sig)
					{
						fprintf(stdout,"child blocking");
						child_blocked[2] = 1;
						scanf("%c",&ch); 
						fprintf(stdout,"child up");
					}
					
				}
			case 3:	 while(1) 
				{	
					
					
				}
			case 4:  while(1) 
				{	
					
					
				}
		}	
	}
	/* second set of children */
	else if(ci<10){
		switch(ci)
		{
			case 5:    
			  {
			    struct sched_param param1;  
			    while(1) 
			      {	
				fprintf(stdout,"sched_getscheduler(child 6)\n");
				print_result(sched_getscheduler(child_pid[5]));
				fprintf(stdout,"child 6 here, going to sleep");
				sleep(10);
				fprintf(stdout,"child 6 here, wokeup");	
				fprintf(stdout,"sched_getparam child 6 \n");
				sched_getparam(child_pid[5], &param1);
				print_result(param1.sched_priority);
			      }
			  }
			case 6:	 while(1) 
				{	
					
					
				}
			case 7:	 while(1) 
				{	
					
					
				}
			case 8:	 while(1) 
				{	
					
					
				}
			case 9:  while(1) 
				{	
					
					
				}
		}	
	}
}

fork_children()
{
  int i; int pid;
  for (i=0;i<MAX_CHILDREN;i++)
	{
		pid=fork();
		if (pid == -1) fprintf(stdout,"Child #%d died at birth, sorry.\n",i); 
		else if (pid == 0)  {
		  fprintf(stdout,"Child #%d here!\n",i);
		  child(i); /*congrats, its a child*/
							}
		sleep(1); /*give children some space*/
	}	
}


int main()
{
    int ret,ret2,cid =0,weight =0;
    struct sched_param param;
    param.sched_priority = 3;
    pid_in_test = 0;

	
	/*
    int pid=fork();
    if (pid == -1) fprintf(stdout,"Child died at birth, sorry.\n");
        else if (pid == 0)  {
	  fprintf(stdout,"Child here!\n");
	  pid_in_test = getpid();
	  int i,x,w;
	  unsigned int y=0;
	  while(1)
	    {
	      x=1;
	      for (i=0;i<4000000;i++)
		{
		  x+=1; /*maybe it should do fomething else....
		  if (x == 3435535) w=32+x/5356; 
		}
	      y++;
	      
	      if (y%10 == 0) fprintf(stdout,"%d\n",sched_getscheduler(0));
	      if (y == 4000000) return(x);
	      
	    }
	}
    */
	
	fprintf(stdout,"---------------------------------------------------------------------------\n");
	param.sched_priority = 3;
	fprintf(stdout,"simple tests\n");
	
		fprintf(stdout,"test1: setscheduler with a bad CID\n");
	        print_result(sched_setscheduler(pid_in_test, SCHED_CWRR, &param));    /* pid 0 = me */
	   
	    fprintf(stdout,"test2: getcweight with bad cid\n");	
			print_result(getcweight(7));
		
		fprintf(stdout,"test3: setcweight with bad cid\n");	
			print_result(setcweight(7, 10));
		
	fprintf(stdout,"---------------------------------------------------------------------------\n\n");
	
	
	
	fork_children();
	fprintf(stdout,"parent: 10 children running around... (setscheduler with CID == -1)\n");
   param.sched_priority = -1;
	fprintf(stdout,"parent: adding 6th child to a container: %d \n",ret);
			ret = sched_setscheduler(child_pid[5], SCHED_CWRR, &param);	print_result(ret);
   
   
	fprintf(stdout,"parent: adding first child to container \n");
		
			ret = sched_setscheduler(child_pid[0], SCHED_CWRR, &param);	print_result(ret);
		param.sched_priority = ret;	
	fprintf(stdout,"parent: adding 2nd child to same container: %d \n",ret);
			ret = sched_setscheduler(child_pid[1], SCHED_CWRR, &param);	print_result(ret);
	fprintf(stdout,"parent: adding 3rd child to same container: %d \n",ret);
			ret = sched_setscheduler(child_pid[2], SCHED_CWRR, &param);	print_result(ret);
	
	fprintf(stdout,"parent: adding 4th child to a new container \n");
		param.sched_priority = -1;
			ret2 = sched_setscheduler(child_pid[4], SCHED_CWRR, &param);	print_result(ret2);
		
	fprintf(stdout,"parent: some info\n");
	fprintf(stdout,"sched_getscheduler(first 3 children...)\n");
        print_result(sched_getscheduler(child_pid[0]));
		print_result(sched_getscheduler(child_pid[1]));
		print_result(sched_getscheduler(child_pid[2]));
		
	fprintf(stdout,"sched_getscheduler(4th)\n");
        print_result(sched_getscheduler(child_pid[3]));
	
	fprintf(stdout,"parent: change 1st container's weight to 5\n");
	print_result(setcweight(param.sched_priority, 5));
	fprintf(stdout,"test2: getcweight of that container... should b 5\n");	
			print_result(getcweight(param.sched_priority));
	
	fprintf(stdout,"---------------------------------------------------------------------------\n\n");   
	fprintf(stdout,"parent: now to mess with the scheduler... start killing children see if containers still there\n");
	
	kill(child_pid[3],SIGTERM);
	param.sched_priority = ret2;
	fprintf(stdout,"parent: adding 5th child to maybe empty container\n",ret);
			ret = sched_setscheduler(child_pid[4], SCHED_CWRR, &param);	print_result(ret);
		
	
	fprintf(stdout,"parent: making all first children block.... wait 2 seconds please\n");
	child_block_sig = 1;
	sleep(2);
	if ( (child_blocked[0] + child_blocked[1] +child_blocked[2]) == 3 )
	child_block_sig = 0;
	
	fprintf(stdout,"press keys 3 times.\n");

	fprintf(stdout,"parent: some info\n");
	fprintf(stdout,"sched_getparam(first 3 children...)\n");
	sched_getparam(child_pid[1], &param);
		print_result(param.sched_priority);
		sched_getparam(child_pid[2], &param);
		print_result(param.sched_priority);
		sched_getparam(child_pid[3], &param);
		print_result(param.sched_priority);
	
	
   
	fprintf(stdout,"parent: \n");
   
	

	fprintf(stdout,"i am going to kill all children");
	sleep(3);
	int i;
	for (i=0;i<MAX_CHILDREN-1;i++)
	{
		kill(child_pid[i],SIGTERM);
	}	

	fprintf(stdout,"parent: adding 9th child to maybe empty container\n",ret);
			ret = sched_setscheduler(child_pid[9], SCHED_CWRR, &param);	print_result(ret);
	kill(child_pid[9],SIGTERM);
	/*
    cid = param.sched_priority;
   
    fprintf(stdout,"test5: sched_getparam\n");
        ret = syscall(__NR_getcweight, cid);
        print_result(ret);
        fprintf(stdout,"test5: getcweight\n");
   
    fprintf(stdout,"test6: sched_getparam\n");
        ret = syscall(__NR_setcweight, cid, weight);
        print_result(ret);
        fprintf(stdout,"test6: setcweight\n");


	*/

	return(0);
}
