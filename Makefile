CFLAGS=-std=c11 -g -fno-common
CC=gcc

rvcc: main.o
	$(CC) -o rvcc $(CFLAGS) main.o

test: rvcc
	./test.sh

clean:
	rm -f rvcc *.o *.s tmp* a.out

.PHONY: test clean
