include Makefile.configure

PROG= pnginfo
SRCS= lgpng.c compats.c ${PROG}.c
OBJS= ${SRCS:.c=.o}

LDADD+= -lz
LDFLAGS+=
CFLAGS+= -std=c99 -Wpointer-sign
CFLAGS+= -Wall -Wextra

.SUFFIXES: .c .o
.PHONY: clean

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ${LDADD}

clean:
	rm -f -- ${OBJS} ${PROG}

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/${PROG}
