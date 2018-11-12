include Makefile.configure

PROG= pnginfo
SRCS= pnginfo.c compats.c
OBJS= ${SRCS:.c=.o}

LDADD?= 
LDFLAGS?=
CFLAGS+= -std=c99

.SUFFIXES: .c .o
.PHONY: clean

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

clean:
	rm -f -- ${PROG} ${OBJS} ${DEPS}

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/${PROG}
