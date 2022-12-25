#ifndef OBJLIST_H
#define OBJLIST_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ObjList.H                                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/Src/ObjList.h 51    10/23/00 10:54a Jasony $
*/			    
//-------------------------------------------------------------------
/*
	Management of the object list
*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#ifndef DACOM_H
#include <DACOM.h>
#endif

#ifndef OBJCLASS_H
#include "ObjClass.h"
#endif

struct IBaseObject;


#ifndef FINAL_RELEASE
#define OBJREFNAME   __FILE__
#else
#define OBJREFNAME   NULL
#endif
//--------------------------------------------------------------------------//
//

struct MISSION_INFO
{
	C8		partName[32];
};

enum UIANIMTYPE
{
	UI_MOVE,
	UI_BUILD,
	UI_RAM,
	UI_MOVE_ATTACK,
	UI_SPECIAL_ATTACK,
	UI_SELL,
	UI_JUMP,
	UI_ATTACK,
	UI_PATROL,
	UI_ADD_SHIP,
	UI_REMOVE_SHIP,
	UI_MOVE_WAYPOINT,


	UI_END		// this one is always the last value
};
//--------------------------------------------------------------------------//
//

struct DACOM_NO_VTABLE IObjectList : public IDAComponent
{
virtual	void __stdcall BeginFrame (void) = 0;

virtual	void __stdcall EndFrame (void) = 0;

virtual	void __stdcall Update (void) = 0;

virtual	void __stdcall Render (void) = 0;

virtual	BOOL32 __stdcall AddObject (IBaseObject *obj) = 0;

virtual	BOOL32 __stdcall AddPartID (IBaseObject *obj, U32 dwMissionID) = 0;	// remember to remove old ID first!!!

virtual	BOOL32 __stdcall RemoveObject (IBaseObject *obj) = 0;

virtual	BOOL32 __stdcall RemovePartID (IBaseObject *obj, U32 dwMissionID) = 0;

virtual	void __stdcall DeleteAllObjects (void) = 0;

virtual	IBaseObject * __stdcall GetObjectList (void) = 0;

virtual	IBaseObject * __stdcall GetTargetList (void) = 0;

virtual	IBaseObject * __stdcall GetHashList (U32 updateBin) = 0;
	
virtual	IBaseObject * __stdcall GetHighlightedList (void) = 0;

virtual	IBaseObject * __stdcall GetSelectedList (void) = 0;

virtual	void __stdcall UnselectObject (IBaseObject*) = 0;

virtual	void __stdcall FlushSelectedList (void) = 0;

virtual void __stdcall FlushHighlightedList (void) = 0;

virtual void __stdcall HighlightObject (IBaseObject *obj) = 0;

virtual void __stdcall SelectHighlightedObjects (U32 wParam=0) = 0;		// wParam is from the mouse message (used for admiral selection behvavior)

virtual	BOOL32 __stdcall Save (struct IFileSystem * outFile) = 0;

virtual	BOOL32 __stdcall Load (struct IFileSystem * inFile,bool bNoDynamics = false, bool bOnlyDynamics = false) = 0;

virtual	BOOL32 __stdcall QuickSave (struct IFileSystem * outFile, bool bDynamicsOnly = false) = 0;

virtual	void __stdcall ResolveAssociations (void) = 0;

virtual	BOOL32 __stdcall Close (void) = 0;

virtual	BOOL32 __stdcall New (void) = 0;

virtual	void __stdcall DEBUG_print (void) = 0;

virtual void __stdcall DEBUG_reloadDatabase (void) = 0;

virtual BOOL32 __stdcall DEBUG_isInUpdate (void) = 0;

virtual	IBaseObject * __stdcall FindObject (U32 dwMissionID, bool bAlwaysIfTaken = false) = 0;

virtual	BOOL32 __stdcall IsInList (const IBaseObject * obj) = 0;

virtual void __stdcall DrawHighlightedList (void);	// called during 2D render section

virtual IBaseObject * __stdcall FindGroupObject (const U32 *pObjIDs, U32 numObjects) = 0;

virtual	BOOL32 __stdcall AddGroupObject (IBaseObject *obj) = 0;

virtual	void __stdcall RemoveGroupObject (IBaseObject *obj) = 0;

// returns true if you revealed an enemy object!
virtual bool __stdcall CastVisibleArea (U32 playerID, U32 systemID, const Vector & pos, const struct FIELDFLAGS & fieldFlags, SINGLE radius, SINGLE cloakRadius) = 0;

// destroy a mission object at a later (safe) time
virtual void __stdcall DeferredDestruction (U32 dwMissionID) = 0;

// returns number of objects in array
virtual U32 __stdcall GetGroupMembers (U32 groupID, U32 objectIDs[MAX_SELECTED_UNITS]) = 0;

virtual void AddTimerTicks (enum CQTIMING timer, U64 ticks) = 0;

virtual void AddTimerTicksToLighting (U64 ticks) = 0;

virtual IBaseObject * __stdcall FindClosestAdmiral (IBaseObject * obj) = 0;

virtual GENRESULT GetUnitFont (struct IFontDrawAgent ** ppAgent) = 0;

virtual SINGLE GetRealRenderTime (void) = 0;

virtual	void * __stdcall FindObject (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid = IBaseObjectID) = 0;

virtual	struct IGroup * __stdcall FindGroupObject (U32 dwMissionID) = 0;

virtual	BOOL32 __stdcall AddGroupPartID (IGroup *obj, U32 dwMissionID) = 0;	// remember to remove old ID first!!!

virtual	BOOL32 __stdcall RemoveGroupPartID (IGroup *obj, U32 dwMissionID) = 0;

virtual void __stdcall BroadcastAllianceForPlayers (U32 playerID1, U32 playerID2) = 0;

virtual U32 __stdcall GetShipsToRender (void) = 0;

virtual void __stdcall IncrementShipsToRender (void) = 0;

virtual bool __stdcall CreateProjectile() = 0;

virtual void __stdcall ReleaseProjectile() = 0;

virtual void __stdcall DEBUG_IncLightCount (void);

virtual void __stdcall DEBUG_DecLightCount (void);

};

//----------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE IArchetypeList : public IDAComponent
{
	virtual	PARCHETYPE __stdcall GetArchetype (const C8 *name) = 0;

	virtual	PARCHETYPE __stdcall LoadArchetype (const C8 *name) = 0;

	virtual	BOOL32 __stdcall UnloadArchetype (const C8 *name) = 0;

	virtual	IBaseObject * __stdcall CreateInstance (PARCHETYPE pArchetype) = 0;

	virtual	IBaseObject * __stdcall CreateInstance (const char *name) = 0;

	virtual	const char * __stdcall GetArchName (PARCHETYPE pArchetype) = 0;

	virtual const char * __stdcall GetArchName (U32 dwArchetypeID) = 0;

	virtual	void * __stdcall GetArchetypeData (PARCHETYPE pArchetype) = 0;

	virtual	void * __stdcall GetArchetypeData (const C8 * name) = 0;

	// the following 4 methods deal with converting local pointers to network safe offsets...
	virtual U32 __stdcall GetArchetypeDataID (const C8 * name) = 0;

	virtual U32 __stdcall GetArchetypeDataID (PARCHETYPE pArchetype) = 0;

	virtual void * __stdcall GetArchetypeData (U32 dwArchetypeID) = 0;

	virtual PARCHETYPE __stdcall LoadArchetype (U32 dwArchetypeID) = 0;

	virtual void * __stdcall GetArchetypeHandle (PARCHETYPE) =0;

	virtual	BOOL32 __stdcall EnumerateArchetypeData (struct IArchetypeEnum * enumerator) = 0;

	virtual	void __stdcall AddRef (PARCHETYPE pArchetype, const char * szUser) = 0;		// add to the usage count

	virtual	void __stdcall Release (PARCHETYPE pArchetype, const char * szUser) = 0;		// decrement the usage count

	virtual IBaseObject * __stdcall CreateUIAnim (UIANIMTYPE type,const Vector & vec) = 0;
};

//----------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE IObjectFactory : public IDAComponent
{
	virtual	HANDLE __stdcall CreateArchetype (const char *szArchname, OBJCLASS objClass, void *data) = 0;

	virtual	BOOL32 __stdcall DestroyArchetype (HANDLE hArchetype) = 0;

	virtual	IBaseObject * __stdcall CreateInstance (HANDLE hArchetype) = 0;

	virtual	void __stdcall EditorCreateInstance (HANDLE hArchetype, const MISSION_INFO & info) = 0;
};

//----------------------------------------------------------------------------
//

struct IArchetypeEnum
{
	virtual	BOOL32 __stdcall ArchetypeEnum (const char * name, void *data, U32 size) = 0;
};


#endif
