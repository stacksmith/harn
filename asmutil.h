extern U32 bits_set(U64 bit,U32 count);
extern U32 bit_test(U64 bit);
extern U32 bits_cnt(U64 bit);
// step through references, starting with bit past top offset;
// each call returns high=bitcnt  low = offset of ref
extern U64 bits_next_ref(U32 top,U32 bottom);

/*----------------------------------------------------------------------------
  bits_drop                     move count bytes from src down to dst
----------------------------------------------------------------------------*/
extern U32 bits_drop(U32 dst,U32 src,U32 count);

/*----------------------------------------------------------------------------
  bits_reref                    scan from top to bottom, replacing all 
                                refs to old with refs to new

  Update all references regardless of type - R32, A32 and A64.
----------------------------------------------------------------------------*/
extern U32 bits_reref(U32 top,U32 bottom,U32 old,U32 new);

/*----------------------------------------------------------------------------
  bits_fix_inside               scan from top to bottom, the span being the
                                dropzone, the range of bytes to be moved down
                                by fixup.  Fix references encountered.

  The entire span is intended to be moved down, and the references are
  processed as follows.  Inside references are contained within the range

  R32     inside      unchanged
  R32     outside     +fixup      since the site is dropping, increase offset
  ABS     inside      -fixup      
  ABS     outside     unchanged
 
----------------------------------------------------------------------------*/

extern U32 bits_fix_inside(U32 top,U32 bottom,  U32 fixup);

extern U32 bits_fix_inside1(U32 dz_start,U32 dz_end,  U32 fixup);


/*----------------------------------------------------------------------------
  bits_fix_outside              scan from top to bottom, the span being the
                                unmoved block of bytes, below the hole, or
                                in a different segment.  Adjust refs into
                                the dropzone by fixup, (logically down)

  R32     inside      -fixup    references into dropzone will be closer
  R32     outside     unchanged
  ABS     inside      -fixup
  ABS     outside     unchanged

Note that the scanned region is where we look for reference sites, while
looking for targets in the dropzone region....
----------------------------------------------------------------------------*/


extern U32 bits_fix_outside(U32 top,U32 bottom,
			    U32 dropzone_start, U32 dropzone_end,
			    U32 fixup);


/*----------------------------------------------------------------------------
  bits_fix_meta                  The meta segment is special: The only 
                                 references allowed are A32.  This routine
 is specifically for fixup of the meta segment during the removal of a header,
 and the implied removal of its artifact.  This allows us to simultaneously
 adjust links within the meta segment and artifact pointers.

 top:       High end of the range scanned.  Low end is C0000000.
 metazone:  start of the dropzone in meta segment.  End is at top!
 fix:       size of hole/drop/fixup amount
 artzone:   start of dropzone in artifact segment.
 artend:    end of dropzone in artifact segment.
 artfix:    references from meta into artzone will be fixed down by this.

 Again, we only fix reference sites inside the meta segment, in preparation
 for the drop at metazone by fix, and in preparation for the drop in an 
 artifact range specified by artfix....

 This is actually an extremely compact, and efficient routine.  It was harder
 to describe is functionality than to write it!
                               
 If we maintain alignment, it may be even easier to do this, perhaps with a
 different kind of a rel table that does not
----------------------------------------------------------------------------*/

extern U32 bits_fix_meta(U32 top, U32 metazone, U32 fix,
			 U32 artzone, U32 artend, U32 artfix);
