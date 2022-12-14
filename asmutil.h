extern U32 bit_set1(U64 bit);
extern U32 bit_set2(U64 bit);
extern U32 bits_set(U64 bit,U32 count);
extern U32 bit_test(U64 bit);
extern U32 bits_cnt(U64 bit);
// step through references, starting with bit past top offset;
// each call returns high=bitcnt  low = offset of ref
extern U64 bits_next_ref(U32 top,U32 bottom);

extern U32 bits_reref(U32 top,U32 bottom,U32 old,U32 new);
