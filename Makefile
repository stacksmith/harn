TOP = ..

CC=gcc

CFLAGS= -Wall -O2 -std=c99 -fno-strict-aliasing -Wno-pointer-sign -Wno-sign-compare -Wno-unused-result -Wno-format-truncation -Wno-stringop-truncation 

cfiles= harn.c elf.c elfdump.c seg.c util.c pkg.c src.c pkgs.c repl.c

all: harn

tsv2names: tsv2names.c
	$(CC) -o $@ $^ $(CFLAGS) 	

asmutil.o:	asmutil.asm
	nasm -f elf64 -l asmutil.lst -o asmutil.o asmutil.asm


harn: $(cfiles) asmutil.o *.h
	$(CC) -o $@ asmutil.o $(cfiles) $(CFLAGS) 

clean:
	rm harn harn.o *.o *~
 
