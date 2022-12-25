#ifndef IEXPLOSION_H
#define IEXPLOSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                            IExplosion.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IExplosion.h 22    10/20/00 9:09p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef MESH_H
#include "Mesh.h"
#endif

#ifndef HEAPOBJ_H
#include <HeapObj.h>
#endif

#ifndef ILIGHT_H
#include "ILight.h"
#endif

typedef S32 INSTANCE_INDEX;

#define MAX_CHILDS 20
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IExplosionOwner : IObject
{
	virtual void RotateShip (SINGLE relYaw, SINGLE relRoll, SINGLE relPitch, SINGLE relAltitude) = 0;

	virtual void OnChildDeath (INSTANCE_INDEX child) = 0;

	virtual U32 GetScrapValue (void) = 0;

	virtual U32 GetFirstNuggetID (void) = 0;

	virtual void SetColors (void) = 0;
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IExplosion : IObject 
{
	virtual BOOL32 InitExplosion (IBaseObject * owner,U32 sideID,U16 sensorRadius, BOOL32 _minimal = 0,
		BOOL32 _notOnObjlist = 0) = 0;

	virtual BOOL32 ShouldRenderParent (void) = 0;

	static void CreateDebrisNuggets (IBaseObject * owner);

	static void RealizeDebrisNuggets (IBaseObject * owner, SINGLE delay = 0);
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IDebris : IObject
{
	virtual void RegisterChild (INSTANCE_INDEX _child);
};
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE IExtent : IObject
{
	virtual void GetExtentInfo (const RECT *&extents,SINGLE *z_step,SINGLE *z_min,U8 *slices) = 0;

	virtual void SetAliasData (U32 _aliasArch,U8 aliasPlayer) = 0;

	virtual void GetAliasData (U32 & _aliasArch,U8 & aliasPlayer) = 0;

	virtual void CalculateRect (bool bEnableMimic) = 0;  //mimic will use this to maintain visuals

	virtual const RECT *GetExtentRect (SINGLE z_val) = 0; 

	virtual void AddChildBlast (IBaseObject *blast) = 0;
};
//---------------------------------------------------------------------------
//-----------------------END IExplosion.h------------------------------------
//---------------------------------------------------------------------------
#endif
