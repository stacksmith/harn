        section .data		; Data section, initialized variables

        section .text           ; Code section.

;;; rdi = base address
;;; rsi = bit to set
	
	global bit_set1
	global bit_set2
	global bits_set
	global bit_test
	global bit_cnt

bit_set2:
	bts [rdi],rsi
	inc rsi
bit_set1:
	bts [rdi],rsi
	ret

;;; rdi = base
;;; rsi = bit
;;; rdx = cnt
bits_set:
	bts [rdi],rsi
	inc rsi
	dec rdx
	jnz bits_set
	ret
	
	
bit_test:
	xor	eax,eax
	bt	[rdi],rsi
	sbb	eax,eax
	ret
	
bit_cnt:
	dec	rsi		;previous bit must be 0!
	bt  	[rdi],rsi
	sbb	eax,eax		;-1 if set, 0 if clear
	js      .x
	dec	eax
.loop:	
	inc	rsi
	inc	eax
	bt	[rdi],rsi
	jc	.loop
.x:	ret
	



	
