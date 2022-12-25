#ifndef IGAMELIGHT_H
#define IGAMELIGHT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                IGameLight.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IGameLight.h 4     5/01/00 9:36a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//

struct _NO_VTABLE IGameLight : IObject
{
	virtual void SetColor (U8 red,U8 green,U8 blue) = 0;

	virtual void GetColor (U8 & red,U8 & green,U8 & blue) = 0;

	virtual void SetPosition (const Vector & pos, U32 newSystemID) = 0;

	virtual void SetName (const char * name) = 0;

	virtual void GetName (char * destName) = 0;

	virtual void SetRange (S32 range,bool bInfinite)= 0;

	virtual void GetRange (S32 &range,bool &bInfinite) = 0;

	virtual void SetDirection (Vector dir, bool bAmbient) = 0;

	virtual void GetDirection (Vector & dir, bool & bAmbient) = 0;

	virtual void SetSystemID (U32 newSystemID) = 0;
};

//---------------------------------------------------------------------------
//------------------------END IGameLight.h------------------------------------
//---------------------------------------------------------------------------
#endif
