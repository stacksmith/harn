// smSymb.h

// smSymbs are symbols stored in the meta segment.


typedef struct sSym {
  U32 cdr;               // fixed up upon local moves only
  U32 hash;
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

#define SYM_NAME_OFF 22
#define SYM_NAME(sym)  (((char*)(sym))+SYM_NAME_OFF)
#define SYM_BYTES(sym) (((sym)->octs)<<3)
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
void sym_dump1(sSym* sym);


//U32 sym_delete(sSym* sym);

U32 sym_del(sSym* sym);
sSym* sym_for_artifact(char* name,U32 art); // called by elf ingetsor


