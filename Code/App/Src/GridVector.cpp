//--------------------------------------------------------------------------//
//                                                                          //
//                             GridVector.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/GridVector.cpp 3     12/10/99 1:55p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "GridVector.h"
#include "CQTrace.h"

#define LTABLESIZE 0x1000
//--------------------------------------------------------------------------//
//
struct __gridvector_setup
{
	SINGLE table[LTABLESIZE];		// assumes 6 bits x + 6 bits y

	__gridvector_setup (void)
	{
		int i;
		for (i = 0; i < LTABLESIZE; i++)
		{
			int x, y;

			x = i >> 6;
			y = i & 0x3F;

			g_grid.table[i] = sqrt( ((SINGLE(x)*SINGLE(x)) + (SINGLE(y)*SINGLE(y))) /16);
		}
	}

} g_grid;
//--------------------------------------------------------------------------//
//
SINGLE GRIDVECTOR::operator - (const GRIDVECTOR & vec) const
{
	S32 _x, _y;
	
	if ((_x = vec.x - x) < 0)
		_x = -_x;
	if ((_y = vec.y - y) < 0)
		_y = -_y;

	U32 mask = (_x|_y) & ~0x3F;
	if (mask)
		return g_grid.table[((_x&0x3F)<<6)|(_y&0x3F)] + (mask/4);
	return g_grid.table[_x<<6|_y];
}
//---------------------------------------------------------------------------
//-----------------------END GridVector.cpp----------------------------------
//---------------------------------------------------------------------------
