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
#include <stdio.h>   // headers for stdlib..
#include <string.h>  // add more if you want.

#include "types.h"   // include your favorite types.  Edit this.
#include "headers.h" // include automatically-generated headers.  Do not edit!
#include "body.c"    // the system will insert the body here.  Do not edit.




