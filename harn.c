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
//#include "unit.h"
//#include "system.h"
#include "pkg.h"



sSeg scode;
sSeg sdata;
//sSystem sys;

sPkg pkg;

typedef U64 (*fptr)(int,int);

void test2(char*name){
  pkg_load_elf(&pkg,name);
  //  unit_dump(puLib);

  //  sys_dump();
  //unit_dump(pu);

  printf("-------------------\n");
 
  fptr entry = (fptr)(U64)(pkg_find_name(&pkg,"bar").data);
  printf("found %p\n",entry);
  if(entry){
    //    fptr entry = (fptr)(U64)(pu->dats[i].off);
    U64 ret = (*entry)(1,2);
    printf("returned: %lx\n",ret);
  }
  
}
void test_data(char*name){
  pkg_load_elf(&pkg,name);
  //  unit_dump(puLib);

  //  sys_dump();
  //unit_dump(pu);
  // pkg_dump(&pkg);  
  printf("-------------------\n");
  
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
  //  sys_init();
  pkg_init(&pkg,"test");
  
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);

  // create bindings for libc
  pkg_lib(&pkg,"libc.so.6","libc.txt");
  
  //  test2(argv[1]);
  test_data(argv[1]);
  hd((U8*)(U64)0x80000AB0,4);
  //   pkg_dump(&pkg);  
  //  testab("o/twoA.o","o/twoB.o");
  //testmult(argc-1,argv+1);
  
  //  printf("sizeof sDataSize is %ld\n",sizeof(sDataSize));
  // sDataSize ds = pkg_find_name(&pkg,"catopen");
  //printf("found: %08x %d\n",ds.data, ds.size);

  return 0;
}
