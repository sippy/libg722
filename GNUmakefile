.SUFFIXES: .So
VPATH = .

PREFIX?= /usr/local
LIBDIR= ${PREFIX}/lib
INCLUDEDIR= ${PREFIX}/include

SRCS_C= g722_decode.c g722_encode.c
SRCS_H= g722.h g722_private.h g722_encoder.h g722_decoder.h

CFLAGS?= -O2 -pipe

OBJS = $(SRCS_C:.c=.o)
OBJS_PIC = $(SRCS_C:.c=.So)

TEST_OUT_FILES= test.raw test.raw.16k pcminb.g722 pcminb.raw.16k \
    test.g722.out

all: libg722.a libg722.so.0 libg722.so

libg722.a: $(OBJS) $(SRCS_H)
	$(AR) cq $@ $(OBJS)
	ranlib $@

libg722.so.0: $(OBJS_PIC) $(SRCS_H)
	$(CC) -shared -o $@ -Wl,-soname,$@ $(OBJS_PIC)

libg722.so: libg722.so.0
	ln -sf libg722.so.0 $@

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.c.So:
	$(CC) -fpic -DPIC -c $(CFLAGS) $< -o $@

clean:
	rm -f libg722.a libg722.so.0 $(OBJS) $(OBJS_PIC) test $(TEST_OUT_FILES)

test: test.c libg722.a libg722.so.0
	${CC} ${CFLAGS} -o $@ test.c -lm -L. -lg722
	LD_LIBRARY_PATH=. ./$@ test.g722 test.raw
	LD_LIBRARY_PATH=. ./$@ --sln16k test.g722 \
	    test.raw.16k
	LD_LIBRARY_PATH=. ./$@ --enc --sln16k --bend \
	    pcminb.dat pcminb.g722
	LD_LIBRARY_PATH=. ./$@ --sln16k --bend \
	    pcminb.g722 pcminb.raw.16k
	LD_LIBRARY_PATH=. ./$@ --enc test.raw \
	    test.g722.out
	sha256 ${TEST_OUT_FILES} | \
	    diff test.checksum -

install:
	install -d ${DESTDIR}${LIBDIR}
	install libg722.a ${DESTDIR}${LIBDIR}
	install libg722.so.0 ${DESTDIR}${LIBDIR}
	ln -sf libg722.so.0 ${DESTDIR}${LIBDIR}/libg722.so
	install -d ${DESTDIR}${INCLUDEDIR}
	install ${SRCS_H} ${DESTDIR}${INCLUDEDIR}
