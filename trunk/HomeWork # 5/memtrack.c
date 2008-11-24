#include "memtrack.h"
#include <linux/mm.h>

/* Global Declarations. */
	static int global_mem_tracker = 0 ;

/* Container list for all mem_trackers */
	struct mem_tracker global_tracker_list ;
	
/*This lock keeps the access to global tracker list synchronized
  kind of a global lock: imitialize it globally*/
rwlock_t global_rw_lock = RW_LOCK_UNLOCKED;


/* given the start and end fields in a vm_area_struct, the function will return a malloced, populated kern_mmdesc structure.. */
struct kern_mmdesc* vma_2_mmdesc(unsigned long vm_start,unsigned long vm_end)
{
  struct kern_mmdesc *a_kern_mmdesc = kmalloc( sizeof(struct kern_mmdesc) + sizeof(char)*((((vm_end - vm_start) >> PAGE_SHIFT) + 7) / 8), GFP_KERNEL );
if (!a_kern_mmdesc) /* malloc faild! */
goto err_nomem; /* err reporting */

a_kern_mmdesc->the_mmdesc.start = vm_start;
a_kern_mmdesc->the_mmdesc.end = vm_end;

return a_kern_mmdesc;

err_nomem: /* get here if no mem err */
errno = ENOMEM;
return NULL;
}

/* The Helper Functions. */
/* Report the size of the bitmap given th start/end pair of mmdesc. */
inline static int mmdesc_bmap(unsigned long start, unsigned long end)
{
  return ((((end - start) >> PAGE_SHIFT) + 7) / 8);
}

/* Get a pointer to the next mmdesc, given the current mmdesc. */
inline static struct mmdesc *mmdesc_next(struct mmdesc *mmdesc)
{
  char *ptr = (char *)mmdesc ;
  ptr += sizeof(struct mmdesc) + mmdesc_bmap(mmdesc -> start , mmdesc -> end) ;
  return (struct mmdesc *) ptr ;
}

/* Get the total size of the populated mmdesc structures. */
inline static int mmdesc_size(struct mmdesc *mmdesc)
{
  int len = 0;
  while(mmdesc -> end != 0xffffffff)
    {
      len += sizeof(struct mmdesc);
      len += mmdesc_bmap(mmdesc -> start , mmdesc -> end) ;
      mmdesc = mmdesc_next(mmdesc);
    }
  return len + sizeof(struct mmdesc) ;
}

/*Important function. Populates the structure to be recorded.*/
int stop_start(struct task_struct *p)
{
	int i;
	int ret;
	int nr_pages =0;
	struct vm_area_struct *tmp = p->mm->mmap;
	struct kern_mmdesc *kern_mmdesc_ptr;
	unsigned long dummy_address; 
	 
	/* used to "walk throught the page tables.... */
	pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep, pte;
   
   /*************************************/

    while (tmp) {
		/* this loop goes through each vma in the list. */
		
		nr_pages =(tmp->vm_end - tmp->vm_start) >> PAGE_SHIFT;
		/* for each VMA we need a kern_mmdesc. */
		kern_mmdesc_ptr = vma_2_mmdesc(tmp->vm_start,tmp->vm_end);
				
		for (i=0;i<nr_pages;i++)
		{
			spin_lock(&p->mm->page_table_lock);
			/* 1: 		extract an address of within the i'th page : */
			dummy_address = tmp->vm_start + 4096*i + 10; /* the 10 is just to make sure its within a page... */
			
			/* 2:		get thats page */
			
			pgd = pgd_offset(p->mm,dummy_address);
			if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
                 goto out;
 
	        pud = pud_offset(pgd, dummy_address);
	        if (pud_none(*pud) || unlikely(pud_bad(*pud)))
	             goto out;
	         
	        pmd = pmd_offset(pud, dummy_address);
	        if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
	             goto out;
					 
	        ptep = pte_offset_map(pmd, dummy_address);
	        if (!ptep)
	             goto out;
				 
			pte = *ptep;
			pte_unmap(ptep);
			if (pte_present(pte)) 
			/* the page is here... */
			{	 
				ret = ptep_test_and_clear_mmtrack_accessed(ptep);
			}
		spin_unlock(&p->mm->page_table_lock);
		}
		/* now lets add this VMA's kern_mmdesc to out list */
		
		spin_lock(&p->ptr_to_my_mem_struct->x);
		list_add_tail( &(p->ptr_to_my_mem_struct->previous_recording), &(kern_mmdesc_ptr->mm_list));
		p->ptr_to_my_mem_struct->kern_mmdesc_counter_previous++;
		spin_unlock(&p->ptr_to_my_mem_struct->x);
		/* just do it for one VMA for now... so break... */
		break;
		tmp = tmp->vm_next;
    }
	
out:
return 0;
}

