include Makefile.configure

SRCS= lgpng.c compats.c
OBJS= ${SRCS:.c=.o}

LDADD+= -lz
LDFLAGS+=
CFLAGS+= -std=c99

.SUFFIXES: .c .o
.PHONY: clean

all: pnginfo pngblank

.c.o:
	${CC} ${CFLAGS} -c $<

pnginfo: pnginfo.o ${OBJS}
	${CC} ${LDFLAGS} -o $@ pnginfo.o ${OBJS} ${LDADD}

pngblank: pngblank.o ${OBJS}
	${CC} ${LDFLAGS} -o $@ pngblank.o ${OBJS} ${LDADD}

clean:
	rm -f -- ${OBJS} pngblank.o pnginfo.o pngblank pnginfo

install:
	mkdir -p ${PREFIX}/bin/
	${INSTALL_PROGRAM} pnginfo ${PREFIX}/bin/pnginfo
	${INSTALL_PROGRAM} pngblank ${PREFIX}/bin/pngblank
