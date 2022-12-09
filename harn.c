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



sSeg scode;
sSeg sdata;
//sSystem sys;

sPkg pkg;

sPkg* pkgs=0;

FILE* faSources;
FILE* fSources;

typedef U64 (*fptr)(int,int);
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
void run_void_fun(char* funname){
  siSymb* symb = pkgs_symb_of_name(funname);
  if(symb){
    //    printf("found %s\n",funname);
    fptr entry = (fptr)(U64)(symb->data);
    U64 ret = (*entry)(1,2);
    printf("returned: %lx\n",ret);
  } else {
    printf("%s not found\n",funname);
  }
}

void import_elf(){

}

extern FILE* fSources;
void edit(char* name){
  printf("Editing [%s]\n",name);
  siSymb* symb =  pkgs_symb_of_name(name);
  // write headers for packages in use
  FILE*f = fopen("sys/headers.h","w");
  pkgs_dump_protos(f);
  fclose(f);

  siSymb_src_to_body(symb);
 

}

char buf[1024];
void main_loop(){
  while(1) {
    printf("> ");
    fgets(buf,1024,stdin);
    // try to find it
    size_t len = strlen(buf);
    len--;
    *(buf+len)=0; // get rid of newline
    if(!strncmp("xx",buf,2)){
      //siSymb* symb =
      pkg_load_elf(pkgs,"sys/test.o");  
      continue;
    }
    if(!strncmp("sys",buf,3)){
      seg_dump(&scode);
      seg_dump(&sdata);
      pkgs_list();
      continue;
    }
    if(!strncmp("cc",buf,2)){
      int ret = system("cd sys; ./build.sh");
      if(!ret){
	siSymb* symb = pkg_load_elf(pkgs,"sys/test.o");
	if(symb){
	  printf("%s: %s %d bytes\n",symb->name,symb->proto,symb->size);
	  //TODO: make up your mind! before or after?
	  FILE* f = fopen("sys/headers.h","w");
	  pkgs_dump_protos(f);
	  fclose(f);
	} else {
	  printf("Compile abandoned\n");
	}
      }
      else
	printf("--- %d\n",ret);
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

   
    
    if(!strncmp("bye",buf,3))
      exit(0);

    run_void_fun(buf);

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
