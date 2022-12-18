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

The head of the package is a cons
 cdr = first symbol in package;
 car = next package (see pkgs)

----------------------------------------------------------------------------*/
sCons* pk_new(char*name){
  return mcons(0,0);
}

void pk_dump(sCons* pk){
  sSym*s = U32_SYM(pk->cdr);
  while(s){
    sym_dump1(s);
    s = U32_SYM(s->cdr); 
  }
}

void pk_push_sym(sCons* pk, sSym* sym){
  sym->cdr = pk->cdr;
  pk->cdr = THE_U32(sym);
}

sCons* pk_find_prev_hash(sCons* prev,U32 hash){
  sSym* next;
  while((next = PTR(sSym*,prev->cdr))){
    if (hash == next->hash) 
      return prev;
    prev = (sCons*)next;
  } 
  return (sCons*)next;
}


sSym* pk_find_hash(sCons* pk,U32 hash){
  while((pk = PTR(sCons*,pk->cdr))){
    if (hash == ((sSym*)pk)->hash) 
      break;
  }
  return PTR(sSym*,pk);
}

void pk_dump_protos(sCons* pk,FILE* f){
  sSym*s = PTR(sSym*,pk->cdr);  //skip pk, cdr is it
  char* proto;
  while(s){
    //    printf("pk_dump_protos: %p\n",s);
    proto = sym_proto(s);
    //    printf("  proto is at %p\n",proto);
    if(*proto){
      fprintf(f,"%s\n",proto);
    }
    s = PTR(sSym*,s->cdr);
  }
}


sSym* pk_find_name(sCons*pk, char*name){
  return pk_find_hash(pk,string_hash(name));
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

sSym*pks_find_name0(sCons*pk, char*name){
  U32 hash = string_hash(name);
  return pks_find_hash0(pk,hash);
}


sCons* pks_find_prev_hash0(sCons*pk, U32 hash){
  sCons* ret;
  do {
    if((ret = pk_find_prev_hash(pk,hash))){
      break;
    }
  } while ((pk = PTR(sCons*,pk->car)));
  return ret;
}


U64 find_global(char*name){
  sSym* sym = pks_find_name0(PTR(sCons*,SRCH_LIST),name);
  if(sym)
    return sym->art;
  else
    return 0;
}

/*
 
sCons* pks_find_prev_hash(sCons*pk, U32 hash,sCons**in){
  sCons* ret;
  do {
    if((ret = pk_find_prev_hash(pk,hash))){
      if(in) *in = pk;
      return ret;
    }
  } while ((pk = PTR(sCons*,pk->car)));
  if(in)  *in = 0;
  return 0;
}
*/
void pks_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");
  sCons* pk = PTR(sCons*,SRCH_LIST);
  do {
    pk_dump_protos(pk,f);
  } while ((pk = PTR(sCons*,pk->car)));
  fclose(f);
}


 


char* next_line(char* p){
  p = strchr(p,10);
  if(p) 
    *p++ = 0;
  return p;
}
 
sCons* pk_from_libtxt(char* name,char*path){
  // a 12-byte prototype of an assembly trampoline
  U8 ljump[12]={0x48,0xB8,   // mov rax,?
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //64-bit address
    0xFF,0xE0 }; // jmp rax
  
  char* buf = filebuf_malloc(path);
  if(!buf) return 0;
  sCons* pk = pk_new(name);
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
void pk_rebind(sCons* pk,char*dllpath){
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  sSym*s = PTR(sSym*,pk->cdr);
  while(s){
     void* pfun = dlsym(dlhan,SYM_NAME(s));
    if(!pfun)
      printf("pk_rebind: could not bind to %s\n",SYM_NAME(s));
    *((void**)(U64)(s->art+2)) = pfun; // fixup to function address
    s = PTR(sSym*,s->cdr); 
  }
  dlclose(dlhan);
}

void srch_list_push(sCons* pk){
  pk->car = SRCH_LIST;  // next package;
  SRCH_LIST = THE_U32(pk);
}

/*----------------------------------------------------------------------------
  pk_incorporate            Given a new, unlinked symbol, make sure it is
                             a valid new symbol, or a valid replacement of
                             an old one (in which case, replace!)

----------------------------------------------------------------------------*/
void pk_incorporate(sSym* new){
  // do we already have a symbol with same name?  Hold onto it.
  sCons* prev = pks_find_prev_hash0(PTR(sCons*,SRCH_LIST), new->hash);
  if(prev) { // older version we are replacing?
    sSym* old = PTR(sSym*,prev->cdr);
    printf("prev: %p, old: %p\n",prev,old);
    // replacing symbol must be in same seg
    if( (SEG_BITS(old->art)) == (SEG_BITS(new->art))){
      pk_push_sym(PTR(sCons*,SRCH_LIST),new); //attach it so we don't lose it
      U32 fixups = aseg_reref(old->art,new->art);
      printf("%d old refs fixed\n",fixups);
      fixups = sym_delete(prev);          //after relocations
      printf("%d gc fixups\n",fixups);
    } else {
      printf("!!! SEGMENT MISMATCH! TODO: FIX!!!!\n");
      //segment mismatch..
    }
  } else {
    pk_push_sym(PTR(sCons*,SRCH_LIST),new);   
  }
}
