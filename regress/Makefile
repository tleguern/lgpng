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
	${CC} -o $@ data.o ../lgpng/liblgpng.a

#test-ihdr: ihdr.o
#	${CC} -o $@ ihdr.o ../lgpng/liblgpng.a

test-stream: stream.o
	${CC} -o $@ stream.o ../lgpng/liblgpng.a

clean:
	rm -f -- *.o test-*
