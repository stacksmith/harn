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
  char* p;
  int* pint;
} crap;
static int j; // .bss
//int foo;
static crap temp = {1,"hello",&j}; //.data.rel.local, .rodata.str1.1
crap* foo =&temp;


