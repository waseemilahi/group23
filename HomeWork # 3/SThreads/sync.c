/*
 * NAME, etc.
 *
 * sync.c
 *
 * Synchronization routines for SThread
 */

#define _REENTRANT

#include "sthread.h"
#include <sched.h>
#include <stdlib.h>
#include <errno.h>



/*

return(-1)    	add_me_to_sleepers(sthread_self());
					out of mem or sthred_self == NULL
               which cannot happen according to its documentation.
return(-2)    someone else owns lock  
return(-3)    cannot unlock thread: either not owner or count !=0. 
return(-4)    could not wake sleepers... go figure  
return(-5)    someone is still using the mutex! while calling destry    	
*/
/* We don't want any one else to use these helpers so they are declared here.*/
int add_me_to_sleepers(sthread_t the_thread,sthread_mutex_t *the_mutex);
int wake_all_sleepers(sthread_mutex_t *the_mutex);

/*First function: Init every thing to NULL*/
int sthread_mutex_init(sthread_mutex_t *mutex)
{
    mutex->lock_owner = NULL;
    mutex->lock_count = 0;
    mutex->sleeping_threads_list = NULL;
    mutex->last_sleeping_thread_in_list = NULL;
    mutex->x = 0;

    return(0);    /* success.*/
}

/*Destroy the Mutex created by init*/
int sthread_mutex_destroy(sthread_mutex_t *mutex)
{
    while(test_and_set(&mutex->x) != 0)
    {
        sched_yield(); /* improved busy waiting*/
    }
/* critical section*/

 if (mutex->sleeping_threads_list == NULL)
   {
    /*no one is using the mutex and its freed.*/
    mutex->x=0;
	return(0);  /*success*/
   }
 else
   { /* someone is still using the mutex! return error*/
   mutex->x=0;
	return(-5);
   }
/* end of critical section*/
}

/*Lock the mutex*/
int sthread_mutex_lock(sthread_mutex_t *mutex)
{
int got_lock = 0; /*flag*/
while(!got_lock)
{

    while(test_and_set(&mutex->x) != 0)
    {
        sched_yield(); /* improved busy waiting*/
    }
    /* critical section*/

    if (mutex->lock_count > 0) /* someone has got the lock*/
    {
        /* if we code right, if lock_count > 0,*/
        /* mutex->lock_owner cannot be NULL. so dont check.*/
        if ( mutex->lock_owner == sthread_self() )
        {/* i own the lock, then inc lock, leave.*/
            mutex->lock_count++;

	         got_lock=1; /*no harm in putting it here*/

  		      mutex->x = 0; /* release critical section lock*/
		      return(0); /*success*/
        }
        else /* someone else owns the lock*/
        {
            int result = add_me_to_sleepers(sthread_self(),mutex);

	         if (result < 0) {mutex->x =0;return(-1);}

           	/*release the lock*/
		      mutex->x = 0;
		      sthread_suspend();        /* goto sleep;*/

   	  }/*end else*/
    }
    else /* no one has the lock.*/
    {
        mutex->lock_count = 1;
        mutex->lock_owner = sthread_self();
		  mutex->x = 0; /* release critical section lock*/
	     got_lock=1;
    }

    /* end of critical section*/
}/*end while !(got lock).*/
return(0); /*success. */
}

/*Try the lock. Don't sleep on failurre*/
int sthread_mutex_trylock(sthread_mutex_t *mutex)
{
    /* similar to lock, but not sleeping.*/

    while(test_and_set(&mutex->x) != 0)
    {
        sched_yield(); /* improved busy waiting*/
    }
    /* critical section*/

    if (mutex->lock_count > 0) /* someone has got the lock*/
    {
        /* if we code right, if lock_count > 0*/
        /* mutex->lock_owner cannot be NULL. so dont check.*/
        if ( mutex->lock_owner == sthread_self() )
        {
        /* i own the lock, then inc lock.*/
            mutex->lock_count++;
        }
        else /* someone else owns the lock*/
        {
            mutex->x = 0;/* release crictical section lock. - DONT SLEEP.*/
            return(-2);    /* didnt get the lock :(*/
        }
    }
    else /* no one has the lock.*/
    {
        mutex->lock_count = 1;
        mutex->lock_owner = sthread_self();    /* mark my territory*/
    }

    /* end of critical section*/
    mutex->x = 0; /* release critical section lock*/
    return(0); /*success.*/
}

