#ifndef DAVARIANT_H
#define DAVARIANT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DAVariant.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/DAVariant.h 5     4/28/00 11:57p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef US_TYPEDEFS
#include "typedefs.h"      // General type definitions
#endif

#ifndef _INC_MEMORY
#include <memory.h>
#endif


#pragma pack (push, 8)

enum DAVARENUM
{
	DAVT_INTEGER = 0x1000,
	DAVT_CONSTANT = 0x2000,
	DAVT_BYREF = 0x4000,
	DAVT_SIGNED = 0x8000,
	DAVT_EMPTY=0,
	DAVT_U8=1 | DAVT_INTEGER,
	DAVT_U16= 2 | DAVT_INTEGER,
	DAVT_U32= 3 | DAVT_INTEGER,
	DAVT_U64 = 4 | DAVT_INTEGER,
	DAVT_SINGLE=5,
	DAVT_DOUBLE=6,
	DAVT_COMPONENT=7,
	DAVT_DISPATCH=8,
	DAVT_STRING=9,
	DAVT_PVOID=10,
	DAVT_RECT=11,
	DAVT_VARIANT=12,
	DAVT_VECTOR=13,
	DAVT_MATRIX=14,
	DAVT_TRANSFORM=15,
	DAVT_PANE=16,
	DAVT_WINDOW=17,
	DAVT_OBJNODE=18,
	DAVT_MESH=19,


	DAVT_PU8=(DAVT_U8 | DAVT_BYREF),
	DAVT_PU16=(DAVT_U16 | DAVT_BYREF),
	DAVT_PU32=(DAVT_U32 | DAVT_BYREF),
	DAVT_PU64=(DAVT_U64 | DAVT_BYREF),
	DAVT_PSINGLE=DAVT_SINGLE | DAVT_BYREF,
	DAVT_PDOUBLE=DAVT_DOUBLE | DAVT_BYREF,
	DAVT_S8=DAVT_U8|DAVT_SIGNED,
	DAVT_S16=DAVT_U16|DAVT_SIGNED,
	DAVT_S32=DAVT_U32|DAVT_SIGNED,
	DAVT_S64=DAVT_U64|DAVT_SIGNED,
	DAVT_PS8=DAVT_STRING,
	DAVT_PS16=(DAVT_S16|DAVT_BYREF),
	DAVT_PS32=(DAVT_S32|DAVT_BYREF),
	DAVT_PS64=(DAVT_S64|DAVT_BYREF),
	DAVT_BOOL32=DAVT_S32,
	DAVT_PBOOL32=DAVT_PS32,
	DAVT_PRECT=DAVT_RECT | DAVT_BYREF,
	DAVT_PVARIANT=DAVT_VARIANT | DAVT_BYREF,
	DAVT_PVECTOR=DAVT_VECTOR | DAVT_BYREF,
	DAVT_PMATRIX=DAVT_MATRIX | DAVT_BYREF,
	DAVT_PTRANSFORM=DAVT_TRANSFORM | DAVT_BYREF,
	DAVT_PPANE=DAVT_PANE | DAVT_BYREF,
	DAVT_PWINDOW=DAVT_WINDOW | DAVT_BYREF,
	DAVT_PPOBJNODE=DAVT_OBJNODE | DAVT_BYREF,
	DAVT_PMESH=DAVT_MESH | DAVT_BYREF,


	DAVT_TYPEMASK	= 0xfff
};

//--------------------------------------------------------------------------//
//------------------------------Property------------------------------------//
//--------------------------------------------------------------------------//

struct PROPERTY
{
   const C8  *name;
   U32        value;
   SINGLE     value2;
   DAVARENUM  type;
};

//
// Macro to Define a Property/value combination
//          _        _
//
// Values of any type (pointers, ints, etc.) are typecast to an unsigned
// 32-bit value for property storage
//

