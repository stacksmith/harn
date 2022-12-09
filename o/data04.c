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


char* a="hello";


/*

  [ 4] .rodata.str1.1    PROGBITS       
  [ 5] .data.rel.local   PROGBITS      
  [ 6] .rela.data.rel.local RELA      
 

 */


