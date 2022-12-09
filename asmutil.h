extern U32 bit_set1(void*p, U64 bit);
extern U32 bit_set2(void*p, U64 bit);
extern U32 bits_set(void*p, U64 bit,U32 count);
extern U32 bit_test(void*p,U64 bit);
extern U32 bits_cnt(void*p,U64 bit);
// step through references, starting with bit past top offset;
// each call returns high=bitcnt  low = offset of ref
extern U64 bits_next_ref(void*p,U64 bit);
