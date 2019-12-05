# main
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O3
SRCS = netutils.c dns2tcp.c
OBJS = $(SRCS:.c=.o)
MAIN = dns2tcp
DESTDIR = /usr/local/bin

# libev
EVCC = $(CC)
EVCFLAGS = -O3 -w
EVSRC = libev/ev.c
EVOBJ = ev.o
EVLIBS = -lm

.PHONY: all
all: $(MAIN)

.PHONY: clean
clean:
	$(RM) *.o $(MAIN)

.PHONY: install
install: $(MAIN)
	mkdir -p $(DESTDIR)
	install -m 0755 $(MAIN) $(DESTDIR)

$(MAIN): $(OBJS) $(EVOBJ)
	$(CC) $(CFLAGS) -s -o $(MAIN) $(OBJS) $(EVOBJ) $(EVLIBS)

# OBJS
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EVOBJ): $(EVSRC)
	$(EVCC) $(EVCFLAGS) -c $(EVSRC) -o $(EVOBJ)
