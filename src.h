/* src.h

The system maintains a log of all compiled sources.  Symbols carry an offset to the source file, as well as the size of the source. 

*/
void src_init();
U32 src_from_body(U32* plen);
void src_to_file(U32 pos,U32 len,FILE*f);
void src_to_body(U32 pos,U32 len);
char* aux_proto();                    //allocates!
