#include <sys/types.h>
#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>

#include "/usr/src/linux-2.6.11.12-hmwk2/include/linux/recinfo.h"
#include "/usr/src/linux-2.6.11.12-hmwk2/include/asm/unistd.h"
#include "/usr/include/errno.h"

#define __NR_setrecording 289 
_syscall2(int ,setrecording,int,enable,int,max_entries)


#define __NR_getrecording 290
_syscall2(int,getrecording,struct reclog*,log,int, nr_logs)


main()
{
  
  // testing this: 
  struct reclog rec_array[10];  

  FILE *fd;
  int max_rex = 10; 

  int l;
  char line[80];
 
 
  // actuall test part:
  l = setrecording(1,10);

  syscall(5,"txt.txt","r"); // fopen
  syscall(6,fd);//fclose
 
  syscall(6,fd); 
  syscall(5,"txt.txt","w"); // add 1.
  syscall(6,fd);            // another

  syscall(289,0,5); // guess which one that is... :p
  
  // end of test part


  int ret = getrecording(rec_array,10);
  

  fprintf(stdout," -------------------------------------\n\n");
  fprintf(stdout," Setrecording returned : %d\n",l);
  fprintf(stdout," There were %d items recorded\n",ret);

  int i;
  fprintf(stdout," -------------------------------------\n\n");
  fprintf(stdout," Following System Calls Were Called:\n\n");

  for(i=0;i<ret;i++)
    fprintf(stdout," N.__NR_[%d]\t(%d\t,%d\t,%d\t,%d\t,%d\t,%d\t)\n",rec_array[i].syscall_nr,rec_array[i].arg1,rec_array[i].arg2,rec_array[i].arg3,rec_array[i].arg4,rec_array[i].arg5,rec_array[i].arg6);
 

}
