PROG= test-ihdr
SRCS= ihdr.c
OBJS= ${SRCS:.c=.o}

LDADD+= -llgpng
LDFLAGS+= -L../
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign -I../
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

clean:
	rm -f -- ${OBJS} ${PROG}
