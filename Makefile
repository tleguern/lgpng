LIB= lgpng
SRCS= lgpng.c lgpng_stream.c
OBJS= ${SRCS:.c=.o}

LDADD+=
LDFLAGS+=
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: lib${LIB}.a pnginfo

.c.o:
	${CC} ${CFLAGS} -c $<

lib${LIB}.a: ${OBJS}
	${AR} cqD lib${LIB}.a ${OBJS}
	${RANLIB} lib${LIB}.a

pnginfo: pnginfo.c lib${LIB}.a
	${CC} ${CFLAGS} pnginfo.c -o pnginfo -L . -l${LIB}

clean:
	rm -f -- ${OBJS} lib${LIB}.a pnginfo pnginfo.o

install:
	mkdir -p ${PREFIX}/lib/
	${INSTALL_PROGRAM} lib${LIB}.a ${PREFIX}/lib/
