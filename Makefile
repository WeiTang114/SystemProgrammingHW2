DEBUG = 1
CC = gcc
CFLAGS = -std=gnu99 -Wall
OBJS = organizer.o judge.o player.o
EXECS = organizer judge player

ifdef DEBUG
	CFLAGS += -g -DDEBUG
endif

all: $(EXECS)



organizer: organizer.o
	$(CC) -o $@ $<
judge: judge.o
	$(CC) -o $@ $<
player: player.o
	$(CC) -o $@ $<

organizer.o: organizer.c
	$(CC) -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

.PHONY: clean
clean:
	rm $(EXECS) $(OBJS)