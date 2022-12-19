        section .data		; Data section, initialized variables

        section .text           ; Code section.

;;; rdi = base address
;;; rsi = bit to set
	global bits_set	        ; (addr,cnt)
	global bit_test
	global bits_cnt
	global bits_next_ref	;
	global bits_reref	; replace old ref to new ref
	global bits_drop	; delete a piece of a segment and rel
	global bits_fix_inside      ; replace refs above hole down by ecx
	global bits_fix_outside
	global bits_fix_meta
;;;--------------------------------------------------------------------------------
;;; bits_set                    Set a zero followed by specified number of bits
;;;
;;; We make inherent assumptions that count is reasonable, and that the rel table
;;; is correctly set to 0 at the site.
;;; 
;;; rdi = addr
;;; rsi = cnt
bits_set:
	xor	eax,eax
.loop:	inc 	edi
	bts 	[rax],rdi
	dec 	esi
	jnz	.loop
	ret
	
;;; rdi = addr
bit_test:
	xor	eax,eax
	bt	[rax],rdi
	sbb	eax,eax
	ret

	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; edx = ptr (address of reference)
;;; ecx = 0
;;; return: count of 1's
bits_next_ref:
	xor	eax,eax		;eax = bit counter
	xor	ecx,ecx		;edx = base(0);

.loop:	dec	edi		;next reference
	cmp	edi,esi
	js      .done          	;exit when below bottom
	
	bt	[rcx],rdi     
	jnc	.loop
 
	;; got a 1!  now count
.loop1:	inc	eax             ; count the bit
	dec     edi             ; previous bit
	bt	[rcx],rdi      
	jc	.loop1
	mov	[rdx],edi	;set caller's pointer
.done:
	ret
%if 0
--------------------------------------------------------------------------------
	REREF               	Replace all references to old with new

	
 edi = addr of top+1,   	;work pointer
 esi = addr of bottom        ;limit 
 edx = old
 ecx = new	 
--------------------------------------------------------------------------------
%endif
bits_reref:
	 push	rbx             ;ebx = count of fixups
	 xor	ebx,ebx
	sub	rcx,rdx         ;compute fixup difference, S64
	jmp	.loop0
;;; R32
.one:	mov	eax,[rdi]	;load 32-bit offset
	lea	eax,[rax+rdi+4] ;convert to abs32
.loopx:	cmp	rax,rdx         ;A64 pointers can point to anything!
	jne	.loop0		;go back to loop; skip this 1 and prev 0
	add	[rdi],ecx       ;fixup (the low part, should work)
	inc     ebx             ;increment fixup count
	
.loop0: xor	eax,eax		;Jump here to clear eaeax = base(0);
.loop:	dec     edi
	cmp	edi,esi        	;the very bottom
	js      .done          	;if offset is 0, exit with 0
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits
 	;; got a 1-0 transition.	
	dec     edi
	bt	[rax],rdi
	jnc	.one		; 0 1 0
	dec	edi
	bt	[rax],rdi
	jnc	.two
	dec	edi
	bt	[rax],rdi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbx
	ret	
;; 0 - 1 - 1 - 0   A32
.two:	mov	eax,[rdi]
	jmp	.loopx
;; 0 - 1 - 1 - 1 - 0   A64 
.three:	mov	rax,[rdi]	;load entire 64-bit pointer
	jmp	.loopx



%if 0
--------------------------------------------------------------------------------
	DROP

When an artifact is eliminated, the resulting hole must be removed, by dropping
the entire area from the top of the hole to the end of the segment.	
--------------------------------------------------------------------------------
%endif

;;; edi = start of hole
;;; esi = end of hole
;;; edx =  bytes to move
bits_drop:
	mov     ecx,edx        	;count
	push	rdi
	push	rsi
	push	rcx

	rep 	movsb		;edi points to new top; esi, old top
	mov	ecx,esi
	sub	ecx,edi	        ;ecx = byte size of hole, for wiping
	mov     edx,ecx         ;edx = byte size of hole
	xor	eax,eax         ;fill top with 0s
	rep     stosb
;;; Now, process rel
	pop	rcx		;restore count of bytes to move
	shr	ecx,3           ;moving bits, not bytes
	pop	rsi             ;source
	shr	esi,3
	pop	rdi             ;destination
	shr	edi,3

	rep 	movsb		;edi points at new reltop, esi, old
	mov	ecx,edx         ;hole size
	shr	ecx,3      
	xor	eax,eax         ;fill top with 0s
	rep     stosb
.x:
	;; 	mov	eax,edx         ;return hole size / amount dropped by
	ret

%if 0
--------------------------------------------------------------------------------
The meta segment is a special case: metadata is in control of other segments...

0110 refs mark artifact pointers - A32

The act of deleting a header implies a deletion of its artifact, and the fixup
can take care of both the local linkage _and_ the artifact pointers.
%endif
	
bits_fix_meta:	
;;; edi = region top+1 ;region bottom is always top & 0xC00000000
;;; esi = metazone start 
;;; edx = metazone drop/fixup
;;; ecx = artzone start 
;;; r8  = artzone drop/fixup
	push    rbx             ;ebx used to synthesize fixup
	push	rbp             ;ebp = count of fixups
	xor	ebp,ebp		;fixup count
	xor	eax,eax
	mov     r9d,ecx
	or      r9d,0x3FFFFFFF  ;r9 is top of art segment
	
.loop: 	dec	edi
	cmp	edi,0xC0000000
	js	.done
	bt	[rax],rdi       ;testing bits
	jnc	.loop		;     1 0 proven
	dec	edi		;   1 1 0 assumed!
	dec	edi             ; 0 1 1 0 assumed
				;
