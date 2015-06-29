CFLAGS=-g -Og -std=c99
LDFLAGS=

CC=gcc
LD=gcc

all:		rssb

clean:		
		rm -vf *.o rssb

rssb:		rssb.o
		${LD} ${LDFLAGS} $< -o $@

%.o:		%.c
		${CC} ${CFLAGS} $< -c -o $@

.PHONY:		all clean
