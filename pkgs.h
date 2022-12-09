/* pkgs

we keep a linked list of packages


 */
void pkgs_add(sPkg* pkg);
void pkgs_list();
siSymb* pkgs_symb_of_name(char* name);
void pkgs_dump_protos(FILE* f);
