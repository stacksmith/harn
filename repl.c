#include <stdio.h>
#include <stdlib.h> //system
#include <string.h>
#include <ctype.h> // for isspace
#include "global.h"
#include "util.h"
#include "elf.h"
#include "seg_meta.h"
#include "seg_art.h"
//#include "pkgs.h"
#include "src.h"
#include "sym.h"
#include "pkg.h"


sSym* ingest_elf(char* name);



//U64 ing_elf_func(sElf* pelf);

extern sSeg* psMeta;


typedef void (*fpreplfun)(char*p);
typedef U64 (*fpvoidfun)();
// bogus, careful...
void exec_repl_sym(sSym* sym,char*p){
  U32 addr = sym->art;
  if(IN_CODE_SEG(addr)){
    //    printf("found %s\n",funname);
    fpreplfun entry = (fpreplfun)(U64)(addr);
    (*entry)(p);
  } else {
    printf("can't run data %s\n",SYM_NAME(sym)); 
  }
}


void run_repl_fun(U32 hash,char*p){
  sSym* sym = ns_find_hash(GNS_AS_SYM,hash);
  if(sym){  //
    exec_repl_sym(sym,p);
  } else {
    printf("run_repl_fun: can't find it\n");
  }
}


/*----------------------------------------------------------------------------
compile

------------------------------------------------------------------------------*/

sSym* repl_compile(char*p){
    pkgs_dump_protos();
  sSym* sym =  ingest_elf("unit");
  if(sym){ // OK, this means we made it!
    printf("Ingested %s: %s %d bytes\n", SYM_NAME(sym),sym_proto(sym),sym->size);
    pkg_incorporate(sym);
  } else { // no symbol
    printf("aborted.\n");
  }
  return sym;

}  

void edit(char* name){
  printf("Editing [%s].. type 'cc' to compile when done...\n",name);
  /*
  siSymb* symb =  pkgs_symb_of_name(name);
  // write headers for packages in use
  pkgs_dump_protos();

  siSymb_src_to_body(symb);
  */
}



char linebuf[1024];

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

/*----------------------------------------------------------------------------
  cmd_hash                  parse and hash a presumed command, updating pptr

A command is an identifier, terminated by a anything below ascii '0'

----------------------------------------------------------------------------*/
U32 cmd_hash(char** ppstart){
  U32 hash = FNV_OFFSET_BASIS;
  U8 c;
  char*p = *ppstart;
  while((c=*p++) && (c>='0')){
    hash = (U32)((hash ^ c) * FNV_PRIME);
  }
  *ppstart=p-1;
  return hash;
}

char* cmd_ws(char*p){
  while(isspace(*p))
    p++;
  return p;
}
//if(0xC3589F16 == hash){

void repl_list(char* p){
  U32 hash = cmd_hash(&p);
  sSym* sym = ns_find_hash(GNS_AS_SYM,hash);
  if(sym){
    puts("-------------------------------------------------------------");
    //    printf("In package: %s\n",SYM_NAME(pkg));Xx
    puts("-------------------------------------------------------------");
    src_to_file(sym->src,stdout); //todo: src length...
    puts("-------------------------------------------------------------");
  }
}



void repl_words(char* p){
  pkg_dump(TOP_PKG);

   //..  .   sSym* q  = PTR(sSym*,SRCH_LIST);
   // q = pkgs_find_hash(q,0xA52bCAF9);  //0xA52bCAF9);//    0xa94d67e5);
   // printf("%p. final\n",q);
  /*
  sSym* q  = PTR(sSym*,SRCH_LIST);
  
  q = PTR(sSym*,0xC0000010L);//q->art);
  //  q = pkg_walk2(q,0x1234,proc2);
  q = pkg_walk(proc1,q,0xA52bCAF9);

  printf("%p. final\n",q);
  
  
  q = pkg_find_hash(q,0xA52bCAF9);  //0xA52bCAF9);//    0xa94d67e5);
  printf("found %p\n",q);
  */
  /*
  sSym* proc(sSym* s,U64 hash){
    return ((s->hash == hash)) ?  s : 0;
  }
  sSym* qqq = apkg_walk2( PTR(sSym*,SRCH_LIST),
			 0xa94d67e5,
			 proc);
  */
  //  sSym* qqq = pk_find_hash1( PTR(sSym*,SRCH_LIST),0xa94d67e5,0,0 );
  //   sSym* qqq = pk_find_hash2( PTR(sSym*,SRCH_LIST),0xa94d67e5 );
  //  if(qqq)    printf("helo %p %08x\n",qqq,qqq->hash);
  //else    printf("0.\n");
}

void repl_pkg(char* p){
  if(!strncmp(p,"ls",2)) {
    ns_pkgs_ls();
    return;
  }
  if(!strncmp(p,"use",3)) {
    p = cmd_ws(p+3);
    U32 hash = cmd_hash(&p);
    sSym* prev =PTR(sSym*,ns_find_prev_pkg_hash(GNS_AS_SYM,hash));
    if(prev && (GNS_AS_SYM != prev)) { // are we already there?
      sSym* pkg = pkg_unlink(prev); // unlink prev's next and get it.
      if(pkg)
	ns_pkg_push(pkg);  // and relink it in front
    }
    ns_pkgs_ls();
  }
}

 
void repl_help(char*p){
  FILE*f = fopen("sys/help_page.txt","r");
  char c;
  while((c = fgetc(f)) != EOF )
    putchar(c);
  fclose(f);
}

