#ifndef SEG_COMMON_DEFS
#define SEG_COMMON_DEFS
/* ============================================================================

There are 3 segments at fixed locations:

0x40000000 CODE_SEG - executable code artifacts (and related constants);
0x80000000 DATA_SEG - global variables, CODE_SEG and DATA_SEG fill-pointers
0xC0000000 META_SEG - metadata

 ============== ==============================================================*/
#define CODE_SEG_ADDR 0x40000000
#define CODE_SEG_SIZE 0x10000000
#define DATA_SEG_ADDR 0x80000000
#define DATA_SEG_SIZE 0x10000000
#define META_SEG_ADDR 0xC0000000
#define META_SEG_SIZE 0x10000000


/*-----------------------------------------------------------------------------
  Data segment keeps the fill-pointers for both data and code segments -- to 
  avoid writing into code segment...   DO NOT PUT IN METADATA!
  Meta fill-ptr is in meta...
-----------------------------------------------------------------------------*/
#define CFILL_ADDR ((U32*)0x80000004L)
#define DFILL_ADDR ((U32*)0x80000000L)
#define CFILL (*CFILL_ADDR)
#define DFILL (*DFILL_ADDR)
#define DFILL_PTR (PTR(U8*,DFILL))
#define CFILL_PTR (PTR(U8*,CFILL))

/*-----------------------------------------------------------------------------
  Meta segment vars
  * MFILL
  * GNS  global namespace
  * REL_FLAG

-----------------------------------------------------------------------------*/
#define MFILL_ADDR ((U32*)0xC0000000L)
#define GNS_ADDR ((U32*)0xC0000008L)
#define REL_FLAG_ADDR  ((U32*)0xC000000CL)

#define MFILL (*MFILL_ADDR)
#define GNS (*GNS_ADDR)
#define TOP_PKG (PTR(sSym*,GNS))
#define GNS_AS_SYM (PTR(sSym*,META_SEG_ADDR))
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
U32 seg_serialize(U32 fillptr,FILE*f);

U32 seg_deserialize(U32 fillptr,FILE*f,U32 already);
U32 seg_deserialize_start(U32 addr,U32 bytes,FILE*f);

typedef struct sCons {
  U32 cdr;
  U32 car;
} sCons;

#endif
