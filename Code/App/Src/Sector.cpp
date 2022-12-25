//--------------------------------------------------------------------------//
//                                                                          //
//                                Sector.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/Sector.cpp 206   8/23/01 1:53p Tmauer $

   Sector map resource
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Sector.h"
#include "Cursor.h"
#include "ObjClass.h"
#include "ObjList.h"
#include "IObject.h"
#include "SysMap.h"
#include "stdio.h"
#include "Camera.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "DrawAgent.h"
#include "IImageReader.h"
#include "Startup.h"
#include "IMissionActor.h"
#include "TerrainMap.h"
#include "Objlist.h"
#include "BaseHotRect.h"
#include "DSounds.h"
#include "IShapeLoader.h"
#include "Resource.h"
#include "MPart.h"
#include "MGlobals.h"
#include "Sfx.h"
#include "GenData.h"
#include "DSysMap.h"
#include "TDocClient.h"
#include "Menu.h"
#include "IJumpgate.h"
#include "FogOfWar.h"
#include "GridVector.h"
#include "IJumpgate.h"
#include "TManager.h"
#include <DSector.h>
#include "Mission.h"
#include "ObjMap.h"
#include "IBackground.h"
#include "DBHotkeys.h"
#include "IGameLight.h"
#include "CQLight.h"
#include "IJumpPlat.h"
#include "IFabricator.h"
#include "UnitComm.h"
#include "CommPacket.h"
#include "IToolbar.h"
#include "SoundManager.h"

//#include <RPUL\PrimitiveBuilder.h>
#include <malloc.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <Vector.h>	  
#include <Engine.h>	
#include <WindowManager.h>
#include <IDocClient.h>
#include <Document.h>
#include <Viewer.h>

#define SECTOR_PRIORITY  (RES_PRIORITY_TOOLBAR+1)

static char szCameraRegKey[] = "Camera";

static char szRegKey[] = "Sector";
					
//VFX color definitions
#define _CLR_GRAY	 RGB(120,120,120)
#define _CLR_WHITE	 RGB(255,255,255)
#define _CLR_YELLOW  RGB(255,255,0)
#define _CLR_RED     RGB(255,0,0)
#define _CLR_BLUE    RGB(0,0,255)
#define _CLR_SKY	 RGB(0,127,255)
#define _CLR_AQUA    RGB(0,130,180)
#define _CLR_DIM_RED RGB(120,0,0)

#define SCALE_FACTOR 0.65
#define SECTOR_TMAP_SIZE  256

static S32 sector_map_left;
static S32 sector_map_right;
static S32 sector_map_top;
static S32 sector_map_bottom;

//sector map parameters
#define MAP_SIZE_X (sector_map_right-sector_map_left)
#define MAP_SIZE_Y (sector_map_bottom-sector_map_top)
#define MAP_ORIGIN_X (sector_map_left)
#define MAP_ORIGIN_Y (sector_map_top)
#define MAP_EXTENT_X MAP_ORIGIN_X+MAP_SIZE_X
#define MAP_EXTENT_Y MAP_ORIGIN_Y+MAP_SIZE_Y
#define MAP_CENTER_X MAP_ORIGIN_X+MAP_SIZE_X/2
#define MAP_CENTER_Y MAP_ORIGIN_Y+MAP_SIZE_Y/2

#define BEGIN_MAPPING(parent, name) \
	{								\
		HANDLE hFile, hMapping;		\
		DAFILEDESC fdesc = name;	\
		void *pImage;				\
		if ((hFile = parent->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)	\
			CQFILENOTFOUND(fdesc.lpFileName);	\
		hMapping = parent->CreateFileMapping(hFile);	\
		pImage = parent->MapViewOfFile(hMapping)

#define END_MAPPING(parent) \
		parent->UnmapViewOfFile(pImage); \
		parent->CloseHandle(hMapping); \
		parent->CloseHandle(hFile); }



//helper function
void DrawRect(S32 x0,S32 y0,S32 x1,S32 y1,U32 pen);

void __stdcall ResetTerrainBlockAllocator (void);

// Gatelinks link jumpgates together and are the "lines" on the map
struct System;

#define DS_HIDDEN  0x00000000
#define DS_NORMAL  0x00000001
#define DS_HIGHLIGHT 0x00000002

struct GateLink
{
	friend struct Sector;

public:
	GateLink(System *A,System *B,GateLink *lastL);
	GateLink(GateLink *lastL);
	~GateLink();
//	BOOL32 Save(IFileSystem *outFile,IFileSystem *file);
//	BOOL32 Load(IFileSystem *inFile,IFileSystem *file);
	BOOL32 SetStateIfYou(U32 startID,U32 finishID,U32 state);
	System *GetOtherSystem(System *thisSystem);
//	U32 GetID();
	void GenerateJumpsXY(System *A,System *B,const Vector &a,const Vector &b, char * jumpArch);
	
	GateLink *next,*last;
	IBaseObject *jump1,*jump2;
	BOOL32 alive;
	
	U32 gateID1,gateID2;
	U32 end_id1,end_id2;

	U32 drawState;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	bool isVisibleToPlayer (U32 playerID, U32 allyMask, int whichEnd);

	bool isInSupply (U32 playerID, U32 allyMask);

	bool isInRootSupply (U32 playerID, U32 allyMask);

	bool isOwnedByAlly (U32 allyMask);

	bool isOwnershipKnownToPlayer (U32 playerID);

private:

//	GATELINK_DATA d;
	System *end1,*end2;


};

// System is the actual individual system
struct System
{

	friend struct Sector;
	
public:
	System (System *lastS);
	~System ();
	void AddLink (GateLink *link);
	void Detach (GateLink *link);
	void GetOrigin (S32 *ox,S32 *oy);
	void GetCenter (S32 *ox,S32 *oy);
	U32 GetID ();
	BOOL32 Save (SYSTEM_DATA *save);
	BOOL32 Load (SYSTEM_DATA *load);
	char *GetName ();
	BOOL32 GetRect (struct tagRECT *rect,bool bAbsolute=0);
	//void SetRect (struct tagRECT *rect);
	void SetSize (S32 _sizeX,S32 _sizeY);
	U32 GetChildren (System **childBuffer,U32 size,U32 playerID);
	U32 GenerateSystemList (U32 *SystemList,System *first);
	void AddShipToSystem (U32 playerMask, U32 playerID, M_OBJCLASS mObjClass);
	void AddPlatformToSystem (U32 playerMask, U32 playerID, M_OBJCLASS mObjClass);
	void AddPlayerAttack (void)
	{
		bPendingAttack = bPlayerAttack = 1;
	}
	void AddNovaBomb (void)
	{
		bPendingNovaBomb = bNovaBomb = 1;
	}
	void SetLightingKit(char * kitName);
	void LoadKit();
	void View ();


	System *next,*last;
	
	System *calledBy;

	U32 playerVisibleMask;
	U32 pendingVisibleMask;

	U32 playerHQVisibleMask;
	U32 pendingHQVisibleMask;

	U32 playerPlatformVisibleMask;
	U32 pendingPlatformVisibleMask;

	bool bPendingAttack, bPlayerAttack;

	bool bPendingNovaBomb, bNovaBomb;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	void getExtent (S32 *x, S32 *y)
	{
		*x = d.sizeX;
		*y = d.sizeY;
	}

	bool isVisibleToPlayer (U32 playerID, U32 allyMask)
	{
		bool result = ((d.alertState[playerID] & S_VISIBLE) != 0);
		if (result==false)
		{
			int i;
			for (i = 1; i <= MAX_PLAYERS; i++)
			{
				if (allyMask & 1)
				{
					if ((result = ((d.alertState[i] & S_VISIBLE) != 0)) != 0)
						break;
				}
				allyMask>>=1;
			}
		}

		return result;
	}

	bool isVisibleToPlayer (U32 playerID)
	{
		return isVisibleToPlayer(playerID, MGlobals::GetAllyMask(playerID) & MGlobals::GetVisibilityMask(playerID));
	}

	bool isInSupply (U32 playerID, U32 allyMask)
	{
		// d.inSupply is shifted up one!?
		return (((d.inSupply >> 1) & allyMask) != 0) || (((d.inRootSupply >> 1) & allyMask) != 0);
	}

	bool isInRootSupply (U32 playerID, U32 allyMask)
	{
		// d.inSupply is shifted up one!?
		return ((d.inRootSupply >> 1) & allyMask) != 0;
	}

private:
	
	SYSTEM_DATA d;
	GateLink *gateLinks[MAX_GATES];
};

bool GateLink::isVisibleToPlayer (U32 playerID, U32 allyMask, int whichEnd)
{
	if (whichEnd==0)
	{
		return ((jump1->GetVisibilityFlags()&allyMask) != 0 && end1->isVisibleToPlayer(playerID, allyMask));
	}
	else
	{
		return ((jump2->GetVisibilityFlags()&allyMask) != 0 && end2->isVisibleToPlayer(playerID, allyMask));
	}
}

bool GateLink::isInSupply (U32 playerID, U32 allyMask)
{
	bool result = (end1->isInSupply(playerID, allyMask) && end2->isInSupply(playerID, allyMask));

	return result;
}

bool GateLink::isInRootSupply (U32 playerID, U32 allyMask)
{
	bool result = (end1->isInRootSupply(playerID, allyMask) && end2->isInRootSupply(playerID, allyMask));

	return result;
}

bool GateLink::isOwnedByAlly (U32 allyMask)
{
	VOLPTR(IJumpGate) ptr = jump1;
	bool result = ptr->IsLocked() && ((1 << (ptr->GetPlayerOwner()-1)) & allyMask) != 0;
	return result;
}

bool GateLink::isOwnershipKnownToPlayer (U32 playerID)
{
	bool result = true;
	VOLPTR(IJumpGate) ptr1 = jump1;
	VOLPTR(IJumpGate) ptr2 = jump2;

	if (ptr1 && ptr2 && ptr1->IsOwnershipKnownToPlayer(playerID)==0 && ptr2->IsOwnershipKnownToPlayer(playerID)==0)
		result = false;

	return result;
}


enum SECTMAP_MODE
{
	SECTMODE_NONE,
	SECTMODE_MOVE,
	SECTMODE_MOVE_ONLY,
	SECTMODE_RALLY,
	SECTMODE_RALLY_ONLY,
	SECTMODE_GOTO,
	SECTMODE_GOTO_CURRENT,  //current system is poked system
	SECTMODE_GOTO_NOWHERE,	// no poked system
	SECTMODE_WORMHOLE		//wormhole creation mode
};


//--------------------------------------------------------------------------//
//  Main Sector Object
//
struct DACOM_NO_VTABLE Sector : public ISector, 
								  IEventCallback,
								  //IResourceClient,
								  DocumentClient,
								  ResourceClient<>,
								  SECTOR_DATA

{
	BEGIN_DACOM_MAP_INBOUND(Sector)
	DACOM_INTERFACE_ENTRY(ISector)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDocumentClient)	
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	END_DACOM_MAP()

	//--------------------------------
	// data items go here
	//--------------------------------
	
	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;

	U32 eventHandle;//, cursorHandle;		// connection handles
	GateLink *firstLink,*lastLink;
	System   *firstSystem,*lastSystem;
	U32 currentSystem;//, pokedSystem;
	System *pokedSystem;
	S32 xExtent,yExtent,xOrg,yOrg;
	BOOL32 bHasFocus;
	bool bLeftButtonValid, bRightButtonValid;
	bool bRallyEnabled;
	int numSystems;
	GateLink *litLink;
	Vector litSpot;
	U32 litPath[MAX_SYSTEMS];
	S32 litSystem;
	U32 pathLength;
	U32 menuID;
	BOOL32 bIgnoreUpdate;
	U32 mapTextureID;
	U32 drawTextureID;
	U32 mapTexW, mapTexH;

	wchar_t toolTip[64];

	//
	// gdi items
	//
	HPEN hAquaPen, hGrayPen, hDimRedPen, hWhitePen, hYellowPen, hRedPen;
	HBRUSH hAquaBrush, hYellowBrush;


	U32 flash;
	bool flashUp;
	U32 updateSectors;

	//table necessary since systemID's can increase without bound
	COMPTR<ITerrainMap> terrainMap[MAX_SYSTEMS+1];
	COMPTR<ITerrainMap> dummyMap;

	SECTMAP_MODE sectmapMode;

	// fancy map tricks
	SINGLE zoneTime,zoneRate;
	Vector zonePos;
	BOOL32 bZoning:1;
	S32 alarmTime[MAX_SYSTEMS];

	//sector effect
	SectorEffects sectorEffect[MAX_PLAYERS][MAX_SYSTEMS];

	//--------------------------------
	// class methods
	//--------------------------------
	
	Sector (void);

	~Sector (void);
    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ISector methods  */
	
	BOOL32 SetCurrentSystem (U32 SystemID);

	U32 GetCurrentSystem (void) const;

	BOOL32 GetTerrainMap (U32 systemID, struct ITerrainMap ** map);

	BOOL32 GetSystemRect (U32 SystemID, struct tagRECT * rect, bool bAbsolute = 0) const;

//	BOOL32 SetSystemRect (U32 SystemID, struct tagRECT * rect);

	BOOL32 GetCurrentRect (struct tagRECT * rect) const;

	void GetDefaultSystemSize (S32 &_sizeX,S32 &_sizeY);

	BOOL32 GetSectorCenter (S32 *x, S32 *y) const;

	BOOL32 SetAlertState (U32 SystemID, U32 alertState, U32 playerID);

	U32 GetAlertState (U32 SystemID, U32 playerID) const;

	S32 GetShortestPath (U32 startSystemID, U32 finishSystemID, U32 * SystemList, U32 playerID) const;

	bool TestPath (U32 startSystemID, U32 finishSystemID, U32 playerID) const;

	bool IsVisibleToPlayer (U32 systemID, U32 playerID) const;

	BOOL32 Load (struct IFileSystem * inFile);

	BOOL32 Save (struct IFileSystem * outFile);

	BOOL32 QuickSave (struct IFileSystem * outFile);

	BOOL32 Close (void);

	BOOL32 New (void);

	struct System *GetPokedSystem (S32 x,S32 y);

	IBaseObject * GetJumpgateDestination (IBaseObject *jump);

	struct IBaseObject *GetJumpgateTo (U32 startSystemID,U32 destSystemID,const Vector &start_pos,U32 playerID);

	struct IBaseObject *GetJumpgateTo (U32 startSystemID,U32 destSystemID,const Vector &start_pos,const Vector &dest_pos,U32 playerID);

	void RegisterJumpgate (IBaseObject *obj,U32 jgateID);

	void Update (void);

	GENRESULT ZoneOn (U8 systemID);

	void HighlightJumpgate (IBaseObject *obj);

	virtual void HighlightMoveSpot (const Vector &pos);

	virtual void ClearMoveSpot ();

	virtual void RevealSystem(U32 systemID, U32 playerID); 

	virtual U32 CreateSystem (U32 xPos, U32 yPos, U32 width, U32 height);

	virtual void CreateJumpGate(U32 systemID1, U32 x1, U32 y1, U32 & id1,U32 systemID2, U32 x2, U32 y2, U32 & id2,char * jumpArch);

	virtual void AddPlatformToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass);

	virtual void AddShipToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass);

	virtual void AddPlayerAttack (U32 systemID);

	virtual void AddNovaBomb(U32 systemID);

	virtual int GetNumSystems()
	{
		return numSystems;
	}

	virtual BOOL32 ResolveJumpgates();

	virtual void SetLightingKit(U32 systemID, char * lightingKit);

	virtual void ViewSystemKit(U32 systemID);

	virtual GT_SYSTEM_KIT GetSystemLightKit(U32 systemID);

	virtual void ComputeSupplyForPlayer(U32 playerID);

	virtual void ComputeSupplyForAllPlayers();

	virtual bool SystemInSupply(U32 systemID, U32 playerID);

	virtual bool SystemInRootSupply(U32 systemID, U32 playerID);

	virtual void RemoveLink(IBaseObject * gate1, IBaseObject * gate2);

	virtual void DeleteSystem(U32 systemID);

	virtual void ReassignSystemID(U32 oldSystemID, U32 newSystemID);

	virtual void GetSystemName(wchar_t * nameBuffer, U32 nameBufferrSize, U32 systemID);

	virtual void GetSystemNameChar (U32 systemID, char * nameBuffer, U32 bufferSize);

	virtual void SetSystemName(U32 systemID, U32 stringID);

