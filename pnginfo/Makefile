include Makefile.configure

PROG= pnginfo
SRCS= ${PROG}.c compats.c
OBJS= ${SRCS:.c=.o}

LDADD+= -lz
LDFLAGS+=
CFLAGS+= -Wall -Wextra -I ../lgpng
CFLAGS+= -std=c99 -Wpointer-sign

.SUFFIXES: .c .o
.PHONY: clean

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c $<

${PROG}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ ${OBJS} ../lgpng/liblgpng.a ${LDADD}

clean:
	rm -f -- ${OBJS} ${PROG}

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/${PROG}
