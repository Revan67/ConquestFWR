#ifndef IRECON_H
#define IRECON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 IRecon.h                               //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IRecon.h 5     9/18/00 5:36p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IReconLauncher : IObject
{
	virtual void KillProbe (U32 dwMissionID) = 0;
};

struct _NO_VTABLE IReconProbe : IObject
{
	virtual void InitRecon(IReconLauncher * _ownerLauncher, U32 _dwMissionID) = 0;

	virtual void ResolveRecon(IBaseObject * _ownerLauncher) = 0;

	virtual void LaunchProbe (IBaseObject * owner, const class TRANSFORM & orientation, const class Vector * pos,
		U32 targetSystemID, IBaseObject * jumpTarget) = 0;

	virtual void ExplodeProbe() = 0;

	virtual void DeleteProbe() = 0;

	virtual bool IsActive() = 0;

	virtual void ReconSwitchID(U32 newOwnerID) = 0;
};

//-----------------------------------------------------------------------------------
//---------------------------END IHarvest.h------------------------------------------
//-----------------------------------------------------------------------------------
#endif