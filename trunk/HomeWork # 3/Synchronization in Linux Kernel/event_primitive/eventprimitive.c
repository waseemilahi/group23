#include "eventprimitive.h"

/*Declare and initialize the global ID to 1*/
static int global_eventID  = 1 ;

/*This lock keeps the access to event primitive synchronized
  kind of a global lock: imitialize it globally*/
struct rw_semaphore rwsem = __RWSEM_INITIALIZER(rwsem);

/*The "SET" of events*/
struct an_event list_of_events;

/*--------------------------------------------------------------------*/
/*First System Call: Opens an event and returns the eventID on success*/
/*--------------------------------------------------------------------*/
asmlinkage int sys_eventopen(void)
{

  struct an_event *my_event;/*event to add in the "SET"*/

  /*Allocate the space for "TMP" event: if it fails return -1*/
  if((my_event =(struct an_event*) kmalloc(sizeof(struct an_event),GFP_KERNEL)) == NULL)return -ENOMEM;

  /*init the reference count to 0*/
  atomic_set(&my_event->reference_count,0);
  
  /*Flag the event as active*/
  my_event->active = 1;

  /* Allocate space and initialize queue head: return -1 on failure*/
  if((  my_event->wait_q = (wait_queue_head_t *)kmalloc(sizeof(wait_queue_head_t),GFP_KERNEL)) == NULL)
    {
      /* Since previous kmalloc succeeded: we need to free that memory */
      kfree(my_event); 
      return -ENOMEM;
    }

  /*init the wait_queue_head_t*/
  init_waitqueue_head(my_event->wait_q);
  /*DECLARE_WAIT_QUEUE_HEAD(my_event->wait_q);*/


  /*initialize the spin lock for 'this' event*/
  spin_lock_init(&my_event->x);

  /*init the lock used for queue manipulation*/
  init_MUTEX(&my_event->wait_queue_sem);

  /*Now get the writers lock and try to add the created event into the 
    "SET" of the events*/

  down_write(&rwsem);
  /*Set the ID as gloabal ID*/
   my_event->eventID = global_eventID;

   global_eventID++;/* for next event*/
 
   /*If its the first element initialize the head*/
   if(my_event->eventID == 1)
     {
       INIT_LIST_HEAD(&list_of_events.event_list);
     }

   /*Initialize the event list of the event to put in the "SET"*/
   INIT_LIST_HEAD(&my_event->event_list);
  
   /*add the new event in the "SET" of events*/
   list_add(&(my_event->event_list),&(list_of_events.event_list));
   
   /*Release the writers lock on entire "SET"*/
  up_write(&rwsem);

  /*Here is the event created*/  
  return (my_event->eventID);
}/*end syscall*/

/*-----------------------------------------------------------------------*/
/*Second System Call: Closes an event and returns the number of processes*/ 
/*Notified for the event closed: returns -1 on all errors                */
/*-----------------------------------------------------------------------*/
asmlinkage int sys_eventclose(int eventID)
{
  
  /* check for the parameters validity: return error if its out of bounds*/
  if(eventID < 1 || eventID >= global_eventID)return -EINVAL;

  /*pointer to the event that 'may' become the event to close*/
  struct an_event *my_event;

  /*tmp heads to manipulate the list*/
  struct list_head *p,*q;

  /*flag that says whether we found the event or not*/
  int found = 0 ;

  /*this gives the number of processes that were woken up from queue*/
  int count = 0 ;

  /*get the global readers lock: we will not modify anything in this section*/
  down_read(&rwsem);

  /*loop through the event "SET" to find the event to close*/
  list_for_each_safe(p,q,&list_of_events.event_list)
    {
      my_event = list_entry(p,struct an_event,event_list);

      if(my_event->eventID == eventID)/*found it*/
	{
          /*already marked for or is closed: return -1*/
	  if(my_event->active == 0)break;
	  else
	    {
              /*its active now go to next step*/
              found = 1;
	      break;
	    }
	}
    }
  /*no such event with the provided ID*/
  if(!found)
    {
      /*release the global lock and return error*/
      up_read(&rwsem);
      return -1;
    }
  /*release the readers lock any way*/
  up_read(&rwsem);

  /*get the lock to manipulate the fields inside the event we will work on*/
  spin_lock(&my_event->x);
  
  /*in sync check for it being active or not*/
  if(!my_event->active)
    {
      spin_unlock(&my_event->x);/*already marked: release lock*/
      return -1;
    }
  else my_event->active = 0;/*i am going to close the event*/

  spin_unlock(&my_event->x);/*now release the lock*/

  /* Until the reference count is >0 we can't close the event so schedule
     out unless reference count is zero and then continue */
  while((int)atomic_read(&my_event->reference_count) > 0)schedule();
  
  /*there may still be 'readers' in the SET, but it doesn't matter,
    we deallocate after they are done any way.*/

  /*lock the queue of the current event, wake everybody up*/
  down(&my_event->wait_queue_sem);

   while(waitqueue_active(my_event->wait_q))
     { 
       wake_up(my_event->wait_q); /*wake up all of the processes on by one*/
      
       count++;/*count how many*/
     }

   /*unlock the queue*/
  up(&my_event->wait_queue_sem);
  
  /*get the writers lock on the "SET" */
  down_write(&rwsem);

   kfree(my_event->wait_q);/*the queue head was allocated dynamically*/
   list_del(p);
   kfree(my_event); 
  
   /*release the global lock*/
  up_write(&rwsem);

  return count;/*number of processes notified*/

}

