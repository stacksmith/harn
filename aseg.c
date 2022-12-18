/* Load a single elf object file and fixup */

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <elf.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>

#include "global.h"
#include "asmutil.h"
#include "aseg.h"

/* ==============================================================

A fillable area for code or data

  ==============================================================*/
/*typedef struct sSeg {
  U8* base; // ptr of allocation/mapping
  U8* fillptr;
  U8* end;
  char name[8];
} sSeg;
*/
void aseg_dump(){
  printf(" %08X %08X \n ",*DFILL_ADDR,*CFILL_ADDR);
}
/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/
void aseg_alloc(){
  void err(char*what){
    fprintf(stderr,"Error allocating %s seg: %d\n",what,errno);
    exit(1);
  }

  // allocate code
  void* pcode = mmap(PTR(void*,CODE_SEG_ADDR), CODE_SEG_SIZE,
		     PROT_READ|PROT_WRITE|PROT_EXEC,
		     0x20 | MAP_SHARED | MAP_FIXED, //MAP_ANONYMOUS
		     0,0);
  if(MAP_FAILED == pcode) err("code");

  void* pdata = mmap(PTR(void*,DATA_SEG_ADDR), DATA_SEG_SIZE,
		     PROT_READ|PROT_WRITE,
		     0x20 | MAP_SHARED | MAP_FIXED, //MAP_ANONYMOUS
		     0,0);
  if(MAP_FAILED == pdata) err("data");

  void* prel = mmap( PTR(void*,CODE_SEG_ADDR/8), (DATA_SEG_SIZE+CODE_SEG_SIZE)/8,
		     PROT_READ | PROT_WRITE,
		     0x20 | MAP_SHARED | MAP_FIXED ,    //MAP_ANONYMOUS
		     0,0);
  
  if(MAP_FAILED == prel) err("rel");

  CFILL = DATA_SEG_ADDR;
  DFILL = DATA_SEG_ADDR+8;

  rel_mark (DATA_SEG_ADDR, 3); //dfill
  rel_mark (DATA_SEG_ADDR+4, 3); //cfill
}
/* ------------------------------------------------------------- */
void rel_mark(U32 pos,U32 kind){
  if(REL_FLAG)
    bits_set(pos, kind);
}

/*
void seg_serialize(sSeg* psg,FILE* f){
  // save data area
  seg_align(psg,8);
  U32 size = psg->fill - (U32)(U64)(psg);  // byte size of segment data
  size_t wr1 = fwrite(psg,1,size,f);
  U8* prel = (U8*)(((U64)psg)>>3);
 
  size_t wr2 = fwrite(prel,1,size>>3,f);
  printf("wrote: %lx and %lx\n",wr1,wr2);
}
// TODO: should wipe rel first; otherwise, deserializing a shorter segment
// than current will leave old garbage.
void seg_deserialize(sSeg* psg,FILE*f){
  fread(psg,1,8,f);
  U32 size = psg->fill - (U32)(U64)psg;
  printf("size %x\n",size);
  size_t rd1 = fread(psg+1,1,size-8,f);
  U8* prel = (U8*)(((U64)psg)>>3);
  size_t rd2 = fread (prel,1,size>>3,f);
  printf("read: %lx %lx\n",rd1+8,rd2);
}
U32 seg_pos(sSeg* psg){
  return psg->fill;
}
*/
/* -------------------------------------------------------------
seg_append  Append a run of bytes to the segment
            size   number of bytes to append
            start  address from which to copy.  
                   if 0, fill with 0 bytes.
---------------------------------------------------------------*/
U32 dseg_append(U8* start,U64 size){
  U32 end =  DFILL + size;
  U8* dest = DFILL_PTR;
  DFILL = end;
  if(start)
    memcpy(dest,start,size);
  else
    memset(dest,0,size);
  return THE_U32(dest);
}

void dseg_align8(){
  DFILL = (DFILL+7) & 0xFFFFFFF8;
}

U32 cseg_prepend(U8* start,U64 size){
  U32 udest = (CFILL - size) & 0xFFFFFFF8;
  U8* dest = (U8*)(U64)udest;
  CFILL = udest;
  if(start){
    memcpy(dest,start,size);
  } else {
    memset(dest,0,size);
  } 
  return THE_U32(dest);
}


