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

#include <sys/endian.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "g722_encoder.h"
#include "g722_decoder.h"

#define BUFFER_SIZE 10

static void
usage(const char *argv0)
{

    fprintf(stderr, "usage: %s [--sln16k] [--bend] file.g722 file.raw\n"
      "       %s --encode [--sln16k] [--bend] file.raw file.g722\n", argv0,
      argv0);
    exit (1);
}

int
main(int argc, char **argv)
{
    FILE *fi, *fo;
    uint8_t ibuf[BUFFER_SIZE];
    int16_t obuf[BUFFER_SIZE * 2];
    G722_DEC_CTX *g722_dctx;
    G722_ENC_CTX *g722_ectx;
    int i, srate, ch, enc, bend;
    size_t oblen;

    /* options descriptor */
    static struct option longopts[] = {
        { "sln16k", no_argument, NULL, 256 },
        { "encode", no_argument, NULL, 257 },
        { "bend",   no_argument, NULL, 258 },
        { NULL,     0,           NULL,  0  }
   };

   srate = G722_SAMPLE_RATE_8000;
   oblen = sizeof(obuf) / 2;
   enc = bend = 0;
   while ((ch = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
       switch (ch) {
       case 256:
           srate &= ~G722_SAMPLE_RATE_8000;
           oblen *= 2;
           break;
       case 257:
           enc = 1;
           break;
       case 258:
           bend = 1;
           break;
       default:
           usage(argv[0]);
       }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2) {
        usage(argv[-optind]);
    }

    fi = fopen(argv[0], "r");
    if (fi == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[0]);
        exit (1);
    }
    fo = fopen(argv[1], "w");
    if (fo == NULL) {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        exit (1);
    }

    if (enc == 0) {
        g722_dctx = g722_decoder_new(64000, srate);
        if (g722_dctx == NULL) {
            fprintf(stderr, "g722_decoder_new() failed\n");
            exit (1);
        }

        while (fread(ibuf, sizeof(ibuf), 1, fi) == 1) {
            g722_decode(g722_dctx, ibuf, sizeof(ibuf), obuf);
            for (i = 0; i < (oblen / sizeof(obuf[0])); i++) {
                if (bend == 0) {
                    obuf[i] = htole16(obuf[i]);
                } else {
                    obuf[i] = htobe16(obuf[i]);
                }
            }
            fwrite(obuf, oblen, 1, fo);
            fflush(fo);
        }
    } else {
        g722_ectx = g722_encoder_new(64000, srate);
        if (g722_ectx == NULL) {
            fprintf(stderr, "g722_encoder_new() failed\n");
            exit (1);
        }
        while (fread(obuf, oblen, 1, fi) == 1) {
            for (i = 0; i < (oblen / sizeof(obuf[0])); i++) {
                if (bend == 0) {
                    obuf[i] = le16toh(obuf[i]);
                } else {
                    obuf[i] = be16toh(obuf[i]);
                }
            }
            g722_encode(g722_ectx, obuf, (oblen / sizeof(obuf[0])), ibuf);
            fwrite(ibuf, sizeof(ibuf), 1, fo);
            fflush(fo);
        }
    }

    fclose(fi);
    fclose(fo);

    exit(0);
}
