# $Id: Makefile,v 1.1 2012/08/07 11:33:45 sobomax Exp $

LIB=	g722
PREFIX?= /usr/local
LIBDIR= ${PREFIX}/lib
NO_SHARED=	YES
NO_PROFILE=	YES
INCLUDEDIR= ${PREFIX}/include
MAN=
SRCS=	g722_decode.c g722_encode.c
INCS=	g722.h g722_private.h g722_encoder.h g722_decoder.h
WARNS?=	2
CFLAGS+= -I${.CURDIR} ${PICFLAG}

TEST_OUT_FILES=	test.raw test.raw.16k pcminb.g722 pcminb.raw.16k \
    test.g722.out

CLEANFILES+=test ${TEST_OUT_FILES}

test: test.c lib${LIB}.a test.g722 pcminb.dat Makefile
	rm -f ${TEST_OUT_FILES}
	${CC} ${CFLAGS} -o ${.TARGET} test.c lib${LIB}.a -lm
	${.CURDIR}/${.TARGET} test.g722 test.raw
	${.CURDIR}/${.TARGET} --sln16k test.g722 test.raw.16k
	${.CURDIR}/${.TARGET} --enc --sln16k --bend pcminb.dat pcminb.g722
	${.CURDIR}/${.TARGET} --sln16k --bend pcminb.g722 pcminb.raw.16k
	${.CURDIR}/${.TARGET} --enc test.raw test.g722.out
	sha256 ${TEST_OUT_FILES} | \
	    diff test.checksum -

.include <bsd.lib.mk>
