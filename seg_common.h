#ifndef SEG_COMMON_DEFS
#define SEG_COMMON_DEFS
/* ==============================================================

Segments are at constant locations

  ==============================================================*/
#define CODE_SEG_ADDR 0x40000000
#define CODE_SEG_SIZE 0x10000000

/*-----------------------------------------------------------------------------
  Data segment keeps the fill-pointers for both data and code segments -- to 
  avoid writing into code segment...

-----------------------------------------------------------------------------*/

#define DATA_SEG_ADDR 0x80000000
#define DATA_SEG_SIZE 0x10000000

#define DFILL_ADDR ((U32*)0x80000000L)
#define CFILL_ADDR ((U32*)0x80000004L)
#define CFILL (*CFILL_ADDR)
#define DFILL (*DFILL_ADDR)
#define DFILL_PTR (PTR(U8*,DFILL))
#define CFILL_PTR (PTR(U8*,CFILL))

/*-----------------------------------------------------------------------------
  Meta segment:
  * MFILL
  * SRCH_LIST
  * REL_FLAG

-----------------------------------------------------------------------------*/
#define META_SEG_ADDR 0xC0000000
#define META_SEG_SIZE 0x10000000

#define MFILL_ADDR     ((U32*)0xC0000000L)
#define SRCH_LIST_ADDR ((U32*)0xC0000008)
#define REL_FLAG_ADDR  ((U32*)0xC000000C)

#define MFILL (*MFILL_ADDR)
#define SRCH_LIST (*SRCH_LIST_ADDR)
#define TOP_PKG (PTR(sSym*,SRCH_LIST))
#define WALK_SRCH_LIST (PTR(sSym*,META_SEG_ADDR))
// Danger zone: the new symbol-space walkers for pks and plst need to start
// with meta seg base!  They will immediately indirect at +8, which happens
// to be SRCH_LIST...  That will also provide PREV for unlinkin..

#define REL_FLAG  (*REL_FLAG_ADDR)

#define MFILL_PTR (PTR(U8*,MFILL))

#define IN_DATA_SEG(addr) (0x80000000 == ((addr) & 0xC0000000))
#define IN_CODE_SEG(addr) (0x40000000 == ((addr) & 0xC0000000))
#define IN_META_SEG(addr) (0xC0000000 == ((addr) & 0xC0000000))
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)
void rel_mark(U32 pos,U32 kind);

void* seg_mmap(void* addr, U64 size, U64 PROT, char* which);

typedef struct sCons {
  U32 cdr;
  U32 car;
} sCons;

#endif
