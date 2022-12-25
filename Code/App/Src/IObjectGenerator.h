#ifndef IOBJECTGENERATOR_H
#define IOBJECTGENERATOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 IObjectGenerator.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IObjectGenerator.h 2     11/18/99 4:55p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IObjectGenerator : IObject
{
	virtual void SetGenerationFrequency(SINGLE _mean, SINGLE _minDiference) = 0;

	virtual void SetGenerationType(U32 _typeID) = 0;

	virtual void EnableGeneration(bool bEnable) = 0;

	virtual U32 ForceGeneration() = 0;
};

//-----------------------------------------------------------------------------------
//---------------------------END IHarvest.h------------------------------------------
//-----------------------------------------------------------------------------------
#endif