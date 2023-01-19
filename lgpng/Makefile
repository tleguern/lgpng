LIB= lgpng
SRCS= lgpng_chunks.c lgpng_chunks_extra.c lgpng_crc.c lgpng_data.c lgpng_stream.c
OBJS= ${SRCS:.c=.o}

LIBVER= 0.1
LDADD+=
LDFLAGS+=
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean install

all: lgpng.c lib${LIB}.a lib${LIB}.so

.c.o:
	${CC} ${CFLAGS} -fPIC -c $<

lgpng_chunks.c: lgpng.h
lgpng_chunks_extra.c: lgpng.h
lgpng_crc.c: lgpng.h
lgpng_data.c: lgpng.h
lgpng_stream.c: lgpng.h

lgpng.c: ${SRCS}
	cat ${SRCS} > lgpng.c

lib${LIB}.a: ${OBJS}
	${AR} rcs lib${LIB}.a ${OBJS}

lib${LIB}.so: ${OBJS}
	${CC} -shared -o $@.${LIBVER} ${OBJS} ${LDFLAGS} -Wl,-soname,$@.${LIBVER}

clean:
	rm -f -- ${OBJS} lgpng.c
	rm -f -- lib${LIB}.a lib${LIB}.so lib${LIB}.so.${LIBVER}

install:
	mkdir -p ${PREFIX}/lib/
	${INSTALL_PROGRAM} lib${LIB}.a ${PREFIX}/lib/