//	DEFMETHOD_(BOOL32,EnumSystems) (System *systemList);
	
	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	//Viewer stuff

	BOOL32 createViewer (void);

	void updateViewer (void);

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);


	/* Sector methods  */

	void OnNoOwner (void)
	{
		// do things when no one has ownership
		// e.g. clear all of the icons for each System
	}

	IDAComponent * GetBase (void)
	{
		return (ISector *) this;
	}

	void GetScreenCoords(S32 *mx,S32 *my,S32 wx,S32 wy);

	void Render();

	virtual void DrawLinks(const PANE &tPane,const Transform &trans,SINGLE scale);

	System *GetSystemHandle(U32 SystemID) const;
	
	void RemoveLink(GateLink *link);
	
	void RemoveSystem(System *syst);

	void updateResourceState (SECTMAP_MODE newMode);

	void updateCursorState (S32 x, S32 y);

	void onMoveClick (S32 x, S32 y, WPARAM wParam);

	void loadTextures (bool bEnable);
	
	void loadInterface (IShapeLoader *loader);

	BOOL32 setLineState (U32 startSystemID, U32 finishSystemID, U32 state);

	static void setHostVisibility (U32 systemID, U32 playerID);

	void setSystemInSupply(System * system,U32 playerID);

	void setSystemInRootSupply(System * system,U32 playerID);

	void sendSightingWarning (U32 systemID, U32 playerMask);

	void PrepareTexture (void);

	SectorEffects * GetSectorEffects(U32 playerID, U32 systemID);

	void updateTexture (U32 dt);

	void createGDIObjects (void);

	void destroyGDIObjects (void);

	void setStatus();
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
Sector::Sector (void)
{
	resPriority = SECTOR_PRIORITY;

	xExtent = yExtent = 20;
	xOrg = 0;
	yOrg = 0;
	currentSystem = 0;
	bHasFocus = 1;
	defaultSizeX = defaultSizeY = currentSizeX = currentSizeY = 100000;
	numSystems = 0;

	flash = 255;
	flashUp = false;
	updateSectors = 0;
}
//--------------------------------------------------------------------------//
//
void GetBMPImage(IImageReader **bmpReader,const char *filename)
{
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = filename;
	HANDLE hMapping;
	U8 * pMemory;
	U32 fileSize;
	
	fdesc.lpImplementation = "DOS";
	
	if (TEXTURESDIR->CreateInstance(&fdesc, file) != GR_OK)
		CQBOMB1("Required file ""%s"" not found.", fdesc.lpFileName);

	hMapping = file->CreateFileMapping();
	pMemory = (U8*) file->MapViewOfFile(hMapping);
	fileSize = file->GetFileSize();
	
	CreateBMPReader(bmpReader);
	(*bmpReader)->LoadImage(pMemory+14, fileSize, 0);		// +14 skip bogus header stuff
	
	file->UnmapViewOfFile(pMemory);
	file->CloseHandle(hMapping);
}

struct MyRECT : RECT
{
	MyRECT(LONG _left,LONG _top,LONG _right,LONG _bottom)
	{
		left = _left;
		top = _top;
		right = _right;
		bottom = _bottom;
	}
};

void Sector::loadTextures (bool bEnable)
{
	if (bEnable)
	{	
		M_RACE race = MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());
		const char * name;
		switch (race)
		{
		case M_MANTIS:
			name = "sectormap_BG_M.tga";
			break;
		case M_SOLARIAN:
			name = "sectormap_BG_S.tga";
			break;
		case M_TERRAN:
		default:
			name = "sectormap_BG_T.tga";
			break;
		}

		mapTextureID = TMANAGER->CreateTextureFromFile(name, TEXTURESDIR, DA::TGA, PF_4CC_DAOT);		// create opaque texture
		drawTextureID = 0;
		//
		// create the draw texture
		//
		U32 l;
		if (PIPE->get_texture_dim(mapTextureID, &mapTexW, &mapTexH, &l) == GR_OK)
		{
			PixelFormat pf;
			if (PIPE->get_texture_format(mapTextureID, &pf) == GR_OK)
			{
				if (PIPE->create_texture(SECTOR_TMAP_SIZE, SECTOR_TMAP_SIZE, pf, 1, 0, drawTextureID) == GR_OK)
				{
				}
			}
		}
		createGDIObjects();
	}
	else
	{
		TMANAGER->ReleaseTextureRef(mapTextureID);
		mapTextureID = 0;
		PIPE->destroy_texture(drawTextureID);
		drawTextureID = 0;
		destroyGDIObjects();
	}
}

