#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"

#include "seg.h"
#include "aseg.h"
#include "sym.h"
#include "pkg.h"

/*----------------------------------------------------------------------------
A package is a group of related symbols, kept in a linked list. 

The prototype area of a package may be used for private storage.  
library bindings use it to store the name of the dll to open

This is a fourth try to streamline packages using the new package walker...
----------------------------------------------------------------------------*/
sSym* pk_new(char*name,char* opt){
  return sym_new(name,0,0,0,opt);
}

void pk_dump(sSym*s){ 
  pkg_walk(s,0,sym_dump1);
}

void pk_push_sym(sSym* pk, sSym* sym){
  sym->cdr = pk->cdr;
  pk->cdr = THE_U32(sym);
}

U32 pk_find_prev_hash_proc(sSym* s,U32 hash, U32 prev){
  return (s->hash == hash) ?  prev : 0;
}

sSym* pk_find_prev_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pk_find_prev_hash_proc);
}
// note: pkgs_walk uses this too!
sSym* pk_find_hash_proc(sSym* s,U32 hash,sSym*prev){
  return (s->hash == hash) ? s : 0;
}

sSym* pk_find_hash(sSym* s,U32 hash){
  return pkg_walk_U32(s,hash,pk_find_hash_proc);
}

U32 pk_dump_protos_proc(sSym* s,FILE* f){
    char* proto = sym_proto(s);
    if(*proto)
      fprintf(f,"%s\n",proto);
    return 0;
  }

void pk_dump_protos(sSym* s,FILE* f){
  pkg_walk(s, (U64)(f), pk_dump_protos_proc);
}


sSym* pk_find_name(sSym*s, char*name){
  return pk_find_hash(s,string_hash(name));
}
/*
sSym* pk_find_name1(sCons*pk, char*name){
  return pk_find_hash(pk,string_hash1(pk,name));
}

sSym* pks_find_hash(sCons*pk, U32 hash,sCons**in){
  sSym* ret;
  do {
    if((ret = pk_find_hash(pk,hash)))
      break;
    // ow, ret is 0!
  } while ( (pk = PTR(sCons*,pk->car)) );
  //either good, or both pk and ret are 0.
  if(in) *in = pk;
  return ret;
}
sSym* pks_find_name(sCons*pk, char*name,sCons**in){
   U32 hash = string_hash(name);
   return pks_find_hash(pk,hash,in);
   
sSym* pks_find_hash0(sCons*pk, U32 hash){
  sSym* ret;
  do {
    if((ret = pk_find_hash(pk,hash)))
      break;
    // ow, ret is 0!
  } while ( (pk = PTR(sCons*,pk->car)) );
  //either good, or both pk and ret are 0.
  return ret;
}
typedef struct sSymIn{
  U32 sym;
  U32 pkg;
} sSymIn;
 
sSymIn pks_find_hash1(sCons*pk, U32 hash){
  sSym* ret;
  do {
    if((ret = pk_find_hash(pk,hash)))
      break;
    // ow, ret is 0!
  } while ( (pk = PTR(sCons*,pk->car)) );
  //either good, or both pk and ret are 0.
  return (sSymIn){ THE_U32(ret),THE_U32(pk) };
}


sSymIn pks_find_name1(sCons*pk, char*name){
  U32 hash = string_hash(name);
  return pks_find_hash1(pk,hash);
}
*/
 

sSym* pks_find_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pk_find_hash_proc);
}

sSym* pks_find_name(sSym*pk, char*name){
  return pks_find_hash(pk,string_hash(name));
}

sSym* pks_find_prev_hash(sSym* pk,U32 hash){
  return pkgs_walk_U32(pk,hash,pk_find_prev_hash_proc);
}

U64 find_global(char*name){
  sSym* sym = pks_find_name(PTR(sSym*,SRCH_LIST),name);
  if(sym)
    return sym->art;
  else
    return 0;
}


void pks_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");
  sSym* pk = PTR(sSym*,SRCH_LIST);
  pkgs_walk (pk, (U64)(f), pk_dump_protos_proc);
}

