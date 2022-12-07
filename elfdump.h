
char* get_str(sElf* pelf, Elf64_Shdr* sh_str, int idx);
void sechead_dump(sElf* pelf, U32 sh_idx);
void secheads_dump(sElf* pelf);

void reltab_dump(sElf*pelf,U32 isec);
void rel_dump(sElf*pelf,Elf64_Rela* p);

void sym_dump(sElf*pelf,Elf64_Sym* psym);
void symtab_dump(sElf*pelf);



void elf_dump(sElf* pelf);
 
