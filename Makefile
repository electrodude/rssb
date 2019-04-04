CFLAGS=-std=c99 -Wall -Wextra
CFLAGS+=-g -Og
#CFLAGS+=-fsanitize=address -fno-omit-frame-pointer
LDFLAGS=

CC=gcc
LD=gcc

all:    rssb

clean:
	rm -vf *.o rssb

rssb:   rssb.o
	${LD} ${LDFLAGS} $< -o $@

%.o:    %.c
	${CC} ${CFLAGS} -c $< -o $@

.PHONY: all clean
