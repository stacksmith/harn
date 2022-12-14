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
//#include "pkg.h"
#include "src.h"

#include "sym.h"
#include "pkg.h"


sSeg* psCode;
sSeg* psData;
sSeg* psMeta;


//sPkg* pkgs=0;


FILE* faSources;
FILE* fSources;

/*
void test(sPkg*pkg,char*elfname,char* funname){
  siSymb* symb = pkg_load_elf(pkg,elfname);

  //  pkg_dump(pkg);


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
C0000000 - D0000000    meta
                              90000000 - A0000000    meta
80000000 - 90000000    code
40000000 - 50000000    data
18000000   1A000000    meta rel
                                   12000000 - 14000000    meta rel
10000000 - 12000000    code rel
08000000 - 0A000000    data rel
*/

void repl_loop();


sSym* ing_elf(sElf* pelf);

int main(int argc, char **argv){
  //meta must be first, otherwise REL_FLAG will crap out
  psMeta = seg_alloc(0x10000000,(void*)SMETA_BASE,PROT_READ|PROT_WRITE);
  psCode = seg_alloc(0x10000000,(void*)0x80000000,
		    PROT_READ|PROT_WRITE|PROT_EXEC);
  psData = seg_alloc(0x10000000,(void*)0x40000000,PROT_READ|PROT_WRITE);


  src_init();      // open source files and setup buffers
  
  {
    seg_reset(psMeta);
    psMeta->fill+=8; // SRCH_LIST at +8, 4 bytes
    REL_FLAG = 1;    // REL_FLAG  at +C, 4 bytes
    seg_rel_mark(psMeta, psMeta->fill-16 ,3); // mark the fill 
    SRCH_LIST = 0;
    seg_rel_mark(psMeta, psMeta->fill-8 ,3); // mark srch_list

    seg_reset(psCode);
    seg_reset(psData);


    sSym* pk = pk_from_libtxt("libc","libc.txt");
    pk_rebind(pk,"libc.so.6");
    srch_list_push(pk);
    
    sSym* pku = pk_new("user");
    srch_list_push(pku);
  }
  //sElf* pelf = elf_load("sys/test.o"); 
  ///sSym* sy = ing_elf(pelf);
  //printf("ingested... %p\n",sy);
  //typedef U64 (*fpvoidfun)();
  //fpvoidfun fun = (fpvoidfun)(U64)sy->data;
  //(*fun)();
  //  printf("%lx \n",pks_find_global("printf"));
  /*
  
  {
    sSym* pk = pk_new("libc.so");
    siSymb* ps = pkg->data;
    while(ps){
      sSym* s = sym_from_siSymb((void*)ps);
      pk_add_sym(pk,s);
      ps = ps->next;
    }
    //    sym_dump1(pk_find_name(pk,"printf"));
    pk_dump(pk);


  }
  */
  //  sPkg* usrpkg = pkg_new();
  //pkg_set_name(usrpkg,"usr");
  //pkgs_add(usrpkg);


  //test(usrpkg,argv[1],argv[2]);
  //test_data(usrpkg,argv[1]);
  //  pkgs_list();
  //pkg_dump(pkg);
  repl_loop();
  //  testab("o/twoA.o","o/twoB.o");
  //testmult(argc-1,argv+1);
  
  //  printf("sizeof sDataSize is %ld\n",sizeof(sDataSize));
  // sDataSize ds = pkg_find_name(&pkg,"catopen");
  //printf("found: %08x %d\n",ds.data, ds.size);

  return 0;
}
