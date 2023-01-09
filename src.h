/* src.h

The system maintains a log of all compiled sources (also: command lines?).  Symbols carry an offset to the source file, as well as the size of the source. 

Occasionally, timestamps are also inserted.

The source always points at a special comment line:
/// nnnnn ooooooo

This line contains, at the very least, a 5-digit hex size of the sources (not
including the comment line!

It may also contain an offset to the previous version of this entry... (TODO)



*/
void src_init();
U32 src_from_body(U32* plen);
void src_to_file(U32 pos,FILE*f);
void src_to_body(U32 pos,U32 len);
char* aux_proto();                    //allocates!

void src_timestamp();
