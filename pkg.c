// symtab.
#include <stdio.h>
#include <stdlib.h> //malloc
#include <string.h> //memcpy
#include <elf.h>
#include <dlfcn.h>

#include "global.h"
#include "elf.h"
#include "elfdump.h"
#include "util.h"
#include "seg.h"
#include "pkg.h"

extern sSeg scode;
extern sSeg sdata;



siSymb* siSymb_new(char* name,U32 data,U32 size){
  siSymb* p = (siSymb*)malloc(sizeof(siSymb));
  size_t lname = strlen(name);
  p->name = (char*)malloc(lname+1);
  strcpy(p->name,name);
  p->hash = string_hash(name);
  p->data = data;
  p->size = size;
  return p;
}

void siSymb_dump(siSymb* p){
  printf("%08X %08x %s\n",p->data, p->size, p->name);
}

void pkg_dump(sPkg* pkg){
  siSymb* p = pkg->data;
  while(p){
    siSymb_dump(p);
    p = p->next;
  }
}

void pkg_init(sPkg* pkg,char* name){
  pkg->data = 0;
}

void pkg_add(sPkg* pkg,char*name,U32 data,U32 size){
  siSymb*p = siSymb_new(name,data,size);
  p->next = pkg->data;
  pkg->data = p;
}

static sDataSize nullds = {0,0};

sDataSize pkg_find_hash(sPkg* pkg,U32 hash){
  siSymb* p = pkg->data;
  while(p){
    if(p->hash == hash)
      return p->ds;
    p = p->next;
  }
  return nullds;
}
sDataSize pkg_find_name(sPkg* pkg,char* name){
  return pkg_find_hash(pkg,string_hash(name));
}

