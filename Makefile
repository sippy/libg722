# $Id: Makefile,v 1.1 2012/08/07 11:33:45 sobomax Exp $

LIB=	g722
SHLIB_MAJOR=	0
PREFIX?= /usr/local
LIBDIR= ${PREFIX}/lib
MK_PROFILE=	no
INCLUDEDIR= ${PREFIX}/include
MAN=
SRCS=	g722_decode.c g722_encode.c
INCS=	g722.h g722_private.h g722_encoder.h g722_decoder.h
WARNS?=	2
CFLAGS+= -I${.CURDIR} ${PICFLAG}

VERSION_DEF=	${.CURDIR}/ld_sugar/Versions.def
SYMBOL_MAPS=	${.CURDIR}/ld_sugar/Symbol.map
CFLAGS+=	-DSYMBOL_VERSIONING

CLEANFILES+=test *.out

TDDIR=	${.CURDIR}/test_data

test: test.c lib${LIB}.a lib${LIB}.so.${SHLIB_MAJOR} ${TDDIR}/fullscale.g722 ${TDDIR}/pcminb.dat ${TDDIR}/test.checksum ${TDDIR}/test.g722 Makefile
	rm -f ${TEST_OUT_FILES}
	${CC} ${CFLAGS} -o ${.TARGET} test.c -lm -L. -l${LIB}
	LD_LIBRARY_PATH=${.CURDIR} ${.CURDIR}/scripts/do-test.sh ${.CURDIR}/${.TARGET}

.include <bsd.lib.mk>
