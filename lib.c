#include <stdio.h>
#include <stdlib.h> //exit
#include <string.h>
#include <dlfcn.h>


#include "global.h"
#include "util.h"

#include "elf.h"
#include "unit.h"
/* lib.c

A library is a kind of a unit containing a set of bindings to a linux DSO.

To create a libunit, provide a text file containing cr-separated lines:
* library name
* binding names, each cr-terminated
...
Each name must be cr-terminated, with no extra
A text file containing cr-terminated lines, each containing the 
name of a symbol that can be resolved in the library, is used to
create a proper ELF-like stringtable containing
0 libname 0 bindings... 0

*/
// load a text file containing cr-terminated names, and
// create a proper nametable
char* lib_load_names(char*path,U32*pcnt){
  char* srcbuf;
  S64 len = file_map((void**)&srcbuf,path,PROT_READ);
  if(len<0) file_map_error_msg(len,path,1);

  char* buf = (char*)malloc(1+len);
  char* p = buf;
  *p++ = 0;
  memcpy(p,srcbuf,len);

  // now, replace crs with 0, and count them
  U32 i = 0;
  while((p = strchr(p,10))){
    *p++ = 0;
    i++;
  }
  *pcnt = i;
  
  return buf;	 
}

sUnit* lib_make(char* dllpath,char*namespath){
  printf("Ingesting dll %s, names in %s\n",
	 dllpath,namespath);
  U32 symcnt;
  char* strings = lib_load_names(namespath,&symcnt);
  //  printf("loaded %d names\n",symcnt);
  void* dlhan = dlopen(dllpath,RTLD_NOW);
  if(!dlhan){
    fprintf(stderr,"Unable to open dll %s\n",dllpath);
    exit(1);
  }
  sUnit* pulib = (sUnit*)malloc(sizeof(sUnit));
  unit_lib(pulib,dlhan,symcnt,strings);
  return pulib;
}

