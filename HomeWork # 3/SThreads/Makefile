#
# Makefile for SThreads
#

CC=gcc
CFLAGS=-g -O2 -Wall
LDFLAGS=-g

.c.s:
	$(CC) $(CFLAGS) $(CPPFLAGS) -S $<

libsthread.a: sthread.o sync.o
	$(AR) cr $@ $^

my_test: my_test.o libsthread.a
	$(CC) $(LDFLAGS) -o $@ my_test.o -L. -lsthread

clean:
	$(RM) libsthread.a my_test *.o *~
