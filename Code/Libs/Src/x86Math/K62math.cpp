//--------------------------------------------------------------------------//
//                                                                          //
//                              K62math.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
  3DNow implementation of I3DMathEngine.
  Author: AMD

  $Header: /Libs/Dev/Src/x86Math/K62math.cpp 10    10/19/99 11:07a Pbleisch $

*/
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "dacom.h"						// DA component manager
#include "da_heap_utility.h"					// Heap manager
#include "TComponent.h"
#include "3dmath.h"

#include "3DNow_VC.h"


static C8 interface_name[] = "I3DMathEngine";	// Interface name used for registration
static C8 implementation_name[] = "3DNow";      // The implementation we want

// This is an arbitrarily small constant used to circumvent matrix inversions that might overflow due to
// small determinants.
// The proper way would be to check the ratio, but this will do for now.
const SINGLE MIN_DET = 1e-8;

static struct x86MathEngine * global_instance;

#define Q_EPSILON	0.00001


#define CODE_3DNOW	1

#ifndef TEST_MATH
#define TEST_MATH   0
#endif

#if CODE_3DNOW

extern "C" void AMD3D_atan(void);
extern "C" void AMD3D_sin(void);
//offsets for class Matrix
#define MATRIX_D_00	0
#define MATRIX_D_01	4
#define MATRIX_D_02	8
#define MATRIX_D_10 12
#define MATRIX_D_11	16
#define MATRIX_D_12	20
#define MATRIX_D_20	24
#define MATRIX_D_21	28
#define MATRIX_D_22	32
//end offsets for class Matrix
//offsets for class Transform
#define TRSFRM_D_00	0
#define TRSFRM_D_01	4
#define TRSFRM_D_02	8
#define TRSFRM_D_10 12
#define TRSFRM_D_11	16
#define TRSFRM_D_12	20
#define TRSFRM_D_20	24
#define TRSFRM_D_21	28
#define TRSFRM_D_22	32
#define TRSFRM_TRSL_X	36
#define TRSFRM_TRSL_Y	40
#define TRSFRM_TRSL_Z	44
//end  offsets for class Transform
// offsets for class Vector
#define VECTOR_X	0
#define VECTOR_Y	4
#define VECTOR_Z	8
//end offsets for class Vector

const __int64 QConst_1_1   = 0x3f8000003f800000;	//1.0|1.0
const __int64 QInt_1_1     = 0x0000000100000001;	//  1|  1
const __int64 QConst_05_05 = 0x3f0000003f000000;	//0.5|0.5
const __int64 Qfconst_2_2  = 0x4000000040000000;	//2.0|2.0	
const __int64 Q_PI_PI	   = 0x40490fdb40490fdb;	//3.14159|3.14159

const SINGLE	low_limit	=	Q_EPSILON - 1.0;
const SINGLE	hi__limit	=	1.0 - Q_EPSILON;

__int64 rcprc_sin_omega;

Quaternion qs;

#endif	//#define CODE_3DNOW


//
// x86MathEngine class declaration.
//

struct x86MathEngine : public I3DMathEngine, IComponentFactory
{
//	BEGIN_DACOM_MAP_INBOUND(x86MathEngine)
//	DACOM_INTERFACE_ENTRY(I3DMathEngine)
//	END_DACOM_MAP()
	
	x86MathEngine()
      {
      }

	~x86MathEngine()
      {
		global_instance = 0;
      }


	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef)           (void)
	{
		return 1;
	}
	
	DEFMETHOD_(U32,Release)          (void)
	{
		return 1;
	}

   // if there are bad things in the descriptor, return an error code
   // we answer any request with success, for now.
   
   DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance)
   {
	   DA3DMATHDESC * info = (DA3DMATHDESC *) descriptor;
	   
	   *instance = 0;
	   
	   if (info==0 || info->interface_name==0)
		   return GR_INVALID_PARMS;
	   if
		(
			info->size == sizeof(DA3DMATHDESC) &&
			(
				strcmp(::interface_name, info->interface_name)==0 ||
				strcmp(IID_I3DMathEngine, info->interface_name)==0
			) &&
			(
				info->implementation==0 ||
				stricmp (::implementation_name, info->implementation)==0
			)
		)
	   {
		   ((I3DMathEngine *)this)->AddRef();
		   *instance = ((I3DMathEngine *)this);
		   return GR_OK;
	   }
	   
	   return GR_INTERFACE_UNSUPPORTED;
   }

//
// Matrix and Transform operations.
//
	DEFMETHOD(inverse)	(Matrix & dst, const Matrix & m);
	DEFMETHOD(scale)	(Matrix & dst, const Matrix & m, SINGLE s);
	DEFMETHOD(general_inverse)	(Transform & dst, SINGLE & w, const Transform & m);

// Determinant of matrix.
	DEFMETHOD_(SINGLE, det)	(const Matrix & m);

//
// Matrix and Transform concatenation.
//
	DEFMETHOD(mul)	(Matrix & dst, const Matrix & m1, const Matrix & m2);
	DEFMETHOD(mul)	(Transform & dst, const Transform & m1, const Transform & m2);

//
// Matrix and Transform operation on Vectors.
//
	DEFMETHOD(transform)	(Vector & dst, const Matrix & m, const Vector & v);
	DEFMETHOD(transform)	(Vector & dst, const Transform & t, const Vector & v);

// rotate with no translation.
	DEFMETHOD(rotate)			(Vector & dst, const Transform & t, const Vector & v);
	DEFMETHOD(inverse_rotate)	(Vector & dst, const Transform & t, const Vector & v);

//
// Transform Vector with Transpose of Matrix. The transpose might also happen
// to be the inverse in the case of a rotation matrix.
//
	DEFMETHOD(transpose_transform)	(Vector & dst, const Matrix & m, const Vector & v);
	DEFMETHOD(inverse_transform)	(Vector & dst, const Transform & t, const Vector & v);

//
// Same operations on lists of Vectors.
//
	DEFMETHOD(transform_list)	(Vector * dst, const Matrix & m, const Vector * src, int n);
	DEFMETHOD(transform_list)	(Vector * dst, const Transform & t, const Vector * src, int n);

	DEFMETHOD(transpose_transform_list)	(Vector * dst, const Matrix & m, const Vector * src, int n);
	DEFMETHOD(inverse_transform_list)	(Vector * dst, const Transform & t, const Vector * src, int n);

//
// Quaternion conversions.
//
	DEFMETHOD(matrix_to_quaternion)	(Quaternion & dst, const Matrix & m);
	DEFMETHOD(quaternion_to_matrix)	(Matrix & dst, const Quaternion & q);

//
// Quaternion operations.
//
	DEFMETHOD(transform)	(Vector & dst, const Quaternion & q, const Vector & v);
	DEFMETHOD(mul)			(Quaternion & dst, const Quaternion & q1, const Quaternion & q2);

//
// Quaternion interpolation.
//
	DEFMETHOD(quat_slerp)	(Quaternion & dst, const Quaternion & q1, const Quaternion & q2, SINGLE t);

// 
// Fast 1/sqrt(x) and sqrt(x)
//
	DEFMETHOD(InvSqrt)	(SINGLE & dst, const SINGLE x);

	DEFMETHOD(Sqrt)	(SINGLE & dst, const SINGLE x);
};

#if TEST_MATH
I3DMathEngine * __stdcall __MATH_ENGINE (void)
{
	// This function simulates the stuff that happens when the math library
	// is built as a component and loaded into a DACOM aggregate.

	static bool initialized = false;
	if (!initialized)
	{
		global_instance = new x86MathEngine;
	}

	DA3DMATHDESC math_info;
	I3DMathEngine * math_engine;

	global_instance->CreateInstance(&math_info, (void **) &math_engine);

	return math_engine;
}
#else

void main(void)
{
}

//
// DLLMain() called on startup/shutdown
//


BOOL COMAPI DllMain(HINSTANCE hinstDLL,  //)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
	


	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			global_instance = new x86MathEngine;

			if (global_instance != NULL)
			{
				ICOManager *DACOM;						// Handle to component manager

				DACOM = DACOM_Acquire();

				if (DACOM != NULL)
				{
					DACOM->RegisterComponent(global_instance, interface_name, DACOM_LOW_PRIORITY);
					DACOM->RegisterComponent(global_instance, IID_I3DMathEngine, DACOM_LOW_PRIORITY);
				}
			}
			break;
		//
		// DLL_PROCESS_DETACH: Release DACOM manager instance
		//
		case DLL_PROCESS_DETACH:
//			if (DACOM != NULL)
//			{
//				DACOM->Release();
//			}
			break;
	}

	return TRUE;
}
#endif (!TEST_MATH)

#if CODE_3DNOW			
#pragma warning( disable : 4799)
#endif

//
// x86 implementation of I3DMathEngine interface.
//

GENRESULT COMAPI x86MathEngine::inverse(Matrix & dst, const Matrix & m)
{
#if CODE_3DNOW			
int tmp[4];


/*	SINGLE dt = 1.0 / 
	(m.d[0][0] * m.d[1][1] * m.d[2][2] +
	 m.d[0][1] * m.d[1][2] * m.d[2][0] +

	 m.d[0][2] * m.d[1][0] * m.d[2][1] -
	 m.d[0][2] * m.d[1][1] * m.d[2][0] -

	 m.d[0][0] * m.d[1][2] * m.d[2][1] -
	 m.d[0][1] * m.d[1][0] * m.d[2][2]
	 );

	//dst.d[0][0] =   (m.d[1][1] * m.d[2][2] - m.d[1][2] * m.d[2][1]) * dt;
	//dst.d[0][1] = - (m.d[0][1] * m.d[2][2] - m.d[0][2] * m.d[2][1]) * dt;
	
	//dst.d[0][2] =   (m.d[0][1] * m.d[1][2] - m.d[1][1] * m.d[0][2]) * dt;
	//dst.d[1][0] = - (m.d[1][0] * m.d[2][2] - m.d[1][2] * m.d[2][0]) * dt;
    
	//dst.d[1][1] =   (m.d[0][0] * m.d[2][2] - m.d[0][2] * m.d[2][0]) * dt;
    //dst.d[1][2] = - (m.d[0][0] * m.d[1][2] - m.d[0][2] * m.d[1][0]) * dt;
    
	//dst.d[2][0] =   (m.d[1][0] * m.d[2][1] - m.d[1][1] * m.d[2][0]) * dt;
    //dst.d[2][1] = - (m.d[0][0] * m.d[2][1] - m.d[0][1] * m.d[2][0]) * dt;
    
	//dst.d[2][2] =   (m.d[0][0] * m.d[1][1] - m.d[0][1] * m.d[1][0]) * dt;
	
*/

	__asm{
	mov		eax, m

	movd		mm4,	[eax + MATRIX_D_22]	//undef|m.d[2][2]			in mm4
	movq		mm0,	[eax + MATRIX_D_00]	//m.d[0][1]|m.d[0][0]		in mm0	
	;
	movq		mm3,	[eax + MATRIX_D_20]	//m.d[2][1]|m.d[2][0]		in mm3
	movq		mm2,	[eax + MATRIX_D_11]	//m.d[1][2]|m.d[1][1]		in mm2	
	;
	movq		mm7,	mm4				//undef|m.d[2][2]			in mm7
	movq		mm1,	[eax + MATRIX_D_02]	//m.d[1][0]|m.d[0][2]		in mm1
	;
	punpckldq	mm4,	mm3				//m.d[2][0]|m.d[2][2]		in mm4
	punpckldq	mm7,	mm7				//m.d[2][2]|m.d[2][2]		in mm7
	;
	movq		mm6,	mm2				//m.d[1][2]|m.d[1][1]		in mm6
	pfmul(mm4_mm0)						//m.d[0][1]*m.d[2][0]|m.d[0][0]*m.d[2][2]		in mm4
	;
	movq		mm5,	mm3				//m.d[2][1]|m.d[2][0]		in mm5
	punpckhdq	mm6,	mm1				//m.d[1][0]|m.d[1][2]		in mm6
	;
	pfmul(mm4_mm2)						//m.d[0][1]*m.d[1][2]*m.d[2][0]|m.d[0][0]*m.d[1][1]*m.d[2][2]		in mm4
	punpckhdq	mm5,	mm7				//m.d[2][2]|m.d[2][1]		in mm5
	;	
	pfmul(mm5_mm0)						//m.d[0][1]*m.d[2][2]|m.d[0][0]*m.d[2][1]		in mm5
	movq		mm7,	mm3				//m.d[2][1]|m.d[2][0]		in mm7
	;
	;
	pfmul(mm5_mm6)						//m.d[0][1]*m.d[1][0]*m.d[2][2]|m.d[0][0]*m.d[1][2]*m.d[2][1]		in mm5
	pxor		mm6,	mm6				//0|0	in mm7
	;
	pfsub(mm6_mm3)						//-m.d[2][1]|-m.d[2][0]		in mm6
	;
	pfsub(mm4_mm5)						//
	movq		mm5,	mm1				//m.d[1][0]|m.d[0][2]		in mm6
	;
	punpckldq	mm6,	mm6				//-m.d[2][0]|-m.d[2][0]		in mm6
	punpckldq	mm1,	mm1				//m.d[0][2]|m.d[0][2]		in mm1
	;
	punpckhdq	mm7,	mm6				//-m.d[2][0]|m.d[2][1]		in mm7
	punpckhdq	mm5,	mm5				//m.d[1][0]|m.d[1][0]		in mm6
	;
	pfmul(mm7_mm1)						//m.d[0][2]*(-m.d[2][0])|m.d[0][2]*m.d[2][1]		in mm7
	punpckldq	mm5,	mm2				//m.d[1][1]|m.d[1][0]		in mm6
	;
	movq		mm1,	[eax + MATRIX_D_02]	//m.d[1][0]|m.d[0][2]		in mm1
	punpckldq	mm5,	mm2				//m.d[1][1]	| undef			in mm5
	;
	movd		mm6,	[eax + MATRIX_D_22]	//undef|m.d[2][2]			in mm6
	pfmul(mm7_mm5)						//m.d[0][2]*m.d[1][1]*(-m.d[2][0])|m.d[0][2]*m.d[1][0]*m.d[2][1]		in mm7
	;
	punpckhdq	mm5,	mm0				//m.d[0][1] | m.d[1][1]		in mm5
	;
	punpckldq	mm6,	mm6				//m.d[2][2]|m.d[2][2]		in mm6
	pfacc(mm4_mm7)
	;
	pfmul(mm6_mm5)						//m.d[0][1]*m.d[2][2] | m.d[1][1]*m.d[2][2]		in mm6
	movq		mm7,	mm2				//m.d[1][2]|m.d[1][1]		in mm7
	;
	pfacc(mm4_mm4)						//undef|det(m)				in mm4	
	punpckhdq	mm7,	mm7				//m.d[1][2]|m.d[1][2]		in mm7
	;
	lea			ecx,	tmp
	punpckldq	mm7,	mm1				//m.d[0][2]|m.d[1][2]		in mm7
	;
	pfrcp(mm0_mm4)
	;
	mov			edx,	dst
	;
	pfrcpit1(mm4_mm0)
	;
	add			ecx,	7
	;
	pfrcpit2(mm4_mm0)						//1/det(m)|1/det(m)			in mm4
	pxor		mm0,	mm0					//0|0						in mm7
	;
	and			ecx,	0xfffffff8
	;
	pfsub(mm0_mm4)							//-1/det(m)|-1/det(m)		in mm7
	;
	punpckldq	mm4,	mm0					//-dt|dt					in mm4
	;	
	movq		[ecx],	mm4
	movq		mm4,	mm3				//m.d[2][1]|m.d[2][0]		in mm4
	;
	movq		mm0,	[eax + MATRIX_D_00]	//m.d[0][1]|m.d[0][0]		in mm0	
	punpckldq	mm3,	mm3				//m.d[2][0]|m.d[2][0]		in mm3
	;
	punpckhdq	mm4,	mm4				//m.d[2][1]|m.d[2][1]		in mm4
	pfmul(mm3_mm5)						//m.d[0][1]*m.d[2][0] | m.d[1][1]*m.d[2][0]		in mm5
	;
	pfmul(mm7_mm4)						//m.d[0][2]*m.d[2][1]|m.d[1][2]*m.d[2][1]		in mm7			
	;
	pfsub(mm6_mm7)						//m.d[0][1]*m.d[2][2] - m.d[0][2]*m.d[2][1] | m.d[1][1]*m.d[2][2] -	m.d[1][2]*m.d[2][1]	in mm6
	movq		mm7,	mm1				//m.d[1][0]|m.d[0][2]		in mm7
	;
	punpckhdq	mm7,	mm7				//m.d[1][0]|m.d[1][0]		in mm7
	;
	pfmul(mm6_ecx)						//dst.d[0][1]|dst.d[0][0]	in mm6
	punpckldq	mm7,	mm0				//m.d[0][0]|m.d[1][0]		in mm7
	;
	pfmul(mm4_mm7)						//m.d[0][0]*m.d[2][1]|m.d[1][0]*m.d[2][1]		in mm4
	punpckhdq	mm7,	mm1				//m.d[1][0]|m.d[0][0]									in mm7
	;
	movq		[edx+MATRIX_D_00],	mm6
	pfmul(mm5_mm7)						//m.d[0][1]*m.d[1][0] |m.d[0][0]*m.d[1][1]				in mm5
	;
	movq		mm6,	mm0				//m.d[0][1]|m.d[0][0]		in mm6
	pfsub(mm4_mm3)						//m.d[0][0]*m.d[2][1]-m.d[0][1]*m.d[2][0]|m.d[1][0]*m.d[2][1]-m.d[1][1]*m.d[2][0]		in mm4
	;
	pfmul(mm5_ecx)						//m.d[0][1]*m.d[1][0]*(-dt) |m.d[0][0]*m.d[1][1]*dt		in mm5
	;
	punpckhdq	mm6,	mm1				//m.d[1][0]|m.d[0][1]		in mm6
	pfmul(mm4_ecx)						//dst.d[2][1]|dst.d[2][0]	in mm4
	;
	movq		mm3,	mm1				//m.d[1][0]|m.d[0][2]		in mm3
	;
	movq		[edx+MATRIX_D_20],	mm4
	movq		mm4,	mm2				//m.d[1][2]|m.d[1][1]		in mm4
	;
	pfacc(mm5_mm5)						//undef|(m.d[0][0]*m.d[1][1] - m.d[0][1]*m.d[1][0])*dt	in mm5
	punpckhdq	mm4,	mm4				//m.d[1][2]|m.d[1][2]		in mm4
	;
	movd		[edx+MATRIX_D_22], mm5		//undef|dst.d[2][2]										in mm5
	punpckldq	mm4,	[eax + MATRIX_D_22]	//m.d[2][2]|m.d[1][2]		in mm4
	;
	punpckldq	mm3,	[eax + MATRIX_D_20]	//m.d[2][0]|m.d[0][2]		in mm3
	pfmul(mm6_mm4)						//m.d[1][0]*m.d[2][2]|m.d[0][1]*m.d[1][2]		in mm6
	;
	punpckldq	mm7,	[eax + MATRIX_D_20]	//m.d[2][0]|undef			in mm7
	punpckldq	mm0,	mm0				//m.d[0][0]|m.d[0][0]		in mm0
	;
	pfmul(mm3_mm2)						//m.d[1][2]*m.d[2][0]|m.d[1][1]*m.d[0][2]		in mm3
	punpckhdq	mm4,	mm2				//m.d[1][2]|m.d[2][2]		in mm4
	;
	pfmul(mm0_mm4)						//m.d[0][0]*m.d[1][2]|m.d[0][0]*m.d[2][2]		in mm0
	punpckhdq	mm7,	mm1				//m.d[1][0]|m.d[2][0]		in mm7
	;
	pfsub(mm6_mm3)
	punpckldq	mm1,	mm1				//m.d[0][2]|m.d[0][2]		in mm1
	;
	pfmul(mm1_mm7)						//m.d[0][2]*m.d[1][0]|m.d[0][2]*m.d[2][0]		in mm1
	;
	pfmul(mm6_ecx)						//dst.d[1][0]|dst.d[0][2]	in mm6
	;
	pfsub(mm0_mm1)						//
	;
	movq		[edx+MATRIX_D_02],	mm6		//dst.d[1][0]|dst.d[0][2]	in mm6
	pfmul(mm0_ecx)						//dst.d[1][2]|dst.d[1][1]	in mm0
	movq		[edx+MATRIX_D_11],	mm0
	femms
	}
#else	//else #if CODE_3DNOW
	SINGLE dt; 
	dt = 1.0 / det(m);

    dst.d[0][0] =   (m.d[1][1] * m.d[2][2] - m.d[1][2] * m.d[2][1]) * dt;
    dst.d[1][0] = - (m.d[1][0] * m.d[2][2] - m.d[1][2] * m.d[2][0]) * dt;
    dst.d[2][0] =   (m.d[1][0] * m.d[2][1] - m.d[1][1] * m.d[2][0]) * dt;
    dst.d[0][1] = - (m.d[0][1] * m.d[2][2] - m.d[0][2] * m.d[2][1]) * dt;
    dst.d[1][1] =   (m.d[0][0] * m.d[2][2] - m.d[0][2] * m.d[2][0]) * dt;
    dst.d[2][1] = - (m.d[0][0] * m.d[2][1] - m.d[0][1] * m.d[2][0]) * dt;
    dst.d[0][2] =   (m.d[0][1] * m.d[1][2] - m.d[0][2] * m.d[1][1]) * dt;
    dst.d[1][2] = - (m.d[0][0] * m.d[1][2] - m.d[0][2] * m.d[1][0]) * dt;
    dst.d[2][2] =   (m.d[0][0] * m.d[1][1] - m.d[0][1] * m.d[1][0]) * dt;
#endif
	return GR_OK;
}

