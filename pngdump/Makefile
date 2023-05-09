include Makefile.configure

PROG= pngdump
SRCS= ${PROG}.c compats.c
OBJS= ${SRCS:.c=.o}

CFLAGS= -Wall -Wextra -I ../lgpng
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} -o $@ ${OBJS} ../lgpng/liblgpng.a

clean:
	rm -f -- ${OBJS}

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/