U64 ingest_run(char* name);

void repl_expr(char*p){
  FILE*f = fopen("sys/cmdbody.c","w");		
  fputs("void command_line(){\n ",f);
  fputs(p,f);
  fputs("\n}\n",f);
  fclose(f);

  ingest_run("commandline");
}

void repl_dump(char*p){
  char* pend;
  U64 addr = strtol(p,&pend,16);
  //  printf("addr is %08lX\n",addr);
  if('r'== *pend)
    addr>>=3;
  if(addr<0xFFFFFFFF){
    hd((void*)addr,4);
  }
}

void repl_save(char*p){
  FILE* f = fopen("image/image.dat","w");
  seg_serialize(DFILL,f);
  seg_serialize(CFILL,f);
  seg_serialize(MFILL,f);
  fclose(f);
}


void repl_load(char*p){
  FILE* f = fopen("image/image.dat","r");
  seg_deserialize_start(DATA_SEG_ADDR,8,f); //load CFILL and DFILL
  seg_deserialize(DFILL,f,8);
  seg_deserialize(CFILL,f,0);

  seg_deserialize_start(META_SEG_ADDR,4,f); //load MFILL
  seg_deserialize(MFILL,f,4);
  fclose(f);
  //TODO: automate search for libraries to rebind upon load
  printf("repl_load is assuming bad things %px\n",TOP_PKG);
  pkg_dump(PTR(sSym*,(TOP_PKG->art)));
  pkg_rebind(PTR(sSym*,(TOP_PKG->art)));
  //pk_rebind( ((sSym*)(U64)SRCH_LIST),"libc.so.6");
}

void repl_edit(char*p){
  sSym* sym = ns_find_name(GNS_AS_SYM,p);
  FILE*f = fopen("sys/body.c","w");
  if(sym){
    src_to_file(sym->src,f);
  }
  fclose(f);
  puts("edit and enter 'cc' when done\n");
}
/*---------------------------------------------------------------------------

  ----------------------------------------------------------------------------*/
void repl_sys(char* p){\
  printf(ANSI_MAGENTA "    DATA     CODE     META\n" ANSI_RESET);
  printf("%08X %08X %08X\n",DFILL,CFILL,MFILL);
  //pkgs_list();
}


/*

sSym* ingest_elf_test(char* name);

sSym* repl_test1(char*p){

  sSym* sym =  ingest_elf_test(p);
  if(sym){ // OK, this means we made it!
    printf("Ingested %s: %s %d bytes\n", SYM_NAME(sym),sym_proto(sym),sym->size);
    pkg_incorporate(sym);
  } else { // no symbol
    printf("aborted.\n");
  }

  return sym;

}

sSym* repl_test(char*p){
  char buf[128];
  for (int i=0;i<50000;i++){
    sprintf(buf,"%s%05d",p,i);
    repl_test1(buf);
  }
  return 0;
}

*/







/*----------------------------------------------------------------------------
  Get a command and execute.
  ... ;     compile and evaluate as a C expression
  <cmd>     if recognized as a special REPL command, do that
            otherwise, try to find hash and execute in-system  
----------------------------------------------------------------------------*/

void repl_loop(){
  pkgs_dump_protos();
  puts(ANSI_CYAN "harn - an interactive C environment.  (c) 2022 StackSmith\n\n\
      ...try 'help', or 'printf(\"Hello World!\\n\");'\n" ANSI_RESET);
  while(1) {
    printf(ANSI_GREEN "> "ANSI_RESET);
    fgets(linebuf,1024,stdin);

    char*p=linebuf;
    U32 hash = cmd_hash(&p); // and advance to ws
    //    printf("Hash: %08X, len %d\n",hash,(U32)(p-linebuf));
    //    U32 cmdlen = (p-linebuf);
    // if terminated by a (, compile line and execute as a function
    if(strchr(p,';')) { repl_expr(linebuf); continue ; }

    // skip any whitespace to parameters, and remove the final cr
    p = cmd_ws(p);
    { size_t len = strlen(linebuf);
      *(linebuf+len-1)=0;
    }

    if(!strncmp("cc",linebuf,2))  { repl_compile(p);continue; }
    //    if(!strncmp("tt",linebuf,2))  { repl_test(p);continue; }
    if(!strncmp("sys",linebuf,3)) { repl_sys(p);    continue; }
    // ox56299123
    // 5CB7AA8A,
    if(!strncmp("ls",linebuf,2)){ repl_words(p); continue; }
    //if(!strncmp("edit",linebuf,4)){ repl_edit(p);  continue; }

    if(!strncmp("help",linebuf,4)){ repl_help(p); continue; }
    if(!strncmp("hd",linebuf,2)){ repl_dump(p); continue; }
    if(!strncmp("list",linebuf,4)){ repl_list(p);  continue; }
    if(!strncmp("pkg",linebuf,3)){ repl_pkg(p);  continue; }

	
    if(!strncmp("save",linebuf,4)){ repl_save(p); continue; }
    if(!strncmp("load",linebuf,4)){ repl_load(p); continue; }

    // user typed in the name of a function to run as void
    // 0x71F39F63
    if(!strncmp("bye",linebuf,3)) exit(0);

    // try to run it as a repl function
    run_repl_fun(hash,p);
 
  }
}
    /*
    if(!strncmp("edit",linebuf,4)){
      edit(linebuf+5);
      system("vim sys/body.c");
      compile();
      pkgs_dump_protos();
      continue;
    }
    */