void Sector::loadInterface(IShapeLoader *loader)
{
	COMPTR<IToolbar> toolbar;
	if (TOOLBAR && TOOLBAR->QueryInterface("IToolbar", toolbar) == GR_OK)
	{
		toolbar->GetSectorMapRect(sector_map_left,sector_map_top,sector_map_right,sector_map_bottom);\
	}

	loadTextures(loader != 0);
	if (loader)
	{	
	}
	else
	{
	}
}
//--------------------------------------------------------------------------//
//
Sector::~Sector (void)
{
	COMPTR<IDAConnectionPoint> connection;

	Close();

	if (TOOLBAR)
	{
		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

#ifdef _DEBUG
	DEFAULTS->SetDataInRegistry(szRegKey, static_cast<SECTOR_DATA *>(this), sizeof(SECTOR_DATA));
#endif

/*	if (CURSOR)
	{
		if (CURSOR->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
			connection->Unadvise(cursorHandle);
	}*/
}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::SetCurrentSystem(U32 systemID)
{
	if (systemID == currentSystem)
		return TRUE;

	RECT sysRect;
	SINGLE rot;

	CQASSERT(systemID != INVALID_SYSTEM_ID);
	
	if (GetSystemRect(systemID,&sysRect))
	{

		currentSizeX = sysRect.right-sysRect.left+1;
		currentSizeY = sysRect.top-sysRect.bottom+1;

		updateViewer();

		System *system;
		
		system = GetSystemHandle(currentSystem);
		
		CAMERA->SnapToTargetRotation();
		
		//save current state in camera
		CAMERA->GetStateInfo(&system->d.cameraBuffer);
		
		//grab world rotation
		rot = system->d.cameraBuffer.worldRotation;

		SINGLE cameraZ = CAMERA->GetPosition().z;
		
		//switch systems
		currentSystem = systemID;	
		EVENTSYS->Send(CQE_SYSTEM_CHANGED, (void*)currentSystem);
		SYSMAP->SetRect(&sysRect);
		
		system = GetSystemHandle(currentSystem);
		
		CAMERA->SetStateInfo(&system->d.cameraBuffer, 0);
		CAMERA->SetOrbitPosition();
		//enforce world rotation
		CAMERA->SetWorldRotation(rot, 0);
		Vector camPos = CAMERA->GetPosition();
		camPos.z = cameraZ;		// restore Z value, assumes pitch is the same across the switch
		CAMERA->SetPosition(&camPos, 0);
		CAMERA->SetLookAtPosition(system->d.cameraBuffer.lookAt);		// maintain same lookat
		
		return TRUE;
	}
	
	return FALSE;
}
//--------------------------------------------------------------------------//
//
U32
Sector::GetCurrentSystem(void) const
{
	return currentSystem;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sector::GetTerrainMap (U32 systemID, struct ITerrainMap ** map)
{
	*map = terrainMap[systemID > MAX_SYSTEMS ? 0 : systemID];

	if (*map)
	{
		(*map)->AddRef();
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::GetSystemRect(U32 SystemID, struct tagRECT * rect,bool bAbsolute) const
{
	System *system = GetSystemHandle(SystemID);
	if (system)
	{
		system->GetRect(rect,bAbsolute);
	
		return TRUE;
	}

	return FALSE;
}

//--------------------------------------------------------------------------//
//
/*BOOL32
Sector::SetSystemRect(U32 SystemID, struct tagRECT * rect)
{
	System *system = GetSystemHandle(SystemID);
	if (system)
	{
		system->SetRect(rect);
	
		return TRUE;
	}

	return FALSE;
}*/

BOOL32 Sector::GetCurrentRect (struct tagRECT * rect) const
{
	System *system = GetSystemHandle(currentSystem);
	if (system)
	{
		system->GetRect(rect);
	
		return TRUE;
	}

	return FALSE;
}
//--------------------------------------------------------------------------//
//
void Sector::GetDefaultSystemSize (S32 &_sizeX,S32 &_sizeY)
{
	_sizeX = defaultSizeX;
	_sizeY = defaultSizeY;
}
//--------------------------------------------------------------------------//
//		
/*BOOL32
Sector::GetSystemCenter(U32 SystemID, S32 *x, S32 *y) const
{
	struct tagRECT * rect;
	
	System *system = GetSystemHandle(SystemID);
	if (system)
	{
		system->GetRect(rect);
	
		*x = (rect.left + rect.right)/2;
		*y = (rect.top + rect.bottom)/2;

		return TRUE;
	}

	return FALSE;
}*/

//--------------------------------------------------------------------------//
//
BOOL32
Sector::GetSectorCenter(S32 *x,S32 *y) const
{
	*x = (xExtent-xOrg) / 2 + xOrg;
	*y = (yExtent-yOrg) / 2 + yOrg;

//	*x = (xExtent) / 2;
//	*y = (yExtent) / 2;
	
	return TRUE;
}
//--------------------------------------------------------------------------//
//
BOOL32 Sector::SetAlertState (U32 SystemID, U32 alertState, U32 playerID)
{
	System *system;
	if ((system = GetSystemHandle(SystemID)) != 0)
	{
		CQASSERT(playerID <= MAX_PLAYERS);
//		if(!(system->d.alertState[playerID] & S_LOCKED))
			system->d.alertState[playerID] = alertState;
		return TRUE;
	}

	return FALSE;
}
//--------------------------------------------------------------------------//
//
U32 Sector::GetAlertState (U32 SystemID, U32 playerID) const
{
	System *system;
	if ((system = GetSystemHandle(SystemID)) !=0)
	{
		CQASSERT(playerID <= MAX_PLAYERS);
		return system->d.alertState[playerID];
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
S32 Sector::GetShortestPath (U32 startSystemID, U32 finishSystemID, U32 * SystemList, U32 playerID) const
{
	CQASSERT(startSystemID && startSystemID <= MAX_SYSTEMS);
	CQASSERT(finishSystemID && finishSystemID <= MAX_SYSTEMS);

	System *first,*finish;
	U32 i,j;
	U32 allyMask;
	if(playerID)
		allyMask = MGlobals::GetAllyMask(playerID) & MGlobals::GetVisibilityMask(playerID);
	else
		allyMask = 0;

	System *tree[10][MAX_SYSTEMS];
	U32 treeSize[10];
	S32 result = -1;

	first = GetSystemHandle(startSystemID);
	CQASSERT(first);
	finish = GetSystemHandle(finishSystemID);
	CQASSERT(finish);

	System *sysPos = firstSystem;
	while (sysPos)
	{
		sysPos->calledBy = 0;
		sysPos = sysPos->next;
	}
	//tree[0] = NULL;
	treeSize[0] = first->GetChildren(tree[0],0,playerID);
	
	for (j=0;j<treeSize[0];j++)
	{
		if (tree[0][j] == finish)
		{
			//generate system list
			result = finish->GenerateSystemList(SystemList,first);

			CQASSERT(SystemList[0]==startSystemID && SystemList[result-1]==finishSystemID);

//			delete tree[0];

			return result;
		}
	}

	for (i=1;i<10;i++)
	{
		treeSize[i] = 0;
	//	tree[i] = NULL;
		
		for (j=0;j<treeSize[i-1];j++)
		{
			//make sure the system isn't hidden unless it's the final destination
			if (tree[i-1][j] == finish || (playerID == 0 || tree[i-1][j]->isVisibleToPlayer(playerID, allyMask)))
			{
				treeSize[i] = tree[i-1][j]->GetChildren(tree[i],treeSize[i],playerID);
			}
		}

		for (j=0;j<treeSize[i];j++)
		{
			if (tree[i][j] == finish)
			{
				//generate system list
				result = finish->GenerateSystemList(SystemList,first);


			/*	//kill tree
				U32 c;

				for (c=0;c<i;c++)
				{
					delete tree[c];
				}*/

				CQASSERT(SystemList[0]==startSystemID && SystemList[result-1]==finishSystemID);

				return result;
			}
		}
	}

/*	U32 c;

	for (c=0;c<i;c++)
	{
		delete tree[c];
	}*/
	
	return result;
}
//--------------------------------------------------------------------------//
//
bool Sector::TestPath (U32 startSystemID, U32 finishSystemID, U32 playerID) const
{
	U32 systemList[MAX_SYSTEMS];

	return (U32(GetShortestPath(startSystemID & ~HYPER_SYSTEM_MASK, finishSystemID & ~HYPER_SYSTEM_MASK, systemList, playerID)) <= MAX_SYSTEMS);
}
//--------------------------------------------------------------------------//
//
bool Sector::IsVisibleToPlayer (U32 systemID, U32 playerID) const
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	return system->isVisibleToPlayer(playerID);
}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::setLineState (U32 startSystemID, U32 finishSystemID, U32 state)
{
	GateLink *linkPos = firstLink;

	while (linkPos)
	{
		linkPos->SetStateIfYou(startSystemID,finishSystemID,state);
		linkPos = linkPos->next;
	}
	
	return FALSE;
}
//--------------------------------------------------------------------------//
//
BOOL32 
Sector::Load (struct IFileSystem * inFile) 
{ 
	MT_SECTOR_SAVELOAD load;

	DAFILEDESC fdesc = "Sector";
	COMPTR<IFileSystem> file;
	U32 result = FALSE;
	U32 dwRead;
	char buffer[4096];

	S32 sx,sy;
	
	xExtent = yExtent = 20;
	xOrg = yOrg = 0x00FFFFFF;

	if (inFile->SetCurrentDirectory("\\MT_SECTOR_SAVELOAD") == 0)
		goto Done;

	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->ReadFile(0,buffer,sizeof(buffer),&dwRead);
	MISSION->CorrelateSymbol("MT_SECTOR_SAVELOAD", buffer, &load);

	numSystems = 0;
	firstSystem = new System(NULL);
	lastSystem = firstSystem;
	
	while (numSystems < MAX_SYSTEMS && lastSystem->Load(&load.sysData[numSystems]))
	{
		numSystems++;
		lastSystem->GetCenter(&sx,&sy);
		if (sx < xOrg)
		{
			xOrg = sx;
		//	yOrg = sx;
		}
		if (sy < yOrg)
		{
		//	xOrg = sy;
			yOrg = sy;
		}
	//	sx += lastSystem->d.sizeX;
	//  sy += lastSystem->d.sizeY;
		if (sx > xExtent)
		{
			xExtent = sx;
		//	yExtent = sx;
		}
		if (sy > yExtent)
		{
			yExtent = sy;
		//	xExtent = sy;
		}
		
		lastSystem->next = new System(lastSystem);
		lastSystem = lastSystem->next;

	}
	file.free();

	//make sector map square and center systems on map
	if (xExtent-xOrg > yExtent-yOrg)
	{
		int diff = ((xExtent-xOrg)-(yExtent-yOrg))/2;
		yOrg -= diff;
		yExtent += diff;
	}
	else
	{
		int diff = ((yExtent-yOrg)-(xExtent-xOrg))/2;
		xOrg -= diff;
		xExtent += diff;
	}

	if (xOrg == xExtent)
	{
		firstSystem->getExtent(&xExtent, &yExtent);
		xOrg = 0;
		yOrg = 0;
	}

	//goofy code
	if (lastSystem != firstSystem)
	{
		lastSystem = lastSystem->last;
		delete lastSystem->next;
		lastSystem->next = NULL;
	}
	else
	{
		delete lastSystem;
		firstSystem = lastSystem = NULL;
	}
	

	/*fdesc.lpFileName = "Links";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	firstLink = new GateLink(NULL);
	lastLink = firstLink;
	
	while (lastLink->Load(inFile,file)) 
	{
		lastLink->next = new GateLink(lastLink);
		lastLink = lastLink->next;
	}
	file.free();

	//dopey code here
	if (lastLink != firstLink)
	{
		lastLink = lastLink->last;
		delete lastLink->next;
		lastLink->next = NULL;
	}
	else
	{
		delete lastLink;
		firstLink = lastLink = NULL;
	}*/

	RECT sysRect;
	System *cur;

/*	fdesc.lpFileName = "State";
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
	{
		currentSystem = firstSystem->GetID();
		cur = firstSystem;
	}
	else
	{
		file->ReadFile(0,&currentSystem,sizeof(U32),&dwRead);
		file.free();

		cur = GetSystemHandle(currentSystem);
	}*/
	currentSystem = load.currentSystem;
	cur = GetSystemHandle(currentSystem);
	if (cur==0)
	{
		currentSystem=1;
		while (cur==0 && currentSystem < MAX_SYSTEMS)
		{
			cur = GetSystemHandle(currentSystem);
			currentSystem++;
		}
	}

	if (cur==0)
	{
		CQERROR0("Mission File Corrupted");
		goto Done;
	}

	EVENTSYS->Send(CQE_SYSTEM_CHANGED, (void*)currentSystem);
	GetSystemRect(currentSystem,&sysRect);
	SYSMAP->SetRect(&sysRect);
	CAMERA->SetStateInfo(&cur->d.cameraBuffer,false);
	currentSizeX = sysRect.right-sysRect.left+1;
	currentSizeY = sysRect.top-sysRect.bottom+1;

	updateViewer();


	System *sysPos;
	sysPos = firstSystem;
	int i;
	for (i=0;i<numSystems;i++)
	{
		RECT rect;
		CQASSERT(sysPos->d.id && sysPos->d.id <= MAX_SYSTEMS);
		CreateTerrainMap(terrainMap[sysPos->d.id]);
		GetSystemRect(sysPos->d.id,&rect);
		terrainMap[sysPos->d.id]->SetWorldRect(rect);
		sysPos = sysPos->next;
	}

	terrainMap[0] = dummyMap;

	result = TRUE;

	OBJMAP->Init();

	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		for(U32 j = 0; j < MAX_SYSTEMS; ++j)
		{
			sectorEffect[i][j].zero();
		}
	}

Done:
	return result;

}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::ResolveJumpgates()
{
	IBaseObject *obj = OBJLIST->GetObjectList();
	while (obj)
	{
		OBJPTR<IJumpGate> jump;
		obj->QueryInterface(IJumpGateID,jump);
		
		if (jump)
		{
			struct JumpgateInfo *info = jump->GetJumpgateInfo();
			GateLink *linkPos = firstLink;
			GateLink *lastLink=0;
			bool bFoundLink=false;
			while (linkPos)
			{
				if (info->gate_id == linkPos->gateID1)
				{
					CQBOMB0("Huh?");
				}
				if (info->gate_id == linkPos->gateID2)
				{
					//finish up link
					linkPos->jump2 = obj;
					linkPos->end_id2 = obj->GetSystemID();
					linkPos->end1 = GetSystemHandle(linkPos->end_id1);
					linkPos->end2 = GetSystemHandle(linkPos->end_id2);
					linkPos->end1->AddLink(linkPos);
					linkPos->end2->AddLink(linkPos);
					bFoundLink=true;
				}
				lastLink = linkPos;
				linkPos = linkPos->next;
			}
			
			//make a link between this jumpgate and one we haven't loaded yet
			if (bFoundLink==false)
			{
				linkPos = new GateLink(lastLink);
				if (lastLink)
					lastLink->next = linkPos;
				else
					firstLink = linkPos;
				linkPos->jump1 = obj;
				linkPos->gateID1 = info->gate_id;
				linkPos->gateID2 = info->exit_gate_id;
				linkPos->end_id1 = obj->GetSystemID();
			}
		}
		
		obj = obj->next;
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
void Sector::SetLightingKit(U32 systemID, char * lightingKit)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	system->SetLightingKit(lightingKit);
}
//--------------------------------------------------------------------------//
//
void Sector::ViewSystemKit(U32 systemID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	system->View();
}
//--------------------------------------------------------------------------//
//
GT_SYSTEM_KIT Sector::GetSystemLightKit(U32 systemID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	GT_SYSTEM_KIT result;

	strcpy(result.fileName,system->d.backgroundName);
	result.numLights = 0;

	IBaseObject * obj = OBJLIST->GetObjectList();

	while(obj)
	{
		if(obj->objClass == OC_LIGHT && obj->GetSystemID() == system->d.id)
		{
			U32 i = result.numLights;
			result.numLights++;
			OBJPTR<IGameLight> gameLight;
			obj->QueryInterface(IGameLightID,gameLight);
			
			gameLight->GetColor(result.lightInfo[i].red,result.lightInfo[i].green,result.lightInfo[i].blue);
			gameLight->GetDirection(result.lightInfo[i].direction,result.lightInfo[i].bAmbient);
			gameLight->GetName(result.lightInfo[i].name);
			Vector tempPos = obj->GetPosition();
			result.lightInfo[i].position.x = tempPos.x/system->d.sizeX;
			result.lightInfo[i].position.y = tempPos.y/system->d.sizeY;
			result.lightInfo[i].position.z = tempPos.z;
			gameLight->GetRange(result.lightInfo[i].range,result.lightInfo[i].bInfinite);
		}
		obj = obj->next; 
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
void Sector::setSystemInSupply(System * system,U32 playerID)
{
	system->d.inSupply |= (0x00000001 << playerID);

	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetPlayerID() && MGlobals::AreAllies(obj->GetPlayerID(),playerID) && obj->objClass == OC_PLATFORM && obj->GetSystemID() == system->d.id)
		{
			MPart part(obj);
			if(part->bReady)
			{
				OBJPTR<IJumpPlat> jumpPlat;
				obj->QueryInterface(IJumpPlatID,jumpPlat);
				if(jumpPlat)
				{
					OBJPTR<IPlatform> platform;
					obj->QueryInterface(IPlatformID,platform);
					if (!platform->IsReallyDead())
					{
						IBaseObject * sibling = jumpPlat->GetSibling();
						
						if (sibling)
						{
							System * system = GetSystemHandle(sibling->GetSystemID());
							if(!((system->d.inSupply >> playerID) & 0x00000001))
								setSystemInSupply(system,playerID);				
						}
					}
				}
			}
		}
		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::setSystemInRootSupply(System * system,U32 playerID)
{
	system->d.inRootSupply |= (0x00000001 << playerID);

	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetPlayerID() && MGlobals::AreAllies(obj->GetPlayerID(),playerID) && obj->objClass == OC_PLATFORM && obj->GetSystemID() == system->d.id)
		{
			MPart part(obj);
			if(part->bReady)
			{
				OBJPTR<IJumpPlat> jumpPlat;
				obj->QueryInterface(IJumpPlatID,jumpPlat);
				if(jumpPlat)
				{
					OBJPTR<IPlatform> platform;
					obj->QueryInterface(IPlatformID,platform);
					if (!platform->IsReallyDead())
					{
						IBaseObject * sibling = jumpPlat->GetSibling();
						
						if (sibling)
						{
							System * system = GetSystemHandle(sibling->GetSystemID());
							if(!((system->d.inRootSupply >> playerID) & 0x00000001))
								setSystemInRootSupply(system,playerID);				
						}
					}
				}
			}
		}
		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::ComputeSupplyForPlayer(U32 playerID)
{
	U32 testSupply[MAX_SYSTEMS];
	U32 count = 0;
	System *sysPos = firstSystem;
	while(sysPos)
	{
		testSupply[count] = sysPos->d.inRootSupply & (0x00000001 << playerID);
		sysPos->d.inSupply &= (~(0x00000001 << playerID));
		sysPos->d.inRootSupply &= (~(0x00000001 << playerID));
		sysPos = sysPos->next;
		++count;
	}

	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetPlayerID() && MGlobals::AreAllies(obj->GetPlayerID(), playerID) && obj->objClass == OC_PLATFORM)
		{
			MPart part(obj);
			if(part->bReady)
			{
				OBJPTR<IPlatform> platform;
				obj->QueryInterface(IPlatformID,platform);
				if(platform->IsRootSupply() && (!platform->IsReallyDead()))
				{
					System * system = GetSystemHandle(obj->GetSystemID());
					if(!((system->d.inRootSupply >> playerID) & 0x00000001))
						setSystemInRootSupply(system,playerID);
				}
				if(platform->IsTempSupply() && (!platform->IsReallyDead()))
				{
					System * system = GetSystemHandle(obj->GetSystemID());
					if(!((system->d.inSupply >> playerID) & 0x00000001))
						setSystemInSupply(system,playerID);
				}
			}
		}
		obj = obj->nextTarget;
	}

	if(playerID == MGlobals::GetThisPlayer() && MGlobals::GetUpdateCount() > 32 && CQFLAGS.bGameActive && CQFLAGS.bInsidePlayerResign==0)
	{
		count = 0;
		sysPos = firstSystem;
		while(sysPos )
		{
			if(sysPos->isVisibleToPlayer(MGlobals::GetThisPlayer()))
			{
				if((sysPos->d.inRootSupply & (0x00000001 << playerID)) && (!testSupply[count]))
				{
					M_RACE race = MGlobals::GetPlayerRace(playerID);
					if(race == M_TERRAN)
						SOUNDMANAGER->PlayCommMessage("T9990.wav");
					else if(race == M_MANTIS)
						SOUNDMANAGER->PlayCommMessage("M9990.wav");
					else if(race == M_SOLARIAN)
						SOUNDMANAGER->PlayCommMessage("C9990.wav");
					SECTOR->ZoneOn(sysPos->d.id);
					return;//only want one sound
				}
				else if((!(sysPos->d.inRootSupply & (0x00000001 << playerID))) && testSupply[count])
				{
					M_RACE race = MGlobals::GetPlayerRace(playerID);
					if(race == M_TERRAN)
						SOUNDMANAGER->PlayCommMessage("T9991.wav");
					else if(race == M_MANTIS)
						SOUNDMANAGER->PlayCommMessage("M9991.wav");
					else if(race == M_SOLARIAN)
						SOUNDMANAGER->PlayCommMessage("C9991.wav");
					SECTOR->ZoneOn(sysPos->d.id);
					return;// only want one sound
				}
			}
			sysPos = sysPos->next;
			++count;
		}
	}

}
//--------------------------------------------------------------------------//
//
void Sector::ComputeSupplyForAllPlayers()
{
	for(U32 i = 1; i <=MAX_PLAYERS; ++i)
	{
		ComputeSupplyForPlayer(i);
	}
}
//--------------------------------------------------------------------------//
//
bool Sector::SystemInSupply(U32 systemID, U32 playerID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	return system->isInSupply(playerID, MGlobals::GetAllyMask(playerID));
}
//--------------------------------------------------------------------------//
//
bool Sector::SystemInRootSupply(U32 systemID, U32 playerID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	return system->isInRootSupply(playerID, MGlobals::GetAllyMask(playerID));
}
//--------------------------------------------------------------------------//
//
void Sector::RemoveLink(IBaseObject * gate1, IBaseObject * gate2)
{
	GateLink * link = firstLink;
	while(link->jump1 != gate1 && link->jump2 != gate1)
	{
		link = link->next;
	}
	delete link;
	delete gate1;
	delete gate2;
}
//--------------------------------------------------------------------------//
//
void Sector::DeleteSystem(U32 systemID)
{
	System * sys = firstSystem;
	while(sys)
	{
		if(sys->d.id == systemID)
		{
			delete sys;
			return;
		}
		sys=sys->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::ReassignSystemID(U32 oldSystemID, U32 newSystemID)
{
	System * sys = firstSystem;
	while(sys)
	{
		if(sys->d.id == oldSystemID)
		{
			sys->d.id = newSystemID;
			for(U32 i = 0; i < MAX_GATES; ++i)
			{
				if(sys->gateLinks[i])
				{
					if(sys->gateLinks[i]->end_id1 == oldSystemID)
						sys->gateLinks[i]->end_id1 = newSystemID;
					if(sys->gateLinks[i]->end_id2 == oldSystemID)
						sys->gateLinks[i]->end_id2 = newSystemID;
				}
			}
			return;
		}
		sys = sys->next;
	}
	terrainMap[newSystemID] = terrainMap[oldSystemID];
}
//--------------------------------------------------------------------------//
//
void Sector::GetSystemName(wchar_t * nameBuffer, U32 nameBufferSize, U32 systemID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	char * name = system->GetName();
	if (name[0])
		_localAnsiToWide(system->GetName(), nameBuffer, nameBufferSize);
	else
		wcsncpy(nameBuffer, L"BUGBUG::Uninitialized System Name", nameBufferSize/sizeof(wchar_t));

}
//--------------------------------------------------------------------------//
//
void Sector::GetSystemNameChar (U32 systemID, char * nameBuffer, U32 bufferSize)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	char * name = system->GetName();
	strncpy(nameBuffer,name,bufferSize);
}
//--------------------------------------------------------------------------//
//
void Sector::SetSystemName (U32 systemID, U32 stringID)
{
	System * system = GetSystemHandle(systemID);
	CQASSERT(system);
	strncpy(system->d.name,_localLoadString(stringID), sizeof(system->d.name) - 1);
}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::Save(IFileSystem *outFile)
{
	MT_SECTOR_SAVELOAD save;
	memset(&save,0,sizeof(MT_SECTOR_SAVELOAD));

	System *systemPos = firstSystem;
	U32 dwWritten;

	COMPTR<IFileSystem> file;

	DAFILEDESC fdesc = "test.utf";

	outFile->CreateDirectory("\\MT_SECTOR_SAVELOAD");
	
	if (outFile->SetCurrentDirectory("\\MT_SECTOR_SAVELOAD") == 0)
		goto Done;

	RecursiveDelete(outFile);

//	fdesc = "State";
	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	//snap to target rotation
	CAMERA->SnapToTargetRotation();

	//store camera rotation	& starfield
	System * cur;
	cur = GetSystemHandle(currentSystem);
	CAMERA->GetStateInfo(&cur->d.cameraBuffer);

	/*if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;
	
	file->WriteFile(0,&currentSystem,sizeof(U32),&dwWritten);
	file.free();*/

	save.currentSystem = currentSystem;
	int i;
	i=0;
	while (systemPos)
	{
		systemPos->Save(&save.sysData[i]);
		systemPos = systemPos->next;
		i++;
	}

	fdesc.lpFileName = "Sector";
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	file->WriteFile(0,&save,sizeof(MT_SECTOR_SAVELOAD),&dwWritten);

//	file.free();

/*	GateLink *linkPos = firstLink;
	fdesc.lpFileName = "Links";
	if (outFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	while (linkPos)
	{
		linkPos->Save(outFile,file);
		linkPos = linkPos->next;
	}
	file.free();*/



	return TRUE;
	
Done:
	return FALSE;

}
//--------------------------------------------------------------------------//
//
BOOL32 Sector::QuickSave (struct IFileSystem * outFile)
{
	return Save(outFile);
}
//--------------------------------------------------------------------------//
//
BOOL32
Sector::Close()
{
	GateLink *linkPos;
	System *SystemPos;

	while (firstLink)
	{
		linkPos = firstLink->next;
		delete firstLink;
		firstLink = linkPos;
	}

	while (firstSystem)
	{
		SystemPos = firstSystem->next;
		delete firstSystem;
		firstSystem = SystemPos;
	}

	numSystems = 0;

	int i;
	for (i = 1; i <= MAX_SYSTEMS; i++)
		terrainMap[i] = dummyMap;

	ResetTerrainBlockAllocator();
	
	if (OBJMAP)
		OBJMAP->Uninit();

	return TRUE;
}
//--------------------------------------------------------------------------//
// re-initialize for a default sector
//
BOOL32
Sector::New (void)
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		for(U32 j = 0; j < MAX_SYSTEMS; ++j)
		{
			sectorEffect[i][j].zero();
		}
	}
	currentSystem = 1;
	firstSystem = lastSystem = new System(NULL);
	firstSystem->d.id = 1;
	firstSystem->getExtent(&xExtent, &yExtent);
	xOrg = yOrg = 0;

	currentSizeX = xExtent;
	currentSizeY = yExtent;

	updateViewer();

	numSystems = 1;
	CreateTerrainMap(terrainMap[1]);
	terrainMap[0] = dummyMap;
	
	RECT sysRect;
	if (GetSystemRect(1,&sysRect))
	{
		terrainMap[1]->SetWorldRect(sysRect);
	//	CAMERA->SetStateInfo(&firstSystem->d.cameraBuffer);
		
		//switch systems
		SYSMAP->SetRect(&sysRect);
		
		System *system = GetSystemHandle(currentSystem);
		
		CAMERA->SetStateInfo(&system->d.cameraBuffer,false);
		CAMERA->SetOrbitPosition();
	}

	EVENTSYS->Send(CQE_SYSTEM_CHANGED, (void*)currentSystem);

	OBJMAP->Init();

	return TRUE;
}
//--------------------------------------------------------------------------//
//

#define POKED_SYS_SIZE 5

System *
Sector::GetPokedSystem(S32 x,S32 y)
{
	S32 ox,oy;
	Vector pos;
	System *sysPos = firstSystem;

	const Transform *trans = CAMERA->GetInverseSectorTransform();

	const bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
	const bool bSpectate = DEFAULTS->GetDefaults()->bSpectatorModeOn;
	const U32 playerID = MGlobals::GetThisPlayer();
	const U32 allyMask = MGlobals::GetAllyMask(playerID) & MGlobals::GetVisibilityMask(playerID);

	while (sysPos)
	{
		if (editMode || bSpectate || sysPos->isVisibleToPlayer(playerID, allyMask))
		{
			sysPos->GetCenter(&ox,&oy);
			pos.set(ox,oy,0);
			pos = trans->rotate_translate(pos);
			
			GetScreenCoords(&ox,&oy,pos.x,pos.y);
			
			if (x >= S32(IDEAL2REALX(ox-POKED_SYS_SIZE)) && x <= S32(IDEAL2REALX(ox+POKED_SYS_SIZE)))
				if (y >= S32(IDEAL2REALY(oy-POKED_SYS_SIZE)) && y <= S32(IDEAL2REALY(oy+POKED_SYS_SIZE)))
				{
					U32 poked = sysPos->GetID();
					litSystem = poked;
					litSpot.set(0,0,0);
					return sysPos;
				}
		}

		sysPos = sysPos->next;
	}

	litSystem = INVALID_SYSTEM_ID;

	return 0;
}
//--------------------------------------------------------------------------//
//
GENRESULT
Sector::Notify(U32 message, void *param)
{
	MSG * msg = (MSG *)param;
	
	switch (message)
	{
	case CQE_UI_RALLYPOINT:
		bRallyEnabled = (param != 0);
		break;

	case CQE_UPDATE:
		if (CQFLAGS.bGameActive && mapTextureID && drawTextureID && CQFLAGS.bNoToolbar==0)
			updateTexture(U32(param) >> 10);
		litSystem = INVALID_SYSTEM_ID;
		litLink = 0;
		break;

	case CQE_DEBUG_HOTKEY:
		if ((U32)param == IDH_VIEW_SYSTEM_KIT)
		{
			ViewSystemKit(GetCurrentSystem());
		}
		break;
	case CQE_LOAD_INTERFACE:
		loadInterface((IShapeLoader *)param);
		break;
	case CQE_KILL_FOCUS:
		bHasFocus = 0;
		pokedSystem = 0;
	//	litSystem = INVALID_SYSTEM_ID;
		desiredOwnedFlags = 0;
		releaseResources();
		break;
	case CQE_SET_FOCUS:
		{
			bHasFocus = 1;
		}
		break;

	case CQE_RENDER_LAST3D:
		{
			S32 x, y;
			WM->GetCursorPos(x, y);
			updateCursorState(x, y);
		}
		if (bZoning)
		{
			zoneTime += (S32)param*zoneRate;
			if (zoneTime > 2.0)
				bZoning = FALSE;
		}
		int i;
		for (i=0;i<MAX_SYSTEMS;i++)
		{
			alarmTime[i] -= (S32)param/(1e6*ELAPSED_TIME);
		}
		if (CQFLAGS.bNoToolbar==0)
		{
			Render();
		}
		break;

	case WM_RBUTTONDOWN:
		if (bHasFocus && sectmapMode != SECTMODE_NONE && ownsResources())
			bRightButtonValid = true;
		break;

	case WM_RBUTTONUP:
		if (bHasFocus && ownsResources() && bRightButtonValid)
		{
			// use left-click model in rally mode
			if (DEFAULTS->GetDefaults()->bRightClickOption == false || sectmapMode == SECTMODE_RALLY || sectmapMode == SECTMODE_RALLY_ONLY)
			{
				System *sys = GetPokedSystem(LOWORD(msg->lParam), HIWORD(msg->lParam));

				if ((sys != NULL) && sys->d.id != currentSystem)
				{
					const bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
					const bool bSpectate = DEFAULTS->GetDefaults()->bSpectatorModeOn;

					if (editMode || bSpectate || sys->isVisibleToPlayer(MGlobals::GetThisPlayer()))
						SetCurrentSystem(sys->d.id);
				}
			}
			else
			{
				onMoveClick(LOWORD(msg->lParam), HIWORD(msg->lParam), msg->wParam);
			}
		}
		bRightButtonValid = false;
		break;

	case WM_LBUTTONDOWN:
		if (bHasFocus && sectmapMode != SECTMODE_NONE && ownsResources())
			bLeftButtonValid = true;
		break;

	case WM_LBUTTONUP:
		if (bHasFocus && ownsResources() && bLeftButtonValid)
		{
			// use left-click model in rally mode
			if (DEFAULTS->GetDefaults()->bRightClickOption == false || sectmapMode == SECTMODE_RALLY || sectmapMode == SECTMODE_RALLY_ONLY)
			{
				onMoveClick(LOWORD(msg->lParam), HIWORD(msg->lParam), msg->wParam);
			}
			else
			{
				// right-click model
				System *sys = GetPokedSystem(LOWORD(msg->lParam), HIWORD(msg->lParam));

				if ((sys != NULL) && sys->d.id != currentSystem)
				{
					const bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
					const bool bSpectate = DEFAULTS->GetDefaults()->bSpectatorModeOn;

					if (editMode || bSpectate || sys->isVisibleToPlayer(MGlobals::GetThisPlayer()))
						SetCurrentSystem(sys->d.id);
				}
			}
		}
		bLeftButtonValid = false;
		break;

	case CQE_CAMERA_MOVED:
		if (bHasFocus)
		{
			S32 x,y;
			WM->GetCursorPos(x,y);
			pokedSystem = GetPokedSystem(x,y);
		}
		break;

	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			updateViewer();
			
			if (viewer)
				viewer->set_display_state(1);
		}
		else if (LOWORD(msg->wParam) == IDM_RESET_VISIBILITY)
		{
			//clear revealed systems
			System *sys = firstSystem;
			while (sys)
			{
				for (int i=0;i<MAX_PLAYERS+1;i++)
					sys->d.alertState[i] &= ~S_VISIBLE;
				sys = sys->next;
			}

			//reset ship bits and
			//reveal systems with ships in them
			IBaseObject *obj = OBJLIST->GetObjectList();
			while (obj)
			{
				if(obj->GetPlayerID())
					RevealSystem(obj->GetSystemID(), obj->GetPlayerID());
			
				obj->ClearVisibilityFlags();

				obj = obj->next;
			}
		}
		break;

	case WM_CLOSE:
		if (viewer)
			viewer->set_display_state(0);
		break;
	}

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
BOOL32 Sector::createViewer (void)
{
	if (CQFLAGS.bNoGDI)
		return 1;

	DACOMDESC desc = "IViewConstructor";
	DOCDESC ddesc;

	ddesc.memory = static_cast<SECTOR_DATA *>(this);
	ddesc.memoryLength = sizeof(SECTOR_DATA);

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "SECTOR_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);
			MoveWindow(hwnd, 120, 120, 400, 200, 1);

			viewer->set_instance_name("Sector");

			MakeConnection(doc);
		}
	}

	return (viewer != 0);
}

//--------------------------------------------------------------------------//
//
void Sector::updateViewer (void)
{
	DWORD dwWritten;

	bIgnoreUpdate++;
	
	if (doc)
	{
		doc->SetFilePointer(0,0);
		doc->WriteFile(0, static_cast<SECTOR_DATA *>(this), sizeof(SECTOR_DATA), &dwWritten, 0);
		doc->UpdateAllClients(0);
	}
	//don't think I need to update system size here

	bIgnoreUpdate--;
}

//--------------------------------------------------------------------------//
// receive notifications that document has changed
//
GENRESULT Sector::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	if (bIgnoreUpdate == 0)
	{
		doc->SetFilePointer(0,0);
		doc->ReadFile(0, static_cast<SECTOR_DATA *>(this), sizeof(SECTOR_DATA), &dwRead, 0);
		System *sys = GetSystemHandle(currentSystem);
		if (sys)
		{
			sys->SetSize(currentSizeX,currentSizeY);
			tagRECT rect;
			sys->GetRect(&rect);
			SYSMAP->SetRect(&rect);
		}
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void Sector::updateResourceState (SECTMAP_MODE newMode)
{
	switch (newMode)
	{
	case SECTMODE_RALLY_ONLY:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		statusTextID = IDS_SYSMAPRALLYONLY;
		cursorID = IDC_CURSOR_MICRO_RALLY;
		if (ownsResources())
			setResources();
		else
			grabAllResources();
		break;
	case SECTMODE_MOVE_ONLY:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (DEFAULTS->GetDefaults()->bRightClickOption)
		{
			statusTextID = IDS_SYSMAPMOVEONLY_R;
			cursorID = IDC_CURSOR_MICRO_MOVE;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		else
		{
			statusTextID = IDS_SYSMAPMOVEONLY;
			cursorID = IDC_CURSOR_MICRO_MOVE;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SECTMODE_RALLY:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		statusTextID = IDS_SYSMAPRALLY;
		cursorID = IDC_CURSOR_MICRO_RALLY;
		if (ownsResources())
			setResources();
		else
			grabAllResources();
		break;

	case SECTMODE_MOVE:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (DEFAULTS->GetDefaults()->bRightClickOption)
		{
			statusTextID = IDS_SYSMAPMOVE_R;
			cursorID = IDC_CURSOR_MICRO_MOVE;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		else
		{
			statusTextID = IDS_SYSMAPMOVE;
			cursorID = IDC_CURSOR_MICRO_MOVE;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SECTMODE_GOTO:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		if (DEFAULTS->GetDefaults()->bRightClickOption == true)
			statusTextID = IDS_SECTORMAPGOTO_R;
		else
			statusTextID = IDS_SECTORMAPGOTO;
		cursorID = IDC_CURSOR_MICRO_GOTO;
		if (ownsResources())
			setResources();
		else
			grabAllResources();
		break;

	case SECTMODE_GOTO_CURRENT:
		{
			desiredOwnedFlags = RF_CURSOR | RF_STATUS;
			statusTextID = 0;
			cursorID = IDC_CURSOR_MICRO_GOTO;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SECTMODE_GOTO_NOWHERE:
		desiredOwnedFlags = RF_CURSOR | RF_STATUS;
		statusTextID = 0;
		cursorID = IDC_CURSOR_MICRO_GOTO;
		if (ownsResources())
			setResources();
		else
			grabAllResources();
		break;

	case SECTMODE_WORMHOLE:
		{
			desiredOwnedFlags = RF_CURSOR | RF_STATUS;
			statusTextID = 0;
			cursorID = IDC_CURSOR_MICRO_WORMHOLE;
			if (ownsResources())
				setResources();
			else
				grabAllResources();
		}
		break;

	case SECTMODE_NONE:
		desiredOwnedFlags = 0;
	//	litSystem = INVALID_SYSTEM_ID;
		pokedSystem = NULL;
		toolTip[0] = 0;
		releaseResources();
		break;
	};

	sectmapMode = newMode;
}
//--------------------------------------------------------------------------//
//
void Sector::updateCursorState (S32 x, S32 y)
{
	SECTMAP_MODE newMode=SECTMODE_NONE;

	if (bHasFocus)
	{
		if (x >= S32(IDEAL2REALX(MAP_ORIGIN_X)) && x <= S32(IDEAL2REALX(MAP_EXTENT_X)) && 
			y >= S32(IDEAL2REALY(MAP_ORIGIN_Y)) && y <= S32(IDEAL2REALY(MAP_EXTENT_Y)))
		{
			pokedSystem = GetPokedSystem(x,y);
			if(pokedSystem!= INVALID_SYSTEM_ID)
			{
				if (pokedSystem->d.name[0])
					_localAnsiToWide(pokedSystem->d.name, toolTip, sizeof(toolTip));
				else
					wcscpy(toolTip, L"BUGBUG::Uninitialized System Name");
			}
			else
			{
				toolTip[0] = 0;
			}
			const U32 playerID = MGlobals::GetThisPlayer();
			MPart ship;
			MPart jumpShip;		// ship that can jump

			if (CQFLAGS.bGamePaused==0 && pokedSystem)
			{
				ship = OBJLIST->GetSelectedList();
				while (ship.isValid())
				{
					if (ship->caps.jumpOk)
					{
						if (ship->systemID != pokedSystem->GetID() && TestPath(ship->systemID, pokedSystem->GetID(), playerID))
							jumpShip = ship;
					}
					if ((ship.obj->objClass == OC_PLATFORM) && ship->caps.buildOk)
					{
						break;
					}
					else if(ship->caps.createWormholeOk)
					{
						break;
					}
					ship = ship.obj->nextSelected;
				}
			}

			if (pokedSystem != 0 && pokedSystem->GetID() != currentSystem)
				newMode = SECTMODE_GOTO;
			else if(pokedSystem != 0 && pokedSystem->GetID() == currentSystem)
				newMode = SECTMODE_GOTO_CURRENT;
			else
				newMode = SECTMODE_GOTO_NOWHERE;
			if(ship.isValid() && ship->caps.createWormholeOk)
			{
				if(pokedSystem)
					newMode = SECTMODE_WORMHOLE;
				else
					newMode = SECTMODE_GOTO_NOWHERE;
			}
			else 
			if (jumpShip.isValid())
			{
				if (pokedSystem->GetID() != currentSystem)
					newMode = SECTMODE_MOVE;
				else
					newMode = SECTMODE_MOVE_ONLY;
			}
			else
			if (bRallyEnabled && ship.isValid())
			{
				if (pokedSystem->GetID() != currentSystem)
					newMode = SECTMODE_RALLY;
				else
					newMode = SECTMODE_RALLY_ONLY;
			}
		}
		else
		{
			newMode = SECTMODE_NONE;
		}
	}

	updateResourceState(newMode);
}
//--------------------------------------------------------------------------//
//
void Sector::onMoveClick (S32 x, S32 y, WPARAM wParam)
{
	if ((sectmapMode == SECTMODE_MOVE||sectmapMode==SECTMODE_MOVE_ONLY) && ownsResources())
	{
		System * sys = GetPokedSystem(x,y);

		if (sys && sys->d.id != INVALID_SYSTEM_ID)
		{
			if (sys->isVisibleToPlayer(MGlobals::GetThisPlayer())==false)
				return;

			RECT rect;
			Vector vec;
			NETGRIDVECTOR netvector;

			sys->GetRect(&rect);
			vec.x = rect.right/2;
			vec.y = rect.top / 2;
			vec.z = 0;

			netvector.init(vec,sys->d.id);

			if(wParam & MK_CONTROL)
			{
				EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_SQMOVE_SELECTED_UNITS : CQE_SMOVE_SELECTED_UNITS, &netvector);
			}
			else
			{
				EVENTSYS->Send((wParam & MK_SHIFT) ? CQE_QMOVE_SELECTED_UNITS : CQE_MOVE_SELECTED_UNITS, &netvector);
			}
		}
	}
	else
	if ((sectmapMode == SECTMODE_RALLY||sectmapMode==SECTMODE_RALLY_ONLY) && ownsResources())
	{
		System * sys = GetPokedSystem(x,y);

		if (sys->d.id != INVALID_SYSTEM_ID)
		{
			if (sys->isVisibleToPlayer(MGlobals::GetThisPlayer())==false)
				return;

			RECT rect;
			Vector vec;
			NETGRIDVECTOR netvector;

			sys->GetRect(&rect);
			vec.x = rect.right/2;
			vec.y = rect.top / 2;
			vec.z = 0;

			netvector.init(vec, sys->d.id);

			EVENTSYS->Send(CQE_SET_RALLY_POINT, &netvector);
		}
	}
	else
	if((sectmapMode == SECTMODE_WORMHOLE) && ownsResources())
	{
		System * sys = GetPokedSystem(x,y);
		if (sys->d.id != INVALID_SYSTEM_ID)
		{
			if (sys->isVisibleToPlayer(MGlobals::GetThisPlayer())==false)
				return;
			IBaseObject * selected = OBJLIST->GetSelectedList();
			if(selected)
			{
				MPart selPart(selected);
				if(selPart.isValid() && selPart->caps.createWormholeOk)
				{
					USR_PACKET<USRCREATEWORMHOLE> packet;
					packet.objectID[0] = selPart->dwMissionID;
					packet.systemID = sys->d.id;
					packet.init(1,false);
					NETPACKET->Send(HOSTID, 0, &packet);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Sector::GetScreenCoords(S32 *mx,S32 *my,S32 wx,S32 wy)
{
	S32 xCenter = (xExtent+xOrg)*0.5;
	S32 yCenter = (yExtent+yOrg)*0.5;

	//find map coordinates and draw out for the center, leaving room for a circular path
	//18 is the buffer for the selection brackets - it seems to be slightly off
//	*mx = MAP_CENTER_X+0.707*((wx-xCenter)*(MAP_SIZE_X-18)/(xExtent-xOrg));
//	*my = MAP_CENTER_Y-0.707*((wy-yCenter)*(MAP_SIZE_Y-18)/(yExtent-yOrg));

	*mx = MAP_CENTER_X+SCALE_FACTOR*((wx-xCenter)*(MAP_SIZE_X)/(xExtent-xOrg));
	*my = MAP_CENTER_Y-SCALE_FACTOR*((wy-yCenter)*(MAP_SIZE_Y)/(yExtent-yOrg));

}

void Sector::Render()
{
	BATCH->set_state(RPR_BATCH,false);
	BATCH->set_render_state(D3DRS_CULLMODE,D3DCULL_NONE);
	BATCH->set_render_state(D3DRS_ZWRITEENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ZENABLE,FALSE);
	BATCH->set_render_state(D3DRS_ALPHABLENDENABLE,TRUE);
	BATCH->set_render_state(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	BATCH->set_render_state(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);

	// draw the background texture
	/*
	PANE pane;
	pane.window = 0;
	pane.x0 = IDEAL2REALX(MAP_ORIGIN_X);
	pane.x1 = IDEAL2REALX(MAP_ORIGIN_X + MAP_SIZE_X - 1);
	pane.y0 = IDEAL2REALY(MAP_ORIGIN_Y);
	pane.y1 = IDEAL2REALY(MAP_ORIGIN_Y + MAP_SIZE_Y - 1);
	*/

	Vector v[4];
	v[0].set(0,0,0);
	v[1].set(MAP_SIZE_X,0,0);
	v[2].set(MAP_SIZE_X,MAP_SIZE_Y,0);
	v[3].set(0,MAP_SIZE_Y,0);

	SINGLE rot;
	if (DEFAULTS->GetDefaults()->bSectormapRotates)
		rot = -CAMERA->GetWorldRotation()*PI/180.0;
	else
		rot = 0;

	TRANSFORM trans;
	trans.translation.x -= MAP_SIZE_X/2;
	trans.translation.y -= MAP_SIZE_Y/2;

	SINGLE cosine, sine;

	cosine = cos(rot);
	sine = sin(rot);

	trans.d[0][0] = (cosine);
	trans.d[1][0] = (sine);
	trans.d[2][0] = 0;

	trans.d[0][1] = - (sine);
	trans.d[1][1] = (cosine);
	trans.d[2][1] = 0;

	SINGLE transx = trans.translation.x;
	trans.translation.x = trans.translation.x*cosine-trans.translation.y*sine;
	trans.translation.y = transx*sine+trans.translation.y*cosine;

	trans.translation.x += 0.5*MAP_SIZE_X+MAP_ORIGIN_X;
	trans.translation.y += 0.5*MAP_SIZE_Y+MAP_ORIGIN_Y;

	for (int i=0;i<4;i++)
	{
		v[i] = trans.rotate_translate(v[i]);
		v[i].x = IDEAL2REALX(v[i].x);
		v[i].y = IDEAL2REALY(v[i].y);
	}

//	OrthoView(&pane);
	OrthoView(NULL);
  	SetupDiffuseBlend(drawTextureID,TRUE);

	PB.Color4ub(255,255,255, 255);
	PB.Begin(PB_QUADS);
	PB.TexCoord2f(0, 0);    PB.Vertex3f(v[0].x, v[0].y, 0);
	PB.TexCoord2f(1, 0);	PB.Vertex3f(v[1].x, v[1].y, 0);
	PB.TexCoord2f(1, 1);	PB.Vertex3f(v[2].x, v[2].y, 0);
	PB.TexCoord2f(0, 1);	PB.Vertex3f(v[3].x, v[3].y, 0);
	PB.End();
}
//--------------------------------------------------------------------------//
//
void Sector::DrawLinks(const PANE &tPane,const Transform &trans,SINGLE scale)
{
	GateLink *linkPos = firstLink;
	
	const bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
	const bool bSpectate = DEFAULTS->GetDefaults()->bSpectatorModeOn;
	
	const U32 playerID = MGlobals::GetThisPlayer();
	const U32 allyMask = MGlobals::GetAllyMask(playerID) & MGlobals::GetVisibilityMask(playerID);
	
	S32 ox,oy;
	S32 px,py;
	Vector pos1;

	while (linkPos)
	{
		if (linkPos == litLink)
			linkPos->drawState = DS_HIGHLIGHT;

		if (editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, allyMask, 0) || linkPos->isVisibleToPlayer(playerID, allyMask, 1))
		{
			int size1 = linkPos->end1->d.sizeX*scale;
			int size2 = linkPos->end2->d.sizeX*scale;
			linkPos->end1->GetCenter(&ox,&oy);
			linkPos->end2->GetCenter(&px,&py);
			ox -= size1/2;
			oy -= size1/2;
			pos1.set(ox,oy,0);
			pos1 += linkPos->jump1->GetPosition()*scale;
			pos1 = trans.rotate_translate(pos1);
		//	GetScreenCoords(&ox,&oy,pos1.x,pos1.y);
			ox = pos1.x;
			oy = pos1.y;

			px -= size2/2;
			py -= size2/2;
			pos1.set(px,py,0);
			pos1 += linkPos->jump2->GetPosition()*scale;
			pos1 = trans.rotate_translate(pos1);
			px = pos1.x;
			py = pos1.y;
		//	GetScreenCoords(&px,&py,pos1.x,pos1.y);
			if (!(editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, allyMask, 1)))
			{
				px -= (px - ox)/2;
				py -= (py - oy)/2;
			}
			if (!(editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, allyMask, 0)))
			{
				ox += (px - ox)/2;
				oy += (py - oy)/2;
			}

			switch (linkPos->drawState)
			{
			case DS_HIGHLIGHT:
				DA::LineDraw(&tPane, ox,oy,px,py,_CLR_WHITE, 1);
				break;
			case DS_NORMAL:
				DA::LineDraw(&tPane, ox,oy,px,py, _CLR_SKY, 1);
				break;
			}
		}
		linkPos = linkPos->next;
	}
}

void Sector::PrepareTexture()
{
	// start with a fresh copy of the texture
	if (drawTextureID && mapTextureID)
		PIPE->blit_texture(drawTextureID, 0, MyRECT(0,0,SECTOR_TMAP_SIZE,SECTOR_TMAP_SIZE), mapTextureID, 0, MyRECT(0,0,mapTexW,mapTexH));
}
//--------------------------------------------------------------------------//
//
SectorEffects * Sector::GetSectorEffects(U32 playerID, U32 systemID)
{
	return &(sectorEffect[playerID-1][systemID-1]);
}
//--------------------------------------------------------------------------//
//
#define SRAD 16		// selection radius
#define RRAD 11		// attack radius
#define TRAD 8		// regular radius
#define URAD 2		// small radius
//--------------------------------------------------------------------------//
//
void Sector::updateTexture (U32 dt)
{
	S32 ox,oy;
	S32 px,py;
	unsigned int i;
	System *sysPos = firstSystem;
   	GateLink *linkPos = firstLink;
//	const Transform *trans = CAMERA->GetInverseSectorTransform();
	const U32 playerID = MGlobals::GetThisPlayer();
	const U32 allyMask = MGlobals::GetAllyMask(playerID);

	const bool editMode = DEFAULTS->GetDefaults()->bEditorMode;
	const bool bSpectate = DEFAULTS->GetDefaults()->bSpectatorModeOn;

	const U32 visAllyMask = (bSpectate) ? 0xff : (allyMask & MGlobals::GetVisibilityMask(playerID));
	HDC hdc;

	// if we are in spectator mode than we can see everything
/*	if (DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		visAllyMask |= 0xff;
	}
*/
	if(flashUp)
	{
		flash += dt/2;
		if(flash >= 255)
		{
			flash = 255;
			flashUp = false;
		}
	}
	else
	{
		flash -= dt/2;
		if(flash <= 120)
		{
			flash = 120;
			flashUp = true;
		}
	}

	while (linkPos)
	{
		linkPos->drawState = DS_NORMAL;
		linkPos = linkPos->next;
	}

	if (litSystem)
	{
		U32 firstSystem;
		IBaseObject *obj = OBJLIST->GetSelectedList();
		if (obj && obj->objClass == OC_SPACESHIP)
		{
			firstSystem = obj->GetSystemID();
			if (firstSystem != HYPER_SYSTEM_ID && !(firstSystem & HYPER_SYSTEM_MASK))
			{
				pathLength = GetShortestPath(firstSystem,litSystem,litPath, playerID);
				if (pathLength != -1)
				{
					for (i=0;i<pathLength-1;i++)
					{
						setLineState(litPath[i],litPath[i+1],DS_HIGHLIGHT);
					}
				}
			}
		}
	}

	PIPE->get_texture_dc(drawTextureID, &hdc);  //get the lock at the last possible moment

	S32 xCenter = (xExtent+xOrg)/2;
	S32 yCenter = (yExtent+yOrg)/2;
	S32 xDiff = xExtent-xOrg;
	S32 yDiff = yExtent-yOrg;

	linkPos = firstLink;
	while (linkPos)
	{
		if (linkPos == litLink)
			linkPos->drawState = DS_HIGHLIGHT;
		
		if (editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, visAllyMask, 0) || linkPos->isVisibleToPlayer(playerID, visAllyMask, 1))
		{
			linkPos->end1->GetCenter(&ox,&oy);
			linkPos->end2->GetCenter(&px,&py);
			ox = (((ox-xCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / xDiff) + (SECTOR_TMAP_SIZE/2);
			oy = SECTOR_TMAP_SIZE - ((((oy-yCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / yDiff) + (SECTOR_TMAP_SIZE/2));

			px = (((px-xCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / xDiff) + (SECTOR_TMAP_SIZE/2);
			py = SECTOR_TMAP_SIZE - ((((py-yCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / yDiff) + (SECTOR_TMAP_SIZE/2));
			
			if (!(editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, visAllyMask, 1)))
			{
				px -= (px - ox)/2;
				py -= (py - oy)/2;
			}
			if (!(editMode || bSpectate || linkPos->isVisibleToPlayer(playerID, visAllyMask, 0)))
			{
				ox += (px - ox)/2;
				oy += (py - oy)/2;
			}

			bool bDeletePen = false;
			HPEN hPen = hGrayPen;
			if (linkPos->drawState == DS_HIGHLIGHT)
				hPen = hWhitePen;
			else  // both systems in supply ?
			{
				if (linkPos->isOwnedByAlly(visAllyMask))
				{
					if (linkPos->isInRootSupply(playerID, visAllyMask))
					{
						hPen = hAquaPen;
					}
					else if (linkPos->isInSupply(playerID, visAllyMask))
					{
						hPen = CreatePen(PS_SOLID, 7, RGB(0, flash, flash));;
						bDeletePen = true;
					}
					else
						hPen = hYellowPen;
				}
				else
				if (linkPos->isOwnedByAlly(~visAllyMask) && linkPos->isOwnershipKnownToPlayer(playerID))
					hPen = hRedPen;
			}

			HGDIOBJ hOldPen = SelectObject(hdc, hPen);
			MoveToEx(hdc, ox, oy, NULL);
			LineTo(hdc, px, py);
			SelectObject(hdc, hOldPen);
			if(bDeletePen)
				DeleteObject(hPen);
		}
		linkPos = linkPos->next;
	}

	while (sysPos)
	{
		if (editMode || bSpectate || sysPos->isVisibleToPlayer(playerID, visAllyMask))
		{
			sysPos->GetCenter(&ox,&oy);
			ox = (((ox-xCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / xDiff) + (SECTOR_TMAP_SIZE/2);
			oy = SECTOR_TMAP_SIZE - ((((oy-yCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / yDiff) + (SECTOR_TMAP_SIZE/2));

			if(sysPos->bNovaBomb)
			{
				HPEN hPen = CreatePen(PS_SOLID, 7, RGB(flash, flash, 0));
				HGDIOBJ hOldPen = SelectObject(hdc, hPen);
				Ellipse(hdc, ox-RRAD, oy-RRAD, ox+RRAD, oy+RRAD);
				SelectObject(hdc, hOldPen);
				DeleteObject(hPen);
			}
			else if (sysPos->bPlayerAttack)		// active attack in progress
			{
				HPEN hPen = CreatePen(PS_SOLID, 7, RGB(flash, 0, 0));
				HGDIOBJ hOldPen = SelectObject(hdc, hPen);
				Ellipse(hdc, ox-RRAD, oy-RRAD, ox+RRAD, oy+RRAD);
				SelectObject(hdc, hOldPen);
				DeleteObject(hPen);
			}
			else
			if(sysPos->playerVisibleMask & (~allyMask))		// else enemies present
			{
				HGDIOBJ hOldPen = SelectObject(hdc, hDimRedPen);
				SelectObject(hdc, GetStockObject(NULL_BRUSH));
				Ellipse(hdc, ox-RRAD, oy-RRAD, ox+RRAD, oy+RRAD);
				SelectObject(hdc, hOldPen);
			}

			if (currentSystem == sysPos->GetID())
			{
				HGDIOBJ hOldPen = SelectObject(hdc, hWhitePen);
				SelectObject(hdc, GetStockObject(NULL_BRUSH));
				Ellipse(hdc, ox-SRAD, oy-SRAD, ox+SRAD, oy+SRAD);
				SelectObject(hdc, hOldPen);
			}

			bool bDeletePen = false;
			HPEN hPen = hGrayPen;
			bool bPlatformInSys = (sysPos->playerPlatformVisibleMask & (1 << (playerID-1))) != 0;
			if (sysPos->isInRootSupply(playerID, visAllyMask))
			{
				hPen = hAquaPen;
			}
			else if (sysPos->isInSupply(playerID, visAllyMask))
			{
				hPen = CreatePen(PS_SOLID, 7, RGB(0, flash, flash));;
				bDeletePen = true;
			}
			else
			if (bPlatformInSys)
				hPen = hYellowPen;

			HGDIOBJ hb = hAquaBrush;

			if ((sysPos->playerHQVisibleMask & (1 << (playerID-1))) == 0)		// no hq
				hb = GetStockObject(BLACK_BRUSH);

			HGDIOBJ hOldPen = SelectObject(hdc, hPen);
			SelectObject(hdc, hb);
			Ellipse(hdc, ox-TRAD, oy-TRAD, ox+TRAD, oy+TRAD);

			if (hb != hAquaBrush && bPlatformInSys)		// player has a platform here
			{
				SelectObject(hdc, (hPen==hYellowPen)?hYellowBrush:hAquaBrush);
				Ellipse(hdc, ox-URAD, oy-URAD, ox+URAD, oy+URAD);
			}
			SelectObject(hdc, hOldPen);

			if(bDeletePen)
				DeleteObject(hPen);
		}
		sysPos = sysPos->next;
	}

	if (bZoning)
	{
		if (zoneTime <= 1.0)
		{
			ox = F2LONG(zonePos.x);
			oy = F2LONG(zonePos.y);

			ox = (((ox-xCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / xDiff) + (SECTOR_TMAP_SIZE/2);
			oy = SECTOR_TMAP_SIZE - ((((oy-yCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / yDiff) + (SECTOR_TMAP_SIZE/2));

			ox = F2LONG(ox * zoneTime);
			oy = F2LONG(oy * zoneTime);

			HGDIOBJ hOldPen = SelectObject(hdc, hAquaPen);
			MoveToEx(hdc, ox, 0, NULL);
			LineTo(hdc, ox, SECTOR_TMAP_SIZE);
			MoveToEx(hdc, 0, oy, NULL);
			LineTo(hdc, SECTOR_TMAP_SIZE, oy);
			SelectObject(hdc, hOldPen);
		}
		else
		{
			ox = F2LONG(zonePos.x);
			oy = F2LONG(zonePos.y);

			ox = (((ox-xCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / xDiff) + (SECTOR_TMAP_SIZE/2);
			oy = SECTOR_TMAP_SIZE - ((((oy-yCenter)*(SECTOR_TMAP_SIZE*SCALE_FACTOR)) / yDiff) + (SECTOR_TMAP_SIZE/2));

			HGDIOBJ hOldPen = SelectObject(hdc, hAquaPen);
			MoveToEx(hdc, ox, 0, NULL);
			LineTo(hdc, ox, SECTOR_TMAP_SIZE);
			MoveToEx(hdc, 0, oy, NULL);
			LineTo(hdc, SECTOR_TMAP_SIZE, oy);
			SelectObject(hdc, hOldPen);
		}
	}

	PIPE->release_texture_dc(drawTextureID, hdc);
}
//--------------------------------------------------------------------------//
//
void Sector::createGDIObjects (void)
{
	hAquaPen =  CreatePen(PS_SOLID, 3, _CLR_AQUA);
	hGrayPen =  CreatePen(PS_SOLID, 3, _CLR_GRAY);
	hWhitePen = CreatePen(PS_SOLID, 3, _CLR_WHITE);
	hDimRedPen = CreatePen(PS_SOLID, 7, _CLR_DIM_RED);
	hYellowPen = CreatePen(PS_SOLID, 3, _CLR_YELLOW);
	hRedPen = CreatePen(PS_SOLID, 3, _CLR_DIM_RED);
	hAquaBrush = CreateSolidBrush(_CLR_AQUA);
	hYellowBrush = CreateSolidBrush(_CLR_YELLOW);
}
//--------------------------------------------------------------------------//
//
void Sector::destroyGDIObjects (void)
{
	if (hAquaPen)
		DeleteObject(hAquaPen);
	if (hGrayPen)
		DeleteObject(hGrayPen);
	if (hWhitePen)
		DeleteObject(hWhitePen);
	if (hDimRedPen)
		DeleteObject(hDimRedPen);
	if (hRedPen)
		DeleteObject(hRedPen);
	if (hYellowPen)
		DeleteObject(hYellowPen);
	if (hAquaBrush)
		DeleteObject(hAquaBrush);
	if (hYellowBrush)
		DeleteObject(hYellowBrush);
	hAquaPen = hGrayPen = hDimRedPen = hRedPen = hWhitePen = hYellowPen = 0;
	hAquaBrush = hYellowBrush = 0;
}
//--------------------------------------------------------------------------//
//
void Sector::setStatus()
{
	if (statusTextID)
	{
		wchar_t buffer[256];

		wcscpy(buffer, _localLoadStringW(statusTextID));

		if (toolTip[0])
		{
			STATUS->SetTextString2(toolTip, buffer);
		}
		else
			STATUS->SetTextString(buffer, STM_DEFAULT);
	}
	else
	if (toolTip[0])
		STATUS->SetTextString(toolTip, STM_NAME);
	else
		STATUS->SetDefaultState();
}
//--------------------------------------------------------------------------//
//
struct IBaseObject * 
Sector::GetJumpgateDestination(IBaseObject *jump)
{
	GateLink * linkPos = firstLink;

	while (linkPos)
	{
		if (jump == linkPos->jump1)
		{
			return linkPos->jump2;
		}
		if (jump == linkPos->jump2)
		{
			return linkPos->jump1;
		}
		linkPos = linkPos->next;
	}

	CQBOMB0("Could not find jumpgate");
	return 0;
}
//--------------------------------------------------------------------------//
//
struct IBaseObject *
Sector::GetJumpgateTo(U32 startSystemID,U32 destSystemID,const Vector &start_pos,U32 playerID)
{
	System *start;
	U32 i;
	IBaseObject * result = NULL;
	IBaseObject * gates[12];
	SINGLE dist[12];
	SINGLE minDist=1e30;
	S32 minSlot=-1;
	U32 gatesFound=0;
	
	start = GetSystemHandle(startSystemID);
	
	for (i=0;i<MAX_GATES;i++)
	{
		if (start->gateLinks[i])
		{
			if (start->gateLinks[i]->jump1->GetSystemID() == destSystemID)
			{
				if (playerID == 0 || start->gateLinks[i]->jump2->IsVisibleToPlayer(playerID))
				{
					CQASSERT(gatesFound < 12 && "If you MUST have 13 parallel jumpgates, go see Rob.");
					if (gatesFound < 12)
					{
						gates[gatesFound] = start->gateLinks[i]->jump2;
						dist[gatesFound] = (gates[gatesFound]->GetPosition()-start_pos).magnitude();
						gatesFound++;
					}
				}
			}
			if (start->gateLinks[i]->jump2->GetSystemID() == destSystemID)
			{
				if (playerID == 0 || start->gateLinks[i]->jump1->IsVisibleToPlayer(playerID))
				{
					CQASSERT(gatesFound < 12 && "If you MUST have 13 parallel jumpgates, go see Rob.");
					if (gatesFound < 12)
					{
						gates[gatesFound] = start->gateLinks[i]->jump1;
						dist[gatesFound] = (gates[gatesFound]->GetPosition()-start_pos).magnitude();
						gatesFound++;
					}
				}
			}
		}
		
	}

	if (gatesFound)
	{
		for (i=0;i<gatesFound;i++)
		{
			if (dist[i] < minDist)
			{
				minDist = dist[i];
				minSlot = i;
			}
		}
		CQASSERT(minSlot != -1);
		result = gates[minSlot];
	}

	CQASSERT(result==0 || result->GetSystemID()==startSystemID);

	return result;
}
//--------------------------------------------------------------------------//
//
struct IBaseObject *
Sector::GetJumpgateTo(U32 startSystemID,U32 destSystemID,const Vector &start_pos,const Vector &dest_pos,U32 playerID)
{
	System *start;
	U32 i;
	IBaseObject * result = NULL;
	IBaseObject * gates[12];
	SINGLE dist[12];
	SINGLE minDist=1e30;
	S32 minSlot=-1;
	U32 gatesFound=0;
	
	start = GetSystemHandle(startSystemID);
	
	for (i=0;i<MAX_GATES;i++)
	{
		if (start->gateLinks[i])
		{
			if (start->gateLinks[i]->jump1->GetSystemID() == destSystemID)
			{
				if (playerID == 0 || start->gateLinks[i]->jump2->IsVisibleToPlayer(playerID))
				{
					CQASSERT(gatesFound < 12 && "If you MUST have 13 parallel jumpgates, go see Rob.");
					if (gatesFound < 12)
					{
						gates[gatesFound] = start->gateLinks[i]->jump2;
						dist[gatesFound] = (gates[gatesFound]->GetPosition()-start_pos).magnitude()+(start->gateLinks[i]->jump1->GetPosition()-dest_pos).magnitude();
						gatesFound++;
					}
				}
			}
			if (start->gateLinks[i]->jump2->GetSystemID() == destSystemID)
			{
				if (playerID == 0 || start->gateLinks[i]->jump1->IsVisibleToPlayer(playerID))
				{
					CQASSERT(gatesFound < 12 && "If you MUST have 13 parallel jumpgates, go see Rob.");
					if (gatesFound < 12)
					{
						gates[gatesFound] = start->gateLinks[i]->jump1;
						dist[gatesFound] = (gates[gatesFound]->GetPosition()-start_pos).magnitude()+(start->gateLinks[i]->jump2->GetPosition()-dest_pos).magnitude();
						gatesFound++;
					}
				}
			}
		}
	}

	if (gatesFound)
	{
		for (i=0;i<gatesFound;i++)
		{
			if (dist[i] < minDist)
			{
				minDist = dist[i];
				minSlot = i;
			}
		}
		CQASSERT(minSlot != -1);
		result = gates[minSlot];
	}

	CQASSERT(result==0 || result->GetSystemID()==startSystemID);

	return result;
}
//--------------------------------------------------------------------------//
//


System *
Sector::GetSystemHandle(U32 SystemID)	const
{
	System *sysPos = firstSystem;

	while (sysPos)
	{
		if (sysPos->GetID() == SystemID)
		{
			return sysPos;
		}
		sysPos = sysPos->next;
	}

	return NULL;
}

void 
Sector::RemoveLink(GateLink *link)
{
	if (lastLink == link)
	{
		lastLink = lastLink->last;
	}
	if (firstLink == link)
	{
		firstLink = firstLink->next;
	}
}

void 
Sector::RemoveSystem(System *syst)
{
	if (lastSystem == syst)
	{
		lastSystem = lastSystem->last;
	}
	if (firstSystem == syst)
	{
		firstSystem = firstSystem->next;
	}
}

void
Sector::Update (void)
{
	updateSectors = (updateSectors+1)%32;
	if(!updateSectors)
	{
		const U32 enemyMask = ~MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
		System * sys = firstSystem;
		while(sys)
		{
			// enemies present, new for this turn
			if((sys->playerVisibleMask & enemyMask) == 0 && (sys->pendingVisibleMask & enemyMask) != 0)
				sendSightingWarning(sys->GetID(), ~enemyMask);

			sys->playerVisibleMask = sys->pendingVisibleMask;
			sys->pendingVisibleMask = 0;

			sys->playerHQVisibleMask = sys->pendingHQVisibleMask;
			sys->pendingHQVisibleMask = 0;
			
			sys->playerPlatformVisibleMask = sys->pendingPlatformVisibleMask;
			sys->pendingPlatformVisibleMask = 0;

			sys->bPlayerAttack = sys->bPendingAttack;
			sys->bPendingAttack = 0;

			sys->bNovaBomb = sys->bPendingNovaBomb;
			sys->bPendingNovaBomb = 0;

			sys = sys->next;
		}
	}
}

//--------------------------------------------------------------------------//
// do the map trick anim doobie
//
GENRESULT Sector::ZoneOn (U8 systemID)
{
	CQASSERT(systemID && systemID <= MAX_SYSTEMS);
	S32 x,y;//,ox,oy;
	System *sys = GetSystemHandle(systemID);
	sys->GetCenter(&x,&y);
//	GetScreenCoords(&ox,&oy,x,y);
//	const Transform *trans = CAMERA->GetInverseSectorTransform();
	zonePos = Vector(x,y,0);
//	zonePos = trans->rotate_translate(zonePos);
	zoneTime = 0;
	zoneRate = 0.0000015;
	bZoning = TRUE;

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void Sector::HighlightJumpgate(IBaseObject *obj)
{
	GateLink *linkPos = firstLink;
	litLink = NULL;
	while (linkPos && !litLink)
	{
		if (linkPos->jump1 == obj)
			litLink = linkPos;
		if (linkPos->jump2 == obj)
			litLink = linkPos;
		linkPos = linkPos->next;
	}
//	IBaseObject *destGate = SECTOR->GetJumpgateDestination(obj);
	litSystem = obj->GetSystemID();
}
//--------------------------------------------------------------------------//
//
void Sector::HighlightMoveSpot(const Vector &pos)
{
	CQASSERT(currentSystem);
	litSpot = pos;
	litSystem = currentSystem;
}

void Sector::ClearMoveSpot()
{
//	litSystem = INVALID_SYSTEM_ID;
}
//--------------------------------------------------------------------------//
//
void Sector::setHostVisibility (U32 systemID, U32 playerID)
{
	CQASSERT(MGlobals::IsHost());
	IBaseObject * obj = OBJLIST->GetTargetList();

	while (obj)
	{
		if (obj->GetSystemID() == systemID)
		{
			U32 id = obj->GetPlayerID();
			if (id)
				MGlobals::SetPlayerVisibility(playerID, id);
		}
		
		obj = obj->nextTarget;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::RevealSystem (U32 systemID, U32 playerID)
{
	System *sysPos=firstSystem;
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	U32 _systems=0, visSystems=0;

	while (sysPos)
	{
		if (sysPos->d.id == systemID)
		{
			if(!(sysPos->d.alertState[playerID] & S_LOCKED))
			{
				sysPos->d.alertState[playerID] |= S_VISIBLE;
				if (MGlobals::IsHost())
					setHostVisibility(systemID, playerID);
			}
		}

		if((sysPos->d.alertState[playerID] & (S_LOCKED|S_VISIBLE)) == S_VISIBLE)
			visSystems++;
		_systems++;
		
		sysPos = sysPos->next;
	}

	if (_systems)
		MGlobals::SetExploredSystemsRatio(playerID, SINGLE(visSystems) / SINGLE(_systems));
}
//--------------------------------------------------------------------------//
//
U32 Sector::CreateSystem (U32 xPos, U32 yPos, U32 width, U32 height)
{
	if(firstSystem)
	{
		lastSystem->next = new System(lastSystem);
		lastSystem = lastSystem->next;

		xExtent = xPos+width;
		yExtent = yPos+height;
		xOrg = xPos;
		yOrg = yPos;

		System *system = firstSystem;
		while(system)
		{
			if(system->d.x < xOrg)
				xOrg = system->d.x;
			if(system->d.y < yOrg)
				yOrg = system->d.y;
			if(system->d.x+system->d.sizeX > xExtent)
				xExtent = system->d.x+system->d.sizeX;
			if(system->d.y+system->d.sizeY > yExtent)
				yExtent = system->d.y+system->d.sizeY;
			system = system->next;
		}

		U32 xDist = xExtent-xOrg;
		U32 yDist = yExtent-yOrg;
		if(xDist > yDist)
		{
			S32 change = (xDist-yDist)/2;
			yOrg -= change;
			yExtent += change;
		}
		else if(xDist < yDist)
		{
			S32 change = (yDist-xDist)/2;
			xOrg -= change;
			xExtent += change;
		}
	}
	else
	{
		xExtent = xPos+width;
		yExtent = yPos+height;
		xOrg = xPos;
		yOrg = yPos;

		firstSystem = new System(NULL);
		lastSystem = firstSystem;
	}
	lastSystem->SetSize(width,height);
	lastSystem->d.x = xPos;
	lastSystem->d.y = yPos;

	if (terrainMap[0] == 0)
	{
		terrainMap[0] = dummyMap;
	}

	++numSystems;
	CQASSERT(numSystems && numSystems <= MAX_SYSTEMS);
	CreateTerrainMap(terrainMap[numSystems]);

	lastSystem->d.id = numSystems;

	currentSystem = lastSystem->d.id;

	RECT sysRect;
	if (GetSystemRect(numSystems,&sysRect))
	{
		terrainMap[numSystems]->SetWorldRect(sysRect);
	//	CAMERA->SetStateInfo(&firstSystem->d.cameraBuffer);
		
		//switch systems
		SYSMAP->SetRect(&sysRect);
		
		System *system = GetSystemHandle(currentSystem);
		
		CAMERA->SetStateInfo(&system->d.cameraBuffer,false);
		CAMERA->SetOrbitPosition();
	}

	FOGOFWAR->MakeMapTextures(numSystems);

	return lastSystem->d.id;
}
//--------------------------------------------------------------------------//
//
void Sector::CreateJumpGate(U32 systemID1, U32 x1, U32 y1, U32 & id1,U32 systemID2, U32 x2, U32 y2, U32 & id2,char * jumpArch)
{
	System * A = firstSystem;
	while(A && (A->d.id != systemID1))
	{
		A = A->next;
	}
	CQASSERT(A);
	System * B = firstSystem;
	while(B && (B->d.id != systemID2))
	{
		B = B->next;
	}
	CQASSERT(B);

	Vector a = Vector(x1,y1,0);
	Vector b = Vector(x2,y2,0);
	if (firstLink)
	{
		lastLink->next = new GateLink(A,B,lastLink);
		lastLink = lastLink->next;
	}
	else
	{
		firstLink = new GateLink(A,B,NULL);
		lastLink = firstLink;
	}
	lastLink->GenerateJumpsXY(A,B,a,b,jumpArch);
	id1 = lastLink->jump1->GetPartID();
	id2 = lastLink->jump2->GetPartID();
}
//--------------------------------------------------------------------------//
//
void Sector::AddPlatformToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass)
{
	System *sysPos=firstSystem;

	while (sysPos)
	{
		if(sysPos->GetID() == systemID)
		{
			sysPos->AddPlatformToSystem(playerMask, playerID, mObjClass);
			return;
		}
		sysPos = sysPos->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::AddShipToSystem (U32 systemID, U32 playerMask, U32 playerID, enum M_OBJCLASS mObjClass)
{
	System *sysPos=firstSystem;

	while (sysPos)
	{
		if(sysPos->GetID() == systemID)
		{
			sysPos->AddShipToSystem(playerMask, playerID, mObjClass);
			return;
		}
		sysPos = sysPos->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::AddPlayerAttack (U32 systemID)
{
	System *sysPos=firstSystem;

	while (sysPos)
	{
		if(sysPos->GetID() == systemID)
		{
			sysPos->AddPlayerAttack();
			return;
		}
		sysPos = sysPos->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::AddNovaBomb(U32 systemID)
{
	System *sysPos=firstSystem;

	while (sysPos)
	{
		if(sysPos->GetID() == systemID)
		{
			sysPos->AddNovaBomb();
			return;
		}
		sysPos = sysPos->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::RegisterJumpgate(IBaseObject *obj,U32 jgateID)
{
	GateLink *linkPos = firstLink;
	while (linkPos)
	{
		if (linkPos->gateID1 == jgateID)
			linkPos->jump1 = obj;
		if (linkPos->gateID2 == jgateID)
			linkPos->jump2 = obj;
		linkPos = linkPos->next;
	}
}
//--------------------------------------------------------------------------//
//
void Sector::sendSightingWarning (U32 systemID, U32 allyMask)
{
	// find a visible enemy in the system
	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject * friendly = 0;
	IBaseObject * admiral = 0;
	IBaseObject * enemy = 0;
	const U32 playerID = MGlobals::GetThisPlayer();

	while (obj)
	{
		const U32 hisPart = obj->GetPartID();
		const U32 hisSide = hisPart & PLAYERID_MASK;

		if (hisSide != 0 && obj->GetSystemID() == systemID)
		{
			if (((1 << (hisSide-1)) & allyMask) == 0)		// if they are NOT allies
			{
				if (enemy==0 && (obj->GetVisibilityFlags() & allyMask))		// is it visible ?
				{
					enemy = obj;
				}
			}
			else		// else friendly
			if (admiral == 0)
			{
				if (MPart(obj)->bReady)
				{
					if (hisSide == playerID && TESTADMIRAL(hisPart))
					{
						admiral = friendly = obj;
					}
					else
					if (friendly==0 || ((rand() % 8) == 0))
					{
						friendly = obj;
					}
				}
			}
		}

		obj = obj->nextTarget;
	}

	if (enemy && friendly)
	{
		MPart part = friendly;
		ENEMYSIGHTED(friendly, enemy->GetPartID(),part.pInit->displayName);
	}

}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//map sub-stuff


static Sector *sector;

/*Jump::Jump(GateLink *gLink)
{
	link = gLink;
	d.x = 0;
	d.y = 0;
	system = NULL;
	jumpGate = NULL;
//	sector = sect;
}

Jump::~Jump()
{
	if (link)
	{
		link->CutMe(this);
		if (link->alive)
			delete link;	
	}
	if (jumpGate)
		delete jumpGate;
	else
		DUMP->alert_box(0,"Invalid Load");
}

U32
Jump::GetSystemID()
{
	return system->GetID();
}

BOOL32
Jump::Save(IFileSystem *outFile,IFileSystem *file)
{
	U32 dwWritten;
	char buffer[64];
	COMPTR<IFileSystem> file2;
	DAFILEDESC fdesc;
	Vector vec;
	OBJPTR<ISaveLoad> pSaveLoad;
//	S32 ox,oy;

	//get position info
	//system->GetOrigin(&ox,&oy);

	vec = jumpGate->GetTransform().get_position();
	d.x = (S32)vec.x;
	d.y = (S32)vec.y;

	if (!file->WriteFile(0,&d,sizeof(d),&dwWritten,0))
		goto Done;

/

	return TRUE;

Done:
	return FALSE;
}

BOOL32
Jump::Load(IFileSystem *inFile,IFileSystem *file)
{
	U32 dwRead;
	//System *sysPos = firstSystem;
	BOOL32 temp = FALSE;
	PARCHETYPE gatetype;
	Vector vec;
//	S32 ox,oy;
	COMPTR<IFileSystem> file2;
	DAFILEDESC fdesc;
	OBJPTR<ISaveLoad> pSaveLoad;
	OBJPTR<IMissionActor> actor;
	char buffer[64];
	
	vec.set(0,0,0);
	
	temp = file->ReadFile(0,&d,sizeof(d),&dwRead,0);

	if (inFile->SetCurrentDirectory("\\Sector\\Jumps") == 0)
		goto Done;

	sprintf(buffer,"%i",d.id);

	gatetype = ARCHLIST->LoadArchetype("Jumpgate");
	if (gatetype != 0)
	{
	//	jumpGate = CreateBaseObject(gateID);
		jumpGate = ARCHLIST->CreateInstance(gatetype);
		jumpGate->objClass = OC_JUMPGATE;
	}
	else
	{
		DUMP->alert_box(0,"Can't find jumpgate file");
		temp = FALSE;
		goto Done;
	}

	if (inFile->SetCurrentDirectory(buffer))
	{
		if (jumpGate->QueryInterface(ISaveLoadID, pSaveLoad) != 0)
		{
			fdesc.lpFileName = ".\\";
			if (inFile->CreateInstance(&fdesc, file2) != GR_OK)
				goto Done;

			pSaveLoad->Load(file2);
			vec = jumpGate->GetTransform().get_position();
			file2.free();
		}
	}
						  
	system = sector->GetSystemHandle(d.syst_id);
//	jumpGate->systemID = system->GetID();
	if (jumpGate->QueryInterface(IMissionActorID, actor))
		actor->SetSystemID(system->GetID());

//	system->GetOrigin(&ox,&oy);
	vec.set((SINGLE)(d.x),(SINGLE)(d.y),vec.z);
	jumpGate->SetPosition(vec);
	OBJLIST->AddObject(jumpGate);
//	jumpGate->Init();		// testing!!! Is this needed? (jy)

	if (inFile->SetCurrentDirectory("\\Sector") == 0)
		goto Done;

	temp = TRUE;

Done:
	return temp;
}*/

GateLink::GateLink(System *A,System *B,GateLink *lastL)
{
	CQASSERT(A);
	CQASSERT(B);
	end1 = A;
	end2 = B;
	end_id1 = A->GetID();
	end_id2 = B->GetID();
	A->AddLink(this);
	B->AddLink(this);

	jump1 = jump2 = NULL;
	
	next = NULL;
	last = lastL;
//	sector = sect;
}

GateLink::GateLink(GateLink *lastL)
{
	alive = TRUE;
	
	end1 = NULL;
	end2 = NULL;
	
	jump1 = jump2 = NULL;

	next = NULL;
	last = lastL;
//	sector = sect;
}

GateLink::~GateLink()
{
	alive = FALSE;
	if (end1)
		end1->Detach(this);
	if (end2)
		end2->Detach(this);

	sector->RemoveLink(this);
	
		
	if (last)
		last->next = next;
	if (next)
		next->last = last;

/*	if (jump1)
		delete jump1;
	if (jump2)
		delete jump2;*/
}

/*BOOL32
GateLink::Save(IFileSystem *outFile,IFileSystem *file)
{
	U32 dwWritten;

	file->WriteFile(0,&d,sizeof(d),&dwWritten,0);

	return TRUE;
}

BOOL32
GateLink::Load(IFileSystem *inFile,IFileSystem *file)
{
	U32 dwRead;
	BOOL32 result = 0;
	
	file->ReadFile(0, &d, sizeof(d), &dwRead, 0);
	if (!dwRead)
		goto Done;
	
	end1 = sector->GetSystemHandle(d.end_id1);
	end2 = sector->GetSystemHandle(d.end_id2);
	
	if ((end1 == NULL) || (end2 == NULL)) 
		goto Done;
	
	end1->AddLink(this);
	end2->AddLink(this);
	
	result = 1;
	
Done:
	
	return result;
}*/

//U32
//GateLink::GetID()
//{
//	return d.id;
//}

/*void GateLink::CutMe(Jump *Jump)
{
	if (jump1 == Jump)
		jump1 = NULL;
	if (jump2 == Jump)
		jump2 = NULL;
}*/

BOOL32 GateLink::SetStateIfYou(U32 startID,U32 finishID,U32 state)
{
	if (end_id1 == startID && end_id2 == finishID)
	{
		drawState = state;
		return TRUE;
	}

	if (end_id2 == startID && end_id1 == finishID)
	{
		drawState = state;
		return TRUE;
	}

	return FALSE;
}

System *
GateLink::GetOtherSystem(System *thisSystem)
{
	if (end1 == thisSystem)
		return end2;
	if (end2 == thisSystem)
		return end1;

	return NULL;
}

void GateLink::GenerateJumpsXY(System *A,System *B,const Vector &a,const Vector &b, char * jumpArch)
{
	jump1 = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(jumpArch),MGlobals::CreateNewPartID(0));
	jump2 = MGlobals::CreateInstance(ARCHLIST->LoadArchetype(jumpArch),MGlobals::CreateNewPartID(0));
	OBJPTR<IJumpGate> gate;
	jump1->QueryInterface(IJumpGateID,gate);
	gate->InitWith(A->GetID(),a,jump1->GetPartID(),jump2->GetPartID());
	OBJLIST->AddObject(jump1);
	jump2->QueryInterface(IJumpGateID,gate);
	gate->InitWith(B->GetID(),b,jump2->GetPartID(),jump1->GetPartID());
	OBJLIST->AddObject(jump2);
	gateID1 = jump1->GetPartID();
	gateID2 = jump2->GetPartID();
}

System::System(System *lastS)
{
//	RECT trect;

	d.x = 0;
	d.y = 0;

//	SYSMAP->GetRect(&trect);
	SECTOR->GetDefaultSystemSize(d.sizeX,d.sizeY);

//	d.sizeX = trect.right+1;	// DEFAULT_SYSTEM_EXTENT;
//	d.sizeY = trect.top+1;		// DEFAULT_SYSTEM_EXTENT;
	
	U32 i;
	for (i=0;i<MAX_GATES;i++)
		gateLinks[i] = NULL;

	next = NULL;
	last = lastS;
	calledBy = NULL;
	
	CAMERA->GetStateInfo(&d.cameraBuffer);

	d.systemKitName[0] = 0;
	d.backgroundName[0] = 0;
}

System::~System()
{
	U32 i;
	for (i=0;i<MAX_GATES;i++)
	{
		if (gateLinks[i])
		{
			delete gateLinks[i];
		}
	}

	sector->RemoveSystem(this);
		
	if (last)
		last->next = next;
	if (next)
		next->last = last;

}

void 
System::AddLink(GateLink *link)
{
	U32 i,emptySlot=255;
	
	//count bacwards until i flips
	for (i=MAX_GATES-1;i<MAX_GATES;i--)
	{
		if (gateLinks[i] == NULL)
			 emptySlot = i;
	}
	CQASSERT(emptySlot != 255);
	gateLinks[emptySlot] = link;
}


void
System::Detach(GateLink *link)
{
	U32 i;
	
	for (i=0;i<MAX_GATES;i++)
	{
		if (gateLinks[i] == link)
			gateLinks[i] = NULL;	
	}

}

void
System::GetOrigin(S32 *ox,S32 *oy)
{
	*ox = d.x;
	*oy = d.y;
}

void
System::GetCenter(S32 *ox,S32 *oy)
{
	*ox = d.x + d.sizeX/2;
	*oy = d.y + d.sizeY/2;
}

U32
System::GetID()
{
	return d.id;
}

BOOL32
System::Save(SYSTEM_DATA *save)
{
	memcpy(save,&d,sizeof(SYSTEM_DATA));

	return 1;
}

BOOL32
System::Load(SYSTEM_DATA *load)
{
	memcpy(&d,load,sizeof(SYSTEM_DATA));
	CAMERA->SetCameraDefaults(d.cameraBuffer);		// restore minZ, maxZ, etc...

	if(d.id && d.systemKitName[0] && d.backgroundName[0])
	{
		BACKGROUND->LoadBackground(d.backgroundName,d.id);
	}
	return (d.id != 0);
}

char *
System::GetName()
{
	return d.name;
}

BOOL32
System::GetRect (struct tagRECT *rect,bool bAbsolute)
{
	if (bAbsolute)
	{
		rect->left = d.x;
		rect->bottom = d.y;
		rect->right = d.x+(d.sizeX)-1;
		rect->top = d.y+(d.sizeY)-1;
	}
	else
	{
		rect->left = 0;
		rect->bottom = 0;
		rect->right = (d.sizeX)-1;
		rect->top = (d.sizeY)-1;
	}

	return TRUE;
}

/*void
System::SetRect (struct tagRECT *rect)
{
	d.sizeX = rect->right-rect->left+1;
	d.sizeY = rect->top-rect->bottom+1;
}*/

void System::SetSize (S32 _sizeX,S32 _sizeY)
{
	d.sizeX = _sizeX;
	d.sizeY = _sizeY;
}

//returns a list of all the systems this system links to
U32 
System::GetChildren (System **returnBuffer,U32 size,U32 playerID)
{
	U32 i,x;
	System *temp;
	System *tempList[MAX_SYSTEMS];

	memset(tempList,0,sizeof(System *)*MAX_SYSTEMS);

	x=0;
	for (i=0;i<MAX_GATES;i++)
	{
		if (gateLinks[i])
		{
			temp = gateLinks[i]->GetOtherSystem(this);
			if (temp)
			{
				//check to make sure the jumpgate out is visible
				Vector spot(20000,20000,0);
				IBaseObject *jgate = gateLinks[i]->jump1;
				if (jgate->GetSystemID() != d.id)
				{
					jgate = gateLinks[i]->jump2;
					CQASSERT(jgate->GetSystemID() == d.id);
				}

				if (playerID == 0 || jgate->IsVisibleToPlayer(playerID))
				{
					if (temp->calledBy == NULL)
					{
						temp->calledBy = this;
						if (x < MAX_SYSTEMS) 
						{
							tempList[x] = temp;
							x++;
						}
					}
				}
			}
		}
	}

	//*returnBuffer = (System **)realloc(*returnBuffer,(size+x)*sizeof(System *));
	memcpy(&returnBuffer[size],tempList,x*sizeof(System *));

	return size+x;
}

U32
System::GenerateSystemList(U32 *SystemList,System *first)
{
	U32 size;
	
	if (this == first)
	{
		SystemList[0] = d.id;
		return 1;
	}

	size = calledBy->GenerateSystemList(SystemList,first);

	calledBy = NULL;

//	*SystemList = (U32 *)realloc(*SystemList,sizeof(U32)*(size+1));

	SystemList[size] = d.id;

	return size+1;
}

void System::AddPlatformToSystem (U32 playerMask, U32 playerID, M_OBJCLASS mObjClass)
{
	pendingVisibleMask |= playerMask;
	if (MGlobals::IsHQ(mObjClass))
		pendingHQVisibleMask |= playerMask;
	if (MGlobals::IsJumpPlat(mObjClass)==false)
		pendingPlatformVisibleMask |= playerMask;
}

void System::AddShipToSystem (U32 playerMask, U32 playerID, M_OBJCLASS mObjClass)
{
	pendingVisibleMask |= playerMask;
}

void System::SetLightingKit (char * kitName)
{
	strcpy(d.systemKitName,kitName);
	if(d.systemKitName[0])
		LoadKit();
}

void System::LoadKit (void)
{
	if(d.systemKitName[0])
	{
		U32 dataSize;
		const GT_SYSTEM_KIT * kit;

		if ((kit = (const GT_SYSTEM_KIT *)GENDATA->GetArchetypeData(d.systemKitName, dataSize)) != 0)
		{
			CQASSERT(dataSize == sizeof(GT_SYSTEM_KIT));

			strcpy(d.backgroundName,kit->fileName);

			IBaseObject * obj = OBJLIST->GetObjectList();

			while(obj)
			{
				if(obj->objClass == OC_LIGHT && obj->GetSystemID() == d.id)
				{
					IBaseObject * delObj = obj;
					obj = obj->next;
					delete delObj;
				}
				else
					obj = obj->next; 
			}

			BACKGROUND->LoadBackground(d.backgroundName,d.id);

			// skipping the kit lights until we have them supported by the DX9 materials
			
			//load lights new lights
			for(U32 i = 0; i < kit->numLights;++i)
			{
				IBaseObject * baseLight = ARCHLIST->CreateInstance("LIGHT!!light");
				VOLPTR(IGameLight) gameLight;
				gameLight = baseLight;
				gameLight->SetColor(kit->lightInfo[i].red,kit->lightInfo[i].green,kit->lightInfo[i].blue);
				gameLight->SetPosition(Vector(d.sizeX*kit->lightInfo[i].position.x,d.sizeY*kit->lightInfo[i].position.y,
					kit->lightInfo[i].position.z), d.id);
				gameLight->SetName(kit->lightInfo[i].name);
				gameLight->SetRange(kit->lightInfo[i].range,kit->lightInfo[i].bInfinite);
				gameLight->SetDirection(kit->lightInfo[i].direction,kit->lightInfo[i].bAmbient);
				OBJLIST->AddObject(baseLight);
			}
			
		}
		else
		{
			BACKGROUND->LoadBackground(d.backgroundName,d.id);
		}
	}
	else
	{
		BACKGROUND->LoadBackground(d.backgroundName,d.id);
	}
}
//---------------------------------------------------------------------------
//
void System::View (void)
{
	SYSTEM_VIEW view;
	strcpy(view.backgroundName,d.backgroundName);
	strcpy(view.systemKitName,d.systemKitName);

	if (DEFAULTS->GetUserData("SYSTEM_VIEW", d.name, &view, sizeof(SYSTEM_VIEW)))
	{
		if(d.systemKitName[0])
		{
			if(strcmp(view.systemKitName,d.systemKitName) == 0)
			{
				strcpy(d.backgroundName,view.backgroundName);

				COMPTR<IDocument> saveLoadDir;
				COMPTR<IDocument> baseDir;
				if (GENDATA->GetDataFile(baseDir) == GR_OK)
				{
					baseDir->CreateDirectory("\\GT_SYSTEM_KIT");

					DAFILEDESC fdesc = "\\GT_SYSTEM_KIT";
					fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
					

					baseDir->CreateInstance(&fdesc,saveLoadDir);

					COMPTR<IDocument> saveFile;

					fdesc = d.systemKitName;
					fdesc.dwCreationDistribution = CREATE_ALWAYS;
					fdesc.dwDesiredAccess = GENERIC_READ |GENERIC_WRITE;
					fdesc.lpImplementation = "DOS";

					if(saveLoadDir->CreateInstance(&fdesc,saveFile) == GR_OK)
					{
						U32 written;
						GT_SYSTEM_KIT result = SECTOR->GetSystemLightKit(d.id);
						saveFile->WriteFile(0,&result,sizeof(GT_SYSTEM_KIT),&written,0);
					}
					else
					{
						U32 error = saveLoadDir->GetLastError();
						CQERROR2("Could not create file %s, error %d",d.systemKitName,error);
					}
				}
				else
				{
					CQERROR0("This function is not available when GDI is off.");
				}
			}
			else
			{
				strcpy(d.systemKitName,view.systemKitName);
			}
		}
		else if(view.systemKitName[0])
		{
			strcpy(d.systemKitName,view.systemKitName);
		}
		else
		{
			strcpy(d.backgroundName,view.backgroundName);
		}

		LoadKit();
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct _sector : GlobalComponent
{
	virtual void Startup (void)
	{
		SECTOR = sector = new DAComponent<Sector>;
		AddToGlobalCleanupList((IDAComponent **) &SECTOR);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
		MENUITEMINFO minfo;

		if (sector->createViewer() == 0)
			CQBOMB0("Viewer could not be created.");

		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDS_VIEWSECTOR;
		minfo.dwTypeData = "Sector";
		minfo.cch = 6;	// length of string "Sector"
			
		if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
			sector->menuID = IDS_VIEWSECTOR;

		sector->initializeResources();

		if (TOOLBAR->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(SECTOR, &sector->eventHandle);

		CreateDummyTerrainMap(sector->dummyMap);
		sector->terrainMap[0] = sector->dummyMap;

		SECTOR_DATA tmpData;
		if (DEFAULTS->GetDataFromRegistry(szRegKey, &tmpData, sizeof(tmpData)) == sizeof(tmpData))
		{
			DWORD dwWritten;
			if (sector->doc)
			{
				sector->doc->SetFilePointer(0,0);
				sector->doc->WriteFile(0, &tmpData, sizeof(tmpData), &dwWritten, 0);
				sector->doc->UpdateAllClients(0);
			}
		}
		
	//	if (CURSOR->QueryOutgoingInterface("IResourceClient", connection) == GR_OK)
	//		connection->Advise(sector->GetBase(), &sector->cursorHandle);
	}
};

static _sector startup;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//------------------TOYS FOR ME---------------------------------------------//
void DrawRect(S32 x0,S32 y0,S32 x1,S32 y1,U32 pen)
{
	DA::LineDraw (0,
			x0,y0,x1,y0, 
			LD_DRAW,
			pen);
	DA::LineDraw (0,
			x1,y0,x1,y1, 
			LD_DRAW,
			pen);
	DA::LineDraw (0,
			x0,y1,x1,y1, 
			LD_DRAW,
			pen);
	DA::LineDraw (0,
			x0,y0,x0,y1, 
			LD_DRAW,
			pen);

}




//--------------------------------------------------------------------------//
//------------------------------End Sector.cpp------------------------------//
//--------------------------------------------------------------------------//
