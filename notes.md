## 
`repl_compile` invokes the compiler build script, which incorporates the user function or data unit contained in `unit.c`.  The compiler leaves us with an ELF object file, and an info.txt file with headers (the last one will match the compiled function, if it was a function). 

`repl_compile` invokes `elf_ingester.c`:`ing_elf` to process the ELF object file, pull the data into an artifact segment (data or code as appropriate), and create a sSym, (see `sym_for_artifact`).  Everything is ready to integrate the symbol into the system.


`repl_compile` then passes the symbol to `pk.c` `pk_incorporate` for integration into the system and the search chain.  If the symbol is new (not found), `pk_incorporate` simply attaches it to the current package.

If a symbol with the same name exists, it is verified against the new symbol to make sure the new symbol is compatible - it must be in the same segment (code or data), and have a compatible signature....  Assuming success, the following takes place:
* The new symbol replaces the old one in the search chain. `pk_incorporate`
* All refs to old artifact are updated to the new one.     `aseg_reref`
* The old symbol and associated artifact are deleted.      `sym_del`

The deletion process is a rather complicated task, as it requires fixups and compaction across three segments.  On success, the number of fixups is reported, and the new variable or function is integrated into the system; the new version of the function or variable is magically connected instead of the old one.










This was a fail, the catdog segment was endless trouble!
```
It would actually make more sense to not have 3 uniform segments, but instead keep a combinded data/code segment growing in different directions (say, data down, code up).  When a hole is created, a single sweep can process it.  
```
In the end I wound up with 3 normalish segments, each a little different...


Segments, Artifacts, References, Symbols and Packages

Segments

Segments are large, managed areas of memory for keeping data and metadata.
There are three segments: code, data, and metadata.  Code segment is for 
executable code, data segment is used for storage of static variables,
strings, and other data used by code.  The Meta(data) segment exists 
exclusively for keeping track of data in code or data segments.

Artifacts

An artifact is a contiguous and indivisible region of code or data in
it appropriate segment.  Artifacts are basic units of operation. A single 
compiled function, a variable, or a string of characters are examples of 
artifacts.  Artifacts occupy space in their respective segments, and 
each artifact has a unique address, and a non-zero size (in bytes).

Artifacts exist exclusively in code and data segments.  The Meta segment 
contains iformation about artifacts and provides the means of tracking, 
operating on, and reasoning about artifacts.  Artifacts exist separately 
from the metadata and are not aware of metadata - the code and data 
segments contain no references to the meta segment.  The Meta segment,
however is all about tracking artifacts. Meta segment contains numerous 
references into code and data segments.

Symbols

Every artifact has a piece of metadata known as a symbol bound to it. 
 A symbol is a named entity which exists eclusively to provide access to 
and information about its artifact ; A symbol is created at the same time 
as the associated artifact, and continues to exist for the entire lifespan 
of the artifact.  At the very minimum, every symbol provides a name, 
information about the artofact's position and size.  Usually symbols 
provide the means of obtaining detailed information about the nature of 
the artifact (such as the source code and build data used to create it) and 
crucial information about how to use the artifact in various contexts.

Segments and References

In addition to providing separate spaces for stoing artifacts and metadata, 
segment provide a crucial service of managing memory.

Segments are pools of memory for storage of artifacts and symbols, but 
segments are not aware of their contents other than providing basic 
and primitive allocation services and keeping a 'fill-pointer'  Segments 
are filled from the bottom up, and upon request, will provide a contiguous 
region of memory.

In addition to allocating memory, segments provide a crucial service of 
managing refernces.  A reference is a kind of a pointer from one allocated 
memory location to another.  References occupy space, which is used to 
encode information about another allocated memory position in the same or
different segment.  Segments keep detailed information about reference: 
each reference site, the size of the reference, and the reference type is
known to the segment.  

Segment know the details of references and are capable of visiting every 
reference site, decypher the reference data, and deterine its target -- 
all without knowing what these references are for.  References are 
registered with the associated segment as needed, and segments provide
services for traversing and identifying each reference, for reasons 
explained later.

Code artifacts are the main users of references: every pointer variable, 
absolute branch, function call is a reference.  However, references need 
not be in code - any stored address of an object in the system is a 
reference: every symbol carries the location of its artifact - an inter-
segment reference. Symbols are generally kept in a linked list structure
 in the metasegment, and each symbol contains a reference to the next
 symbol in the list.

Currently, the system uses three kinds of references: (R32) 32-bit relative 
offsets (used extensively in x86 code), (A64) - 64-bit absolute references, 
used mainly as pointer variables, and (A32) 32-bit absolute references 
used by the system for tasks such as linking symbols together, or referring 
to objects inside the system, (as it located in a 32-bit memory space).

