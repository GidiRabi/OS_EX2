CC = gcc
CFLAGS = -Wall -g -fprofile-arcs -ftest-coverage

.PHONY: all clean

all: mync ttt

mync: mync.o
	$(CC) $(CFLAGS) -o $@ $^

ttt: ttt.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o mync ttt *.gcda *.gcno *.gcov