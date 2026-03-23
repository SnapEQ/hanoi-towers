CC = gcc
CFLAGS = -g -Wall -pedantic -fsanitize=undefined $(shell pkg-config --cflags sdl2 SDL2_gfx)
LDLIBS = -fsanitize=undefined $(shell pkg-config --libs sdl2 SDL2_gfx) -lm

all: main

main: main.o primlib.o
	$(CC) $^ -o $@ $(LDLIBS)

main.o: main.c primlib.h
	$(CC) $(CFLAGS) -c main.c

primlib.o: primlib.c primlib.h
	$(CC) $(CFLAGS) -c primlib.c

clean:
	rm -f main *.o
