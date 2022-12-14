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

sSym* sym_new(char* name, U32 art, U32 size, U32 src,char* proto){
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
  p->art = art;
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
  printf("%p: %s:\t %08X %04X %s\n",sym,SYM_NAME(sym),sym->art, sym->size,sym_proto(sym));
}
/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
create a symbol from ELF sisymb   ;TODO: wtf?
----------------------------------------------------------------------------*/
sSym* sym_from_siSymb(void* v){
  siSymb*s = (siSymb*)v;
  return sym_new(s->name,s->data,s->size,s->src,s->proto);
}

