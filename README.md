# HARN
`harn` is a fine-grained interactive linker (toy) for a (subset of) C.

Unlike normal linkers and loaders, which drop the ball upon creating an executable image, `harn` is a dynamic environment, complete with managed code and data segments, symbol tables, function prototypes, sources, and a relocation engine for moving code and data objects around while maintaining referential integrity.

`harn` provides REPL-like environment allowing editing, recompiling, and relinking individual functions and data objects, one at a time.  Objects are linked into an image, complete with the sources and function prototypes.  Tools may be constructed interactively to manipulate and reason about the components.

The system supports packages, which are groups of symbols and associated code and data, which may be combined as needed into a searchable namespace.

C is not an ideal language for this kind of interaction, and some hard issues are being resolved.  In particular, the compiler allows for reasonably clean extraction of functions and their prototypes.  Data object, however are opaque and dangerous.  Currently, data types must be managed manually by attaching appropriate headers to packages; changes in global data declarations and types require a clear understanding of dependencies.  I am looking into a few different ways of automating dependency tracking.

