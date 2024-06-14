CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99
TARGETS = mync ttt

all: $(TARGETS)

mync: mync.c
	$(CC) $(CFLAGS) -o mync mync.c

ttt: ttt.c
	$(CC) $(CFLAGS) -o ttt ttt.c

clean:
	$(RM) $(TARGETS)