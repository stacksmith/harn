#include <sys/types.h>
#include <stdint.h>

typedef uint8_t   U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int32_t  S32;
typedef int64_t  S64;
// a convenient way to return data and size in a single U64
typedef struct sDataSize {
  U32 data;
  U32 size;
} sDataSize;

// a resolver function to lookup symbol addresses
typedef U64 (*pfresolver)(char* name);

/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/
