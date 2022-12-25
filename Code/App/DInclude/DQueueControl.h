#ifndef DQUEUECONTROL_H
#define DQUEUECONTROL_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DQueueControl.h                                    //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DQueueControl.h 1     3/24/00 8:25a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

//--------------------------------------------------------------------------//
//  Definition file for queue controls
//--------------------------------------------------------------------------//

struct GT_QUEUECONTROL : GENBASE_DATA
{
};
//---------------------------------------------------------------------------
//
struct QUEUECONTROL_DATA
{
    S32 xOrigin, yOrigin;
	S32 width, height;
};

#define QUEUECONTROL_TYPE "QueueControl!!Default"

#endif