;;; %if 0
	mov	ebx,r8d		;prepare to drop by artzone drop
	cmp	[rdi],ecx	;<bottom?
	cmovb	ebx,eax		;adjust by 0 (avoiding branches)
	cmp	[rdi],r9d       ;> artzone top?
	cmova   ebx,eax         ;adjust by 0
	cmp     [rdi],esi       ;> metazone start?
	cmovae  ebx,edx         ;adjust by meta fixup.
	sub	[rdi],ebx	;fix this artifact pointer by drop or 0

	test 	ebx,ebx		;did we fixup?  TODO: do we really care?
	setnz	bl
	and	ebx,1		;if fixup,1; otherwise 0
	add	ebp,ebx
	
	dec	edi             ; 0 1 1 0  ;may go belof Cxxx, but will
	jmp     .loop           ;       ^  ;test before reading...

.done:	mov	eax,ebp
	pop	rbp
	pop	rbx
	ret


%if 0
/* -------------------------------------------------------------
        fixup_outside  - region is outside dropzone!

Scan a region that has not been moved for refs into a dropzone 
and perform the following fixup:
 * absolute refs into dropzone	-size
 * relative refs into dropzone	-size
 * absolute refs elsewhere       unchanged
 * relative refs elsewhere       unchanged

Technically, can return -1 if too many 1 bits encountered...

 edi = region top+1,   
 esi = dropzone start  (before drop)
 edx = dropzone end    (before drop) 
 ecx = fixup 
fixup	                ;aka hole size ; ; ;
*/
%endif 
bits_fix_outside:
	push	rbx             ;ebx = count of fixups
	xor	ebx,ebx		;fixup count
	push	rbp
	mov	ebp,edi		;scan segment top
	and	ebp,0xC0000000   ;scan segment bottom
	jmp	.loop0
	;; 0 - 1 - 0    R32
.one:	mov	eax,[rdi]	;load R32 reference
	lea	eax,[rax+rdi+4] ;convert to A32
.loopx:	cmp	rax,rsi         ;< dropzone start? A64s can point
	jb	.loop0		;      anywhere at all!
	cmp	rax,rdx		;> dropzone end? skip.
	ja	.loop0          ;= end? will fix up (FILLPTR!)
	inc     ebx             ;increment fixup count
	sub	[rdi],ecx       ;drop ref by fixup
	
.loop0: xor	eax,eax		;Jump here to clear ea = base(0);
.loop:	dec     edi
	cmp	edi,ebp        	;still above region floor?
	js      .done          	;no? done
	bt	[rax],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits; eax is 0
 	;; got a 1-0 transition.	
	dec     edi
	bt	[rax],rdi
	jnc	.one
	dec	edi
	bt	[rax],rdi
	jnc	.two
	dec	edi
	bt	[rax],rdi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbp
	pop	rbx
	ret	

.two:;; 0 - 1 - 1 - 0   A32
	mov	eax,[rdi]	;rax = target... if in dropzone,
	jmp	.loopx          ;will fix up there, same as rel.
	
.three:	;; 0 - 1 - 1 - 1 - 0   A64 
	mov	rax,[rdi]	;load entire 64-bit pointer
	jmp	.loopx

%if 0
/* -------------------------------------------------------------
        fixup_inside - fixup region inside dropzone

Scan the dropzone proper, the region that has been dropped, and
perform the following fixup:
 * absolute refs to dropped area	-size
 * relative refs to dropped area	unchanged
 * absolute refs outside	        unchanged
 * relative refs outside                +size
	;; 
 edi = dropzone start   also, scan-segment bottom
 esi = dropzone end     also, scan-segment top
 edx = fixup
*/
%endif

bits_fix_inside:	
	push	rbx             ;ebx = count of fixups
	mov     ecx,esi         ;ecx = constant top of range
	xor	ebx,ebx		;fixup count
	jmp	.loop0
	;; 0 - 1 - 0    R32     RELATIVE...
.one:	mov	eax,[rsi]	;load 32-bit offset
	lea	eax,[rax+rsi+4] ;convert to abs32
.loopx:	cmp	rax,rdi         ;target < dropzone bottom (A64 possible)
	jb	.fixpl		;fix by +holesize (outside!)
	cmp	rax,rcx		;target >= dropzone? 
	jb	.loop0          ; = should not really happen!
.fixpl:	inc     ebx             ;increment fixup count
	add	[rsi],edx       ;make the offset larger
	
.loop0: xor	eax,eax		;Jump here to clear ea = base(0);
.loop:	dec     esi
	cmp	esi,edi        	;Scan region
	js      .done          	;until end
	bt	[rax],rsi       ;testing bits
	jnc	.loop           ;skipping 0 bits
 	;; got a 1-0 transition.  Now dispatch on number of bits.
	dec     esi
	bt	[rax],rsi
	jnc	.one
	dec	esi
	bt	[rax],rsi
	jnc	.two
	dec	esi
	bt	[rax],rsi
	jnc	.three
;;; more than three 1's, return -1.
	sbb	ebx,ebx		;-1, since C is set...
.done:	mov	eax,ebx 	;return number of fixes
	pop	rbx
	ret	
.three:	;; 0 - 1 - 1 - 1 - 0   A64 (could ref entirely outside!)
	mov	rax,[rsi]	;load entire 64-bit pointer
	jmp	.abs		;same as A32
.two:;; 0 - 1 - 1 - 0   A32
	mov	eax,[rsi]
	;; fix abs references into the dropzone by -fixup
.abs:	cmp	rax,rdi		;abs target < dropzone?
	jb	.loop0          ; no action
	cmp	rax,rcx         ;abs target >= dropzone?
	jae	.loop0          ; no action
	sub     [rsi],edx       ;within drop zone, fix by -hole!
	jmp	.loop0

