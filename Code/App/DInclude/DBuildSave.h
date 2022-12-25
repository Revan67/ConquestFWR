#ifndef DBUILDSAVE_H
#define DBUILDSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DBuildSave.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DBuildSave.h 15    9/19/00 2:06p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

#define MAX_FAB_WORKING 1
//--------------------------------------------------------------------------//

struct BUILD_SAVELOAD
{
	bool building:1;
	bool whole:1;
	bool pause:1;
	bool bReverseBuild:1;
	bool bDismantle:1;

	SINGLE timeSpent;
	U16 hullPointsAdded;
	U16 hullPointsFinish;
	U32 buildProcessID;
	S32 builderType;
};


#endif
