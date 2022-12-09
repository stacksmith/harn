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

sPkg* pkgs=0;

typedef U64 (*fptr)(int,int);

void test(sPkg*pkg,char*elfname,char* funname){
  pkg_load_elf(pkg,elfname);

  //  pkg_dump(pkg);

  printf("-------------------\n");

  if(funname){
    siSymb* symb = pkgs_symb_of_name(funname);
    if(symb){
      printf("found %s\n",funname);
      fptr entry = (fptr)(U64)(symb->data);
      U64 ret = (*entry)(1,2);
      printf("returned: %lx\n",ret);
    } else {
      printf("%s not found\n",funname);
    }
  }
}


int main(int argc, char **argv){
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);
// create bindings for libc
  sPkg* pkg = pkg_new(&pkg);
  pkg_lib(pkg,"libc.so.6","libc.txt");
  pkgs_add(pkg);

  sPkg* usrpkg = pkg_new();
  pkg_set_name(usrpkg,"usr");
  pkgs_add(usrpkg);
  
  test(usrpkg,argv[1],argv[2]);
  //test_data(usrpkg,argv[1]);
  pkgs_list();
  pkg_dump(pkg);  
  //  testab("o/twoA.o","o/twoB.o");
  //testmult(argc-1,argv+1);
  
  //  printf("sizeof sDataSize is %ld\n",sizeof(sDataSize));
  // sDataSize ds = pkg_find_name(&pkg,"catopen");
  //printf("found: %08x %d\n",ds.data, ds.size);

  return 0;
}
