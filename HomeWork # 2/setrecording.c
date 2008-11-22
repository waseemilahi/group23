#include <linux/setrecording.h>

asmlinkage int sys_setrecording(int enable,int max_entries)
{

  if(max_entries>4000 || max_entries<0)return -EINVAL;

  if(enable<0 || enable>1)return -EINVAL;

  struct kern_reclog *tmp;
  struct list_head *pos, *q;

  if(enable==0){

    current->record=0;
 
    return 0;//*success*/
  }

  if(enable==1){

    current->record=1;

    if(max_entries >= current->current_log_entries)
      current->max_log_entries=max_entries;

    int i = current->current_log_entries - max_entries; 

    list_for_each_safe(pos,q,&(current->reclist))
      {

	tmp=list_entry(pos,struct kern_reclog,reclog_list);

	list_del(pos);
	kfree(tmp);

	current->current_log_entries--;

	i--;
 
	if(i==0)return 0;
      
      }

  }
 
}
