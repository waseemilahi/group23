/*
 * NAME, etc.
 *
 * sync.h
 */

#ifndef _STHREAD_SYNC_H_
#define _STHREAD_SYNC_H_

#include <stdio.h>
#include <unistd.h>

struct node{
struct node* next;
sthread_t sleeping_thread;
};

struct sthread_mutex_struct{

/*used in two occasions: multiple locks & unauthorized unlocks.*/
sthread_t lock_owner;

/*for recursive locking, we need a count rather than a flag (locked/unlocked)*/
int lock_count;

/* instead of busy waiting, send waiting threads to bed.*/
/* keep track of who's sleeping with this list*/
struct node *sleeping_threads_list;
struct node *last_sleeping_thread_in_list;

/* will be used in all test and set calls. declared here as  global varible.*/
volatile unsigned long x;

};

typedef struct sthread_mutex_struct sthread_mutex_t;

/*function declarations*/
int sthread_mutex_init(sthread_mutex_t *mutex);
int sthread_mutex_destroy(sthread_mutex_t *mutex);
int sthread_mutex_lock(sthread_mutex_t *mutex);
int sthread_mutex_trylock(sthread_mutex_t *mutex);
int sthread_mutex_unlock(sthread_mutex_t *mutex);

#endif
