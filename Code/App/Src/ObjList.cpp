//--------------------------------------------------------------------------//
//                                                                          //
//                               ObjList.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/Src/ObjList.cpp 373   10/27/00 2:45p Jasony $
*/			    
//---------------------------------------------------------------------------
#include "pch.h"
#include <globals.h>

#include "TResClient.h"
#include "IObject.h"
#include "Objlist.h"
#include "Objwatch.h"
#include "ObjClass.h"
#include "IResource.h"
#include "Resource.h"
#include "UserDefaults.h"
#include "Menu.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "Mission.h"
#include "DBHotkeys.h"
#include "Hotkeys.h"
#include "EventPriority.h"
#include "NetPacket.h"
#include "Startup.h"
#include "MGlobals.h"
#include <DBaseData.h>
#include <DMBaseData.h>
#include "FogOfWar.h"
#include "Sector.h"
#include "BaseHotRect.h"
#include "DrawAgent.h"
#include "IBlast.h"
#include "IGroup.h"
#include "Field.h"
#include "OpAgent.h"
#include "UserDefaults.h"
#include "IAdmiral.h"
#include "GenData.h"
#include "IUnbornMeshList.h"
#include "IFabricator.h"
#include "IDust.h"
#include "ILauncher.h"
#include "SFX.h"
#include "IPLANET.h"
#include "EventScheduler.h"
#include "EffectPlayer.h"
#include <IParticleMAnager.h>
//#include <IRenderDebugger.h>

//for timing only
#include "SysMap.h"
#include "MPart.h"
#include <DEffectOpts.h>
#include "IMissionActor.h"
#include "CQBatch.h"
#include "IMovieCamera.h"
#include "TerrainMap.h"
#include "GridVector.h"
#include "MeshRender.h"
#include "ObjMap.h"
#include "ObjMapIterator.h"
#include "INugget.h"
#include <DSector.h>
#include "CQLight.h"
#include "THashList.h"
#include "IJumpPlat.h"
#include "IAttack.h"

#include <lightman.h>
#include <HKEvent.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IConnection.h>
#include <Engine.h> 
#include <VFX.h>
#include <3DMath.h>
#include <FileSys.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <WindowManager.h>
#include <Renderer.h>
#include <IAnim.h>
#include <Pixel.h>
#include <IRenderPrimitive.h>

#include <CommCtrl.h>
#include <stdlib.h>
#include <stdio.h>

//MeshRender.cpp
void UpdateRenderEffects(SINGLE dt);

#define OBJLIST_PRIORITY      (RES_PRIORITY_MEDIUM-10)
//#define OBJLIST_HIGH_PRIORITY (RES_PRIORITY_MEDIUM+20)

#pragma warning (disable : 4200)	// zero sized array

#define OPPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#define OPPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2)
#define OPPRINT3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3)
#define OPPRINT4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4)

//Flagship.cpp
void clearFleetGroupList();
void clearFleetMemberList();


extern "C"
__declspec(dllimport) IHeap * __stdcall GetBatchHeap (void);
//-----------------------------------------------------------------------------------
//
static char *timingNames[TIMING_END] = {
	"Flush",
	"Render2D",
	"Background",
	"Swap",
	"Messaging",
	"Update2D",
	"ClearBuffers",
	"PlayerAI",
	"OpAgent",
 };

//-----------------------------------------------------------------------------------
//
template <int Size> 
struct REPORTFONT : IDebugFontDrawAgent
{
	char * buffer;
	int index;

	REPORTFONT (void)
	{
		buffer = (char *) VirtualAlloc(0, Size, MEM_COMMIT, PAGE_READWRITE);
		index = 0;
	}

	~REPORTFONT (void)
	{
		VirtualFree(buffer, 0, MEM_RELEASE);
		buffer = 0;
	}

	REPORTFONT & operator = (const REPORTFONT<Size> & text)
	{
		memcpy(buffer, text.buffer, Size);
		index = text.index;
		return *this;
	}

	/* IDAComponent methods */
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		*instance = 0;
		return GR_GENERIC;
	}
	
	DEFMETHOD_(U32,AddRef) (void)
	{
		return 2;
	}
	
	DEFMETHOD_(U32,Release) (void)
	{
		return 1;
	}

	/* IDebugFontDrawAgent methods */

	virtual U32 __stdcall GetFontHeight (void) const
	{
		return DEBUGFONT->GetFontHeight();
	}

	virtual U32 __stdcall GetStringWidth (const char *string) const
	{
		CQBOMB0("Not implemented");
		return 0;
	}

	virtual void __stdcall StringDraw (PANE *pane, S32 x, S32 y, const char *string, COLORREF color)
	{	
		addText(string);
		addText("\r\n", 2);
	}

	/* REPORTFONT methods */
	
	void addText (const char * pText, int len)
	{
		const int imax = Size - index - 1;
		len = __min(imax, len);

		memcpy(buffer+index, pText, len);
		index += len;
	}

	void addText (const char * pText)
	{
		addText(pText, strlen(pText));
	}
};
//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------
enum CURSOR_MODE
{
	NOT_OWNED=0,
	LASSO,
	SELECT,
	GUIMENU
};

//--------------------------------------------------------------------------//
//
struct ARCHDATATYPE
{
	C8 name[32];
	BASIC_DATA * objData;
	U32 dataSize;		// size of data chunk in bytes
	bool editDropable:1;	// if true, add to insertionlist
};

//--------------------------------------------------------------------------//
//
struct ARCHDATA
{
	U32 numArchetypes;
	ARCHDATATYPE type[];
};
//--------------------------------------------------------------------------//
//
struct ARCHNODE 
{
	struct ARCHNODE * prev, * next;
	U32 usage;
	ARCHDATATYPE * archDataType;
	COMPTR<IObjectFactory> factory;
	HANDLE hArchetype;

#ifndef FINAL_RELEASE
	
	struct NAMENODE
	{
		NAMENODE * pNext;
		const char * szName;
	} * nameList;

	~ARCHNODE (void)
	{
		reset();
	}

	void reset (void)		// flush the names list
	{
		NAMENODE * node = nameList;

		while (node)
		{
			nameList = node->pNext;
			delete node;
			node = nameList;
		}
	}

	void addName (const char * name)
	{
		if (name)
		{
			NAMENODE * node = new NAMENODE;
			node->pNext = nameList;
			nameList = node;
			node->szName = name;
		}
	}

	void removeName (const char * name)
	{
		if (name)
		{
			NAMENODE * node = nameList, *prev=0;

			while (node)
			{
	//			if (strcmp(name, node->szName) == 0)
				if (name == node->szName)
				{
					if (prev)
						prev->pNext = node->pNext;
					else
						nameList = node->pNext;
					delete node;
					break;
				}

				prev = node;
				node = node->pNext;
			}
		}
	}

#endif

	ARCHNODE (void)
	{
		memset(this, 0, sizeof(*this));
	}

private:
	// not allowed
	ARCHNODE & operator = (const ARCHNODE & obj)
	{
		return *this;
	}
};
//--------------------------------------------------------------------------//
//
struct TERMINATION_NODE
{
	struct TERMINATION_NODE * pNext;
	U32 victimID;
};
//--------------------------------------------------------------------------//
//
struct ACTOR
{
	struct ACTOR * pNext;
	
	static U32 hash (U32 object)
	{
		return ((object>>4) & 0xff);
	}

	U32 hash (void)
	{
		return hash(dwMissionID);
	}
	
	bool compare (U32 object)
	{
		return (object == dwMissionID);
	}
	
	void print (void)
	{
	}
	
	ACTOR (U32 _dwMissionID)
	{
		actor = 0;
		pNext = 0;
		dwMissionID = _dwMissionID;
	}
	
	//
	// User data
	//
	IBaseObject * actor; 
	U32 dwMissionID;
};
//--------------------------------------------------------------------------//
//
struct GROUPACTOR
{
	struct GROUPACTOR * pNext;
	
	static U32 hash (U32 object)
	{
		return ((object>>4) & 0xff);
	}

	U32 hash (void)
	{
		return hash(dwMissionID);
	}
	
	bool compare (U32 object)
	{
		return (object == dwMissionID);
	}
	
	void print (void)
	{
	}
	
	GROUPACTOR (U32 _dwMissionID)
	{
		actor = 0;
		pNext = 0;
		dwMissionID = _dwMissionID;
	}
	
	//
	// User data
	//
	IGroup * actor; 
	U32 dwMissionID;
};
//--------------------------------------------------------------------------//
//
#define TIMING_HISTORY 32
struct TimingStruct
{
	int currentFrame;

	U32 sector[TIMING_HISTORY];
	U32 mission[TIMING_HISTORY];
	U32 engine[TIMING_HISTORY];
	U32 deferredTerm[TIMING_HISTORY];
	U32 render[TIMING_HISTORY];
	U32 fogupdate[TIMING_HISTORY];
	U32 lighting[TIMING_HISTORY];
	U32 visibility[TIMING_HISTORY];
	U32 player_visibility[TIMING_HISTORY];
	U32 nugget_update[TIMING_HISTORY];
	U32 fogrender[TIMING_HISTORY];
	U32 map[TIMING_HISTORY];
//	U32 draw_prim[TIMING_HISTORY];
//	U32 flush[TIMING_HISTORY];
	U32 timer_array[TIMING_END][TIMING_HISTORY];
	//  local update
	U32 local[TIMING_HISTORY];
	U32 group[TIMING_HISTORY];
	U32 harvest[TIMING_HISTORY];
	U32 gunboat[TIMING_HISTORY];
	U32 weapon[TIMING_HISTORY];
	U32 total[TIMING_HISTORY];
	U32 update_total[TIMING_HISTORY];
	U32 render_flush[TIMING_HISTORY];

	void update (void)
	{
		currentFrame = (currentFrame + 1) % TIMING_HISTORY;
		sector[currentFrame] =
		mission[currentFrame] =
		local[currentFrame] =
		engine[currentFrame] =
		deferredTerm[currentFrame] = 
		render[currentFrame] = 
		fogupdate[currentFrame] =
		lighting[currentFrame] = 
		visibility[currentFrame] =
		player_visibility[currentFrame] = 
		nugget_update[currentFrame] =
		fogrender[currentFrame] = 
		map[currentFrame] = 
		group[currentFrame] = 
		harvest[currentFrame] = 
		gunboat[currentFrame] =
		weapon[currentFrame] = 
		total[currentFrame] = 
		render_flush[currentFrame] = 
		update_total[currentFrame] = 0;
		
		int i = TIMING_END;
		do
		{
			timer_array[i][currentFrame] = 0;
		} while (--i >= 0);
	}
	
