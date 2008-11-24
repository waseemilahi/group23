HomeWork # 5 Group # 23 (Working Set Tracking)

			
		The new system call is in the memtrack.c file inside directory memtrack.
	sys_memtrack() uses multiple helper functions to do its job. The functionaly
	implemented is as requested in the assignment. Other than this directory,
	we also change three other files. 
		First off comes the sched.h. All the required data structures and new new 
	entries for task_struct are in this file. We have ker_mmdesc that wraps the mmdesc
	structure which was provided by the assignment, which is also declared in sched.h.
	Two new fields were added to the task_struct. One is is an integer flag that gets set
	when the memtrack() is called for the first time for a process. The other field 
	is the pointer to yet another data structure, also declared inside sched.h, mem_tracker.
	mem_tracker is the "list" for kern_mmdesc. Each kern_mmdesc corresponds to a
	vma structure. 
		Both the above mentioned fields get initialized inside do_fork inside fork.c.
	Last is the pgtable.h file. For our implementation, we are using the unused bits,
	to manipulate the mem_recording. To make this work there are changes made to the 
	routines that use PAGE_ASSESSED flag since we will be monopolizing this flag. 
		Now inside sys_memtrack(), we do take care of all the asked functionalities.
	First off the boundaries are checked to see if correct arguments were passed. Then
	we check if the given pid belongs to a valid process or not. Then we check if is was
	very first memtrack() call after boot up, if it was we initialize a list to hold the 
	data for all the processes. We then check for the flag for process(p). If is was 0,
	we set it and also initialize the mem_tracker sructure for it. And add that to the global
	list that was mentioned just before. This pointer gets deleted when the process exits.
		Now we start the recording process. First we check if it was the repeat from user.
	The user may have come before with not enough len. If that is true we copy the elements
	from previous list to user then return number of bytes transfered. If this is not the case,
	then we actualy populate the list using the function mark_mem(), then we copy is to user space.
	Before this copy we do check if the length is greater than size, it not we return the size to 
	the recorded data that couldn't be copied to the user space.
	
	Test Programs:
		We wrote one test program. The program calls memtrack every 5 seconds and reports 
		the status to the screen like it is specified in the homework assignment.

	Every call to memtrack is double:
	first call is with no arguments, and is intended to get the correct size needed for the buffer.
	then the buffer is allocated and memtrack is called again with the correct size buffer.
	
								THE END

	

