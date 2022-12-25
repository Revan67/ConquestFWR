;/*****************************************************************************
; *
; * Copyright (c) 1996 ADVANCED MICRO DEVICES, INC. All Rights reserved.
; *
; * This software is unpublished and contains the trade secrets and
; * confidential proprietary information of AMD. Unless otherwise
; * provided in the Software Agreement associated herewith, it is
; * licensed in confidence "AS IS" and is not to be reproduced in
; * whole or part by any means except for backup. Use, duplication,
; * or disclosure by the Government is subject to the restrictions
; * in paragraph(b)(3)(B)of the Rights in Technical Data and
; * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
; * Software owned by Advanced Micro Devices Inc., One AMD Place
; * P.O. Box 3453, Sunnyvale, CA 94088-3453.
; *
; *****************************************************************************
; *
; * AMDSIN.ASM
; *
; * AMD3D sine, cosine and sin/cosine math function.
; *****************************************************************************
	TITLE	AMDsin.asm
	.586
        .K3D
.model  FLAT
PUBLIC  _AMD3D_sin
CONST	SEGMENT
FOUROPI DD	03fa2f983r			; 1.27324
XMAX	=	046c90fdbH			; 25735.9
XMIN	=	039e8977eH			; 0.0004436336
MASKP	DD	07fffffffh
	DD	0

HEADTAIL EQU    $
PIO4HEAD DD     0bf490000r                      ; -0.785156
PIO4TAIL DD     0b97daa22r                      ; -0.000241913
PIO4	DD	03f490fdbr			; 0.785398
	DD	03f490fdbr
MO5642	EQU 	$
MO56	DD	0bc924925r			; -0.0178571
MO42	DD	0bcc30c31r			; -0.0238095
ONE	DD	03f800000r			; 1
	DD	03f800000r
MO3020	EQU	$
MO30	DD	0bd088889r			; -0.0333333
MO20	DD	0bd4ccccdr			; -0.05
MO1206	EQU	$
MO12	DD	0bdaaaaabr			; -0.0833333
MO6	DD	0be2aaaabr			; -0.166667
MO2	DD	0bf000000r			; -0.5
	DD	0				; filler
CONST	ENDS

_TEXT	SEGMENT



_AMD3D_sin      PROC NEAR
;       mmx_single AMD3D_sin(mmx_single arg);

; 	arg in LH of mm0, result in LH of mm0, HH of mm0 undefined

; 	returns 0.0 if the abs(arg) is greater than 8192 * pi
;	returns arg if abs(arg) < 0.0004436336: precise enough

;	otherwise reduces arg to [0; pi/4) using two-step algorithm
;	(from "Software Manual for the Elementary Functions" by Cody, Waite)
;	for higher precision.

;	Let abs(arg) == N * pi/4 + x; 0 <= x < pi/4
;	Result := sign(arg) * (N & 4 ? -1 : 1)
;		case N mod 4 of
;			0: sin(x);
;			1: cos(pi/4 - x);
;			2: cos(x);
;			3: sin(pi/4 - x);
;		endcase
;
; sin(x) is approximated with x - x^3/3! + x^5/5! - x^7/7! ==
;	x * (1 - x^2 / 6 * (1 - x^2 / 20 * (1 - x^2 / 42) ) )

; cos(x) is approximated with 1 - x^2/2! + x^4/4! - x^6/6! + x^8/8! ==
;	1 - x^2 / 2 * (1 - x^2 / 12 * (1 - x^2 * 30 * (1 - x^2 / 56) ) )

; The cosine is computed similarly.

; The average execution time, assuming one mispredict out of two,
; is 41 + 6 = 47 cycles. Precision within [0, 2*pi] range is
; about 22 bits.

	movd	eax, mm0	; 1
        pand    mm0, qword ptr MASKP

	mov	ebx, eax	; 2
	and	ebx, 80000000h

	xor	eax, ebx	; 3
	cmp	eax, XMAX

	movd	mm2, FOUROPI    ; 4
	jl	x2

	pxor	mm0, mm0	; 5
	ret
x2:
	cmp	eax, XMIN	; 5
	movq	mm1, mm0

	jg	x3		; 6

	ret                     ; 7
x3:
	pfmul	mm0, mm2	; 7	; y = x * ( 4 / pi)
        movq    mm3, qword ptr HEADTAIL

        movq    mm5, qword ptr ONE ; 8

        pf2id   mm0, mm0           ; 9     ; y = int(y)
        movq    mm7, qword ptr MO5642   ;

        movq    mm6, qword ptr MO3020   ; 10

	movd	ecx, mm0        ; 11
	pi2fd	mm0, mm0		; y = float(y)

	mov	edx, ecx        ; 12

	punpckldq mm0,	mm0	; 13
	and	edx, 4

	shl	edx, 29		; 14
	pfmul	mm0, mm3	; 	; y = headtail * y

	xor	ebx, edx	; 15
        movq    mm3, qword ptr PIO4       ;

	pfadd	mm1, mm0	; 16	; x = x - head(y)

	punpckhdq mm0,	mm0     ; 17
	test	ecx, 1		;

	pfadd	mm1, mm0	; 18  	; x = x - tail(y)

;	stall

	punpckldq mm1, mm1	; 20
	jz	x5              ;	assume predicted non-taken

	pfsubr	mm1, mm3	; 21	; x = pi/4 - x
x5:
	mov	edx, ecx        ; 22
	shr	edx, 1

	movq	mm2, mm1	; 23	; x; x
	pfmul	mm1, mm1	;  	; y = x * x

	xor	ecx, edx        ; 24
        movq    mm4, qword ptr MO1206

	pfmul	mm7, mm1	; 25	mm7 = -y / 56,42

	pfmul	mm6, mm1	; 26   	mm6 = -y / 30,20

	pfadd	mm7, mm5        ; 27	mm7 = z1 = 1 - y / 56,42
	pfmul	mm4, mm1  	;     	mm4 = -y / 12,6

;	stall

	pfmul	mm7, mm6	; 29   	mm7 = -y * z1 / 30,20
        movq    mm6, qword ptr MO2

;	stall

	pfadd	mm7, mm5	; 30	mm7 = z2 = 1 - y * z1 / 30,20
	pfmul	mm6, mm1        ;	mm6 = -y / 2

;	stall

	pfmul	mm4, mm7	; 32	mm4 = -y * z2 / 12,6

;	stall

	pfadd	mm4, mm5	; 34	mm4 = z3 = 1 - y * z2 / 12,6
	test	ecx, 1          ;

	jz	x7		; 35

	pfmul	mm4, mm6        ; 36	mm4 = -y * z3 / 2

;	stall

	pfadd	mm4, mm5        ; 39	mm4 = z4 = 1 - y * z3 / 2
x8:

	movd	mm0, ebx        ; 40

	pxor	mm0, mm4	; 41
	ret
x7:
	punpckhdq mm4, mm4	; 37

	pfmul	mm4, mm2	; 38
	jmp	x8		;

_AMD3D_sin      ENDP



_AMD3D_cos      PROC NEAR

;       mmx_single AMD3D_cos(mmx_single arg);
;	arg in LH of mm0, result in LH of mm0, HH of mm0 undefined
; 	returns 1.0 if abs(arg) >= 8192 * pi
;       The average execution time is 41 + 6 = 47 cycles

	movd	eax, mm0	; 1
        pand    mm0, qword ptr MASKP

	and	eax, 7fffffffh	; 2
	cmp	eax, XMAX

	movd	mm2, FOUROPI    ; 3
	jl	x2

	movd	mm0, dword ptr ONE	; 4
	ret
x2:
	movq	mm1, mm0	; 4
	pfmul	mm0, mm2	; 	; y = x * ( 4 / pi)

        movq    mm3, qword ptr HEADTAIL ; 5

	pf2id	mm0, mm0	; 6	; y = int(y)

        movq    mm7, qword ptr MO5642   ; 7

	movd	ecx, mm0        ; 8

        movq    mm6, qword ptr MO3020   ; 9
	pi2fd	mm0, mm0		; y = float(y)

	mov	ebx, ecx        ; 10

	punpckldq mm0,	mm0	; 11
	and	ebx, 4          ;

	pfmul	mm0, mm3	; 12	; y = headtail * y
	shl	ebx, 29

        movq    mm3, qword ptr PIO4  ; 13

	pfadd	mm1, mm0	; 14	; x = x - head(y)
        movq    mm5, qword ptr ONE      ;

	punpckhdq mm0,	mm0	; 15
	test	ecx, 1		;

	pfadd	mm1, mm0	; 16	; x = x - tail(y)

;	stall

	punpckldq mm1, mm1      ; 18
	jz	x5              ;

	pfsubr	mm1, mm3	; 19	; x = pi/4 - x
x5:
	movq	mm2, mm1	; 20	; x
	mov	edx, ecx        ;

	pfmul	mm1, mm1	; 21 	; y = x * x
	shr	edx, 1

	xor	ecx, edx	; 22
	shl	edx, 31

	xor	ebx, edx	; 23
        movq    mm4, qword ptr MO1206   ;

	pfmul	mm7, mm1	; 25   	mm7 = -y / 56,42

	pfmul	mm6, mm1	; 26   	mm6 = -y / 30,20

	pfadd	mm7, mm5        ; 27	mm7 = z1 = 1 - y / 56,42
	pfmul	mm4, mm1  	;     	mm4 = -y / 12,6

;	stall

	pfmul	mm7, mm6	; 29   	mm7 = -y * z1 / 30,20
        movq    mm6, qword ptr MO2

;	stall

	pfadd	mm7, mm5	; 31	mm7 = z2 = 1 - y * z1 / 30,20

;	stall

	pfmul	mm4, mm7	; 33	mm4 = -y * z2 / 12,6
	test	ecx, 1          ;

;	stall

	pfadd	mm4, mm5	; 35	mm4 = z3 = 1 - y * z2 / 12,6
	pfmul	mm6, mm1        ;	mm6 = -y / 2

	jnz	x7              ; 36

	pfmul	mm4, mm6        ; 37	mm4 = -y * z3 / 2

;	stall

	pfadd	mm4, mm5        ; 39	mm4 = z4 = 1 - y * z3 / 2

x8:
	movd	mm0, ebx	; 40

	pxor	mm0, mm4        ; 41
	ret
x7:
	punpckhdq mm4, mm4	; 37
	pfmul	mm4, mm2	; 38
	jmp	x8
_AMD3D_cos      ENDP



_AMD3D_sincos   PROC NEAR
; mmx_double AMD3D_sincos(mmx_single arg);
; arg in LH of mm0, result: cosine in LH of mm0, sine in HH of mm0
; returns cos = 1.0, sin = 0.0 if abs(arg) >= 8192 * pi

; The average execution time = 43 + 6 = 49 clocks

	movd	eax, mm0	; 1
        pand    mm0, qword ptr MASKP

	mov	ebx, eax	; 2	cos sign - in sign bit
	mov	edx, eax	; 	both sign - flag

	and	ebx, 80000000h	; 3
	shr	edx, 31

	xor	eax, ebx	; 4
	cmp	eax, XMAX	;

	movd	mm2, FOUROPI	; 5
	jl	x2              ;

        movd    mm0, dword ptr ONE      ; 6
	ret
x2:
	movq	mm1, mm0
	pfmul	mm0, mm2	; 6	; y = x * ( 4 / pi)

        movq    mm3, qword ptr HEADTAIL ; 7

	pf2id	mm0, mm0	; 8	; y = int(y)

        movq    mm7, qword ptr MO5642   ; 9

	movd	ecx, mm0        ; 10
	pi2fd	mm0, mm0		; y = float(y)

	mov	esi, ecx	; 11
        movq    mm6, qword ptr MO3020   ;

	punpckldq mm0,	mm0	; 12
        movq    mm5, qword ptr ONE      ;

	pfmul	mm0, mm3	; 13	; y = headtail * y
        movq    mm3, qword ptr PIO4
;	stall
	pfadd	mm1, mm0	; 15	; x = x - head(y)
	shr	esi, 2

	punpckhdq mm0,	mm0	; 16
	xor	edx, esi

	pfadd	mm1, mm0	; 17	; x = x - tail(y)
	test	ecx, 1		;

        punpckldq       mm1, mm1 ; 19    ; dup mm1
	jz	x5              ;

	pfsubr	mm1, mm3	; 20	; x = pi/4 - x
x5:
	movq	mm2, mm5	; 20
	shl	edx, 31

	punpckldq mm2, mm1	; 21	; mm2 = 1; x
	pfmul	mm1, mm1	 	; y = x * x

	mov	esi, ecx	; 22
        movq    mm4, qword ptr MO1206   ;

	shr	esi, 1		; 23
	pfmul	mm7, mm1	;    	mm7 = -y / 56,42

	xor	ecx, esi	; 24
	pfmul	mm6, mm1	;    	mm6 = -y / 30,20

	shl	esi, 31         ; 25
	pfadd	mm7, mm5        ; 	mm7 = z1 = 1 - y / 56,42

	xor	ebx, esi        ; 26
	pfmul	mm4, mm1  	;     	mm4 = -y / 12,6

	pfmul	mm7, mm6	; 27   	mm7 = -y * z1 / 30,20
        movq    mm6, qword ptr MO2

	pfadd	mm7, mm5	; 28	mm7 = z2 = 1 - y * z1 / 30,20
	pfmul	mm6, mm1        ; 	mm6 = -y / 2; 0
;	stall

	pfmul	mm4, mm7	; 30	mm4 = -y * z2 / 12,6

	movd	mm0, edx	; 31

	pfadd	mm4, mm5	; 32	mm4 = z3 = 1 - y * z2 / 12,6
	punpckldq mm6, mm5	; 	mm6 = -y/2; 1

	psrlq	mm5, 32		; 33	mm5 = 1; 0

	pfmul	mm4, mm6        ; 34	mm4 = -y * z3 / 2; z3

        punpckldq mm0, mm0      ; 35
	movd	mm1, ebx

	pfadd	mm4, mm5        ; 36	mm4 = z4 = 1 - y * z3 / 2; z3
	test	ecx, 1          ;
;	stall

	pfmul	mm4, mm2	; 38	mm4 = cos; sin
	jz	x7
;	stall

	punpckldq mm5, mm4	; 40

	punpckhdq mm4, mm5      ; 41
x7:
	pxor	mm4, mm1	; 42	mm4 = cos, sin

	pxor	mm0, mm4	; 43
	ret

_AMD3D_sincos   ENDP
_TEXT	ENDS
END

