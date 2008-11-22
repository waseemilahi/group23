            HomeWork # 2 Group # 23 (Group Assignment)

	Two new system calls were added to the kernel after applying 
	KDB patch to it.
	
	The system calls were placed inside their separate directories.
	These directories were places in the source tree inside 
	directory linux-2.6.11.12-hmwk2.

	the respective names for each are :

	getrecording/ containing getrecording.h and getrecording.c
	setrecording/ containing setrecording.h and setrecording.c

	other than that the two requested files were also created
	and the approprite material was put in them.

	Aside from these files. Appropriate changes were made in 
	asm-i386/unistd.h
	include/linux/sched.h
	kernel/fork.c
	arch/i386/kernel/entry.S

	and some other files to accomodate and install new system calls.

Little Explaination:

Fork.c

	when free_task is called, we added a call to
	void exit_reclog(struct task_struct *tsk)
	which frees the memory taken by our rec list, which is defined in the
	task_struct. exit_reclog is also defined inside fork.c

	when fork is called, it duplicates a process' task_struct. in order not 
        to duplicate recorded information, we cleared all the fields of the recorded 
        structre in do_fork. We also use the added counters in INIT_TASK macro. 

getrecording.h
getrecording.c

	these files implements the sys_getrecording routine. 
	get recording accessess the curruent thread's task_struct and
	retrieves for the user the defined number of entries requested. 
	the information is copied to the userspace into a preallocated array of 
        structures.the routine deletes the entries it retrieves. 
	works correctly.


recinfo.h
	
	this file hold the reclog structure for the user.

sched.h

	added fields to the task struct. a list head and some control 
variables. structure kern_reclog defined in it.


setrecording.h
setrecording.c
	
	these files implement the sys_setrecording routine.
	set recording set a flag in the task_struct to enable recording
	and adjusts the amount to entries to store. if the amount is specified 
        by the user in a consecutive call is smaller than the previous amount, the 
        function will delete records untill the max recorded amount specified by 
        the user is reached. Also the user can't specify more than 4000 entries
	per process.
	
	
sysrecord.c

	this function is called when any system call gets executed on the 
        system.	if the current process' record flag is set, it will append the 
        information recorded to the process' reclist. a list of recorded 
        syscall and thier arguments. This is where the entire list gets created.


test-record.c

	this program tests the system calls written. we tested many 
        combinations, and end cases.enabling recording multiple time with
        smaller max argument. calling get recording multiple times to 
        reduce the list size and called both with bad arguments.


init_task.h: include/linux/

		 new fields entered in task table

