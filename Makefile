.SUFFIXES: .c .o
.PHONY: clean distclean install regress

include Makefile.configure

CFLAGS+= -Wpointer-sign -Wtype-limits -Wunused-function -Wconversion
CFLAGS+= -fsanitize-trap=undefined
CFLAGS+= -I. -std=c17

SRCS =  lgpng_chunks.c \
	lgpng_chunks_extra.c \
	lgpng_crc.c \
	lgpng_data.c \
	lgpng_stream.c
OBJS= ${SRCS:.c=.o}
MAN1S= pngdump.1
MANS= ${MAN1S}

REGRESS = regress/test-data \
	  regress/test-stream \
	  regress/test-pngextract.sh

all: lgpng.c liblgpng.a pngdump pngexplode pngextract pnginfo pngshuffle ${REGRESS}

regress: ${REGRESS}
	@for f in ${REGRESS} ; do \
		printf "%s" "./$${f}... " ; \
		./$$f >/dev/null 2>/dev/null || { echo "fail" ; exit 1 ; } ; \
		echo "ok" ; \
	done

compats.o: config.h

${OBJS}: lgpng.h

# Export an amalgamated C file for easy inclusion in a project
lgpng.c:
	cat ${SRCS} > lgpng.c

liblgpng.a: ${OBJS} compats.o
	${AR} rcs $@ ${OBJS} compats.o

pngdump: pngdump.o compats.o liblgpng.a
	${CC} -o $@ pngdump.o compats.o liblgpng.a -lz

pngexplode: pngexplode.o compats.o liblgpng.a
	${CC} -o $@ pngexplode.o compats.o liblgpng.a

pngextract: pngextract.o compats.o liblgpng.a
	${CC} -o $@ pngextract.o compats.o liblgpng.a

pnginfo: pnginfo.o compats.o liblgpng.a
	${CC} -o $@ pnginfo.o compats.o liblgpng.a -lz

pngshuffle: pngshuffle.o compats.o liblgpng.a
	${CC} -o $@ pngshuffle.o compats.o liblgpng.a

# Regression tests
regress/test-data: regress/test-data.c config.h lgpng.h liblgpng.a
	${CC} -o $@ regress/test-data.c compats.o liblgpng.a

regress/test-stream: regress/test-stream.c config.h lgpng.h liblgpng.a
	${CC} -o $@ regress/test-stream.c compats.o liblgpng.a

clean:
	rm -f lgpng.c
	rm -f liblgpng.a
	rm -f pngdump pngexplode pngextract pnginfo pngshuffle
	rm -f pngdump.o pngexplode.o pngextract.o pnginfo.o pngshuffle.o
	rm -f ${OBJS} compats.o tests.o
	rm -f ${REGRESS} regress/*.o

distclean: clean
	rm -f config.h config.log Makefile.configure

install: all
	mkdir -p ${DESTDIR}${INCLUDEDIR}
	mkdir -p ${DESTDIR}${MANDIR}/man1
	${INSTALL_LIB} liblgpng.a ${DESTDIR}${LIBDIR}
	${INSTALL_DATA} lgpng.h ${DESTDIR}${INCLUDEDIR}
	${INSTALL_MAN} ${MANS} ${DESTDIR}${MANDIR}/man1/
	${INSTALL_PROGRAM} pngdump ${DESTDIR}${BINDIR}
	${INSTALL_PROGRAM} pngexplode ${DESTDIR}${BINDIR}
	${INSTALL_PROGRAM} pngextract ${DESTDIR}${BINDIR}
	${INSTALL_PROGRAM} pnginfo ${DESTDIR}${BINDIR}
	${INSTALL_PROGRAM} pngshuffle ${DESTDIR}${BINDIR}
