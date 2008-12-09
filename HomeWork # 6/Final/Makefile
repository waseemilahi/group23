obj-m += ptreefs.o
KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/2.6.11.12zap/build M=$(PWD) modules
clean :
	make -C /lib/modules/2.6.11.12zap/build M=$(PWD) clean
