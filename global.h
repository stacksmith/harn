#include <sys/types.h>
#include <stdint.h>

typedef uint8_t   U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int32_t  S32;
typedef int64_t  S64;
// a convenient way to return data and size in a single U64
typedef struct sBounds {
  union {
    U64 value;
    struct {
      U32 addr;
      U32 size;
    };
  };
} sBounds;

#define  BOUNDS(address,size) (sBounds){.addr=(address),(size)}
#define  BOUNDS_V(val) (sBounds){.value=(val)}
#define  BOUNDS_0 (sBounds){.value=0}

// a resolver function to lookup symbol addresses
typedef U64 (*pfresolver)(char* name);

#define PTR(type,val) ( (type)(U64)(val) )
#define X32(val) ( (U32)(U64)(val) )
#define THE_U32(it) ((U32)(U64)(it))

#define ROUND8(val) (((THE_U32(val))+7) & 0xFFFFFFF8)
/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/
