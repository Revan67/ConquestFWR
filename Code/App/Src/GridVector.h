#ifndef GRIDVECTOR_H
#define GRIDVECTOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             GridVector.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GridVector.h 26    9/09/00 1:07a Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#define GRIDSIZE 4096
#define HALFGRID 2048

#undef GRIDVECTOREXTERN
#ifdef BUILD_TRIM
#define GRIDVECTOREXTERN __declspec(dllexport)
#else
#define GRIDVECTOREXTERN __declspec(dllimport)
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

inline long __fastcall F2LONG (SINGLE s)
{
	long result;
	__asm  fld	dword ptr[s]
	__asm  fistp dword ptr[result]
	return result;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct GRIDVECTOR
{
	/*
	GRIDVECTOR (void)
	{
	}

	GRIDVECTOR (const Vector & vec)
	{
		*this = vec;
	}
	*/

	GRIDVECTOR & operator = (const Vector & vec)
	{
		x = ((F2LONG(vec.x) * 4)+((GRIDSIZE-1)/2)) / GRIDSIZE;
		y = ((F2LONG(vec.y) * 4)+((GRIDSIZE-1)/2)) / GRIDSIZE;
		return *this;
	}

	GRIDVECTOR & bigGridSquare (const Vector & vec)
	{
		U32 _x = U32(vec.x);
		U32 _y = U32(vec.y);

		x = (_x * 4) / GRIDSIZE;
		y = (_y * 4) / GRIDSIZE;

		x &= ~3;
		x |= 2;
		y &= ~3;
		y |= 2;

		return *this;
	}

	GRIDVECTOR & noEdges (const Vector & vec)
	{
		U32 _x = U32(vec.x);
		U32 _y = U32(vec.y);

		x = (_x * 4) / GRIDSIZE;
		y = (_y * 4) / GRIDSIZE;

		if (x%4==0)
			x |= 1;
		if (y%4==0)
			y |= 1;

		return *this;
	}

	GRIDVECTOR & init (SINGLE X, SINGLE Y)		// in terms of squares
	{
		x = X*4;		// leave these the same, round down expected
		y = Y*4;
		return *this;
	}

	GRIDVECTOR & quarterpos (void)
	{
		x |= 1;
		y |= 1;
		return *this;
	}

	GRIDVECTOR & centerpos (void)
	{
		x &= ~3;
		x |= 2;
		y &= ~3;
		y |= 2;
		return *this;
	}

	GRIDVECTOR & cornerpos (void)
	{
		x &= ~3;
		y &= ~3;
		return *this;
	}

	GRIDVECTOR & zero (void)
	{
		x = y = 0;
		return *this;
	}

	operator Vector (void) const
	{
		return Vector(x*(GRIDSIZE/4), y*(GRIDSIZE/4), 0);
	}

	SINGLE getX (void) const
	{
		return SINGLE(x)*0.25;
	}

	SINGLE getY (void) const
	{
		return SINGLE(y)*0.25;
	}

	int getIntX (void) const
	{
		return x >> 2;
	}

	int getIntY (void) const
	{
		return y >> 2;
	}

	GRIDVECTOREXTERN SINGLE operator - (const GRIDVECTOR & vec) const;

	bool operator == (const GRIDVECTOR & vec) const
	{
		return (x == vec.x && y == vec.y);
	}

	bool operator != (const GRIDVECTOR & vec) const
	{
		return (x != vec.x || y != vec.y);
	}

	bool isZero (void) const
	{
		return (x==0 && y==0);
	}

	bool isMostlyEqual (const GRIDVECTOR & vec) const
	{
		return (((x&~3)==(vec.x&~3)) && ((y&~3)==(vec.y&~3)));
	}

	static GRIDVECTOR Create (const Vector & vec)
	{
		GRIDVECTOR tmp;
		return (tmp = vec);
	}
protected:
	U8 x;
	U8 y;

	friend struct TerrainMap;
	friend struct NETGRIDVECTOR;
};
//---------------------------------------------------------------------------
//
struct NETGRIDVECTOR : GRIDVECTOR
{
	bool operator == (const NETGRIDVECTOR & vec) const
	{
		return (systemID == vec.systemID && x == vec.x && y == vec.y);
	}

	bool operator != (const NETGRIDVECTOR & vec) const
	{
		return (systemID != vec.systemID || x != vec.x || y != vec.y);
	}

	NETGRIDVECTOR & init (const Vector & vec, U32 _systemID)
	{
		x = ((F2LONG(vec.x) * 4)+((GRIDSIZE-1)/2)) / GRIDSIZE;
		y = ((F2LONG(vec.y) * 4)+((GRIDSIZE-1)/2)) / GRIDSIZE;

		systemID = _systemID;

		return *this;
	}
	
	NETGRIDVECTOR & init (const GRIDVECTOR & vec, U32 _systemID)
	{
		x = vec.x;
		y = vec.y;
		systemID = _systemID;

		return *this;
	}

	U8 systemID;
};
//---------------------------------------------------------------------------
//-----------------------END GridVector.h------------------------------------
//---------------------------------------------------------------------------
#endif
