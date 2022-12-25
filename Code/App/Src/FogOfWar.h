#ifndef FOGOFWAR_H
#define FOGOFWAR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             FogOfWar.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
	/*
    $Header: /Conquest/App/Src/FogOfWar.h 18    5/07/01 9:22a Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef IOBJECT_H
#include "IObject.h"
#endif

typedef S32 INSTANCE_INDEX;     // defined by Engine.h

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IFogOfWar : IDAComponent
{
    virtual BOOL32 CheckVisiblePosition (const class Vector & pos) = 0;

	virtual BOOL32 CheckHardFogVisibility (U32 playerID,U32 systemID,const class Vector & pos, SINGLE radius=0) = 0;
    
	virtual BOOL32 CheckHardFogVisibilityForThisPlayer (U32 systemID,const class Vector & pos, SINGLE radius=0) = 0;

	virtual BOOL32 CheckVisiblePositionCloaked (const class Vector & pos) = 0;

    virtual void Update (void) = 0;

    virtual void Render (void) = 0;

	virtual void MapRender (int sys,int x,int y,int sizex,int sizey) = 0;

//	virtual void AddNebulae (struct Cloudzone *zones, U32 numZones) = 0;

	// object has already verified that its side == global side
	virtual void RevealZone (struct IBaseObject * obj, SINGLE radius, SINGLE cloakRadius) = 0;

	virtual void RevealBlackZone (U32 playerID,U32 systemID,const Vector &pos, SINGLE radius) = 0;

	virtual void ClearAllHardFog () = 0;

	virtual SINGLE GetPercentageFogCleared(U32 playerID,U32 systemID) = 0;

	virtual BOOL32 New();

	virtual	BOOL32 __stdcall Save (struct IFileSystem * outFile) = 0;

	virtual	BOOL32 __stdcall Load (struct IFileSystem * inFile) = 0;

	virtual void MakeMapTextures(int numSystems) = 0;

	virtual Vector FindHardFog (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints) = 0;

	virtual Vector FindHardFogFiltered (U32 playerID, U32 systemID, const class Vector & pos, Vector * avoidList, U32 numAvoidPoints, const class Vector & filterPos,SINGLE fiterRad) = 0;
};

//---------------------------------------------------------------------------
//-----------------------END FogOfWar.h--------------------------------------
//---------------------------------------------------------------------------
#endif
