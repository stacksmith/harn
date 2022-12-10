        section .data		; Data section, initialized variables

        section .text           ; Code section.

;;; rdi = base address
;;; rsi = bit to set
	
	global bit_set1
	global bit_set2
	global bits_set
	global bit_test
	global bits_cnt
	global bits_next_ref	;
	global bits_reref
;;; edi=addr
bit_set2:	
	xor	eax,eax
	bts 	[rax],rdi
	inc 	edi
;;; edi=addr
bit_set1:
	xor	eax,eax
	bts 	[rax],rdi
	ret

;;; rdi = addr
;;; rsi = cnt
bits_set:
	xor	eax,eax
.loop:	bts 	[rax],rdi
	inc 	rdi
	dec 	rsi
	jnz	.loop
	ret
	
;;; rdi = addr
bit_test:
	xor	eax,eax
	bt	[rax],rdi
	sbb	eax,eax
	ret
;;; rdi = addr
bits_cnt:
	dec	rdi		;previous bit must be 0!
	xor	esi,esi		;esi = base (0)
	bt  	[rsi],rdi
	sbb	eax,eax		;-1 if error
	js      .x              ;if set, we are done, -1=error!
.loop:	
	inc	rdi		;next bit.
	inc	eax		;+1, one ahead
	bt	[rdi],rsi
	jc	.loop
	dec	eax            	;we were at +1, so fix it
.x:	ret

	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; return: high=cnt low=bits at reference
bits_next_ref:
	xor	eax,eax		;eax = bit counter
	xor	edx,edx		;edx = base(0);
	
.loop:	dec	edi
	cmp	edi,esi
	je      .done          	;if offset is 0, exit with 0
	bt	[rdx],rdi
	jnc	.loop
 
	;; got a 1!  now count
.loop1:	inc	eax
	dec     edi             ; previous bit
	bt	[rdx],rdi
	jc	.loop1
	shl	rax,32		; put count into high dword
	inc	edi 		; point at actual ref
	or	rax,rdi		; put offset into low dword;
.done:
	ret
	
;;; edi = addr of top+1,   	;used as bit 
;;; esi = addr of bottom
;;; edx = old
;;; ecx = new
;;; return: high=cnt low=bits at reference
bits_reref:
	push	rbx
	sub	rcx,rdx         ;compute fixup difference, S64
.loop0:	xor	eax,eax		;eax = bit counter
	xor	ebx,ebx		;edx = base(0);
.loop:	dec	edi		;scan bits down, until
	cmp	edi,esi        	;the very bottom
	je      .done          	;if offset is 0, exit with 0
	bt	[rbx],rdi       ;testing bits
	jnc	.loop           ;skipping 0 bits
 	;; got a 1!  now count
.loop1:	inc	eax
	dec     edi             ; previous bit
	bt	[rbx],rdi
	jc	.loop1
	inc	edi 		; point at actual ref
	dec	eax
	jnz	.large
;;; 32-bit offset relocation
	mov	ebx,[rdi]	;load 32-bit offset
	lea	ebx,[rbx+rdi+4] ;convert to pointer
	cmp	ebx,edx
	jne	.loop0		;go back to loop, but reset ebx
	add	[rdi],ecx	;fixup
	jmp	.loop0
;;; 64-bit relocation; edi = address  edx = old
.large: cmp	[rdi],rdx       ;pointing at old?
	jne	.loop0		;no match, go back to loop
	add	[rdi],rcx       ;fixup
	jmp	.loop0

.done:	pop	rbx
	ret



	
