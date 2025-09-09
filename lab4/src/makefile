CC = gcc
CFLAGS = -I. -I../../lab3/src -pthread
LDFLAGS = -pthread -L. -L../../lab3/src -lutils -lsum

all: process_memory psum

process_memory: process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

psum: parallel_sum.o libsum.a
	$(CC) -o psum parallel_sum.o $(LDFLAGS)

parallel_sum.o: parallel_sum.c sum_lib.h ../../lab3/src/utils.h
	$(CC) $(CFLAGS) -c parallel_sum.c -o parallel_sum.o

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) $(CFLAGS) -c sum_lib.c -o sum_lib.o

libsum.a: sum_lib.o
	ar rcs libsum.a sum_lib.o

clean:
	rm -f *.o process_memory psum libsum.a