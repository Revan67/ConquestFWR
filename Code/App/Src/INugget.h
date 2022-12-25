#ifndef INUGGET_H
#define INUGGET_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                INugget.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/INugget.h 24    10/02/00 8:56p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _NO_VTABLE INugget : IObject
{
	// lifeTime = 0 means infinite life
	// bRealized = true if user can see the nugget (explosion nuggets are created early)
	virtual void InitNugget(U32 partID, U32 _systemID, const Vector &_destPos,S32 scrapValue, SINGLE lifeTime, bool bRealized) = 0;

	virtual void SetDepleted (bool depSetting) = 0;

	virtual void IncHarvestCount () = 0;

	virtual void DecHarvestCount () = 0;

	virtual U8 GetHarvestCount () = 0;

	virtual void SetProcessID (U32 newProcID) = 0;

	virtual U32 GetProcessID () = 0;

	virtual enum M_RESOURCE_TYPE GetResourceType() = 0;

	virtual enum M_NUGGET_TYPE GetNuggetType() = 0;

	virtual U32 GetSupplies() = 0;

	virtual U32 GetMaxSupplies() = 0;

	virtual void SetSupplies(U32 newSupplies) =0;
};

struct _NO_VTABLE INuggetManager : IDAComponent
{
	virtual IBaseObject * FindNugget (U32 dwMissionID) = 0;

	virtual void FindNugget (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid) = 0;

	// if networked, nugget is initially invisible
	virtual void CreateNugget (PARCHETYPE pArchetype, U32 systemID, const Vector & position, U32 supplyValue, U32 lifeTime,
		U32 dwMissionID,bool network) = 0;

	// make nugget visible
	virtual void RealizeNugget (U32 nuggetID, const Vector & position, U32 systemID, SINGLE animDelay) = 0;
	
	virtual U32 GetSyncData (void * buffer) = 0;

	virtual void PutSyncData (void * buffer,U32 bufferSize) = 0;

	virtual void ReceiveNuggetData (void * buffer,U32 bufferSize) = 0;

	virtual void ReceiveNuggetDeath (U32 nuggetID, void * buffer, U32 bufferSize) = 0;

	virtual void Save (IFileSystem * outFile) = 0;

	virtual void Load (IFileSystem * inFile) = 0;

	virtual void Close() = 0;

	virtual void Update() = 0;

	virtual void Render() = 0;

	virtual void UpdateVisibilityFlags() = 0;

	virtual void TestVisible(const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer) = 0;

	virtual IBaseObject * TestVisibleHighlight(SINGLE & closest, const USER_DEFAULTS & defaults, const U32 currentSystem, const U32 currentPlayer, const RECT & selectionRect) = 0;

	virtual void MapRender(bool bPing) = 0;

	virtual void DeleteNugget(U32 dwMissionID) = 0;

	virtual void SendDeleteNugget(U32 dwMissionID, U32 deathOp) = 0;

	virtual IBaseObject * GetFirstNugget(U32 systemID) = 0;

	virtual IBaseObject * GetNextNugget(IBaseObject *nugget) = 0;

	virtual void RemoveNuggetsFromSystem(U32 systemID) = 0;

	virtual void MoveNuggetsToSystem(U32 oldSystemID, U32 newSystemID) = 0;

	virtual void CameraMove(U32 systemID) = 0;
};

//---------------------------------------------------------------------------
//--------------------------END INugget.h------------------------------------
//---------------------------------------------------------------------------
#endif
