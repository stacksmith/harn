// symtab.
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy



#include "global.h"
#include "util.h"
#include "pkg.h"
#include "pkgs.h"



/*******************************************************************************
PACKAGES subsystem.

We keep a linked list of packages.

*******************************************************************************/
extern sPkg* pkgs;
void pkgs_add(sPkg* pkg){
  if(pkg->next){
    printf("pkgs_add: pkg is not free!\n");
    exit(1);
  }
  pkg->next = pkgs;
  pkgs = pkg;
}

void pkgs_list(){
  sPkg* p = pkgs;
  printf("Packages:");
  while(p){
    printf(" %s",p->name);
    p=p->next;
  }
  printf("\n");
}


siSymb* pkgs_symb_of_name(char* name){
  sPkg* pkg = pkgs;
  U32 hash = string_hash(name);
  while(pkg){
    siSymb* symb = pkg_symb_of_hash(pkg,hash);
    if(symb)
      return symb;
    pkg=pkg->next;
  }
  return 0;
}


void pkgs_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");
  sPkg* pkg = pkgs;
  while(pkg){
    pkg_dump_protos(pkg,f);
    pkg = pkg->next;
  }
  fclose(f);
}
