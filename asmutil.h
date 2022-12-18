extern U32 bits_set(U64 bit,U32 count);
extern U32 bit_test(U64 bit);
extern U32 bits_cnt(U64 bit);
// step through references, starting with bit past top offset;
// each call returns high=bitcnt  low = offset of ref
extern U64 bits_next_ref(U32 top,U32 bottom);

extern U32 bits_reref(U32 top,U32 bottom,U32 old,U32 new);
extern U32 bits_drop(U32 start,U32 end,U32 unused,U32 count);

// Dropzone addresses are _before_ the drop!
//
// fix region that has not been moved
extern U32 bits_fix_outside(U32 top,U32 bottom,
			    U32 dropzone_start, U32 dropzone_end,
			    U32 fixup);
// fix region that has been moved
extern U32 bits_fix_inside(U32 top,U32 bottom,
			   U32 top1, U32 fixup);


// fixup area above dropzone (top to hole)
extern U32 bits_fix_hi(U32 top,U32 unused,U32 hole,U32 size);
// fixup a hole in the meta segment (special!)
extern U32 bits_fix_meta(U32 top, U32 bottom,
			 U32 hole,U32 size,
			 U32 arthole, U32 artholesize);
