PROG= pnginfo
SRCS= pnginfo.c
OBJS= ${SRCS:.c=.o}

LDADD?= 
LDFLAGS?=
CFLAGS+= -std=c99 -Wall -Wextra

.SUFFIXES: .c .o
.PHONY: clean

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

clean:
	rm -f -- ${PROG} ${OBJS} ${DEPS}