GENRESULT COMAPI x86MathEngine::scale(Matrix & dst, const Matrix & m, SINGLE s)
{

#if CODE_3DNOW

	__asm{
		movd		mm0,	s
		mov			eax,	m
		;	
		movq		mm1,	[eax+MATRIX_D_00]	//m.d[0][1]|m.d[0][0]	in mm1
		mov			edx,	dst
		;
		movq		mm2,	[eax+MATRIX_D_02]	//m.d[0][1]|m.d[0][0]	in mm1
		punpckldq	mm0,	mm0					//s|s		in mm0
		;
		movq		mm3,	[eax+MATRIX_D_11]	//m.d[0][1]|m.d[0][0]	in mm1
		pfmul(mm1_mm0)							//m.d[0][1]*s|m.d[0][0]*s	in mm1
		;
		movq		mm4,	[eax+MATRIX_D_20]	//m.d[0][1]|m.d[0][0]	in mm1
		pfmul(mm2_mm0)							//m.d[0][1]*s|m.d[0][0]*s	in mm1
		;
		movd		mm5,	[eax+MATRIX_D_22]	//m.d[0][1]|m.d[0][0]	in mm1
		pfmul(mm3_mm0)							//m.d[0][1]*s|m.d[0][0]*s	in mm1
		;
		movq		[edx+MATRIX_D_00],	mm1		//dst.d[0][1]|dst.d[0][0]	in mm1
		pfmul(mm4_mm0)							//m.d[0][1]*s|m.d[0][0]*s	in mm1
		;
		movq		[edx+MATRIX_D_02],	mm2		//dst.d[0][1]|dst.d[0][0]	in mm1
		pfmul(mm5_mm0)							//m.d[0][1]*s|m.d[0][0]*s	in mm1
		;
		movq		[edx+MATRIX_D_11],	mm3		//dst.d[0][1]|dst.d[0][0]	in mm1
		movq		[edx+MATRIX_D_20],	mm4		//dst.d[0][1]|dst.d[0][0]	in mm1
		movd		[edx+MATRIX_D_22],	mm5		//dst.d[0][1]|dst.d[0][0]	in mm1
		femms										
	}

#else	//else #if CODE_3DNOW
	
	dst.d[0][0] = m.d[0][0] * s;
	dst.d[0][1] = m.d[0][1] * s;
	dst.d[0][2] = m.d[0][2] * s;
	dst.d[1][0] = m.d[1][0] * s;
	dst.d[1][1] = m.d[1][1] * s;
	dst.d[1][2] = m.d[1][2] * s;
	dst.d[2][0] = m.d[2][0] * s;
	dst.d[2][1] = m.d[2][1] * s;
	dst.d[2][2] = m.d[2][2] * s;
#endif

	return GR_OK;
}

//

SINGLE COMAPI x86MathEngine::det(const Matrix & m)
{
	return (m.d[0][0] * m.d[1][1] * m.d[2][2] +
			m.d[0][1] * m.d[1][2] * m.d[2][0] +
			m.d[0][2] * m.d[1][0] * m.d[2][1] -
			m.d[0][0] * m.d[1][2] * m.d[2][1] -
			m.d[0][1] * m.d[1][0] * m.d[2][2] -
			m.d[0][2] * m.d[1][1] * m.d[2][0]);
}

// assumes last row is 0 0 0 1
GENRESULT COMAPI x86MathEngine::general_inverse(Transform & dst, SINGLE & w, const Transform & t)
{
	GENRESULT result;

	SINGLE determinant = t.d[0][0] * (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) -
						 t.d[0][1] * (t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) +
						 t.d[0][2] * (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]);

	if (fabs(determinant) > MIN_DET)
	{
		const SINGLE dt = 1.0f / determinant;

		dst.d[0][0] =  (t.d[1][1] * t.d[2][2] - t.d[2][1] * t.d[1][2]) * dt;
		dst.d[0][1] = -(t.d[0][1] * t.d[2][2] - t.d[2][1] * t.d[0][2]) * dt;
		dst.d[0][2] =  (t.d[0][1] * t.d[1][2] - t.d[1][1] * t.d[0][2]) * dt;

		dst.d[1][0] = -(t.d[1][0] * t.d[2][2] - t.d[2][0] * t.d[1][2]) * dt;
		dst.d[1][1] =  (t.d[0][0] * t.d[2][2] - t.d[2][0] * t.d[0][2]) * dt;
		dst.d[1][2] = -(t.d[0][0] * t.d[1][2] - t.d[1][0] * t.d[0][2]) * dt;

		dst.d[2][0] =  (t.d[1][0] * t.d[2][1] - t.d[2][0] * t.d[1][1]) * dt;
		dst.d[2][1] = -(t.d[0][0] * t.d[2][1] - t.d[2][0] * t.d[0][1]) * dt;
		dst.d[2][2] =  (t.d[0][0] * t.d[1][1] - t.d[1][0] * t.d[0][1]) * dt;

		dst.translation.x = -( t.d[0][1] * t.d[1][2] * t.translation.z +
							   t.d[1][1] * t.d[2][2] * t.translation.x +
							   t.d[2][1] * t.d[0][2] * t.translation.y -
							   t.d[0][1] * t.d[2][2] * t.translation.y -
							   t.d[1][1] * t.d[0][2] * t.translation.z -
							   t.d[2][1] * t.d[1][2] * t.translation.x ) * dt;

		dst.translation.y =  ( t.d[0][0] * t.d[1][2] * t.translation.z +
							   t.d[1][0] * t.d[2][2] * t.translation.x +
							   t.d[2][0] * t.d[0][2] * t.translation.y -
							   t.d[0][0] * t.d[2][2] * t.translation.y -
							   t.d[1][0] * t.d[0][2] * t.translation.z -
							   t.d[2][0] * t.d[1][2] * t.translation.x ) * dt;

		dst.translation.z = -( t.d[0][0] * t.d[1][1] * t.translation.z +
							   t.d[1][0] * t.d[2][1] * t.translation.x +
							   t.d[2][0] * t.d[0][1] * t.translation.y -
							   t.d[0][0] * t.d[2][1] * t.translation.y -
							   t.d[1][0] * t.d[0][1] * t.translation.z -
							   t.d[2][0] * t.d[1][1] * t.translation.x ) * dt;

		w = ( t.d[0][0] * t.d[1][1] * t.d[2][2] +
			  t.d[1][0] * t.d[2][1] * t.d[0][2] +
			  t.d[2][0] * t.d[0][1] * t.d[1][2] -
			  t.d[2][0] * t.d[1][1] * t.d[0][2] -
			  t.d[0][0] * t.d[2][1] * t.d[1][2] -
			  t.d[1][0] * t.d[0][1] * t.d[2][2] ) * dt;

		result = GR_OK;
	}
	else
	{
		dst.zero(); 
		result = GR_INVALID_PARMS;
	}

	return result;
}

