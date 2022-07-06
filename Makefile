LIB= lgpng
SRCS= lgpng.c lgpng_crc.c lgpng_stream.c
OBJS= ${SRCS:.c=.o}

LDADD+=
LDFLAGS+=
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: lib${LIB}.a pnginfo pngdump pngexplode pngshuffle

.c.o:
	${CC} ${CFLAGS} -c $<

lib${LIB}.a: ${OBJS}
	${AR} cqD lib${LIB}.a ${OBJS}
	${RANLIB} lib${LIB}.a

pnginfo: lib${LIB}.a pnginfo.c
	${CC} ${CFLAGS} pnginfo.c -o pnginfo -L . -l${LIB}

pngdump: lib${LIB}.a pngdump.c
	${CC} ${CFLAGS} pngdump.c -o pngdump -L . -l${LIB}

pngexplode: lib${LIB}.a pngexplode.c
	${CC} ${CFLAGS} pngexplode.c -o pngexplode -L . -l${LIB}

pngshuffle: lib${LIB}.a pngshuffle.c
	${CC} ${CFLAGS} pngshuffle.c -o pngshuffle -L . -l${LIB}


clean:
	rm -f -- ${OBJS} lib${LIB}.a pnginfo pnginfo.o pngdump pngdump.o pngexplode pngexplode.o pngshuffle pngshuffle.o

install:
	mkdir -p ${PREFIX}/lib/
	${INSTALL_PROGRAM} lib${LIB}.a ${PREFIX}/lib/
