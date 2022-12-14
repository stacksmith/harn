#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"
#include "seg.h"
#include "sym.h"

extern sSeg* psCode;
extern sSeg* psData;
extern sSeg* psMeta;




struct siSymb;
typedef struct siSymb {
  struct siSymb* next;
  char* name;                 // malloc'ed
  union {
    sDataSize ds;
    struct {
      U32 data;
      U32 size;
    };
  };
  char* proto;                // malloc'ed
  U32 src;                    // file
  U32 srclen;
  U32 hash;
  
} siSymb;

sSym* sym_new(char* name, U32 data, U32 size, U32 src,char* proto){
  U32 namelen = strlen(name);
  U32 protolen = proto ? strlen(proto) : 0;
  U32 octs = (SYM_NAME_OFF + namelen + protolen + 2 + 7) >> 3;
  U8* bptr = seg_append(psMeta,0,octs*8);
  U32 uptr = (U32)(U64)bptr;
  sSym* p = (sSym*)bptr;
  
  seg_rel_mark(psMeta,uptr+0,3); // next pointer is a 32-bit pointer
  seg_rel_mark(psMeta,uptr+8,3); // data pointer is a 32-bit pointer

  seg_align(psMeta,8);
  p->hash = string_hash(name);
  p->data = data;
  p->size = size;
  p->src  = src;
  p->octs = octs;
  p->namelen = namelen+1;
  strcpy(bptr+SYM_NAME_OFF, name);

  if(proto){
    strcpy(bptr+SYM_NAME_OFF+1+namelen, proto);
  }
  seg_align(psMeta,8);
  return p;
}
/*
void sym_wipe_last(sSym* sym){
  U32 bytes = sym->octs * 8;
  U32 addr = sym->data;
  U32 size = sym->size;
  if(IN_CODE_SEG(addr)){
    psCode->fill = addr; // drop the code segment, eliminate code
  } else {
    psData->fill = addr;
  }
  memset((U8*)(U64)addr,0,size); // clear it
  memset((U8*)(U64)(addr/8),0,(size+7)/8); // clear rel

  psMeta->fill = (U32)(U64)sym;  
  memset(sym,0,bytes);
  memset((U8*)(U64)(psMeta->fill/8),0,sym->octs); // clear rel
}
*/
char* sym_name(sSym* sym){
  return ((char*)sym)+SYM_NAME_OFF;
}
char* sym_proto(sSym* sym){
  char* pname = ((char*)sym)+SYM_NAME_OFF;
  return (pname + sym->namelen);
}

void sym_dump1(sSym* sym){
  printf("%p: %s:\t %08X %04X %s\n",sym,SYM_NAME(sym),sym->data, sym->size,sym_proto(sym));
}
sSym* sym_from_siSymb(void* v){
  siSymb*s = (siSymb*)v;
  return sym_new(s->name,s->data,s->size,s->src,s->proto);
}

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
/*
void pk_wipe_last_sym(sSym* pk){
  sSym* ret = U32_SYM(pk->next);
  pk->next = ret->next;
  sym_wipe_last(ret);
}
*/
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


sSym* pks_find_hash(sSym*pk, U32 hash){
  sSym* ret;
  do {
    if((ret = pk_find_hash(pk,hash)))
      return ret;
  } while ((pk = U32_SYM(pk->data)));
  return 0;
}

sSym* pks_find_name(sSym*pk, char*name){
  return pks_find_hash(pk,string_hash(name));
}

void pks_dump_protos(){
  FILE* f = fopen("sys/headers.h","w");
  sSym* pk = (sSym*)(U64)SRCH_LIST;
  do {
    pk_dump_protos(pk,f);
  } while ((pk = U32_SYM(pk->data)));
  fclose(f);
}


U64 find_global(char*name){
  sSym* sym = pks_find_name((sSym*)(U64)SRCH_LIST,name);
  if(sym)
    return sym->data;
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
    *((void**)(U64)(s->data+2)) = pfun; // fixup to function address
    s = U32_SYM(s->next); 
  }
  dlclose(dlhan);
}

void srch_list_push(sSym* pk){
  pk->data = SRCH_LIST;  // we point at whatever srch_list did
  SRCH_LIST = (U32)(U64)pk;
}
