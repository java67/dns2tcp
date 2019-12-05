CC = gcc
CFLAGS = -O3 -w

.PHONY: all clean

all: ev.o

clean:
	$(RM) *.o

ev.o: ev.c
	$(CC) $(CFLAGS) -c ev.c -o ev.o
