#ifndef DICON_H
#define DICON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DIcon.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DIcon.h 4     9/13/00 2:23a Agarde $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

namespace ICONTXT
{
	enum ICON_TOOLTIP
	{
	NO_TOOLTIP			= 0,
	SYSTEM_INSUPPLY		= IDS_TT_INSUPPLY,
	SYSTEM_OUTSUPPLY	= IDS_TT_OUTSUPPLY
	};
};

//--------------------------------------------------------------------------//
//  Definition file for icon controls
//--------------------------------------------------------------------------//

struct GT_ICON : GENBASE_DATA
{
};
//---------------------------------------------------------------------------
//
struct ICON_DATA
{
	U32 baseImage;
    S32 xOrigin, yOrigin;
	ICONTXT::ICON_TOOLTIP tooltip;
};

#define ICON_TYPE "Icon!!Default"

#endif
