#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "util.h"
#include "src.h"

char* srcbuf;
U32 srcbuf_size=0x10000;

extern FILE* faSources; // append-only
extern FILE* fSources;  // random-access
char* srcname =  "image/sources.txt";
char* bodyname = "sys/body.c";
char* auxname =  "sys/info.txt";

void src_init(){
  srcbuf =  (char*)malloc(srcbuf_size);
  faSources = fopen(srcname,"a");
  fSources = fopen(srcname,"a+");
  if(!(faSources && fSources)){
    fprintf(stderr,"src_init: unable to open source file %s\n",srcname);
    exit(1);
  }
}
/* src_from_body    Extract presumed source from 'body' file, and append it
                    to the sources log.  Return sources file position and len

 */
U32 src_from_body(U32* plen){
  // read source
  FILE* f = fopen(bodyname,"r");
  // if body 
  if(f) {
    src_timestamp();
    size_t len = fread(srcbuf,1,srcbuf_size,f);
    fclose(f);
    srcbuf[len]=0;
    size_t pos = ftell(faSources);
    fprintf(faSources,"/// %05X\n",(U32)len); // 10 characters at +4
    fputs(srcbuf,faSources);
    fflush(faSources);
    *plen = (U32)len;
    return (U32)pos;
  } else {
    *plen = 0;
    return 0;
  }
}
void src_to_file(U32 pos,FILE*f){
  if(pos){
    fseek(fSources,pos,SEEK_SET);  // seek in sources
    fread(srcbuf,1,4096,fSources);  // read source
    U64 len = strtol(srcbuf+4,0,16);
    if(len>srcbuf_size) {
      printf("len is messed up!\n");
      return;
    }
    if(len>4096) 
      fread(srcbuf+4096,1,len-4096,fSources);
    fwrite(srcbuf+10,1,len,f);     // write to body
  }  
}  
void src_to_body(U32 pos,U32 len){
  FILE* f = fopen(bodyname,"w");
  src_to_file(pos,f);
  fclose(f);
}


/* aux_proto   for functions, extract the prototype saved on the last line 
               of file the compiler generated with the -aux-info switch.
*/
char* aux_proto(){
  FILE* f = fopen(auxname,"r");
  if(!f){
    fprintf(stderr,"aux_proto: unable to open %s\n",auxname);
    exit(1);
  }

  // load the end of the file, tucked to the top of buf
  fseek(f,0,SEEK_END);
  long pos = ftell(f);
  //  char* beg =buf;
  //  printf("pos: %ld\n",pos);
  if(pos>=1024){
    fseek(f,pos-1024,SEEK_SET);
    fread(srcbuf+1,1,1024,f);
    *srcbuf = 0;
    //beg = buf+1;
  } else {
    fseek(f,0,SEEK_SET);
    fread(srcbuf+1+1024-pos,1,pos,f);
    *(srcbuf+1024-pos) = 0;
    //beg = buf+1+1024-pos;
  }
  // now scan backwards
  char* start = srcbuf+1024-4; // back up past ' */<cr>'
  while(*start != '/') start--; 
  *--start = 0;             // chop off K&R comment
  while(*start != '/') start--; // back up to end of first comment
  start+=2;                     // but skip the / and space
  char* ret = (char*)malloc(1+strlen(start));
  strcpy(ret,start);
  fclose(f);
  return ret;
}
#include "time.h"


time_t srcLastStamp = 0L;
void src_timestamp(){
    time_t my_time = time(NULL);
    if( (my_time - srcLastStamp) > 60 ) {
      fprintf(faSources,"/// %s", ctime(&my_time));
      // fflush(faSources);
      srcLastStamp = my_time;
    }
}
