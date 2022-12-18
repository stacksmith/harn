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
  printf(" %08X %08X \n",CFILL,DFILL);
}
/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/
void aseg_alloc(){

  seg_mmap(PTR(void*,CODE_SEG_ADDR), CODE_SEG_SIZE,
	   PROT_READ|PROT_WRITE|PROT_EXEC,"code");

  seg_mmap( PTR(void*,CODE_SEG_ADDR/8), CODE_SEG_SIZE/8,
	    PROT_READ | PROT_WRITE,"code-rel");
	    
  seg_mmap(PTR(void*,DATA_SEG_ADDR), DATA_SEG_SIZE,
	   PROT_READ|PROT_WRITE,"data");
  
  seg_mmap( PTR(void*,DATA_SEG_ADDR/8), DATA_SEG_SIZE/8,
	    PROT_READ | PROT_WRITE,"data-rel");

  CFILL = CODE_SEG_ADDR;
  DFILL = DATA_SEG_ADDR+8;

  rel_mark (THE_U32(DFILL_ADDR),  2); //dfill A32
  rel_mark (THE_U32(CFILL_ADDR),  2); //cfill A32
}
/* ------------------------------------------------------------- */

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

*/
/* -------------------------------------------------------------
xseg_append  Append a run of bytes to the segment
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

U32 cseg_append(U8* start,U64 size){
  U32 end =  CFILL + size;
  U8* dest = CFILL_PTR;
  CFILL = end;
  if(start)
    memcpy(dest,start,size);
  else
    memset(dest,0,size);
  return THE_U32(dest);
}

void cseg_align8(){
  CFILL = (CFILL+7) & 0xFFFFFFF8;
}
/*----------------------------------------------------------------------------
  aseg_delete                     Delete an artifact; drop rest of segment;
                                 perform fixups within asegs.

----------------------------------------------------------------------------*/
#include "util.h"

typedef struct sSegDrop {
  U32 start;
  U32 end;
  U32 fixup;
} sSegDrop;
  
U32 aseg_delete1(sSegDrop* dz,  U32 other_end){
  U32 ret = 0;
  U32 dz_target = dz->start - dz->fixup;
  // fix the dropzone itself
  //  hd(PTR(U8*,0x40000e40),4);
  ret += bits_fix_inside1(dz->start, dz->end,  dz->fixup); // 
  // fix the bottom part of dropzone segment, target downto base.
  ret += bits_fix_outside(dz_target, 
			  dz->start, dz->end, dz->fixup); // dropzone bounds
  // now fix entire other segment,  top is provided, bottom is seg base.
  ret += bits_fix_outside(other_end, 
			  dz->start, dz->end, dz->fixup); // dropzone bounds
  // drop the dropzone; fill with 0.
  bits_drop(dz_target, dz->start, dz->end - dz->start);
  //  hd(PTR(U8*,0x40000e40),4);

  return ret;
  
}
U32 aseg_delete(U32 dz_start, U32 dz_end, U32 fixup,  U32 other_end){
  U32 ret = 0;
  U32 dz_target = dz_start - fixup;
  // fix the dropzone itself
  //  hd(PTR(U8*,0x40000e40),4);
  ret += bits_fix_inside1(dz_start, dz_end,  fixup); // 
  // fix the bottom part of dropzone segment, target downto base.
  ret += bits_fix_outside(dz_target, 
			  dz_start, dz_end, fixup); // dropzone bounds
  // now fix entire other segment,  top is provided, bottom is seg base.
  ret += bits_fix_outside(other_end, 
			  dz_start, dz_end, fixup); // dropzone bounds
  // drop the dropzone; fill with 0.
  bits_drop(dz_target, dz_start, dz_end-dz_start);
  //  hd(PTR(U8*,0x40000e40),4);

  return ret;
  
}
/*----------------------------------------------------------------------------
  aseg_reref                    Fix all refs to old, point them at new.

Notably, mseg is not affected
----------------------------------------------------------------------------*/

U32 aseg_reref(U32 old, U32 new){
  // fix code segment refs
  U32 fixcnt = bits_reref(CFILL, CODE_SEG_ADDR, old, new);
  // fix data segment refs
  fixcnt +=    bits_reref(DFILL, DATA_SEG_ADDR, old, new);
  return fixcnt;
}
  
