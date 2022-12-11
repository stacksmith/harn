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
#include "sg.h"

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
void sg_dump(sSg* psg){
  printf("Segment %p %08x\n",psg, psg->fill);
}
/* -------------------------------------------------------------
   seg_alloc

---------------------------------------------------------------*/
sSg* sg_alloc(U64 req_size, void* req_addr, U32 prot){
  // allocate data area
  sSg* psg = (sSg*)mmap(req_addr, req_size, prot,
			0x20 | MAP_SHARED | MAP_FIXED, //MAP_ANONYMOUS
			0,0);
  if(MAP_FAILED == psg){
    fprintf(stderr,"Error allocating %p seg: %d\n",req_addr,errno);
    exit(1);
  }
  // allocate rel area
  U8* prel =(U8*)mmap((void*)(((U64)req_addr)/8), req_size/8,
		      PROT_READ | PROT_WRITE,
		      0x20 | MAP_SHARED | MAP_FIXED ,    //MAP_ANONYMOUS
		      0,0);
  if(MAP_FAILED == prel){
    fprintf(stderr,"Error allocating rel table of %p seg: %d\n",req_addr,errno);
    exit(1);
  }
  psg->fill = (U32)(U64)psg + 8;
  psg->end =  psg->fill + (U32)req_size;
  return psg;
}

U32 sg_pos(sSg* psg){
  return psg->fill;
}

void sg_align(sSg*psg, U64 align){
  int rem  = psg->fill % (U32)align;
  if(rem) {
    sg_append(psg,0,align-rem);
    //    printf("Inserted pad of %ld bytes\n",align-rem);
  }  
}


/* -------------------------------------------------------------
seg_append  Append a run of bytes to the segment
            size   number of bytes to append
            start  address from which to copy.  
                   if 0, fill with 0 bytes.
---------------------------------------------------------------*/
U8* sg_append(sSg* psg,U8* start,U64 size){
  U32 end = psg->fill + size;
  U8* dest = (U8*)(U64)psg->fill;
  if(end >= psg->end) {
    sg_dump(psg);
    fprintf(stderr,"seg_append failed: out of space\n");
    exit(1);
  } else {
    psg->fill = end;
    if(start)
      memcpy(dest,start,size);
    else
      memset(dest,0,size);
  } 
  return dest;
}
/*----------------------------------------------------------------------------
 RELOCATIONS

 We observe that:
 * code segment may have Off32 or Abs64 references;
 * data segment may have Abs64 refs or Abs32 refs (not for C);
 * meta segment only has Abs32 refs.

----------------------------------------------------------------------------*/

extern U32 rel_flag;
/* ------------------------------------------------------------- */
void sg_rel_mark(sSg* psg,U32 pos,U32 kind){
  if(rel_flag){
    if( (pos<(U32)(U64)psg) || (pos >= psg->fill)){
      printf("seg_rel_mark: pos %08X is out of bounds\n",pos);
      exit(1);
    }
    if(kind>3){
      printf("seg_rel_mark: kind %d is out of bounds\n",kind);
      exit(1);
    }
    //  printf("setting bits\n");
    bits_set(pos, kind);
  }
}
#include "util.h"
/*
U32 cnt_refs(sSeg*pseg,void*target){
  seg_align(pseg,8);
  //  U8* relbase = pseg->prel;
  U32 bottom = (U32)(U64)(pseg->prel);
  U32 off = seg_off(pseg);
  U32 i=0;
  U64 result;
  hd(pseg->base,16);
  //  printf("Entering loop %p %d %d\n",relbase,off,off>>3);
  while((result = bits_next_ref(off,bottom))){
    //    printf(".1.\n");
    //    printf("ref %d at $%08X",(U32)(result>>32),(U32)result);
    off = (U32) result;
    U8* ref = (U8*)(U64)off;  //position of reference
    void* tgt = (result&3)-1 ? 
      *(void**)ref :
      (void*) ((*(S32*)ref) + (ref+4));
    if(tgt==target)
      i++;
    //    printf("at %p, %p\n",ref,tgt);

  }
  return i;
}
*/
/*
U32 seg_reref(sSeg*pseg,U64 old,U64 new){
  seg_align(pseg,8);
  //  U8* relbase = pseg->prel;
  U8* segbase = pseg->base;
  U32 off = (seg_pos(pseg));
  U32 bottom = (U32)(U64)(pseg->base);
  U64 diff = new-old;
  U64 result;
  U32 i=0;
  hd(segbase,16);
  hd(pseg->prel,16);
  //  printf("Entering loop %x %x\n",off,bottom);
  while((result = bits_next_ref(off,bottom))){
    //printf(".1. %016lx\n",result);
    //printf("ref %d at $%08X\n",(U32)(result>>32),(U32)result);
    off = (U32) result;
    U8* ref = (U8*)(U64)off;  //position of reference
    if((result&3)-1) {
      if(old == (*(U64*)ref)) {
	printf("64-bit fixup at %p, by %08lX\n",ref,diff);
	i++;
	(*(U64*)ref) += diff;
      }
    } else {
      if(old == ((U64)((*(S32*)ref) + (ref+4)))){
	printf("32-bit fixup at %p, by %08X\n",ref,(U32)diff);
	i++;
	(*(U32*)ref) += (U32)diff;
      }
    }
  }
  return i;
}
*/
void sg_reref(sSg*psg,U32 old,U32 new){
  printf("%08X %08X %08X %08X \n",
	 sg_pos(psg),
	 (U32)(U64)psg,
	 old,new);
  bits_reref1(sg_pos(psg),
	     (U32)(U64)psg,
	     old,new);
  printf("returned\n");
	 
}
  