void srch_list_push(sSym* pk){
  pk->art = SRCH_LIST;  // next package;
  SRCH_LIST = THE_U32(pk);
}


sSym* srch_list_find_pkg(U32 hash){
  sSym* pk = PTR(sSym*,SRCH_LIST);
  do {
    if(pk->hash == hash)
      break;
  } while((pk = PTR(sSym*,pk->art)));
  return pk;
}
// first find it, then call this..
// 0 means SRCH_LIST is pointing at it.
sSym* srch_list_prev_pkg(sSym* pkg){

  sSym* pk = PTR(sSym*,SRCH_LIST);
  return
  if(pk== pkg)
    return 0 ; 
  do {
    if(pk->art == THE_U32(pkg))
      break;
  } while((pk = PTR(sSym*,pk->art)));
  return pk;
}


void srch_list_pkgs_ls(){
  sSym* pk = (PTR(sSym*,SRCH_LIST));
  while(pk){
    printf("%s ",SYM_NAME(pk));
    pk= (PTR(sSym*,pk->art));
  }
  printf("\n");
}


char* next_line(char* p){
  p = strchr(p,10);
  if(p) 
    *p++ = 0;
  return p;
}
// using a text file at path to initialize names of dl symbols...
// the library package is not actually linked to the system -
// pk_rebind must be invoked to bind to the current addresses after
// a load...
sSym* pk_from_libtxt(char* name,char* dlpath,char*path){
  // a 12-byte prototype of an assembly trampoline
  U8 ljump[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0 }; // jmp rax
  
  char* buf = filebuf_malloc(path);
  if(!buf)
    return 0;
  sSym* pk = pk_new(name,dlpath); 
  char* pc = buf;
  char* next;
  while((next = next_line(pc))){
    U32 addr = cseg_append(ljump,sizeof(ljump));
    cseg_align8();
    rel_mark( addr+2, 3); // mark the 64-bit address
    sSym* sy = sym_new(pc,addr,sizeof(ljump),0,0);
    pk_push_sym(pk,sy) ;
    pc = next;
  }
  filebuf_free(buf);
  return pk;
  
}
void pk_rebind(sSym* s){
  char* dllpath = sym_proto(s);
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  
  while((s = PTR(sSym*,s->cdr))){
    void* pfun = dlsym(dlhan,SYM_NAME(s));
    if(!pfun)
      printf("pk_rebind: could not bind to %s\n",SYM_NAME(s));
    *((void**)(U64)(s->art+2)) = pfun; // fixup to function address
    dlclose(dlhan);
  }
}


/*----------------------------------------------------------------------------
  pk_incorporate            Given a new, unlinked symbol, make sure it is
                             a valid new symbol, or a valid replacement of
                             an old one (in which case, replace!)

This is particularly tricky, as:

----------------------------------------------------------------------------*/
void pk_incorporate(sSym* new){
  // do we already have a symbol with same name?  Hold onto it.
  sSym* prev = pks_find_prev_hash(PTR(sSym*,SRCH_LIST), new->hash);
  if(prev) { // older version we are replacing?
    sSym* old = PTR(sSym*,prev->cdr);
    //printf("prev: %p, old: %p\n",prev,old);
    // replacing symbol must be in same seg
    if( (SEG_BITS(old->art)) == (SEG_BITS(new->art))){
      new->cdr = old->cdr;   // relink new in the same place old was
      prev->cdr = THE_U32(new);
     
      U32 fixups = aseg_reref(old->art,new->art);
  //  printf("%d old refs fixed from %08X to %08X\n",fixups, old->art, new->art);
      //      hd(PTR(void*,old->art),4);
      fixups += sym_del(old);       
      printf("%d fixups\n",fixups);
    } else {
      printf("!!! SEGMENT MISMATCH! TODO: FIX!!!!\n");
      //segment mismatch..
    }
  } else {
    pk_push_sym(PTR(sSym*,SRCH_LIST),new);   
  }
}