#define DP(n,v)            (const C8 *) n, (U32)((void *) (v)),                      0, DAVT_U32
#define DP_SINGLE(n,v)     n, 0, static_cast<SINGLE>(v),                                DAVT_SINGLE
#define DP_U32(n,v)        n, (U32)((void *) static_cast<U32>(v)),                   0, DAVT_U32
#define DP_STRING(n,v)     n, (U32)((void *) static_cast<const char *>(v)),          0, DAVT_STRING
#define DP_COMPONENT(n,v)  n, (U32)((void *) static_cast<struct IDAComponent *>(v)), 0, DAVT_COMPONENT
#define DP_PRECT(n,v)      n, (U32)((void *) static_cast<struct tagRECT *>(v)),      0, DAVT_PRECT
#define DP_PVECTOR(n,v)    n, (U32)((void *) static_cast<class Vector *>(v)),        0, DAVT_PVECTOR
#define DP_PMATRIX(n,v)    n, (U32)((void *) static_cast<class Matrix *>(v)),        0, DAVT_PMATRIX
#define DP_PTRANSFORM(n,v) n, (U32)((void *) static_cast<class Transform *>(v)),     0, DAVT_PTRANSFORM
#define DP_PPANE(n,v)      n, (U32)((void *) static_cast<struct ViewRect *>(v)),     0, DAVT_PPANE
#define DP_PWINDOW(n,v)    n, (U32)((void *) static_cast<struct _window *>(v)),      0, DAVT_PWINDOW
#define DP_PPOBJNODE(n,v)  n, (U32)((void *) static_cast<struct OBJNODE **>(v)),     0, DAVT_PPOBJNODE
#define DP_PMESH(n,v)	   n, (U32)((void *) static_cast<struct Mesh *>(v)),		 0, DAVT_PMESH
#define DP_OBJLIST(n,v)    n, (U32)((void *) static_cast<struct OBJNODE **>(v)),     0, DAVT_PPOBJNODE
#define DP_PVOID(n,v)      n, (U32)((void *) static_cast<void *>(v)),                0, DAVT_PVOID


//--------------------------------------------------------------------------//
//------------------------------DACOM_VARIANT-------------------------------//
//--------------------------------------------------------------------------//


struct DACOM_VARIANT
{
	DAVARENUM variantType;
	U32		  dwReserved1;

	union 
	{
		U32						longVal;
		U64						longLongVal;
		SINGLE					singleVal;
		DOUBLE					doubleVal;
		struct IDAComponent *	component;
		struct IDADispatch *	dispatch;
		const C8 *				stringVal;
		U32 *					pLongVal;
		SINGLE *				pSingleVal;
		DOUBLE *				pDoubleVal;
		struct DACOM_VARIANT *	pVariant;
		void *					pVoid;
		struct tagRECT *		pRect;
		class  Vector *		    pVector;
		class  Matrix *		    pMatrix;
		class  Transform *      pTransform;
		struct ViewRect *		pPane;
		struct _window *		pWindow;
		struct OBJNODE **		ppObjNode;
		struct Mesh *			pMesh;
	};


	DACOM_VARIANT (void)
	{
		memset(this, 0, sizeof(*this));
	}

	DACOM_VARIANT (U8 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_U8;
	}

	DACOM_VARIANT (U16 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_U16;
	}

	DACOM_VARIANT (U32 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_U32;
	}

	DACOM_VARIANT (U64 _val)
	{
		longLongVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_U64;
	}

	DACOM_VARIANT (SINGLE _val)
	{
		singleVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_SINGLE;
	}

	DACOM_VARIANT (DOUBLE _val)
	{
		doubleVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_DOUBLE;
	}

	DACOM_VARIANT (struct IDAComponent *_val)
	{
		component = _val;
		dwReserved1 = 0;
		variantType = DAVT_COMPONENT;
	}

	DACOM_VARIANT (struct IDADispatch *_val)
	{
		dispatch = _val;
		dwReserved1 = 0;
		variantType = DAVT_DISPATCH;
	}

