// smSymb.h
/*
  Symbols live in the meta segment and contain enough data to track
artifacts in code or data segment, and maintain a search chain using
cdr link A32 pointer.   

Search chains are headed by packages, which are special symbols that 
point at another package, forming a package list.  Packages have names
just like normal symbols.

Packages are therefore searched by cdr threading; art points at the
next package to search.

 */


typedef struct sSym {
  U32 hash;                // oddly, it's better to have hash at 0 
  U32 cdr;                // see pkg.asm for details, maybe.
  union {
    U64 bounds;
    struct {
      U32 art;               // local, code, or data  data, must fix
      U32 size;
    };
  };
  U32 src;                // file offset
  U8  octs;               // total size in blocks of 8.
  U8  namelen;
} sSym;
// null-terminated name follows;
// null-terminated signature follows;
// padded to align8, so octs accounts for this space...

#define SYM_NAME_OFF 22
#define SYM_NAME(sym)  (((char*)(sym))+SYM_NAME_OFF)
#define SYM_BYTES(sym) (((sym)->octs)<<3)
#define SYM_CDR(sym) (PTR(sSym*,(sym)->cdr))
#define U32_SYM(usym) ((sSym*)(U64)(usym))

// name
// 0
// prototype
// 0
//smSymb* smSymb_new(char* name,U32 data,U32 size);
//void    smSymb_set_src(smSymb* symb,U32 src, U32 srclen);
//void    smSymb_set_proto(smSymb* symb,char* proto);
sSym* sym_new(char* name, U32 data, U32 size, U32 src,char* proto);
char* sym_proto(sSym* sym);
U32 sym_dump1(sSym* sym);


//U32 sym_delete(sSym* sym);

U32 sym_del(sSym* sym);
sSym* sym_for_artifact(char* name,U32 art); // called by elf ingesror


