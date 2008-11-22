#ifndef __LINUX_RECINFO_H
#define __LINUX_RECINFO_H

struct reclog{
  long syscall_nr;
  long arg1;
  long arg2;
  long arg3;
  long arg4;
  long arg5;
  long arg6;
};

#endif
