/*
gcc -c -mmanual-endbr test10.c -O2 -aux-info sym.txt

Dump:
objdump -S test10.o

Relocations:
readelf -W -r test10.o 

Symbols:
readelf -W -s test10.o 


 */

// as simple as it gets
#include <stdio.h>
#include <string.h>

static int q = 3;

int* a=&q;


/*
 [ 2] .data             PROGBITS  0000000000000000 000040 000004 00  WA  0   0  4
 [ 4] .data.rel.local   PROGBITS  0000000000000000 000048 000008 00  WA  0   0  8
 [ 5] .rela.data.rel.local RELA   0000000000000000 000108 000018 18   I  8   4  8
  

 */


