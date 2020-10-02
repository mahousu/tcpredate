CFLAGS= -O3

TESTS= timevaltest1 timevaltest2 timevaltest3 timevaltest4 timevaltest5

all: tcpredate tests

depend:
	makedepend *.c

tcpredate: tcpredate.o timeval32.o
	$(CC) $(CFLAGS) -o tcpredate tcpredate.o timeval32.o -lm

tests: $(TESTS)

timevaltest1: timevaltest.c timeval32.o
	$(CC) $(CFLAGS) -DTEST1 -o timevaltest1 timevaltest.c timeval32.o -lm

timevaltest2: timevaltest.c timeval32.o
	$(CC) $(CFLAGS) -DTEST2 -o timevaltest2 timevaltest.c timeval32.o -lm

timevaltest3: timevaltest.c timeval32.o
	$(CC) $(CFLAGS) -DTEST3 -o timevaltest3 timevaltest.c timeval32.o -lm

timevaltest4: timevaltest.c timeval32.o
	$(CC) $(CFLAGS) -DTEST4 -o timevaltest4 timevaltest.c timeval32.o -lm

timevaltest5: timevaltest.c timeval32.o
	$(CC) $(CFLAGS) -DTEST5 -o timevaltest5 timevaltest.c timeval32.o -lm

clean:
	-rm -f tcpredate *.o $(TESTS)


# DO NOT DELETE THIS LINE -- make depend depends on it.

