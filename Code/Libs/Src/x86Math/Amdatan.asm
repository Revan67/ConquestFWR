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
; * AMDATAN.ASM
; *
; * AMD3D arctangent math function.
; *****************************************************************************
        TITLE   AMDatan.asm
	.586
        .K3D
.model  FLAT
PUBLIC  _AMD3D_atan
CONST	SEGMENT
ONE	DD	03f800000r			; 1
SQRT3	DD	03fddb3d7r		; 1.73205
P1	DD	0bd508691r			; -0.0509096
P0	DD	03ef110f6r			; 0.470833
Q0	DD	03fb4ccd3r			; 1.4125
pis	DD	000000000r			; 0
	DD	03f060a92r			; 0.523599	pi/6
	DD	03fc90fdbr			; 1.5708	pi/2
	DD	03f860a92r			; 1.0472	pi/3
MASKP	DD	07fffffffh
	DD	0
CONST	ENDS

_TEXT	SEGMENT

;       mmx_single AMD3d_atan(mmx_single arg);
;	arg in LH of mm0, result in LH of mm0, HH of mm0 undefined
;       The average execution time (50% branch prediction rate)
;	is 48 + 6 = 54 cycles

_AMD3D_atan       PROC NEAR
	movd	edx, mm0	; 1
	xor	ecx, ecx

	movd	mm1, dword ptr MASKP	; 2
	mov	eax, edx

	and	edx, 80000000h	; 3	sign
	and	eax, 7fffffffh

	cmp	eax, 3f800000H	; 4	1.0
	movd	mm3, dword ptr SQRT3	;

	pand	mm0, mm1	; 5
	movd	mm4, dword ptr ONE

	movq	mm1, mm0
	jle	SHORT $L84	; 6

	pfrcp	mm1, mm0	; 7	atan(x) = pi/2 - atan(1/x)
	mov	ECX,2		; flag

        pfrcpit1 mm0, mm1       ; 9

        pfrcpit2 mm0, mm1       ; 11

        movd    eax, mm0        ; 13
	movq	mm1, mm0
$L84:
	movd	mm5, dword ptr P0	; 14
	cmp	eax, 3e8930a3H	; 	2.0 - sqrt(3)

	movd	mm6, dword ptr Q0	; 15
	jle	SHORT $L85

	pfmul	mm0, mm3	; 16	atan(x) = pi/6 + atan(f)
	pfadd	mm1, mm3	;	where f = (x * sqrt(3) - 1) / (sqrt(3) + x)

	inc	ECX		; 17
	nop

	pfsub	mm0, mm4	; 18
	pfrcp	mm2, mm1

        pfrcpit1 mm1, mm2       ; 20

        pfrcpit2 mm1, mm2       ; 22

	pfmul	mm0, mm1	; 24
$L85:
        movd    mm1, dword ptr P1  ; 25

	movq	mm7, mm0	; 26	a rational approximation from
	pfmul	mm0, mm0	;	Cody/Waite is used

	mov	eax, ecx	; 27

	pfmul	mm1, mm0	; 28
	pfadd	mm6, mm0

	and	eax, 2		; 29

	pfsub	mm1, mm5	; 30
	pfrcp	mm3, mm6

	shl	eax, 30		; 31

	movd	mm4, eax	; 32
        pfrcpit1 mm6, mm3

        pfrcpit2 mm6, mm3       ; 34

	pfmul	mm1, mm6	; 36
	shl		ecx,	2

	pfmul	mm0, mm1	; 38
    movd    mm1, dword ptr pis[ecx] ; corrections

	pfmul	mm0, mm7	; 40

	pfadd	mm0, mm7	; 42

	pxor	mm0, mm4	; 44

	pfadd	mm0, mm1	; 46
	movd	mm1, edx

	pxor	mm0, mm1	; 48	sign
	ret	0
_AMD3D_atan       ENDP
_TEXT	ENDS
END
