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
#define REL_FLAG (*(U32*)(U64)(0xC000000C))
#define IN_DATA_SEG(addr) (0x80000000 == ((addr) & 0xC0000000))
#define IN_CODE_SEG(addr) (0x40000000 == ((addr) & 0xC0000000))
#define IN_META_SEG(addr) (0xC0000000 == ((addr) & 0xC0000000))
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)

void seg_dump(sSeg* psg);
sSeg* seg_alloc(U64 req_size, void* req_addr, U32 prot);
void seg_reset(sSeg* sg);
void seg_serialize(sSeg* psg,FILE* f);
void seg_deserialize(sSeg* psg,FILE* f);

U32  seg_pos(sSeg* psg);

void seg_align(sSeg*psg, U64 align);
U8*  seg_append(sSeg* psg,U8* start,U64 size);

void seg_rel_mark(sSeg*psg,U32 pos,U32 kind);


U32 seg_reref(sSeg*psg,U32 old,U32 new);
