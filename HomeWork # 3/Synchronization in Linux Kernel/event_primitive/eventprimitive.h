#ifndef __EVENTPRIMITIVE_H
#define __EVENTPRIMITIVE_H

#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <asm/semaphore.h>

/* The event primitive */

struct an_event{

  int eventID;/* Well: the ID of cource*/

  spinlock_t x; /*To lock fields inside the event primitive*/

  wait_queue_head_t *wait_q;/*Marks the beginning of the wait queue*/

  struct semaphore wait_queue_sem;/*Lock for the wait queue modification*/

  struct list_head event_list;/*To keek the set of events*/

  int active;/*whether the current event has been flagged to close or not*/

  atomic_t reference_count;/*The powerfull one :)*/

};

#endif
