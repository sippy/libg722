/*
 * Copyright (c) 2014-2016 Sippy Software, Inc., http://www.sippysoft.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#if defined(_WIN32)
#include <stdlib.h>
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#elif defined(__APPLE__)
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#else
#include <endian.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g722_encoder.h"
#include "g722_decoder.h"

/* Define byte order conversion functions for macOS */
#if defined(__APPLE__)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#elif defined(_WIN32)
#define g722_bswap16(x) ((uint16_t) ((((uint16_t) (x) & 0x00ff) << 8) | (((uint16_t) (x) & 0xff00) >> 8)))
#define htole16(x) (x)
#define le16toh(x) (x)
#define htobe16(x) ((int16_t) g722_bswap16(x))
#define be16toh(x) ((int16_t) g722_bswap16(x))
#endif

#define BUFFER_SIZE 10

static void
usage(const char *argv0)
{

    fprintf(stderr, "usage: %s [--sln16k] [--bend] file.g722 file.raw\n"
      "       %s --encode [--sln16k] [--bend] file.raw file.g722\n", argv0,
      argv0);
    exit (1);
}

static int
parse_options(int argc, char **argv, int *srate, int *oblen, int *enc, int *bend)
{
    int argi;

    *srate = G722_SAMPLE_RATE_8000;
    *oblen = 1;
    *enc = 0;
    *bend = 0;

    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--sln16k") == 0) {
            *srate &= ~G722_SAMPLE_RATE_8000;
            *oblen = 2;
        } else if (strcmp(argv[argi], "--encode") == 0 || strcmp(argv[argi], "--enc") == 0) {
            *enc = 1;
        } else if (strcmp(argv[argi], "--bend") == 0) {
            *bend = 1;
        } else if (strncmp(argv[argi], "--", 2) == 0) {
            usage(argv[0]);
        } else {
            break;
        }
    }

    return argi;
}

int
main(int argc, char **argv)
{
    FILE *fi, *fo;
    uint8_t ibuf[BUFFER_SIZE];
    int16_t obuf[BUFFER_SIZE * 2];
    G722_DEC_CTX *g722_dctx;
    G722_ENC_CTX *g722_ectx;
    int i, srate, enc, bend;
    int oblen;
    int first_arg;

    first_arg = parse_options(argc, argv, &srate, &oblen, &enc, &bend);

    if (argc - first_arg != 2) {
        usage(argv[0]);
    }

    fi = fopen(argv[first_arg], "rb");
    if (fi == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[first_arg]);
        exit (1);
    }
    fo = fopen(argv[first_arg + 1], "wb");
    if (fo == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[first_arg + 1]);
        exit (1);
    }

    int ib;
    if (enc == 0) {
        g722_dctx = g722_decoder_new(64000, srate);
        if (g722_dctx == NULL) {
            fprintf(stderr, "g722_decoder_new() failed\n");
            exit (1);
        }
        while ((ib=fread(ibuf, 1, sizeof(ibuf), fi)) >= 1) {
            g722_decode(g722_dctx, ibuf, ib, obuf);
            for (i = 0; i < (ib * oblen); i++) {
                if (bend == 0) {
                    obuf[i] = htole16(obuf[i]);
                } else {
                    obuf[i] = htobe16(obuf[i]);
                }
            }
            fwrite(obuf, ib * oblen * sizeof(obuf[0]), 1, fo);
            fflush(fo);
        }
    } else {
        g722_ectx = g722_encoder_new(64000, srate);
        if (g722_ectx == NULL) {
            fprintf(stderr, "g722_encoder_new() failed\n");
            exit (1);
        }
        int insize = sizeof(obuf) / ((oblen == 1) ? 2 : 1);
        while ((ib=fread(obuf, 1, insize, fi)) >= 1) {
            int ibnelem = ib / sizeof(obuf[0]);
            for (i = 0; i < ibnelem; i++) {
                if (bend == 0) {
                    obuf[i] = le16toh(obuf[i]);
                } else {
                    obuf[i] = be16toh(obuf[i]);
                }
            }
            g722_encode(g722_ectx, obuf, ibnelem, ibuf);
            fwrite(ibuf, ibnelem / oblen, 1, fo);
            fflush(fo);
        }
    }

    fclose(fi);
    fclose(fo);

    exit(0);
}
