#include <linux/getrecording.h>
  
asmlinkage int sys_getrecording(struct reclog *log,int nr_logs)
{

  if(log == NULL || nr_logs < 0)return -EINVAL;  

  //if()return -EFAULT;

  //For empty reclist
  if(current->current_log_entries == 0)
    return 0;

  //  int i = nr_logs , j = current->current_log_entries;
  //int k=current->current_log_entries,l =  0;

  struct kern_reclog *temp_reclog;
 
  int logs_copied_counter=0;
  struct list_head *current_pos,*temp_pos;

  list_for_each_safe(current_pos,temp_pos,&(current->reclist))
    {
      temp_reclog=list_entry(current_pos,struct kern_reclog,reclog_list);
 
      if(temp_reclog == NULL)return logs_copied_counter;

      /*      if(copyout(&(temp_reclog->syscall_nr),&log[logs_copied_counter].syscall_nr,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg1),&log[logs_copied_counter].arg1,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg2),&log[logs_copied_counter].arg2,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg3),&log[logs_copied_counter].arg3,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg4),&log[logs_copied_counter].arg4,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg5),&log[logs_copied_counter].arg5,sizeof(log))!=0)return -EFAULT;
      if(copyout(&(temp_reclog->arg6),&log[logs_copied_counter].arg6,sizeof(log))!=0)return -EFAULT;
      */

if(put_user(temp_reclog->syscall_nr,&(log[logs_copied_counter].syscall_nr))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg1,&(log[logs_copied_counter].arg1))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg2,&(log[logs_copied_counter].arg2))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg3,&(log[logs_copied_counter].arg3))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg4,&(log[logs_copied_counter].arg4))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg5,&(log[logs_copied_counter].arg5))!=0)
       return -EFAULT;
 if(put_user(temp_reclog->arg6,&(log[logs_copied_counter].arg6))!=0)
       return -EFAULT;
	 /*
      log->syscall_nr=tmp->syscall_nr;
      log->arg1 = tmp->arg1;
      log->arg2 = tmp->arg2;
      log->arg3 = tmp->arg3;
      log->arg4 = tmp->arg4;
      log->arg5 = tmp->arg5;
      log->arg6 = tmp->arg6; 
	 */
	 logs_copied_counter++;
	 current->current_log_entries--;
	 list_del(current_pos);
	 kfree(temp_reclog);
	 /*
      log++;
      
      l++;

      i--;j--;current->current_log_entries--;
      if(i==0)return nr_logs;
      if(j==0)return k;
      if(log == NULL)return l;
	 */

	 if(current->current_log_entries == 0)break;

    }

      return(logs_copied_counter);

}
