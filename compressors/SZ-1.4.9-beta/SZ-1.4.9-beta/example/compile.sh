#!/bin/sh

gcc -lm -I../sz/include -I../zlib -O3   -o testdouble_compress_ee testdouble_compress_ee.c ../sz/.libs/libsz.a ../zlib/.libs/libzlib.a -lm

