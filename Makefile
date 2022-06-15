LIB= lgpng
SRCS= lgpng.c
OBJS= ${SRCS:.c=.o}

LDADD+=
LDFLAGS+=
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean

all: lib${LIB}.a helper

.c.o:
	${CC} ${CFLAGS} -c $<

lib${LIB}.a: ${OBJS}
	${AR} cqD lib${LIB}.a ${OBJS}
	${RANLIB} lib${LIB}.a

helper: helper.c
	${CC} ${CFLAGS} helper.c -o helper -L . -l${LIB}

clean:
	rm -f -- ${OBJS} lib${LIB}.a

install:
	mkdir -p ${PREFIX}/lib/
	${INSTALL_PROGRAM} lib${LIB}.a ${PREFIX}/lib/
