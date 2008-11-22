/*
currently missing: 
1	usage of locks for the mmtrack struct.
	also, the initialization of the locks.

2	a global list head for all memtrack elements. 

3	copy to user, not done yet. need to decide on a method.


	------------------------------------------------
	switch_clear_current_data_and_start_recording_new_info; //switch pointers
	------------------------------------------------
	we need this: at the begining of recording and end of recording... 
	split to: 
	
	start_recording()
	end_recording()
	switch_recorded_data()  // ATOMIC! 

	
	sort_recorded_data_if_needed; uncertain if needed. 
	
	convert_information_to_string; &&copy_it_to_user; -> only mising a bit
	free_old_data; -> taken care of in the copy to user.


*/


int memtrack(pid_t pid, struct mmdesc __user *mm, int len);
{
	int byte_size_of_collected_data =0; /* in bytes! must include last structure too */
	struct task_struct *p;
	mem_tracker *current_mem_struct
	struct mem_tracker *mem_tracker_ptr;
	int mem_tracker_ptr_used = 0;
		
	mem_tracker_ptr =  kmalloc( sizeof(struct mem_tracker), GFP_KERNEL );	/* malloced here to avoid mallocation under a lock */
	if (!mem_tracker_ptr)	/* malloc faild! */
		goto err_nomem;		/* err reporting */
		
	/* check trivial stuff here */
	if ((len<0) || (pid<0)) goto err_bad_argument;
	
	read_lock_irq(&tasklist_lock); /* get a lock to find our task_struct */
	
	p = find_process_by_pid(pid); /* try to get the process! */
 	if (!p) /* the process doesnt exist */	
		{
			free(mem_tracker_ptr);				/* allocated but never used */
			read_unlock_irq(&tasklist_lock);	/* release lock */
			goto err_pid_not_found; 			/* err reporting */
		}
		
	if(!p->memtracker)
		{
			/* this is the first call to the never ending?! function.... */
			
			p->memtracker = 1;	/* set a recording flag so we dont end up in this if again */ 
						 
			p->ptr_to_my_mem_struct = mem_tracker_ptr; /* update the ptr in the task struct to point to the memory recording struct. */
			
			/* initialize the structure */
			LIST_HEAD_INIT(mem_tracker_ptr->current_recording);
			LIST_HEAD_INIT(mem_tracker_ptr->previous_recording);
			mem_tracker_ptr->previous_recording_size = 0;
			mem_tracker_ptr->current_recording_size  = 0;	
			
			mem_tracker_ptr_used = 1; /* this is a flag that we used the allocated structure and we should not free it */
			
			/***************************************/
			/* IF WE HAVE A CENTRALIZED PLACE TO HOLD THE MEMTRACKER (WHICH WE SHOULD, WE SHOULD ADD IT TO THAT LIST ASWELL */
			//list_add_tail( &the_list, p->ptr_to_my_mem_struct);
			/*****************************************/
			
			read_unlock_irq(&tasklist_lock);	/* we are holding a lock... */
			goto err_bad_argument;				/* this who knows if this should or should not be an error... */ 
		}
		
		
		/* ok done with initial test cases now, lets get he process that is already recorded something */
		
		current_mem_struct = p->ptr_to_my_mem_struct; /* keep a ptr to the mem_struct to avoid using two BLAH->BLAH'S_FIELD->ANOTHER ONE....*/
		
/**************************************/
	read_unlock_irq(&tasklist_lock);	/* we are dont with the task struct lock. we will not change information related to the task struct.  we can however change current_mem_struct! */ 
/**************************************/

	if (!mem_tracker_ptr_used) free(mem_tracker_ptr); /* we didnt want to malloc with a lock in hand. so malloced outside lock and if not used, freed. now im freeing it */
		
		
	if (current_mem_struct->previous_recording) /* if its not NULL we need to get rid of this first before we can stop the current recording and report it. otherwise we loose data! */
	/* a better explination: we get here only if someone called memtrack before but did not have enough space for the results. so we already moved the recorded information
	/* to the "old list". now, the user called memtrack again with enough space? so we need to give him the previous data */
	
			byte_size_of_collected_data = current_mem_struck->previous_recording_size + 8;	// if we report it, we need to add the last struct which is 8 bytes. 
					
			if( (!mm) || (byte_size_of_collected_data > len) )
			/* (!mm): user just wants to know the size of data, or the user just didnt give a big enough buffer AGAIN! */
			/* dont forget to add the size of the terminating struct... (the indicator of the last item in the buffer..*/
			{
				goto out; 
			}
			
			/* if we got here, dont do anything to the currently recording status. just report the data you previously recorded but didnt get a chance to copy to the user.*/
			
			/*
			THESE FUNCTIONS NEED TO BE IMPLEMENTED
			sort_old_recorded_data_if_needed;
			convert_old_information_to_string;
			copy_it_to_user;
			free_old_data;
			*/
			goto out; 
		}
	
	/* if we are here, then there there is no previous data waiting for the user, just the current data! */
	
	/* before we do anything, check if there is even data to report at this point. */
	if (!current_mem_struct->current_recording) /* dont have any recorded data yet*/
		goto out; 
	
	/* regular case: stop recording old data, start recording news data */
		
	/* PSUEDO COMMAND */
	switch_clear_current_data_and_start_recording_new_info; //switch pointers
	
	
	
	byte_size_of_collected_data = current_mem_struck->previous_recording_size + 8;	// if we report it, we need to add the last struct which is 8 bytes. 
		
	if(	(!mm) || /*(!mm): user just wants to know the size of data,*/
		(byte_size_of_collected_data > len)) /* or the user just didnt give a big enough buffer */
			goto out; 
		
		
			/*
			THESE FUNCTIONS NEED TO BE IMPLEMENTED
			sort_old_recorded_data_if_needed;
			convert_old_information_to_string;
			copy_it_to_user;
			free_old_data;
			*/


	/* i love goto statements, so ill also use them! */

 	/* this is the regular retunn, it will be reached in 2 cases: everything went well, new data was posted to *mm and we need to retun its lenght. */
	/* OR! if len was not big enough for the data and we need to return the size of the data */