/*------------------------------------------------------------*/
/*Third System Call: waits on an event: returns -1 for failure*/
/*------------------------------------------------------------*/
asmlinkage int sys_eventwait(int eventID)
{
  /*check if the parameter is out of bounds: return failure it true*/
  if(eventID < 1 || eventID >= global_eventID)return -EINVAL;
 
  struct an_event *my_event;/*the pointer to the 'current' event*/
  struct list_head *p,*q;/*helpers*/
  int found = 0;/*flag: event exists or not*/

  /*global lock*/
  down_read(&rwsem); 

  /*search for the event asked for by the caller*/
  list_for_each_safe(p,q,&list_of_events.event_list)
    {
      my_event = list_entry(p,struct an_event,event_list);

      if(my_event->eventID == eventID)/*found it*/
	{
	  if(my_event->active == 0)break;/*event already closed*/
	  else
	    { 
	      found = 1;/*flag found*/
	      break;
	    }
	}
    }
  if(!found)/*no such event*/
    {
      up_read(&rwsem);/*release the lock and return error*/
      return -1;
    }

  /* We have the event and we can continue.*/

  /*lock the event*/
  spin_lock(&my_event->x);

  /*again double check with synch*/
   if(!my_event->active)
     {
       spin_unlock(&my_event->x);/*same routine: release the lock*/
       return -1;
     }

      /*i am in: increment the counter to stop event close*/
      atomic_inc(&my_event->reference_count);
  
  spin_unlock(&my_event->x);/*release the lock*/

  down(&my_event->wait_queue_sem);/*lock the queue*/

   DEFINE_WAIT(one_waitq_item); /*static allocation: No need for kfree later*/
	  
    /* adds the element and sets its state */
   prepare_to_wait_exclusive(my_event->wait_q,&one_waitq_item,TASK_INTERRUPTIBLE);

   up(&my_event->wait_queue_sem);/*release the queue lock*/

   /*i am out decrement my counter*/
  spin_lock(&my_event->x);

   atomic_dec(&my_event->reference_count);

  spin_unlock(&my_event->x);

  schedule();/*schedule out: since  i am waiting for sig*/
   
   /* To make current process gets off the queue */
  finish_wait(my_event->wait_q,&one_waitq_item);/*i am done: take me off*/
   
  up_read(&rwsem);/*release the global lock*/
  
   return 0;/*success*/
}

/*------------------------------------------------------------*/
/*Fourth System Call: signals the event and returns the number*/ 
/*of processes notified: returns -1 on failure                */
/*------------------------------------------------------------*/
asmlinkage int sys_eventsig(int eventID)
{
  /*boundary check for the parameter */ 
  if(eventID < 1 || eventID >= global_eventID)return -EINVAL;

  struct an_event *my_event;/*pointer for current event*/
  struct list_head *p,*q;/*helpers*/
  int found = 0;/*flag*/
  int count = 0;/*number of process notified*/

  down_read(&rwsem);/*grab the lock for reading*/

  /*search for the event*/
  list_for_each_safe(p,q,&list_of_events.event_list)
    {
      my_event = list_entry(p,struct an_event,event_list);
      if(my_event->eventID == eventID)/*found it*/
	{

	  if(my_event->active == 0)break;/*never mind*/
	  else
	    {
	      found = 1;/*flag it*/
	      break;
	    }
	}
    }
  if(!found)/*return failure*/
    {
      up_read(&rwsem);/*release lock before you leave*/
      return -1;
    }

  /* lock the fields and check*/
 spin_lock(&my_event->x);

  if(!my_event->active)
    {
      spin_unlock(&my_event->x);
      up_read(&rwsem);/*need to release both locks before return*/
      return -1;
    }
   /*ok i am in mow*/
   atomic_inc(&my_event->reference_count);
  
  spin_unlock(&my_event->x);

  down(&my_event->wait_queue_sem);/* lock the queue*/
    
   while(waitqueue_active (my_event->wait_q))
     {
       /* we sent processes to sleep with the Exclussive flag set
	  so when we call wakeup, only 1 will be woken and we can
	  count how many processes were actually still sleeping */
       wake_up(my_event->wait_q);       
     
       count++;
     }

   up(&my_event->wait_queue_sem);/*release the lock*/
  
   /*evnt:i am out*/
  spin_lock(&my_event->x);
    atomic_dec(&my_event->reference_count);
  spin_unlock(&my_event->x);

  up_read(&rwsem);/*release the read lock*/

  return count;/*number of processes notified*/

}
