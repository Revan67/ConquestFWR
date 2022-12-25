#ifndef IPARTICLECIRCLE_H
#define IPARTICLECIRCLE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IParticleCircle.h                           //
//                                                                          //
//                  COPYRIGHT (C) 2004 Warthog, INC.                        //
//                                                                          //
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

struct _NO_VTABLE IParticleCircle : IObject
{
	virtual void InitParticleCircle(IBaseObject * owner,SINGLE radius) = 0;

	virtual void SetActive(bool bSetting) = 0;
};


//---------------------------------------------------------------------------
//-------------------------END IParticleCircle.h-----------------------------
//---------------------------------------------------------------------------
#endif
