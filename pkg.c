#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"
#include "seg.h"
#include "sym.h"
#include "pkg.h"

extern sSeg* psCode;
extern sSeg* psData;
extern sSeg* psMeta;


/*----------------------------------------------------------------------------
A package is a group of related symbols, kept in a linked list. 

A special symbol is used as the head of the list; its
 next = first symbol in package;
 art  = next package (see pkgs)

Note: this is the only time the artifact pointer refers to this, meta, segment!

----------------------------------------------------------------------------*/
sSym* pk_new(char*name){
  return sym_new(name,0,0,0,0);
}
void pk_dump(sSym* pk){
  sSym*s = U32_SYM(pk->next);
  while(s){
    sym_dump1(s);
    s = U32_SYM(s->next); 
  }
}

void pk_push_sym(sSym* pk, sSym* sym){
  sym->next = pk->next;
  pk->next = (U32)(U64)sym;
}

sSym* pk_find_prev_hash(sSym* pk, U32 hash){
  sSym* prev = pk;
  sSym* s    = U32_SYM(pk->next);
  while(s){
    if(hash == s->hash) {
      return prev;
    }
    prev = s;
    s = U32_SYM(s->next);
  }
  return s;
}
sSym* pk_find_hash(sSym* pk,U32 hash){
  sSym*s = U32_SYM(pk->next);  //skip pk, next is it
  while(s){
    if(hash == s->hash) 
      return s;
    s = U32_SYM(s->next);
  }
  return s;
}

void pk_dump_protos(sSym* pk,FILE* f){
  sSym*s = U32_SYM(pk->next);  //skip pk, next is it
  char* proto;
  while(s){
    proto = sym_proto(s);
    if(*proto){
      fprintf(f,"%s\n",proto);
    }
    s = U32_SYM(s->next);
  }
}

sSym* pk_find_name(sSym*pk, char*name){
  return pk_find_hash(pk,string_hash(name));
}


sSym* pks_find_hash(sSym*pk, U32 hash,sSym**in){
  sSym* ret;
  do {
    if((ret = pk_find_hash(pk,hash))){
      if(in)
	*in = pk;
      return ret;
    }
  } while ((pk = U32_SYM(pk->art)));
  if(in)
    *in = 0;
  return 0;
}

sSym* pks_find_prev_hash(sSym*pk, U32 hash,sSym**in){
  sSym* ret;
  do {
    if((ret = pk_find_prev_hash(pk,hash))){
      if(in)
	*in = pk;
      return ret;
    }
  } while ((pk = U32_SYM(pk->art)));
  
  if(in)
    *in = 0;
  return 0;
}

sSym* pks_find_name(sSym*pk, char*name,sSym**in){
  return pks_find_hash(pk,string_hash(name),in);
}


void pks_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");
  sSym* pk = (sSym*)(U64)SRCH_LIST;
  do {
    pk_dump_protos(pk,f);
  } while ((pk = U32_SYM(pk->art)));
  fclose(f);
}



U64 find_global(char*name){
  sSym* sym = pks_find_name((sSym*)(U64)SRCH_LIST,name,0);
  if(sym)
    return sym->art;
  else
    return 0;
}

 


char* next_line(char* p){
  p = strchr(p,10);
  if(p) 
    *p++ = 0;
  return p;
}
sSym* pk_from_libtxt(char* name,char*path){
  // a 12-byte prototype of an assembly trampoline
  U8 ljump[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0 }; // jmp rax
  
  char* buf = filebuf_malloc(path);
  if(!buf) return 0;
  sSym* pk = pk_new(name);
  char* pc = buf;
  char* next;
  while((next = next_line(pc))){
    seg_align(psCode,8);
    U32 addr = (U32)(U64)seg_append(psCode,ljump,sizeof(ljump));
    sSym* sy = sym_new(pc,addr,sizeof(ljump),0,0);
    pk_push_sym(pk,sy) ;
    pc = next;
  }
  filebuf_free(buf);
  return pk;
  
}
void pk_rebind(sSym* pk,char*dllpath){
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  sSym*s = U32_SYM(pk->next);
  while(s){
     void* pfun = dlsym(dlhan,SYM_NAME(s));
    if(!pfun)
      printf("pk_rebind: could not bind to %s\n",SYM_NAME(s));
    *((void**)(U64)(s->art+2)) = pfun; // fixup to function address
    s = U32_SYM(s->next); 
  }
  dlclose(dlhan);
}

void srch_list_push(sSym* pk){
  pk->art = SRCH_LIST;  // we point at whatever srch_list did
  SRCH_LIST = (U32)(U64)pk;
}
