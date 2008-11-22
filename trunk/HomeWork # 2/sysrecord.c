#include <linux/sched.h>
#include <asm/current.h>
#include <asm/unistd.h>
#include <linux/slab.h>//kmalloc and kfree
#include <linux/kernel.h>

asmlinkage void record_syscall(long arg1,long arg2,long arg3,long arg4,long arg5,long arg6)
{
  long index;

//what happend?!?? let me fix it
//there is no header nothing! ill copy it from what you sent me
__asm__ __volatile__("mov %%eax, %0;" : "r=" (index));

// printk("index is %ld\n",index);
  //if logging is disabled do nothing.
  if(!(current->record==1)) return ;

  //else

  if(current->current_log_entries < current->max_log_entries){

    struct kern_reclog *tmp=(struct kern_reclog*)kmalloc(sizeof(struct kern_reclog),GFP_KERNEL);

      //    INIT_LIST_HEAD(&tmp->reclog_list);

       tmp->syscall_nr=index;
       tmp->arg1=arg1;
       tmp->arg2=arg2;
       tmp->arg3=arg3;
       tmp->arg4=arg4;
       tmp->arg5=arg5;
       tmp->arg6=arg6;
    
       list_add_tail(&(tmp->reclog_list),&(current->reclist)); 

       printk("inside sysrecord: recieved: %ld : %ld : %ld\n",tmp->arg1,tmp->arg2, tmp->arg3);
       
       current->current_log_entries++;   

       return ;

  }
 
}
