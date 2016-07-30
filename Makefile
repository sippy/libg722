# $Id: Makefile,v 1.1 2012/08/07 11:33:45 sobomax Exp $

LIB=	g722
SHLIB_MAJOR=	0
PREFIX?= /usr/local
LIBDIR= ${PREFIX}/lib
NO_PROFILE=	YES
INCLUDEDIR= ${PREFIX}/include
MAN=
SRCS=	g722_decode.c g722_encode.c
INCS=	g722.h g722_private.h g722_encoder.h g722_decoder.h
WARNS?=	2
CFLAGS+= -I${.CURDIR} ${PICFLAG}

VERSION_DEF=	${.CURDIR}/ld_sugar/Versions.def
SYMBOL_MAPS=	${.CURDIR}/ld_sugar/Symbol.map
CFLAGS+=	-DSYMBOL_VERSIONING

TEST_OUT_FILES=	test.raw test.raw.16k pcminb.g722 pcminb.raw.16k \
    test.g722.out

CLEANFILES+=test ${TEST_OUT_FILES}

test: test.c lib${LIB}.a lib${LIB}.so.${SHLIB_MAJOR} test.g722 pcminb.dat Makefile
	rm -f ${TEST_OUT_FILES}
	${CC} ${CFLAGS} -o ${.TARGET} test.c -lm -L. -l${LIB}
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/${.TARGET} test.g722 test.raw
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/${.TARGET} --sln16k test.g722 \
	    test.raw.16k
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/${.TARGET} --enc --sln16k --bend \
	    pcminb.dat pcminb.g722
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/${.TARGET} --sln16k --bend \
	    pcminb.g722 pcminb.raw.16k
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/${.TARGET} --enc test.raw \
	    test.g722.out
	sha256 ${TEST_OUT_FILES} | \
	    diff test.checksum -

.include <bsd.lib.mk>
