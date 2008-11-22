#ifndef __LINUX_GETRECORDING_H
#define __LINUX_GETRECORDING_H

#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/linkage.h>
#include <asm/errno.h>
#include <asm/current.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/recinfo.h>
#include <asm/types.h>
#include <asm/system.h>
#include <asm/uaccess.h>

_syscall2(int, getrecording,struct reclog *,arg1,int,arg2);

#endif

