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
#include "aseg.h"
//#include "unit.h"
//#include "system.h"
//#include "pkg.h"
#include "src.h"

#include "sym.h"
#include "pkg.h"


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


int main(int argc, char **argv){
  //meta must be first, otherwise REL_FLAG will crap out
  mseg_alloc();
  aseg_alloc();
  src_init();      // open source files and setup buffers

  sCons* pk = pk_from_libtxt("libc","libc.txt");
  pk_rebind(pk,"libc.so.6");
  srch_list_push(pk);
  aseg_dump();
 

   sym_dump1(pk_find_name(pk,"printf"));
  
  sCons* pku = pk_new("user");
  srch_list_push(pku);
  repl_loop();
  return 0;
}