	DACOM_VARIANT (const C8 *_val)
	{
		stringVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_STRING;
	}

	DACOM_VARIANT (const void *_val)
	{
		pVoid = (void *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PVOID;
	}

	DACOM_VARIANT (const struct DACOM_VARIANT *_val)
	{
		pVariant = (struct DACOM_VARIANT *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PVARIANT;
	}

	DACOM_VARIANT (const struct tagRECT *_val)
	{
		pRect = (struct tagRECT *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PRECT;
	}

	DACOM_VARIANT (const class Vector *_val)
	{
		pVector = (class Vector *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PVECTOR;
	}

	DACOM_VARIANT (const class Matrix *_val)
	{
		pMatrix = (class Matrix *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PMATRIX;
	}

	DACOM_VARIANT (const class Transform *_val)
	{
		pTransform = (class Transform *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PTRANSFORM;
	}

	DACOM_VARIANT (const struct ViewRect *_val)
	{
		pPane = (struct ViewRect *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PPANE;
	}

	DACOM_VARIANT (const struct _window *_val)
	{
		pWindow = (struct _window *)_val;
		dwReserved1 = 0;
		variantType = DAVT_PWINDOW;
	}

	DACOM_VARIANT (const struct OBJNODE **_val)
	{
		ppObjNode = (struct OBJNODE **)_val;
		dwReserved1 = 0;
		variantType = DAVT_PPOBJNODE;
	}

	DACOM_VARIANT (const struct Mesh * _val)
	{
		pMesh = (struct Mesh *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PMESH;
	}

	DACOM_VARIANT (const U8 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PU8;
	}

	DACOM_VARIANT (const U16 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PU16;
	}

	DACOM_VARIANT (const U32 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PU32;
	}

	DACOM_VARIANT (const U64 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PU64;
	}

	DACOM_VARIANT (const SINGLE *_val)
	{
		pSingleVal = (SINGLE *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PSINGLE;
	}

	DACOM_VARIANT (const DOUBLE *_val)
	{
		pDoubleVal = (DOUBLE *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PDOUBLE;
	}

	DACOM_VARIANT (S8 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_S8;
	}

	DACOM_VARIANT (S16 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_S16;
	}

	DACOM_VARIANT (S32 _val)
	{
		longVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_S32;
	}

	DACOM_VARIANT (S64 _val)
	{
		longLongVal = _val;
		dwReserved1 = 0;
		variantType = DAVT_S64;
	}

	DACOM_VARIANT (const S16 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PS16;
	}

	DACOM_VARIANT (const S32 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PS32;
	}

	DACOM_VARIANT (const S64 *_val)
	{
		pLongVal = (U32 *) _val;
		dwReserved1 = 0;
		variantType = DAVT_PS64;
	}


	DACOM_VARIANT (int _intVal)
	{
		longVal = _intVal;
		dwReserved1 = 0;
		variantType = DAVT_S32;
	}

	DACOM_VARIANT (unsigned int _intVal)
	{
		longVal = _intVal;
		dwReserved1 = 0;
		variantType = DAVT_U32;
	}

	DACOM_VARIANT (const PROPERTY & property)
	{
		variantType = property.type;
		dwReserved1 = 0;
		if (variantType == DAVT_SINGLE)
			singleVal = property.value2;
		else
			pVoid = (void *) property.value;
	}

	//---------------------
	//
	operator S32 (void)
	{
		S32 result;

		if ((variantType & DAVT_BYREF) == 0 && (variantType & DAVT_INTEGER))
			result = longVal;
		else
		if (variantType == DAVT_SINGLE)
			result = (S32) singleVal;
		else
		if (variantType == DAVT_DOUBLE)
			result = (S32) doubleVal;
		else
			result = 0;

		return result;
	}

	//---------------------
	//
	operator U32 (void)
	{
		U32 result;

		if ((variantType & DAVT_BYREF) == 0 && (variantType & DAVT_INTEGER))
			result = longVal;
		else
		if (variantType == DAVT_SINGLE)
			result = (U32) singleVal;
		else
		if (variantType == DAVT_DOUBLE)
			result = (U32) doubleVal;
		else
			result = 0;

		return result;
	}

	//---------------------
	//
	operator U64 (void)
	{
		U64 result;

		if ((variantType & DAVT_BYREF) == 0 && variantType & DAVT_INTEGER)
		{
			if ((variantType & DAVT_TYPEMASK) == (DAVT_U64&DAVT_TYPEMASK))
				result = longLongVal;
			else
			if (variantType & DAVT_SIGNED)
				result = (U64) ((S32) longVal);
			else
				result = (U64) longVal;
		}
		else
		if (variantType == DAVT_SINGLE)
			result = (U64) singleVal;
		else
		if (variantType == DAVT_DOUBLE)
			result = (U64) doubleVal;
		else
			result = 0;

		return result;
	}

	//---------------------
	//
	operator SINGLE (void)
	{
		SINGLE result;

		if ((variantType & DAVT_BYREF) == 0 && variantType & DAVT_INTEGER)
		{
			if ((variantType & DAVT_TYPEMASK) == (DAVT_U64&DAVT_TYPEMASK))
			{
				if (variantType & DAVT_SIGNED)
					result = (SINGLE) ((S64) longLongVal);
				else
					result = (SINGLE) ((S64) longLongVal);		// this is wrong because of compiler limitation!!
			}
			else
			if (variantType & DAVT_SIGNED)
				result = (SINGLE) ((S32) longVal);
			else
				result = (SINGLE) longVal;
		}
		else
		if (variantType == DAVT_SINGLE)
			result = singleVal;
		else
		if (variantType == DAVT_DOUBLE)
			result = (SINGLE) doubleVal;
		else
			result = 0;

		return result;
	}

	//---------------------
	//
	operator DOUBLE (void)
	{
		DOUBLE result;

		if ((variantType & DAVT_BYREF) == 0 && variantType & DAVT_INTEGER)
		{
			if ((variantType & DAVT_TYPEMASK) == (DAVT_U64&DAVT_TYPEMASK))
			{
				if (variantType & DAVT_SIGNED)
					result = (DOUBLE) ((S64)longLongVal);
				else
					result = (DOUBLE) ((S64)longLongVal);	// this is wrong because of compiler limitation!!!
			}
			else
			if (variantType & DAVT_SIGNED)
				result = (DOUBLE) ((S32)longVal);
			else
				result = (DOUBLE) longVal;
		}
		else
		if (variantType == DAVT_SINGLE)
			result = (DOUBLE) singleVal;
		else
		if (variantType == DAVT_DOUBLE)
			result = doubleVal;
		else
			result = 0;

		return result;
	}

	//---------------------
	//
	operator struct IDAComponent * (void)
	{
		if (variantType == DAVT_COMPONENT || variantType == DAVT_DISPATCH)
			return component;
		return 0;
	}

	//---------------------
	//
	operator struct IDADispatch * (void)
	{
		if (variantType == DAVT_DISPATCH)
			return dispatch;
		return 0;
	}

	//---------------------
	//
	operator const C8 * (void)
	{
		if (variantType == DAVT_STRING)
			return stringVal;
		return 0;
	}

	//---------------------
	//
	operator void * (void)
	{
		if (variantType == DAVT_PVOID || (variantType & DAVT_BYREF))
			return pVoid;
		return 0;
	}

	//---------------------
	//
	operator struct tagRECT * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PRECT)
			return pRect;
		return 0;
	}

	//---------------------
	//
	operator class Vector * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PVECTOR)
			return pVector;
		return 0;
	}

	//---------------------
	//
	operator class Matrix * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PMATRIX)
			return pMatrix;
		return 0;
	}

	//---------------------
	//
	operator class Transform * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PTRANSFORM)
			return pTransform;
		return 0;
	}

	//---------------------
	//
	operator struct ViewRect * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PPANE)
			return pPane;
		return 0;
	}