	U32 do_average (U32 var[TIMING_HISTORY])
	{
		U64 result = var[0];
		
		for (int i=1; i < TIMING_HISTORY; i++)
		{
			result += var[i];
		}
		return U32(result / TIMING_HISTORY);
	}
	U32 do_sum (U32 var[TIMING_HISTORY])
	{
		U32 result = var[0];
		
		for (int i=1; i < TIMING_HISTORY; i++)
		{
			result += var[i];
		}
		return result;
	}
	void add_to_sector (U64 value)
	{
		sector[currentFrame] += (U32) value;
	}
	void add_to_mission (U64 value)
	{
		mission[currentFrame] += (U32) value;
	}
	void add_to_local (U64 value)
	{
		local[currentFrame] += (U32) value;
	}
	void add_to_group (U64 value)
	{
		group[currentFrame] += (U32) value;
	}
	void add_to_harvest (U64 value)
	{
		harvest[currentFrame] += (U32) value;
	}
	void add_to_gunboat (U64 value)
	{
		gunboat[currentFrame] += (U32) value;
	}
	void add_to_weapon (U64 value)
	{
		weapon[currentFrame] += (U32) value;
	}
	void add_to_engine (U64 value)
	{
		engine[currentFrame] += (U32) value;
	}
	void add_to_destruction (U64 value)
	{
		deferredTerm[currentFrame] += (U32) value;
	}
	void add_to_render (U64 value)
	{
		render[currentFrame] += (U32) value;
	}
	void add_to_fogupdate (U64 value)
	{
		fogupdate[currentFrame] += (U32) value;
	}
	void add_to_lighting (U64 value)
	{
		lighting[currentFrame] += (U32) value;
	}
	void add_to_visibility (U64 value)
	{
		visibility[currentFrame] += (U32) value;
	}
	void add_to_player_visibility (U64 value)
	{
		player_visibility[currentFrame] += (U32) value;
	}
	void add_to_nugget_update (U64 value)
	{
		nugget_update[currentFrame] += (U32) value;
	}
	void add_to_fogrender (U64 value)
	{
		fogrender[currentFrame] += (U32) value;
	}
	void add_to_map (U64 value)
	{
		map[currentFrame] += (U32) value;
	}
	void add_to_flush (U64 value)
	{
		render_flush[currentFrame] += (U32) value;
	}
	/*
	void add_to_draw_prim (U64 value)
	{
		draw_prim[currentFrame] += (U32) value;
	}*/
	void add_to_timer_array(int timer,U64 value)
	{
		timer_array[timer][currentFrame] += U32(value);
	}
	void add_to_total (U64 value)
	{
		total[currentFrame] += (U32) value;
	}
	void add_to_update_total (U64 value)
	{
		update_total[currentFrame] += (U32) value;
	}
};
//--------------------------------------------------------------------------//
//------------------------------ObjectList Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ObjectList : public IObjectList, 
										   IEventCallback,
										   IArchetypeList,
										   ResourceClient<>,
										   ConnectionPointContainer<ObjectList>
{
	U32 eventHandle;		// handles to callback
	IBaseObject *objectList;
	IBaseObject *hashList[32];
	IBaseObject *targetList;
	IBaseObject *highlightedList;
	IBaseObject *selectedList;
//	IBaseObject *fighterList;		// special list of fighters (for target acquisition)
	IBaseObject *groupList, *groupListEnd;
	RECT lassoRect, selectionRect;
	bool bHasFocus, bGrabbedFocus, bInLocalUpdate, bInsideClose;

	ARCHNODE *archList;							// pointer to first ARCHNODE instance in archetype list
	ARCHDATA *archData;							// pointer to loaded archetype database info

	HashList<ACTOR, U32, 256> actorList;
	HashList<GROUPACTOR, U32, 256> groupActorList;

	//
	// frame timing stuff
	//
	unsigned __int64 lastClockTick;
	DOUBLE  clockPeriod, updateTimeRemainder, fps;
	SINGLE realRenderTime;		// according to real frame rate -- not game frame rate
	SINGLE gameTime;//time from the start of the game in seconds, note that this should not be used by game logic. for visuals only.
	U32 numRealTimeFrames;
	unsigned __int64 waitUntilTick;
	DOUBLE timeWaited;		// time spent in tight loop to limit frame rate
	DOUBLE elapsedTime;		// total time of last frame (including wasted time)
	unsigned __int64 ticksForBaseFrameRate;	 // # of high performance ticks for 1/30
	U32 frameCount;	// number of real-time frames since program start
	bool bTimeTraceThisSecond;
	int subParFrames;		// number of frames where we have had below-spec frame rate
	bool bWarningIssued;
	bool bWasQuickLoad;
	U32 objectsOnScreen, totalObjects;
	U32 numProjectiles;
	S32 numLights;
	//
	//
	//
	U32 creationCounter;		// increment everytime an object is created

	TimingStruct timing;
	//
	// editor cursor modes
	//
	
	CURSOR_MODE cursorMode;

	//
	// animation type for user input animations (ObjComm needs this)
	//
	PARCHETYPE pAnimType[UI_END];
	
	//
	// dialog box data
	//
	HWND hDlgBox;
	BOOL32 bEditorMode;
	U32  dlgArchID;

	//
	// viewer data
	//
	S32 viewerCount;

	// deferred destruction list
	TERMINATION_NODE * pHitList;

	// Agent needed for unit text
	COMPTR<IFontDrawAgent> unitFont;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ObjectList)
	DACOM_INTERFACE_ENTRY(IObjectList)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IArchetypeList)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	BEGIN_DACOM_MAP_OUTBOUND(ObjectList)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IEventCallback", point)
	DACOM_INTERFACE_ENTRY_AGGREGATE("IObjectFactory", point2)
	END_DACOM_MAP()

	ConnectionPoint<ObjectList,IEventCallback> point;
	ConnectionPoint<ObjectList,IObjectFactory> point2;

	ObjectList (void);

	~ObjectList (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IObjectList methods */

	DEFMETHOD_(void,BeginFrame) (void);

	DEFMETHOD_(void,EndFrame) (void);

	DEFMETHOD_(void,Update) (void);

	DEFMETHOD_(void,Render) (void);

	DEFMETHOD_(BOOL32,AddObject) (IBaseObject *obj);

	DEFMETHOD_(BOOL32,AddPartID) (IBaseObject *obj, U32 dwMissionID);

	DEFMETHOD_(BOOL32,RemoveObject) (IBaseObject *obj);

	DEFMETHOD_(BOOL32,RemovePartID) (IBaseObject *obj, U32 dwMissionID);

	DEFMETHOD_(void,DeleteAllObjects) (void);

	DEFMETHOD_(void,UnselectObject) (IBaseObject*);
	
//	DEFMETHOD_(void,SetFighterList) (IBaseObject * obj);	  // set the first fighter in the list

	DEFMETHOD_(IBaseObject *,GetObjectList) (void);

	DEFMETHOD_(IBaseObject *,GetTargetList) (void);

	virtual	IBaseObject * __stdcall GetHashList (U32 updateBin);

	DEFMETHOD_(IBaseObject *,GetHighlightedList) (void);

	DEFMETHOD_(IBaseObject *,GetSelectedList) (void);

	DEFMETHOD_(void,FlushSelectedList) (void);

	DEFMETHOD_(void,FlushHighlightedList) (void);

	DEFMETHOD_(void,HighlightObject) (IBaseObject *obj);

	DEFMETHOD_(void,SelectHighlightedObjects) (U32 wParam);

	DEFMETHOD_(BOOL32,Save) (struct IFileSystem * outFile);

	DEFMETHOD_(BOOL32,Load) (struct IFileSystem * inFile,bool bNoDynamics = false, bool bOnlyDynamics = false);

	DEFMETHOD_(BOOL32,QuickSave) (struct IFileSystem * outFile, bool bDynamicsOnly = false);

	DEFMETHOD_(void,ResolveAssociations) (void);

	DEFMETHOD_(BOOL32,Close) (void);

	DEFMETHOD_(BOOL32,New) (void);

	DEFMETHOD_(void,DEBUG_print) (void);

	virtual void __stdcall DEBUG_reloadDatabase (void);

	virtual BOOL32 __stdcall DEBUG_isInUpdate (void)
	{
		return bInLocalUpdate;
	}

	DEFMETHOD_(IBaseObject *,FindObject) (U32 dwMissionID, bool bAlwaysIfTaken = false);

	virtual	void * __stdcall FindObject (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid );

	DEFMETHOD_(BOOL32,IsInList) (const IBaseObject * obj);

	virtual void __stdcall DrawHighlightedList (void);

	virtual IBaseObject * __stdcall FindGroupObject (const U32 *pObjIDs, U32 numObjects);

	virtual	BOOL32 __stdcall AddGroupObject (IBaseObject *obj);

	virtual	void __stdcall RemoveGroupObject (IBaseObject *obj);

	virtual bool __stdcall CastVisibleArea (U32 playerID, U32 systemID, const Vector & pos, const FIELDFLAGS & fieldFlags, SINGLE radius, SINGLE cloakRadius);

	virtual void __stdcall DeferredDestruction (U32 dwMissionID);

	virtual U32 __stdcall GetGroupMembers (U32 groupID, U32 objectIDs[MAX_SELECTED_UNITS]);

	virtual IBaseObject * __stdcall FindClosestAdmiral (IBaseObject * obj);

	virtual GENRESULT GetUnitFont (struct IFontDrawAgent ** ppAgent);

	virtual SINGLE GetRealRenderTime (void);

	virtual	struct IGroup * __stdcall FindGroupObject (U32 dwMissionID);

	virtual	BOOL32 __stdcall AddGroupPartID (IGroup *obj, U32 dwMissionID);	// remember to remove old ID first!!!

	virtual	BOOL32 __stdcall RemoveGroupPartID (IGroup *obj, U32 dwMissionID);

	virtual void __stdcall BroadcastAllianceForPlayers (U32 playerID1, U32 playerID2);

	virtual U32 __stdcall GetShipsToRender (void);

	virtual void __stdcall IncrementShipsToRender (void);

	virtual bool __stdcall CreateProjectile();

	virtual void __stdcall ReleaseProjectile();

	virtual void __stdcall DEBUG_IncLightCount (void);

	virtual void __stdcall DEBUG_DecLightCount (void);

	/* IArchetypeList methods */

	DEFMETHOD_(PARCHETYPE,GetArchetype) (const C8 *name);

	DEFMETHOD_(PARCHETYPE,LoadArchetype) (const C8 *name);

	DEFMETHOD_(BOOL32,UnloadArchetype) (const C8 *name);

	DEFMETHOD_(IBaseObject *,CreateInstance) (PARCHETYPE pArchetype);

	DEFMETHOD_(IBaseObject *,CreateInstance) (const char *name);

	DEFMETHOD_(const char *,GetArchName) (PARCHETYPE pArchetype);

	virtual const char *  __stdcall GetArchName (U32 dwArchetypeID);

	DEFMETHOD_(void *,GetArchetypeData) (PARCHETYPE pArchetype);

	DEFMETHOD_(void *,GetArchetypeData) (const C8 * name);

	virtual U32 __stdcall GetArchetypeDataID (const C8 * name);

	virtual U32 __stdcall GetArchetypeDataID (PARCHETYPE pArchetype);

	virtual void * __stdcall GetArchetypeData (U32 dwArchetypeID);

	virtual PARCHETYPE __stdcall LoadArchetype (U32 dwArchetypeID);

	DEFMETHOD_(void *,GetArchetypeHandle) (PARCHETYPE);

	DEFMETHOD_(BOOL32,EnumerateArchetypeData) (struct IArchetypeEnum * enumerator);

	DEFMETHOD_(void,AddRef) (PARCHETYPE pArchetype, const char * szUser);		// add to the usage count

	DEFMETHOD_(void,Release) (PARCHETYPE pArchetype, const char * szUser);		// decrement the usage count

	virtual IBaseObject * __stdcall CreateUIAnim (UIANIMTYPE type, const Vector & vec);	// create an instance of the UI animation (for ObjComm.cpp)

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* ObjectList methods */

	void renderGridLines (void);

	void removeObject (IBaseObject *obj);

	BOOL CALLBACK loadArchDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static BOOL CALLBACK _loadArchDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	void flushUnusedArchetypes (void);

	void unloadArchetype (ARCHNODE * node);

	U32 getNumArchetypes (void);

	void setCursorMode (CURSOR_MODE newMode);

	BOOL32 loadTypesData (void);

	ARCHDATATYPE * getArchDataType (const C8 * name);

	void initializeArchListbox (HWND hListbox);

	void getLassoRect (RECT & rect) const;

	void local_update (void);
	void local_group_update (void);

	void engine_update (SINGLE dt);

	void drawSelectedList (void);

	void unselectObjects (void);

	void unhighlightObject (IBaseObject*);

	BOOL32 createPart (U32 archID, const char *partName);

	void setTextForSelection (HWND hwnd);

	static int getNumSelected (IBaseObject * selected);

	static void clearHighlightBits (IBaseObject * highlightList);

	void pruneHighlightList (void);		// remove objects from highlight list that don't have bit set

	void updatePlayerVisibility (void);

	void processDeferredTermination (void);

	static int testHighlightCount (IBaseObject * highlighted, U32 playerID);

	void notifyHostMigration (void);

	void highlightFleetships (void);

	void AddTimerTicks (enum CQTIMING timer, U64 ticks)
	{
		if (timer < TIMING_END)
			timing.add_to_timer_array(timer,ticks);
	}

	void AddTimerTicksToLighting (U64 ticks)
	{
		timing.add_to_lighting(ticks);
	}

	void createUnitFont (void);

	void deferredDestruction (U32 dwMissionID);

	void DEBUG_forceFlush (void);

#ifndef FINAL_RELEASE
	void issuePerformanceWarning (bool bWarning);
#endif

	void debug_print (IDebugFontDrawAgent * DEBUGFONT, bool bDetailedReport);
		
	void doPerformanceTrace (U32 updateCounter);
};
//--------------------------------------------------------------------------//
//
ObjectList::ObjectList (void) : point(0), point2(1)
{
	bHasFocus = 1;
	resPriority = OBJLIST_PRIORITY;
	numProjectiles = 0;
}
//--------------------------------------------------------------------------//
//
ObjectList::~ObjectList (void)
{
	COMPTR<IDAConnectionPoint> connection;

	DeleteAllObjects();
	flushUnusedArchetypes();
	releaseResources();

//	ENGINE->destroy_instance(spotlight_index);

	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	// destroy the object list, archetype list ?

	free(archData);
	archData = 0;

	SendMessage(hDlgBox, WM_CLOSE, 0, 0);
	hDlgBox = 0;
}
//--------------------------------------------------------------------------//
// sets 'numRealTimeFrames' 
//	numRealTimeFrames = 1;
//
void ObjectList::BeginFrame (void)
{
	__int64 clockTick, slamTicks;

	timing.update();

	QueryPerformanceCounter((LARGE_INTEGER *)&clockTick);

	if (lastClockTick == 0)
		lastClockTick = clockTick;

	timing.add_to_total(clockTick-lastClockTick);

	elapsedTime = DOUBLE(int(clockTick - lastClockTick)) * clockPeriod;

	if (elapsedTime < 0 || elapsedTime > 0.25)
		elapsedTime = 0.25;
	realRenderTime = elapsedTime;

	UpdateRenderEffects(realRenderTime);

	fps = (elapsedTime) ? 1.0 / elapsedTime : RENDER_PERIOD;		// for lod

	if (floor(fps) <= 4)
	{
		if (++subParFrames > 4 * 5)		// 5 seconds at or below 4 fps
		{
			if (bWarningIssued == 0)
			{
				bWarningIssued = true;
#ifndef FINAL_RELEASE
				if (PLAYERID)		// only issue warning in multiplayer
					issuePerformanceWarning(true);
#endif
			}
		}
	}
	else
		subParFrames = 0;

	lastClockTick = clockTick;

	if (waitUntilTick == 0)
		slamTicks = 0;
	else
		slamTicks = clockTick - waitUntilTick;
	//
	// calculate the waiting time needed to insure stable frame rate
	//
	USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();

	ELAPSED_TIME = DEF_ELAPSED_TIME;
	REALTIME_FRAMERATE = DEF_REALTIME_FRAMERATE;

	if (defaults->bNoFrameLimit==0)
	{
		if (__int64(waitUntilTick = clockTick + ticksForBaseFrameRate - slamTicks) < 0)
			waitUntilTick += slamTicks;
		if (fabs(elapsedTime - RENDER_PERIOD) < (RENDER_PERIOD / 20.0))
			elapsedTime = RENDER_PERIOD;

		//
		// convert -10 to +10 to .25 to 4.0
		//
		double gameSpeed = double(DEFAULTS->GetDefaults()->gameSpeed);
		double mul = pow(2, (gameSpeed * (1/5.0)));
		elapsedTime *= mul;

		if (defaults->bConstUpdateRate==0)
		{
			ELAPSED_TIME *= mul;
			REALTIME_FRAMERATE /= mul;
		}
	}

	numRealTimeFrames = 0;
	updateTimeRemainder += elapsedTime;

	while (updateTimeRemainder >= ELAPSED_TIME/8)
	{
		numRealTimeFrames++;
		updateTimeRemainder -= ELAPSED_TIME/8;
	}

#if 0
	if (DEFAULTS->GetDefaults()->bNoFrameLimit==0)
	{
		if (numRealTimeFrames != 1)
		{
			CQTRACE52("numRealTimeFrames = %d. Elapsed=%d", numRealTimeFrames, (U32)(elapsedTime*1000));
		}
	}
#endif
}
//--------------------------------------------------------------------------//
// wait the amount of time needed to lock the frame rate
// sets 'timeWaited'
//
void ObjectList::EndFrame (void)
{
	unsigned __int64 clockTick, firstClockTick;

	QueryPerformanceCounter((LARGE_INTEGER *)&firstClockTick);

	if (firstClockTick >= waitUntilTick)
	{
		timeWaited = 0;
		waitUntilTick = firstClockTick;
	}
	else
	{
		do 
		{
			Sleep(0);
			QueryPerformanceCounter((LARGE_INTEGER *)&clockTick);
		} while (clockTick < waitUntilTick);
			
		timeWaited = DOUBLE(int(clockTick - firstClockTick)) * clockPeriod;
		waitUntilTick = clockTick;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::Update (void)
{
	U32 i;
	U32 updateCounter = MGlobals::GetUpdateCount();			// number of times we have updated AI
	U64 firsttick,lasttick;
	if (CQFLAGS.bTracePerformance)
		doPerformanceTrace(updateCounter);
	QueryPerformanceCounter((LARGE_INTEGER *)&firsttick);
#if 0
	CQASSERT(HEAP->EnumerateBlocks());
#endif


	if (CQFLAGS.bGamePaused==0 && CQFLAGS.bRTGamePaused==0 && DEFAULTS->GetDefaults()->bEditorMode == 0)
	{
		U64 pretick, posttick;
		
			QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
		processDeferredTermination();
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			timing.add_to_destruction(posttick - pretick);

		pretick = posttick;

		for (i = 0; i < numRealTimeFrames; i++)
		{
				QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
			if ((updateCounter & 7) == 0)
				SECTOR->Update();
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_sector(posttick - pretick);
				pretick = posttick;
			if ((updateCounter & 7) == 0)
				MISSION->Update();
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_mission(posttick - pretick);
				pretick = posttick;
			local_update();
				QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
			local_group_update();
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_group(posttick - pretick);
				pretick = posttick;
			updatePlayerVisibility();
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_player_visibility(posttick - pretick);
				pretick = posttick;

			NUGGETMANAGER->Update();
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_nugget_update(posttick - pretick);
				pretick = posttick;

			if (elapsedTime >= ELAPSED_TIME && i+2< numRealTimeFrames && ((updateCounter & 7) == 0) )
			{
				engine_update(ELAPSED_TIME);
					QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
					timing.add_to_engine(posttick - pretick);
				elapsedTime -= ELAPSED_TIME;
				CAMERAMANAGER->PhysUpdate(ELAPSED_TIME);
			}

			// reset update count if a bogus load mission command was sent by the script
			if (updateCounter==0 || MGlobals::GetUpdateCount() != 0)
			{
				MGlobals::SetUpdateCount(++updateCounter);
				SCHEDULER->UpdateQueue();
			}
		}

		if (elapsedTime > 0)
		{
				QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
			engine_update(elapsedTime);
				QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
				timing.add_to_engine(posttick - pretick);
			CAMERAMANAGER->PhysUpdate(elapsedTime);
		}

			QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
		processDeferredTermination();
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			timing.add_to_destruction(posttick - pretick);
	}
	else
	if (DEFAULTS->GetDefaults()->bEditorMode)
		processDeferredTermination();
	else
	if(CQFLAGS.bRTGamePaused && CQFLAGS.bGamePaused==0)
	{
		for (i = 0; i < numRealTimeFrames; i++)
		{
			if (elapsedTime >= ELAPSED_TIME && i+2< numRealTimeFrames && ((updateCounter & 7) == 0) )
			{
				elapsedTime -= ELAPSED_TIME;
				CAMERAMANAGER->PhysUpdate(ELAPSED_TIME);
			}
			if ((updateCounter & 7) == 0)
				MISSION->Update();
			// reset update count if a bogus load mission command was sent by the script
			if (updateCounter==0 || MGlobals::GetUpdateCount() != 0)
			{
				MGlobals::SetUpdateCount(++updateCounter);
				SCHEDULER->UpdateQueue();
			}
		}
		if (elapsedTime > 0)
		{
			CAMERAMANAGER->PhysUpdate(elapsedTime);
		}
	}
	else if(CQFLAGS.bGamePaused)
	{
		PARTMAN->Update(gameTime);
	}

	if (CQFLAGS.bGameActive)
	{
		U64 pretick, posttick;
			QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
		FOGOFWAR->Update();	// full update if an object was removed
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			timing.add_to_fogupdate(posttick - pretick);
	}

#if 0
	CQASSERT(HEAP->EnumerateBlocks());
#endif
	QueryPerformanceCounter((LARGE_INTEGER *)&lasttick);
	timing.add_to_update_total(lasttick-firsttick);
}
//--------------------------------------------------------------------------//
//
void ObjectList::renderGridLines (void)
{
	RECT rc;
	U32 currSystem = SECTOR->GetCurrentSystem();
	SECTOR->GetSystemRect(currSystem, &rc);

	SINGLE xpos = 0.0f, ypos = 0.0f;
	SINGLE size = SINGLE(rc.right - rc.left);

	DisableTextures();
	CAMERA->SetModelView();

	PIPE->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	PIPE->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	PB.Color4ub(200,200,200, 180);

	while (xpos < size)
	{
		PB.Begin(PB_LINES);
		PB.Vertex3f(xpos, 0, 0);
		PB.Vertex3f(xpos, size, 0);
		PB.Vertex3f(0, ypos, 0);
		PB.Vertex3f(size, ypos, 0);
		PB.End();

		xpos += GRIDSIZE;
		ypos += GRIDSIZE;
	}

	// hey, while we're at it, let's draw the footprint for the current system
	COMPTR<ITerrainMap> map;
	SECTOR->GetTerrainMap(currSystem, map);
	map->RenderEdit();
}
//--------------------------------------------------------------------------//
//
void ObjectList::Render (void)
{
	BOOL32 bCheckRect;
	U64 pretick, posttick;
	U64 firsttick;
	QueryPerformanceCounter((LARGE_INTEGER *)&firsttick);
	const U32 currentSystem = SECTOR->GetCurrentSystem();
	const U32 currentPlayer = MGlobals::GetThisPlayer();
	const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();
	const bool bPointRect = (selectionRect.top == selectionRect.bottom && selectionRect.right == selectionRect.left);

#if 0
	CQASSERT(HEAP->EnumerateBlocks());
#endif

	if (CQEFFECTS.bNoForceMaxLOD==0)
		LODPERCENT = 1;
	else
	if (CQEFFECTS.bNoForceMinLOD==0)
		LODPERCENT = 0;
	else
	{
		LODPERCENT = CAMERA->GetCameraLOD();
		if(CQEFFECTS.bExpensiveTerrain)
			LODPERCENT = __max(LODPERCENT, 0.01);		// turn off LOD=0 optimization except in cheap terrain case
	}

	// don't check for highlight when cursor is over the toolbar
	bCheckRect = (bHasFocus && (bPointRect==false || TOOLBAR->bAlert==0));

	//clearHighlightBits(highlightedList);
	FlushHighlightedList();

//	ILight * lights[8];

	CAMERA->SetPerspective();

	if (CQEFFECTS.bExpensiveTerrain==0)
	{
		BATCH->set_state(RPR_BATCH,FALSE);
//		FOGOFWAR->Render();
		DUSTMANAGER->Render();
		FIELDMGR->RenderBackground();
		BATCH->set_state(RPR_BATCH,TRUE);
	}

	if (DEFAULTS->GetDefaults()->bShowGrids)
	{
		renderGridLines();
		bEditorMode = false;
	}

	if (bEditorMode)
	{
		renderGridLines();
	}

	if (DEFAULTS->GetDefaults()->bShowGrids)
		bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;

	QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
	LIGHT->update_lighting(MAINCAM);
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_lighting(posttick - pretick);

	IBaseObject *ptr = objectList;

	pretick = posttick;

	objectsOnScreen = 0;

	if (bCheckRect)
	{
		if (bPointRect)
		{
			SINGLE closeness;
			SINGLE closest = 9999999.0f;
			IBaseObject* highlighted = NULL;

			bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
			highlighted = NUGGETMANAGER->TestVisibleHighlight(closest, defaults, currentSystem, currentPlayer,selectionRect);
			while (ptr)
			{
				ptr->TestVisible(defaults, currentSystem, currentPlayer);
				if (ptr->bVisible &&
					(editMode || (ptr->objClass & CF_SELECT1PASS)) )
				{
					closeness = ptr->TestHighlight(selectionRect);
					if (ptr->bHighlight && closeness < closest)
					{
						closest = closeness;
						highlighted = ptr;
					}
				}
				ptr = ptr->next;
			}
			if (highlighted == NULL)
			{
				ptr = objectList;

				while (ptr)
				{
					if (ptr->bVisible && (ptr->objClass & CF_SELECT2PASS))
					{
						closeness = ptr->TestHighlight(selectionRect);
						if (ptr->bHighlight && closeness < closest)
						{
							closest = closeness;
							highlighted = ptr;
						}
					}
					ptr = ptr->next;
				}
			}
			else
			{
				//some objects still expect to have test highlight called
				ptr = objectList;

				while (ptr)
				{
					if (ptr->bVisible && (ptr->objClass & CF_SELECT2PASS))
					{
						ptr->TestHighlight(selectionRect);
					}
					ptr = ptr->next;
				}
			}
			if (highlighted)
				HighlightObject(highlighted);
		}
		else
		{
			NUGGETMANAGER->TestVisible(defaults, currentSystem, currentPlayer);
			while (ptr)
			{
				ptr->TestVisible(defaults, currentSystem, currentPlayer);
				if (ptr->bVisible)
				{
					ptr->TestHighlight(selectionRect);
					if (ptr->bHighlight)
						HighlightObject(ptr);
				}
				ptr = ptr->next;
			}
		}
	}
	else
	{
		NUGGETMANAGER->TestVisible(defaults, currentSystem, currentPlayer);
		RECT rect = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };
		while (ptr)
		{
			ptr->TestVisible(defaults, currentSystem, currentPlayer);
			if (ptr->bVisible)
				ptr->TestHighlight(rect);		// objects expect to be called every frame
			ptr->bHighlight = 0;
			ptr = ptr->next;
		}
	}

	pruneHighlightList();		// remove extraneous obj's from highlight list

	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_visibility(posttick - pretick);
	pretick = posttick;

	if(CQEFFECTS.bExpensiveTerrain)
		FIELDMGR->RenderBackground();

	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	pretick = posttick;

	BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_OPAQUE);//RP_OPAQUE | RP_TRANSLUCENT_UNSORTED);
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_flush(posttick-pretick);
	pretick = posttick;

	static bool bDoIt=false;

	/*IRenderDebugger *rd;
	if (bDoIt)
	{
		GS->QueryInterface(IID_IRenderDebugger,(void **)&rd);
		CQASSERT(rd);
		rd->set_trace_enable(IRD_ST_ALL);
		rd->set_trace_output_dir("C:\\conquest\\src\\temp");
		rd->dump_current_state(IRD_ST_ALL,"C:\\conquest\\src\\temp");
	}*/
	LIGHTS->ActivateBestLights(CAMERA->GetLookAtPosition(),8,800000);
	

	ptr = objectList;
	while (ptr)
	{
		if (ptr->objClass == OC_PLANETOID)
		{
			OBJPTR<IPlanet> planet;
			if (ptr && ptr->QueryInterface(IPlanetID, planet))
			{
			//	if (!planet->IsRenderingFaded())
				{
					if ((ptr->objClass & CF_RENDER2ND) == 0 && (ptr->objClass & CF_RENDEROVERFOG) == 0)
					{
						BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
						ptr->Render();
					}
				}
			}
		}
		ptr = ptr->next;
	}

	ptr = objectList;
	while (ptr)
	{

		bool renderNow = true;
		if (ptr->objClass == OC_PLANETOID)
		{
			OBJPTR<IPlanet> planet;
			if (ptr && ptr->QueryInterface(IPlanetID, planet))
			//if (!planet->IsRenderingFaded())
			{
				renderNow = false;
			}
		}
		
		if (renderNow)
		{
		if ((ptr->objClass & CF_RENDER2ND) == 0 && (ptr->objClass & CF_RENDEROVERFOG) == 0)
		{
			BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
			// call render anyway, because object still have things to render (e.g. fighters)
			ptr->Render();
		}
		}
		ptr = ptr->next;
	}


	EmptyStack();
/*	if (bDoIt)
	{
		rd->set_trace_enable(IRD_ST_OFF);
		rd->save_trace_capture("C:\\conquest\\src\\temp\\someships");
	}*/

	EFFECTPLAYER->Render();

	UNBORNMANAGER->Render();

//	LIGHTS->DeactivateAllLights();

	NUGGETMANAGER->Render();
	EmptyStack();

	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	pretick = posttick;


	BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_OPAQUE);//RP_OPAQUE | RP_TRANSLUCENT_UNSORTED);
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_flush(posttick-pretick);
	pretick = posttick;

	ptr = objectList;	
	while (ptr)
	{
		if (ptr->objClass & CF_RENDER2ND)
		{
			BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
			// call render anyway, because object still have things to render (e.g. fighters)
			ptr->Render();
		}
		ptr = ptr->next;
	}
