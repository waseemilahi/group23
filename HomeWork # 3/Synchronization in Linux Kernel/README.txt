  		HomeWork # 3 Group # 23 (Kernel Synchronization Primitive)

	

	Four new system calls were added to the kernel after applying 
	KDB patch to it.
	
	The system calls were placed inside a single file inside a 
	separate directory. This directory was placed in the source
	tree inside directory linux-2.6.11.12-hmwk3.

	The names for the two files are as :

	event_primitive/eventprimitive.h
	event_primitive/eventprimitive.c


	Aside from these files. Appropriate changes were made in 

	asm-i386/unistd.h
	include/linux/syscalls.h
	arch/i386/kernel/entry.S

Little Explaination:

eventprimitive.h
		
		This file holds all the include files required.
		It also declares the struct used as event primitive.

eventprimitive.c

		This file includes the eventprimitive.h file. Then it
		declares the required global variables.

		i)  global event ID.
		ii) read write semaphore.
		iii)struct an_event (variable for 'SET' of events)		

		Then it implements the four system call routines. The code
		itself is very well documented. The four routines do 
		exactly what was asked in the requirement. Methods used to
		achieve that goal may differ from some other methods.
		In the end every thing seems to be quite efficent. 
		We do use different synchronization primitives
		various levels. The reference count and the active(flag)
		are important pieces. Reference count stops the eventclose
		from 'destroying' the event until there is no one trying
		to gett on wait queue. The active 'flag' makes sure only
		one eventclose operates on any event.

Test Files:

		There are two test files. One does basic testing. It uses
		three events and six processes to do wait, sig and close,
		while testing all the conditions asked for.
		The other does a bit more extensive checking with
		multithreaded operations.It checks for dummy eventID's
		in the system calls. FIrst one covers the behaviour of 
		sig and wait, but the second one checks the wait close
		relationship closely. Both tests are included in the 
		submission.