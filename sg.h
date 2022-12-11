/* ==============================================================

A fillable area for code or data

  ==============================================================*/
// segment header at the start of each segment
typedef struct sSg {
  U32 fill;
  U32 end;
} sSg;

// check which seg
#define SMETA_BASE 0xC0000000
#define REL_FLAG (*(U32*)(U64)(0xC000000C))
#define IN_CODE_SEG(addr) (0x80000000 == ((addr) & 0xC0000000))
#define IN_DATA_SEG(addr) (0x40000000 == ((addr) & 0xC0000000))
#define IN_META_SEG(addr) (0xC0000000 == ((addr) & 0xC0000000))
// to check if two addrs are in the same seg, for instance:
#define SEG_BITS(addr) ((addr) & 0xC0000000)

void sg_dump(sSg* psg);
sSg* sg_alloc(U64 req_size, void* req_addr, U32 prot);
void sg_reset(sSg* sg);
void sg_serialize(sSg* psg,FILE* f);
void sg_deserialize(sSg* psg,FILE* f);

U32  sg_pos(sSg* psg);

void sg_align(sSg*psg, U64 align);
U8*  sg_append(sSg* psg,U8* start,U64 size);

void sg_rel_mark(sSg*psg,U32 pos,U32 kind);


void sg_reref(sSg*psg,U32 old,U32 new);
