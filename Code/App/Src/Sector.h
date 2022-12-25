#ifndef SECTOR_H
#define SECTOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                Sector.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/Sector.h 38    9/22/00 2:49p Jasony $

   Sector map resource
*/
//--------------------------------------------------------------------------//

#ifndef IRESOURCE_H
#include "IResource.h"
#endif

#ifndef VFX_H
#include "vfx.h"
#endif

#define INVALID_SYSTEM_ID 0
#define HYPER_SYSTEM_ID   0
#define MAX_SYSTEMS 16
#define S_VISIBLE 0x00000001
#define S_LOCKED  0x00000002

struct IBaseObject;

struct SectorEffects
{
	bool bIntelegenceEffect:1;
	
	SINGLE getWeaponDamageMod()
	{
		if(bIntelegenceEffect)
			return 0.1;
		return 0.0;
	};
	
	SINGLE getWeaponRangeMod()
	{
		if(bIntelegenceEffect)
			return 0.1;
		return 0.0;
	};

	SINGLE getSensorMod()
	{
		if(bIntelegenceEffect)
			return 1.1;
		return 1.0;
	};

	SINGLE getDefenceMod()
	{
		if(bIntelegenceEffect)
			return 0.1;
		return 0.0;
	};

	SINGLE getSupplyMod()
	{
		if(bIntelegenceEffect)
			return 0.1;
		return 0.0;
	};

	SINGLE getSpeedMod()
	{
		if(bIntelegenceEffect)
			return 0.1;
		return 0.0;
	};

	void zero()
	{
		memset(this,0,sizeof(SectorEffects));
	}
};

//--------------------------------------------------------------------------//
//


struct DACOM_NO_VTABLE ISector : public IDAComponent
{
	virtual BOOL32 SetCurrentSystem (U32 SystemID) = 0;

	virtual U32 GetCurrentSystem (void) const = 0;

	virtual BOOL32 GetTerrainMap (U32 systemID, struct ITerrainMap ** map) = 0;

	virtual BOOL32 GetSystemRect (U32 SystemID, struct tagRECT * rect,bool bAbsolute=0) const = 0;

	//virtual BOOL32 SetSystemRect (U32 SystemID, struct tagRECT * rect) = 0;

	virtual BOOL32 GetCurrentRect (struct tagRECT * rect) const = 0;

	virtual void GetDefaultSystemSize (S32 &_sizeX,S32 &_sizeY) = 0;

	virtual BOOL32 GetSectorCenter (S32 *x, S32 *y) const = 0;

	virtual BOOL32 SetAlertState (U32 SystemID, U32 alertState, U32 playerID) = 0;

	virtual U32 GetAlertState (U32 SystemID, U32 playerID) const = 0;

	virtual S32 GetShortestPath (U32 startSystemID, U32 finishSystemID, U32 * SystemList, U32 playerID) const = 0;

	// returns true if path is possible
	virtual bool TestPath (U32 startSystemID, U32 finishSystemID, U32 playerID) const = 0;

	virtual bool IsVisibleToPlayer (U32 systemID, U32 playerID) const = 0;

	virtual BOOL32 Load (struct IFileSystem * inFile) = 0;

	virtual BOOL32 Save (struct IFileSystem * outFile) = 0;

	virtual BOOL32 QuickSave (struct IFileSystem * outFile) = 0;

	virtual BOOL32 Close (void) = 0;

	virtual BOOL32 New (void) = 0;

	virtual struct System *GetPokedSystem (S32 x,S32 y) = 0;

	virtual IBaseObject * GetJumpgateDestination (IBaseObject *jump) = 0;

	//playerID == 0 means all gates are visible
	virtual struct IBaseObject *GetJumpgateTo (U32 startSystemID,U32 destSystemID,const Vector &start_pos,U32 playerID=0);

	virtual struct IBaseObject *GetJumpgateTo (U32 startSystemID,U32 destSystemID,const Vector &start_pos,const Vector &dest_pos,U32 playerID=0);

	virtual void RegisterJumpgate (IBaseObject *obj,U32 jgateID) = 0;

	virtual void Update (void) = 0;

	virtual GENRESULT ZoneOn (U8 systemID);

	virtual void HighlightJumpgate (IBaseObject *obj) = 0;

	virtual void HighlightMoveSpot (const Vector &pos) = 0;

	virtual void ClearMoveSpot (void) = 0;

	virtual void RevealSystem (U32 systemID, U32 playerID) = 0; 

	virtual U32 CreateSystem (U32 xPos, U32 yPos, U32 width, U32 height) = 0;

	virtual void CreateJumpGate(U32 systemID1, U32 x1, U32 y1, U32 & id1,U32 systemID2, U32 x2, U32 y2, U32 & id2,char * archtype) =0;

	virtual void AddShipToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass) = 0;

	virtual void AddPlatformToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass) = 0;

	virtual void AddPlayerAttack (U32 systemID) = 0;;

	virtual void AddNovaBomb(U32 systemID) = 0;

	virtual int GetNumSystems() = 0;

	virtual BOOL32 ResolveJumpgates() = 0;

	virtual void DrawLinks(const PANE &tPane,const Transform &trans,SINGLE scale) = 0;

	virtual void SetLightingKit(U32 systemID, char * lightingKit) = 0;

	virtual void ViewSystemKit(U32 systemID) =0;

	virtual struct GT_SYSTEM_KIT GetSystemLightKit(U32 systemID) = 0;

	virtual void ComputeSupplyForPlayer(U32 playerID) = 0;

	virtual void ComputeSupplyForAllPlayers() = 0;

	virtual bool SystemInSupply(U32 systemID, U32 playerID) = 0;

	virtual bool SystemInRootSupply(U32 systemID, U32 playerID) = 0;

	virtual void RemoveLink(IBaseObject * gate1, IBaseObject * gate2) = 0;

	virtual void DeleteSystem(U32 systemID) = 0;

	virtual void ReassignSystemID(U32 oldSystemID, U32 newSystemID) = 0;

	virtual void GetSystemName(wchar_t * nameBuffer, U32 nameBufferrSize, U32 systemID) = 0;

	virtual void GetSystemNameChar (U32 systemID, char * nameBuffer, U32 bufferSize) = 0;

	virtual void SetSystemName(U32 systemID, U32 stringID) = 0;

	virtual void PrepareTexture (void) = 0;

	virtual SectorEffects * GetSectorEffects(U32 playerID, U32 systemID) = 0;
};

#define MAX_GATES 15


/*struct Jump;
struct GateLink;
struct System;*/

/*struct JUMP_DATA
{
	U32 id;
	S32 x,y;
	U32 syst_id;
};

struct GATELINK_DATA
{
//	U32 id;
	U32 end_id1,end_id2;
	U32 drawState;

};

struct SYSTEM_DATA
{
	U32 id;
	char name[256];
	S32 x,y;
	S32 sizeX,sizeY;
	U32 ownershipState,alertState,icon;
};
  */


//--------------------------------------------------------------------------//
//------------------------------End Sector.h--------------------------------//
//--------------------------------------------------------------------------//
#endif