//	LIGHTS->DeactivateAllLights();

	drawSelectedList();
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	pretick = posttick;

	BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_OPAQUE);//RP_OPAQUE | RP_TRANSLUCENT_UNSORTED);
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_flush(posttick-pretick);
	pretick = posttick;

	if (CQEFFECTS.bExpensiveTerrain)
	{
		FOGOFWAR->Render();
//		DUSTMANAGER->Render();
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_fogrender(posttick - pretick);


	//Render things that must draw over fog
	ptr = objectList;	
	while (ptr)
	{
		if (ptr->objClass & CF_RENDEROVERFOG)
		{
			BATCH->set_render_state(D3DRS_DITHERENABLE,FALSE);
			// call render anyway, because object still have things to render (e.g. fighters)
			ptr->Render();
		}
		ptr = ptr->next;
	}
	LIGHTS->DeactivateAllLights();
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	pretick = posttick;

	BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_OPAQUE);//RP_OPAQUE | RP_TRANSLUCENT_UNSORTED);
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_flush(posttick-pretick);
	pretick = posttick;

	BATCH->set_state(RPR_BATCH,FALSE);

	if (cursorMode == LASSO && ownsResources())
	{
		OrthoView();
		DisableTextures();
		
		PB.Begin(PB_LINES);
		PB.Color3ub(0,255,0);
		PB.Vertex3f(selectionRect.left,selectionRect.top, 0);
		PB.Vertex3f(selectionRect.right,selectionRect.top, 0);
		
		PB.Vertex3f(selectionRect.right,selectionRect.top, 0);
		PB.Vertex3f(selectionRect.right,selectionRect.bottom, 0);
		
		PB.Vertex3f(selectionRect.right,selectionRect.bottom, 0);
		PB.Vertex3f(selectionRect.left,selectionRect.bottom, 0);
		
		PB.Vertex3f(selectionRect.left,selectionRect.bottom, 0);
		PB.Vertex3f(selectionRect.left,selectionRect.top, 0);
		PB.End();
	}

#if 0
	CQASSERT(HEAP->EnumerateBlocks());
