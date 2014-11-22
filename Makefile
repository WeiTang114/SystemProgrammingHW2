DEBUG = 
CC = gcc
CFLAGS = -std=gnu99 -Wall -I./
OBJS = organizer.o judge.o player.o utils.o
EXECS = organizer judge player

ifdef DEBUG
	CFLAGS += -g -DDEBUG
endif

all: $(EXECS)

organizer: organizer.o
	$(CC) -o $@ $<
judge: judge.o utils.o
	$(CC) -o $@ $< utils.o
player: player.o utils.o
	$(CC) -o $@ $< utils.o
organizer.o: organizer.c
	$(CC) -c $< $(CFLAGS)
judge.o: judge.c
	$(CC) -c $< $(CFLAGS)
%.o: %.c
	$(CC) -c $< $(CFLAGS)

.PHONY: clean
clean:
	rm $(EXECS) $(OBJS)