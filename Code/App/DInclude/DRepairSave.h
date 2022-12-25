#ifndef DREPAIRSAVE_H
#define DREPAIRSAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                DRepairSave.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DRepairSave.h 1     9/03/99 6:12p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef DTYPES_H
#include "DTypes.h"
#endif

//--------------------------------------------------------------------------//

struct REPAIR_SAVELOAD
{
	U32 repairAtID;
	U32 repairAgentID;
	U8 repairMode;  //use REPAIRMODE enum
};


#endif
