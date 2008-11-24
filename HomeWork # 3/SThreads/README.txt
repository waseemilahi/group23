	HomeWork # 3 Group # 23 (SThreads)


	In this half of the assignment. The approprite 
	code was added to the skeleton given. However some 
	additional helper routines were written as well.

Little Explaination:

	Again, the files themselves have good documentation to 
	explain what's going on in them.

sync.h

	We declare two new structs in this file.
	One is struct node, basically, for sleeping thread list.
	The second one is the structure, which is our primitive
	basically. It contains two pointers to the list of 
	sleeping threads and some other helper variables.

sync.c

	Here we fill up the skeletons provided, and also define
	two helper functions, that are used by the provided functions.
	One adds a thread to the sleepers list and the other wakes up
	all the sleepers. There can be two ways in which it can
	be done. FIFO and LIFO.We chose FIFO, for it is a fair way
	of assigning slots and implemented LIFO to test/prove that
	lifo is not fair (choice between the two is hardcoded). Of 
	course since we are implementing synchronization primitive,
	we have to be carefull about locks.
	There are comments through out the code that explain the 
	usage of various techniques.

Makefile

	make         : makes the library
	make my_test : makes the test program
	

my_test.c

	Our code should work on any test program. we used an extensive 
	test code to achieve synchronization among 3 threads. 

	we used the test program provided to us and modified it in the
	following way: three threads are created then try to obtain
	locks to reach a printf statement. each thread then releases
	the lock and waits, then tries again. we used sleep since a
	thread that ir runing will obtain a lock, print release the
	lock and still have time to run. so it will repeast this until
	it is scheduled out. so, we used wait, to make the thread
	schedule out.
	
	In additon, we only had a single processor machine. so threads
	were scheduled one after the other, so the synchronization we
	could provide was mutex over the printf section (critical) and
	fairness as far as waking up sleeping threads. all these actions
	were done using our primitives to provide synchronization.	
	The output is printed to the standard out. 



------------------------------------------------------------------------

Assumptions:

	We assume the calling threads, will handle the returned values 
	and take care of any other conditions. We are implementing the
	lock primitives. how a user uses them is up to the user and can
	cause problems on user end, but it has nothing to do with our
	implementation.	

	If you call destroy and there are threads waiting, return error. 
	when calling wakeup consecutively, we assume threads are 
	scheduled in the order in which they were woken up. 
	This is not clear from the thread code. (to us)

Difference from assignment:

	I guess its safe to say that we do not stray from what was
	required. We do implement helpers. To facilitate the lock and 
	unlock functions.