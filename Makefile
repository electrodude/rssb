CFLAGS=-g -Og -std=c99
LDFLAGS=

CC=gcc
LD=gcc

all:		rssb

rssb:		rssb.o
		${LD} ${LDFLAGS} $< -o $@

%.o:		%.c
		${CC} ${CFLAGS} $< -c -o $@
