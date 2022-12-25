#ifndef DLAUNCHER_H
#define DLAUNCHER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DLauncher.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DLauncher.h 5     12/13/99 10:12p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

//--------------------------------------------------------------------------//
//
struct RangeFinderSaveLoad
{
	SINGLE rangeError;
	SINGLE accuracy;
	U32 targetID;
};
//--------------------------------------------------------------------------//
//
struct BASE_LAUNCHER : BASIC_DATA
{
	LAUNCHCLASS type;

	char weaponType[GT_PATH];		// the launcher shoots this type object, can also be an effect file
	S32 supplyCost;
	SINGLE refirePeriod;

	U32 launcherSpecialID;//
};

#endif
