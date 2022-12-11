#include <stdio.h>
#include <stdlib.h> //system
#include <string.h>
#include <ctype.h> // for isspace
#include "global.h"
#include "util.h"
#include "elf.h"
#include "seg.h"
#include "pkg.h"
#include "pkgs.h"
#include "src.h"

extern sSeg scode;
extern sSeg sdata;

extern sPkg* pkgs;


typedef void (*fpreplfun)(char*p);
typedef U64 (*fpvoidfun)();

void exec_repl_symb(siSymb* symb,char*p){
  U32 addr = symb->data;
  if(IN_CODE_SEG(addr)){
    //    printf("found %s\n",funname);
    fpreplfun entry = (fpreplfun)(U64)(addr);
    (*entry)(p);
  } else {
    printf("can't run data %s\n",symb->name);
  }
}

void run_repl_fun(U32 hash,char*p){
  siSymb* symb = pkgs_symb_of_hash(hash);
  if(symb){  // symb is known to be a symb
    exec_repl_symb(symb,p);
  } else {
    printf("can't find it\n");
  }
}





void edit(char* name){
  printf("Editing [%s].. type 'cc' to compile when done...\n",name);
  siSymb* symb =  pkgs_symb_of_name(name);
  // write headers for packages in use
  pkgs_dump_protos();

  siSymb_src_to_body(symb);
}



siSymb* compile(void){
  siSymb* symb=0;
  int ret = system("cd sys; ./build.sh");
  if(!ret){ // build shell-out successful:
    // create an elf object
    sElf* pelf = elf_load("sys/test.o"); 
    Elf64_Sym* psym = elf_unique_global_symbol(pelf);
    if(psym){ // We identified the global symbol in the ELF file.
      // do we already have a symbol with same name?  Hold onto it.
      siSymb* oldsymb = pkgs_symb_of_name(ELF_SYM_NAME(pelf,psym));
      // load the entire ELF trainwreck, and add symb to our
      symb = pkg_load_elf(pkgs,pelf,psym); // topmost pkg
      if(symb){ // OK, this means we are done with ingestion.
	printf("Ingested %s: %s %d bytes\n",
	       symb->name,symb->proto,symb->size);
	// Now that the new object is in our segment, is there an 
	if(oldsymb) { // older version we are replacing?
	  printf("old version exists\n");
	  // first, make ssure it's in the same seg.
	  if( (SEG_BITS(oldsymb->data)) == (SEG_BITS(symb->data))){
	    // yes, both are in the same seg.  Fix old refs
	    seg_reref(&scode,oldsymb->data,symb->data); // in code seg
	    seg_reref(&sdata,oldsymb->data,symb->data); // in data seg
	  } else { // No, different segs.  Nothing good can come of it.
	    fprintf(stderr,"New one is in a different seg! Abandoning!\n");
	    symb = pkg_drop_symb(pkgs); // drop new object from topmost pkg
	  }
	} // else a new symbol, no problem
      } else {
	printf("ELF not ingested due to unresolved ELF symbols.\n");
      }
      pkgs_dump_protos();
    } else {
      printf("One and only one ELF global symbol must exist.  Abandoning\n");
    }
    // finally, delete the elf data
    elf_delete(pelf);
  }else { 
    printf("OS shellout to compiler failed! Build Error %d\n",ret);
  }
  return symb;
}
siSymb* repl_compile(char*p){
  return compile();
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
void repl_sys(char* p){
  seg_dump(&scode);
  seg_dump(&sdata);
  pkgs_list();
}

void repl_list(char* p){
  U32 hash = cmd_hash(&p);
  siSymb* symb = pkgs_symb_of_hash(hash);
  if(symb){
    src_to_file(symb->src,symb->srclen,stdout);
  }
}

void repl_words(char* p){
  pkg_words(pkgs);
}

void repl_help(char*p){
  printf("commands:\n sys  words  bye  edit <name>  list <name>\n");
  printf(".<expr><cr> to execute an expression.  For instance:\n");
  printf(".puts(\"Hello, world\");<cr>\n\n");
  
}

void repl_expr(char*p){
  FILE*f = fopen("sys/body.c","w");		\
  fputs("void command_line(void){\n ",f);
  fputs(p,f);
  fputs("\n}\n",f);
  fclose(f);
  siSymb* symb = compile();
  if(symb) {
    exec_repl_symb(symb,p);
    pkg_drop_symb(pkgs);
  }
}


/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

void repl_loop(){
  pkgs_dump_protos();

  while(1) {
    printf("> ");
    fgets(linebuf,1024,stdin);

    char*p=linebuf;
    U32 hash = cmd_hash(&p); // and advance to ws
    printf("Hash: %08X, len %d\n",hash,(U32)(p-linebuf));
    U32 cmdlen = (p-linebuf);
    // if terminated by a (, compile line and execute as a function
    if('('==*p) { repl_expr(linebuf); continue ; }

    // skip any whitespace to parameters, and remove the final cr
    p = cmd_ws(p);
    { size_t len = strlen(linebuf);
      *(linebuf+len-1)=0;
    }

    if(!strncmp("sys",linebuf,3)) { repl_sys(p);    continue; }
    // ox56299123
    if(!strncmp("cc",linebuf,2))  { repl_compile(p);continue; }
    // 5CB7AA8A,
    if(!strncmp("words",linebuf,4)){ repl_words(p); continue; }
    
    if(!strncmp("list",linebuf,4)){ repl_list(p);  continue; }

    if(!strncmp("help",linebuf,4)){ repl_help(p); continue; }

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