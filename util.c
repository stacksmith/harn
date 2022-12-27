/* Load a single self-contained ELF object file and link */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "global.h"

void* hd_line(void* ptr){
  U8*p=(U8*)ptr;
  printf("%p ",p);
  for(int i=0;i<16;i++){
    printf("%02X ",p[i]);
  }
  for(int i=0;i<16;i++){
    printf("%c",isprint(p[i])?p[i]:128);
  }
  printf("\n");
  return(p+16);
}

void* hd(void*ptr,int lines){
  for(int i=0;i<lines;i++){
    ptr = hd_line(ptr);
  }
  return ptr;
}


#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261


U32 string_hash(char*p){
  U32 hash = FNV_OFFSET_BASIS;
  U8 c;
  while((c=*p++)){
    hash = (U32)((hash ^ c) * FNV_PRIME);
  }
  return hash;
}




typedef struct sFileMap {
  void* addr;
  union {
    size_t size;
    U32 err;
  }  ;
} sFileMap;
/* -------------------------------------------------------------
   file_map   map a file into memory
 -------------------------------------------------------------*/
S64 file_map(void** pbuf, char* path,int prot){
  int fd = open(path, O_RDONLY);
  printf("fd = %08x\n",fd);
  if(fd<0) return -1;
  
  off_t len = lseek(fd, 0, SEEK_END);
  if(len<0){
    close(fd);
    return -2;
  }

  void*ptr = mmap(0, len, prot, MAP_PRIVATE, fd, 0);
  close(fd);
  close(fd);
  if(!ptr) {
    return -3;
  }

  *pbuf = ptr;
  return len;
  
}
void unmap(void* buf,size_t size){
  munmap(buf,size);
}
static char* msg[] = {
  "opening %s\n",
  "seeking end of %s\n",
  "mapping %s\n"
};
  

void file_map_error_msg(S64 id,char*extra,U32 exitcode){
  fprintf(stderr,"Error ");
  fprintf(stderr,msg[abs(id)],extra);
  if(exitcode)
    exit(exitcode);
}

  
  //    printf(
  //    printf(
/* -------------------------------------------------------------
   file_load into a malloc'ed buffer
 -------------------------------------------------------------*/
#include <errno.h>
char* filebuf_malloc(char* path,FILE** pf){
  FILE* f = fopen(path,"r");
  printf("f: %p\n",f);
  if(!f) {
    printf("Could not fopen,%d\n",errno);
    return 0;
  }
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char* buf = (char*)malloc(len+1);
  size_t r = fread(buf,1,len,f);
  if(r!=len) {
    printf("Could not fread,%d\n",errno);
    fclose(f);
    free(buf);
    return 0;
  }
  //  int closed = fclose(f);
  //if(closed){
  //  printf("Could not fclose,%d\n",errno);
  //}
  buf[len]=0;
  if(pf)
    *pf = f;
  return buf;
  
}

void filebuf_free(char* buf){
  free(buf);
}
