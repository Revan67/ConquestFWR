//--------------------------------------------------------------------------//
//                                                                          //
//                              MathLib..cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
  $Header: /Libs/Src/x86math/MathLib.cpp 4     5/20/98 4:16p Jasony $
*/
//--------------------------------------------------------------------------//
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "3DMath.h"




I3DMathEngine * __stdcall __MATH_ENGINE (void)
{
	DA3DMATHDESC math_info;
	I3DMathEngine * math_engine;

	DACOM_Acquire()->CreateInstance(&math_info, (void **) &math_engine);

	return math_engine;
}





//--------------------------------------------------------------------------//
//------------------------End MathLib.cpp-----------------------------------//
//--------------------------------------------------------------------------//
