include Makefile.configure

PROG= pngdump
SRCS= ${PROG}.c compats.c
OBJS= ${SRCS:.c=.o}

LDADD+= -llgpng
LDFLAGS+= -L../lgpng
CFLAGS= -Wall -Wextra -I ../lgpng
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

clean:
	rm -f -- ${OBJS}

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/
