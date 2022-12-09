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

typedef struct crap {
  int x;
  int y;
} crap;

//int foo;
static crap temp = {1,2};
crap* foo =&temp;


