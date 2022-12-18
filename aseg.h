/* ==============================================================

Artifact segments, for code and data...  While it's tempting to
make segments uniform, we want to avoid fill pointer in code seg;
and want the option of loading code and data without meta, so we 
are stuck with fill pointers in the data segment...

Code is at $40000000  read,write,execute
Data is at $80000000  read,write

Special A32 32-byte pointers:
$80000000 CFILL  fill pointer for the code segment 
$80000004 DFILL  fill pointer for the data segment 
  ==============================================================*/
// segment header at the start of each segment
//

#include "seg_common.h"

void aseg_dump();
void aseg_alloc();

void dseg_align8();
U32  dseg_append(U8* start,U64 size);
void cseg_align8();
U32  cseg_append(U8* start,U64 size);




U32 aseg_delete(U32 dz_start, U32 dz_end, U32 fixup, U32 other_end);
U32 aseg_delete1(U32 dz_start, U32 fixup);


U32 aseg_reref(U32 old, U32 new);
