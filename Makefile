TOP = ..

CC=gcc

CFLAGS= -Wall -Os -std=c99 \
-Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation  \
-fcf-protection=none    \
-fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables \
-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 \
-fno-stack-protector  -fno-mudflap 

cfiles= harn.c elf.c elfdump.c util.c src.c repl.c sym.c pkg.c elf_ingester.c \
seg_common.c seg.c aseg.c 

all: harn

tsv2names: tsv2names.c
	$(CC) -o $@ $^ $(CFLAGS) 	

asmutil.o:	asmutil.asm
	nasm -f elf64 -l asmutil.lst -o asmutil.o asmutil.asm


harn: $(cfiles) asmutil.o *.h
	$(CC) -o $@ asmutil.o $(cfiles) $(CFLAGS) 

clean:
	rm harn harn.o *.o *~
 