#endif

	U64 lastMapTime = SYSMAP->GetMapTiming();
	timing.add_to_map(lastMapTime);

	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_render(posttick-firsttick);
}
//--------------------------------------------------------------------------//
//
void ObjectList::DEBUG_print (void)
{
	debug_print(DEBUGFONT, (CQFLAGS.debugPrint==DBG_ALL));
}
//--------------------------------------------------------------------------//
//
void ObjectList::debug_print (IDebugFontDrawAgent * DEBUGFONT, bool bDetailedReport)
{
	U32 unitsInSystem=0, totalUnits=0, playerUnits=0;
	const U32 systemID = SECTOR->GetCurrentSystem();
	const U32 playerID = MGlobals::GetThisPlayer();

	totalObjects=0;
	char buffer[256];
	int y = 30;
	const int inc = DEBUGFONT->GetFontHeight();
	int numGroups=0;
	bool bTimeTraceNeeded = (CQFLAGS.bTraceMission != 0 && bTimeTraceThisSecond==0);
	const U32 DCOLOR = (THEMATRIX->IsMaster()) ? RGB(200,200,200) : RGB(100,200,100);
	const U32 numSFXHandles = SFXMANAGER->DEBUG_GetNumHandles();

	IBaseObject * ptr = objectList;
	while (ptr)
	{	
		if (DEBUGFONT == ::DEBUGFONT)
			ptr->DEBUG_print();
		if (ptr->objClass & (OC_PLATFORM | OC_SPACESHIP))
		{
			if (ptr->GetSystemID() == systemID)
				unitsInSystem++;

			if (ptr->GetPlayerID() == playerID)
				playerUnits++;

			totalUnits++;
		}

		totalObjects++;

		ptr = ptr->next;
	}

	ptr = groupList;
	while (ptr)
	{	
		numGroups++;
		ptr = ptr->next;
	}

	{
		const U32 updateCounter = MGlobals::GetUpdateCount();			// number of times we have updated AI
		U32 hours;
		U32 minutes;
		U32 seconds;
		U32 rem;

		hours   = updateCounter/(U32(REALTIME_FRAMERATE*8*3600));
		rem = updateCounter%(U32(REALTIME_FRAMERATE*8*3600));
		minutes = rem/(U32(REALTIME_FRAMERATE*8*60));
		rem = rem%(U32(REALTIME_FRAMERATE*8*60));
		seconds = rem / (U32(REALTIME_FRAMERATE*8));
		
		sprintf(buffer, "Time: %01d:%02d:%02d", hours, minutes, seconds);
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		if ((seconds % 16) != 0)
		{
			bTimeTraceThisSecond = false;
			bTimeTraceNeeded = false;
		}
		else
		if (bTimeTraceNeeded)
		{
			bTimeTraceThisSecond = true;		// don't do this next frame
			OPPRINT0(buffer);
		}
		else
			bTimeTraceNeeded = false;
	}

	sprintf(buffer, "LOD: %4d, System: %d, Agents: %3d", U32(LODPERCENT*1000), SECTOR->GetCurrentSystem(), THEMATRIX->GetNumAgents());
	DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
	y += inc;

	if (PLAYERID!=0 && MGlobals::IsHost())
	{
		U32 numHosts = NETPACKET->GetNumHosts();
		U32 mask, syncData, opData, harvest;
		if (numHosts > 1)
			numHosts--;
		THEMATRIX->GetDataThroughput(&mask, &syncData, &opData, &harvest);
		sprintf(buffer, "OPAGENT SYNC: %X, %4d, %4d, %4d", mask, syncData*numHosts, opData*numHosts, harvest*numHosts);
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		if (bTimeTraceNeeded)
		{
			OPPRINT1("  %s\r\n", buffer);
		}

		y += inc;
	}
	else
	if (bTimeTraceNeeded)
		OPPRINT0("\r\n");


	sprintf(buffer, "Groups: %d, SFX: %d, LT: %d, Objects: TU:%3d, SU:%3d, PU:%3d", numGroups, numSFXHandles, numLights, totalUnits, unitsInSystem, playerUnits);
	DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
	y += inc;

	sprintf(buffer, "Current Player: %d", MGlobals::GetThisPlayer());
	DEBUGFONT->StringDraw(0, 8, y, buffer, COLORTABLE[MGlobals::GetColorID(MGlobals::GetThisPlayer())]);
	y += inc;

	if (bDetailedReport==false && CQFLAGS.debugPrint == DBG_SOME)
		return;

	//-----------update----------------

	sprintf(buffer, "Update total: %4d", timing.do_average(timing.update_total));
	DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
	y += inc;

	if (bDetailedReport || CQFLAGS.debugPrint == DBG_ALL)
	{
		sprintf(buffer, "   sector: %4d", timing.do_average(timing.sector));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   mission: %4d", timing.do_average(timing.mission));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "      %s: %4d", timingNames[TIMING_PLAYERAI], timing.do_average(timing.timer_array[TIMING_PLAYERAI]));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "      %s: %4d", timingNames[TIMING_OPAGENT], timing.do_average(timing.timer_array[TIMING_OPAGENT]));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   group: %4d", timing.do_average(timing.group));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
		
		sprintf(buffer, "   weapons: %4d", timing.do_average(timing.weapon));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
		
		sprintf(buffer, "   harvest: %4d", timing.do_average(timing.harvest));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   gunboat: %4d", timing.do_average(timing.gunboat));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
		
		sprintf(buffer, "   other: %4d", timing.do_average(timing.local));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   physics: %4d", timing.do_average(timing.engine));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   destruction: %4d", timing.do_average(timing.deferredTerm));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   fog: %4d", timing.do_average(timing.fogupdate));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   visibility: %4d", timing.do_average(timing.player_visibility));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   nugget: %4d", timing.do_average(timing.nugget_update));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
	}

	//-----------render----------------

	sprintf(buffer, "Render total: %4d", timing.do_average(timing.render));
	DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
	y += inc;

	if (bDetailedReport || CQFLAGS.debugPrint == DBG_ALL)
	{
		sprintf(buffer, "   lighting: %4d", timing.do_average(timing.lighting));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   visibility: %4d", timing.do_average(timing.visibility));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
		
		sprintf(buffer, "   fog: %4d", timing.do_average(timing.fogrender));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;

		sprintf(buffer, "   flush: %4d", timing.do_average(timing.render_flush));
		DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
		y += inc;
	}

	{
		int i = 0;

		for (i = 0; i < TIMING_PLAYERAI; i++)
		{
			char * name = (timingNames[i]) ? timingNames[i] : "?";
			sprintf(buffer, "%s: %4d", name, timing.do_average(timing.timer_array[i]));
			DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
			y += inc;

			if (i == TIMING_RENDER2D)
			{
				if (bDetailedReport || CQFLAGS.debugPrint == DBG_ALL)
				{
					sprintf(buffer, "   map: %4d", timing.do_average(timing.map));
					DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
					y += inc;
				}
			}
		}
	}

	sprintf(buffer, "total: %4d", timing.do_average(timing.total));
	DEBUGFONT->StringDraw(0, 8, y, buffer, DCOLOR);
	y += inc;

	if (bDetailedReport || CQFLAGS.debugPrint == DBG_ALL)
	{
		NETPACKET->DEBUG_print(DEBUGFONT);
	}
}
#define PFPRINT0(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define PFPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#define PFPRINT2(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2)
#define PFPRINT3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3)
#define PFPRINT4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4)
#define PFPRINT5(exp,p1,p2,p3,p4,p5) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4, p5)
#define PFPRINT6(exp,p1,p2,p3,p4,p5,p6) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1, p2, p3, p4, p5, p6)
//--------------------------------------------------------------------------//
//
void ObjectList::doPerformanceTrace (U32 updateCounter)
{
	U32 seconds = updateCounter/(U32(DEF_REALTIME_FRAMERATE*8));
	static bool bNextFrameOk;

	if (bNextFrameOk && (seconds % 60) == 0)
	{
		U32 hours;
		U32 minutes;
		U32 rem;
		U32 total;
		U32 update;
		SINGLE lowBudget, medBudget;

		hours   = updateCounter/(U32(REALTIME_FRAMERATE*8*3600));
		rem = updateCounter%(U32(REALTIME_FRAMERATE*8*3600));
		minutes = rem/(U32(REALTIME_FRAMERATE*8*60));
		rem = rem%(U32(REALTIME_FRAMERATE*8*60));
		seconds = rem / (U32(REALTIME_FRAMERATE*8));
			
		total = timing.do_average(timing.total);
		update = timing.do_average(timing.update_total) + timing.do_average(timing.timer_array[TIMING_UPDATE2D]);
	
		if (total && update)
		{
			// 10 FPS, budget 50% for update, adjust to 100%
			lowBudget = ((total * fps) / 10) * 0.50 * 0.01;
			lowBudget = update / lowBudget;

			// 15 FPS, budget 50% for update, adjust to 100%
			medBudget = ((total * fps) / 15) * 0.50 * 0.01;
			medBudget = update / medBudget;

			PFPRINT3("Time: %01d:%02d:%02d", hours, minutes, seconds);
			PFPRINT3("   FPS: %2d,  %3.1f%% at 10FPS, %3.1f%% at 15FPS\r\n", F2LONG(fps), lowBudget, medBudget);
			bNextFrameOk = false;
		}
	}
	else
	if ((seconds % 60) != 0)
		bNextFrameOk = true;
}
#ifndef FINAL_RELEASE
//--------------------------------------------------------------------------//
//
void ObjectList::issuePerformanceWarning (bool bWarning)
{
	char buffer[256];
	const USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();
	REPORTFONT<4096> report;
	MEMORYSTATUS memoryStatus;
	U32 dwBufferSize;

	if (bWarning)
		report.addText("\r\nPerformance Warning");
	else
		report.addText("\r\nPerformance Report");
	report.addText(" for machine ");
	dwBufferSize = sizeof(buffer);
	GetComputerName(buffer, &dwBufferSize);
	report.addText(buffer);
	report.addText("\r\n");
	sprintf(buffer, "FPS: %.1f\r\n", fps);
	report.addText(buffer);

	debug_print(&report, true);
	report.addText("-----------------\r\n");
#ifdef _DEBUG
	report.addText("DEBUG BUILD\r\n");
#endif
	sprintf(buffer, "Speed Setting: %d\r\n", defaults->gameSpeed);
	report.addText(buffer);
	sprintf(buffer, "Video Resolution: %dx%d", SCREENRESX, SCREENRESY);
	report.addText(buffer);
	if (SCREENRESX==640 && SCREENRESY==480 && CQFLAGS.bFrameLockEnabled==0)
		report.addText(" LOCKING DISABLED\r\n");
	else
		report.addText("\r\n", 2);
	sprintf(buffer, "AI Players: %d\r\n", MISSION->GetNumAIPlayers());
	report.addText(buffer);
	sprintf(buffer, "%s mode, gdi=%s, Fog: %s\r\n", ((CQFLAGS.bFullScreen)?"Full Screen":"Windowed"), ((CQFLAGS.bNoGDI)?"off":"on"),
		((defaults->fogMode==FOGOWAR_NORMAL)?"Normal":((defaults->fogMode==FOGOWAR_NONE)?"None":"Explored")));
	report.addText(buffer);
	sprintf(buffer, "WT: %d, ET: %d, XT: %d, T: %d, B: %d\r\n", CQEFFECTS.bWeaponTrails,
			CQEFFECTS.bEmissiveTextures, CQEFFECTS.bExpensiveTerrain, CQEFFECTS.bTextures, CQEFFECTS.bBackground);
	report.addText(buffer);
	sprintf(buffer, "Objects on screen: %d, Objects in list: %d, Toolbar: %s\r\n", objectsOnScreen, totalObjects, ((CQFLAGS.bNoToolbar)?"off":"on"));
	report.addText(buffer);
	sprintf(buffer, "/mdump: %s, /dumpfile: %s, /record: %s /const_update: %s\r\n", ((CQFLAGS.bTraceMission)?"on":"off"), ((CQFLAGS.bDumpFile)?"on":"off"), ((CQFLAGS.bHostRecordMode)?"on":"off"), ((defaults->bConstUpdateRate)?"on":"off"));
	report.addText(buffer);
	GlobalMemoryStatus(&memoryStatus);
	sprintf(buffer, "PMemory: %d MB, VMemory: %d MB, VAddress: %d MB, HeapSize: %d MB\r\n", memoryStatus.dwTotalPhys>>20, memoryStatus.dwAvailPageFile>>20, memoryStatus.dwAvailVirtual>>20,
		(HEAP->GetHeapSize()+GetBatchHeap()->GetHeapSize()) >> 20);
	report.addText(buffer);
	sprintf(buffer, "TEXTURES: %d MB, VERTEX: %d MB, SOUND: %d MB, TexLOD: %d\r\n", TEXMEMORYUSED>>20, VBMEMORYUSED>>20, SNDMEMORYUSED>>20, TEXLOD);
	report.addText(buffer);

	CQIMAGE->Report(report.buffer);
}
#endif // !FINAL_RELEASE
//--------------------------------------------------------------------------//
//
void ObjectList::DEBUG_reloadDatabase (void)
{
	CQASSERT(archList==0 && objectList==0);
	if (archList==0 && objectList==0)
	{
		loadTypesData();
	}
	else
		CQERROR0("Mission file still open! Cannot reload database.");
}
//--------------------------------------------------------------------------//
//
IBaseObject * ObjectList::FindObject (U32 dwMissionID, bool bAlwaysIfTaken)
{
	ACTOR * node;

	if ((node = actorList.findHashedNode(dwMissionID)) != 0)
	{
		IBaseObject * obj = node->actor;
		if (bAlwaysIfTaken || obj->prev || obj->next || obj == objectList || obj->objClass==OC_FIGHTER)
			return obj;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
struct IGroup * ObjectList::FindGroupObject (U32 dwMissionID)
{
	GROUPACTOR * node;

	if ((node = groupActorList.findHashedNode(dwMissionID)) != 0)
	{
		return node->actor;
	}

	return 0;
}
//--------------------------------------------------------------------------//
BOOL32 ObjectList::AddGroupPartID (IGroup *obj, U32 dwMissionID)
{
	if (dwMissionID)
	{
		GROUPACTOR * node;

		if ((node = groupActorList.findHashedNode(dwMissionID)) != 0)
		{
			CQASSERT(node->actor == obj);
		}
		else
		{
			node = new GROUPACTOR(dwMissionID);
			groupActorList.addNode(node);
		}
		
		node->actor = obj;
		node->dwMissionID = dwMissionID;
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::RemoveGroupPartID (IGroup *obj, U32 dwMissionID)
{
	if (bInsideClose==0 && dwMissionID)
	{
		GROUPACTOR * node;
		node = groupActorList.findHashedNode(dwMissionID);
		if (node)
		{
			CQASSERT(node->actor == obj);
			groupActorList.removeNode(node);
			delete node;
		}
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
void ObjectList::BroadcastAllianceForPlayers (U32 playerID1, U32 playerID2)
{
	// call OnAlliaceChange for every object that is playerID1 or playerID2 and has the IAttack interface
	IBaseObject * obj = objectList;
	
	U32 allyMask = 0;
	allyMask |= 1 << (playerID1-1);
	allyMask |= 1 << (playerID2-1);

	while (obj)
	{
		if ((obj->GetPlayerID() == playerID1) || (obj->GetPlayerID() == playerID2))
		{
			VOLPTR(IAttack) attack = obj;
			if (attack)
			{
				attack->OnAllianceChange(allyMask);
			}
		}
		obj = obj->next;
	}
}
//--------------------------------------------------------------------------//
//
U32 ObjectList::GetShipsToRender()
{
	return objectsOnScreen;
}
//--------------------------------------------------------------------------//
//
void ObjectList::IncrementShipsToRender()
{
	++objectsOnScreen;
}
//--------------------------------------------------------------------------//
//
bool ObjectList::CreateProjectile()
{
	USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();
	if(numProjectiles < defaults->maxProjectiles)
	{
		++numProjectiles;
		return true;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
void ObjectList::ReleaseProjectile()
{
	if(numProjectiles > 0)
		--numProjectiles;
}
//--------------------------------------------------------------------------//
// TODO: Put some logic here for shadow system
//
void * ObjectList::FindObject (U32 dwMissionID, U32 fromPlayerID, OBJPTR<IBaseObject> & obj, enum OBJID objid) 
{
	IBaseObject * result = FindObject(dwMissionID);
	if (result)
	{
		if(THEMATRIX->IsMaster())
		{
			if(result->IsTargetableByPlayer(fromPlayerID))
			{
				result->QueryInterface(objid, obj, fromPlayerID);
			}
			else
			{
				result = 0;
				obj = NULL;
			}
		}
		else
			result->QueryInterface(objid, obj, fromPlayerID);
	}
	else
	{
		result = 0;
		obj = NULL;
	}
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::IsInList (const IBaseObject * obj)
{
	BOOL32 result = 0;

	if (obj->prev || obj->next || obj == objectList)
		result = 1;

	return result;
}
//--------------------------------------------------------------------------//
// set the first fighter in the list
//
/*void ObjectList::SetFighterList (IBaseObject * obj)
{
	fighterList = obj;
}*/
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::AddObject (IBaseObject *obj)
{
	CQASSERT(obj->prev == 0);
	CQASSERT(obj->next == 0);
	CQASSERT(obj != objectList);

	obj->prev = 0;
	obj->prevTarget = 0;
	obj->nextTarget = 0;
	obj->next = objectList;
	obj->nextSelected = obj->prevSelected = 0;
	obj->nextHighlighted = obj->prevHighlighted = 0;

	if (objectList)
	{
		objectList->prev = obj;
	}

	objectList = obj;

	//rmarr code
	obj->hashNext = hashList[obj->updateBin];
	if (hashList[obj->updateBin])
	{
		hashList[obj->updateBin]->hashPrev = obj;
	}

	hashList[obj->updateBin] = obj;
	//end rmarr code

	IBaseObject::MDATA mdata;
	if (obj->GetMissionData(mdata))  //	if (obj->objClass & CF_MISSION)
	{
		obj->nextTarget = targetList;
		if (targetList)
			targetList->prevTarget = obj;
		targetList = obj;
	}

	return 1;
}
//-------------------------------------------------------------------
//
BOOL32 ObjectList::AddPartID (IBaseObject *obj, U32 dwMissionID)
{
	if (dwMissionID)
	{
		ACTOR * node;

		if ((node = actorList.findHashedNode(dwMissionID)) != 0)
		{
			CQASSERT(node->actor == obj);
		}
		else
		{
			node = new ACTOR(dwMissionID);
			actorList.addNode(node);
		}
		
		node->actor = obj;
		node->dwMissionID = dwMissionID;

		IBaseObject::MDATA mdata;
		if (obj->GetMissionData(mdata))
			MENU->AddToPartMenu(mdata.pSaveData->partName, dwMissionID);
	}

	return 1;
}
//-------------------------------------------------------------------
//
BOOL32 ObjectList::RemoveObject (IBaseObject *obj)
{
	if (obj==objectList || obj->next || obj->prev)
	{
		CQASSERT (bInLocalUpdate==false && "Can't remove an object during update loop!");
		removeObject(obj);
	}
#ifndef FINAL_RELEASE
	if(obj->objClass == OC_SPACESHIP || obj->objClass == OC_PLATFORM)
		CQASSERT(!OBJMAP->IsObjectInMap(obj));
#endif

	return 1;
}
//-------------------------------------------------------------------
//
void ObjectList::removeObject (IBaseObject *obj)
{
	if (obj == objectList)
	{
		objectList = objectList->next;
	}
	if (obj->prev)
		obj->prev->next = obj->next;
	if (obj->next)
		obj->next->prev = obj->prev;
	obj->prev = obj->next = 0;

	if (obj == hashList[obj->updateBin])
	{
		hashList[obj->updateBin] = hashList[obj->updateBin]->hashNext;
	}
	if (obj->hashPrev)
		obj->hashPrev->hashNext = obj->hashNext;
	if (obj->hashNext)
		obj->hashNext->hashPrev = obj->hashPrev;
	obj->hashPrev = obj->hashNext = 0;

	if (obj == targetList)
		targetList = targetList->nextTarget;
	if (obj->prevTarget)
		obj->prevTarget->nextTarget = obj->nextTarget;
	if (obj->nextTarget)
		obj->nextTarget->prevTarget = obj->prevTarget;
	obj->prevTarget = obj->nextTarget = 0;

	UnselectObject(obj);
	unhighlightObject(obj);
}
//-------------------------------------------------------------------
//
BOOL32 ObjectList::RemovePartID (IBaseObject *obj, U32 dwMissionID)
{
	if (bInsideClose==0 && dwMissionID)
	{
		ACTOR * node;
		node = actorList.findHashedNode(dwMissionID);
		if (node)
		{
			CQASSERT(node->actor == obj);
			actorList.removeNode(node);
			delete node;

			MENU->RemoveFromPartMenu(dwMissionID);
		}
	}

	return 1;
}
//-------------------------------------------------------------------
//
void ObjectList::DeleteAllObjects (void)
{
	while (objectList)
	{
		delete objectList;
	}

/*	CQASSERT(fighterList == 0);*/

	while (groupList)
	{
		delete groupList;
	}
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::GetObjectList (void)
{
	return objectList;
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::GetTargetList (void)
{
	return targetList;
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::GetHashList (U32 updateBin)
{
	CQASSERT(updateBin < 32);
	return hashList[updateBin];
}
//-------------------------------------------------------------------
//
IBaseObject *ObjectList::GetHighlightedList (void)
{
	return highlightedList;
}
//-------------------------------------------------------------------
//
IBaseObject *ObjectList::GetSelectedList (void)
{
	return selectedList;
}
//-------------------------------------------------------------------
//
PARCHETYPE ObjectList::GetArchetype (const C8 *name)
{
	ARCHNODE *list = archList;

	while (list)
	{
		if (strcmp(name, list->archDataType->name) == 0)
			break;
		list = list->next;
	}
	
	return list;
}
//-------------------------------------------------------------------
//
const char * ObjectList::GetArchName (PARCHETYPE pArchetype)
{
	if (pArchetype)
		return pArchetype->archDataType->name;
	else
		return 0;
}
//-------------------------------------------------------------------
//
const char * ObjectList::GetArchName (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return dataType->name;
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
void * ObjectList::GetArchetypeData (PARCHETYPE pArchetype)
{
	if (pArchetype)
		return pArchetype->archDataType->objData;
	else
		return 0;
}
//-------------------------------------------------------------------
//
void * ObjectList::GetArchetypeData (const C8 * name)
{
	ARCHDATATYPE * dataType;
		
	if ((dataType = getArchDataType(name)) != 0)
		return dataType->objData;
	else
		return 0;
}
//-------------------------------------------------------------------
//
U32 ObjectList::GetArchetypeDataID (const C8 * name)
{
	ARCHDATATYPE * dataType;
		
	if ((dataType = getArchDataType(name)) != 0)
		return (U32(dataType) - U32(archData));

	return 0;
}
//-------------------------------------------------------------------
//
U32 ObjectList::GetArchetypeDataID (PARCHETYPE pArchetype)
{
	ARCHDATATYPE * dataType;
	
	if (pArchetype && (dataType = pArchetype->archDataType) != 0)
		return (U32(dataType) - U32(archData));

	return 0;
}
//-------------------------------------------------------------------
//
void * ObjectList::GetArchetypeData (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return dataType->objData;
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
PARCHETYPE ObjectList::LoadArchetype (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return LoadArchetype(dataType->name);
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
BOOL32 ObjectList::EnumerateArchetypeData (struct IArchetypeEnum * enumerator)
{
	BOOL32 result = 1;
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if ((result = enumerator->ArchetypeEnum(type->name, type->objData, type->dataSize)) == 0)
			break;
		type++;
		i--;
	}

	return result;
}
//-------------------------------------------------------------------
//
//#define _ARCHTTIMER_
PARCHETYPE ObjectList::LoadArchetype (const C8 *name)
{
	PARCHETYPE result;

	if ((result = GetArchetype(name)) == 0)
	{
#ifdef _ARCHTTIMER_
		U64 pretick, posttick;
		QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
#endif
		ARCHDATATYPE * dataType;
		
		if ((dataType = getArchDataType(name)) != 0)
		{
			CONNECTION_NODE<IObjectFactory> *node= point2.pClientList;
			HANDLE handle=0;

			result = new ARCHNODE;
			result->prev=0;
			if ((result->next=archList) != 0)
				archList->prev = result;
			result->archDataType = dataType;
			archList = result;

			CURSOR->SetBusy(1);
			
			while (node)
			{
				if ((handle = node->client->CreateArchetype(dataType->name, dataType->objData->objClass, dataType->objData)) != 0)
					break;
				node = node->pNext;
			}

			if (node)
			{
				result->factory = node->client;
				result->hArchetype = handle;
			}
			else
			{
				unloadArchetype(result);
				result = 0;
				CQERROR1("No provider found for '%s'", dataType->name);
			}

			CURSOR->SetBusy(0);
#ifdef _ARCHTTIMER_
		U64 clockFrequency;
		QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
		QueryPerformanceFrequency((LARGE_INTEGER *) &clockFrequency);

		DOUBLE loadtime = posttick-pretick;
		DOUBLE freq = clockFrequency;
		loadtime = loadtime/freq;
		SINGLE singleTime = (SINGLE)loadtime;
		char buffer[256];
		sprintf(buffer,"***Archlist Load:%s Time:%f\n",dataType->name,singleTime);
		OutputDebugString(buffer);

#endif
		}
		else
		{
			if (name[0])
				CQERROR1("Invalid archetype name: '%s'", name);
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
BOOL32 ObjectList::UnloadArchetype (const C8 *name)
{
	BOOL32 result=0;
	ARCHNODE *list = archList;

	while (list)
	{
		if (strcmp(name, list->archDataType->name) == 0)
		{
			unloadArchetype(list);
			result = 1;
			break;
		}
		list = list->next;
	}
	
	return result;
}
//-------------------------------------------------------------------
//
void * ObjectList::GetArchetypeHandle(PARCHETYPE pArchetype)
{
	return pArchetype->hArchetype;
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::CreateInstance (PARCHETYPE pArchetype)
{
	IBaseObject * obj = 0;
	ARCHNODE * node = pArchetype;

	if (node == 0)
		goto Done;

#if 0   //def _JASON
	CQTRACE12("Entering CreateInstance(\"%s\") HEAP=%d", GetArchName(pArchetype), HEAP->GetAvailableMemory());
	MarkAllocatedBlocks(HEAP);
#endif
	CQFLAGS.bInsideCreateInstance=1;	// for test purposes only
	obj = node->factory->CreateInstance(node->hArchetype);
	CQFLAGS.bInsideCreateInstance=0;	// for test purposes only
#if 0 // def _JASON
	CQTRACE12("Leaving CreateInstance(\"%s\")  HEAP=%d", GetArchName(pArchetype), HEAP->GetAvailableMemory());
	PrintHeap(HEAP);
#endif

	if (obj)
	{
		obj->pArchetype = pArchetype;
		obj->updateBin = creationCounter++;
		node->usage++;
	}

Done:
	return obj;
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::CreateInstance (const char *name)
{
//	IBaseObject *obj = 0;
	ARCHNODE * node = LoadArchetype(name);

	return CreateInstance(node);
}
//-------------------------------------------------------------------
//
void ObjectList::AddRef (PARCHETYPE pArchetype, const char * szUser)
{
	pArchetype->usage++;
#ifndef FINAL_RELEASE
	pArchetype->addName(szUser);
#endif  // !FINAL_RELEASE
}
//-------------------------------------------------------------------
//
void ObjectList::Release (PARCHETYPE pArchetype, const char * szUser)
{
	pArchetype->usage--;
	CQASSERT(S32(pArchetype->usage) >= 0);
#ifndef FINAL_RELEASE
	pArchetype->removeName(szUser);
#endif
}
//-------------------------------------------------------------------
//
IBaseObject * ObjectList::CreateUIAnim (UIANIMTYPE type,const Vector &vec)
{
	CQASSERT(U32(type) < UI_END);

	const char *szTypes[UI_END] = {"UIANIM!!Move", "UIANIM!!Build","UIANIM!!Ram",
		"UIANIM!!Move_Attack","UIANIM!!Special_Attack","UIANIM!!Sell","UIANIM!!Jump",
		"UIANIM!!Attack","UIANIM!!Patrol","UIANIM!!Add_Ship","UIANIM!!Remove_Ship", "UIANIM!!Move_waypoint"};

	if (pAnimType[type] == 0)
		pAnimType[type] = LoadArchetype(szTypes[type]);
	CQASSERT(pAnimType[type]);

	IBaseObject *obj = CreateInstance(pAnimType[type]);

	OBJPTR<IBlast> blast;

	AddObject(obj);

	if (obj->QueryInterface(IBlastID,blast))
	{
		blast->InitBlast(Transform(vec), SECTOR->GetCurrentSystem(), 0);
	}

	return obj;
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT ObjectList::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;
	CONNECTION_NODE<IEventCallback> *node= point.pClientList;
	static bool bIgnore = false;

	switch (message)
	{
	case CQE_HOST_MIGRATE:
		notifyHostMigration();
		break;
	
	case CQE_GOTO_PART:
		{
			IBaseObject * obj;
			if ((obj = FindObject(U32(param))) != 0)
			{
				SECTOR->SetCurrentSystem(obj->GetSystemID());
				CAMERA->SetLookAtPosition(obj->GetPosition());
			}
		}
		break;

	case CQE_PART_SELECTION:
		{
			IBaseObject * obj;
			if ((obj = FindObject(U32(param))) != 0)
				obj->View();
		}
		break;
	
	case CQE_KILL_FOCUS:
		if ((IObjectList *)param != this)
		{
			bHasFocus = FALSE;
			desiredOwnedFlags = 0;
			releaseResources();
			setCursorMode(NOT_OWNED);
		}
		break;
	case CQE_SET_FOCUS:
		bHasFocus = TRUE;
		break;

	case WM_LBUTTONUP:
		if (bIgnore)
		{
			bIgnore = false;
			setCursorMode(SELECT);
			break;
		}
		if (ownsResources())// || (DEFAULTS->GetDefaults()->bRightClickOption && bEditorMode))
		{
			// don't deselect platforms
			if (testHighlightCount(highlightedList, MGlobals::GetThisPlayer()))
			{
				if (!(msg->wParam & MK_CONTROL))
				{
					if (msg->wParam & MK_SHIFT)
					{
						unselectObjects();
					}
					else
						FlushSelectedList();
				}
			}
			else if (highlightedList == NULL && DEFAULTS->GetDefaults()->bRightClickOption)
			{
				EVENTSYS->Send(CQE_NEW_SELECTION, 0);
			}

			if (cursorMode == LASSO)
			{
				// is the first thing selected a platform?  If so, then deselect it (unless it's a gunplat)
				MPart firstSelected = OBJLIST->GetSelectedList();
				if (DEFAULTS->GetDefaults()->bRightClickOption && firstSelected.isValid() && 
					(MGlobals::IsPlatform(firstSelected->mObjClass) && MGlobals::IsGunPlat(firstSelected->mObjClass) == false))
				{
					EVENTSYS->Send(CQE_NEW_SELECTION, 0);		// call ourselves to flushSelectedList
				}
				else
				{
					SelectHighlightedObjects(msg->wParam);
				}
			}
		}
		if (cursorMode == LASSO)
			setCursorMode(SELECT);
		break;

	case WM_LBUTTONDOWN:
		// don't want to set the cursor mode to lasso if we are going to jump
		if (CQFLAGS.bGameActive && bHasFocus && short(HIWORD(msg->lParam)) <= CAMERA->GetPane()->y1) 
		{
			IBaseObject * obj;
			obj = OBJLIST->GetHighlightedList();
			bool bJumpgate = obj ? (obj->objClass == OC_JUMPGATE) : false;

			if ((DEFAULTS->GetDefaults()->bRightClickOption == false) || (bJumpgate == false))
			{
				setCursorMode(LASSO);
				lassoRect.left = lassoRect.right  = short(LOWORD(msg->lParam));
				lassoRect.top  = lassoRect.bottom = short(HIWORD(msg->lParam));
			}
		}
		break;

	case WM_LBUTTONDBLCLK:
		// select all similar units
		if (ownsResources() && highlightedList)
		{
			// duh...the object you are selecting has to be your own ship dumbass
			if (highlightedList->GetPlayerID() == MGlobals::GetThisPlayer())
			{
				MPart part = highlightedList;
				M_OBJCLASS objectClass = part->mObjClass;

				IBaseObject * obj = GetTargetList();
				FlushHighlightedList();

				while (obj)
				{
					part = obj;

					if (part.isValid() && obj->bVisible && part->mObjClass == objectClass)
					{
						HighlightObject(obj);
					}

					obj = obj->nextTarget;
				}

				SelectHighlightedObjects(msg->wParam);
				bIgnore = true;
			}
		}
	break;


	case WM_RBUTTONDOWN:
		if (viewerCount==0 && DEFAULTS->GetDefaults()->bEditorMode && ownsResources())
		{
			IBaseObject * ptr = selectedList;

			while (ptr)
			{
				PostMessage(hMainWindow, CQE_VIEW_OBJECT, 0, (LPARAM)ptr);
				viewerCount++;
				ptr = ptr->nextSelected;
			}

			if (highlightedList && highlightedList->bSelected==0 && highlightedList->objClass != OC_JUMPGATE)
			{
				PostMessage(hMainWindow, CQE_VIEW_OBJECT, 0, (LPARAM)highlightedList);
				viewerCount++;
			}
		}
		break;

	case WM_RBUTTONUP:
		if (bHasFocus && ownsResources())
		{
			if (cursorMode == LASSO)
				setCursorMode(SELECT);
			else if (cursorMode == SELECT && DEFAULTS->GetDefaults()->bRightClickOption == false)
			{
				EVENTSYS->Send(CQE_NEW_SELECTION, 0);		// call ourselves to flushSelectedList
			}
		}
		break;

	case CQE_VIEW_OBJECT:
		{
			((IBaseObject *)(msg->lParam))->View();
			viewerCount--;
		}
		break;
	
#ifndef FINAL_RELEASE	
	case CQE_DEBUG_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch ((U32) param)
			{
			case IDH_UNLOAD_TYPES:
				flushUnusedArchetypes();
				break;
			} // end switch ((U32) param)
		}
		break; // end case CQE_DEBUG_HOTKEY

	case CQE_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch ((U32) param)
			{
			case IDH_REPORT_STATS:
				issuePerformanceWarning(false);
				break;
			} // end switch ((U32) param)
		}
		break;	// end case CQE_HOTKEY
#endif
		
	case CQE_NEW_SELECTION:
		if ((void *)static_cast<ObjectList *>(this) != param)
		{
			FlushSelectedList();
			setCursorMode(SELECT);
		}
		break; 
		
	case WM_MOUSEMOVE:
		if (CQFLAGS.bGameActive && bHasFocus && short(HIWORD(msg->lParam)) <= CAMERA->GetPane()->y1)
		{
			if (cursorMode == NOT_OWNED)
				setCursorMode(SELECT);
		}
		else if (cursorMode != LASSO)
		{
		 	setCursorMode(NOT_OWNED);
		}
		if (ownsResources()  && cursorMode == LASSO)
		{
			if (bGrabbedFocus == 0)
			{
				bGrabbedFocus = 1;
				EVENTSYS->Send(CQE_KILL_FOCUS, (IObjectList *)this);
			}
			lassoRect.right  = short(LOWORD(msg->lParam));
			lassoRect.bottom = short(HIWORD(msg->lParam));
			getLassoRect(selectionRect);
		}
		else
		{
			selectionRect.left = short(LOWORD(msg->lParam));
			selectionRect.top = short(HIWORD(msg->lParam));
			selectionRect.bottom = selectionRect.top;
			selectionRect.right = selectionRect.left;
		}
		break;
		
	case WM_MENUSELECT:
		if (HIWORD(msg->wParam) == 0xFFFF)		// menu item was selected
		{
			desiredOwnedFlags = 0;
			releaseResources();
			setCursorMode(NOT_OWNED);
		}
		break;

	case CQE_EDITOR_MODE:
		bEditorMode = (param != 0);
		FlushSelectedList();
		ShowWindow(hDlgBox, (bEditorMode)?SW_SHOW:SW_HIDE);
		EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
		break;
	}
		
	while (node)
	{
		node->client->Notify(message, param);
		node = node->pNext;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void ObjectList::setCursorMode (CURSOR_MODE newMode)
{
	if (newMode != cursorMode)
	{
Top:
		desiredOwnedFlags = 0;
		releaseResources();

		switch (newMode)
		{
		case LASSO:
			desiredOwnedFlags = RF_CURSOR;
			cursorID = IDC_CURSOR_DEFAULT;
			if (ownsResources())
			{
				setResources();
			}
			else
			{
		   		grabAllResources();
			}
			cursorMode = newMode;
			break;

		case NOT_OWNED:
			desiredOwnedFlags = 0;
			cursorID = 0;
			if (bGrabbedFocus)
			{
				if (bHasFocus)
					EVENTSYS->Send(CQE_SET_FOCUS, (IObjectList *)this);
				bGrabbedFocus = 0;
			}
			cursorMode = newMode;
			break;

		case SELECT:
			{
				S32 x, y;
				WM->GetCursorPos(x,y);
				if (y <= CAMERA->GetPane()->y1)
				{
					desiredOwnedFlags = RF_CURSOR;
					statusTextID = cursorID = 0;
		
					if (ownsResources())
						setResources();
					else
					if (grabAllResources() == 0)
					{
						newMode = NOT_OWNED;
						goto Top;	// could not allocate resources
					}

					if (bGrabbedFocus)
					{
						EVENTSYS->Send(CQE_SET_FOCUS, (IObjectList *)this);
						bGrabbedFocus = 0;
					}
					cursorMode = newMode;
				}
				else
				{
				 	newMode = NOT_OWNED;
				 	goto Top;	// could not allocate resources
				}
			}
					
			break;
		};
	}
}
//-------------------------------------------------------------------
//
BOOL ObjectList::_loadArchDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	ObjectList * _this;

	if (message == WM_INITDIALOG)
		SetWindowLong(hwnd, DWL_USER, lParam);
	
	if ((_this = (ObjectList *) GetWindowLong(hwnd, DWL_USER)) != 0)
		return _this->loadArchDlgProc(hwnd, message, wParam, lParam);

	return 0;
}
//-------------------------------------------------------------------
//
static BOOL32 GuardedEnableWindow (HWND hwnd, BOOL bEnable)
{
	BOOL bCurrent = IsWindowEnabled(hwnd);

	if (bCurrent != bEnable)
	{
		EnableWindow(hwnd, bEnable);
		return 1;
	}

	return 0;
}
//----------------------------------------------------------------------------
//
static LONG CALLBACK editControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
 	WNDPROC oldProc = (WNDPROC) GetWindowLong(hwnd, GWL_USERDATA);

	switch (message)
	{
		case WM_KEYDOWN:
			switch (LOWORD(wParam))
			{
			case VK_TAB:
				SetFocus(GetDlgItem(GetParent(hwnd), IDC_TREE1));
				return 0;
			case VK_RETURN:
				PostMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(IDOK, 0), (LONG)hwnd);
				return 0;
			}
			break;
	}

	return CallWindowProc(oldProc, hwnd, message, wParam, lParam);
}
//-------------------------------------------------------------------
//
void ObjectList::setTextForSelection (HWND hwnd)
{
 	HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);

	SetWindowText(hPartName, "Part*");
}
//--------------------------------------------------------------------------//
//
static HTREEITEM addTreeItem (HWND hTree, HTREEITEM hParent, const char * name, U32 data=0)
{
	HTREEITEM result = (hParent) ? TreeView_GetChild(hTree, hParent) : TreeView_GetRoot(hTree);

	if (result)
	{
		//
		// see if name already exists
		//
		do
		{
			TVITEMEX item;
			char buffer[64];
			memset(&item, 0, sizeof(item));

			item.mask = TVIF_HANDLE | TVIF_TEXT;
			item.hItem = result;
			item.cchTextMax = sizeof(buffer);
			item.pszText = buffer;
			buffer[0] = 0;

			if (TreeView_GetItem(hTree, &item))
			{
				if (strcmp(item.pszText, name)==0)
					break;
			}

			result = TreeView_GetNextSibling(hTree, result);

		} while (result);
	}

	//
	// else create a new item
	//
	if (result == 0)
	{
		TVINSERTSTRUCT insert;
		memset(&insert, 0, sizeof(insert));
		insert.hParent = hParent;
		insert.hInsertAfter = TVI_SORT;
		insert.itemex.mask = TVIF_TEXT | TVIF_PARAM;
		insert.itemex.pszText = const_cast<char *>(name);
		insert.itemex.cchTextMax = strlen(name);
		insert.itemex.lParam = data;

		result = TreeView_InsertItem(hTree, &insert);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void ObjectList::initializeArchListbox (HWND hTree)
{
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if (type->editDropable)
		{
			char buffer[64];
			strncpy(buffer, type->name, sizeof(buffer));
			char *ptr = strstr(buffer, "!!");
			if (ptr)
			{
				HTREEITEM hItem = 0;
				*ptr=0;
				ptr+=2;

				if (ptr[0] && ptr[1] == '_')
				{
					switch (toupper(ptr[0]))
					{
					case 'M':
						hItem = addTreeItem(hTree, hItem, "Mantis");
						ptr+=2;
						break;
					case 'T':
						hItem = addTreeItem(hTree, hItem, "Terran");
						ptr+=2;
						break;
					case 'S':
						hItem = addTreeItem(hTree, hItem, "Solarian");
						ptr+=2;
						break;
					case 'V':
						hItem = addTreeItem(hTree, hItem, "Vyrium");
						ptr+=2;
						break;
					default:
						hItem = addTreeItem(hTree, hItem, "Misc");
						break;
					}
				}
				else
					hItem = addTreeItem(hTree, hItem, "Misc");

				hItem = addTreeItem(hTree, hItem, buffer);
				addTreeItem(hTree, hItem, ptr, (U32(type) - U32(archData)));
			}
			else
			{
				HTREEITEM hItem = addTreeItem(hTree, 0, "Misc");
				addTreeItem(hTree, hItem, type->name, (U32(type) - U32(archData)));
			}
		}
		type++;
		i--;
	}

	TreeView_SelectItem(hTree, 0);
}
//-------------------------------------------------------------------
//
struct loadDlgSaveStruct : WINDOWPLACEMENT
{
	loadDlgSaveStruct (void)
	{
		length = sizeof(*this);
	}
};
BOOL ObjectList::loadArchDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			initializeArchListbox(GetDlgItem(hwnd, IDC_TREE1));
			SetFocus(GetDlgItem(hwnd, IDC_TREE1));
			HMENU hMenu = GetSystemMenu(hwnd, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND|MF_DISABLED); 
			SendMessage(GetDlgItem(hwnd, IDM_ADD_PART), BM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadCursor(hResource, MAKEINTRESOURCE(IDC_CURSOR_PLACEOBJECT))); 
			WNDPROC oldProc;
			loadDlgSaveStruct loadStruct;

			HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);
			if ((oldProc = (WNDPROC) GetWindowLong(hPartName, GWL_WNDPROC)) != 0)
			{
				SetWindowText(hPartName, "");
				SetWindowLong(hPartName, GWL_USERDATA, (LONG) oldProc);
				SetWindowLong(hPartName, GWL_WNDPROC, (LONG) editControlProcedure);
			}
			setTextForSelection(hwnd);

			if (DEFAULTS->GetDataFromRegistry("loadDialog", &loadStruct, sizeof(loadStruct)) == sizeof(loadStruct))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.top,
							 loadStruct.rcNormalPosition.right - loadStruct.rcNormalPosition.left,
							 loadStruct.rcNormalPosition.bottom - loadStruct.rcNormalPosition.top,
							 SWP_NOZORDER|SWP_NOSIZE);
				if ((bEditorMode = DEFAULTS->GetDefaults()->bEditorMode) != 0)
					ShowWindow(hwnd, loadStruct.showCmd);
			}
		}
		break;

	case WM_INITMENU:
		{
			HMENU hMenu = (HMENU) wParam;
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND|MF_GRAYED); 
			EnableMenuItem(hMenu, SC_MAXIMIZE, MF_BYCOMMAND|MF_GRAYED); 
			EnableMenuItem(hMenu, SC_SIZE, MF_BYCOMMAND|MF_GRAYED); 
			SetWindowLong(hwnd, DWL_MSGRESULT, 0);
			result = 1;
		}
		break;

	case WM_CLOSE:
		{
			loadDlgSaveStruct loadStruct;

			if (GetWindowPlacement(hwnd, &loadStruct))
				DEFAULTS->SetDataInRegistry("loadDialog", &loadStruct, sizeof(loadStruct));
			DestroyWindow(hwnd);
			dlgArchID = 0;
			result = 1;
		}
		break;

	case WM_NOTIFY:
		switch (LOWORD(wParam))
		{
		case IDC_TREE1:
			{
				NMHDR * hdr = (NMHDR *) lParam;
				switch (hdr->code)
				{
				case NM_RETURN:
					PostMessage(hwnd, WM_COMMAND, MAKELONG(IDOK, 0), 0);
					SetWindowLong(hwnd, DWL_MSGRESULT, 1);
					result = 1;
					break;
				case TVN_KEYDOWN:
					{
						NMTVKEYDOWN * hdr = (NMTVKEYDOWN *) lParam;

						switch (hdr->wVKey)
						{
						case VK_TAB:
//						case VK_RETURN:
							PostMessage(hwnd, WM_COMMAND, MAKELONG(IDOK, 0), 0);
							SetWindowLong(hwnd, DWL_MSGRESULT, 1);
							result = 1;
							break;
						}
					}
					break; // end case TVN_KEYDOWN

				case TVN_SELCHANGED:
					{
						NMTREEVIEW * hdr = (NMTREEVIEW *) lParam;
						dlgArchID = hdr->itemNew.lParam;
						setTextForSelection(hwnd);
						EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
						GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 1);	// enable button
					}
					break;	// end case TVN_SELCHANGED

				case NM_DBLCLK:
					if (IsWindowEnabled(GetDlgItem(hwnd, IDM_ADD_PART)))
						SendMessage(hwnd, WM_COMMAND, IDM_ADD_PART, 0);
					else
						SetFocus(GetDlgItem(hwnd, IDC_PART_NAME));
					break;  // end case NM_DBLCLK
				}
			};
			break;	// end case IDC_TREE1
		} // end switch (LOWORD(wParam))
		break; // end case WM_NOTIFY

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetFocus() == GetDlgItem(hwnd, IDC_PART_NAME) && IsWindowEnabled(GetDlgItem(hwnd, IDM_ADD_PART)))
				SendMessage(hwnd, WM_COMMAND, IDM_ADD_PART, 0);
			else
				SetFocus(GetDlgItem(hwnd, IDC_PART_NAME));
			break;
	
		case IDC_PART_NAME:
			switch (HIWORD(wParam))
			{
			case EN_SETFOCUS:
				SendMessage((HWND)lParam, EM_SETSEL, 0, -1);	// select all text
				break;
			case EN_CHANGE:
				EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
				if (GetWindowTextLength((HWND)lParam) == 0)
				{
					GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 0);	// disable button
				}
				else
				{
					GuardedEnableWindow(GetDlgItem(hwnd, IDM_ADD_PART), 
						(dlgArchID != 0));
				}
				break;
			}
			break;

		case IDM_ADD_PART:
			{
				HWND hPartName = GetDlgItem(hwnd, IDC_PART_NAME);
				char partName[32];

				if (dlgArchID!=0 && GetWindowTextLength(hPartName))
				{
					GetWindowText(hPartName, partName, sizeof(partName));

					if (createPart(dlgArchID, partName))
					{
//						setTextForSelection(hwnd);
						SetFocus(hMainWindow);
					}
				}

			}
			break;
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
void ObjectList::flushUnusedArchetypes (void)
{
	ARCHNODE * node = archList;

	while (node)
	{
		if (node->usage == 0)
		{
			unloadArchetype(node);
			node = archList;
		}
		else
			node = node->next;
	}
}
//-------------------------------------------------------------------
//
void ObjectList::unloadArchetype (ARCHNODE * node)
{
	if (node->factory)
		node->factory->DestroyArchetype(node->hArchetype);

	if (node == archList)
		archList = node->next;
	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;

	delete node;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::Save (struct IFileSystem * outFile)
{
	IBaseObject * obj = objectList;
	BOOL32 result = 0;
	COMPTR<IFileSystem> file;
	OBJPTR<ISaveLoad> pSaveLoad;
	char buffer[64];
	DAFILEDESC fdesc = buffer;
	U32 saveCounter=0;		// unique number

	// make sure we don't have lingering objects in the list
	processDeferredTermination();


	fdesc.lpImplementation = "DOS";
	outFile->CreateDirectory("\\ObjectList");

	if (outFile->SetCurrentDirectory("\\ObjectList") == 0)
		goto Done;

	outFile->CreateDirectory("Instances");
	if (outFile->SetCurrentDirectory("Instances") == 0)
		goto Done;

	RecursiveDelete(outFile);

	while (obj)
	{
		//
		// see if object has data to store
		// 
		if (obj->QueryInterface(ISaveLoadID, pSaveLoad) != 0)
		{
			outFile->CreateDirectory(obj->pArchetype->archDataType->name);
			wsprintf(buffer, "%s\\%u", obj->pArchetype->archDataType->name, saveCounter++);

			outFile->CreateDirectory(buffer);

			if (outFile->CreateInstance(&fdesc, file) != GR_OK)
				goto Done;

			BOOL32 saveOk = pSaveLoad->Save(file);

			if (saveOk == 0)
			{
				if (outFile->RemoveDirectory(buffer) == 0)
				{
					CQERROR1("Could not remove directory: '%s'", buffer);
					goto Done;
				}
			}
		}

		obj = obj->next;
	}

	if (saveCounter)
	{
		if (outFile->SetCurrentDirectory("\\ObjectList") == 0)
			goto Done;
		fdesc.lpFileName = "Count";
		fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		fdesc.dwShareMode = 0;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (outFile->CreateInstance(&fdesc, file) == GR_OK)
		{
			DWORD dwWritten;
			file->WriteFile(0, &saveCounter, sizeof(saveCounter), &dwWritten, 0);
			file.free();
		}
	}

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::QuickSave (struct IFileSystem * outFile, bool bDynamicsOnly)
{
	IBaseObject * obj = objectList;
	BOOL32 result = 0;
	COMPTR<IFileSystem> file;
	OBJPTR<IQuickSaveLoad> pSaveLoad;
	DAFILEDESC fdesc = ".\\";
	U32 saveCounter = 0;

	// make sure we don't have lingering objects in the list
	processDeferredTermination();

	fdesc.lpImplementation = "DOS";
	outFile->CreateDirectory("\\ObjectList");

	if (outFile->SetCurrentDirectory("\\ObjectList") == 0)
		goto Done;

	outFile->CreateDirectory("QuickSave");
	if (outFile->SetCurrentDirectory("QuickSave") == 0)
		goto Done;

	RecursiveDelete(outFile);

	while (obj)
	{
		//
		// see if object has data to store
		// 
		if (obj->QueryInterface(IQuickSaveLoadID, pSaveLoad) != 0)
		{
			if((!bDynamicsOnly) || ((obj->objClass == OC_SPACESHIP) || (obj->objClass == OC_PLATFORM) || 
				(obj->objClass == OC_WAYPOINT) ||(obj->objClass == OC_PLAYERBOMB) ||
				(obj->objClass == OC_MOVIECAMERA) ||(obj->objClass == OC_OBJECT_GENERATOR) ||
				(obj->objClass == OC_TRIGGER) ||(obj->objClass == OC_SCRIPTOBJECT)))
			{
				outFile->CreateDirectory(obj->pArchetype->archDataType->name);
				if (outFile->SetCurrentDirectory(obj->pArchetype->archDataType->name) == 0)
				{
					CQERROR1("Could not create directory: '%s'", obj->pArchetype->archDataType->name);
					goto Done;
				}

				if (outFile->CreateInstance(&fdesc, file) != GR_OK)
					goto Done;

				pSaveLoad->QuickSave(file);
				saveCounter++;

				if (outFile->SetCurrentDirectory("..") == 0)
					goto Done;
			}
		}

		obj = obj->next;
	}

	if (saveCounter)
	{
		if (outFile->SetCurrentDirectory("\\ObjectList") == 0)
			goto Done;
		fdesc.lpFileName = "Count";
		fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		fdesc.dwShareMode = 0;
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (outFile->CreateInstance(&fdesc, file) == GR_OK)
		{
			DWORD dwWritten;
			file->WriteFile(0, &saveCounter, sizeof(saveCounter), &dwWritten, 0);
			file.free();
		}
	}

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
U32 ObjectList::getNumArchetypes (void)
{
	ARCHNODE * node = archList;
	U32 result = 0;

	while (node)
	{
		result++;
		node = node->next;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::Close (void)
{
	numProjectiles = 0;
	bInLocalUpdate = false;		// prevent false secondary asserts
	bInsideClose = true;
	MENU->RemoveAllFromPartMenu();
	UNBORNMANAGER->Unload();
	NUGGETMANAGER->Close();
	if(SCHEDULER)
		SCHEDULER->CleanQueue();
	actorList.reset();
	groupActorList.reset();
	DeleteAllObjects();
	flushUnusedArchetypes();
	subParFrames = 0;
	numLights = 0;
	bWarningIssued = false;
	bWasQuickLoad = false;
	unitFont.free();

	if (archList)
		DEBUG_forceFlush();
	actorList.reset();
	groupActorList.reset();
	memset(pAnimType, 0, sizeof(pAnimType));

	{ // flush termination list
		TERMINATION_NODE * node = pHitList;
		while (node)
		{
			pHitList = node->pNext;
			delete node;
			node = pHitList;
		}
	}

	clearFleetGroupList();
	clearFleetMemberList();

	bInsideClose = false;

	return 1;
}
//--------------------------------------------------------------------------//
//
void ObjectList::DEBUG_forceFlush (void)
{
	ARCHNODE * node = archList;

	while (node)
	{
		CQERROR3("archetype '%s' has %d dangling references from \"%s\"!", node->archDataType->name, node->usage, (node->nameList) ? node->nameList->szName : "?");
		node = node->next;
	}

	archList = 0;
	exit(-1);
}
//--------------------------------------------------------------------------//
//
void ObjectList::DEBUG_IncLightCount (void)
{
	numLights++;
}
//--------------------------------------------------------------------------//
//
void ObjectList::DEBUG_DecLightCount (void)
{
	numLights--;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::New (void)
{
	CQBATCH->Reset();
	LODPERCENT = 1;
	createUnitFont();
	ELAPSED_TIME = DEF_ELAPSED_TIME;
	REALTIME_FRAMERATE = DEF_REALTIME_FRAMERATE;
	gameTime = 0;

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::Load (struct IFileSystem * inFile,bool bNoDynamics, bool bOnlyDynamics)
{
	BOOL32 result=0;
	ARCHNODE *node;
	IBaseObject * obj;
	DAFILEDESC fdesc;
	COMPTR<IFileSystem> file;
	WIN32_FIND_DATA data;
	HANDLE handle;
	bWasQuickLoad = 0;
	U32 savedCount = 0, loadCount = 0;
	gameTime = 0;

	CQBATCH->Reset();
	LODPERCENT = 1;
	createUnitFont();
	ELAPSED_TIME = DEF_ELAPSED_TIME;
	REALTIME_FRAMERATE = DEF_REALTIME_FRAMERATE;

	if (inFile->SetCurrentDirectory("\\ObjectList") == 0)
		goto Done;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing

	//
	// determine how many objects we will be loading...
	//
	fdesc.lpFileName = "Count";
	handle = inFile->OpenChild(&fdesc);
	if (handle != INVALID_HANDLE_VALUE)
	{
		DWORD dwRead;
		inFile->ReadFile(handle, &savedCount, sizeof(savedCount), &dwRead, 0);
		if (dwRead != sizeof(savedCount))
			savedCount = 0;
		inFile->CloseHandle(handle);
		handle = INVALID_HANDLE_VALUE;
	}

	MISSION->SetObjlistLoadProgress(0);
	
	//
	// load the instances
	//

	if (inFile->SetCurrentDirectory("Instances") != 0)
	{
		OBJPTR<ISaveLoad> pSaveLoad;
		CQFLAGS.bLoadingObjlist = 1;

		if ((handle = inFile->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
		do
		{
			fdesc.lpFileName = data.cFileName;
			fdesc.hFindFirst = handle;
			if (inFile->CreateInstance(&fdesc, file) != GR_OK)
				break;

			if ((node = LoadArchetype(data.cFileName)) != 0)
			{
				HANDLE handle;
	
				if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
				do
				{
					COMPTR<IFileSystem> file2;
					fdesc.lpFileName = data.cFileName;
					fdesc.hFindFirst = handle;
					if (file->CreateInstance(&fdesc, file2) != GR_OK)		// file2 ->saveLoadType
						break;
					else
					{
						if ((obj = CreateInstance(node)) != 0)
						{
							//
							// see if object has other data to load
							// 
							if (obj->QueryInterface(ISaveLoadID, pSaveLoad) != 0)
							{
								pSaveLoad->Load(file2);
							}
							AddObject(obj);
							loadCount++;
							if (savedCount)
								MISSION->SetObjlistLoadProgress(loadCount / SINGLE(savedCount));
						}
					}

				} while (file->FindNextFile(handle, &data));

				file->FindClose(handle);
			}

		} while (inFile->FindNextFile(handle, &data));

		inFile->FindClose(handle);
		CQFLAGS.bLoadingObjlist = 0;
	}
	else
	if (inFile->SetCurrentDirectory("QuickSave") != 0)
	{
		if(!bNoDynamics)//if we are loading up new platforms from a quick save the max resource values need to be reset.
			MGlobals::ResetResourceMax();

		OBJPTR<IQuickSaveLoad> pSaveLoad;
		bWasQuickLoad = true;

		if ((handle = inFile->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
		do
		{
			fdesc.lpFileName = data.cFileName;
			fdesc.hFindFirst = handle;
			if (inFile->CreateInstance(&fdesc, file) != GR_OK)
				break;

			if ((node = LoadArchetype(data.cFileName)) != 0)
			{
				bool bLoad = true;
				if(bOnlyDynamics)
				{
					BASIC_DATA * aData  = (BASIC_DATA *)(GetArchetypeData(node));
					bLoad = ((aData->objClass == OC_SPACESHIP) || (aData->objClass == OC_PLATFORM) || 
						(aData->objClass == OC_WAYPOINT) ||(aData->objClass == OC_PLAYERBOMB) ||
						(aData->objClass == OC_MOVIECAMERA) ||(aData->objClass == OC_OBJECT_GENERATOR) ||
						(aData->objClass == OC_TRIGGER) ||(aData->objClass == OC_SCRIPTOBJECT));
				}
				else if(bNoDynamics)
				{
					BASIC_DATA * aData  = (BASIC_DATA *)(GetArchetypeData(node));
					bLoad = ((aData->objClass != OC_SPACESHIP) && (aData->objClass != OC_PLATFORM) && 
						(aData->objClass != OC_WAYPOINT) &&(aData->objClass != OC_PLAYERBOMB) &&
						(aData->objClass != OC_MOVIECAMERA) &&(aData->objClass != OC_OBJECT_GENERATOR) &&
						(aData->objClass != OC_TRIGGER) &&(aData->objClass != OC_SCRIPTOBJECT));
				}
				if(bLoad)
				{

					HANDLE handle;
		
					if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
					do
					{
						COMPTR<IFileSystem> file2;
						fdesc.lpFileName = data.cFileName;
						fdesc.hFindFirst = handle;
						if (file->CreateInstance(&fdesc, file2) != GR_OK)		// file2 ->saveLoadType
							break;
						else
						{
							HANDLE handle;
							char szTypeName[64];
							strncpy(szTypeName, data.cFileName, sizeof(szTypeName));

							if ((handle = file2->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
							do
							{
								HANDLE hFile;
								fdesc.lpFileName = data.cFileName;
								fdesc.hFindFirst = handle;
								
								if ((hFile = file2->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
								{
									DWORD dwSize;
									char rawData[1024];

									if (file2->ReadFile(hFile, rawData, sizeof(rawData), &dwSize, 0))
									{
										if ((obj = CreateInstance(node)) != 0)
										{
											//
											// see if object has other data to load
											// 
											if (obj->QueryInterface(IQuickSaveLoadID, pSaveLoad) != 0)
											{
												pSaveLoad->QuickLoad(szTypeName, data.cFileName, rawData, dwSize);
											}
										}
										AddObject(obj);
										loadCount++;
										if (savedCount)
											MISSION->SetObjlistLoadProgress(loadCount / SINGLE(savedCount));
									}
									file2->CloseHandle(hFile);
								}

							} while (file2->FindNextFile(handle, &data));

							file2->FindClose(handle);
						}

					} while (file->FindNextFile(handle, &data));

					file->FindClose(handle);
				}
				else if(bNoDynamics)
				{
					CQBOMB1("Load Warning: Dynamic object in terrain map :%s",data.cFileName);
				}
			}

		} while (inFile->FindNextFile(handle, &data));

		inFile->FindClose(handle);
	}

	ENGINE->update(0);		// make sure child objects get initialized properly
	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void ObjectList::ResolveAssociations (void)
{
	SECTOR->ResolveJumpgates();
	if (bWasQuickLoad==0)
	{
		OBJPTR<ISaveLoad> pSaveLoad;

		IBaseObject* obj;

		for (obj = objectList; obj; obj = obj->next)
		{
			//
			// see if object has data to store
			// 
			if (obj->QueryInterface(ISaveLoadID, pSaveLoad) != 0)
			{
				pSaveLoad->ResolveAssociations();
			}
		}
	}
	else
	{
		OBJPTR<IQuickSaveLoad> pSaveLoad;

		IBaseObject* obj;

		for (obj = objectList; obj; obj = obj->next)
		{
			//
			// see if object has data to store
			// 
			if (obj->QueryInterface(IQuickSaveLoadID, pSaveLoad) != 0)
			{
				pSaveLoad->QuickResolveAssociations();
			}
		}
	}

	//
	// recalculate visibility
	//
	IBaseObject *obj=objectList, *end=0;

	while (obj)
	{
		obj->CastVisibleArea();
		end = obj;
		obj = obj->next;
	}

	obj = end;
	while (obj)
	{
		obj->UpdateVisibilityFlags();
		obj = obj->prev;
	}

	FOGOFWAR->Update();
	CAMERAMANAGER->ResolveMovieCamera();

	notifyHostMigration();
}
//--------------------------------------------------------------------------//
// return total numbers of files, and cumulative size of all files
//
static void get_total_bytes (IFileSystem * file, U32 & dataSize, U32 & numFiles)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	DAFILEDESC fdesc=data.cFileName;
	
	if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		fdesc.hFindFirst = handle;
		do
		{
			// make sure this not a silly "." entry
			if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (strcmp(data.cFileName, "Parsed Files"))
					{
						COMPTR<IFileSystem> pNewFile;
						// traverse subdirectory
						if (file->CreateInstance(&fdesc, pNewFile) == GR_OK)
						{
							get_total_bytes(pNewFile, dataSize, numFiles);
						}
					}
				}
				else 
				{	
				 	dataSize += data.nFileSizeLow;
					numFiles++;
				}
			}

		} while (file->FindNextFile(handle, &data));

		file->FindClose(handle);
	}
}
//--------------------------------------------------------------------------//
//
/*
struct ARCHDATATYPE
{
	C8 name[64];
	void * objData;
	U32 dataSize;		// size of data chunk in bytes
};
struct ARCHDATA
{
	U32 numArchetypes;
	ARCHDATATYPE type[];
};
*/
//--------------------------------------------------------------------------//
//
static U32 calcCheckSum (const U8 * buffer, U32 bufferSize, U32 checkSum)
{
	while (bufferSize-- > 0)
	{
		checkSum += buffer[bufferSize];
	}
	return ~checkSum;
}
//--------------------------------------------------------------------------//
//
static void load_bytes (IFileSystem * file, ARCHDATA * archData, U32 & checkSum)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	DAFILEDESC fdesc=data.cFileName;
	
	if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		fdesc.hFindFirst = handle;
		do
		{
			// make sure this not a silly "." entry
			if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// traverse subdirectory
					if (strcmp(data.cFileName, "Parsed Files"))
					{
						COMPTR<IFileSystem> pNewFile;
						if (file->CreateInstance(&fdesc, pNewFile) == GR_OK)
						{
							load_bytes(pNewFile, archData, checkSum);
						}
					}
				}
				else 
				{	
					HANDLE hFile;

					if ((hFile = file->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
					{
						U32 i = archData->numArchetypes++;
						DWORD dwRead;

						if (i != 0)
							archData->type[i].objData = (BASIC_DATA *) (((U8 *)archData->type[i-1].objData) + archData->type[i-1].dataSize);

						strncpy(archData->type[i].name, data.cFileName, sizeof(archData->type[i].name)-1);
						archData->type[i].dataSize = file->GetFileSize(hFile);
						file->ReadFile(hFile, archData->type[i].objData, archData->type[i].dataSize, &dwRead, 0);
						file->CloseHandle(hFile);
						checkSum = calcCheckSum((U8 *)archData->type[i].objData, dwRead, checkSum);
						archData->type[i].editDropable = archData->type[i].objData->bEditDropable;
					}
				}
			}

		} while (file->FindNextFile(handle, &data));

		file->FindClose(handle);
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::loadTypesData (void)
{
	DAFILEDESC fdesc = "GameTypes.db";
	COMPTR<IFileSystem> file;
	U32 dataSize=0, numFiles=0, checkSum=0;
	BOOL32 result = 0;

	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
	{
		fdesc.lpFileName = "..\\DB\\GameTypes.db";
		if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
		{
			CQFILENOTFOUND(fdesc.lpFileName);
			goto Done;
		}
	}
	
	get_total_bytes(file, dataSize, numFiles);
	
	::free(archData);
	archData = (ARCHDATA *) calloc(sizeof(ARCHDATA)+(sizeof(ARCHDATATYPE)*numFiles)+dataSize, 1);

	archData->type[0].objData = (BASIC_DATA *) (((U8 *)archData) + sizeof(ARCHDATA)+(sizeof(ARCHDATATYPE)*numFiles)); // mark beginning of data
	load_bytes(file, archData, checkSum);
	SendMessage(hMainWindow, CQE_DATA_CHECKSUM, 0, checkSum);

	if (hDlgBox)
		SendMessage(hDlgBox, WM_CLOSE, 0, 0);
	hDlgBox = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_DIALOG2), hMainWindow, _loadArchDlgProc, (LPARAM) this);

	result = 1;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
ARCHDATATYPE * ObjectList::getArchDataType (const C8 * name)
{
	ARCHDATATYPE * result = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if (strcmp(result->name, name) == 0)
			return result;
		result++;
		i--;
	}

	return 0;
}
//--------------------------------------------------------------------------//
//
void ObjectList::FlushSelectedList (void)
{
	static bool recurse = false;
	if (recurse) return;
	recurse = true;

	while (selectedList)
		UnselectObject(selectedList);

	recurse = false;
}
//--------------------------------------------------------------------------//
//
void ObjectList::FlushHighlightedList (void)
{
	while (highlightedList)
		unhighlightObject(highlightedList);
}
//--------------------------------------------------------------------------//
// called during 2D render section
//
void ObjectList::DrawHighlightedList (void)
{
	bool bAllShips = (DEFAULTS->GetDefaults()->bEditorMode);

	IBaseObject * obj = targetList;

	BATCH->set_state(RPR_BATCH, TRUE);
	while (obj)
	{
		obj->DrawFleetMoniker(bAllShips);
		obj = obj->nextTarget;
	}

	if (DBHOTKEY->GetHotkeyState(IDH_HIGHLIGHT_ALL))
	{
		// highlight all visible objects
		IBaseObject * obj = objectList;

		while (obj)
		{
			if (obj->bVisible)
				obj->DrawHighlighted();
			obj = obj->next;
		}
	}
	else
	{
		IBaseObject * obj = highlightedList;

		while (obj)
		{
			obj->DrawHighlighted();
			obj = obj->nextHighlighted;
		}
	}

	BATCH->set_state(RPR_BATCH, FALSE);
}
//--------------------------------------------------------------------------//
//
IBaseObject * ObjectList::FindGroupObject (const U32 *pObjIDs, U32 numObjects)
{
	OBJPTR<IGroup> group;
	IBaseObject * ptr = groupList;

	while (ptr)
	{
		if (ptr->objClass == OC_GROUP)
		{
			if (ptr->QueryInterface(IGroupID, group))
			{
				if (group->TestGroup(pObjIDs, numObjects))
				{
					// move object up to the front of the list
					if (ptr != groupList)
					{
						RemoveGroupObject(ptr);
						AddGroupObject(ptr);
					}
					break;		// found a match
				}
			}
		}

		ptr = ptr->next;
	}

	return ptr;
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::AddGroupObject (IBaseObject *obj)
{
	CQASSERT(obj->prev == 0);
	CQASSERT(obj->next == 0);
	CQASSERT(obj != groupList);

	obj->prev = 0;
	obj->prevTarget = 0;
	obj->nextTarget = 0;
	obj->next = groupList;
	obj->nextSelected = obj->prevSelected = 0;
	obj->nextHighlighted = obj->prevHighlighted = 0;

	if (groupList)
		groupList->prev = obj;
	else
		groupListEnd = obj;

	groupList = obj;

	return 1;
}
//--------------------------------------------------------------------------//
//
void ObjectList::drawSelectedList (void)
{
	IBaseObject * obj = selectedList;

	while (obj)
	{
		obj->DrawSelected();
		obj = obj->nextSelected;
	}
}
//--------------------------------------------------------------------------//
//
int ObjectList::testHighlightCount (IBaseObject * highlighted, U32 playerID)
{
	int count = 0;

	while (highlighted)
	{
		if ((highlighted->objClass & CF_SELECTABLE) && highlighted->GetPlayerID() == playerID)
		{
			MPart part = highlighted;

			if (part->bReady)
				count++;
		}
		highlighted = highlighted->nextHighlighted;
	}

	return count;
}
//--------------------------------------------------------------------------//
//
int ObjectList::getNumSelected (IBaseObject * selected)
{
	int result = 0;

	while (selected)
	{
		result++;
		selected = selected->nextSelected;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void ObjectList::highlightFleetships (void)
{
	IBaseObject *obj = highlightedList;
	MPart part;
	OBJPTR<IAdmiral> admiral;

	while (obj)
	{
		if ((part = obj).isValid())
		{		
			U32 partID = TESTADMIRAL(part->dwMissionID) ? part->dwMissionID : part->admiralID;
			IBaseObject * adObj;
			if (partID && (adObj = FindObject(partID))!=0 && adObj->QueryInterface(IAdmiralID, admiral))
			{
				U32 objectIDs[MAX_SELECTED_UNITS];
				U32 i, num = admiral->GetFleetMembers(objectIDs);

				for (i = 0; i < num; i++)
				{
					if (TESTADMIRAL(objectIDs[i]) == 0)
					{
						IBaseObject * ptr = FindObject(objectIDs[i]);
						if (ptr)
							HighlightObject(ptr);
					}
				}
			}
		}

		obj = obj->nextHighlighted;
	}
}
//--------------------------------------------------------------------------//
// add every highlighted object to the selected list
//
void ObjectList::SelectHighlightedObjects (U32 wParam)
{
	// if single selected an admiral
	if ((wParam & MK_SHIFT)==0 && highlightedList && highlightedList->nextHighlighted==0)
		highlightFleetships();

	IBaseObject *obj;
	if ((obj = highlightedList) == 0)
		return;

	const BOOL32 bEditorMode = DEFAULTS->GetDefaults()->bEditorMode;
	const U32 playerID = MGlobals::GetThisPlayer();
	const bool bMultiselect = ((selectedList&&selectedList!=obj) || testHighlightCount(highlightedList, playerID)>1); 
	bool bObjAdded = false;
	int selectCount = getNumSelected(selectedList);

	if (bEditorMode && bMultiselect)
		return;

	if (bEditorMode)
	{
		if (selectCount < MAX_SELECTED_UNITS)
		{
			// If it is not already in the list...
			if (!(obj == selectedList || obj->prevSelected || obj->nextSelected))
			{
				if ((obj->nextSelected = selectedList) != 0)
					selectedList->prevSelected = obj;
				obj->prevSelected = 0;
				selectedList = obj;
				obj->bSelected = 1;
				bObjAdded = true;
				selectCount++;
			}
		}
	}
	else  // not editor mode
	{
		do
		{
			if (selectCount >= MAX_SELECTED_UNITS)
				break;

			if (obj->GetPlayerID() == playerID)
			{
				VOLPTR(IPlatform) plat = obj;
				if (bMultiselect==false || (obj->objClass == OC_SPACESHIP || (plat && plat->IsDeepSpacePlatform())))
				{
					MPart part = obj;

					if (part->bReady && (!(part->bUnselectable)))
					{
						// don't select hidden admiral shuttle
						if (obj->objMapNode != 0)
						{
							// If it is not already in the list...
							if (!(obj == selectedList || obj->prevSelected || obj->nextSelected))
							{
								if(selectCount == 1) //make sure we unselect a single select object if we get here
								{
									IBaseObject * sel = selectedList;
									VOLPTR(IPlatform) plat = sel;
									if(plat && (!plat->IsDeepSpacePlatform()))
									{
										UnselectObject(sel);
									}
								}
								if ((obj->nextSelected = selectedList) != 0)
									selectedList->prevSelected = obj;
								obj->prevSelected = 0;
								selectedList = obj;
								obj->bSelected = 1;
								bObjAdded = true;
								selectCount++;
							}
						}
					}
				}
			}

		} while ((obj = obj->nextHighlighted) != 0);
	}

	if (bObjAdded)
	{
		EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
		EVENTSYS->Send(CQE_SELECTION_EVENT, 0);
	}
}
//--------------------------------------------------------------------------//
// add object to the highlighted list
//
void ObjectList::HighlightObject (IBaseObject* obj)
{
	// If it is not already in the list...
	if (!(obj == highlightedList || obj->prevHighlighted || obj->nextHighlighted))
	{
		//
		// add it to the end of the list
		//
		IBaseObject * node=highlightedList, * prev = 0;
		
		while (node)
		{
			prev = node;
			node = node->nextHighlighted;
		}
		
		if (prev)
		{
			prev->nextHighlighted = obj;
			obj->prevHighlighted = prev;
		}
		else // empty list
		{
			highlightedList = obj;
		}
	}
}

//--------------------------------------------------------------------------//
// remove object from the highlighted list
//
void ObjectList::unhighlightObject (IBaseObject* obj)
{
	// If it is  in the list...
	if (obj == highlightedList || obj->prevHighlighted || obj->nextHighlighted)
	{
		if (obj->prevHighlighted)
			obj->prevHighlighted->nextHighlighted = obj->nextHighlighted;
		else
		{
			CQASSERT(highlightedList == obj);
			highlightedList = obj->nextHighlighted;
		}

		if (obj->nextHighlighted)
			obj->nextHighlighted->prevHighlighted = obj->prevHighlighted;

		obj->prevHighlighted = 0;
		obj->nextHighlighted = 0;
		obj->bHighlight = 0;
	}
}
//--------------------------------------------------------------------------//
// remove object from the selected list
//
void ObjectList::UnselectObject (IBaseObject* obj)
{
	// Is it in the list at all?
	if (obj == selectedList || obj->prevSelected || obj->nextSelected)
	{
		if (obj->prevSelected)
			obj->prevSelected->nextSelected = obj->nextSelected;
		else
		{
			CQASSERT(selectedList==obj);
			selectedList = obj->nextSelected;
		}

		if (obj->nextSelected)
			obj->nextSelected->prevSelected = obj->prevSelected;

		obj->prevSelected = 0;
		obj->nextSelected = 0;
		obj->bSelected = 0;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::getLassoRect (RECT & rect) const
{
	const PANE * pane = CAMERA->GetPane();

	rect.left  = __max(pane->x0, __min(lassoRect.left, lassoRect.right));
	rect.right = __max(lassoRect.left, lassoRect.right);
	rect.right = __min(rect.right, pane->x1);

	rect.top    = __min(lassoRect.top, lassoRect.bottom);
	rect.bottom = __max(lassoRect.top, lassoRect.bottom);
	rect.bottom = __min(rect.bottom, pane->y1 - IDEAL2REALY(10));	// adjust for toolbar slop area
}
//--------------------------------------------------------------------------//
//
void ObjectList::local_update (void)
{
	IBaseObject *ptr, *next, *deleteList=0;
	const U32 updateCounter = MGlobals::GetUpdateCount();			// number of times we have updated AI
	U32 updateCount = (updateCounter & 7);
	U64 pretick, posttick;
	MPart part;
	
	QueryPerformanceCounter((LARGE_INTEGER *)&pretick);

	bInLocalUpdate=true;
	
	while (updateCount < 32)
	{
		ptr = hashList[updateCount];
		while (ptr)
		{
			next = ptr->hashNext;
			
			//	if (updateCount == (ptr->updateBin&7))
			//	{
			if (ptr->Update() == 0)
			{
				removeObject(ptr);
				ptr->next = deleteList;
				deleteList = ptr;
			}
			
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			
			part = ptr;
			if (part.isValid())
			{
				if (MGlobals::IsGunboat(part->mObjClass))
				{
					timing.add_to_gunboat(posttick - pretick);
				}
				else if (MGlobals::IsHarvester(part->mObjClass))
				{
					timing.add_to_harvest(posttick - pretick);
				}
				else
				{
					timing.add_to_local(posttick - pretick);
				}
			}
			else
				if (ptr->objClass == OC_WEAPON)
					timing.add_to_weapon(posttick - pretick);
				else
					timing.add_to_local(posttick - pretick);
			
			pretick = posttick;
			//	}
				
			ptr = next;
		}
		updateCount += 8;
	}

	bInLocalUpdate=false;

	ptr = deleteList;
	while (ptr)
	{
		next = ptr->next;
		delete ptr;
		ptr = next;
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
	timing.add_to_destruction(posttick - pretick);
}
//--------------------------------------------------------------------------//
//
void ObjectList::local_group_update (void)
{
	IBaseObject *ptr = groupListEnd, *prev, *deleteList=0;

	bInLocalUpdate=true;

	while (ptr)
	{
		int retcode;

		prev = ptr->prev;
		if ((retcode = ptr->Update()) == 0)
		{
			RemoveGroupObject(ptr);
			ptr->prev = deleteList;
			deleteList = ptr;
		}
		else
		if (retcode == 1)		// needs more time, no use updating the rest of the list
			break;
		ptr = prev;
	}

	bInLocalUpdate=false;

	ptr = deleteList;
	while (ptr)
	{
		prev = ptr->prev;
		delete ptr;
		ptr = prev;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::engine_update (SINGLE dt)
{
#if 0
	// testing!!!! 
	ANIM->update(dt);
	ENGINE->update(dt);
#else
	gameTime += dt;

	IBaseObject *ptr = objectList;

	bInLocalUpdate=true;

	while (ptr)
	{
		ptr->PhysicalUpdate(dt);
		ptr = ptr->next;
	}

	bInLocalUpdate=false;

	PARTMAN->Update(gameTime);
	EFFECTPLAYER->Update();

#endif
}
//--------------------------------------------------------------------------//
//
void ObjectList::unselectObjects (void)
{
	IBaseObject* obj;
	IBaseObject* next;
	const U32 currentSystem = SECTOR->GetCurrentSystem();
	const U32 currentPlayer = MGlobals::GetThisPlayer();
	const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();

	for (obj = selectedList; obj; obj = next)
	{
		next = obj->nextSelected;

		obj->TestVisible(defaults, currentSystem, currentPlayer);
		if (obj->bVisible)
		{
			obj->TestHighlight(selectionRect);

			if (obj->bHighlight)
			{
				UnselectObject(obj);
				unhighlightObject(obj);
			}
		}
	}

	EVENTSYS->Send(CQE_NEW_SELECTION, static_cast<ObjectList *>(this));
}
//--------------------------------------------------------------------------//
//
BOOL32 ObjectList::createPart (U32 partID,
							   const char *partName)
{
	BOOL32 result = 0;
	PARCHETYPE pArchetype;
	BASIC_DATA * base;
	MISSION_INFO info;

	pArchetype = LoadArchetype(partID);
	if (pArchetype == 0)
	{
		CQERROR1("Real-time archetype \"%s\" load failed!", GetArchName(partID));
		goto Done;
	}
	base = (BASIC_DATA *) GetArchetypeData(pArchetype);
	CQASSERT(base);

	EVENTSYS->Send(CQE_NEW_SELECTION, 0);

	strncpy(info.partName, partName, sizeof(info.partName));

	MISSION->SetUnsavedData();
	pArchetype->factory->EditorCreateInstance(pArchetype->hArchetype, info);
	result = 1;
Done:
	return result;
}
//-------------------------------------------------------------------
//
void ObjectList::RemoveGroupObject (IBaseObject *obj)
{
	CQASSERT(obj->objClass == OC_GROUP);
	if (obj == groupList)
		groupList = groupList->next;
	if (obj == groupListEnd)
		groupListEnd = groupListEnd->prev;
	if (obj->prev)
		obj->prev->next = obj->next;
	if (obj->next)
		obj->next->prev = obj->prev;
	obj->prev = obj->next = 0;
}
//--------------------------------------------------------------------------//
// cannot not short circuit this code when fog is off because of cloaking and mimic!
//
bool ObjectList::CastVisibleArea (U32 playerID, U32 systemID, const Vector & _pos, const FIELDFLAGS & fieldFlags, SINGLE radius, SINGLE cloakRadius)
{
//	IBaseObject *ptr = objectList;
/*	if (fieldFlags.sensorDamping())
	{
		radius *= 0.5;		// don't affect cloaked radius?
	}*/
	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	const U32 playerMask = (1 << (playerID-1));
	bool result = false;
	GRIDVECTOR pos;
	pos = _pos;

	if (DEFAULTS->GetDefaults()->fogMode == FOGOWAR_NONE)
		radius = 5000;			// a large number

	if ((systemID & 0x80) == 0)
	{
		FOGOFWAR->RevealBlackZone(playerID,systemID,_pos,radius);
	}
	
	ObjMapIterator iter(systemID,_pos,radius*GRIDSIZE);
	while (iter)
	{
		const U32 hisSide = iter->dwMissionID & PLAYERID_MASK;
		if (hisSide == 0  || ((1 << (hisSide-1)) & allyMask) == 0 || iter->obj->objClass == OC_NUGGET)		// if they are NOT allies or it is a nugget
		{
			if (iter->obj->IsPendingVisibilityToPlayer(playerID)==false || (iter->flags & OM_MIMIC))
			{
				GRIDVECTOR gridPos = iter->obj->GetGridPosition(); 
				
				if (gridPos.isZero() == 0)
				{
					SINGLE dist = pos - gridPos;
					
					if ((hisSide==0 || iter->obj->objClass == OC_WEAPON) && dist!=0)	// assume this is a terrain feature
					{
						dist -= 3;		// make sure terrain features turn on easily
					}
					
					if ((iter->flags & OM_MIMIC) && cloakRadius)
					{
						VOLPTR(ILaunchOwner) launchOwner=iter->obj;
						CQASSERT(launchOwner);
						OBJPTR<ILauncher> launcher;
						launchOwner->GetLauncher(2,launcher);
						if (launcher)
						{
							VOLPTR(IMimic) mimic=launcher.Ptr();
							if (dist <= cloakRadius)
							{
								if (hisSide!=0)
								{
									mimic->SetDiscoveredToAllies(allyMask);
								}
								else
									mimic->SetDiscoveredToAllies(playerMask);
							}
						}
					}
					
					if (iter->obj->fieldFlags.sensorDamping() && fieldFlags.sensorDamping()==false)		// he is in a neb, we're not
					{
						if (dist*2.4 <= radius)
						{
							if (hisSide!=0)
							{
								iter->obj->SetVisibleToAllies(allyMask);
								result = true;
							}
							else
								iter->obj->SetVisibleToAllies(playerMask);
						}
					}
					else
					if (iter->obj->bCloaked) 
					{
						if (dist <= cloakRadius && cloakRadius) //this is necessary or units will be detected at 0 distance
						{
							if (hisSide!=0)
							{
								iter->obj->SetVisibleToAllies(allyMask);
								result = true;
							}
							else
								iter->obj->SetVisibleToAllies(playerMask);
						}
					}
					else
					{
						if (dist <= radius)
						{
							if (hisSide!=0)
							{
								iter->obj->SetVisibleToAllies(allyMask);
								result = true;
							}
							else
							{
								iter->obj->SetVisibleToAllies(playerMask);
							}
						}
					}
				}
			}
		}
			
		++iter;
	}
	return result;
}
//--------------------------------------------------------------------------//
//
void ObjectList::DeferredDestruction (U32 victimID)
{
	deferredDestruction(victimID);

	IBaseObject * obj = OBJLIST->FindObject(victimID);
	if(obj && obj->objClass == OC_PLATFORM)
	{
		OBJPTR<IJumpPlat> jumpplat;
		obj->QueryInterface(IJumpPlatID,jumpplat);
		if (jumpplat)
		{
			IBaseObject * sibling = jumpplat->GetSibling();
			if (sibling)
			{
				deferredDestruction(sibling->GetPartID());
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::deferredDestruction (U32 victimID)
{
	//
	// add victim to the termination list
	//
	TERMINATION_NODE * node = pHitList, *prev = 0;

	while (node)
	{
		if (node->victimID == victimID)
			break;

		prev = node;
		node = node->pNext;
	}

	if (node==0)	// not found, add it to the list
	{
		node = new TERMINATION_NODE;
		node->pNext = 0;
		node->victimID = victimID;

		if (prev)
			prev->pNext = node;
		else
			pHitList = node;
	}
}
//--------------------------------------------------------------------------//
//
U32 ObjectList::GetGroupMembers (U32 groupID, U32 objectIDs[MAX_SELECTED_UNITS])
{
	IBaseObject * obj = FindObject(groupID);
	U32 result = 0;

	if (obj)
	{
		if (obj->objClass != OC_GROUP)
		{
			objectIDs[0] = groupID;
			result = 1;
		}
		else
		{
			OBJPTR<IGroup> pGroup;
			U32 tmpIDs[MAX_SELECTED_UNITS];
			S32 i, j;
			MPart part;

			obj->QueryInterface(IGroupID, pGroup);
			CQASSERT(pGroup);

			j = pGroup->GetObjects(tmpIDs);

			for (i = 0; i < j; i++)
			{
				part = FindObject(tmpIDs[i]);
				if (part.isValid() && part->groupID == groupID)
					objectIDs[result++] = tmpIDs[i];
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
IBaseObject * ObjectList::FindClosestAdmiral (IBaseObject * from)
{
	// find an admiral in the same system
	IBaseObject * obj = targetList;
	const U32 playerID = from->GetPlayerID();
	const U32 systemID = from->GetSystemID();

	while (obj)
	{
		U32 hisPart = obj->GetPartID();
		if ((hisPart & PLAYERID_MASK) == playerID && TESTADMIRAL(hisPart) && obj->GetSystemID() == systemID)
		{
			return obj;
		}

		obj = obj->nextTarget;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
GENRESULT ObjectList::GetUnitFont (struct IFontDrawAgent ** ppAgent)
{
	*ppAgent = unitFont;
	unitFont->AddRef();
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
SINGLE ObjectList::GetRealRenderTime (void)
{
	return realRenderTime;
}
//--------------------------------------------------------------------------//
//
void ObjectList::clearHighlightBits (IBaseObject * list)
{
	while (list)
	{
		list->bHighlight = 0;
		list = list->nextHighlighted;
	}
}
//--------------------------------------------------------------------------//
// remove objects from highlight list that don't have bit set
//
void ObjectList::pruneHighlightList (void)
{
	IBaseObject * node = highlightedList, * next = 0;

	while (node)
	{
		next = node->nextHighlighted;
		if (node->bHighlight == 0)
			unhighlightObject(node);
		node = next;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::updatePlayerVisibility (void)
{
	IBaseObject *ptr;// = objectList;
	const U32 updateCounter = MGlobals::GetUpdateCount();			// number of times we have updated AI
	ptr = hashList[updateCounter&31];

	while (ptr)
	{
//		if (ptr->updateBin == (updateCounter & 31))		// assumes that updateBin is 5 bits, unsigned
//		{
			ptr->CastVisibleArea();
			if (((updateCounter>>5) & 7) == 0)			// assumes that updateBin is 5 bits, unsigned
				ptr->UpdateVisibilityFlags();			// update visibility every 8 seconds
			else
				ptr->UpdateVisibilityFlags2();
	//	}

		ptr = ptr->hashNext;
	}

	if (DEFAULTS->GetDefaults()->fogMode == FOGOWAR_NONE || DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		U32 i;
		const U32 playerID = MGlobals::GetThisPlayer();
		const U32 numSystems = SECTOR->GetNumSystems();
		Vector pos;
		FIELDFLAGS fieldFlags;
		pos.zero();
		fieldFlags.zero();
		for (i = 1; i <= numSystems; i++)
		{
			CastVisibleArea(playerID, i, pos, fieldFlags, 128, 0);
		}
	}

	if (updateCounter < 8)	// do at start of a game
	{
		if (DEFAULTS->GetDefaults()->fogMode != FOGOWAR_NORMAL)
		{
			U32 i, playerID;
			const U32 numSystems = SECTOR->GetNumSystems();
			Vector pos;
			FIELDFLAGS fieldFlags;
			pos.zero();
			fieldFlags.zero();

			for (playerID=1; playerID <= MAX_PLAYERS; playerID++)
			{
				for (i = 1; i <= numSystems; i++)
				{
					SECTOR->RevealSystem(i, playerID);
					CastVisibleArea(playerID, i, pos, fieldFlags, 128, 0);
				}
			}
		}
	}

	NUGGETMANAGER->UpdateVisibilityFlags();
}
//--------------------------------------------------------------------------//
//
void ObjectList::processDeferredTermination (void)
{
	TERMINATION_NODE * node = pHitList;
	IBaseObject * obj;

	while (node)
	{
		pHitList = node->pNext;
		if ((obj = FindObject(node->victimID)) != 0)
		{
			delete obj;
#ifndef FINAL_RELEASE
			CQASSERT(!OBJMAP->IsObjectInMap(obj));
#endif
		}
		else
			NUGGETMANAGER->DeleteNugget(node->victimID);

		delete node;
		node = pHitList;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::notifyHostMigration (void)
{
	IBaseObject * obj = targetList;
	OBJPTR<IMissionActor> actor;
	bool bMaster = THEMATRIX->IsMaster();

	while (obj)
	{
		if (obj->QueryInterface(IMissionActorID, actor))
		{
			actor->OnMasterChange(bMaster);
		}

		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void ObjectList::createUnitFont (void)
{
	PGENTYPE pFontType;
	if ((pFontType = GENDATA->LoadArchetype("Font!!ObjectType")) != 0)
	{
		COMPTR<IDAComponent> pComp;
		if (GENDATA->CreateInstance(pFontType, pComp) == GR_OK)
			pComp->QueryInterface("IFontDrawAgent", unitFont);
	}
	CQASSERT(unitFont!=0);
}
//--------------------------------------------------------------------------//
//-------------------------------OBJECT Methods-----------------------------//
//--------------------------------------------------------------------------//
//
IBaseObject::~IBaseObject(void)
{
#ifndef FINAL_RELEASE
	if (debugName && CQFLAGS.bTraceMission)
	{
		FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "~BaseObject() \"%s\"\r\n", debugName);
	}
#endif
	if (pArchetype != 0)
	{
		pArchetype->usage--;
		CQASSERT(S32(pArchetype->usage) >= 0);
	}

	if (OBJLIST)
	{
		if (objClass == OC_GROUP)
			OBJLIST->RemoveGroupObject(this);
		else
			OBJLIST->RemoveObject(this);

		UnregisterWatchersForObject(this);
	}

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct _olist : GlobalComponent
{
	ObjectList * list;

	virtual void Startup (void)
	{
		OBJLIST = list = new DAComponent<ObjectList>;
		AddToGlobalCleanupList((IDAComponent **) &OBJLIST);

		OBJLIST->QueryInterface("IArchetypeList", (void **)&ARCHLIST);
		AddToGlobalCleanupList((IDAComponent **) &ARCHLIST);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		__int64 clockFrequency;

		if (list->loadTypesData() == 0)
			CQBOMB0("Load failed on game database.");

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(OBJLIST, &list->eventHandle);
			FULLSCREEN->SetCallbackPriority(list, EVENT_PRIORITY_OBJLIST);
		}

		list->initializeResources();
		
		if (QueryPerformanceFrequency((LARGE_INTEGER *) &clockFrequency) == 0)
			CQBOMB0("High performance clock not supported on this system.");

		list->clockPeriod = 1.0 / ((DOUBLE) clockFrequency);
		list->ticksForBaseFrameRate  = clockFrequency / RENDER_FRAMERATE;
	}
};

static _olist olist;

//-------------------------------------------------------------------
//-------------------------END ObjList.cpp---------------------------
//-------------------------------------------------------------------
