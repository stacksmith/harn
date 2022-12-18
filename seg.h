/* ==============================================================

A fillable area for code or data

  ==============================================================*/
// segment header at the start of each segment
typedef struct sSeg {
  U32 fill;
  U32 end;
  U32 srchlist;
  U32 rel_flag; 
} sSeg;

// check which seg
#define MFILL_ADDR ((U32*)0xC0000000L)
#define MFILL (*MFILL_ADDR)


#define SMETA_BASE 0xC0000000
#define SMETA_SIZE 0x10000000

#define MTOP_ADDR ((U32*)0xC0000004)
#define MTOP (*MTOP_ADDR)
#define SRCH_LIST_ADDR ((U32*)0xC0000008)
#define SRCH_LIST (*SRCH_LIST_ADDR)
#define REL_FLAG  (*(U32*)(U64)(0xC000000C))

#define IN_DATA_SEG(addr) (0x80000000 == ((addr) & 0xC0000000))
#define IN_CODE_SEG(addr) (0x40000000 == ((addr) & 0xC0000000))
#define IN_META_SEG(addr) (0xC0000000 == ((addr) & 0xC0000000))
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)
void rel_mark(U32 pos,U32 kind);


void mseg_dump();
void mseg_alloc();
void mseg_reset();
//void seg_serialize(sSeg* psg,FILE* f);
//void seg_deserialize(sSeg* psg,FILE* f);


void mseg_align8();
U8*  mseg_append(U8* start,U32 size);



U32 seg_reref(sSeg*psg,U32 old,U32 new);
