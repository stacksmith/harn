# HARN
`harn` is a (work-in-progress) fine-grained interactive linker (toy) for a (subset of) C, licensed under GPL v3.0.  What can you do with it (eventually)?  Edit and compile C functions (or data objects), one at a time; execute your own or any libc function from the command line...
```
harn - an interactive C environment.  (c) 2022 StackSmith

      ...try 'help', or 'printf("Hello World!\n");'
```

Harn is an experimental research platform and is not meant to be a production environment in the near future.

Status: early proof of concept; allows for command-line execution of functions; adding functions to the symbol table by editing and compiling; recompiling existing functions replaces older versions and relinks all references; save and load image...

### Project goals:
`harn` is an attempt to create a REPL-like environment for editing, recompiling, and relinking and executing individual functions and manipulating data objects, from inside a running process.  

The system aims to enable interactive C code development à la Lisp or Forth, blending the coding, debugging, and testing into a single environment.

It is a non-goal to create a full-scale real linker, preserve any C semantics, or comply with any standards or acceptable norms. `harn` is built to operate on a subset of C compiled in a very specific way.

### Approach:

`harn` maintains the bookkeeping data and manages the `coding-time` referential integrity and garbage collection, keeping all code hot and functions individually accessible. 

The approach tried here is to keep linked and relocated code in an image-like environment complete with sources and allow incremental re-compilation of functions by harnessing gcc.  The system maintains enough individual state (prototypes, headers, etc) to concoct function-level compilation units and raids the resultant ELF object file.

The system futher maintains relocation metadata of the ingested artifacts, allowing it to move artifacts around, replace artifacts on demand, and delete unreferenced artifacts.  The metadata allows us to further reason about artifacts and their interaction with each other.

For details, see the doc directory.

### Technical Limitations

C is not an ideal language for this kind of interaction, and some hard issues are being resolved.  It is a goal of this project to use a reasonable subset of C and an unmodified gcc to produce the artifacts.

The system allows editing and recompilation of sub-file units, namely code fragments containing a single global object such as a function or a variable.  The resulting artifact is stored, along with source and other metadata.

Each artifact must be either functional, with a single global function (and possibly additional local functions and read-only data, or a global data object.

Functions are fully described by the headers automatically-extracted during compilation and made available to the rest of the system.  Data objects are potentially much more complicated, both in terms of access (unlike functions with a single entry point and generally-trivial callsites, data objects may be accessed in many unanticipated ways), structure, and pervasiveness in multiple functions - potentially requiring recompilation of many artifacts upon change...

Current handling of data is largely manual, requiring careful usage of headers for declaring types and global variables.




