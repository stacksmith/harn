TOP = ..

CC=gcc

CFLAGS= -Wall -Os -std=c99 \
-Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation  \
-fcf-protection=none    \
-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables \
-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
-fno-stack-protector  -fno-mudflap 

cfiles= harn.c elf.c elfdump.c util.c src.c repl.c sym.c pkg.c \
elf_ingester.c seg_common.c seg_art.c seg_meta.c  

all: harn

tsv2names: tsv2names.c
	$(CC) -o $@ $^ $(CFLAGS) 	

seg_asm.o:	seg_asm.asm
	nasm -f elf64 -l seg_asm.lst -o seg_asm.o seg_asm.asm

pkg_asm.o:	pkg_asm.asm
	nasm -f elf64 -l pkg_asm.lst -o pkg_asm.o pkg_asm.asm


harn: $(cfiles) seg_asm.o pkg_asm.o *.h
	$(CC) -o $@ seg_asm.o pkg_asm.o $(cfiles) $(CFLAGS) 

clean:
	rm harn harn.o *.o *~
 
