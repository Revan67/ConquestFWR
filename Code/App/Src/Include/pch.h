//--------------------------------------------------------------------------//
//                                                                          //
//                                 PCH.H                                    //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/Include/pch.h 9     9/27/00 11:43a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
// 4514: unused inline function
// 4201: nonstandard no-name structure use
// 4100: formal parameter was not used
// 4512: assignment operator could not be generated
// 4245: conversion from signed to unsigned
// 4127: constant condition expression
// 4355: 'this' used in member initializer
// 4244: conversion from int to unsigned char, possible loss of data
// 4200: zero sized struct member
// 4710: inline function not expanded
// 4702: unreachable code
// 4786: truncating function name (255 chars) in browser info
// 4711: function selected for inline expansion
#pragma warning (disable : 4514 4201 4100 4512 4245 4127 4355 4244 4710 4702 4786 4711)
#include <windows.h>
#pragma warning (disable : 4355 4201)


#include <typedefs.h>
#include <vfx.h>
#include <TComponent.h>
#include <Engine.h>
#include <GL\GL.h>
#include <3DMath.h>
#include "MyVertex.h"
#pragma warning (disable : 4355 4200 4201)
#ifdef FINAL_RELEASE
#pragma warning (disable : 4189)  // variable set but not used
#endif
// This file exists solely for the purpose of creating a pre-compiled header


//---------------------------------------------------------------------------
//--------------------------End Pch.h----------------------------------------
//---------------------------------------------------------------------------
