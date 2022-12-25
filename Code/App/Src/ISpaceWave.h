#ifndef ISPACEWAVE_H
#define ISPACEWAVE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ISpaceWave.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog TX, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IBlast.h 12    4/20/00 1:34p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//typedef S32 INSTANCE_INDEX;

struct _NO_VTABLE ISpaceWave : IObject
{
	virtual void StartSpaceWave(Vector & start, Vector & end, SINGLE travelTime, U32 _systemID, U32 _playerID) = 0;
};

//---------------------------------------------------------------------------
//-------------------------END IBlast.h--------------------------------------
//---------------------------------------------------------------------------
#endif
