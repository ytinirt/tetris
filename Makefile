CC=gcc
CFLAGS=-Wall
LDFLAGS=-lncurses

tetris: tetris.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm -f tetris *.o
