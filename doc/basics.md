# Basics

## Segments

Segments are large, managed areas of memory for keeping code, data, and metadata.  Code segment is for executable code, data segment is used for storage of static variables, strings, and other data used by code.  The Meta(data) segment exists exclusively for keeping additional information about what's in the code and data segments.

## Artifacts

An artifact is a contiguous and indivisible region of code or data in its appropriate segment.  Artifacts are basic units of storage and operation. A single compiled function, a variable, or a string of characters are examples of artifacts.  Artifacts occupy space in their respective segments, and each artifact has a unique address, and a non-zero size (in bytes).

Artifacts exist exclusively in code and data segments.  The Meta segment contains iformation about artifacts and provides the means of tracking, operating on, and reasoning about artifacts.  Artifacts exist separately from the metadata and are not aware of metadata.  The Meta segment, however is all about tracking artifacts.

## Segments and References

In addition to allocating memory, segments provide a crucial service of managing references.  A reference is a pointer or an offset stored in a segment -- such as a pointer variable, or a jump to a function.  Segments track all refences in the system.

All references must be registered with the segment in which they exist.  This is taken care by the ELF ingester, and is generally transparent.

## Relocation and Referential Integrity

At certain times it is necessary to remove artifacts that are not in use and compact the segment.  Segments are capable of making sure that all references remain correct after such movement.  

Segments can answer questions such as: who is calling a function or accessing a variable.

Segments can assist in updating all calls to an old version of a function to a new one.

## Symbols

Every artifact has a piece of metadata known as a symbol bound to it.  A symbol is a named entity which exists exclusively to provide access to and information about its artifact ; A symbol is created at the same time as the associated artifact, and continues to exist for the entire lifespan of the artifact.  

At the very minimum, every symbol provides a name, information about the artofact's position and size.  Usually symbols provide the means of obtaining detailed information about the nature of the artifact (such as the source code and build data used to create it) and crucial information about how to use the artifact in various contexts.

## Names and Packages

For convenience, symbols are grouped into lists called packages.  For example, at initialization, harn creates a package "libc" containing bindings to the libc library -- to enable functions like 'printf' and 'putc'.  The system also creates a 'user' package for user code.

The system keeps a list of packages known as the Global Namespace.  The frist package in the namespace is considered 'active', and new compiles are placed into it.

## REPL

The REPL provides a number of built-in commands for examining and manipulating the system.  As of today it supports:
```
 bye               exit
 cc                compile sys/unit.c; please edit sys/body.c first
 hd XXXXXXXX       hexdump data at hex address
 hd XXXXXXXXr      hexdump rel data for hex address
 edit <name>       put the source of symbol into sys/body.c
 help              displays this message
 list <name>       show the source of the symbol
 load              load the image file
 save              save the image in image/image.dat file
 sys               show system statistics
 words             show the symbols in the active package
 pkg ls            show packages; the first one is active
 pkg use <name>    makes the named package active (and pulls it to top)
```

