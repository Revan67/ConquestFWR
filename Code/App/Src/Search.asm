comment %
//--------------------------------------------------------------------------//
//                                                                          //
//                             SEARCH.ASM                                   //
//                                                                          //
//--------------------------------------------------------------------------//

    $Header: /Conquest/App/Src/Search.asm 7     3/31/00 2:01a Jasony $
			    
//-------------------------------------------------------------------
%

.386
.model FLAT, SYSCALL

.data
DRET	DD	0		; return address for DUMP::bomb() and DUMP::alert_box() methods

.code

$T27306	DQ	0bff921fafc8b007ar		; -90degrees (-1.57radians)
$T27307	DQ	03ff921fafc8b007ar		; 90degrees (1.57radians)



;;---------------------------------------------------------------
;;---------------------------------------------------------------
;; const DWORD * __fastcall dwordsearch (DWORD len, DWORD value, const DWORD * buffer);
;; RETURNS a pointer to matching DWORD, or NULL if no match occurred
;;
;; ECX = length of buffer (in DWORDS)
;; EDX = value to compare against
;; [ESP+4] = address of buffer
;;
?dwordsearch@@YIPBKKKPBK@Z			PROC
		or	ecx,ecx
		cld
		mov	eax, edx		;; eax = value to find
		je  Fail
		mov	edx, edi		;; save edi
		mov	edi, [esp+4]	;; edi = address of buffer

		repne	scasd
		lea	eax,[edi][-4]
		mov	edi,edx			;; restore edi
		je	Success
Fail:
		xor	eax,eax
ALIGN 4
Success:
		ret	4

?dwordsearch@@YIPBKKKPBK@Z	ENDP
;;---------------------------------------------------------------
;; void * __fastcall unmemchr (const void * ptr, int c, int size);
;; RETURNS a pointer to first non-matching BYTE, or NULL if no non-match occurred
;;
;; ECX = address of buffer
;; EDX = value to compare against
;; [ESP+4] = length of buffer (in BYTES)
;;
?unmemchr@@YIPAXPBXHH@Z			PROC
		cld
		mov	eax, edx		;; eax = value to find
		mov	edx, edi		;; save edi
		mov	edi, ecx		;; edi = address of buffer
		mov	ecx, [esp+4]	;; ecx = length

		repe	scasb
		lea	eax,[edi][-1]
		mov	edi,edx			;; restore edi
		jne	Success
		xor	eax,eax
ALIGN 4
Success:
		ret	4

?unmemchr@@YIPAXPBXHH@Z			ENDP
;--------------------------------------------------------------------
; [esp+4] = ptr
; [esp+8] = size in bytes
;
_clearmemFPU	proc
		
		mov	ecx,[esp+8]
		mov	edx,[esp+4]
		shr	ecx,5			;; must be aligned on 32 bytes
		fldz
		je	Done
		dec	ecx
		je	Done
ALIGN 4
movloop:
		fst	QWORD PTR [edx+0]
		mov	eax,[edx+32]
		fst	QWORD PTR [edx+16]
		fst	QWORD PTR [edx+8]
		fst	QWORD PTR [edx+24]
		dec	ecx
		lea	edx,[edx+32]
		jg	movloop
Done:
		fst	QWORD PTR [edx+0]
		fst	QWORD PTR [edx+16]
		fst	QWORD PTR [edx+8]
		fstp	QWORD PTR [edx+24]	;; pop FP stack
		ret
_clearmemFPU 	endp
;--------------------------------------------------------------------
; double __fastcall get_angle (double *x, double *y);
; return atan(x/y)
; ecx -> x,  edx -> y
;
?get_angle@@YINPAN0@Z PROC NEAR				; get_angle, COMDAT

 		fld	QWORD PTR [ecx]
 		fld	QWORD PTR [edx]
		fldz
 		fcomp 
		fnstsw	ax
		sahf
		je	ZeroDivide
		; else assume everything ok
		fpatan
		ret
ALIGN 4
ZeroDivide:
		fcompp			; 0 - x , pop everything off
		fnstsw	ax
		sahf
		jnc	ZeroYNegX
		; else positive X
		fld	QWORD PTR $T27307	; 90
		ret
ZeroYNegX:
		fld	QWORD PTR $T27306	; -90
		ret
?get_angle@@YINPAN0@Z ENDP				; get_angle
;--------------------------------------------------------------------
;--------------------------------------------------------------------
; SINGLE __stdcall get_angle (SINGLE x, SINGLE y);
; return atan(x/y)
; [esp+4] -> x,  [esp+8] -> y
;
?get_angle@@YGMMM@Z PROC NEAR			; get_angle

 		fld	DWORD PTR [esp+4]
 		fld	DWORD PTR [esp+8]
		fldz
 		fcomp 
		fnstsw	ax
		sahf
		je	ZeroDivide
		; else assume everything ok
		fpatan
		ret 8
ALIGN 4
ZeroDivide:
		fcompp			; 0 - x , pop everything off
		fnstsw	ax
		sahf
		jnc	ZeroYNegX
		; else positive X
		fld	QWORD PTR $T27307	; 90
		ret	8
ZeroYNegX:
		fld	QWORD PTR $T27306	; -90
		ret 8
?get_angle@@YGMMM@Z ENDP

;--------------------------------------------------------------------
;--replace standard C float-to-long routine--------------------------
;--------------------------------------------------------------------

__ftol  PROC NEAR
	lea	ecx,[esp-8]
	sub	esp,16		; allocate frame
	and	ecx,-8		; align pointer on boundary of 8
	fld	st(0)			; duplicate FPU stack top
	fistp	qword ptr[ecx]
	fild	qword ptr[ecx]
	mov	edx,[ecx+4]		; high dword of integer
	mov	eax,[ecx]		; low dword of integer
	test	eax,eax
	je	integer_QNaN_or_zero

arg_is_not_integer_QNaN:
	fsubp	st(1),st		; TOS=d-round(d), 	{ st(1)=st(1)-st & pop ST }
	test	edx,edx		; check the sign of the integer
	jz		positive		
; number is negative
					; dead cycle
					; dead cycle
	fstp	dword ptr[ecx]	; result of subtraction
	mov	ecx,[ecx]		; dword of difference (single precision)
	add	esp,16
	xor	ecx,80000000h
	add	ecx,7fffffffh	; if difference > 0, then increment integer
	
	adc	eax,0			; inc eax (add CARRY flag)
	ret

positive:
	fstp	dword ptr[ecx]	; result of subtraction
	mov	ecx,[ecx]		; dword of difference (single precision)

	add	esp,16
	add	ecx,7fffffffh	; if difference < 0, then decrement integer
	sbb	eax,0			; dec eax (subtract CARRY flag)
	ret

integer_QNaN_or_zero:
	test	edx,7fffffffh
	jnz	arg_is_not_integer_QNaN
	fstp st(0)
	fstp st(0)
	add	esp,16
	ret
__ftol ENDP

;--------------------------------------------------------------------
; reverse memory copy
;--------------------------------------------------------------------

_rmemcpy  PROC NEAR

		push	esi
		push	edi
		mov		ecx, DWORD ptr [esp+20]
		mov		esi, DWORD ptr [esp+16]
		mov		edi, DWORD ptr [esp+12]
		lea		esi, [esi][ecx][-1]
		std											;; set the direction flag
		lea		edi, [edi][ecx][-1]
		rep		movsb
		cld											;; clear the direction flag
		pop		edi
		pop		esi
		ret

_rmemcpy  ENDP






		END

;--------------------------------------------------------------------
;---------------------------END Search.asm---------------------------
;--------------------------------------------------------------------