//
// Matrix and Transform concatenation.
//
GENRESULT COMAPI x86MathEngine::mul(Matrix & dst, const Matrix & m1, const Matrix & m2)
{
#if CODE_3DNOW
/*
	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] + m1.d[0][1] * m2.d[1][0] + m1.d[0][2] * m2.d[2][0];
	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] + m1.d[0][1] * m2.d[1][1] + m1.d[0][2] * m2.d[2][1];
	
    dst.d[1][0] = m1.d[1][0] * m2.d[0][0] + m1.d[1][1] * m2.d[1][0] + m1.d[1][2] * m2.d[2][0];
	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] + m1.d[1][1] * m2.d[1][1] + m1.d[1][2] * m2.d[2][1];
	

	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] + m1.d[2][1] * m2.d[1][0] + m1.d[2][2] * m2.d[2][0];
	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] + m1.d[2][1] * m2.d[1][1] + m1.d[2][2] * m2.d[2][1];
	
	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] + m1.d[0][1] * m2.d[1][2] + m1.d[0][2] * m2.d[2][2];
	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] + m1.d[1][1] * m2.d[1][2] + m1.d[1][2] * m2.d[2][2];

    dst.d[2][2] = m1.d[2][0] * m2.d[0][2] + m1.d[2][1] * m2.d[1][2] + m1.d[2][2] * m2.d[2][2];
*/
	__asm{
			mov			eax,	m1
			mov			ecx,	m2
			movd		mm3,	[eax+MATRIX_D_00]		//undef |m1.d[0][0]
			mov			edx,	dst
			;
			movq		mm0,	[ecx+MATRIX_D_00]		//m2.d[0][1]|m2.d[0][0]		in	mm0
			;
			movd		mm4,	[eax+MATRIX_D_01]		//undef |m1.d[0][1]
			punpckldq	mm3,	mm3						//m1.d[0][0]|m1.d[0][0]		in mm3
			;
			movd		mm1,	[ecx+MATRIX_D_10]		//undef|m2.d[1][0]			in mm1
			pfmul(mm3_mm0)								//m1.d[0][0]*m2.d[0][1]|m1.d[0][0]*m2.d[0][0]		in mm3
			;
			punpckldq	mm4,	mm4						//m1.d[0][1] | m1.d[0][1]	in mm4
			punpckldq	mm1,	[ecx+MATRIX_D_11]		//m2.d[1][1]|m2.d[1][0]		in mm1
			;
			movd		mm5,	[eax+MATRIX_D_02]		//undef|m1.d[0][2]			in mm5
			pfmul(mm4_mm1)								//m1.d[0][1]*m2.d[1][1] | m1.d[0][1]*m2.d[1][0]	in mm4
			;
			movq		mm2,	[ecx+MATRIX_D_20]		//m2.d[2][1]|m2.d[2][0]		in mm2
			movd		mm6,	[eax+MATRIX_D_10]		//undef |m1.d[1][0]
			;
			pfadd(mm4_mm3)	//m1.d[0][0]*m2.d[0][1]+m1.d[0][1]*m2.d[1][1] | m1.d[0][0]*m2.d[0][0]+m1.d[0][1]*m2.d[1][0]	in mm4				
			punpckldq	mm5,	mm5						//m1.d[0][2]|m1.d[0][2]		in mm5
			;
			pfmul(mm5_mm2)								//m1.d[0][2]*m2.d[2][1]|m1.d[0][2]*m2.d[2][0]		in mm5	
			punpckldq	mm6,	mm6						//m1.d[1][0]|m1.d[1][0]		in mm6
			;
			movd		mm7,	[eax+MATRIX_D_11]		//undef |m1.d[1][1]
			pfmul(mm6_mm0)								//m1.d[1][0]*m2.d[0][1]|m1.d[1][0]*m2.d[0][0]		in mm6
			;
			pfadd(mm5_mm4)								//dst.d[0][1] |	dst.d[0][0] in mm5
			movd		mm3,	[eax+MATRIX_D_12]		//undef|m1.d[1][2]			in mm5
			;
			punpckldq	mm7,	mm7						//m1.d[1][1] | m1.d[1][1]	in mm7
			movd		mm4,	[eax+MATRIX_D_20]		//undef |m1.d[2][0]
			;
			pfmul(mm7_mm1)								//m1.d[1][1]*m2.d[1][1] | m1.d[1][1]*m2.d[1][0]	in mm7
			punpckldq	mm3,	mm3						//m1.d[1][2]|m1.d[1][2]		in mm5
			;
			movq		[edx+MATRIX_D_00],	mm5			//dst.d[0][1] |	dst.d[0][0] in mm5	
			punpckldq	mm4,	mm4						//m1.d[2][0]|m1.d[2][0]		in mm4
			;
			pfadd(mm7_mm6)	//m1.d[1][0]*m2.d[0][1]+m1.d[1][1]*m2.d[1][1] | m1.d[1][0]*m2.d[0][0]+m1.d[1][1]*m2.d[1][0]	in mm7			
			pfmul(mm3_mm2)								//m1.d[1][2]*m2.d[2][1]|m1.d[1][2]*m2.d[2][0]		in mm5	
			;
			movd		mm5,	[eax+MATRIX_D_21]		//undef |m1.d[2][1]
			pfmul(mm4_mm0)								//m1.d[2][0]*m2.d[0][1]|m1.d[2][0]*m2.d[0][0]		in mm4
			;
			movd		mm6,	[eax+MATRIX_D_22]		//undef|m1.d[2][2]			in mm6
			pfadd(mm3_mm7)								//dst.d[1][1] | dst.d[1][0]	in mm3
			;
			punpckldq	mm5,	mm5						//m1.d[2][1] | m1.d[2][1]	in mm5
			punpckldq	mm7,	[eax+MATRIX_D_00]		//m1.d[0][0]|undef			in mm7
			;
			punpckldq	mm6,	mm6						//m1.d[2][2]|m1.d[2][2]		in mm6
			pfmul(mm5_mm1)								//m1.d[2][1]*m2.d[1][1] | m1.d[2][1]*m2.d[1][0]	in mm5
			;
			movd		[edx+MATRIX_D_10],	mm3			//dst.d[1][1] | dst.d[1][0]	in mm3
			pfmul(mm6_mm2)								//m1.d[2][2]*m2.d[2][1]|m1.d[2][2]*m2.d[2][0]		in mm6	
			;
			punpckhdq	mm3,	mm3						//dst.d[1][1] | dst.d[1][1]	in mm3
			pfadd(mm5_mm4)	//m1.d[2][0]*m2.d[0][1]+m1.d[2][1]*m2.d[1][1]|m1.d[2][0]*m2.d[0][0]+m1.d[2][1]*m2.d[1][0]	in mm5
			;
			movd		[edx+MATRIX_D_11],	mm3			//dst.d[1][1] | dst.d[1][1]	in mm3
			punpckhdq	mm7,	[eax+MATRIX_D_02]		//m1.d[0][0]|m1.d[0][0]		in mm7
			;
			movd		mm3,	[ecx+MATRIX_D_02]		//undef|m2.d[0][2]			in mm3
			pfadd(mm6_mm5)								//dst.d[2][1]|dst.d[2][0]	in mm6
			;
			movd		mm0,	[eax+MATRIX_D_01]		//undef	| m1.d[0][1]		in mm0
			punpckldq	mm1,	[eax+MATRIX_D_02]		//m1.d[0][2]|undef			in mm1
			;
			punpckldq	mm3,	mm3						//m2.d[0][2]|m2.d[0][2]		in mm3
			movd		mm4,	[ecx+MATRIX_D_12]		//undef|m2.d[1][2]			in mm4
			;
			movq		[edx+MATRIX_D_20],	mm6			//dst.d[2][1]|dst.d[2][0]	in mm6	
			punpckldq	mm0,	[eax+MATRIX_D_11]		//m1.d[1][1] | m1.d[0][1]	in mm0
			;
			pfmul(mm7_mm3)								//m1.d[1][0]*m2.d[0][2]|m1.d[0][0]*m2.d[0][2]		in mm7
			punpckldq	mm4,	mm4						//m2.d[1][2] | m2.d[1][2]	in mm4
			;
			punpckhdq	mm1,	[eax+MATRIX_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm1
			pfmul(mm0_mm4)		// 	m1.d[1][1]*m2.d[1][2] | m1.d[0][1]*m2.d[1][2]	in mm0
			;
			movd		mm2,	[ecx+MATRIX_D_22]		//undef|m2.d[2][2]			in mm2
			punpckldq	mm3,	mm4						//m2.d[1][2]|m2.d[0][2]		in mm3
			;
			pfadd(mm0_mm7)		//m1.d[1][0]*m2.d[0][2]+m1.d[1][1]*m2.d[1][2] | m1.d[0][0]*m2.d[0][2]+m1.d[0][0]*m2.d[0][2]		in mm0
			movq		mm5,	[eax+MATRIX_D_20]		//m1.d[2][1]|m1.d[2][0]		in mm5
			;
			movd		mm6,	[eax+MATRIX_D_22]		//undef|m1.d[2][2]				in mm6
			punpckldq	mm2,	mm2						//m2.d[2][2]|m2.d[2][2]		in mm2
			;
			pfmul(mm1_mm2)								//m1.d[1][2]*m2.d[2][2] |m1.d[0][2]*m2.d[2][2]		in mm1
			;
			pfmul(mm5_mm3)						//m1.d[2][1]*m2.d[1][2]|m1.d[2][0]*m2.d[0][2]		in mm5
			;
			pfadd(mm1_mm0)								//dst.d[1][2]|dst.d[0][2]	in mm1
			pfmul(mm6_mm2)								//undef|m1.d[2][2]*m2.d[2][2]	in mm6
			;
			pfacc(mm5_mm5)						//undef|m1.d[2][1]*m2.d[1][2]+m1.d[2][0]*m2.d[0][2]		in mm5
			;
			movd		[edx+MATRIX_D_02],	mm1
			pfadd(mm6_mm5)								//undef|dst.d[2][2]				in mm6
			;
			punpckhdq	mm1,	mm1						//dst.d[1][2]|dst.d[1][2]	in mm1
			;
			movd		[edx+MATRIX_D_12],	mm1			//dst.d[1][2]|dst.d[1][2]	in mm1
			movd		[edx+MATRIX_D_22],	mm6			//undef|dst.d[2][2]				in mm6
			femms
	}
		

#else	//#if CODE_3DNOW
	for (int i = 0; i < 3; i++)
	{
		dst.d[i][0] = m1.d[i][0] * m2.d[0][0] + m1.d[i][1] * m2.d[1][0] + m1.d[i][2] * m2.d[2][0];
		dst.d[i][1] = m1.d[i][0] * m2.d[0][1] + m1.d[i][1] * m2.d[1][1] + m1.d[i][2] * m2.d[2][1];
		dst.d[i][2] = m1.d[i][0] * m2.d[0][2] + m1.d[i][1] * m2.d[1][2] + m1.d[i][2] * m2.d[2][2];
	}
/*
 FOR SOME UNKNOWN REASON, THE VC++ 5.0 OPTIMIZER CHOKES THE FOLLOWING CODE.
*/
/*
	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] + m1.d[0][1] * m2.d[1][0] + m1.d[0][2] * m2.d[2][0];
	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] + m1.d[0][1] * m2.d[1][1] + m1.d[0][2] * m2.d[2][1];
	
    dst.d[1][0] = m1.d[1][0] * m2.d[0][0] + m1.d[1][1] * m2.d[1][0] + m1.d[1][2] * m2.d[2][0];
	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] + m1.d[1][1] * m2.d[1][1] + m1.d[1][2] * m2.d[2][1];
	

	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] + m1.d[2][1] * m2.d[1][0] + m1.d[2][2] * m2.d[2][0];
	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] + m1.d[2][1] * m2.d[1][1] + m1.d[2][2] * m2.d[2][1];
	
	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] + m1.d[0][1] * m2.d[1][2] + m1.d[0][2] * m2.d[2][2];
	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] + m1.d[1][1] * m2.d[1][2] + m1.d[1][2] * m2.d[2][2];

    dst.d[2][2] = m1.d[2][0] * m2.d[0][2] + m1.d[2][1] * m2.d[1][2] + m1.d[2][2] * m2.d[2][2];
*/
#endif	//#if CODE_3DNOW

	return GR_OK;
}


GENRESULT COMAPI x86MathEngine::mul(Transform & dst, const Transform & m1, const Transform & m2)
{
#if CODE_3DNOW
/*
	
	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] + m1.d[0][1] * m2.d[1][0] + m1.d[0][2] * m2.d[2][0];
	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] + m1.d[0][1] * m2.d[1][1] + m1.d[0][2] * m2.d[2][1];

	dst.d[1][0] = m1.d[1][0] * m2.d[0][0] + m1.d[1][1] * m2.d[1][0] + m1.d[1][2] * m2.d[2][0];
	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] + m1.d[1][1] * m2.d[1][1] + m1.d[1][2] * m2.d[2][1];
	
	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] + m1.d[2][1] * m2.d[1][0] + m1.d[2][2] * m2.d[2][0];
	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] + m1.d[2][1] * m2.d[1][1] + m1.d[2][2] * m2.d[2][1];

	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] + m1.d[0][1] * m2.d[1][2] + m1.d[0][2] * m2.d[2][2];
	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] + m1.d[1][1] * m2.d[1][2] + m1.d[1][2] * m2.d[2][2];

	dst.d[2][2] = m1.d[2][0] * m2.d[0][2] + m1.d[2][1] * m2.d[1][2] + m1.d[2][2] * m2.d[2][2];

    dst.translation.x = m1.d[0][0] * m2.translation.x +	m1.d[0][1] * m2.translation.y +	m1.d[0][2] * m2.translation.z +	m1.translation.x;
	dst.translation.y = m1.d[1][0] * m2.translation.x + m1.d[1][1] * m2.translation.y +	m1.d[1][2] * m2.translation.z + m1.translation.y;
	dst.translation.z = m1.d[2][0] * m2.translation.x +	m1.d[2][1] * m2.translation.y +	m1.d[2][2] * m2.translation.z +	m1.translation.z;
*/
	__asm{
			mov			eax,	m1
			mov			ecx,	m2
			
			movd		mm3,	[eax+TRSFRM_D_00]		//undef |m1.d[0][0]
			mov			edx,	dst
			;
			movq		mm0,	[ecx+TRSFRM_D_00]		//m2.d[0][1]|m2.d[0][0]		in	mm0
			;
			movd		mm4,	[eax+TRSFRM_D_01]		//undef |m1.d[0][1]
			punpckldq	mm3,	mm3						//m1.d[0][0]|m1.d[0][0]		in mm3
			;
			movd		mm1,	[ecx+TRSFRM_D_10]		//undef|m2.d[1][0]			in mm1
			pfmul(mm3_mm0)								//m1.d[0][0]*m2.d[0][1]|m1.d[0][0]*m2.d[0][0]		in mm3
			;
			punpckldq	mm4,	mm4						//m1.d[0][1] | m1.d[0][1]	in mm4
			punpckldq	mm1,	[ecx+TRSFRM_D_11]		//m2.d[1][1]|m2.d[1][0]		in mm1
			;
			movd		mm5,	[eax+TRSFRM_D_02]		//undef|m1.d[0][2]			in mm5
			pfmul(mm4_mm1)								//m1.d[0][1]*m2.d[1][1] | m1.d[0][1]*m2.d[1][0]	in mm4
			;
			movq		mm2,	[ecx+TRSFRM_D_20]		//m2.d[2][1]|m2.d[2][0]		in mm2
			movd		mm6,	[eax+TRSFRM_D_10]		//undef |m1.d[1][0]
			;
			pfadd(mm4_mm3)	//m1.d[0][0]*m2.d[0][1]+m1.d[0][1]*m2.d[1][1] | m1.d[0][0]*m2.d[0][0]+m1.d[0][1]*m2.d[1][0]	in mm4				
			punpckldq	mm5,	mm5						//m1.d[0][2]|m1.d[0][2]		in mm5
			;
			pfmul(mm5_mm2)								//m1.d[0][2]*m2.d[2][1]|m1.d[0][2]*m2.d[2][0]		in mm5	
			punpckldq	mm6,	mm6						//m1.d[1][0]|m1.d[1][0]		in mm6
			;
			movd		mm7,	[eax+TRSFRM_D_11]		//undef |m1.d[1][1]
			pfmul(mm6_mm0)								//m1.d[1][0]*m2.d[0][1]|m1.d[1][0]*m2.d[0][0]		in mm6
			;
			pfadd(mm5_mm4)								//dst.d[0][1] |	dst.d[0][0] in mm5
			movd		mm3,	[eax+TRSFRM_D_12]		//undef|m1.d[1][2]			in mm5
			;
			punpckldq	mm7,	mm7						//m1.d[1][1] | m1.d[1][1]	in mm7
			movd		mm4,	[eax+TRSFRM_D_20]		//undef |m1.d[2][0]
			;
			pfmul(mm7_mm1)								//m1.d[1][1]*m2.d[1][1] | m1.d[1][1]*m2.d[1][0]	in mm7
			punpckldq	mm3,	mm3						//m1.d[1][2]|m1.d[1][2]		in mm5
			;
			movq		[edx+TRSFRM_D_00],	mm5			//dst.d[0][1] |	dst.d[0][0] in mm5	
			punpckldq	mm4,	mm4						//m1.d[2][0]|m1.d[2][0]		in mm4
			;
			pfadd(mm7_mm6)	//m1.d[1][0]*m2.d[0][1]+m1.d[1][1]*m2.d[1][1] | m1.d[1][0]*m2.d[0][0]+m1.d[1][1]*m2.d[1][0]	in mm7			
			pfmul(mm3_mm2)								//m1.d[1][2]*m2.d[2][1]|m1.d[1][2]*m2.d[2][0]		in mm5	
			;
			movd		mm5,	[eax+TRSFRM_D_21]		//undef |m1.d[2][1]
			pfmul(mm4_mm0)								//m1.d[2][0]*m2.d[0][1]|m1.d[2][0]*m2.d[0][0]		in mm4
			;
			movd		mm6,	[eax+TRSFRM_D_22]		//undef|m1.d[2][2]			in mm6
			pfadd(mm3_mm7)								//dst.d[1][1] | dst.d[1][0]	in mm3
			;
			punpckldq	mm5,	mm5						//m1.d[2][1] | m1.d[2][1]	in mm5
			punpckldq	mm7,	[eax+TRSFRM_D_00]		//m1.d[0][0]|undef			in mm7
			;
			punpckldq	mm6,	mm6						//m1.d[2][2]|m1.d[2][2]		in mm6
			pfmul(mm5_mm1)								//m1.d[2][1]*m2.d[1][1] | m1.d[2][1]*m2.d[1][0]	in mm5
			;
			movd		[edx+TRSFRM_D_10],	mm3			//dst.d[1][1] | dst.d[1][0]	in mm3
			pfmul(mm6_mm2)								//m1.d[2][2]*m2.d[2][1]|m1.d[2][2]*m2.d[2][0]		in mm6	
			;
			punpckhdq	mm3,	mm3						//dst.d[1][1] | dst.d[1][1]	in mm3
			pfadd(mm5_mm4)	//m1.d[2][0]*m2.d[0][1]+m1.d[2][1]*m2.d[1][1]|m1.d[2][0]*m2.d[0][0]+m1.d[2][1]*m2.d[1][0]	in mm5
			;
			movd		[edx+TRSFRM_D_11],	mm3			//dst.d[1][1] | dst.d[1][1]	in mm3
			punpckhdq	mm7,	[eax+TRSFRM_D_02]		//m1.d[0][0]|m1.d[0][0]		in mm7
			;
			movd		mm3,	[ecx+TRSFRM_D_02]		//undef|m2.d[0][2]			in mm3
			pfadd(mm6_mm5)								//dst.d[2][1]|dst.d[2][0]	in mm6
			;
			movd		mm0,	[eax+TRSFRM_D_01]		//undef	| m1.d[0][1]		in mm0
			punpckldq	mm1,	[eax+TRSFRM_D_02]		//m1.d[0][2]|undef			in mm1
			;
			punpckldq	mm3,	mm3						//m2.d[0][2]|m2.d[0][2]		in mm3
			movd		mm4,	[ecx+TRSFRM_D_12]		//undef|m2.d[1][2]			in mm4
			;
			movq		[edx+TRSFRM_D_20],	mm6			//dst.d[2][1]|dst.d[2][0]	in mm6	
			punpckldq	mm0,	[eax+TRSFRM_D_11]		//m1.d[1][1] | m1.d[0][1]	in mm0
			;
			pfmul(mm7_mm3)								//m1.d[1][0]*m2.d[0][2]|m1.d[0][0]*m2.d[0][2]		in mm7
			punpckldq	mm4,	mm4						//m2.d[1][2] | m2.d[1][2]	in mm4
			;
			punpckhdq	mm1,	[eax+TRSFRM_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm1
			pfmul(mm0_mm4)		// 	m1.d[1][1]*m2.d[1][2] | m1.d[0][1]*m2.d[1][2]	in mm0
			;
			movd		mm2,	[ecx+TRSFRM_D_22]		//undef|m2.d[2][2]			in mm2
			punpckldq	mm3,	mm4						//m2.d[1][2]|m2.d[0][2]		in mm3
			;
			pfadd(mm0_mm7)		//m1.d[1][0]*m2.d[0][2]+m1.d[1][1]*m2.d[1][2] | m1.d[0][0]*m2.d[0][2]+m1.d[0][0]*m2.d[0][2]		in mm0
			movq		mm5,	[eax+TRSFRM_D_20]		//m1.d[2][1]|m1.d[2][0]		in mm5
			;
			movd		mm6,	[eax+TRSFRM_D_22]		//undef|m1.d[2][2]				in mm6
			punpckldq	mm2,	mm2						//m2.d[2][2]|m2.d[2][2]		in mm2
			;
			pfmul(mm1_mm2)								//m1.d[1][2]*m2.d[2][2] |m1.d[0][2]*m2.d[2][2]		in mm1
			;
			pfmul(mm5_mm3)						//m1.d[2][1]*m2.d[1][2]|m1.d[2][0]*m2.d[0][2]		in mm5
			;
			pfadd(mm1_mm0)								//dst.d[1][2]|dst.d[0][2]	in mm1
			pfmul(mm6_mm2)								//undef|m1.d[2][2]*m2.d[2][2]	in mm6
			;
			pfacc(mm5_mm5)						//undef|m1.d[2][1]*m2.d[1][2]+m1.d[2][0]*m2.d[0][2]		in mm5
			;
			movd		[edx+TRSFRM_D_02],	mm1
			pfadd(mm6_mm5)								//undef|dst.d[2][2]				in mm6
			;
			punpckhdq	mm1,	mm1						//dst.d[1][2]|dst.d[1][2]	in mm1
			;
			movd		[edx+TRSFRM_D_12],	mm1			//dst.d[1][2]|dst.d[1][2]	in mm1
			movd		[edx+TRSFRM_D_22],	mm6			//undef|dst.d[2][2]				in mm6
			
			//*** WARNING: Don't do this! Very bad thing.
			//movd		mm4,m2.translation.x			//undef|m2.translation.x				in mm4
		
			movd		mm4,	[ecx+TRSFRM_TRSL_X]		//undef|m2.translation.x				in mm4
			punpckldq	mm4,	[ecx+TRSFRM_TRSL_Y]		//m2.translation.y|m2.translation.x		in mm4	
			
			movq		mm7,	[eax+TRSFRM_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			pfmul(mm7_mm4)	//m1.d[0][1]*m2.translation.y|m1.d[0][0]*m2.translation.x			in mm7

			movd		mm3,	[eax+TRSFRM_D_10]		//undef|m1.d[1][0]		in mm3
			punpckldq	mm3,	[eax+TRSFRM_D_11]		//m1.d[1][1]|m1.d[1][0]					in mm3
			pfmul(mm3_mm4)	//m1.d[1][1]*m2.translation.y|m1.d[1][0]*m2.translation.x			in mm3
			pfacc(mm7_mm3)	//m1.d[1][1]*m2.translation.y+m1.d[1][0]*m2.translation.x | m1.d[0][1]*m2.translation.y+m1.d[0][0]*m2.translation.x			in mm7
			
			punpckldq	mm0,	[eax+TRSFRM_D_02]		//m1.d[0][2]|undef			in mm0
			punpckhdq	mm0,	[eax+TRSFRM_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0

			movd		mm2,	[ecx+TRSFRM_TRSL_Z]		//undef|m2.translation.z	in mm2
			punpckldq	mm2,	mm2						//m2.translation.z|m2.translation.z	in mm2
			pfmul(mm0_mm2)	//m1.d[1][2]*m2.translation.z|m1.d[0][2]*m2.translation.z		in mm0
			pfadd(mm0_mm7)	//  m1.d[1][0]*m2.translation.x+m1.d[1][1]*m2.translation.y+m1.d[1][2]*m2.translation.z		\
							//| m1.d[0][0]*m2.translation.x+m1.d[0][1]*m2.translation.y+m1.d[0][2]*m2.translation.z		in mm0
			
			movd		mm1,	[eax+TRSFRM_TRSL_X]		//undef|m1.translation.x				in mm1			
			punpckldq	mm1,	[eax+TRSFRM_TRSL_Y]		//m1.translation.y|m1.translation.x		in mm1
			pfadd(mm1_mm0)								//dst.translation.y|dst.translation.x	in mm1
			movd		[edx+TRSFRM_TRSL_X],	mm1		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm1,	mm1						//dst.translation.y|dst.translation.y   in mm1
			movd		[edx+TRSFRM_TRSL_Y],	mm1		//dst.translation.y|dst.translation.y   in mm1
			
			movq		mm5,	[eax+TRSFRM_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			pfmul(mm5_mm4)	//m1.d[2][1]*m2.translation.x|m1.d[2][0]*m2.translation.x			in mm5

			punpckldq	mm6,	[eax+TRSFRM_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm6_mm2)								//m1.d[2][2]*m2.translation.z|undef		in mm6
			punpckhdq	mm6,	[eax+TRSFRM_TRSL_Y]		//m1.translation.z|m1.d[2][2]*m2.translation.z	in mm6
			pfacc(mm6_mm5)
			pfacc(mm6_mm6)								//undef|dst.translation.z				in mm6
			movd		[edx+TRSFRM_TRSL_Z],	mm6		//undef|dst.translation.z				in mm6
			femms

}
					
#else	//#if CODE_3DNOW

	dst.d[0][0] = m1.d[0][0] * m2.d[0][0] + m1.d[0][1] * m2.d[1][0] + m1.d[0][2] * m2.d[2][0];
	dst.d[0][1] = m1.d[0][0] * m2.d[0][1] + m1.d[0][1] * m2.d[1][1] + m1.d[0][2] * m2.d[2][1];
	dst.d[0][2] = m1.d[0][0] * m2.d[0][2] + m1.d[0][1] * m2.d[1][2] +  m1.d[0][2] * m2.d[2][2];

	dst.translation.x = m1.d[0][0] * m2.translation.x +	m1.d[0][1] * m2.translation.y +	m1.d[0][2] * m2.translation.z +	m1.translation.x;

	dst.d[1][0] = m1.d[1][0] * m2.d[0][0] + m1.d[1][1] * m2.d[1][0] +  m1.d[1][2] * m2.d[2][0];
	dst.d[1][1] = m1.d[1][0] * m2.d[0][1] + m1.d[1][1] * m2.d[1][1] +  m1.d[1][2] * m2.d[2][1];
	dst.d[1][2] = m1.d[1][0] * m2.d[0][2] + m1.d[1][1] * m2.d[1][2] +  m1.d[1][2] * m2.d[2][2];

	dst.translation.y = m1.d[1][0] * m2.translation.x + m1.d[1][1] * m2.translation.y +	m1.d[1][2] * m2.translation.z + m1.translation.y;

	dst.d[2][0] = m1.d[2][0] * m2.d[0][0] + m1.d[2][1] * m2.d[1][0] + m1.d[2][2] * m2.d[2][0];
	dst.d[2][1] = m1.d[2][0] * m2.d[0][1] + m1.d[2][1] * m2.d[1][1] + m1.d[2][2] * m2.d[2][1];
	dst.d[2][2] = m1.d[2][0] * m2.d[0][2] + m1.d[2][1] * m2.d[1][2] + m1.d[2][2] * m2.d[2][2];

	dst.translation.z = m1.d[2][0] * m2.translation.x +	m1.d[2][1] * m2.translation.y +	m1.d[2][2] * m2.translation.z +	m1.translation.z;

#endif	//#if CODE_3DNOW

	return GR_OK;
}

//
// Matrix and Transform operation on Vectors.
//
GENRESULT COMAPI x86MathEngine::transform(Vector & dst, const Matrix & m, const Vector & v)
{
	
#if CODE_3DNOW
	__asm{
			mov			ecx,	v
			mov			eax,	m
			
			movq		mm4,	[ecx]v.x		//v.y|v.x				in mm4
			movq		mm7,	[eax+MATRIX_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			;
			movq		mm3,	[eax+MATRIX_D_10]		//undef|m1.d[1][0]		in mm3
			pfmul(mm7_mm4)	//m1.d[0][1]*v.y|m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm0,	[eax+MATRIX_D_02]		//m1.d[0][2]|undef			in mm0
			movq		mm5,	[eax+MATRIX_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			;
			movd		mm2,	[ecx]v.z			//undef|v.z	in mm2
			pfmul(mm3_mm4)	//m1.d[1][1]*v.y|m1.d[1][0]*v.x			in mm3
			;
			punpckhdq	mm0,	[eax+MATRIX_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0
			;
			punpckldq	mm2,	mm2						//v.z|v.z	in mm2
			pfacc(mm7_mm3)	//m1.d[1][1]*v.y+m1.d[1][0]*v.x | m1.d[0][1]*v.y+m1.d[0][0]*v.x			in mm7
			;
			pfmul(mm0_mm2)	//m1.d[1][2]*v.z|m1.d[0][2]*v.z		in mm0
			;
			movd	mm6,	[eax+MATRIX_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm5_mm4)	//m1.d[2][1]*v.y|m1.d[2][0]*v.x			in mm5
			;
			mov			edx,	dst
			pfadd(mm0_mm7)	//  m1.d[1][0]*v.x+m1.d[1][1]*v.y+m1.d[1][2]*v.z | m1.d[0][0]*v.x+m1.d[0][1]*v.y+m1.d[0][2]*v.z		in mm0
			;
			pfacc(mm5_mm5)
			pfmul(mm6_mm2)								//undef| m1.d[2][2]*v.z		in mm6
			;
			movd		[edx+VECTOR_X],	mm0		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm0,	mm0						//dst.translation.y|dst.translation.y   in mm1
			;
			pfadd(mm6_mm5)								//undef|dst.translation.z				in mm6
			movd		[edx+VECTOR_Y],	mm0		//dst.translation.y|dst.translation.y   in mm1
			movd		[edx+VECTOR_Z],	mm6		//undef|dst.translation.z				in mm6
			femms
	}

#else	//#if CODE_3DNOW
	dst.x = m.d[0][0] * v.x + m.d[0][1] * v.y + m.d[0][2] * v.z;
	dst.y = m.d[1][0] * v.x + m.d[1][1] * v.y + m.d[1][2] * v.z;
	dst.z = m.d[2][0] * v.x + m.d[2][1] * v.y + m.d[2][2] * v.z;
#endif	//#if CODE_3DNOW

	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::transform(Vector & dst, const Transform & t, const Vector & v)
{
#if CODE_3DNOW
	__asm{
			mov			ecx,	v
			mov			eax,	t
			movq		mm4,	[ecx+VECTOR_X]			//v.y|v.x				in mm4
			movq		mm7,	[eax+TRSFRM_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			movq		mm3,	[eax+TRSFRM_D_10]		//m1.d[1][1]|m1.d[1][0]		in mm3
			pfmul(mm7_mm4)	//m1.d[0][1]*v.y|m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm0,	[eax+TRSFRM_D_02]		//m1.d[0][2]|undef			in mm0
			movq		mm5,	[eax+TRSFRM_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			;
			movd		mm2,	[ecx+VECTOR_Z]			//undef|v.z	in mm2
			pfmul(mm3_mm4)	//m1.d[1][1]*v.y|m1.d[1][0]*v.x			in mm3
			;
			punpckhdq	mm0,	[eax+TRSFRM_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0
			movd		mm1,	[eax+TRSFRM_TRSL_X]		//undef|m1.translation.x				in mm1			
			;
			punpckldq	mm2,	mm2						//v.z|v.z	in mm2
			pfacc(mm7_mm3)	//m1.d[1][1]*v.y+m1.d[1][0]*v.x | m1.d[0][1]*v.y+m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm1,	[eax+TRSFRM_TRSL_Y]		//m1.translation.y|m1.translation.x		in mm1
			pfmul(mm0_mm2)	//m1.d[1][2]*v.z|m1.d[0][2]*v.z		in mm0
			;
			punpckldq	mm6,	[eax+TRSFRM_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm5_mm4)	//m1.d[2][1]*v.x|m1.d[2][0]*v.x			in mm5
			;
			pfadd(mm0_mm7)	//  m1.d[1][0]*v.x+m1.d[1][1]*v.y+m1.d[1][2]*v.z | m1.d[0][0]*v.x+m1.d[0][1]*v.y+m1.d[0][2]*v.z		in mm0
			;
			mov			edx,	dst
			pfmul(mm6_mm2)								//m1.d[2][2]*v.z|undef		in mm6
			;
			pfadd(mm1_mm0)								//dst.translation.y|dst.translation.x	in mm1
			;
			punpckhdq	mm6,	[eax+TRSFRM_TRSL_Y]		//m1.translation.z|m1.d[2][2]*v.z	in mm6
			;
			movd		[edx+VECTOR_X],	mm1		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm1,	mm1						//dst.translation.y|dst.translation.y   in mm1
			;
			pfacc(mm6_mm5)
			movd		[edx+VECTOR_Y],	mm1		//dst.translation.y|dst.translation.y   in mm1
			pfacc(mm6_mm6)								//undef|dst.translation.z				in mm6
			movd		[edx+VECTOR_Z],	mm6		//undef|dst.translation.z				in mm6
			femms
	}

#else	//#if CODE_3DNOW
	dst.x = (t.d[0][0] * v.x) + (t.d[0][1] * v.y) +	(t.d[0][2] * v.z) +	(t.translation.x);
	dst.y = (t.d[1][0] * v.x) +	(t.d[1][1] * v.y) +	(t.d[1][2] * v.z) +	(t.translation.y);
	dst.z = (t.d[2][0] * v.x) +	(t.d[2][1] * v.y) +	(t.d[2][2] * v.z) +	(t.translation.z);
#endif	//#if CODE_3DNOW


	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::rotate(Vector & dst, const Transform & t, const Vector & v)
{
#if CODE_3DNOW
	__asm{
			mov			ecx,	v
			mov			eax,	t
			movq		mm4,	[ecx+VECTOR_X]			//v.y|v.x				in mm4
			movq		mm7,	[eax+TRSFRM_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			movq		mm3,	[eax+TRSFRM_D_10]		//m1.d[1][1]|m1.d[1][0]		in mm3
			pfmul(mm7_mm4)	//m1.d[0][1]*v.y|m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm0,	[eax+TRSFRM_D_02]		//m1.d[0][2]|undef			in mm0
			movq		mm5,	[eax+TRSFRM_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			;
			movd		mm2,	[ecx+VECTOR_Z]			//undef|v.z	in mm2
			pfmul(mm3_mm4)	//m1.d[1][1]*v.y|m1.d[1][0]*v.x			in mm3
			;
			punpckhdq	mm0,	[eax+TRSFRM_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0
			;
			punpckldq	mm2,	mm2						//v.z|v.z	in mm2
			pfacc(mm7_mm3)	//m1.d[1][1]*v.y+m1.d[1][0]*v.x | m1.d[0][1]*v.y+m1.d[0][0]*v.x			in mm7
			;
			movd		mm6,	[eax+TRSFRM_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm0_mm2)	//m1.d[1][2]*v.z|m1.d[0][2]*v.z		in mm0
			;
			pfmul(mm5_mm4)	//m1.d[2][1]*v.x|m1.d[2][0]*v.x			in mm5
			;
			mov			edx,	dst
			pfadd(mm0_mm7)	//  m1.d[1][0]*v.x+m1.d[1][1]*v.y+m1.d[1][2]*v.z | m1.d[0][0]*v.x+m1.d[0][1]*v.y+m1.d[0][2]*v.z		in mm0
			;
			pfmul(mm6_mm2)								//m1.d[2][2]*v.z|undef		in mm6
			pfacc(mm5_mm5)
			;
			movd		[edx+VECTOR_X],	mm0		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm0,	mm0						//dst.translation.y|dst.translation.y   in mm1
			;
			pfadd(mm6_mm5)								//undef|dst.translation.z				in mm6
			movd		[edx+VECTOR_Y],	mm0		//dst.translation.y|dst.translation.y   in mm1
			movd		[edx+VECTOR_Z],	mm6		//undef|dst.translation.z				in mm6
			femms
	}

#else	//#if CODE_3DNOW
	dst.x = (t.d[0][0] * v.x) +	(t.d[0][1] * v.y) +	(t.d[0][2] * v.z);
	dst.y = (t.d[1][0] * v.x) +	(t.d[1][1] * v.y) +	(t.d[1][2] * v.z);
	dst.z = (t.d[2][0] * v.x) +	(t.d[2][1] * v.y) +	(t.d[2][2] * v.z);
#endif

	return GR_OK;
}


GENRESULT COMAPI x86MathEngine::inverse_rotate(Vector & dst, const Transform & t, const Vector & v)
{
#if CODE_3DNOW
	__asm{
		mov			ecx,	v
		mov			eax,	t
		movq		mm0,	[ecx+VECTOR_X]		//v.y|v.x	in mm0
		movq		mm4,	[eax+TRSFRM_D_00]	//t.d[0][1]|t.d[0][0]			in mm4
		;
		movq		mm2,	mm0					//v.y|v.x	in mm2
		movq		mm1,	mm0					//v.y|v.x	in mm1
		;
		movq		mm5,	[eax+TRSFRM_D_10]	//t.d[1][1]|t.d[1][0]			in mm5
		punpckldq	mm1,	mm1					//v.x|v.x	in mm1
		;
		punpckhdq	mm2,	mm2					//v.y|v.y	in mm2
		pfmul(mm1_mm4)							//t.d[0][1]*v.x|t.d[0][0]*v.x	in mm1
		;
		movd		mm3,	[ecx+VECTOR_Z]		//undef|v.z						in mm3
		pfmul(mm2_mm5)							//t.d[1][1]*v.y|t.d[1][0]*v.y	in mm2
		;
		movq		mm6,	[eax+TRSFRM_D_20]	//t.d[2][1]|t.d[2][0]			in mm6
		punpckldq	mm7,	[eax+TRSFRM_D_02]	//t.d[0][2]|undef				in mm7
		;
		pfadd(mm2_mm1)							//t.d[0][1]*v.x+t.d[1][1]*v.y|t.d[0][0]*v.x+t.d[1][0]*v.y	in mm2
		punpckldq	mm3,	mm3					//v.z|v.z						in mm3
		;
		punpckhdq	mm7,	[eax+TRSFRM_D_11]	//t.d[1][2]|t.d[0][2]			in mm7
		pfmul(mm6_mm3)							//t.d[2][1]*v.z|t.d[2][0]*v.z	in mm6
		;
		movd		mm4,	[eax+TRSFRM_D_22]	//undef|t.d[2][2]				in mm4
		pfmul(mm7_mm0)							//t.d[1][2]*v.y|t.d[0][2]*v.x	in mm7
		;
		mov			edx,	dst
		pfadd(mm6_mm2)							//dst.y | dst.x					in mm6
		;
		pfmul(mm4_mm3)							//undef|t.d[2][2]*v.z			in mm4
		pfacc(mm7_mm7)							//undef|t.d[1][2]*v.y+t.d[0][2]*v.x		in mm7
		;
		movq		[edx+VECTOR_X],	mm6
		pfadd(mm4_mm7)							//undef|dst.z					in mm4
		movd		[edx+VECTOR_Z],	mm4			//undef|dst.z					in mm4
		femms
	}
#else	//#if CODE_3DNOW
	dst.x = (t.d[0][0] * v.x) +	(t.d[1][0] * v.y) +	(t.d[2][0] * v.z);
	dst.y = (t.d[0][1] * v.x) +	(t.d[1][1] * v.y) +	(t.d[2][1] * v.z);
	dst.z = (t.d[0][2] * v.x) +	(t.d[1][2] * v.y) +	(t.d[2][2] * v.z);
#endif

	return GR_OK;
}

//
// Transform Vector with Transpose of Matrix. The transpose might also happen
// to be the inverse in the case of a rotation matrix.
//
GENRESULT COMAPI x86MathEngine::transpose_transform(Vector & dst, const Matrix & m, const Vector & v)
{
#if CODE_3DNOW
	__asm{
		mov			ecx,	v
		mov			eax,	m
		movq		mm0,	[ecx+VECTOR_X]		//v.y|v.x	in mm0
		movq		mm4,	[eax+MATRIX_D_00]	//t.d[0][1]|t.d[0][0]			in mm4
		;
		movq		mm2,	mm0					//v.y|v.x	in mm2
		movq		mm1,	mm0					//v.y|v.x	in mm1
		;
		movq		mm5,	[eax+MATRIX_D_10]	//t.d[1][1]|t.d[1][0]			in mm5
		punpckldq	mm1,	mm1					//v.x|v.x	in mm1
		;
		punpckhdq	mm2,	mm2					//v.y|v.y	in mm2
		pfmul(mm1_mm4)							//t.d[0][1]*v.x|t.d[0][0]*v.x	in mm1
		;
		movd		mm3,	[ecx+VECTOR_Z]		//undef|v.z						in mm3
		pfmul(mm2_mm5)							//t.d[1][1]*v.y|t.d[1][0]*v.y	in mm2
		;
		movq		mm6,	[eax+MATRIX_D_20]	//t.d[2][1]|t.d[2][0]			in mm6
		punpckldq	mm7,	[eax+MATRIX_D_02]	//t.d[0][2]|undef				in mm7
		;
		pfadd(mm2_mm1)							//t.d[0][1]*v.x+t.d[1][1]*v.y|t.d[0][0]*v.x+t.d[1][0]*v.y	in mm2
		punpckldq	mm3,	mm3					//v.z|v.z						in mm3
		;
		punpckhdq	mm7,	[eax+MATRIX_D_11]	//t.d[1][2]|t.d[0][2]			in mm7
		pfmul(mm6_mm3)							//t.d[2][1]*v.z|t.d[2][0]*v.z	in mm6
		;
		movd		mm4,	[eax+MATRIX_D_22]	//undef|t.d[2][2]				in mm4
		pfmul(mm7_mm0)							//t.d[1][2]*v.y|t.d[0][2]*v.x	in mm7
		;
		mov			edx,	dst
		pfadd(mm6_mm2)							//dst.y | dst.x					in mm6
		;
		pfmul(mm4_mm3)							//undef|t.d[2][2]*v.z			in mm4
		pfacc(mm7_mm7)							//undef|t.d[1][2]*v.y+t.d[0][2]*v.x		in mm7
		;
		movq		[edx+VECTOR_X],	mm6
		pfadd(mm4_mm7)							//undef|dst.z					in mm4
		movd		[edx+VECTOR_Z],	mm4			//undef|dst.z					in mm4
		femms
	}

#else	//#if CODE_3DNOW
	dst.x = m.d[0][0] * v.x + m.d[1][0] * v.y + m.d[2][0] * v.z;
	dst.y = m.d[0][1] * v.x + m.d[1][1] * v.y + m.d[2][1] * v.z;
	dst.z = m.d[0][2] * v.x + m.d[1][2] * v.y + m.d[2][2] * v.z;
#endif
	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::inverse_transform(Vector & dst, const Transform & t, const Vector & v)
{
	
#if CODE_3DNOW
	__asm{
		mov			ecx,	v
		mov			eax,	t
		movq		mm0,	[ecx+VECTOR_X]		//v.y|v.x	in mm0
		movq		mm1,	[eax]t.translation.x	//t.translation.y|t.translation.x
		movq		mm5,	[eax+TRSFRM_D_10]	//t.d[1][1]|t.d[1][0]			in mm5
		pfsub(mm0_mm1)							//v.y-t.translation.y|v.x-t.translation.x	in mm0
		;
		movd		mm3,	[ecx+VECTOR_Z]		//undef|_z						in mm3
		movq		mm2,	mm0					//_y|_x	in mm2
		;
		movq		mm4,	[eax+TRSFRM_D_00]	//t.d[0][1]|t.d[0][0]			in mm4
		movq		mm1,	mm0					//_y|_x	in mm1
		;
		movd		mm6,	[eax]t.translation.z	
		punpckldq	mm1,	mm1					//_x|_x	in mm1
		;
		punpckhdq	mm2,	mm2					//_y|_y	in mm2
		pfmul(mm1_mm4)							//t.d[0][1]*_x|t.d[0][0]*_x	in mm1
		;
		pfsub(mm3_mm6)							//???| _z						in mm6
		pfmul(mm2_mm5)							//t.d[1][1]*_y|t.d[1][0]*_y	in mm2
		;
		movq		mm6,	[eax+TRSFRM_D_20]	//t.d[2][1]|t.d[2][0]			in mm6
		punpckldq	mm7,	[eax+TRSFRM_D_02]	//t.d[0][2]|undef				in mm7
		;
		pfadd(mm2_mm1)							//t.d[0][1]*_x+t.d[1][1]*_y|t.d[0][0]*_x+t.d[1][0]*_y	in mm2
		punpckldq	mm3,	mm3					//_z|_z						in mm3
		;
		punpckhdq	mm7,	[eax+TRSFRM_D_11]	//t.d[1][2]|t.d[0][2]			in mm7
		pfmul(mm6_mm3)							//t.d[2][1]*_z|t.d[2][0]*_z	in mm6
		;
		movd		mm4,	[eax+TRSFRM_D_22]	//undef|t.d[2][2]				in mm4
		pfmul(mm7_mm0)							//t.d[1][2]*_y|t.d[0][2]*_x	in mm7
		;
		mov			edx,	dst
		pfadd(mm6_mm2)							//dst.y | dst.x					in mm6
		;
		pfmul(mm4_mm3)							//undef|t.d[2][2]*_z			in mm4
		pfacc(mm7_mm7)							//undef|t.d[1][2]*_y+t.d[0][2]*_x		in mm7
		;
		movq		[edx+VECTOR_X],	mm6
		pfadd(mm4_mm7)							//undef|dst.z					in mm4
		movd		[edx+VECTOR_Z],	mm4			//undef|dst.z					in mm4
		femms
	}
#else	//#if CODE_3DNOW
	
	
	SINGLE _x,_y,_z;
//
// Invert translation of source vector
//
	_x = v.x - t.translation.x;
	_y = v.y - t.translation.y;
	_z = v.z - t.translation.z;

//
// Multiply inverse-translated source vector by inverted rotation transform
//

	dst.x = (t.d[0][0] * _x) + (t.d[1][0] * _y) + (t.d[2][0] * _z);
	dst.y = (t.d[0][1] * _x) + (t.d[1][1] * _y) + (t.d[2][1] * _z);
	dst.z = (t.d[0][2] * _x) + (t.d[1][2] * _y) + (t.d[2][2] * _z);
#endif

	return GR_OK;
}

//
// Same operations on lists of Vectors.
//
GENRESULT COMAPI x86MathEngine::transform_list(Vector * dst, const Matrix & m, const Vector * src, int n)
{
#if CODE_3DNOW
	__asm{
		mov	esi, n
		test	esi, esi
		jle	SHORT $transform_list_done
			mov			ecx,	src
			mov			eax,	m
			mov			edx,	dst
$transform_list_loop:
			movq		mm4,	[ecx]src.x				//v.y|v.x				in mm4
			movq		mm7,	[eax+MATRIX_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			;
			movq		mm3,	[eax+MATRIX_D_10]		//undef|m1.d[1][0]		in mm3
			pfmul(mm7_mm4)	//m1.d[0][1]*v.y|m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm0,	[eax+MATRIX_D_02]		//m1.d[0][2]|undef			in mm0
			movq		mm5,	[eax+MATRIX_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			;
			movd		mm2,	[ecx]src.z			//undef|v.z	in mm2
			pfmul(mm3_mm4)	//m1.d[1][1]*v.y|m1.d[1][0]*v.x			in mm3
			;
			punpckhdq	mm0,	[eax+MATRIX_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0
			;
			punpckldq	mm2,	mm2						//v.z|v.z	in mm2
			pfacc(mm7_mm3)	//m1.d[1][1]*v.y+m1.d[1][0]*v.x | m1.d[0][1]*v.y+m1.d[0][0]*v.x			in mm7
			;
			add		ecx, TYPE Vector
			pfmul(mm0_mm2)	//m1.d[1][2]*v.z|m1.d[0][2]*v.z		in mm0
			;
			movd	mm6,	[eax+MATRIX_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm5_mm4)	//m1.d[2][1]*v.y|m1.d[2][0]*v.x			in mm5
			;
			pfadd(mm0_mm7)	//  m1.d[1][0]*v.x+m1.d[1][1]*v.y+m1.d[1][2]*v.z | m1.d[0][0]*v.x+m1.d[0][1]*v.y+m1.d[0][2]*v.z		in mm0
			;
			pfacc(mm5_mm5)
			pfmul(mm6_mm2)								//undef| m1.d[2][2]*v.z		in mm6
			;
			movd		[edx+VECTOR_X],	mm0		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm0,	mm0						//dst.translation.y|dst.translation.y   in mm1
			;
			pfadd(mm6_mm5)								//undef|dst.translation.z				in mm6
			movd		[edx+VECTOR_Y],	mm0		//dst.translation.y|dst.translation.y   in mm1
			add		edx, TYPE Vector
			dec	esi
			movd		[edx+VECTOR_Z-TYPE Vector],	mm6		//undef|dst.translation.z				in mm6
			jne	SHORT $transform_list_loop
			femms
$transform_list_done:
	}

#else	//#if CODE_3DNOW		
	for (S32 i = 0; i < n; i++)
	{
	
		
		SINGLE _x,_y,_z;

		_x = src->x;
		_y = src->y;
		_z = src->z;
		dst->x = m.d[0][0] * _x + m.d[0][1] * _y + m.d[0][2] * _z;
		dst->y = m.d[1][0] * _x + m.d[1][1] * _y + m.d[1][2] * _z;
		dst->z = m.d[2][0] * _x + m.d[2][1] * _y + m.d[2][2] * _z;
		dst++;
		src++;
		//transform(*(dst++), m, *(src++));
	}
#endif
		
	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::transform_list(Vector * dst, const Transform & t, const Vector * src, int n)
{
	
#if CODE_3DNOW
	__asm{
		mov	esi, n
		test	esi, esi
		jle	SHORT $t_transform_list_done
			mov			ecx,	src
			mov			eax,	t
			mov			edx,	dst
$t_transform_list_loop:
			movq		mm4,	[ecx+VECTOR_X]			//v.y|v.x				in mm4
			movq		mm7,	[eax+TRSFRM_D_00]		//m1.d[0][1]|m1.d[0][0]					in mm7	
			movq		mm3,	[eax+TRSFRM_D_10]		//m1.d[1][1]|m1.d[1][0]		in mm3
			pfmul(mm7_mm4)	//m1.d[0][1]*v.y|m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm0,	[eax+TRSFRM_D_02]		//m1.d[0][2]|undef			in mm0
			movq		mm5,	[eax+TRSFRM_D_20]		//m1.d[2][1]|m1.d[2][0]					in mm5
			;
			movd		mm2,	[ecx+VECTOR_Z]			//undef|v.z	in mm2
			pfmul(mm3_mm4)	//m1.d[1][1]*v.y|m1.d[1][0]*v.x			in mm3
			;
			punpckhdq	mm0,	[eax+TRSFRM_D_11]		//m1.d[1][2]|m1.d[0][2]		in mm0
			movd		mm1,	[eax+TRSFRM_TRSL_X]		//undef|m1.translation.x				in mm1			
			;
			punpckldq	mm2,	mm2						//v.z|v.z	in mm2
			pfacc(mm7_mm3)	//m1.d[1][1]*v.y+m1.d[1][0]*v.x | m1.d[0][1]*v.y+m1.d[0][0]*v.x			in mm7
			;
			punpckldq	mm1,	[eax+TRSFRM_TRSL_Y]		//m1.translation.y|m1.translation.x		in mm1
			pfmul(mm0_mm2)	//m1.d[1][2]*v.z|m1.d[0][2]*v.z		in mm0
			;
			punpckldq	mm6,	[eax+TRSFRM_D_22]		//m1.d[2][2]|undef						in mm6		
			pfmul(mm5_mm4)	//m1.d[2][1]*v.x|m1.d[2][0]*v.x			in mm5
			;
			pfadd(mm0_mm7)	//  m1.d[1][0]*v.x+m1.d[1][1]*v.y+m1.d[1][2]*v.z | m1.d[0][0]*v.x+m1.d[0][1]*v.y+m1.d[0][2]*v.z		in mm0
			;
			pfmul(mm6_mm2)								//m1.d[2][2]*v.z|undef		in mm6
			add		ecx, TYPE Vector
			;
			pfadd(mm1_mm0)								//dst.translation.y|dst.translation.x	in mm1
			add		edx, TYPE Vector
			;
			punpckhdq	mm6,	[eax+TRSFRM_TRSL_Y]		//m1.translation.z|m1.d[2][2]*v.z	in mm6
			;
			movd		[edx+VECTOR_X-TYPE Vector],	mm1		//dst.translation.y|dst.translation.x	in mm1
			punpckhdq	mm1,	mm1						//dst.translation.y|dst.translation.y   in mm1
			;
			pfacc(mm6_mm5)
			movd		[edx+VECTOR_Y-TYPE Vector],	mm1		//dst.translation.y|dst.translation.y   in mm1
			pfacc(mm6_mm6)								//undef|dst.translation.z				in mm6
			dec	esi
			movd		[edx+VECTOR_Z-TYPE Vector],	mm6		//undef|dst.translation.z				in mm6
			jne	SHORT $t_transform_list_loop
			femms
$t_transform_list_done:

	}

#else	//#if CODE_3DNOW
	for (S32 i = 0; i < n; i++)
	{
		SINGLE _x,_y,_z;

		_x = src->x;
		_y = src->y;
		_z = src->z;
		
		dst->x = (t.d[0][0] * _x) +	(t.d[0][1] * _y) + 	(t.d[0][2] * _z) +	(t.translation.x);
		dst->y = (t.d[1][0] * _x) +	(t.d[1][1] * _y) +	(t.d[1][2] * _z) +	(t.translation.y);
		dst->z = (t.d[2][0] * _x) + (t.d[2][1] * _y) +	(t.d[2][2] * _z) +	(t.translation.z);

		dst++;
		src++;
	}
#endif

	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::transpose_transform_list(Vector * dst, const Matrix & m, const Vector * src, int n)
{
#if CODE_3DNOW
	__asm{
		mov	esi, n
		test	esi, esi
		jle	SHORT $m_inverse_transform_list_done
		mov			ecx,	src
		mov			eax,	m
		mov			edx,	dst
$m_inverse_transform_list_loop:
		
		movq		mm0,	[ecx+VECTOR_X]		//v.y|v.x	in mm0
		movq		mm4,	[eax+MATRIX_D_00]	//t.d[0][1]|t.d[0][0]			in mm4
		;
		movq		mm5,	[eax+MATRIX_D_10]	//t.d[1][1]|t.d[1][0]			in mm5
		movq		mm1,	mm0					//v.y|v.x	in mm1
		;
		movq		mm2,	mm0					//v.y|v.x	in mm2
		punpckldq	mm1,	mm1					//v.x|v.x	in mm1
		;
		punpckhdq	mm2,	mm2					//v.y|v.y	in mm2
		pfmul(mm1_mm4)							//t.d[0][1]*v.x|t.d[0][0]*v.x	in mm1
		;
		movd		mm3,	[ecx+VECTOR_Z]		//undef|v.z						in mm3
		pfmul(mm2_mm5)							//t.d[1][1]*v.y|t.d[1][0]*v.y	in mm2
		;
		movq		mm6,	[eax+MATRIX_D_20]	//t.d[2][1]|t.d[2][0]			in mm6
		punpckldq	mm7,	[eax+MATRIX_D_02]	//t.d[0][2]|undef				in mm7
		;
		pfadd(mm2_mm1)							//t.d[0][1]*v.x+t.d[1][1]*v.y|t.d[0][0]*v.x+t.d[1][0]*v.y	in mm2
		punpckldq	mm3,	mm3					//v.z|v.z						in mm3
		;
		punpckhdq	mm7,	[eax+MATRIX_D_11]	//t.d[1][2]|t.d[0][2]			in mm7
		pfmul(mm6_mm3)							//t.d[2][1]*v.z|t.d[2][0]*v.z	in mm6
		;
		movd		mm4,	[eax+MATRIX_D_22]	//undef|t.d[2][2]				in mm4
		pfmul(mm7_mm0)							//t.d[1][2]*v.y|t.d[0][2]*v.x	in mm7
		;
		pfadd(mm6_mm2)							//dst.y | dst.x					in mm6
		add		ecx, TYPE Vector
		;
		pfmul(mm4_mm3)							//undef|t.d[2][2]*v.z			in mm4
		pfacc(mm7_mm7)							//undef|t.d[1][2]*v.y+t.d[0][2]*v.x		in mm7
		;
		movq		[edx+VECTOR_X],	mm6
		;
		pfadd(mm4_mm7)							//undef|dst.z					in mm4
		add		edx, TYPE Vector
		;
		dec	esi
		movd		[edx+VECTOR_Z-TYPE Vector],	mm4			//undef|dst.z					in mm4
		jne	SHORT $m_inverse_transform_list_loop
		femms
$m_inverse_transform_list_done:
	}

#else	//#if CODE_3DNOW

	for (S32 i = 0; i < n; i++)
	{
		SINGLE _x,_y,_z;

		_x = src->x;
		_y = src->y;
		_z = src->z;
		
		dst->x = m.d[0][0] * _x + m.d[1][0] * _y + m.d[2][0] * _z;
		dst->y = m.d[0][1] * _x + m.d[1][1] * _y + m.d[2][1] * _z;
		dst->z = m.d[0][2] * _x + m.d[1][2] * _y + m.d[2][2] * _z;

		dst++;
		src++;
	}

#endif

	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::inverse_transform_list(Vector * dst, const Transform & t, const Vector * src, int n)
{
#if CODE_3DNOW
	__asm{
		mov	esi, n
		test	esi, esi
		jle	SHORT $t_inverse_transform_list_done
		mov			ecx,	src
		mov			eax,	t
		mov			edx,	dst
$t_inverse_transform_list_loop:

		movq		mm0,	[ecx+VECTOR_X]		//v.y|v.x	in mm0
		movq		mm1,	[eax]t.translation.x	//t.translation.y|t.translation.x
		movq		mm5,	[eax+TRSFRM_D_10]	//t.d[1][1]|t.d[1][0]			in mm5
		pfsub(mm0_mm1)							//v.y-t.translation.y|v.x-t.translation.x	in mm0
		;
		movd		mm3,	[ecx+VECTOR_Z]		//undef|_z						in mm3
		movq		mm2,	mm0					//_y|_x	in mm2
		;
		movq		mm4,	[eax+TRSFRM_D_00]	//t.d[0][1]|t.d[0][0]			in mm4
		movq		mm1,	mm0					//_y|_x	in mm1
		;
		movd		mm6,	[eax]t.translation.z	
		punpckldq	mm1,	mm1					//_x|_x	in mm1
		;
		punpckhdq	mm2,	mm2					//_y|_y	in mm2
		pfmul(mm1_mm4)							//t.d[0][1]*_x|t.d[0][0]*_x	in mm1
		;
		pfsub(mm3_mm6)							//???| _z						in mm6
		pfmul(mm2_mm5)							//t.d[1][1]*_y|t.d[1][0]*_y	in mm2
		;
		movq		mm6,	[eax+TRSFRM_D_20]	//t.d[2][1]|t.d[2][0]			in mm6
		punpckldq	mm7,	[eax+TRSFRM_D_02]	//t.d[0][2]|undef				in mm7
		;
		pfadd(mm2_mm1)							//t.d[0][1]*_x+t.d[1][1]*_y|t.d[0][0]*_x+t.d[1][0]*_y	in mm2
		punpckldq	mm3,	mm3					//_z|_z						in mm3
		;
		punpckhdq	mm7,	[eax+TRSFRM_D_11]	//t.d[1][2]|t.d[0][2]			in mm7
		pfmul(mm6_mm3)							//t.d[2][1]*_z|t.d[2][0]*_z	in mm6
		;
		movd		mm4,	[eax+TRSFRM_D_22]	//undef|t.d[2][2]				in mm4
		pfmul(mm7_mm0)							//t.d[1][2]*_y|t.d[0][2]*_x	in mm7
		;
		pfadd(mm6_mm2)							//dst.y | dst.x					in mm6
		add		ecx, TYPE Vector
		;
		pfmul(mm4_mm3)							//undef|t.d[2][2]*_z			in mm4
		pfacc(mm7_mm7)							//undef|t.d[1][2]*_y+t.d[0][2]*_x		in mm7
		;
		movq		[edx+VECTOR_X],	mm6
		;
		pfadd(mm4_mm7)							//undef|dst.z					in mm4
		add		edx, TYPE Vector
		dec	esi
		movd		[edx+VECTOR_Z-TYPE Vector],	mm4			//undef|dst.z					in mm4
		jne	SHORT $t_inverse_transform_list_loop
		femms
$t_inverse_transform_list_done:
	}
#else	//#if CODE_3DNOW
	
	for (S32 i = 0; i < n; i++)
	{
		SINGLE _x,_y,_z;

		//
		// Invert translation of source vector
		//
		_x = src->x - t.translation.x;
		_y = src->y - t.translation.y;
		_z = src->z - t.translation.z;
		
		//
		// Multiply inverse-translated source vector by inverted rotation transform
		//
		dst->x = (t.d[0][0] * _x) +	(t.d[1][0] * _y) +  (t.d[2][0] * _z);
		dst->y = (t.d[0][1] * _x) +	(t.d[1][1] * _y) +	(t.d[2][1] * _z);
		dst->z = (t.d[0][2] * _x) +	(t.d[1][2] * _y) +	(t.d[2][2] * _z);

		src++;
		dst++;
	//	inverse_transform(*(dst++), t, *(src++));
	}
#endif

	return GR_OK;
}

//
// Quaternion conversions.
//
static int nxt[3] = {1, 2, 0};


GENRESULT COMAPI x86MathEngine::matrix_to_quaternion(Quaternion & dst, const Matrix & m)
{
#if CODE_3DNOW

	SINGLE trace;
	SINGLE temp[2];
	//trace = m.d[0][0] + m.d[1][1] + m.d[2][2];
	__asm{
		mov		eax,	m
		movd	mm0,	[eax+MATRIX_D_00]			//???|	m.d[0][0]	in mm0
		movd	mm1,	[eax+MATRIX_D_11]			//???|	m.d[1][1]	in mm1
		movd	mm2,	[eax+MATRIX_D_22]			//???|	m.d[2][2]	in mm2
		movq	mm3,	mm0							//???|	m.d[0][0]					in mm3
		pfadd(mm3_mm1)							//???|m.d[0][0]+m.d[1][1]			in mm3
		movq	mm4,	QConst_1_1	
		pfadd(mm3_mm2)							//???|m.d[0][0]+m.d[1][1]+m.d[2][2]	in mm3
		movd	trace,	mm3							//???|trace							in mm3
	}

	if (*((int*)&trace) > 0)
	{
		//SINGLE st = (SINGLE) sqrt(trace + 1.0);
		//dst.w = st * 0.5;
		//st = 0.5 / st;
		//dst.x = (m.d[2][1] - m.d[1][2]) * st;
		//dst.y = (m.d[0][2] - m.d[2][0]) * st;
		//dst.z = (m.d[1][0] - m.d[0][1]) * st;
	__asm{
		mov		eax,	m
		pfadd(mm3_mm4)							//???|trace+1.0		in mm3
		;
		movd	mm0,	[eax+MATRIX_D_21]		//???| m.d[2][1]	in mm0
		;
		pfrsqrt(mm4_mm3)		
		movd	mm1,	[eax+MATRIX_D_12]		//???| m.d[1][2]	in mm1
		;
		punpckldq	mm7,	[eax+MATRIX_D_20]	//m.d[2][0]|???		in mm7
		;
		movq	mm5,	mm4
		pfmul(mm4_mm4)
		;
		movq	mm6,	QConst_05_05	
		pfsub(mm0_mm1)							//???| m.d[2][1]-m.d[1][2]			in mm0
		;
 		punpckhdq	mm7,	[eax+MATRIX_D_00]	//m.d[0][1]|m.d[2][0]	in mm7
		pfrsqit1(mm4_mm3)
		;
		movq		mm2,	[eax+MATRIX_D_02]	//m.d[1][0]|m.d[0][2]	in mm2
		pfmul(mm3_mm6)							//???|(trace+1.0)*0.5	in mm3
		;
		mov		edx,	dst
//		pfrcpit2(mm4_mm5)						//1/sqrt(trace+1.0)|1/sqrt(trace+1.0)	in mm4

			pfrcpit2(mm4_mm5)					//???|1/sqrt(trace+1.0)	in mm4
//			movq temp, mm4
			punpckldq	mm4,	mm4				//1/sqrt(trace+1.0)|1/sqrt(trace+1.0)	in mm4
//			movq temp, mm4

		;
		pfsub(mm2_mm7)							//m.d[1][0]-m.d[0][1]|m.d[0][2]-m.d[2][0]	in mm2	

//			movq temp, mm2

		pfmul(mm3_mm4)							//???|dst.w				in mm3
		pfmul(mm4_mm6)							//0.5/st|0.5/st			in mm4

//			movq temp, mm4

		movd	[edx]dst.w,	mm3					//???|dst.w				in mm3
		pfmul(mm0_mm4)							//???| (m.d[2][1]-m.d[1][2]) * st	in mm0
		pfmul(mm2_mm4)							//dst.z|dst.y			in mm2
		movd	[edx]dst.x,	mm0					//???| dst.x		in mm0
		movq	[edx]dst.y,	mm2					//dst.z|dst.y			in mm2

//			movq temp, mm2
//			punpckhdq	mm2,	[eax+MATRIX_D_20]	//m.d[2][0]|dst.z		in mm2
//			movd [edx]dst.z, mm2

		femms
		}
	}
	else
	{
		int i;	// j == 1 k== 2
		//if (m.d[1][1] > m.d[0][0])
		//{
		//	i = 1;	// j == 2 k== 0
		//}
		//if (m.d[2][2] > m.d[i][i])
		//{
		//	i = 2;	// j == 0 k== 1
		//}
		__asm{
			movq		mm4,	mm1				//???|m.d[1][1]					in mm4
			movq		mm3,	mm1				//???|m.d[1][1]					in mm3
			pfmax(mm4_mm0)						//???|max(m.d[1][1],m.d[0][0])	in mm4
			punpckldq	mm3,	mm2				//m.d[2][2]|m.d[1][1]			in mm3			
			punpckldq	mm0,	mm4				//max(m.d[1][1],m.d[0][0])|	m.d[0][0]	in mm0
			pfcmpgt(mm3_mm0)					//(m.d[2][2]>max(m.d[1][1],m.d[0][0]))|(m.d[1][1]>m.d[0][0])	in mm3
			psrlq		mm3,	31				//???|i in						mm3
			movd		i,		mm3
			movq		mm3,	QConst_1_1		//1.0|1.0		in mm3
		}
		
		switch(i&3){

		case 0:	//i == 0 j == 1 k== 2
		{
		//SINGLE stor = (SINGLE) sqrt((m.d[0][0] - (m.d[1][1] + m.d[2][2])) + 1.0);
		//dst.x = st * 0.5;
		//st = 0.5 / st;
		//dst.w = (m.d[2][1] - m.d[1][2]) * stor;
		//dst.y = (m.d[1][0] + m.d[0][1]) * stor;
		//dst.z = (m.d[2][0] + m.d[0][2]) * stor;
		__asm{
		mov		eax,	m
		pfadd(mm3_mm0)							//???|	m.d[0][0]					in mm3
		movd	mm7,	[eax+MATRIX_D_21]		//???|m.d[2][1]		in mm7
		pfsub(mm3_mm1)							//???|m.d[0][0]+m.d[1][1]			in mm3
		movd	mm6,	[eax+MATRIX_D_12]		//???|m.d[1][2]		in mm6
		movd	mm0,	[eax+MATRIX_D_10]		//???|m.d[1][0]				in mm0
		pfsub(mm3_mm2)							//???|m.d[0][0]+m.d[1][1]+m.d[2][2]	in mm3
		movd		mm1,	[eax+MATRIX_D_01]	//???|m.d[0][0]				in mm1
		;
		pfsub(mm7_mm6)							//???|m.d[2][1]-m.d[1][2]		in mm7
		punpckldq	mm0,	[eax+MATRIX_D_20]	//m.d[2][0]|m.d[1][0]		in mm0
		;
		pfrsqrt(mm4_mm3)		
		punpckldq	mm1,	[eax+MATRIX_D_02]	//m.d[0][2]|m.d[0][1]		in mm1
		;
		movq	mm5,	mm4
		;
		pfmul(mm4_mm4)
		punpckldq	mm3,	mm3
		;
		pfadd(mm0_mm1)
		pfrsqit1(mm4_mm3)
		;
		movq	mm6,	QConst_05_05	
		pfmul(mm3_mm6)				//???|(trace+1.0)*0.5	in mm3
		;
		pfrcpit2(mm4_mm5)			//1/sqrt(...+1.0)|1/sqrt(...+1.0)	in mm4
		;
		pfmul(mm3_mm4)				//???|dst.w				in mm3
		mov		edx,	dst
		;
		pfmul(mm4_mm6)				//0.5/st|0.5/st			in mm4
		movd	[edx]dst.x,	mm3		//???|dst.w				in mm3
		pfmul(mm7_mm4)				//???|(m.d[2][1]-m.d[1][2])*st	in mm7
		pfmul(mm0_mm4)
		movd	[edx]dst.w,	mm7		//???|(m.d[2][1]-m.d[1][2])*st	in mm7
		movq	[edx]dst.y,	mm0
		femms
		}
		
		 }
		break;
		case 1:	//i == 1 j == 2 k== 0
		{
		
		//SINGLE st = (SINGLE) sqrt((m.d[1][1] - (m.d[2][2] + m.d[0][0])) + 1.0);
		//dst.y = st * 0.5;
		//st = 0.5 / st;
		//dst.w = (m.d[0][2] - m.d[2][0]) * st;
		//dst.x = (m.d[0][1] + m.d[1][0]) * st;
		//dst.z = (m.d[2][1] + m.d[1][2]) * st;

		__asm{
		mov		eax,	m
		pfadd(mm3_mm1)							//???|m.d[1][1]+ 1.0				in mm3
		movd	mm7,	[eax+MATRIX_D_02]		//???|m.d[0][2]		in mm7
		pfsub(mm3_mm0)							//???|m.d[1][1]-m.d[0][0]			in mm3
		movd	mm6,	[eax+MATRIX_D_20]		//???|m.d[2][0]		in mm6
		punpckhdq	mm0,	[eax+MATRIX_D_00]	//m.d[0][1]|???				in mm0
		;
		pfsub(mm3_mm2)							//???|m.d[1][1]-m.d[0][0]-m.d[2][2]	in mm3
		punpckhdq	mm1,	[eax+MATRIX_D_02]	//m.d[1][0]|???				in mm1
		;
		pfrsqrt(mm4_mm3)		
		punpckhdq	mm0,	[eax+MATRIX_D_20]	//m.d[2][1]|m.d[0][1]		in mm0
		;
		pfsub(mm7_mm6)							//???|m.d[0][2]-m.d[2][0]		in mm7
		punpckhdq	mm1,	[eax+MATRIX_D_11]	//m.d[1][2]|m.d[1][0]		in mm1
		;
		movq	mm5,	mm4
		;
		pfmul(mm4_mm4)
		punpckldq	mm3,	mm3
		;
		pfadd(mm0_mm1)
		pfrsqit1(mm4_mm3)
		;
		movq	mm6,	QConst_05_05	
		pfmul(mm3_mm6)					//???|(trace+1.0)*0.5	in mm3
		;
		pfrcpit2(mm4_mm5)				//1/sqrt(...+1.0)|1/sqrt(...+1.0)	in mm4
		pfmul(mm3_mm4)					//???|dst.w				in mm3
		mov		edx,	dst
		pfmul(mm4_mm6)					//0.5/st|0.5/st			in mm4
		movd	[edx]dst.y,	mm3			//???|dst.y				in mm3
		pfmul(mm7_mm4)					//???|(m.d[0][2]-m.d[2][0])*st	in mm7
		pfmul(mm0_mm4)
		movd		[edx]dst.w,	mm7		//???|(m.d[0][2]-m.d[2][0])*st	in mm7
		movd		[edx]dst.x,	mm0
		punpckhdq	mm0,	mm0
		movd		[edx]dst.z,	mm0
		femms
		}

		}
		break;
		case 2:
		case 3://i == 2 j == 0 k== 1
		{
		//SINGLE st = (SINGLE) sqrt((m.d[2][2] - (m.d[0][0] + m.d[1][1])) + 1.0);
		//dst.z = st * 0.5;
		//st = 0.5 / st;
		//dst.w = (m.d[1][0] - m.d[0][1]) * st;
		//dst.x = (m.d[0][2] + m.d[2][0]) * st;
		//dst.y = (m.d[1][2] + m.d[2][1]) * st;
		__asm{
		mov				eax,	m
		pfadd(mm3_mm2)								//???|m.d[2][2]+ 1.0				in mm3
		movd			mm7,	[eax+MATRIX_D_10]	//???|m.d[1][0]		in mm7
		pfsub(mm3_mm0)								//???|m.d[1][1]-m.d[0][0]			in mm3
		movd			mm6,	[eax+MATRIX_D_01]	//???|m.d[0][1]		in mm6
		;
		pfsub(mm3_mm1)								//???|m.d[1][1]-m.d[0][0]-m.d[2][2]	in mm3
		punpckldq	mm0,	[eax+MATRIX_D_02]		//m.d[0][2]|???				in mm0
		;
		pfsub(mm7_mm6)								//???|m.d[1][0]-m.d[0][1]		in mm7
		movq			mm1,	[eax+MATRIX_D_20]	//m.d[2][1]|m.d[2][0]		in mm1
		;
		pfrsqrt(mm4_mm3)		
		punpckhdq		mm0,	[eax+MATRIX_D_11]	//m.d[1][2]|m.d[0][2]		in mm0
		;
		movq			mm5,	mm4
		;
		pfmul(mm4_mm4)
		punpckldq		mm3,	mm3
		;
		pfadd(mm0_mm1)
		pfrsqit1(mm4_mm3)
		;
		movq			mm6,	QConst_05_05	
		pfmul(mm3_mm6)								//???|(...+1.0)*0.5	in mm3
		;
		pfrcpit2(mm4_mm5)							//1/sqrt(...+1.0)|1/sqrt(...+1.0)	in mm4
		;
		pfmul(mm3_mm4)								//???|dst.z				in mm3
		mov				edx,	dst
		;
		pfmul(mm4_mm6)								//0.5/st|0.5/st			in mm4
		movd	[edx]dst.z,	mm3						//???|dst.z				in mm3
		pfmul(mm7_mm4)								//???|(m.d[1][0]-m.d[0][1])*st	in mm7
		pfmul(mm0_mm4)
		movd		[edx]dst.w,	mm7					//???|(m.d[1][0]-m.d[0][1])*st	in mm7
		movd		[edx]dst.x,	mm0
		punpckhdq	mm0,	mm0
		movd		[edx]dst.y,	mm0
		femms
		}

		}
		break;
		}
	}

#else	//#if CODE_3DNOW
	
	SINGLE trace = m.d[0][0] + m.d[1][1] + m.d[2][2];
	if (trace > 0.0)
	{
		SINGLE st = (SINGLE) sqrt(trace + 1.0);
		dst.w = st * 0.5;
		st = 0.5 / st;

		dst.x = (m.d[2][1] - m.d[1][2]) * st;
		dst.y = (m.d[0][2] - m.d[2][0]) * st;
		dst.z = (m.d[1][0] - m.d[0][1]) * st;
	}
	else
	{
		int i = 0;	// j == 1 k== 2
		if (m.d[1][1] > m.d[0][0])
		{
			i = 1;	// j == 2 k== 0
		}
		if (m.d[2][2] > m.d[i][i])
		{
			i = 2;	// j == 0 k== 1
		}

		int j = nxt[i];
		int k = nxt[j];

		SINGLE st = (SINGLE) sqrt((m.d[i][i] - (m.d[j][j] + m.d[k][k])) + 1.0);

		dst.d[i+1] = st * 0.5;
		st = 0.5 / st;
		dst.w = (m.d[k][j] - m.d[j][k]) * st;
		dst.d[j+1] = (m.d[j][i] + m.d[i][j]) * st;
		dst.d[k+1] = (m.d[k][i] + m.d[i][k]) * st;
	}
#endif

	return GR_OK;
}

//
GENRESULT COMAPI x86MathEngine::quaternion_to_matrix(Matrix & dst, const Quaternion & q)
{
#if CODE_3DNOW
	
	//SINGLE s = 2.0 / (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
	//SINGLE xs = q.x * s;
	
	//SINGLE wx = q.w * xs;
	//SINGLE xx = q.x * xs;

	//SINGLE ys = q.y * s;
	//SINGLE zs = q.z * s;

	//SINGLE wy = q.w * ys;
	//SINGLE xz = q.x * zs;
	//SINGLE xy = q.x * ys;
	//SINGLE wz = q.w * zs;

	//SINGLE yy = q.y * ys;
	//SINGLE zz = q.z * zs;
	//SINGLE yz = q.y * zs;

	//dst.d[0][0] = 1.0 - (yy + zz);
	//dst.d[1][1] = 1.0 - (xx + zz);
	//dst.d[2][2] = 1.0 - (xx + yy);

//	dst.d[1][0] = xy + wz;
//	dst.d[2][1] = yz + wx;
//	dst.d[0][2] = xz + wy;

//	dst.d[2][0] = xz - wy;
//	dst.d[0][1] = xy - wz;
//	dst.d[1][2] = yz - wx;

	__asm{

			mov		eax,	q
			movq	mm0,	[eax]q.w		//q.x|q.w			in mm0
			movq	mm1,	[eax]q.y		//q.z|q.y			in mm1
			movq	mm2,	mm0				//q.x|q.w			in mm2
			movq	mm3,	mm1				//q.z|q.y			in mm3
			pfmul(mm2_mm2)					//q.x*q.x|q.w*q.w	in mm2
			;
			movq		mm5,	mm0			//q.x|q.w			in mm5
			pfmul(mm3_mm3)					//q.z*q.z|q.y*q.y	in mm3
			;
			pfacc(mm3_mm2)					//q.x*q.x+q.w*q.w|q.z*q.z+q.y*q.y	in mm3
			;
			pfacc(mm3_mm3)					//q.x*q.x+q.w*q.w+q.z*q.z+q.y*q.y|q.x*q.x+q.w*q.w+q.z*q.z+q.y*q.y	in mm3
			;
			movq	mm2,	Qfconst_2_2		//2.0|2.0		in mm2
			pfrcp(mm4_mm3)
			;
			pfrcpit1(mm3_mm4)
			mov		edx,	dst
			;
			pfrcpit2(mm3_mm4)
			movq		mm7,	mm0			//q.x|q.w				in mm7
			;
			pfmul(mm2_mm3)					//s|s			in mm2
			punpckhdq	mm7,	mm7			//q.x|q.x				in mm7
			;
			movq	mm6,	mm0				//q.x|q.w				in mm6	
			pfmul(mm5_mm2)					//q.x*s(xs)|???		in mm5
			;
			pfmul(mm2_mm1)					//q.z*s(zs)|q.y*s(ys)	in mm2
			punpckldq	mm7,	mm0			//q.w|q.x				in mm7
			;
			punpckhdq	mm5,	mm5			//xs|xs				in mm5
			;
			movq	mm3,	mm1				//q.z|q.y					in mm3
			pfmul(mm5_mm0)					//q.x*xs(xx)|q.w * xs(wx)	in mm5
			;
			pfmul(mm6_mm2)					//q.x*zs(xz)|q.w*ys(wy)	in mm6
			;
			pfmul(mm7_mm2)					//q.w*zs(wz)|q.x*ys(xy)				in mm7
			
			pfmul(mm3_mm2)					//q.z*zs(zz)|q.y*ys(yy)		in mm3
			punpckhdq	mm2,	mm2			//zs|zs						in mm2
			;
			pfmul(mm2_mm1)					//???|q.y*zs(yz)			in mm2
			movq		mm0,	mm3			//zz|yy			in mm0
			;
			movq		mm1,	QConst_1_1
			punpckhdq	mm0,	mm5			//xx|zz			in mm0
			;
			pfacc(mm0_mm3)					//zz+yy|xx+zz	in mm0
			;
			punpckldq	mm4,	mm3			//yy|???		in mm4
			;
			pfsub(mm1_mm0)					//1-(zz+yy)|1-(xx+zz)	in mm1
			pfadd(mm4_mm5)					//xx+yy|???		in mm4
			;
			movq		mm0,	QConst_1_1
			movq		mm3,	mm6				//xz | wy		in mm3
			;
			movd		[edx+MATRIX_D_11],	mm1
			punpckhdq	mm1,	mm1				//???|1-(zz+yy)	in mm1
			;
			pfsub(mm0_mm4)						//1-(xx+yy)|???		in mm0
			movd		[edx+MATRIX_D_00],	mm1	//???|1-(zz+yy)	in mm1
			;
			movq		mm1,	mm5				//xx | wx		in mm1
			pfacc(mm3_mm3)						//???|xz+wy		in mm3
			;
			punpckhdq	mm0,	mm0				//???|1-(xx+yy)		in mm0
			punpckldq	mm1,	mm2				//yz | wx		in mm1
			;
			movd		[edx+MATRIX_D_22],	mm0	//???|1-(xx+yy)		in mm0
			pfacc(mm1_mm7)						//wz+xy|yz+wx	in mm1
			;
			movq		mm0,	mm6				//xz | wy		in mm0
			movd		[edx+MATRIX_D_02],	mm3	//???|xz+wy		in mm3
			;
			punpckhdq	mm0,	mm0				//xz | xz		in mm0
			pfsub(mm2_mm5)						//???|yz-wx		in mm2
			;
			movq		mm4,	mm7				//wz | xy		in mm4
			pfsub(mm0_mm6)						//???|xz-wy		in mm0
			;
			movd		[edx+MATRIX_D_21], mm1	//wz+xy|yz+wx	in mm1
			punpckhdq	mm4,	mm4				//wz | wz		in mm4
			;
			punpckhdq	mm1,	mm1				//wz+xy|wz+xy	in mm1
			pfsub(mm7_mm4)						//???| xy-wz	in mm7
			;
			movd		[edx+MATRIX_D_10], mm1	//wz+xy|wz+xy	in mm1
			movd		[edx+MATRIX_D_20], mm0	//???|xz-wy		in mm0
			movd		[edx+MATRIX_D_12], mm2	//???|yz-wx		in mm2
			movd		[edx+MATRIX_D_01], mm7	//???| xy-wz	in mm7

			femms
	}

#else
	
	SINGLE s = 2.0 / (q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);

	SINGLE xs = q.x * s;
	SINGLE ys = q.y * s;
	SINGLE zs = q.z * s;

	SINGLE wx = q.w * xs;
	SINGLE wy = q.w * ys;
	SINGLE wz = q.w * zs;

	SINGLE xx = q.x * xs;
	SINGLE xy = q.x * ys;
	SINGLE xz = q.x * zs;

	SINGLE yy = q.y * ys;
	SINGLE yz = q.y * zs;
	SINGLE zz = q.z * zs;

	dst.d[0][0] = 1.0 - (yy + zz);
	dst.d[1][0] = xy + wz;
	dst.d[2][0] = xz - wy;
	
	dst.d[0][1] = xy - wz;
	dst.d[1][1] = 1.0 - (xx + zz);
	dst.d[2][1] = yz + wx;
	
	dst.d[0][2] = xz + wy;
	dst.d[1][2] = yz - wx;
	dst.d[2][2] = 1.0 - (xx + yy);

#endif
	return GR_OK;
}

//
// Quaternion interpolation.
//


GENRESULT COMAPI x86MathEngine::quat_slerp(Quaternion & dst, const Quaternion & q1, const Quaternion & q2, SINGLE t)
{
#if CODE_3DNOW

	Quaternion sum, dif;
	SINGLE s2,s1;
	SINGLE cos_omega, omega_limit;
	//sum.w = q1.w + q2.w;
	//sum.x = q1.x + q2.x;
	//sum.y = q1.y + q2.y;
	//sum.z = q1.z + q2.z;
	//s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

	//dif.w = q1.w - q2.w;
	//dif.x = q1.x - q2.x;
	//dif.y = q1.y - q2.y;
	//dif.z = q1.z - q2.z;
	//s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
/*
	if (s1 > s2)
	{
		qs.w = -q2.w;
		qs.x = -q2.x;
		qs.y = -q2.y;
		qs.z = -q2.z;
	}
	else
	{
		qs = q2;
	}

	cos_omega = q1.w * qs.w + q1.x * qs.x + q1.y * qs.y + q1.z * qs.z;
	omega_limit = cos_omega + hi__limit;
*/
	__asm{
			mov		eax	,	q1
			mov		ecx,	q2
			movq	mm0,	[eax]q1.w		//q1.x|q1.w		in mm0
			movq	mm2,	[ecx]q2.w		//q2.x|q2.w		in mm2
			movq	mm1,	[eax]q1.y		//q1.z|q1.y		in mm0
			movq	mm4,	mm0				//q1.x|q1.w		in mm4
			;
			movq	mm3,	[ecx]q2.y		//q2.z|q2.y		in mm2
			pfadd(mm0_mm2)					//sum.x|sum.w	in mm0
			;
			movq	mm5,	mm1				//q1.z|q1.y		in mm4
			pfsub(mm4_mm2)					//dif.x|dif.w	in mm4
			;
			pfadd(mm1_mm3)					//sum.z|sum.y	in mm1
			pfmul(mm0_mm0)					//sum.x*sum.x|sum.w*sum.w	in mm0
			;
			pfsub(mm5_mm3)					//dif.z|dif.y	in mm5
			pfmul(mm4_mm4)					//dif.x*dif.x|dif.w*dif.w	in mm4
			;
			pfmul(mm1_mm1)					//sum.z*sum.z|sum.y*sum.y	in mm1
			pfmul(mm5_mm5)					//dif.z*dif.z|dif.y*dif.y	in mm5
			pfacc(mm1_mm0)
			pfacc(mm5_mm4)
			pfacc(mm1_mm1)					//s2|s2			in mm1
			pfacc(mm5_mm5)					//s1|s1			in mm5
			;
			movd	s2,		mm1
			;		
			movd	s1,		mm5
			pfcmpgt(mm5_mm1)				//(s1 > s2)|(s1 > s2)	in mm5
			;
			por		mm5,	QInt_1_1
			;
			pi2fd(mm5_mm5)					//((s1 > s2)?-1.0:+1.0)|((s1 > s2)?-1.0:+1.0)	in mm5
			;
			movq	mm6,	[eax]q1.w		//q1.x|q1.w		in mm6
			;
			pfmul(mm2_mm5)					//qs.x|qs.w		in mm2
			;
			movq	mm7,	[eax]q1.y		//q1.z|q1.y		in mm7
			pfmul(mm3_mm5)					//qs.z|qs.y		in mm3
			;
			movq	qs.w,	mm2				//qs.x|qs.w		in mm2
			pfmul(mm2_mm6)					//q1.x*qs.x|q1.w*qs.w		in mm2
			;
			movq	qs.y,	mm3				//qs.z|qs.y		in mm3
			pfmul(mm3_mm7)					//q1.z*qs.z|q1.y*qs.y		in mm3
			;
			movd	mm0,	hi__limit
			pfacc(mm3_mm2)
			pfacc(mm3_mm3)					//???|cos_omega				in mm3
			pfadd(mm0_mm3)
			movd	cos_omega,	mm3			//???|cos_omega				in mm3
			movd	omega_limit,	mm0		//???|omega_limit	in mm0
	}

// Check for cases where rotations are nearly opposite, which causes numerical
// weirdness.
	//if ((1.0 + cos_omega) > Q_EPSILON)
	if ( *(int*)&omega_limit > 0)
	{
	// Check for case where rotations are very close, which also causes 
	// weirdness. 
		//if ((1.0 - cos_omega) > Q_EPSILON)
		if (*(int*)&hi__limit > *(int*)&cos_omega)
		{
			SINGLE omega;
			//SINGLE omega = acos(cos_omega);
			//SINGLE sin_omega = sin(omega);
			//s1 = sin((1.0 - t) * omega) / sin_omega;
			//s2 = sin(t * omega) / sin_omega;

		__asm{
			movd	mm0,	QConst_1_1		//???|1.0			in mm0
			movd	mm1,	cos_omega		//???|cos_omega		in mm1
			movq	mm3,	mm1
			pfmul(mm1_mm1)					//???|cos_omega*cos_omega	in mm1
			pfrcp(mm4_mm3)
			pfsub(mm0_mm1)					//???|1-cos_omega*cos_omega
			pfrcpit1(mm3_mm4)
			pfrsqrt(mm1_mm0)
			pfrcpit2(mm3_mm4)				//???|1/cos_omega
			movq	mm2,	mm1
			pfmul(mm1_mm1)
			pfrsqit1(mm1_mm0)
			pfmul(mm0_mm3)
			pfrcpit2(mm1_mm2)				//1/sqrt(...)|1/sqrt(...)	in mm1
			pfmul(mm0_mm1)					//???|sin_omega				in mm0
			movq	rcprc_sin_omega, mm1
			call    AMD3D_atan
			movd	mm2,	QConst_1_1
			pxor	mm1,	mm1				//???|0.0					in mm1
			movd	mm3,	t
			pfcmpgt(mm1_mm0)				//???|(0 > omega)			in mm1
			movd	mm4,	Q_PI_PI
			pfsub(mm2_mm3)					//???|1.0 - t			in mm2
			pand	mm1,	mm4			//???|(0 > omega)&Q_PI_PI	in mm1
			pfadd(mm0_mm1)					//???|
			pfmul(mm3_mm0)					//???|t * omega			in mm3
			pfmul(mm0_mm2)					//???|(1.0 - t) * omega	in mm0
			movd	omega,		mm3			//???|t * omega			in mm3
			call	AMD3D_sin
			movd	mm1,	rcprc_sin_omega	
			pfmul(mm1_mm0)					//???|s1	in mm1
			movd	mm0,	omega
			movd	s1,		mm1
			call	AMD3D_sin
			movd	mm1,	rcprc_sin_omega	
			pfmul(mm1_mm0)					//???|s2	in mm1
			//movd	s2,		mm1
			movd		mm0,	s1
			punpckldq	mm1,	mm1			//s2|s2		in mm1
			punpckldq	mm0,	mm0			//s1|s1		in mm0
		}

		}
		else
		{
		// Too close, do straight linear interpolation.
			__asm {
				movd		mm1,	t
				movq		mm0,	QConst_1_1			//1.0|1.0				in mm0
				punpckldq	mm1,	mm1				//t(s2)|t(s2)			in mm1
				pfsub(mm0_mm1)						//1.0-t(s1)|1.0-t(s1)	in mm0				
			}
			//s1 = 1.0 - t;
			//s2 = t;
		}

		__asm{
			mov		eax	,	q1			
			movq	mm3,	qs.w			//qs.x|qs.w			in mm2
			movq	mm2,	[eax]q1.w		//q1.x|q1.w			in mm2
			;
			movq	mm4,	[eax]q1.y		//q1.z|q1.y			in mm4
			pfmul(mm3_mm1)					//s2*qs.x|s2*qs.w	in mm2
			;
			movq	mm5,	qs.y			//qs.z|qs.y			in mm5
			pfmul(mm2_mm0)					//s1*q1.x|s1*q1.w	in mm2
			;
			pfmul(mm4_mm0)					//s1*q1.z|s1*q1.y	in mm4
			;
			pfmul(mm5_mm1)					//s2*qs.x|s2*qs.w	in mm5
			pfadd(mm2_mm3)					//dst.x|dst.w		in mm2
			;
			mov		edx,	dst
			pfadd(mm4_mm5)					//dst.x|dst.w		in mm2
			;
			movq	[edx]dst.w,	mm2			//dst.x|dst.w		in mm2
			movq	[edx]dst.y,	mm4			//dst.x|dst.w		in mm2
			femms
		}
/*
		dst.w = s1 * q1.w + s2 * qs.w;
		dst.x = s1 * q1.x + s2 * qs.x;
		dst.y = s1 * q1.y + s2 * qs.y;
		dst.z = s1 * q1.z + s2 * qs.z;
*/
	}
	else
	{
		SINGLE half_pi = SINGLE(PI / 2.0);
/*
		s1 = sin((1.0 - t) * half_pi);
		s2 = sin(t * half_pi);
		dst.w = q1.z;
		dst.x = s1 * q1.x + s2 * (-q1.y);
		dst.y = s1 * q1.y + s2 * q1.x;
		dst.z = s1 * q1.z + s2 * (-q1.w);
*/
		__asm{
			movd	mm0,	QConst_1_1
			movd	mm1,	t
			movd	mm2,	half_pi
			pfsub(mm0_mm1)
			pfmul(mm0_mm2)
			call	AMD3D_sin
			movd	s1,		mm0
			movd	mm1,	half_pi
			movd	mm0,	t
			pfmul(mm0_mm1)
			call	AMD3D_sin
			mov			eax,	q1		
			punpckldq	mm0,	mm0		//s2|s2		in mm0
			;
			movq		mm2,	[eax]q1.w	//q1.x|q1.w		in mm2
			pxor		mm4,	mm4
			;
			movd		mm1,	s1		//???|s1	in mm1
			pxor		mm7,	mm7
			;
			movq		mm3,	[eax]q1.y	//q1.z|q1.y		in mm3
			pfsub(mm4_mm2)					//???|-q1.w		in mm4
			;
			punpckhdq	mm2,	mm2			//q1.x|q1.x		in mm2
			punpckldq	mm1,	mm1		//s1|s1		in mm1
			;
			movq		mm6,	mm3			//???|q1.y		in mm6
			pfsub(mm7_mm0)				//-s2|-s2	in mm7
			;
			punpckldq	mm2,	mm4			//-q1.w | q1.x		in mm2
			pfmul(mm3_mm1)					//s1*q1.z|s1q1.y		in mm3
			;
			movq		mm5,	mm2			//???|q1.x		in mm5
			pfmul(mm6_mm7)				//???|-s2*q1.y		in mm6
			;
			mov		edx,	dst			
			pfmul(mm5_mm1)				//???|s1*q1.x		in mm5
			;
			pfmul(mm2_mm0)					//s2*(-q1.w) | s2*q1.x		in mm2
			;
			movd		mm1,	[eax]q1.z
			pfadd(mm5_mm6)				//???|dst.x			in mm5
			;
			pfadd(mm3_mm2)				//dst.z|dst.y	in mm3
			movd	[edx]dst.w,	mm1
			movd	[edx]dst.x,	mm5		//???|dst.x			in mm5
			movq	[edx]dst.y,	mm3
			femms
		}
	}	


#else	
	// Make sure we're taking the shorter arc.

	Quaternion sum, dif, qs;

	sum.w = q1.w + q2.w;
	sum.x = q1.x + q2.x;
	sum.y = q1.y + q2.y;
	sum.z = q1.z + q2.z;

	dif.w = q1.w - q2.w;
	dif.x = q1.x - q2.x;
	dif.y = q1.y - q2.y;
	dif.z = q1.z - q2.z;

	SINGLE s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
	SINGLE s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

	if (s1 > s2)
	{
		qs.w = -q2.w;
		qs.x = -q2.x;
		qs.y = -q2.y;
		qs.z = -q2.z;
	}
	else
	{
		qs = q2;
	}

	SINGLE cos_omega = q1.w * qs.w + q1.x * qs.x + q1.y * qs.y + q1.z * qs.z;

// Check for cases where rotations are nearly opposite, which causes numerical
// weirdness.
	if ((1.0 + cos_omega) > Q_EPSILON)
	{
	// Check for case where rotations are very close, which also causes 
	// weirdness. 
		if ((1.0 - cos_omega) > Q_EPSILON)
		{
			SINGLE omega = acos(cos_omega);
			SINGLE sin_omega = sin(omega);
	
			s1 = sin((1.0 - t) * omega) / sin_omega;
			s2 = sin(t * omega) / sin_omega;
		}
		else
		{
		// Too close, do straight linear interpolation.
			s1 = 1.0 - t;
			s2 = t;
		}

		dst.w = s1 * q1.w + s2 * qs.w;
		dst.x = s1 * q1.x + s2 * qs.x;
		dst.y = s1 * q1.y + s2 * qs.y;
		dst.z = s1 * q1.z + s2 * qs.z;
	}
	else
	{
		dst.w =  q1.z;
		dst.x = -q1.y;
		dst.y =  q1.x;
		dst.z = -q1.w;

		SINGLE half_pi = SINGLE(PI / 2.0);

		s1 = sin((1.0 - t) * half_pi);
		s2 = sin(t * half_pi);

		dst.x = s1 * q1.x + s2 * dst.x;
		dst.y = s1 * q1.y + s2 * dst.y;
		dst.z = s1 * q1.z + s2 * dst.z;
	}
#endif

	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::transform(Vector & dst, const Quaternion & q, const Vector & v)
{

#if CODE_3DNOW
	//SINGLE dot_prod1, dot_prod2;
	
	//dot_prod1 = q.w * q.w - (q.x * q.x + q.y * q.y + q.z * q.z);
	//dst.x = v.x * dot_prod1;
	//dst.y = v.y * dot_prod1;
	//dst.z = v.z * dot_prod1;
	
	//dot_prod2 = 2.0 * (q.x * v.x + q.y * v.y + q.z * v.z);
	//dst.x += q.x * dot_prod2;
	//dst.y += q.y * dot_prod2;
	//dst.z += q.z * dot_prod2;
	
	//dst.x += (q.y * v.z - q.z * v.y)* q.w * 2.0;
	//dst.y += (q.z * v.x - q.x * v.z)* q.w * 2.0;
	//dst.z += (q.x * v.y - q.y * v.x)* q.w * 2.0;

	__asm{
			mov		eax,	q
			mov		ecx,	v

			movq	mm0,	[eax]q.w		//q.x|q.w			in mm0
			movq	mm1,	[eax]q.y		//q.z|q.y			in mm1
			movd	mm5,	[ecx]v.x	//???|v.x
			movq	mm2,	mm0		//q.x|q.w			in mm2
			movq	mm3,	mm1		//q.z|q.y			in mm3
			pfmul(mm2_mm2)			//q.x*q.x|q.w*q.w			in mm2
			;
			punpckhdq	mm0,	mm0		//q.x|q.x		in mm0
			pfmul(mm3_mm3)			//q.z*q.z|q.y*q.y	in mm3
			;
			movq	mm4,	mm2		//q.x*q.x|q.w*q.w			in mm4
			pfmul(mm0_mm5)				//???|q.x*v.x
			;
			pfacc(mm3_mm3)			//q.z*q.z+q.y*q.y|q.z*q.z+q.y*q.y	in mm3
			punpckhdq	mm4,	mm4 //???|q.x*q.x				in mm4
			;
			movq		mm6,	[ecx]v.y	//v.z|v.y					in mm6
			pfsub(mm2_mm4)			//???|q.w*q.w - q.x*q.x		in mm2
			;
			pfsub(mm2_mm3)			//???|dot_prod1		in mm2
			movq		mm7,	mm6			//v.z|v.y					in mm7
			;
			pfmul(mm5_mm2)				//???|v.x*dot_prod1(dst.x)
			punpckldq	mm2,	mm2			//dot_prod1|dot_prod1		in mm2
			;
			pfmul(mm2_mm6)					//v.z*dot_prod1(dst.z)|v.y*dot_prod1(dst.y)	in mm2
			;
			pfmul(mm6_mm1)		//q.z * v.z|q.y * v.y	in mm6
			movq	mm3,	Qfconst_2_2
			;
			movq		mm4,	mm1		//q.z|q.y		in mm1
			pfacc(mm6_mm6)		//???|q.z * v.z+q.y * v.y	in mm6
			;
			punpckhdq	mm4,	mm4		//q.z|q.z		in mm1
			;
			pfadd(mm6_mm0)		//???|q.z * v.z+q.y * v.y+q.x*v.x	in mm6
			movd	mm0,	[eax]q.x		//???|q.x		in mm0
			;
			punpckldq	mm4,	mm1		//q.y|q.z		in mm4
			;
			pfmul(mm6_mm3)		//???|dot_prod2	in mm6	
			;
			pfmul(mm4_mm7)		//q.y*v.z|q.z*v.y			in mm4
			movd	mm7,	Qfconst_2_2
			;
			pfmul(mm0_mm6)					//???|q.x * dot_prod2	in mm0
			punpckldq	mm6,	mm6		//dot_prod2|dot_prod2			in mm6
			;
			pfmul(mm6_mm1)				//q.z*dot_prod2|q.y*dot_prod2	in mm6
			movq		mm3,	mm4		//q.y*v.z|q.z*v.y			in mm3
			;
			pfadd(mm5_mm0)					//???dst.x				in mm5
			punpckhdq	mm3,	mm3		//q.y*v.z|q.y*v.z			in mm3
			;
			pfadd(mm2_mm6)				//dst.z|dst.y					in mm2
			movd	mm0,	[eax]q.w
			;
			movd		mm6,[eax]q.x			//???|q.x		in mm6
			pfsub(mm3_mm4)				//???|q.y * v.z-q.z * v.y	in mm3
			;
			pfmul(mm7_mm0)			//???|q.w * 2.0
			punpckldq	mm6,	mm1				//q.y|q.x		in mm6
			;
			mov		edx,	dst
			punpckhdq	mm1,	mm1				//q.z|q.z		in mm1
			;
			pfmul(mm3_mm7)				//???|(q.y * v.z - q.z * v.y)* q.w * 2.0	in mm3
			movq		mm0,	[ecx]v.x		//v.y|v.x				in mm0
			;
			punpckldq	mm1,	mm6				//q.x|q.z		in mm1
			;
			pfadd(mm5_mm3)				//???|dst.x		in mm5
			movd		mm3,	[ecx]v.z		//???|v.z		in mm3
			;
			pfmul(mm1_mm0)				//q.x * v.y|q.z * v.x	in mm1
			punpckldq	mm3,	mm0				//v.x|v.z		in mm3
			;
			punpckldq	mm7,	mm7			//q.w * 2.0|q.w * 2.0	in mm7
			pfmul(mm6_mm3)						//q.y*v.x|q.x*v.z		in mm6
			;
			pfsub(mm1_mm6)			//q.x * v.y-q.y*v.x|q.z * v.x-q.x*v.z		in mm1
			movd	[edx]dst.x,	mm5		//???|dst.x		in mm5
			pfmul(mm1_mm7)
			pfadd(mm2_mm1)			//dst.z|dst.y	in mm2
			movq	[edx]dst.y,	mm2	//dst.z|dst.y	in mm2
			femms
	}

#else
	
	dst = v * (q.s * q.s - dot_product(q.v, q.v));
	dst += q.v * 2.0 * dot_product(q.v, v);
	dst += cross_product(q.v, v) * q.s * 2.0;
#endif

	return GR_OK;
}

//

GENRESULT COMAPI x86MathEngine::mul(Quaternion & dst, const Quaternion & q1, const Quaternion & q2)
{

#if CODE_3DNOW
	
	//dst.w = q1.w * q2.w - (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z);
	//dst.x = q1.x * q2.w + q2.x * q1.w + (q1.y * q2.z - q1.z * q2.y);
	//dst.y = q1.y * q2.w + q2.y * q1.w + (q1.z * q2.x - q1.x * q2.z);
	//dst.z = q1.z * q2.w + q2.z * q1.w + (q1.x * q2.y - q1.y * q2.x);

	__asm{

			mov		eax,	q1
			mov		ecx,	q2
			movq	mm0,	[eax]q1.w		//q1.x|q1.w				in mm0
			movq	mm1,	[ecx]q2.w		//q2.x|q2.w				in mm1
			movq	mm2,	[eax]q1.y		//q1.z|q1.y				in mm2
			movq	mm3,	[ecx]q2.y		//q2.z|q2.y				in mm3
			pfmul(mm0_mm1)			//q1.x*q2.x|q1.w*q2.w			in mm0
			;
			mov		edx,	dst
			movq		mm5,	mm1			//q2.x|q2.w				in mm5
			;
			movq		mm4,	mm0		//q1.x*q2.x|q1.w*q2.w		in mm4
			punpckhdq	mm0,	mm0		//q1.x*q2.x|q1.x*q2.x		in mm0
			;
			pfmul(mm2_mm3)					//q1.z*q2.z|q1.y*q2.y	in mm2
			pfsub(mm4_mm0)				//???|q1.w*q2.w-q1.x*q2.x	in mm4
			;
			punpckhdq	mm5,	mm5			//q2.x|q2.x				in mm5
			movq		mm6,	mm3			//q2.z|q2.y			in mm6
			;
			pfacc(mm2_mm2)					//???|q1.z*q2.z+q1.y*q2.y	in mm2
			punpckldq	mm5,	mm1			//q2.w|q2.x				in mm5
			;
			pfmul(mm5_eax)					//q1.x*q2.w|q1.w*q2.x	in mm5
			punpckhdq	mm6,	mm6			//q2.z|q2.z			in mm6
			;
			pfsub(mm4_mm2)					//???|dst.w				in mm4
			movq		mm7,	[eax]q1.y	//q1.z|q1.y			in mm7
			;
			pfacc(mm5_mm5)					//???|q1.x*q2.w+q1.w*q2.x	in mm5
			punpckldq	mm6,	mm3			//q2.y|q2.z			in mm6
			;
			pfmul(mm6_mm7)			//q1.z*q2.y|q1.y*q2.z	in mm6
			movd	[edx]dst.w,	mm4			//???|dst.w				in mm4
			;
			movd		mm2,	[eax]q1.w	//???|q1.w		in mm2
			movq		mm0,	mm1			//q2.x|q2.w			in mm0
			;
			pfadd(mm5_mm6)					//???|q1.x*q2.w+q1.w*q2.x+q1.y * q2.z	in mm5
			punpckhdq	mm6,	mm6			//q1.y*q2.z|q1.y*q2.z	in mm6
			;
			punpckldq	mm0,	mm0			//q2.w|q2.w			in mm0
			punpckldq	mm2,	mm2			//q1.w|q1.w		in mm2
			;
			pfsub(mm5_mm6)					//???|dst.x				in mm5
			pfmul(mm0_mm7)					//q1.z*q2.w|q1.y*q2.w			in mm0
			;
			punpckhdq	mm7,	[eax]q1.w	//q1.x|q1.z		in mm7
			pfmul(mm2_mm3)					//q2.z*q1.w|q2.y*q1.w		in mm2
			;
			punpckhdq	mm1,	mm1			//q2.x|q2.x		in mm1
			movd		[edx]dst.x,	mm5		//???|dst.x				in mm5
			;
			pfadd(mm2_mm0)					//q1.z*q2.w+q2.z*q1.w|q2.y*q1.w+q1.y*q2.w		in mm2
			punpckldq	mm1,	mm3			//q2.y|q2.x		in mm1
			;
			pfmul(mm7_mm1)					//q1.x*q2.y|q1.z*q2.x		in mm7
			movd		mm4,	[eax]q1.x	//???|q1.x		in mm4
			;
			punpckhdq	mm3,	[ecx]q2.w	//q2.x|q2.z		in mm3
			;
			pfadd(mm2_mm7)		//q1.z*q2.w+q2.z*q1.w+q1.x*q2.y|q2.y*q1.w+q1.y*q2.w+q1.z*q2.x	in mm2		
			punpckldq	mm4,	[eax]q1.y	//q1.y|q1.x		in mm4
			pfmul(mm3_mm4)					//q2.x*q1.y|q2.z*q1.x		in mm3
			pfsub(mm2_mm3)					//dst.z|dst.y	in mm2
			movq		[edx]dst.y,	mm2		//dst.z|dst.y	in mm2
			femms
	}

#else
	dst.s = q1.s * q2.s - dot_product(q1.v, q2.v);
	dst.v = q1.v * q2.s + q2.v * q1.s + cross_product(q1.v, q2.v);
#endif
	return GR_OK;
}

#include "inv_sqrt.h"

static ISQRT inv_sqrt_obj;

GENRESULT COMAPI x86MathEngine::InvSqrt(SINGLE & dst, const SINGLE x)
{
	dst = inv_sqrt_obj.InvSqrt(x);
	return GR_OK;
}

GENRESULT COMAPI x86MathEngine::Sqrt(SINGLE & dst, const SINGLE x)
{
	dst = x * inv_sqrt_obj.InvSqrt(x);
	return GR_OK;
}

//--------------------------------------------------------------------------//
//------------------------End x86Math.cpp-----------------------------------//
//--------------------------------------------------------------------------//
