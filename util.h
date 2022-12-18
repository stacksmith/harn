void* hd_line(void* ptr);
void* hd(void*ptr,int lines);

U32 string_hash(char*p);
U32 string_hash1(void* ptr, char*str);

S64 file_map(void** pbuf, char* path,int prot);
void file_map_error_msg(S64 id,char*extra,U32 exitcode);
void unmap(void*buf,size_t size);

#ifndef PROT_READ
  #define PROT_READ 1
  #define PROT_WRITE 2
#endif

char* filebuf_malloc(char* path); // mallocs
void filebuf_free(char* buf);
