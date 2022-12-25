#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "global.h"
#include "util.h"
#include "src.h"
#include "seg_meta.h"
#include "seg_art.h" 

#include "sym.h"
#include "seg_asm.h"

char* sym_name(sSym* sym){
  return ((char*)sym)+SYM_NAME_OFF;
}
char* sym_proto(sSym* sym){
  char* pname = ((char*)sym)+SYM_NAME_OFF;
  return (pname + sym->namelen);
}

U32 sym_dump1(sSym* sym){
  printf("%p: %s:\t %08X %04X %s\n",sym,SYM_NAME(sym),sym->art, sym->size,sym_proto(sym));
  return 0;
}
sSym* sym_new(char* name, U32 art, U32 size, U32 src,char* proto){
  U32 namelen = strlen(name);
  U32 protolen = proto ? strlen(proto) : 0;
  U32 octs = (SYM_NAME_OFF + namelen + protolen + 2 + 7) >> 3;
  U8* bptr = mseg_append(0,octs*8);
  U32 uptr = (U32)(U64)bptr;
  sSym* p = (sSym*)bptr;
  // There are two reeferences: both are A32 pointers
  rel_mark(uptr+4,2); // next   symbols are linked-listed...
  rel_mark(uptr+8,2); // art    artifact reference
  mseg_align8();
  // Now populate the fields
  p->hash    = string_hash(name);
  p->art     = art;
  p->size    = size;
  p->src     = src;
  p->octs    = octs;
  p->namelen = namelen+1;
  // copy the provided name directly after the symbol structure
  strcpy(bptr+SYM_NAME_OFF, name); // null-terminated
  // If prototype provided, copy that after the name
  if(proto)
    strcpy(bptr+SYM_NAME_OFF+1+namelen, proto);
  // always keep the segments aligned to 8
  mseg_align8();
  return p;
}
/*----------------------------------------------------------------------------
  sym_delete                  delete a symbol and its artifact.

  Must be unlinked and unreferenced!

  It's a little convoluted, but this is a multi-step process which must be
  completed, as referential integrity is broken while segments are fixed-up
  and dropped, one by one...  If anything fails here we are toast; luckily,
  not too much can go wrong, god willing.

  The symbol in the metasegment has a reference to the artifact in
  code or data segment, which mus likewize be removed.

  The are occupied by the symbol or artifact is referred to as a hole,
  while the span above the hole to the top of the segment is called
  'dropzone'.  The dropzone will be dropped covering the hole, later.
  before that happens, we need to fix a variety of references which will
  be affected by the movement of data -- references in all segments!
----------------------------------------------------------------------------*/

U32 sym_del(sSym* sym){
  // start by assembling data about the symbol bounds in the meta seg,
  // as well as the artifact boundaries in one of the two art segs.
  U32 adz_fix  = ROUND8(sym->size);   // art dropzone will drop this much
  U32 adz_tgt  = sym->art;           // to this target pozition
  U32 adz_lo   = adz_tgt + adz_fix; // starting from here, up
  U32 mfill    = MFILL; // MFILL will be altered - keep original copy.
  U32 hole     = THE_U32(sym);    // meta segment hole/drop-target;
  U32 holesize = SYM_BYTES(sym); // meta segment fixup
  U32 dropzone = hole+holesize; // meta segment dropzone start
  // fixupe all intersegment meta pointers in anticipation of the drop;
  // while traversing, fixup artifact references to post-art-drop state.
  U32 ret = bits_fix_meta(mfill, dropzone, holesize,  adz_lo, adz_fix);
  // The system is now broken; do not access any segment data!
  // compact meta segment and rel table
  bits_drop(hole, dropzone,  mfill-dropzone); // like memcpy
  // OK, this fixes most of metadata except for artifact pointers
  // Determine which segment the artifact sits in, and get the
  U32 adz_hi, otherend;    // fill-ptrs/tops for both segments.
  if(IN_DATA_SEG(adz_lo)){ 
    adz_hi =  DFILL;    otherend = CFILL;
  } else {
    adz_hi =  CFILL;    otherend = DFILL;
  }
  // fix the references inside the artifact drop zone itself
  ret += bits_fix_inside(adz_lo, adz_hi,  adz_fix);
  // now fix the outside, bottom part of the artifact segment
  ret += bits_fix_outside(adz_tgt,   adz_lo, adz_hi, adz_fix);
  // and the entire other segment as there may be cross-seg refs
  ret += bits_fix_outside(otherend,  adz_lo, adz_hi, adz_fix); 
  // finally, compact the artifact segment and rel tables
  bits_drop(adz_tgt, adz_lo, (adz_hi - adz_lo)); // like memcpy
  //  hd(PTR(U8*,0x40000e40),4);
  return ret;
}


/*----------------------------------------------------------------------------
  sym_for_artifact            create a sSym for an ELF in aseg...

This is called by ing_elf after ingesting the data.  Some assumptions:
* sElf structure is still alive and contains the name (ing_elf deletes later)
* gcc invoked with '-aux-info info.txt'; last line is func prototype
* compiled source for this artifact is in 'code.c'; will be absorbed.

----------------------------------------------------------------------------*/
sSym* sym_for_artifact(char* name,U32 art){
  U32 srclen; //not used!
  U32 src = src_from_body(&srclen);  // store source and get file offset
  // if function, extract proto
  char* proto = 0;
  U32 size;
  if(IN_CODE_SEG(art)){
    proto = aux_proto();    // allocated buffer with prototype from aux-info
    size = CFILL - art;
    cseg_align8();
  } else {
    size = DFILL - art;
    dseg_align8();
  }
  // create a new symbol (proto is copied);
  sSym* ret = sym_new(name, art, size, src, proto);
  free(proto);
  return ret;
}

