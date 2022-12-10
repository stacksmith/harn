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
#include "src.h"
#include "pkgs.h"



sSeg scode;
sSeg sdata;
//sSystem sys;

sPkg pkg;

sPkg* pkgs=0;

FILE* faSources;
FILE* fSources;

typedef void (*fpvoid)();
typedef U64 (*fptr)(int,int);
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
*/
void run_void_fun(siSymb* symb){
  // symb is known to be a symb
  U32 addr = symb->data;
  if(IN_CODE_SEG(addr)){
    //    printf("found %s\n",funname);
    fpvoid entry = (fpvoid)(U64)(addr);
    (*entry)();
  } else {
    printf("can't run data\n");
  }
}





void edit(char* name){
  printf("Editing [%s].. type 'cc' to compile when done...\n",name);
  siSymb* symb =  pkgs_symb_of_name(name);
  // write headers for packages in use
  FILE*f = fopen("sys/headers.h","w");
  pkgs_dump_protos(f);
  fclose(f);

  siSymb_src_to_body(symb);
 

}


siSymb* compile(){
  siSymb* symb=0;
  int ret = system("cd sys; ./build.sh");
  if(!ret){ // build shell-out successful:
    // create an elf object
    sElf* pelf = elf_load("sys/test.o"); 
    Elf64_Sym* psym = elf_unique_global_symbol(pelf);
    if(psym){ // We identified the global symbol in the ELF file.
      // do we already have a symbol with same name?  Hold onto it.
      siSymb* oldsymb = pkgs_symb_of_name(ELF_SYM_NAME(pelf,psym));
      // load the entire ELF trainwreck, and add symb to our
      symb = pkg_load_elf(pkgs,pelf,psym); // topmost pkg
      if(symb){ // OK, this means we are done with ingestion.
	printf("Ingested %s: %s %d bytes\n",
	       symb->name,symb->proto,symb->size);
	// Now that the new object is in our segment, is there an 
	if(oldsymb) { // older version we are replacing?
	  printf("old version exists\n");
	  // first, make ssure it's in the same seg.
	  if( (SEG_BITS(oldsymb->data)) == (SEG_BITS(symb->data))){
	    // yes, both are in the same seg.  Fix old refs
	    seg_reref(&scode,oldsymb->data,symb->data); // in code seg
	    seg_reref(&sdata,oldsymb->data,symb->data); // in data seg
	  } else { // No, different segs.  Nothing good can come of it.
	    fprintf(stderr,"New one is in a different seg! Abandoning!\n");
	    pkg_drop_symb(pkgs); // drop new object from topmost pkg
	  }
	} // else a new symbol, no problem
      } else {
	printf("ELF not ingested due to unresolved ELF symbols.\n");
      }
    } else {
      printf("One and only one ELF global symbol must exist.  Abandoning\n");
    }
    // finally, delete the elf data
    elf_delete(pelf);
  }else { 
    printf("OS shellout to compiler failed! Build Error %d\n",ret);
  }
  return symb;
}

char buf[1024];
void main_loop(){
  pkgs_dump_protos();
  while(1) {
    printf("> ");
    fgets(buf,1024,stdin);
    // try to find it
    size_t len = strlen(buf);
    len--;
    *(buf+len)=0; // get rid of newline
    
    // expression to execute?
    if(*buf=='.'){
      FILE*f = fopen("sys/body.c","w");		\
      fputs("void command_line(void){\n ",f);
      fputs(buf+1,f);
      fputs("\n}\n",f);
      fclose(f);
      siSymb* symb = compile();
      if(symb)
	run_void_fun(symb);
      pkg_drop_symb(pkgs);
      continue;
    }
    /*
      if(!strncmp("xx",buf,2)){
      //siSymb* symb =
      pkg_load_elf(pkgs,"sys/test.o");  
      continue;
      }
    */
    if(!strncmp("sys",buf,3)){
      seg_dump(&scode);
      seg_dump(&sdata);
      pkgs_list();
      continue;
    }
    
    if(!strncmp("cc",buf,2)){
      compile();
      pkgs_dump_protos();
      continue;
    }
    if(!strncmp("hh",buf,2)){
      FILE*f = fopen("sys/headers.h","w");
      pkgs_dump_protos(f);
      fclose(f);
      continue;
    }
    
    if(!strncmp("words",buf,4)){
      pkg_words(pkgs);
      continue;
    }
    
    if(!strncmp("edit",buf,4)){
      edit(buf+5);
      pkgs_dump_protos(stdout);
      continue;
    }
    
    if(!strncmp("list",buf,4)){
      siSymb* symb = pkgs_symb_of_name(buf+5);
      if(symb){
	src_to_file(symb->src,symb->srclen,stdout);
      }
      continue;
    }
    if(!strncmp("bye",buf,3))
      exit(0);
    // user typed in the name of a function to run as void
    siSymb* symb = pkgs_symb_of_name(buf);
    if(symb){
      run_void_fun(symb);
    } else {
      printf("%s not found\n",buf);
    }
  }
}


int main(int argc, char **argv){
  seg_alloc(&scode,"SCODE",0x10000000,(void*)0x80000000,
	    PROT_READ|PROT_WRITE|PROT_EXEC);
  seg_alloc(&sdata,"SDATA",0x10000000,(void*)0x40000000,
	    PROT_READ|PROT_WRITE);
  src_init();
  

   // create bindings for libc
  sPkg* pkg = pkg_new(&pkg);
  pkg_lib(pkg,"libc.so.6","libc.txt");
  pkgs_add(pkg);

  sPkg* usrpkg = pkg_new();
  pkg_set_name(usrpkg,"usr");
  pkgs_add(usrpkg);


  //test(usrpkg,argv[1],argv[2]);
  //test_data(usrpkg,argv[1]);
  //  pkgs_list();
  //pkg_dump(pkg);

  main_loop();
  //  testab("o/twoA.o","o/twoB.o");
  //testmult(argc-1,argv+1);
  
  //  printf("sizeof sDataSize is %ld\n",sizeof(sDataSize));
  // sDataSize ds = pkg_find_name(&pkg,"catopen");
  //printf("found: %08x %d\n",ds.data, ds.size);

  return 0;
}
