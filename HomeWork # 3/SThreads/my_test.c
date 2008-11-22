#define _REENTRANT
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sthread.h"
#include <sched.h>

sthread_mutex_t mutex;
int active_thread_count;

int threadmain(void *arg)
{
  sthread_mutex_lock(&mutex);
    active_thread_count++;
  sthread_mutex_unlock(&mutex);

  int threadno = (int)arg;
  int i,j;

  for (i=0;i<8;i++)
  	{
	    printf("\tthread %d:attempting to obtain lock\n", threadno);
   	 j = sthread_mutex_lock(&mutex);
		 printf("tthread %d: got the lock! Return value: %d\n", threadno,j);
	    sleep(1);
		 printf("thread %d: preparing to release the lock...\n", threadno);
	    j = sthread_mutex_unlock(&mutex);
		 printf("\tthread %d:lock released! Return value: %d\n", threadno,j);

	    /* simulate other instructions that need to be carried out.*/
	    sleep(1);

  	}

  sthread_mutex_lock(&mutex);
  	 active_thread_count--;
  sthread_mutex_unlock(&mutex);

   return 0;
}

int main(int argc, char *argv[])
{
  sthread_t thr1, thr2, thr3;
  active_thread_count = 0;

  sthread_mutex_init(&mutex);

  if (sthread_init() == -1)
    fprintf(stderr, "%s: sthread_init: %s\n", argv[0], strerror(errno));

  if (sthread_create(&thr1, threadmain, (void *)1) == -1)
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));

  if (sthread_create(&thr2, threadmain, (void *)2) == -1)
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));

  if (sthread_create(&thr3, threadmain, (void *)3) == -1)
    fprintf(stderr, "%s: sthread_create: %s\n", argv[0], strerror(errno));


/* i dont want main to exit untill my threads are done playing.*/
/* active thread_count is atomically updated by each thread*/
/* when its born(++), and when it dies(--)*/

	while (active_thread_count > 0)sleep(1);

	if	(sthread_mutex_destroy(&mutex) !=0) fprintf(stdout,"exiting and stoping active threads.\n");
	else 	fprintf(stdout,"exiting quietly\n");
	sleep(1);
   return 0;
}
