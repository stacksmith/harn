/* Load a single elf object file and fixup */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
 
#include "global.h"
#include "util.h"
#include "elf.h"
#include "elfdump.h" 
#include "seg.h"
#include "unit.h"
#include "lib.h"
#include "system.h"




sSeg scode;
sSeg sdata;
sSystem sys;

typedef U64 (*fptr)(int,int);

void test2(char*name){
  sUnit* pu = sys_load_elf(name);

  //  unit_dump(puLib);

  //  sys_dump();
  //unit_dump(pu);
  
  printf("-------------------\n");

  fptr entry = (fptr)sys_symbol_address("bar");
  printf("found %p\n",entry);
  if(entry){
    //    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(1,2);
    printf("returned: %lx\n",ret);
  }
}
/*
void testab(char*name1,char*name2){
  sys_load_two(name1,name2);
  fptr entry = (fptr)sys_symbol_address("funA");
  printf("found %p\n",entry);
  if(entry){
    //    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(5,0);
    printf("returned: %ld\n",ret);
  }
}
*/
/*
void testmult(U32 cnt,char**paths){
  sys_load_mult(cnt,paths);

  //  U64 q = sys_symbol_address("str1");
  //  printf("found q: %lX\n",q);

  seg_dump(&scode);
  seg_dump(&sdata);
  fptr entry = (fptr)sys_symbol_address("bar");
  printf("found bar at %p\n",entry);
  if(entry){
    //    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(5,0);
    printf("returned: %ld\n",ret);
  }

  //  unit_dump(sys.units[1]);
  //sys_dump();
  //hd(0x40000000,8);
}
*/
int main(int argc, char **argv){
  sys_init();
  
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  // create bindings for libc
  sys_add(lib_make("libc.so.6","libc.txt"));
 
  test2(argv[1]);
  //  testab("o/twoA.o","o/twoB.o");
  //testmult(argc-1,argv+1);
  return 0;
}
