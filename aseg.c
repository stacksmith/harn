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
  printf(" %08X %08X \n",CFILL,DFILL);
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

  void* prelc = mmap( PTR(void*,CODE_SEG_ADDR/8), CODE_SEG_SIZE/8,
		     PROT_READ | PROT_WRITE,
		     0x20 | MAP_SHARED | MAP_FIXED ,    //MAP_ANONYMOUS
		     0,0);
   if(MAP_FAILED == prelc) err("code-rel");

  
  void* pdata = mmap(PTR(void*,DATA_SEG_ADDR), DATA_SEG_SIZE,
		     PROT_READ|PROT_WRITE,
		     0x20 | MAP_SHARED | MAP_FIXED, //MAP_ANONYMOUS
		     0,0);
  if(MAP_FAILED == pdata) err("data");
 
  void* preld = mmap( PTR(void*,DATA_SEG_ADDR/8), DATA_SEG_SIZE/8,
		     PROT_READ | PROT_WRITE,
		     0x20 | MAP_SHARED | MAP_FIXED ,    //MAP_ANONYMOUS
		     0,0);
  
  if(MAP_FAILED == preld) err("data-rel");

  CFILL = CODE_SEG_ADDR;
  DFILL = DATA_SEG_ADDR+8;

  rel_mark (DATA_SEG_ADDR,   3); //dfill A32
  rel_mark (DATA_SEG_ADDR+4, 3); //cfill A32
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
  aseg_chomp                     Delete an artifact; drop rest of segment;
                                 perform fixups within asegs.

The segment in which the artifact is deleted must be processed in two steps:
the area below the dropzone must be processed first, followed by the 
area inside dropzone.

The other segment must be processed using the outside code.


----------------------------------------------------------------------------*/

U32 dseg_chomp(U32 addr, U32 size){
  U32 ret = 0;
  // determine dropzone bounds, pre-drop
  U32 dz_target = addr;
  U32 dz_start  = addr + size;
  U32 dz_end    = DFILL;                  
  U32 dz_size   = dz_end - dz_start;
  
   // drop the dropzone; fill with 0.
  bits_drop(dz_target, dz_start, 0, dz_size);
  /*  
  ret += bits_fix_inside(dz_target + dz_size, dz_target, // post-drop region
		  dz_start,dz_end,                // pre-drop dropzone
		  size);                          // fixup
  */
  ret += bits_fix_outside(dz_target, DATA_SEG_ADDR,  // region below drop
		   dz_start,dz_end,           // refs to pre-drop dropzone
		   size);                     // fixup by -size
  // now fix code segment!
  ret += bits_fix_outside(CFILL, CODE_SEG_ADDR,  // entire code segment
			  dz_start,dz_end,       // refs to pre-drop dropzone
			  size);                 // fixup by -size
  return ret;
  
}
#include "util.h"
U32 cseg_chomp(U32 addr, U32 size){
  U32 ret = 0;
  // determine dropzone bounds, pre-drop
  U32 dz_target = addr;
  U32 dz_start  = addr + size;
  U32 dz_end    = CFILL;                  
  U32 dz_size   = dz_end - dz_start;

hd(PTR(U8*,0x40000E40),4);
printf("\n");

 printf("fix_inside(%08x,%08x,  %08x\n",
	dz_end, dz_start,size);                          // fixup

 
 ret += bits_fix_inside(dz_end,dz_start,  // pre-drop dropzone
			dz_end,size);           // fixup

printf("\ninside: %d\n",ret); hd(PTR(U8*,0x40000E40),4);

 printf("fix_outside(%08x,%08x, %08x,%08x, %08x\n",
	dz_target, CODE_SEG_ADDR,  // region below drop
	dz_start,dz_end,           // refs to pre-drop dropzone
	size);                     // fixup by -size

	
  ret += bits_fix_outside(dz_target, CODE_SEG_ADDR,  // region below drop
		   dz_start,dz_end,           // refs to pre-drop dropzone
		   size);                     // fixup by -size
  
printf("\noutside:%d\n",ret); hd(PTR(U8*,0x40000E40),4);  

  // now fix data segment!
  ret += bits_fix_outside(DFILL, DATA_SEG_ADDR,  // entire code segment
			  dz_start,dz_end,       // refs to pre-drop dropzone
			  size);                 // fixup by -size
printf("\n%ds:d\n",ret); hd(PTR(U8*,0x40000E40),4);  

// drop the dropzone; fill with 0.
 printf("bit_drop(%08x,%08x, %d, %08x)\n",
	dz_target, dz_start, 0, dz_size);
 bits_drop(dz_target, dz_start, 0, dz_size);
 printf("after drop:\n");
 hd(PTR(U8*,0x40000E40),4);

 


 return ret;

}
U32 aseg_chomp(U32 addr, U32 size){
  if(IN_DATA_SEG(addr))
    return dseg_chomp(addr,size);
  else
    return cseg_chomp(addr,size);
}

    
