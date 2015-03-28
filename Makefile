CFLAGS=-g -Og -std=c99
LDCLAGS=

CC=gcc
LD=gcc

all:		rssb

rssb:		rssb.o
		${LD} ${LDFLAGS} $< -o $@

%.o:		%.c
		${CC} ${CFLAGS} $< -c -o $@