Referential Integrity

The requirement to register references with segments seems draconian to 
the uninitiated, but in practice, it is not much of an issue.  Code objects,
for instance, are generally ingested from ELF object files which contain 
relocation information, and such data is automatically added to the 
appropriate segments.  Any data entities generated in the normal course of
opepration - the creation of artifacts and symbols, automatically registers
all references created in the process.  Any tool that is a part of the 
system will create references transparently, and while it is crucial to
understand how they work, it very rare unusual to have to create them
manually.

The reason for, and the consequence of, keeping references is, that we are 
able to move memory around - mostly for compaction, while keeping every 
reference pointing to its intended target.  This is a unique quality of 
the system, known as referential integrity.

Every function called from anywhere within the system, every variable 
referenced by code, and every reference connecting symbols to each other 
and their artifacts, is guaranteed to be properly targeted, regardless of 
where the constituent parts are located!

Memory Compaction

As described earlier, segments do not know about their contents: such 
details are handed by other components, but are capable of moving memory 
when requested.  In practice this happens at clearly specified times, 
such as when objects are deleted, triggering a memory compaction.  

In practice, segment memory is allocated starting at the bottom, and 
segments grow in the upward direction.  Deleted regions may occur 
anywhere in the allocated area, and compaction closes the formed gaps by 
dropping the memory from above.  The system strives to keep its memory 
contiguous, and eliminate wasted space.

Fragile Moments

While memory is in motion and references are being updated, the system's
referential integrity is compromised.  This is not a big problem because 
the system operates at human speed - it is not a general-purpose allocation
system, but a way to clean up after largely manual tasks such as recompiling
 code  or deleting objects.  The compaction happens at specific times, 
halting the system.  In practice it should not be oservable in any meaningful 
way.

# Referential Services

In addition to providing referential integriyt, the reference subsystem 
provides a valuable window into the connectivity of various componets, 
and sometimes, offers a unique way to work with the system.  For instance, 
it is trivial to check if a particular function is being called, from 
anywhere in the system!  The system can easily identify every call site, 
and provide information about who is using what artifacts or parts of the
system.

This allows the system to be defensive and resilient - the system will 
not allow the deletion of a function if there are callers - minimizing 
dangling references.

The system oversees many tasks that are currently risky and require a 
monumental effort and often result in broken systems.  For example, the 
system can assist in updating all references to a particular object - be 
it a piece of code, or a managed table of resources - to another one 
(obviously it must be compatible).  Some tasks are obvous in effect, but 
are highy unusual.  For instance, recompilation of a functions results in 
the newly compiled function immediately integrated (if so desired), as the
old version is freed from references and available for deletion (if so 
desired). 

Live code objects

Today's systems are generally not capable of hot-patching code as a 
standard procedure and rely on crutches such as batch compilation and 
linking.  Even dynamic loaders and fixup engines present in todays 
operating systems are limited to fixing up linkage to specific libraries 
at specific times, and require clumsy PLT and GOT tables.  This leads to
code bloat and complexity, and particular methodologies for creating 
code for such particular cirumstances.  

This system, on the other hand, provides transparent managment of
references once created, and regular code becomes pliable.  In fact, 
many opportunities for smaller and faster code are available: instead of
allocating memory indirectly, it possible to allocate and place objects
at absolute addresses - should the data be moved, all references will 
be taken care of.  This works well with persistance services provided 
by the system.

Combined with the metadata stored in the system, referential data provides
unprecedented access to code.  Instead of working with large, unknowable 
and unmanageable blobs, the system provides a fine-grained access to the 
components and the ability to incrementally rearrange the environment.





otherwise known as an artifact.  
the data segment.Symbols are named entities used for  associated with some artifact in either data or
code segment.   Symbols themselves are variable-sized datastructures
located in the metadata segment.  S exist exclusively in the metadata segment, and 
are generally connected using the internally-provided cdr link pointer.

A package is a conceptual group of symbols.  Packages are 

A pkg is a is a special symbol used to act as a head of a chain of symbols:
known as a package.  A pkg has a name but no data associated with and it
its datasize is 0.  A pkg provides an initial cdr link to a chain of 
symbols, but nothing is ever linked to a symbol using the cdr pointer.

The artifact pointer of a pkg is (mis)used to form a special linked list of
pkgs.  So pkgs are linked together horizontally, and each one start its
own chain of symbols.  

Sometimes it is convenient to think of an entire chain of symbols with 
a pkg at the head as a 'package'.