/* --------------------------------------------------------------------------- */
/* The Actual System Call. */
asmlinkage long sys_memtrack(pid_t pid, struct mmdesc __user *mm, int len)
{

    int byte_size_of_collected_data =0; /* in bytes! must include last structure too */
	struct task_struct *p;
	struct mem_tracker *current_mem_struct;
	struct mem_tracker *mem_tracker_ptr;
	int mem_tracker_ptr_used = 0;
	int count = 0;
	struct kern_mmdesc *a_kern_mmdesc,*an_kern_mmdesc;
	struct mmdesc *a_mmdesc;
	
	struct list_head *the_entry, *the_position;
		
    mem_tracker_ptr =  kmalloc( sizeof(struct mem_tracker), GFP_KERNEL );	/* malloced here to avoid mallocation under a lock */
	if (!mem_tracker_ptr)	/* malloc faild! */
		goto err_nomem;		/* err reporting */
		
	/* check trivial stuff here */
	if ((len<0) || (pid<0)) goto err_bad_argument;
	
	read_lock_irq(&tasklist_lock); /* get a lock to find our task_struct */
	
	p = find_task_by_pid(pid); /* try to get the process! */
 	if (!p) /* the process doesnt exist */	
		{
			kfree(mem_tracker_ptr);				/* allocated but never used */
			read_unlock_irq(&tasklist_lock);	/* release lock */
			goto err_pid_not_found; 			/* err reporting */
		}
		
	/* If this is the first time ever memtrack call after reboot.
		We initialize a global list of mem_tracks to hold all the mem_track structs.*/
		
		write_lock(&global_rw_lock);
		
		if(global_mem_tracker == 0)
		{
			global_mem_tracker ++;
			/*Its a dummy head no need to set other fields for it.*/
			INIT_LIST_HEAD(&global_tracker_list.mem_list);
		}
		
		write_unlock(&global_rw_lock);
		
	/* First memtrack() call for process p. Set every thing up for it. */
		if(!p->memtracker)
		{
			/* this is the first call to the never ending?! function.... */
			
			p->memtracker = 1;	/* set a recording flag so we dont end up in this if again */ 
						 
			p->ptr_to_my_mem_struct = mem_tracker_ptr; /* update the ptr in the task struct to point to the memory recording struct. */
			
			/* initialize the structure */
			INIT_LIST_HEAD(&mem_tracker_ptr->previous_recording);
			INIT_LIST_HEAD(&mem_tracker_ptr->mem_list);
				
			/*initialize the spin lock for 'this' mem_tracker*/
			spin_lock_init(&mem_tracker_ptr->x);	
			mem_tracker_ptr->kern_mmdesc_counter_previous = 0;
						
			mem_tracker_ptr_used = 1; /* this is a flag that we used the allocated structure and we should not free it */
			
				
		/* WE HAVE A CENTRALIZED PLACE TO HOLD THE MEMTRACKER (WE SHOULD ADD IT TO THAT LIST ASWELL */
				
			/*---------------------------------------*/			
				write_lock(&global_rw_lock);
				list_add_tail( &(global_tracker_list.mem_list), &(mem_tracker_ptr->mem_list));
				write_unlock(&global_rw_lock);
			/*---------------------------------------*/
							
			read_unlock_irq(&tasklist_lock);	/* we are holding a lock... */				
			goto out;				/* Home work wants us to return 0 in this case. */ 
		}
		
			
		/* ok done with initial test cases now, lets get the process that is already recorded something */
		
		current_mem_struct = p->ptr_to_my_mem_struct; /* keep a ptr to the mem_struct to avoid using two BLAH->BLAH'S_FIELD->ANOTHER ONE....*/
		

		/* we are done with the task struct lock. we will not change information related to the task struct.  we can however change current_mem_struct! */ 
		/*------------------------------------*/
		read_unlock_irq(&tasklist_lock);	
		/*------------------------------------*/
		
		if (!mem_tracker_ptr_used) kfree(mem_tracker_ptr); /* we didnt want to malloc with a lock in hand. so malloced outside lock and if not used, freed. now im freeing it */	
				
		spin_lock(&current_mem_struct->x);
			
		if (!list_empty(&current_mem_struct->previous_recording)) 
		{	/* if its not NULL we need to get rid of this first before we can stop the current recording and report it. otherwise we loose data! */
			/* a better explination: we get here only if someone called memtrack before 
			but did not have enough space for the results. so we kept the information in previous_recording.
			now, the user called memtrack again with enough space? so we need to give him the previous data */
	
			list_for_each_safe(the_entry, the_position,&current_mem_struct->previous_recording)
			{
				/* get an item from list */
				a_kern_mmdesc = list_entry(the_entry,struct kern_mmdesc,mm_list);
				/* get the mmdesc it contains */
				a_mmdesc = &(a_kern_mmdesc->the_mmdesc);
				/* check if this is the last item ... */		
				if (count == current_mem_struct->kern_mmdesc_counter_previous)break; 
			
				count ++;
				byte_size_of_collected_data += mmdesc_bmap(a_mmdesc->start,a_mmdesc->end);
				byte_size_of_collected_data += 8;			
			}
			
			byte_size_of_collected_data += 8;	// if we report it, we need to add the last struct which is 8 bytes.
				
			count = 0;
			
			if( (!mm) || (byte_size_of_collected_data > len) )
			/* (!mm): user just wants to know the size of data, or the user just didnt give a big enough buffer AGAIN! */
			/* dont forget to add the size of the terminating struct... (the indicator of the last item in the buffer..*/
			{
				spin_unlock(&current_mem_struct->x);
				goto out ;
			}
			
			/* allocate the last structure */
			an_kern_mmdesc = kmalloc( sizeof(struct kern_mmdesc), GFP_KERNEL );	
			if (!an_kern_mmdesc)
			{
				spin_unlock(&current_mem_struct->x);
				goto err_nomem;
			}

			/* fill it up with the data that represents the last structure */
				an_kern_mmdesc->the_mmdesc.start = 0x0;       
				an_kern_mmdesc->the_mmdesc.end   = 0xffffffff;
			   
			   /* add it to the tail of the list , no need to update the total size of the output, since we take it into account in memtrak (the + 8)  */
				list_add_tail( &current_mem_struct->previous_recording, &(an_kern_mmdesc->mm_list));
				current_mem_struct->kern_mmdesc_counter_previous ++;
			
				count = 0;
			
			/* if we got here, dont do anything to the currently recording status.
			   just report the data you previously recorded but didnt get a chance to copy to the user.*/
			   
			   /* now, run through all the recorded info and extract the mmdesc elements from the list, then report them, to user */
				list_for_each_safe(the_entry, the_position,&current_mem_struct->previous_recording)
				{			
					/* get an item from list */
					a_kern_mmdesc = list_entry(the_entry,struct kern_mmdesc,mm_list);
					/* get the mmdesc is contains */
					a_mmdesc = &a_kern_mmdesc->the_mmdesc;
				
					/* now ADD the data to the output: */
					if(a_mmdesc->end != 0xffffffff)									
					  if(copy_to_user((mm + count),a_mmdesc,(8+mmdesc_bmap(a_mmdesc->start,a_mmdesc->end)))){
						spin_unlock(&current_mem_struct->x);
						goto err_copy_to_user ;
					  }
					
					count += (8 + mmdesc_bmap(a_mmdesc->start,a_mmdesc->end));
				
					current_mem_struct->kern_mmdesc_counter_previous --;
					
					/* remove the item from list.... */		
					list_del(&a_kern_mmdesc->mm_list);
					/* remove item from memory */
					kfree(a_kern_mmdesc);
				}
			
				spin_unlock(&current_mem_struct->x);
				goto out ;	
		}
		
		spin_unlock(&current_mem_struct->x);
					
		/*This function populates the list of kern_mmdescs using the bits from page table. */
		stop_start(p);
								
		spin_lock(&current_mem_struct->x);
		if(list_empty(&current_mem_struct->previous_recording))
		{
			spin_unlock(&current_mem_struct->x);
			goto out;
		}
		spin_unlock(&current_mem_struct->x);
		
			/* allocate the last structure */
			an_kern_mmdesc = kmalloc( sizeof(struct kern_mmdesc), GFP_KERNEL );	
			if (!an_kern_mmdesc)goto err_nomem;
				
				/* fill it up with the data that represents the last structure */
				an_kern_mmdesc->the_mmdesc.start = 0x0;       
				an_kern_mmdesc->the_mmdesc.end   = 0xffffffff;
			   
			spin_lock(&current_mem_struct->x);
				
			   /* add it to the tail of the list , no need to update the total size of the output, since we take it into account in memtrak (the + 8)  */
				list_add_tail( &current_mem_struct->previous_recording, &(an_kern_mmdesc->mm_list));
				current_mem_struct->kern_mmdesc_counter_previous ++;
							
				count = 0;
			
				/* if we got here, dont do anything to the currently recording status.
			   just report the data you previously recorded but didnt get a chance to copy to the user.*/
			   
			   /* now, run through all the recorded info and extract the mmdesc elements from the list, then report them, to user */
				list_for_each_safe(the_entry, the_position,&current_mem_struct->previous_recording)
				{			
					/* get an item from list */
					a_kern_mmdesc = list_entry(the_entry,struct kern_mmdesc,mm_list);
					/* get the mmdesc is contains */
					a_mmdesc = &a_kern_mmdesc->the_mmdesc;
				
					/* now ADD the data to the output: */
					if(a_mmdesc->end != 0xffffffff)									
					  if(copy_to_user((mm + count),a_mmdesc,(8+mmdesc_bmap(a_mmdesc->start,a_mmdesc->end)))){
						spin_unlock(&current_mem_struct->x);
						goto err_copy_to_user ;
					  }
									
					count += (8 + mmdesc_bmap(a_mmdesc->start,a_mmdesc->end));
					
					current_mem_struct->kern_mmdesc_counter_previous --;
					
					/* remove the item from list.... */		
					list_del(&a_kern_mmdesc->mm_list);
					/* remove item from memory */
					kfree(a_kern_mmdesc);
				}
			
				spin_unlock(&current_mem_struct->x);
				goto out ;	
					
			spin_unlock(&current_mem_struct->x);
		
 /* this is the regular retunn, it will be reached in 2 cases: everything went well, new data was posted to *mm and we need to retun its lenght. */
	/* OR! if len was not big enough for the data and we need to return the size of the data */
out:
	 return  byte_size_of_collected_data; 		 
	
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
