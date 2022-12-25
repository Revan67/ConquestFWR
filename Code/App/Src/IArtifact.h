#ifndef IARTIFACT_H
#define IARTIFACT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               IArtifact.h                                //
//                                                                          //
//                  COPYRIGHT (C) 2004 by WARTHOG, INC.                     //
//                                                                          //
//--------------------------------------------------------------------------//
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IArtifact : IObject 
{
	virtual void InitArtifact(struct IBaseObject * _owner) = 0;

	virtual void RemoveArtifact() = 0;

	virtual struct ArtifactButtonInfo * GetButtonInfo() = 0;

	virtual bool IsUsable() = 0;

	virtual bool IsToggle() = 0;

	virtual bool IsTargetArea() = 0;

	virtual bool IsTargetPlanet() = 0;

	virtual void SetTarget(IBaseObject * target) = 0;

	virtual U32 GetSyncDataSize() = 0;

	virtual U32 GetSyncData(void * buffer) = 0;

	virtual void PutSyncData(void * buffer, U32 bufferSize) = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IArtifactHolder : IObject
{
	virtual bool HasArtifact() = 0;

	virtual struct IBaseObject * GetArtifact() = 0;

	virtual void SetArtifact(const char * artifactName) = 0;

	virtual void DestroyArtifact() = 0;

	virtual void UseArtifactOn(IBaseObject * target, U32 agentID) =0;

	virtual SINGLE GetWeaponRange() = 0;
};

//----------------------------------------------------------------------------
//-----------------------END IArtifact.h--------------------------------------
//----------------------------------------------------------------------------
#endif