out:
	 return  byte_size_of_collected_data; 		// no parenthesis around byte_size_of_collected_data for carlos.  
	
err_nomem: /* get here if no mem err */
    errno = ENOMEM;
    return -1;	

err_pid_not_found: /* pid was not found */
    errno = ESRCH;
    return -1;	

err_copy_to_user: /*the label says it all! */ 
    errno = EFAULT;
    return -1;		
	
err_bad_argument:
	errno = EINVAL;
    return -1;

}

EXPORT_SYMBOL_GPL(memtrack);




-----------------------------------------------------------------------------------------------------------------
/* add to existing structures */
task_struct
	{
		int memtracker = 0;
		mem_tracker* ptr_to_my_mem_struct = NULL;
	}
	
	

	
	
// one of these per process. we should just put a ptr to it in the process so we dont need to search for it in  a list. 
// processes have a hashtable = fast searching.	
struct mem_tracker {

	/* add 2 locks... dont forget to initialize them*/
	struct list_head 			current_recording
	int 						current_recording_size;
	
	struct list_head 			previous_recording
	int 						previous_recording_size;

	
	};

	
struct mmdesc {
	unsigned long start;		/* starting address */
	unsigned long end;		/* ending address */
	char bitmap[0]; 		/* working set */
};

struct kern_mmdesc
{
struct list_head mm_list;
struct mmdesc the_mmdesc;
};
create_user_output(pid_t pid)
{
/* this may sleep so no locks! called when the list is static and no longer growing. */
	mem_tracker *current_mem_struct;
	struct list_head *the_entry, *the_position;
	struct kern_mmdesc *a_kern_mmdesc;
	struct mmdesc *a_mmdesc;
		
	read_lock_irq(&tasklist_lock); /* get a lock to find our task_struct */
	p = find_process_by_pid(pid);
 	if (!p) 	
		{
			read_unlock_irq(&tasklist_lock);
			goto err_pid_not_found; 
		}
	
	current_mem_struct = p->ptr_to_my_mem_struct;
	read_unlock_irq(&tasklist_lock);

/* got the process! now lets report the info */	
	
	/* we need to create the last entry in the list like the HW specified to mark the end of the buffer. */
	
	/* allocate the last structure */
	a_kern_mmdesc = kmalloc( sizeof(struct kern_mmdesc), GFP_KERNEL );	
	if (!a_kern_mmdesc)	
		goto err_nomem:
	
	/* fill it up with the data that represents the last structure */
	a_kern_mmdesc->the_mmdesc.start = 0x0;       
    a_kern_mmdesc->the_mmdesc.end   = 0xffffffff
	
	/* add it to the tail of the list , no need to update the total size of the output, since we take it into account in memtrak (the + 8)  */
   	list_add_tail( &current_mem_struct->previous_recording, a_kern_mmdesc);
	
	/* now, run through all the recorded info and extract the mmdesc elements from the list, then report the, to user */
	list_for_each_safe(lst_entry, the_position,&current_mem_struct->previous_recording)
	{
		/* get an item from list */
		a_kern_mmdesc = list_entry(the_entry,struct kern_mmdesc,mm_list);
		/* get the mmdesc is contains */
		a_mmdesc = &a_kern_mmdesc->the_mmdesc;
		/* check if this is the last item ... */		
		if (a_mmdesc->end == 0xffffffff) 
		{
			/* this is the last one...*/
			list_del(&a_kern_mmdesc->mm_list);
			kfree(a_kern_mmdesc);
			break; 
		}
		/* now ADD the data to the output: */
	
		/*PSUEDO!!!! add the entry to a buffer  OR just copy it to the user buffer, requires us to keep track on how much we copied*/
		do_something() 
		
		/* remove the item from list.... */		
		list_del(&a_kern_mmdesc->mm_list);
		/* remove item from memory */
		kfree(a_kern_mmdesc);
	}
	
	
/*
		some code to reference.... 
	
	if ( 0 != __copy_to_user(&log[logs_copied],&krl->log,sizeof(struct reclog)) )
		goto err_copy_to_user;
		/*
		unsigned long __copy_to_user (void __user * to, const void * from, unsigned long n);
		Arguments
		to		Destination address, in user space. 
		from    Source address, in kernel space. 
		n	    Number of bytes to copy. 	
		*/
*/	
	
	
err_nomem: /* get here if no mem err */
    errno = ENOMEM;
    return -1;	

	
err_pid_not_found: /* pid was not found */
    errno = ESRCH;
    return -1;	

}