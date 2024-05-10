CC = gcc
CFLAGS = -Wall -O3 -ggdb

all:
	$(CC) $(CFLAGS) main.c pp.c -o ils5011

clean:
	rm -f ils5011

.PHONY:	all clean
