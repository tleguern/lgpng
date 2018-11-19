include Makefile.configure

SRCS= lgpng.c compats.c
OBJS= ${SRCS:.c=.o}

LDADD+= -lz
LDFLAGS+=
CFLAGS+= -std=c99

.SUFFIXES: .c .o
.PHONY: clean

.c.o:
	${CC} ${CFLAGS} -c $<

all: pnginfo pngblank

pnginfo: pnginfo.o ${OBJS}
	${CC} ${LDFLAGS} -o $@ $> ${LDADD}

pngblank: pngblank.o ${OBJS}
	${CC} ${LDFLAGS} -o $@ $> ${LDADD}

clean:
	rm -f -- ${OBJS} pngblank.o pnginfo.o pngblank pnginfo

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} ${PROG} ${PREFIX}/bin/${PROG}
