#ifndef DEFFECTOPTS_H
#define DEFFECTOPTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              DEffectOpts.h								//
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DEffectOpts.h 16    8/23/01 9:12a Tmauer $
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#undef CQEXTERN
#ifdef _ADB
 #define CQEXTERN
#else
 #define __readonly
 #ifdef BUILD_GLOBALS
  #define CQEXTERN __declspec(dllexport)
 #else
  #define CQEXTERN __declspec(dllimport)
 #endif
#endif

#define CQEFFECTSOPTIONS_VERSION 5			// change this version number when making incompatible changes

//-------------------------------------------------------------------
//
struct GlobalEffectsOptions
{
	enum OPTVAL { off, on=-1 };
	__readonly U32 version;
	OPTVAL bWeaponTrails:1;
	OPTVAL bEmissiveTextures:1;
	OPTVAL bExpensiveTerrain:1;
	OPTVAL bTextures:1;
	OPTVAL bBackground:1;
	OPTVAL bHighBackground:1;
	OPTVAL bFastRender:1;
	OPTVAL bNoForceMinLOD:1;
	OPTVAL bNoForceMaxLOD:1;
	U32	nFlatShipScale:3;	
} CQEXTERN CQEFFECTS;

//this is used to looad the old setting without loss.
struct GlobalEffectsOptions_V3
{
	enum OPTVAL { off, on=-1 };
	__readonly U32 version;
	OPTVAL bWeaponTrails:1;
	OPTVAL bEmissiveTextures:1;
	OPTVAL bExpensiveTerrain:1;
	OPTVAL bTextures:1;
	OPTVAL bBackground:1;
	OPTVAL bFastRender:1;
	OPTVAL b3DShips:1;
	OPTVAL bNoForceMinLOD:1;
	OPTVAL bNoForceMaxLOD:1;
};

#undef CQEXTERN
//-------------------------------------------------------------------

#endif

