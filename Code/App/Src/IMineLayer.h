#ifndef IMINELAYER_H
#define IMINELAYER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Minelayer.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IMineLayer.h 5     6/09/99 4:05p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IMinelayer : IObject 
{
	/*	-Move to position (get to good position, aim)
		-Start Mining (missionId = field Id we are working on, may need to create)
			Inc layer reference in mine field.
		-Stop Mining 
			percentDone = 0 to 1.0
			Dec Reference in mine field
	*/
	virtual void PrepareForMining(const Vector & pos, U32 agentID);
	virtual void StartMining(U32 missionID, U32 agentID);
	virtual void StopMining(SINGLE percentDone, U32 agentID);
};

//---------------------------------------------------------------------------
//-----------------------END Minelayer.h------------------------------------
//---------------------------------------------------------------------------
#endif
