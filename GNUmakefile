.SUFFIXES: .So
VPATH = .

LIBDIR= ${PREFIX}/lib
INCLUDEDIR= ${PREFIX}/include

SRCS_C= g722_decode.c g722_encode.c
SRCS_H= g722.h g722_private.h g722_encoder.h g722_decoder.h

CFLAGS?= -O2 -pipe -Wno-attributes

OBJS = $(SRCS_C:.c=.o)
OBJS_PIC = $(SRCS_C:.c=.So)

OS_NAME:=	$(shell uname -s)
ifeq ($(OS_NAME),Darwin)
    SONAME=	-install_name
    PREFIX?=	$(HOME)/Library/libg722
else
    SONAME=	-soname
    PREFIX?=	/usr/local
endif

all: libg722.a libg722.so.0 libg722.so

libg722.a: $(OBJS) $(SRCS_H)
	$(AR) cq $@ $(OBJS)
	ranlib $@

libg722.so.0: $(OBJS_PIC) $(SRCS_H)
	$(CC) -shared -o $@ -Wl,${SONAME},$@ $(OBJS_PIC)

libg722.so: libg722.so.0
	ln -sf libg722.so.0 $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.c.So:
	$(CC) -fpic -DPIC -c $(CFLAGS) $< -o $@

clean:
	rm -f libg722.a libg722.so.0 $(OBJS) $(OBJS_PIC) test *.out

test: test.c libg722.a libg722.so.0
	${CC} ${CFLAGS} -o $@ test.c -lm -L. -lg722
	LD_LIBRARY_PATH=. ./scripts/do-test.sh ./$@

install:
	install -d ${DESTDIR}${LIBDIR}
	install libg722.a ${DESTDIR}${LIBDIR}
	install libg722.so.0 ${DESTDIR}${LIBDIR}
	ln -sf libg722.so.0 ${DESTDIR}${LIBDIR}/libg722.so
	install -d ${DESTDIR}${INCLUDEDIR}
	install ${SRCS_H} ${DESTDIR}${INCLUDEDIR}
