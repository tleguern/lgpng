LDADD+= -llgpng
LDFLAGS+= -L../lgpng
CFLAGS= -Wall -Wextra
CFLAGS+= -std=c99 -Wpointer-sign -I../lgpng
NOMAN= yes

.SUFFIXES: .c .o
.PHONY: clean

#all: test-data test-ihdr test-stream
all: test-data test-stream

.c.o:
	${CC} ${CFLAGS} -c $<

test-data: data.o
	${CC} ${LDFLAGS} -o $@ data.o ${LDADD}

#test-ihdr: ihdr.o
#	${CC} ${LDFLAGS} -o $@ ihdr.o ${LDADD}

test-stream: stream.o
	${CC} ${LDFLAGS} -o $@ stream.o ${LDADD}

clean:
	rm -f -- *.o test-*
