LIB= lgpng
SRCS= lgpng_chunks.c lgpng_crc.c lgpng_data.c lgpng_stream.c
OBJS= ${SRCS:.c=.o}

LDADD+=
LDFLAGS+=
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: lib${LIB}.a

.c.o:
	${CC} ${CFLAGS} -c $<

lgpng_chunks.c: lgpng.h
lgpng_crc.c: lgpng.h
lgpng_data.c: lgpng.h
lgpng_stream.c: lgpng.h

lib${LIB}.a: ${OBJS}
	${AR} cqD lib${LIB}.a ${OBJS}
	ranlib lib${LIB}.a

clean:
	rm -f -- ${OBJS} lib${LIB}.a

install:
	mkdir -p ${PREFIX}/lib/
	${INSTALL_PROGRAM} lib${LIB}.a ${PREFIX}/lib/