	//---------------------
	//
	operator struct _window * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PWINDOW)
			return pWindow;
		return 0;
	}

	//---------------------
	//
	operator struct OBJNODE ** (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PPOBJNODE)
			return ppObjNode;
		return 0;
	}

	//---------------------
	//
	operator struct Mesh * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PMESH)
			return pMesh;
		return 0;
	}

	//---------------------
	//
	operator struct DACOM_VARIANT * (void)
	{
		if (variantType == DAVT_PVOID || variantType == DAVT_PVARIANT)
			return pVariant;
		return 0;
	}

	//---------------------
	//
	operator U8 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U8&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (U8 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator U16 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U16&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (U16 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator U32 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U32&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (U32 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator U64 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U64&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (U64 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator S16 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U16&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (S16 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator S32 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U32&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (S32 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator S64 * (void)
	{
		if (((variantType & DAVT_TYPEMASK) == (DAVT_U64&DAVT_TYPEMASK)) && (variantType & DAVT_BYREF))
			return (S64 *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator SINGLE * (void)
	{
		if (variantType == DAVT_PSINGLE)
			return (SINGLE *) pVoid;
		return 0;
	}

	//---------------------
	//
	operator DOUBLE * (void)
	{
		if (variantType == DAVT_PDOUBLE)
			return (DOUBLE *) pVoid;
		return 0;
	}

	void coerce (DAVARENUM vartype)
	{
		if (vartype != DAVT_EMPTY)
		{
			if (vartype & DAVT_BYREF)
			switch (vartype & DAVT_TYPEMASK)
			{
				case DAVT_U8&DAVT_TYPEMASK:
					(*this) = (U8 *)(*this);
					break;
				case DAVT_U16&DAVT_TYPEMASK:
					(*this) = (U16 *)(*this);
					break;
				case DAVT_U32&DAVT_TYPEMASK:
					(*this) = (U32 *)(*this);
					break;
				case DAVT_U64&DAVT_TYPEMASK:
					(*this) = (U64 *)(*this);
					break;
				case DAVT_PRECT&DAVT_TYPEMASK:
					(*this) = (tagRECT *)(*this);
					break;
				case DAVT_PVECTOR&DAVT_TYPEMASK:
					(*this) = (class Vector *)(*this);
					break;
				case DAVT_PMATRIX&DAVT_TYPEMASK:
					(*this) = (class Matrix *)(*this);
					break;
				case DAVT_PTRANSFORM&DAVT_TYPEMASK:
					(*this) = (class Transform *)(*this);
					break;
				case DAVT_PPANE&DAVT_TYPEMASK:
					(*this) = (struct ViewRect *)(*this);
					break;
				case DAVT_PWINDOW&DAVT_TYPEMASK:
					(*this) = (struct _window *)(*this);
					break;
				case DAVT_PPOBJNODE&DAVT_TYPEMASK:
					(*this) = (struct OBJNODE **)(*this);
					break;
				case DAVT_PVARIANT&DAVT_TYPEMASK:
					(*this) = (DACOM_VARIANT *)(*this);
					break;
			}
			else
			if (vartype & DAVT_INTEGER)
			{
				(*this) = U64((*this));
			}
			else
			switch (vartype)
			{
				case DAVT_SINGLE:
					(*this) = SINGLE((*this));
					break;
				case DAVT_DOUBLE:
					(*this) = DOUBLE((*this));
					break;
				case DAVT_COMPONENT:
					(*this) = (struct IDAComponent *)(*this);
					break;
				case DAVT_DISPATCH:
					(*this) = (struct IDADispatch *)(*this);
					break;
				case DAVT_STRING:
					(*this) = (const C8 *)(*this);
					break;
				case DAVT_PVOID:
					(*this) = (void *)(*this);
					break;
			} // end switch on property type
		}
	}
};

#pragma pack ( pop )




#endif