Searching

Symbols exist to track and name artifacts.  Searching by name is a 
basic capability of the system.  Symbol names should be unique at least
within a package.  To facilitate searches, each symbol, in addition to
a name string, has a 32-bit hash.  Searching for hashes is substantially 
easier.

Searching can be localized to a particular package, or be performed 
across the global search space by successively visiting and sdescending 
into packages of a srchlist.

Name and Search semantics

The specially-linked list of PKG heads and the symbol lists associated with
them make up the global namespace.  Topologically, the packages form a 
two-level system.  The top level consists of named PKG heads linked 
into a list using their art pointers.  Each PKG, using its cdr pointer,
is a start of a linked list of named symbols.

It is important to think of the two tiers as entirely different.  The 
PKG heads exist exclusively to act as head for a group of symbols.
pointer to form a linked list their size is always set to zero, and they 
never have an artifact associated with them.

Each package is a linked list of symbols, with a pkg acting as a head. 
It is important to understand that although the pkg represents a package, 
or a searchable list of symbols, it itself is not considered a part of
the package!


and it is performed by traversing the 
srchlist of pkgs, and descenting into the symbol chains., and provide names for

Another way to think of Or, you can think of each

Together this srchlist is used to provide naming services and track 
artifacts.   There is a global srchlist

, and the artifact is used to link pkgs together into a
search-list.  
/*----------------------------------------------------------------------------

----------------------------------------------------------------------------*/

This walker starts with a package and immediately skips it without any
operation.  This guarantees that every symbol processed always has a prev
symbol, and the walker provides prev to allow easy unlinking.

U32 pkgfun(cSym* s, cSym*prev, U64 param) ; along these lines...
	;; 		 

Be careful declaring the called function!  It is tempting to use a void*
in the walker setup, and it will generally work, but note that gcc has 
given me hours of grief because I called a function return a U8 flag --
which made it leterally 6 bytes to compare hash!  But the walker exits on
rax currently, and the function set a byte, giving me intermittent failures
depending on who screwed with rax...

This walker starts with a package and immediately skips it without any
operation.  This guarantees that every symbol processed always has a prev
symbol, and the walker provides prev to allow easy unlinking.

U32 pkgfun(cSym* s, cSym*prev, U64 param) ; along these lines...
	;; 		 

Be careful declaring the called function!  It is tempting to use a void*
in the walker setup, and it will generally work, but note that gcc has 
given me hours of grief because I called a function return a U8 flag --
which made it leterally 6 bytes to compare hash!  But the walker exits on
rax currently, and the function set a byte, giving me intermittent failures
depending on who screwed with rax...
ols in a package, and
           do something with them.
By the way, with wrapper functions like these, make sure parameter lists
match as much as possible, otherwise functions get really big shuffling
registers.   Moved parameters pf pkg_walkly to keep pkg and parm as 1 and2 ;
now will move the invoked proc parameters to match:
(pkg,parm,prev) because next is pretty rare, really...

  Looking at pkg_walk: packages always start with head, which is not a part
  of the package nameset.  We enter with a reference to the head, and 
  immediately thread through to the first symbol.  This gives us the head
  as the PREV during iteration, so we can link in front of the first sym.

  Here we work on heads only, but what do we do with the first one? We 
  don't really need a useless head that just takes up space and gets skipped ;
  the initial reference comes from SRCH_LIST and we want to search the first
  head we get.  So how do we link in?

  As luck would have it, SRCH_LIST happes to be located at offset 8 from 
  the bottom of meta segment, and all heads are linked using the art ptr,
  which is located at offset 8 of each head!

  So we can pretend the bottom of the meta segment is the first head, and
  link in at offset 8.  Scary. but as long as we PROMISE to do nothing else
  with the PREV parameter except for linking at +8, it is safe.

	 Q
 . 

  The top level is for organization purposes, and allows us to arrange the
  search order, etc.  The vertical 
    PKG represent a package, or a a chain of symbols ; the P
  *

	;; 


pkg_walk - This is a generic rotine to visit symbols in a package, and
           do something with them.  We start with a PKG as the root of
 the symbol linked list.  The PKG is nota part of the package! and   We skip it upon entry..

The canonical usage is walking a package looking for a hash sent as
 a paramete,r witha callback:

sSym* proc1(sSym* s,U32 hashm, sSym*prev, ){
  return (s->hash == hash) ? (U64)(prev) : 0;
}
q = pkg_walk(q,0xA52bCAF9,proc1) ;

This will find the symbol with a matching hash and return the previous 
symbol, allowing us to unlink it.  

