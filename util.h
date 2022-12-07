void* hd_line(void* ptr);
void* hd(void*ptr,int lines);

U32 string_hash(char*p);


S64 file_map(void** pbuf, char* path,int prot);
void file_map_error_msg(S64 id,char*extra,U32 exitcode);

#ifndef PROT_READ
  #define PROT_READ 1
  #define PROT_WRITE 2
#endif

