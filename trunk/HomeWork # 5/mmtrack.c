#include <stdio.h>
#include <sys/types.h>
#include "asm/unistd.h"
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define PAGE_SHIFT 12

struct mmdesc {
	unsigned long start;		/* starting address */
	unsigned long end;		/* ending address */
	char bitmap[0]; 		/* working set */
};

inline static int mmdesc_bmap(unsigned long start, unsigned long end)
{
        return ((((end - start) >> PAGE_SHIFT) + 7) / 8);
}

/* get a pointer to the next mmdesc, given the current mmdesc */
inline static struct mmdesc *mmdesc_next(struct mmdesc *mmdesc)
{
        char *ptr = (char *) mmdesc;
        ptr += sizeof(struct mmdesc) +
                mmdesc_bmap(mmdesc->start, mmdesc->end);
        return (struct mmdesc *) ptr;
}

/* get the total size of the populated mmdesc structure */
inline static int mmdesc_size(struct mmdesc *mmdesc)
{
        int len = 0;
        while (mmdesc->end != 0xffffffff) {
                len += sizeof(struct mmdesc);
                len += mmdesc_bmap(mmdesc->start, mmdesc->end);
                mmdesc = mmdesc_next(mmdesc);
        }
        return len + sizeof(struct mmdesc);
}



int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s PID\n", argv[0]);
		exit(1);
	}
	char input[7];
	int i;
	pid_t pid =0;
	
	pid = atoi(argv[1]);	
	
	/* got the input. */
	int stop = 7;
	int bufsize = 0;
	struct mmdesc *mm;
	while(0)
	{
	  bufsize =  syscall(223,pid, NULL, 0);
		mm = malloc(bufsize+1);
		if(!mm) {
		perror("failed. out of memory");
		return -1;
		}
		bufsize = syscall(223,pid, mm , bufsize+1);
		if (!bufsize)
			{
			  printf("Returned : %d\n",bufsize);
				return -1;
			}


		fprintf(stdout,"FIELD           VALUE       LENGTH");
		fprintf(stdout,"-------------   ----------  --------------------------");
	
		int len = 0;
        while (mm->end != 0xffffffff) 
		{

		fprintf(stdout,"mmdesc->start\t0x%x (4 bytes)",mm->start);
		fprintf(stdout,"mmdesc->end\t0x%x (4 bytes)",mm->end);
		fprintf(stdout,"bitmap\t (%d bytes: to hold ? bits)",mmdesc_bmap(mm->start,mm->end));
		
		mm = mmdesc_next(mm);
        }
		
		fprintf(stdout,"mmdesc->start\t0x%x (4 bytes)",mm->start);
		fprintf(stdout,"mmdesc->end\t0x%x (4 bytes)",mm->end);
		fprintf(stdout,"bitmap\t (%d bytes: to hold ? bits)",mmdesc_bmap(mm->start,mm->end));

	
		sleep(5);
		if(!stop--) break;
	}

}