/*Unlock Mutex*/
int sthread_mutex_unlock(sthread_mutex_t *mutex)
{
    while(test_and_set(&mutex->x) != 0)
    {
        sched_yield(); /* improved busy waiting.*/
    }
    /* critical section*/
    if ( (mutex->lock_count == 0) || (mutex->lock_owner != sthread_self()) )
    /* its not even locked!! OR cannot unlock something you didnt lock. err*/
    {
        mutex->x=0; /*release critical section lock.*/
        return(-3); /*error.*/
    }
    else /* im the owner*/
    {
        /* there is a simple case: i locked the same thread a bunch of times.*/
        if (mutex->lock_count > 1)
            mutex->lock_count--;
        else
            {
                 mutex->lock_count = 0;
                 mutex->lock_owner = NULL;

                /* check if any sleepers*/
                if (mutex->sleeping_threads_list != NULL)
                {
                  /* We can wake threads in FIFO or LIFO*/
                  int result = wake_all_sleepers(mutex);
                  if (result < 0)
                  {
                    /*could not wake sleepers... go figure*/
                    mutex->x = 0;
                    return(-4);
                  }
                }

             }
    }
    /* end of critical section*/
    mutex->x = 0; /* release critical section lock*/
  return(0); /*success.*/
}

/*First Helper function: Adds a thread to the sleeping thread list*/
int add_me_to_sleepers(sthread_t the_thread,sthread_mutex_t *the_mutex)
{
	/* used to demonstrate the fairness. fifo is the way.*/
	int fifo = 1;

    /* this function is protected by a lock obtained by the parent.*/

    /* special case:*/
    if (the_mutex->sleeping_threads_list == NULL)
    {
        /* first element in list.*/
        struct node *tmp = (struct node*) malloc(sizeof(struct node));
	     if (tmp == NULL) /* malloc failed. no mem left.*/
				return(-ENOMEM);

         tmp->sleeping_thread = the_thread;
         tmp->next = NULL;
         the_mutex->sleeping_threads_list = tmp;
	      the_mutex->last_sleeping_thread_in_list = tmp;
    }
    else
	 {
    	if (fifo)
		  {
			/*create a new tail! since we want fifo.*/

			struct node *tmp = (struct node*) malloc(sizeof(struct node));
			if (tmp == NULL) /* malloc failed. no mem left.*/
				return(ENOMEM);

			tmp->sleeping_thread = the_thread;

			/* correct tmp's pointers:*/
			tmp->next = NULL;
			the_mutex->last_sleeping_thread_in_list->next = tmp;
			the_mutex->last_sleeping_thread_in_list = tmp;
			}
    	else	/* LIFO!*/
      	{
			struct node *tmp = (struct node*) malloc(sizeof(struct node));
			if (tmp == NULL)return(ENOMEM);

			tmp->sleeping_thread = the_thread;

			/* correct tmp's pointers:*/
         /* tmp points to first item.*/
			tmp->next = the_mutex->sleeping_threads_list;

         /* tmp now first item.*/
			the_mutex->sleeping_threads_list = tmp;
    		}
		}

	return(0); /*sucess*/
}

/*Wake up all slepers: Order is set by the previous function*/
int wake_all_sleepers(sthread_mutex_t *the_mutex)   /*fifo! */
{
	/* this function is protected by a lock obtained by the parent.*/

	/* wake nodes up. fifo style.*/
	/* this garentees fairness, and eliminates starvation. */
	struct node *ptr;
	struct node *next;
	for(ptr = the_mutex->sleeping_threads_list; ptr != NULL; ptr = next)
		{
			sthread_wake(ptr->sleeping_thread); /* wake the waiting thread*/
			next = ptr->next;	/* make sure we know where to go next: b4 freeing ptr.*/
			free(ptr);    /* clear oldest thread in list*/
		}
   /*NULLify the head and tail pointers and the list is as good as new(empty).*/
   the_mutex->sleeping_threads_list = NULL;
   the_mutex->last_sleeping_thread_in_list = NULL;

   return(0); /*success*/
}
