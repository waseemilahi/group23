HomeWork # 4 Group # 23 (Kernel Synchronization Primitive)

	
All of the code for this assignment is in sched.c with few fields
added to task_struct in sched.h as well.

In sched.c a new struct weighted_container was declared. 
this struct contains the contianer's run queue and additional information 
such as
reference counters, CID, weight and so on. 
the container can assume two states: active and sleeping. 
a sleeping container is defined as a container that has no active tasks 
associated with it but is not empty (ex: one of its task is blocking)


The prio_array_t and runqueue_t data structures were hacked.
Anchors to the container active and sleeping queues were added in
runqueue. in addition the number of sleeping and active containers are also
maintained the the queue structure. 

the prio_array_t data structure was modified to include the 
5 pointers. 
3 pointers are used to reference the lists of the container active and
sleeping queues in addition to the first container's run queue. 
both active and expired runqueus were updates to point to these queues.

the additional two pointers are used to reference the 
active/sleeping containers counters. 

The values referenced by these pointers were initialized inside sched_init 
function.

the task_struct structure was modifieid to include a the pointer and CID or 
its parent structure.

Two new requested system calls were implemented with their code
at the bottom of the file. The do all the requested work and
their system routines are named sys_getcweight() and sys_setceight(); 

List of other functions that were edited:
	
	enqueue_task();
	dequeue_task();
	requeue_task();

	__activate_task();
	deactivate_task();

	scheduler_tick();
	schedule();
	sched_fork();
	sched_exit();
	do_sched_setscheduler();
	sched_setscheduler();
	sched_getparam();
	
New functions added to manipulate containers:

	enqueue_container();
	dequeue_container();
	requeue_container();
	enqueue_container_head();

The schedule works along side the rest of the schedulers, maintaining
the defined hierarchy.

TEST FILE:

      	We try to test the functionality of the scheduler 
we implemented. adding containers and manipulating the state of its runque 
elements. 
Making sure the system works smoothly and the priorities are in order.

			THE END

	

