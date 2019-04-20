SZPATH=/home/jonccal/research/compressors/SZ-1.4.11.0/sz
SZFLAG="-I$SZPATH/include $SZPATH/.libs/libSZ.a $SZPATH/..//zlib/.libs/libzlib.a "

#echo rm bitflip_test 
if [ -f bitflip_test.out ]; then
	rm bitflip_test.out
fi
echo gcc -lm -g -o bitflip_test.out bitflip_test.c ${SZFLAG}
gcc -lm -g -o bitflip_test.out bitflip_test.c $SZFLAG

echo ""
echo gcc -lm -g -o bitflip_decompress.out bitflip_decompress.c ${SZFLAG}
gcc -lm -g -o bitflip_decompress.out bitflip_decompress.c $SZFLAG
