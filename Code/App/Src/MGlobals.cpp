//--------------------------------------------------------------------------//
//                                                                          //
//                                MGlobals.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MGlobals.cpp 257   10/18/02 2:36p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>
#include <MGlobals.h>

#include <wchar.h>

#include "Resource.h"
#include "Startup.h"
#include "UserDefaults.h"
#include "menu.h"
#include <DGlobalData.h>
#include <DEffectOpts.h>
#include "Document.h"
#include "Mission.h"
#include "StringData.h"
#include "GenData.h"
#include "DBHotkeys.h"
#include "MPart.h"
#include "IFighter.h"
#include "OpAgent.h"
#include "IAdmiral.h"
#include "ILauncher.h"
#include "IFighter.h"
#include "RandomNum.h"
#include "Sector.h"

#include "Objlist.h"
#include "IMissionActor.h"
#include "Objwatch.h"
#include "CQGame.h"
#include "CQBatch.h"
#include "ScrollingText.h"
#include "Hotkeys.h"

#include <DMBaseData.h>

#include <IDocClient.h>
#include <FileSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <Heapobj.h>
#include <Viewer.h>
#include <EventSys.h>
#include <3dmath.h>
#include <Engine.h>
#include <MemFile.h>
#include <IConnection.h>

#include <malloc.h>

using namespace CQGAMETYPES;

#define DEF_MAX_CP_PER_PLAYER  100
#define GROUPID 15

#ifdef FINAL_RELEASE
#define OPPRINT1(exp,p1) ((void)0)
#else
#define OPPRINT1(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp, p1)
#endif // !FINAL_RELEASE
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct WindowPosStruct : WINDOWPLACEMENT
{
	WindowPosStruct (void)
	{
		length = sizeof(*this);
	}
};

//----------------------------------------------------------------------------

static MT_GlobalData globalData;
static GT_GLOBAL_VALUES globalValues;
static void updateGlobalViewer (void);
static U8 twoWayAllyMask[MAX_PLAYERS];
static bool resignState[MAX_PLAYERS];	// valid from 0 to 7

U32 techCheckSum = 0;
U32 resourceCheckSum = 0;

inline U32 computeTechCheckSum(MT_GlobalData * data)
{
	U32 checkSum = 0;
	U32 value = 0;
	for(U32 i = 0; i < MAX_PLAYERS_PLUS_1; ++i)
	{
		value += data->techNode[i].race[M_TERRAN].build;
		checkSum ^= data->techNode[i].race[M_TERRAN].build ;
		value += data->techNode[i].race[M_MANTIS].build;
		checkSum ^= data->techNode[i].race[M_MANTIS].build ;
		value += data->techNode[i].race[M_SOLARIAN].build;
		checkSum ^= data->techNode[i].race[M_SOLARIAN].build ;
		value += data->techNode[i].race[M_TERRAN].common;
		checkSum ^= data->techNode[i].race[M_TERRAN].common ;
		value += data->techNode[i].race[M_MANTIS].common;
		checkSum ^= data->techNode[i].race[M_MANTIS].common ;
		value += data->techNode[i].race[M_SOLARIAN].common;
		checkSum ^= data->techNode[i].race[M_SOLARIAN].common;
		value += data->techNode[i].race[M_TERRAN].common_extra;
		checkSum ^= data->techNode[i].race[M_TERRAN].common_extra ;
		value += data->techNode[i].race[M_MANTIS].common_extra;
		checkSum ^= data->techNode[i].race[M_MANTIS].common_extra ;
		value += data->techNode[i].race[M_SOLARIAN].common_extra;
		checkSum ^= data->techNode[i].race[M_SOLARIAN].common_extra;
		value += data->techNode[i].race[M_TERRAN].tech;
		checkSum ^= data->techNode[i].race[M_TERRAN].tech ;
		value += data->techNode[i].race[M_MANTIS].tech;
		checkSum ^= data->techNode[i].race[M_MANTIS].tech ;
		value += data->techNode[i].race[M_SOLARIAN].tech;
		checkSum ^= data->techNode[i].race[M_SOLARIAN].tech;

		for(U32 j = 0; j < NUM_RACES_PLUS_1; ++j)
		{
			value += data->playerTechLevel[i][j].engine;
			checkSum ^= data->playerTechLevel[i][j].engine ;
			value += data->playerTechLevel[i][j].hull;
			checkSum ^= data->playerTechLevel[i][j].hull ;
			value += data->playerTechLevel[i][j].supplies;
			checkSum ^= data->playerTechLevel[i][j].supplies ;
			value += data->playerTechLevel[i][j].targeting;
			checkSum ^= data->playerTechLevel[i][j].targeting ;
			value += data->playerTechLevel[i][j].damage;
			checkSum ^= data->playerTechLevel[i][j].damage ;
			value += data->playerTechLevel[i][j].shields;
			checkSum ^= data->playerTechLevel[i][j].shields ;
			value += data->playerTechLevel[i][j].sensors;
			checkSum ^= data->playerTechLevel[i][j].sensors ;
			value += data->playerTechLevel[i][j].fighter;
			checkSum ^= data->playerTechLevel[i][j].fighter ;
			value += data->playerTechLevel[i][j].tanker;
			checkSum ^= data->playerTechLevel[i][j].tanker ;
			value += data->playerTechLevel[i][j].tender;
			checkSum ^= data->playerTechLevel[i][j].tender ;
			value += data->playerTechLevel[i][j].fleet;
			checkSum ^= data->playerTechLevel[i][j].fleet ;
		}
	}
	checkSum ^= value;
	return checkSum;

};

inline U32 computeResourceCheckSum(MT_GlobalData * data)
{
	U32 checkSum = 0;
	U32 value = 0;
	for(U32 i = 0; i < MAX_PLAYERS_PLUS_1; ++i)
	{
		checkSum ^= data->gas[i] ;
		value += data->gas[i];
		checkSum ^= data->gasMax[i];
		value += data->gasMax[i];
		checkSum ^= data->metal[i];
		value += data->metal[i];
		checkSum ^= data->metalMax[i];
		value += data->metalMax[i];
		checkSum ^= data->crew[i];
		value += data->crew[i];
		checkSum ^= data->crewMax[i];
		value += data->crewMax[i];
		checkSum ^= data->totalCommandPts[i];
		value += data->totalCommandPts[i];
		checkSum ^= data->usedCommandPts[i];
		value += data->usedCommandPts[i];
		checkSum ^= data->maxComPts[i];
		value += data->maxComPts[i];
	}
	checkSum ^= value;
	return checkSum;
};


#define ANTIHACK_RES1 \
	bool __hacker__ = false;\
	{\
		U32 __res_check__ = computeResourceCheckSum(&globalData);\
		if(__res_check__ != resourceCheckSum) \
		{\
			if (globalData.updateCount > ((U32)RENDER_FRAMERATE)*60*3 && (PLAYERID!= 0))\
			{\
				EVENTSYS->Post(CQE_HOTKEY, (void *)IDH_GAME_OPTIONS);\
			}\
			__hacker__ = true;\
		}\
	}

#define ANTIHACK_RES2 \
	if(!__hacker__)\
	{\
		resourceCheckSum = computeResourceCheckSum(&globalData); \
	}

#define ANTIHACK_TECH1 \
	bool __hacker2__ = false;\
	{\
		U32 __tech_check__ = computeTechCheckSum(&globalData);\
		if(__tech_check__ != techCheckSum) \
		{\
			if (globalData.updateCount > ((U32)RENDER_FRAMERATE)*60*3 && (PLAYERID!= 0)) \
			{\
				EVENTSYS->Post(CQE_HOTKEY, (void *)IDH_GAME_OPTIONS);\
			}\
			__hacker2__ = true;\
		}\
	}

#define ANTIHACK_TECH2 \
	if(!__hacker2__)\
	{\
		techCheckSum = computeTechCheckSum(&globalData); \
	}

//---------------------------------------------------------------------------
//
static U32 calcAllyMask (U32 playerID)
{
	U32 result = globalData.allyMask[--playerID];
	int i;

	for (i=0; i < MAX_PLAYERS; i++)
	{
		if (result & (1 << i))		// do I think I'm allied?
		{
			if ((globalData.allyMask[i] & (1 << playerID)) == 0)	// does he disagree?
			{
				result &= ~(1 << i);	// clear the alliance bit
			}
		}
	}
	
	return result;
}
//----------------------------------------------------------------------------
//
static void recalcAllyMasks (void)
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
		twoWayAllyMask[i] = calcAllyMask(i+1);
}
//----------------------------------------------------------------------------
//
static void initGlobalData (void)
{
	int i;

	memset(&globalData, 0, sizeof(globalData));
	memset(resignState, 0, sizeof(resignState));

	for (i = 0; i <= MAX_PLAYERS; i++)
		globalData.colorAssignment[i] = i;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.gas[i] = BASE_MAX_GAS;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.metal[i] = BASE_MAX_METAL;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.crew[i] = BASE_MAX_CREW;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.gasMax[i] = 0;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.metalMax[i] = 0;

	for (i = 1; i <= MAX_PLAYERS; i++)
		globalData.crewMax[i] = 0;

	for (i = 0; i <= MAX_PLAYERS; i++)
		globalData.playerRace[i] = M_TERRAN;

	for (i = 0; i < MAX_PLAYERS; i++)
		globalData.allyMask[i] = (1 << i);

	for (i = 0; i < MAX_PLAYERS; i++)
		globalData.visibilityMask[i] = (1 << i);

	recalcAllyMasks();

	for (i = 0; i < MAX_PLAYERS; i++)
		globalData.playerAssignments[i] = i + 1;		// convert lobby slot into playerID

	globalData.currentPlayer = 1;
	globalData.bGlobalLighting = true;		// turn on the standard lights
	for(i = 1; i <= MAX_PLAYERS; i++)
		globalData.maxComPts[i] = DEF_MAX_CP_PER_PLAYER;

	globalData.availableTechNode.InitLevel(TECHTREE::FULL_TREE);

	techCheckSum = computeTechCheckSum(&globalData);
	resourceCheckSum = computeResourceCheckSum(&globalData);
//	const GT_GLOBAL_VALUES * values = (const GT_GLOBAL_VALUES *) GENDATA->GetArchetypeData("GlobalValues");
}
//----------------------------------------------------------------------------

struct DACOM_NO_VTABLE GlobalViewer : IEventCallback, IDocumentClient
{
	//
	// incoming interface map
	//
  
	BEGIN_DACOM_MAP_INBOUND(GlobalViewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	END_DACOM_MAP()

	// infrastructure
	COMPTR<IViewer> viewer;
	COMPTR<IViewer> viewer2;
	COMPTR<IViewer> viewer3;
	COMPTR<IDocument> doc1, doc2, doc3;

	U32 eventHandle;

	U32 docHandle1, docHandle2, docHandle3;

	// methods

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	GlobalViewer (void)
	{
		memset(&CQEFFECTS, 0xFF, sizeof(CQEFFECTS));
		CQEFFECTS.version = CQEFFECTSOPTIONS_VERSION;
	}

	~GlobalViewer (void);
	
	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	DEFMETHOD(OnClose) (struct IDocument *_doc)
	{
		if (_doc && _doc == doc1)
		{
			COMPTR<IDAConnectionPoint> connection;
			GENRESULT result;
			
			if ((result = _doc->QueryOutgoingInterface("IDocumentClient", connection)) == GR_OK)
				connection->Unadvise(docHandle1);
			connection.free();
			doc1 = 0;
			return result;
		}
		else
		if (_doc && _doc == doc2)
		{
			COMPTR<IDAConnectionPoint> connection;
			GENRESULT result;
			
			if ((result = _doc->QueryOutgoingInterface("IDocumentClient", connection)) == GR_OK)
				connection->Unadvise(docHandle2);
			connection.free();
			doc2 = 0;
			return result;
		}
		else
		if (_doc && _doc == doc3)
		{
			COMPTR<IDAConnectionPoint> connection;
			GENRESULT result;
			
			if ((result = _doc->QueryOutgoingInterface("IDocumentClient", connection)) == GR_OK)
				connection->Unadvise(docHandle3);
			connection.free();
			doc3 = 0;
			return result;
		}
		return GR_OK;
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	/* GlobalViewer methods */

	BOOL32 MakeConnection (IDocument * _doc, U32 & handle)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (_doc->QueryOutgoingInterface("IDocumentClient", connection) == GR_OK)
		{
			if (connection->Advise(getBase(), &handle) == GR_OK)
			{
				return 1;
			}
		}

		return 0;
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void init(void);

	BOOL32 createViewer (void);
	BOOL32 createViewer2 (void);
	BOOL32 createViewer3 (void);
};
//------------------------------------------------------------------------
//------------------------------------------------------------------------
GlobalViewer::~GlobalViewer (void)
{
	HWND hwnd;

	if (viewer)
	{
		viewer->get_main_window((void **)&hwnd);

		WindowPosStruct windowPos;

		if (GetWindowPlacement(hwnd, &windowPos))
			DEFAULTS->SetDataInRegistry("globalsDialog", &windowPos, sizeof(windowPos));
	}

	if (viewer2)
	{
		viewer2->get_main_window((void **)&hwnd);

		WindowPosStruct windowPos;

		if (GetWindowPlacement(hwnd, &windowPos))
			DEFAULTS->SetDataInRegistry("effectsDialog", &windowPos, sizeof(windowPos));
	}

	DEFAULTS->SetDataInRegistry("effectsOptions", &CQEFFECTS, sizeof(CQEFFECTS));

	if (GS)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

	if (doc1)
		OnClose(doc1);
	if (doc2)
		OnClose(doc2);
	if (doc3)
		OnClose(doc3);
}

//------------------------------------------------------------------------

void GlobalViewer::init()
{
	COMPTR<IDAConnectionPoint> connection;
	
	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
	{
		connection->Advise(getBase(), &eventHandle);
	}

	HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
	MENUITEMINFO minfo;

	memset(&minfo, 0, sizeof(minfo));
	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_ID | MIIM_TYPE;
	minfo.fType = MFT_STRING;
	minfo.wID = IDS_VIEW_GLOBALS;
	minfo.dwTypeData = "Global Values";
	minfo.cch = strlen(minfo.dwTypeData);
		
	InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);

	minfo.wID = IDS_VIEW_GLOBAL_DATA;
	minfo.dwTypeData = "Global Data";
	minfo.cch = strlen(minfo.dwTypeData);
		
	InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);

	minfo.wID = IDS_VIEW_EFFECT_OPTIONS;
	minfo.dwTypeData = "Effect Options";
	minfo.cch = strlen(minfo.dwTypeData);
		
	InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);

	initGlobalData();

	GlobalEffectsOptions options;
	if (DEFAULTS->GetDataFromRegistry("effectsOptions", &options, sizeof(options)) == sizeof(options))
	{
		if (options.version == CQEFFECTSOPTIONS_VERSION)
			CQEFFECTS = options;
		else if(options.version == 3 && CQEFFECTSOPTIONS_VERSION == 5)
		{
			GlobalEffectsOptions_V3 * effV3 = (GlobalEffectsOptions_V3 *)(&options);
			CQEFFECTS.bBackground = (GlobalEffectsOptions::OPTVAL)(effV3->bBackground);
			CQEFFECTS.bEmissiveTextures = (GlobalEffectsOptions::OPTVAL)(effV3->bEmissiveTextures);
			CQEFFECTS.bExpensiveTerrain = (GlobalEffectsOptions::OPTVAL)(effV3->bExpensiveTerrain);
			CQEFFECTS.bFastRender = (GlobalEffectsOptions::OPTVAL)(effV3->bFastRender);
			CQEFFECTS.bHighBackground = GlobalEffectsOptions::off;
			CQEFFECTS.bNoForceMaxLOD = (GlobalEffectsOptions::OPTVAL)(effV3->bNoForceMaxLOD);
			CQEFFECTS.bNoForceMinLOD = (GlobalEffectsOptions::OPTVAL)(effV3->bNoForceMinLOD);
			CQEFFECTS.bTextures = (GlobalEffectsOptions::OPTVAL)(effV3->bTextures);
			CQEFFECTS.bWeaponTrails = (GlobalEffectsOptions::OPTVAL)(effV3->bWeaponTrails);
			CQEFFECTS.nFlatShipScale = 1;
			CQEFFECTS.version = CQEFFECTSOPTIONS_VERSION;
		}
		else
		{
			CQEFFECTS.bBackground = GlobalEffectsOptions::on;
			CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::on;
			CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::on;
			CQEFFECTS.bFastRender = GlobalEffectsOptions::on;
			CQEFFECTS.bHighBackground = GlobalEffectsOptions::off;
			CQEFFECTS.bNoForceMaxLOD = GlobalEffectsOptions::on;
			CQEFFECTS.bNoForceMinLOD = GlobalEffectsOptions::on;
			CQEFFECTS.bTextures = GlobalEffectsOptions::on;
			CQEFFECTS.bWeaponTrails = GlobalEffectsOptions::on;
			CQEFFECTS.nFlatShipScale = 1;
			CQEFFECTS.version = CQEFFECTSOPTIONS_VERSION;
		}
	}
	else
	{
		CQEFFECTS.bBackground = GlobalEffectsOptions::on;
		CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::on;
		CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::on;
		CQEFFECTS.bFastRender = GlobalEffectsOptions::on;
		CQEFFECTS.bHighBackground = GlobalEffectsOptions::off;
		CQEFFECTS.bNoForceMaxLOD = GlobalEffectsOptions::on;
		CQEFFECTS.bNoForceMinLOD = GlobalEffectsOptions::on;
		CQEFFECTS.bTextures = GlobalEffectsOptions::on;
		CQEFFECTS.bWeaponTrails = GlobalEffectsOptions::on;
		CQEFFECTS.nFlatShipScale = 1;
		CQEFFECTS.version = CQEFFECTSOPTIONS_VERSION;
	}

}
//--------------------------------------------------------------------------//
//
GENRESULT GlobalViewer::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	if (doc == doc3)
	{
		DWORD dwRead;
		doc->SetFilePointer(0,0);
		doc->ReadFile(0, &globalValues, sizeof(globalValues), &dwRead, 0);
	}
	else
	if (doc == doc1)
	{
		// validate the new data
		static bool recurse;

		if (recurse==false)
		{
			recurse=true;
			if (globalData.currentPlayer < 1 || globalData.currentPlayer > 8)
				globalData.currentPlayer = 1;

			int i;
			for (i = 0; i <= MAX_PLAYERS; i++)
				if (globalData.colorAssignment[i] > MAX_PLAYERS)
					globalData.colorAssignment[i] = 0;

			recalcAllyMasks();

			doc->UpdateAllClients();
			recurse=false;
		}
	}
	else
	if (doc == doc2)
	{
		CQASSERT(PIPE);
		if (CQBATCH)
			CQBATCH->Reset();
		if (CQEFFECTS.bEmissiveTextures)
		{}
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 GlobalViewer::createViewer (void)
{
	if (doc1 == 0)
	{
		DOCDESC ddesc;
		MEMFILEDESC mdesc = "Global Data";
		COMPTR<IFileSystem> file;

		mdesc.lpBuffer = &globalData;
		mdesc.dwBufferSize = sizeof(globalData);
		mdesc.dwFlags = CMF_DONT_COPY_MEMORY;
		mdesc.dwDesiredAccess |= GENERIC_WRITE;
		mdesc.dwShareMode = 0;

		if (DACOM->CreateInstance(&mdesc, file) == GR_OK)
			file->AddRef();
		ddesc.lpParent = file;
		ddesc.lpImplementation = "DOS";
		ddesc.dwDesiredAccess |= GENERIC_WRITE;
		ddesc.dwShareMode = 0;
		ddesc.lpFileName = mdesc.lpFileName;

		if (file!=0 && DACOM->CreateInstance(&ddesc, doc1) != GR_OK)
			file->Release();		// get rid of the extra reference
	}

	if (doc1)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "MT_GlobalData";
		vdesc.doc = doc1;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);

			WindowPosStruct windowPos;

			if (DEFAULTS->GetDataFromRegistry("globalsDialog", &windowPos, sizeof(windowPos)) == sizeof(windowPos))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 windowPos.rcNormalPosition.left,
							 windowPos.rcNormalPosition.top,
							 windowPos.rcNormalPosition.right - windowPos.rcNormalPosition.left,
							 windowPos.rcNormalPosition.bottom - windowPos.rcNormalPosition.top,
							 SWP_NOZORDER);
			}
			viewer->set_instance_name("Global Data");

			MakeConnection(doc1, docHandle1);

			OnUpdate(doc1,NULL,NULL);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
//
BOOL32 GlobalViewer::createViewer2 (void)
{
	DOCDESC ddesc;
	MEMFILEDESC mdesc = "Effect Options";
	COMPTR<IFileSystem> file;

	mdesc.lpBuffer = &CQEFFECTS;
	mdesc.dwBufferSize = sizeof(CQEFFECTS);
	mdesc.dwFlags = CMF_DONT_COPY_MEMORY;
	mdesc.dwDesiredAccess |= GENERIC_WRITE;
	mdesc.dwShareMode = 0;

	if (DACOM->CreateInstance(&mdesc, file) == GR_OK)
		file->AddRef();
	ddesc.lpParent = file;
	ddesc.lpImplementation = "DOS";
	ddesc.dwDesiredAccess |= GENERIC_WRITE;
	ddesc.dwShareMode = 0;
	ddesc.lpFileName = mdesc.lpFileName;

	if (file!=0 && DACOM->CreateInstance(&ddesc, doc2) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "GlobalEffectsOptions";
		vdesc.doc = doc2;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer2) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer2->get_main_window((void **)&hwnd);

			WindowPosStruct windowPos;

			if (DEFAULTS->GetDataFromRegistry("effectsDialog", &windowPos, sizeof(windowPos)) == sizeof(windowPos))
			{
				SetWindowPos(hwnd, HWND_TOPMOST,
							 windowPos.rcNormalPosition.left,
							 windowPos.rcNormalPosition.top,
							 windowPos.rcNormalPosition.right - windowPos.rcNormalPosition.left,
							 windowPos.rcNormalPosition.bottom - windowPos.rcNormalPosition.top,
							 SWP_NOZORDER);
			}
			viewer2->set_instance_name("Effect options");

			MakeConnection(doc2, docHandle2);

			OnUpdate(doc2,NULL,NULL);
		}
	}
	else
	if (file)
		file->Release();		// get rid of the extra reference

	//
	//  create viewer for effect options
	//

	return (viewer2 != 0);
}
//--------------------------------------------------------------------------//
//
BOOL32 GlobalViewer::createViewer3 (void)
{
	COMPTR<IDocument> pDatabase;

	if (GENDATA->GetDataFile(pDatabase) == GR_OK)
	{
		// create viewer for global values

		if (doc3==0 && pDatabase->GetChildDocument("\\GT_GLOBAL_VALUES\\GlobalValues", doc3) == GR_OK)
		{
			VIEWDESC vdesc;
			HWND hwnd;

			vdesc.className = "GT_GLOBAL_VALUES";
			vdesc.doc = doc3;
			vdesc.hOwnerWindow = hMainWindow;
			
			if (CQFLAGS.bNoGDI==0 && PARSER->CreateInstance(&vdesc, viewer3) == GR_OK)
			{
				COMPTR<IDAConnectionPoint> connection;

				viewer3->get_main_window((void **)&hwnd);

				WindowPosStruct windowPos;

				if (DEFAULTS->GetDataFromRegistry("globalsDialog", &windowPos, sizeof(windowPos)) == sizeof(windowPos))
				{
					SetWindowPos(hwnd, HWND_TOPMOST,
								 windowPos.rcNormalPosition.right+20,
								 windowPos.rcNormalPosition.top,
								 windowPos.rcNormalPosition.right - windowPos.rcNormalPosition.left,
								 windowPos.rcNormalPosition.bottom - windowPos.rcNormalPosition.top,
								 SWP_NOZORDER);
				}
				viewer3->set_instance_name("GlobalValues");
			}
			
			MakeConnection(doc3, docHandle3);

			OnUpdate(doc3,NULL,NULL);
		}
	}
	else
	{
		U32 dataSize;
		const GT_GLOBAL_VALUES * values;

		if ((values = (const GT_GLOBAL_VALUES *)GENDATA->GetArchetypeData("GlobalValues", dataSize)) != 0)
		{
			globalValues = *values;
		}
		else
			CQERROR0("Failed to get GlobalValues from GenData!");

	}

	return (viewer3 != 0);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT GlobalViewer::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (message == WM_COMMAND)
	{
		switch (LOWORD(msg->wParam))
		{
		case IDS_VIEW_GLOBALS:
			if (viewer3!=0 || createViewer3())
				viewer3->set_display_state(1);
			break;

		case IDS_VIEW_GLOBAL_DATA:
			DEFAULTS->GetUserData("MT_GlobalData", "Global Data", &globalData, sizeof(globalData));
			//if (viewer!=0 || createViewer())
			//	viewer->set_display_state(1);
			break;

		case IDS_VIEW_EFFECT_OPTIONS:
			if (viewer2!=0 || createViewer2())
				viewer2->set_display_state(1);
			break;
		} // end switch
	}
	else
	if (message == CQE_DEBUG_HOTKEY && CQFLAGS.bGameActive && DEFAULTS->GetDefaults()->bCheatsEnabled)
	{
		switch (U32(param))
		{
		case IDH_INC_PLAYER_NUM:
			if (globalData.currentPlayer < MAX_PLAYERS)
			{
				globalData.currentPlayer++;
				EVENTSYS->Send(CQE_NEW_SELECTION, 0);
			}
			break;
		case IDH_DEC_PLAYER_NUM:
			if (globalData.currentPlayer > 1)
			{
				globalData.currentPlayer--;
				EVENTSYS->Send(CQE_NEW_SELECTION, 0);
			}
			break;
		}
	}

	return GR_OK;
}
//---------------------------------------------------------------------------
//---------------------------Begin MGlobals class----------------------------
//---------------------------------------------------------------------------
//
inline static void setAlly (U32 playerID1, U32 playerID2)			// one-way alliance
{
	globalData.allyMask[playerID1-1] |= 1 << (playerID2-1);
}
//---------------------------------------------------------------------------
//
inline static void setEnemy (U32 playerID1, U32 playerID2)			// one-way alliance
{
	globalData.allyMask[playerID1-1] &= ~(1 << (playerID2-1));
}

U32 MGlobals::GetUpgradeLevel (U32 playerID,enum UG_TYPE ug_type, S32 race)
{
	switch (ug_type)
	{
	case UG_FIGHTER:
		return globalData.playerTechLevel[playerID][race].fighter;
		break;
	case UG_SHIELDS:
		return globalData.playerTechLevel[playerID][race].shields;
		break;
	}

	return -1;
}
//---------------------------------------------------------------------------
//
void MGlobals::SetAlly (U32 playerID1, U32 playerID2, bool bAlly)			// one-way alliance
{
	CQASSERT(playerID1-1 < MAX_PLAYERS && playerID2-1 < MAX_PLAYERS);
	if (bAlly)
		setAlly(playerID1, playerID2);
	else
		setEnemy(playerID1, playerID2);
	recalcAllyMasks();
}
//---------------------------------------------------------------------------
//
bool MGlobals::AreAllies (U32 playerID1, U32 playerID2)
{
	CQASSERT2(playerID1-1 < MAX_PLAYERS && playerID2-1 < MAX_PLAYERS, "p1==%d, p2==%d", playerID1, playerID2);
	//
	// must be two way alliance
	//

	return ((twoWayAllyMask[playerID1-1] & twoWayAllyMask[playerID2-1] & (1 << (playerID1-1))) != 0);
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetOneWayAllyMask (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	return globalData.allyMask[playerID-1];
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetAllyMask (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	return twoWayAllyMask[playerID-1];
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetAllyData (U8 * buffer, U32 bufferSize)
{
	CQASSERT(bufferSize == sizeof(globalData.allyMask));

	memcpy(buffer, globalData.allyMask, bufferSize);
	return bufferSize;
}
//---------------------------------------------------------------------------
//
void MGlobals::SetAllyData (const U8 * buffer, U32 bufferSize)
{
	CQASSERT(bufferSize == sizeof(globalData.allyMask));

	// get the old one way alliances
	U32 oldAlliances[MAX_PLAYERS];
	U32 newAlliances[MAX_PLAYERS];
	U32 i;
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		oldAlliances[i] = GetOneWayAllyMask(i+1);
	}

	// reset the alliances
	memcpy(globalData.allyMask, buffer, bufferSize);
	recalcAllyMasks();

	const U32 allyMask = GetAllyMask(MGlobals::GetThisPlayer());

	// if any of the old alliances with this player has changed, than inform the user
	const U32 thisID = GetThisPlayer()-1;
	
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		const U32 hisID = i+1;

		if (thisID+1 == hisID)
		{
			continue;
		}

		newAlliances[i] = GetOneWayAllyMask(hisID);
		
		if ((oldAlliances[i] & (1 << thisID)) != (newAlliances[i] & (1 << thisID)))
		{
			// there has been a change to the play this player is one-way alligned with you
			wchar_t buffer[128];
			wchar_t name[64];

			memset(name, 0, sizeof(name));
			memset(buffer, 0, sizeof(buffer));

			U32 enemySlot;
			U32 slots[MAX_PLAYERS];
			
			GetSlotIDForPlayerID(hisID, slots); 
			enemySlot = slots[0]; 
			GetPlayerNameBySlot(enemySlot, name, sizeof(name));

			if (name[0])
			{
				// are we friend of foe now?
				if (allyMask & (1 << (hisID-1)))
				{
					swprintf(buffer, L"%s %s", name, _localLoadStringW(IDS_MESSAGE_ALLY_BOTH));
					SCROLLTEXT->SetTextString(buffer);
				}
				else if (newAlliances[i] & (1 << thisID))
				{
					// the other guy wishes to be friends
					swprintf(buffer, L"%s %s", name, _localLoadStringW(IDS_MESSAGE_ALLY_FRIEND));
					SCROLLTEXT->SetTextString(buffer);
				}
				else
				{
					// the other guy doesn't care for you at all
					swprintf(buffer, L"%s %s", name, _localLoadStringW(IDS_MESSAGE_ALLY_ENEMY));
					SCROLLTEXT->SetTextString(buffer);
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetVisibilityMask (U32 playerID)
{
	CQASSERT(playerID-1 < MAX_PLAYERS);

	return globalData.visibilityMask[playerID-1];
}
//---------------------------------------------------------------------------
//
void MGlobals::SetVisibilityMask (U32 playerID, U32 visibilityMask)
{
	CQASSERT(playerID-1 < MAX_PLAYERS);

	globalData.visibilityMask[playerID-1] = visibilityMask;
}
//---------------------------------------------------------------------------
//
void MGlobals::SetPlayerVisibility (U32 playerID1, U32 playerID2)
{
	CQASSERT(playerID1-1 < MAX_PLAYERS && playerID2-1 < MAX_PLAYERS);

	globalData.visibilityMask[playerID1-1] |= 1 << (playerID2-1);
	globalData.visibilityMask[playerID2-1] |= 1 << (playerID1-1);
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetThisPlayer (void)
{
	return globalData.currentPlayer;
}
//---------------------------------------------------------------------------
//
bool MGlobals::IsHost (void)
{
	return (PLAYERID==HOSTID);
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerFromPartID (U32 dwMissionID)
{
	return dwMissionID & PLAYERID_MASK;
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetOwnerFromPartID (U32 dwMissionID)
{
	return (dwMissionID & ~SUBORDID_MASK);
}
//---------------------------------------------------------------------------
//
U32 MGlobals::GetColorID (U32 playerID)
{
	CQASSERT(playerID <= MAX_PLAYERS);
	return globalData.colorAssignment[playerID];
}
//---------------------------------------------------------------------------
// only called on host machine
//
static U32 lastPartID;		// last one created (on client side, might be different from globalData)
inline static U32 createNewPartID (U32 playerID)
{
	globalData.lastPartID++;
	return (globalData.lastPartID << 4 | (playerID & PLAYERID_MASK));
}
//---------------------------------------------------------------------------
//
U32 MGlobals::CreateNewPartID (U32 playerID)
{
	if (IsHost() || CQFLAGS.bGameActive==0)		// game not started yet (generating map) 
	{
		return createNewPartID(playerID);
	}
	CQBOMB0("MGlobals::CreateNewPartID() called on client machine!");
	return 0;
}
//---------------------------------------------------------------------------
//
U32 MGlobals::CreateNewGroupPartID (void)
{
	if (IsHost() || CQFLAGS.bGameActive==0)		// game not started yet (generating map) 
	{
		return createNewPartID(GROUPID);
	}
	CQBOMB0("MGlobals::CreateNewGroupPartID() called on client machine!");
	return 0;
}
//---------------------------------------------------------------------------
//
U32 MGlobals::CreateNewJumpgatePartID (void)
{
	if (IsHost() || CQFLAGS.bGameActive==0)		// game not started yet (generating map) 
	{
		return createNewPartID(0);
	}
	CQBOMB0("MGlobals::CreateNewJumpgatePartID() called on client machine!");
	return 0;
}
//---------------------------------------------------------------------------
//
U32 MGlobals::CreateSubordinatePartID (void)
{
	CQASSERT(((lastPartID&SUBORDID_MASK) >> 24) != 127);		// out of subordinate part id's
	lastPartID += (1 << 24);		// go to next subpart, same player
	return lastPartID;
}
//---------------------------------------------------------------------------
//
bool MGlobals::IsUpdateFrame (U32 dwMissionID)
{
	U32 syncMask;
	THEMATRIX->GetDataThroughput(&syncMask);

	if (syncMask < 3)
		syncMask = 3;

	return ((rand() & syncMask) == syncMask);
}
//---------------------------------------------------------------------------
//
static void initMissionData (const MPartNC & part, U32 dwMissionID)
{
	U32 playerID = MGlobals::GetPlayerFromPartID(dwMissionID);

	part->mObjClass = part.pInit->mObjClass;
	if ((part->race = part.pInit->race) == M_NO_RACE)
		playerID = 0;
	part->dwMissionID = dwMissionID;
	part->playerID = playerID;
	part->caps = part.pInit->caps;
	part->techLevel.damage    = __max(part->techLevel.damage,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].damage));
	part->techLevel.engine    = __max(part->techLevel.engine,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].engine));
	part->techLevel.hull      = __max(part->techLevel.hull,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].hull));
	part->techLevel.supplies  = __max(part->techLevel.supplies,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].supplies));
	part->techLevel.targeting = __max(part->techLevel.targeting,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].targeting));
	part->techLevel.shields   = __max(part->techLevel.shields,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].shields));
	part->techLevel.sensors   = __max(part->techLevel.sensors,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].sensors));

				
	if((part->mObjClass == M_HARVEST) || (part->mObjClass == M_GALIOT) || (part->mObjClass == M_SIPHON))
		part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].tanker));
	else if(MGlobals::IsSupplyShip(part->mObjClass) || MGlobals::IsHQ(part->mObjClass) || MGlobals::IsTenderPlat(part->mObjClass))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].tender));
	else if(MGlobals::IsCarrier(part->mObjClass) ||
					part->mObjClass == M_SPACESTATION ||
					part->mObjClass == M_PLASMAHIVE)
		part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].fighter));
	else if(part->mObjClass == M_FLAGSHIP)
		part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].fleet));
	else if(part->mObjClass == M_TROOPSHIP)
	{
		if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP3,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
		else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP2,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
		else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP1,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
		else
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
	}
	else if(part->mObjClass == M_LEECH)
	{
		if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_LEECH2,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
		else if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_LEECH1,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
		else
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
	}
	else if(part->mObjClass == M_LEGIONAIRE)
	{
		if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION4,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL4);
		else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION3,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
		else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION2,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
		else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION1,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
		else
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
	}
	else if(part.pInit->displayName == OBJNAMES::MT_MISSILE_CRUISER)
	{
		if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK3,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
		else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK2,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
		else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK1,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
		else
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
	}
	else if(part.pInit->displayName == OBJNAMES::MT_KHAMIR)
	{
		if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM2,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
		else if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM1,0,0,0,0,0))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
		else
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
	}
	
	U32 oldHullMax = part->hullPointsMax;
	part->hullPointsMax   = part.pInit->hullPointsMax   * (1.0 + globalValues.techUpgrades[part->race].hull[globalData.playerTechLevel[playerID][part->race].hull]);
	if(oldHullMax && oldHullMax != part->hullPointsMax && THEMATRIX->IsMaster())
	{
		part->hullPoints = (part->hullPoints*part->hullPointsMax)/oldHullMax;
	}
	if((part->mObjClass == M_HARVEST) || (part->mObjClass == M_GALIOT) || (part->mObjClass == M_SIPHON))
		part->supplyPointsMax = part.pInit->supplyPointsMax * (1.0 + globalValues.techUpgrades[part->race].tanker[globalData.playerTechLevel[playerID][part->race].tanker]);
	else
		part->supplyPointsMax = part.pInit->supplyPointsMax * (1.0 + globalValues.techUpgrades[part->race].supplies[globalData.playerTechLevel[playerID][part->race].supplies]);

	part->maxVelocity = part.pInit->maxVelocity * (1.0 + globalValues.techUpgrades[part->race].engine[globalData.playerTechLevel[playerID][part->race].engine]);
	part->sensorRadius = part.pInit->sensorRadius * (1.0 + globalValues.techUpgrades[part->race].sensors[globalData.playerTechLevel[playerID][part->race].sensors]);;
	part->cloakedSensorRadius = part.pInit->cloakedSensorRadius * (1.0 + globalValues.techUpgrades[part->race].sensors[globalData.playerTechLevel[playerID][part->race].sensors]);;

	OBJPTR<IMissionActor> actor;

	if (part.obj->QueryInterface(IMissionActorID, actor))
		actor->InitActor();
}
//---------------------------------------------------------------------------
//
void MGlobals::InitMissionData (IBaseObject * obj, U32 dwMissionID)
{
	if ((dwMissionID & SUBORDID_MASK) == 0)	// can't already have subordinate defined
	{
		U32 num = (dwMissionID&~ADMIRAL_MASK) >> 4;
		globalData.lastPartID = __max(globalData.lastPartID, num);
		lastPartID = dwMissionID;
	}

	initMissionData(obj, dwMissionID);
}
//---------------------------------------------------------------------------
//
struct IBaseObject * MGlobals::CreateInstance (PARCHETYPE pArchetype, U32 newPartID)
{
	IBaseObject * obj;

	CQASSERT(newPartID!=0);

	if ((newPartID & SUBORDID_MASK) == 0)	// can't already have subordinate defined
	{
		U32 num = (newPartID&~ADMIRAL_MASK) >> 4;
		globalData.lastPartID = __max(globalData.lastPartID, num);
		lastPartID = newPartID;
	}
	
	MPartNC part = obj = ARCHLIST->CreateInstance(pArchetype);

	if (part.isValid())
	{
		initMissionData(part, newPartID);

		_ltoa(newPartID, part->partName+1, 16);
		part->partName[0] = '#';
		strupr(part->partName);
		int len = strlen(part->partName);
		part->partName[len++] = '#';
		part->partName[len] = 0;

		const char * pString = _localLoadString(part.pInit->displayName);
		const char * tmp = strchr(pString, '#');
		int maxlen = tmp ? (tmp-pString) : strlen(pString);
		maxlen = __min(maxlen, int(sizeof(M_STRING)-len-1));

		memcpy(part->partName+len, pString, maxlen);
		part->partName[len+maxlen] = 0;
		char * temp2;
		if ((temp2 = strstr(part->partName, " (")) != 0)
		{
			temp2[0] = 0;		// get rid of the trailing ( ! ) 
		}

		OBJLIST->AddPartID(obj, part->dwMissionID);
	}
	
	return obj;
}
//----------------------------------------------------------------------------------------------
//
//SINGLE MGlobals::GetBaseAccuracy (SINGLE shipAccuracy, SINGLE admiralBonus, 
//				   			      SINGLE shipSpeedMod, SINGLE targetSpeedMod, enum M_RACE shipRace,
//								  U32 playerID)
SINGLE MGlobals::GetBaseTargetingAccuracy (struct IBaseObject * obj, struct IBaseObject * target, struct IBaseObject * admiral)
{
	SINGLE result;
	SINGLE shipSpeedMod, targetSpeedMod;
	const Vector velocity = obj->GetVelocity();
	MPart part = obj;
	MPart a_part = admiral;
	const U32 shipRace = part->race;
	const SINGLE admiralBonus = (a_part.isValid()) ? 
		globalValues.techUpgrades[shipRace].admiralTargetingExp[a_part->techLevel.experience] +
			globalValues.techUpgrades[shipRace].fleet[a_part->techLevel.classSpecific]
		: 0;

	CQASSERT(part.isValid());
	if (part->maxVelocity == 0)
		CQBOMB1("maxVelocity is not initialized for part %s", (const char *)part->partName);

	if (velocity.x || velocity.y)
	{
		SINGLE vel = velocity.magnitude();
		if ((shipSpeedMod = vel / part->maxVelocity) > 1)
			shipSpeedMod = 1;
	}
	else
		shipSpeedMod = 0;
	
	const Vector targetVel = target->GetVelocity();
	if (targetVel.x || targetVel.y)
	{
		SINGLE vel = targetVel.magnitude();
		if ((targetSpeedMod = vel / part->maxVelocity) > 1)
			targetSpeedMod = 1;
	}
	else
		targetSpeedMod = 0;

	SINGLE movePenalty = (globalValues.targetingValues.movePenaltySelf   * shipSpeedMod) + 
						 (globalValues.targetingValues.movePenaltyTarget * targetSpeedMod);
	
	result = part.pInit->baseWeaponAccuracy + obj->fieldFlags.targetingBonus() +
			 globalValues.techUpgrades[shipRace].targeting[part->techLevel.targeting] +
			 globalValues.techUpgrades[shipRace].shipTargetingExp[part->techLevel.experience] +
			 admiralBonus + globalValues.raceTargetingBonus[shipRace] - movePenalty;

	CQTRACER1("GetBaseTargetingAccuracy() for part '%s': ", (const char *)part->partName);
	CQTRACER3("S Mod=%f, T Mod=%f  ->  movePenalty=%f\n", shipSpeedMod, targetSpeedMod, movePenalty);
	CQTRACER4("   (base)%f+(tech)%f+(exp)%f+(admiral)%f", 
			 part.pInit->baseWeaponAccuracy,
			 globalValues.techUpgrades[shipRace].targeting[part->techLevel.targeting],
			 globalValues.techUpgrades[shipRace].shipTargetingExp[part->techLevel.experience],
			 admiralBonus);
	CQTRACER4("+(race)%f+(field)%f-(move)%f = %f\n", 
		     globalValues.raceTargetingBonus[shipRace],
			 obj->fieldFlags.targetingBonus(),
			 movePenalty,
			 result);

	if (result > 1)
		result = 1;
	else
	if (result < globalValues.targetingValues.minAccuracy)
		result = globalValues.targetingValues.minAccuracy;

	return result;
}
//----------------------------------------------------------------------------------------------
//
IBaseObject * findFighter(U32 dwMissionID)
{
	IBaseObject * owner = OBJLIST->FindObject(dwMissionID & (~SUBORDID_MASK));
	if(owner)
	{
		OBJPTR<ILaunchOwner> launcherOwner;
		owner->QueryInterface(ILaunchOwnerID,launcherOwner);
		if(launcherOwner)
		{
			OBJPTR<ILauncher> launcher;
			OBJPTR<IFighterOwner> fighterWing;
			for(U32 i = 0; i < 4; ++i)
			{
				launcherOwner->GetLauncher(i,launcher);
				if(launcher && launcher.Ptr()->QueryInterface(IFighterOwnerID,fighterWing))
				{
					IBaseObject * fighter = fighterWing->FindFighter(dwMissionID);
					if(fighter)
						return fighter;
				}
			}
		}
	}

	return NULL;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetEffectiveDamage (U32 amount, struct IBaseObject * _shooter, struct IBaseObject * _target, U32 shooterMissionID)
{
	SINGLE result = amount;
	MPart shooter = _shooter;
	MPart target = _target;

	bool bNoDammage = false;
	if (amount == 0)		// if no damage in, no damage out
		return 0;

	if (shooter.isValid() && target.isValid())
	{
		if(!(_target->effectFlags.canBeHurt()))
		{
			bNoDammage = true;
			result = 0;
		}
		else
		{
			CQTRACER2("GetEffectiveDamage() for part '%s' -> '%s':\n", (const char *)shooter->partName, (const char*)target->partName);
			CQTRACER1("   (base)%d",amount);

			if(shooter->fleetID)
			{
				VOLPTR(IAdmiral) flagship;
				OBJLIST->FindObject(shooter->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
				if(flagship.Ptr())
				{
					result *= 1 + flagship->GetDamageBonus(shooter->mObjClass,shooter.pInit->armorData.myArmor,target.obj->objClass,target->race,target.pInit->armorData.myArmor);
					CQTRACER1("*(admiral damage)%f",(1 + flagship->GetDamageBonus(shooter->mObjClass,shooter.pInit->armorData.myArmor,target.obj->objClass,target->race,target.pInit->armorData.myArmor)));
				}
			}

			if(target->fleetID)
			{
				VOLPTR(IAdmiral) flagship;
				OBJLIST->FindObject(target->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
				if(flagship.Ptr())
				{
					result *= 1 - flagship->GetDefenceBonus(target->mObjClass,target.pInit->armorData.myArmor);
					CQTRACER1("*(admiral defence bonus)%f",(1 - flagship->GetDefenceBonus(target->mObjClass,target.pInit->armorData.myArmor)));
				}
			}

			result *= 1+SECTOR->GetSectorEffects(shooter->playerID,shooter->systemID)->getWeaponDamageMod();
			CQTRACER1("*(sector damage bonus)%f",1+SECTOR->GetSectorEffects(shooter->playerID,shooter->systemID)->getWeaponDamageMod());

			result *= 1-SECTOR->GetSectorEffects(target->playerID,target->systemID)->getDefenceMod();
			CQTRACER1("*(sector defence bonus)%f",1-SECTOR->GetSectorEffects(target->playerID,target->systemID)->getDefenceMod());

			SINGLE targetShields = (target.pInit->baseShieldLevel + globalValues.techUpgrades[target->race].shields[target->techLevel.shields]);
			if (_target->fieldFlags.shieldEnhance())
				targetShields *= 2;
			if (_target->fieldFlags.shieldsInoperable())
				targetShields = 0;
			if (_target->effectFlags.getShieldsInoperable())
				targetShields = 0;
			targetShields = __min(1, targetShields);

			result *= 1 + shooter.pInit->armorData._damageTable[target.pInit->armorData.myArmor];
			CQTRACER1("*(armor)%f",1 + shooter.pInit->armorData._damageTable[target.pInit->armorData.myArmor]);
			result *= 1 + globalValues.techUpgrades[shooter->race].damage[shooter->techLevel.damage];
			CQTRACER1("*(weapon upgrade)%f",1 + globalValues.techUpgrades[shooter->race].damage[shooter->techLevel.damage]);
			result *= 1 - targetShields;
			CQTRACER1("*(tshileds)%f",(1 - targetShields));
			result *= _target->fieldFlags.getDamageModifier();
			CQTRACER1("*(field Damage mod)%f",_target->fieldFlags.getDamageModifier());

			result *= _target->effectFlags.getDamageModifier();
			CQTRACER1("*(effect Damage mod)%f",_target->effectFlags.getDamageModifier());

			result *= _shooter->effectFlags.getWeaponDamageModifier();
			CQTRACER1("*(effect Weapon Damage mod)%f",_target->effectFlags.getWeaponDamageModifier());

			CQTRACER1("=%f\n",result);
		}
	}
	else
	if (target.isValid())
	{
		if(!(_target->effectFlags.canBeHurt()))
		{
			result = 0;
			bNoDammage = true;
		}
		else
		{
			OBJPTR<IFighter> fighter;

			if(!_shooter)
			{
				_shooter = findFighter(shooterMissionID);
			}

			if (_shooter && _shooter->QueryInterface(IFighterID, fighter))
			{
				CQTRACER1("GetEffectiveDamage() for part FIGHTER -> '%s':\n", (const char*)target->partName);
				CQTRACER1("  ( (base)%d",amount);
				M_RACE shooterRace = M_NO_RACE;
				IBaseObject * obj = OBJLIST->FindObject(fighter->GetFighterOwner());
				if(obj)
				{
					MPart fighterOwner = obj;
					if(fighterOwner.isValid())
					{
						shooterRace  = fighterOwner->race;
						//add fighter upgrade here
//						result += fighterOwner->techLevel.classSpecific;
//						CQTRACER1("+(fighter upgrade)%d )",(fighterOwner->techLevel.classSpecific));

						result *= 1 + globalValues.techUpgrades[shooterRace].damage[fighterOwner->techLevel.damage];//weapon upgrades
						CQTRACER1("*(weapon upgrade)%f",(1 + globalValues.techUpgrades[shooterRace].damage[fighterOwner->techLevel.damage]));
						if(fighterOwner->fleetID)
						{
							VOLPTR(IAdmiral) flagship;
							OBJLIST->FindObject(fighterOwner->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
							if(flagship.Ptr())
							{
								result *= 1 + flagship->GetDamageBonus(fighterOwner->mObjClass,fighterOwner.pInit->armorData.myArmor,target.obj->objClass,target->race,target.pInit->armorData.myArmor);
								CQTRACER1("*(admiral damage)%f",(1 + flagship->GetDamageBonus(fighterOwner->mObjClass,fighterOwner.pInit->armorData.myArmor,target.obj->objClass,target->race,target.pInit->armorData.myArmor)));
								result *= 1 + flagship->GetFighterDamageBonus();
								CQTRACER1("*(admiral fighter damage)%f",(1 + flagship->GetFighterDamageBonus()));
							}
						}
					}
				}

				if(target->fleetID)
				{
					VOLPTR(IAdmiral) flagship;
					OBJLIST->FindObject(target->fleetID,TOTALLYVOLATILEPTR,flagship,IAdmiralID);
					if(flagship.Ptr())
					{
						result *= 1 - flagship->GetDefenceBonus(target->mObjClass,target.pInit->armorData.myArmor);
						CQTRACER1("*(admiral defence bonus)%f",(1 - flagship->GetDefenceBonus(target->mObjClass,target.pInit->armorData.myArmor)));
					}
				}

				SINGLE targetShields = (target.pInit->baseShieldLevel + globalValues.techUpgrades[target->race].shields[target->techLevel.shields]);
				if (_target->fieldFlags.shieldEnhance())
					targetShields *= 2;
				if (_target->fieldFlags.shieldsInoperable())
					targetShields = 0;
				targetShields = __min(1, targetShields);

				FighterInfo info;
				
				fighter->GetFighterInfo(info);

				result *= 1 + info.pFighterData->armorData._damageTable[target.pInit->armorData.myArmor];
				CQTRACER1("*(armor)%f",1 + info.pFighterData->armorData._damageTable[target.pInit->armorData.myArmor]);

				result *= 1 - targetShields;
				CQTRACER1("*(tshields)%f",(1 - targetShields));

				result *= _target->fieldFlags.getDamageModifier();
				CQTRACER1("*(field Damage mod)%f",_target->fieldFlags.getDamageModifier());
				result *= _target->effectFlags.getDamageModifier();
				CQTRACER1("*(effect Damage mod)%f",_target->effectFlags.getDamageModifier());
				result *= _shooter->effectFlags.getWeaponDamageModifier();
				CQTRACER1("*(effect Weapon Damage mod)%f",_target->effectFlags.getWeaponDamageModifier());

				result *= fighter->GetHullPoints();
			
				CQTRACER2("*(numFighters)%d=%f\n" 
							, fighter->GetHullPoints(), 
							result);
			}else
			{
				SINGLE targetShields = (target.pInit->baseShieldLevel + globalValues.techUpgrades[target->race].shields[target->techLevel.shields]);
				if (_target->fieldFlags.shieldEnhance())
					targetShields *= 2;
				if (_target->fieldFlags.shieldsInoperable())
					targetShields = 0;
				targetShields = __min(1, targetShields);
				result *= 1 - targetShields;
				result *= _target->fieldFlags.getDamageModifier();
				result *= _target->effectFlags.getDamageModifier();
			}
		}
	}

	S32 r = F2LONG(result);
	if(!bNoDammage)
		r = __max(1, r);
	return r;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetIndExperienceLevel (U32 numKills)
{
	U32 result=0;
	
	while (result < NUM_TECH_LEVELS-1)
	{
		CQASSERT(globalValues.individualKillChart[result] > 0);
		if (numKills <= globalValues.individualKillChart[result])
			goto Done;
	
		result++;
	}
Done:
	return result;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetAdmiralExperienceLevel (U32 numKills)
{
	U32 result=0;

	while (result < NUM_TECH_LEVELS-1)
	{
		CQASSERT(globalValues.admiralKillChart[result] > 0);
		if (numKills <= globalValues.admiralKillChart[result])
			goto Done;
	
		result++;
	}
Done:

	return result;
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetAIBonus (U32 playerID)
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		if(globalData.playerAssignments[i] == playerID && globalData.gameSettings.slot[i].type == CQGAMETYPES::COMPUTER)
		{
			CQGAMETYPES::COMP_CHALANGE compCH =	globalData.gameSettings.slot[i].compChalange;
			if(compCH == EASY_CH)
				return -0.25;
			else if(compCH == AVERAGE_CH)
				return 0.0;
			else if(compCH == HARD_CH)
				return 0.25;
			else if(compCH == IMPOSIBLE_CH)
				return 0.50;
			else if(compCH == NIGHTMARE_CH)
				return 0.65;
		}
	}
	return 0.0;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::AdvancedAI (U32 playerID, bool expertOnly)
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		if(globalData.playerAssignments[i] == playerID && globalData.gameSettings.slot[i].type == CQGAMETYPES::COMPUTER)
		{
			CQGAMETYPES::COMP_CHALANGE compCH =	globalData.gameSettings.slot[i].compChalange;
			if(compCH == EASY_CH)
			{
				if(expertOnly)
					return false;
				else
					return true;
			}
			else if(compCH == AVERAGE_CH)
				return false;
			else if(compCH == HARD_CH)
				return true;
			else if(compCH == IMPOSIBLE_CH)
				return true;
			else if(compCH == NIGHTMARE_CH)
				return true;
		}
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsNightmareAI (U32 playerID)
{
	for(U32 i = 0; i < MAX_PLAYERS; ++i)
	{
		if(globalData.playerAssignments[i] == playerID && globalData.gameSettings.slot[i].type == CQGAMETYPES::COMPUTER)
		{
			CQGAMETYPES::COMP_CHALANGE compCH =	globalData.gameSettings.slot[i].compChalange;
			if(compCH == NIGHTMARE_CH)
				return true;
			else 
				return false;
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------
//
static BOOL32 __stdcall save (IFileSystem * outFile, U32 version)
{
	BOOL32 result = 0;
	HANDLE hFile;
	DAFILEDESC fdesc = "Globals";
	DWORD dwWritten;

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	
	outFile->CreateDirectory("\\MT_GlobalData");
	if (outFile->SetCurrentDirectory("\\MT_GlobalData")==0)
		goto Done;

	if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	globalData.gameSettings.version = version;
	result = outFile->WriteFile(hFile, &globalData, sizeof(globalData), &dwWritten, 0);
	outFile->CloseHandle(hFile);

Done:
	return result;
}
//----------------------------------------------------------------------------------------------
//
BOOL32 MGlobals::Save (IFileSystem * outFile)
{
	return save(outFile, GetBuildVersion());
}
//----------------------------------------------------------------------------------------------
//
BOOL32 MGlobals::Load (IFileSystem * inFile)
{
	BOOL32 result = 0;
	HANDLE hFile;
	DAFILEDESC fdesc = "Globals";
	DWORD dwRead;
	U8 buffer[sizeof(globalData) * 2];

	if (inFile->SetCurrentDirectory("\\MT_GlobalData")==0)
		goto Done;

	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	result = inFile->ReadFile(hFile, buffer, sizeof(buffer), &dwRead, 0);
	MISSION->CorrelateSymbol("MT_GlobalData", buffer, &globalData);
	recalcAllyMasks();
	memset(resignState, 0, sizeof(resignState));
	updateGlobalViewer();

	inFile->CloseHandle(hFile);

	techCheckSum = computeTechCheckSum(&globalData);
	resourceCheckSum = computeResourceCheckSum(&globalData);

Done:
	return result;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetFileDescription (const C8 * fileName, wchar_t * string, U32 bufferSize)
{
	// load the new global data, read the string
	COMPTR<IFileSystem> tempFile;
	DAFILEDESC fdesc = fileName;

	MT_GlobalData data;
	memset(&data, 0, sizeof(MT_GlobalData));

	SAVEDIR->CreateInstance(&fdesc, tempFile);

	if (tempFile == NULL || tempFile->SetCurrentDirectory("\\MT_GlobalData") == 0)
	{
		return string[0] = 0;
	}

	HANDLE hFile;
	DWORD dwRead;
	fdesc = "Globals";

	if ((hFile = tempFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		return string[0] = 0;
	}

	tempFile->ReadFile(hFile, &data, sizeof(data), &dwRead, 0);

	wcsncpy(string, data.gameDescription, bufferSize);
	
	tempFile->CloseHandle(hFile);
	return wcslen(string);
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetFileDescription (IFileSystem * fileDir, const C8 * fileName, wchar_t * string, U32 bufferSize)
{
	// load the new global data, read the string
	COMPTR<IFileSystem> tempFile;
	DAFILEDESC fdesc = fileName;

	MT_GlobalData data;
	memset(&data, 0, sizeof(MT_GlobalData));

	fileDir->CreateInstance(&fdesc, tempFile);

	if (tempFile == NULL || tempFile->SetCurrentDirectory("\\MT_GlobalData") == 0)
	{
		return string[0] = 0;
	}

	HANDLE hFile;
	DWORD dwRead;
	fdesc = "Globals";

	if ((hFile = tempFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		return string[0] = 0;
	}

	tempFile->ReadFile(hFile, &data, sizeof(data), &dwRead, 0);

	wcsncpy(string, data.gameDescription, bufferSize);
	
	tempFile->CloseHandle(hFile);
	return wcslen(string);
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetFileMaxPlayers (IFileSystem * fileDir, const C8 * fileName, U32 & maxPlayers)
{
	// load the new global data, read the string
	COMPTR<IFileSystem> tempFile;
	DAFILEDESC fdesc = fileName;

	MT_GlobalData data;
	memset(&data, 0, sizeof(MT_GlobalData));

	fileDir->CreateInstance(&fdesc, tempFile);

	if (tempFile == NULL || tempFile->SetCurrentDirectory("\\MT_GlobalData") == 0)
	{
		return 0;
	}

	HANDLE hFile;
	DWORD dwRead;
	fdesc = "Globals";

	if ((hFile = tempFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	tempFile->ReadFile(hFile, &data, sizeof(data), &dwRead, 0);

	maxPlayers = data.maxPlayers;
	
	tempFile->CloseHandle(hFile);
	return 1;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetFileMaxPlayers (const C8 * fileName, U32 & maxPlayers)
{
	// load the new global data, read the string
	COMPTR<IFileSystem> tempFile;
	DAFILEDESC fdesc = fileName;

	MT_GlobalData data;
	memset(&data, 0, sizeof(MT_GlobalData));

	SAVEDIR->CreateInstance(&fdesc, tempFile);

	if (tempFile == NULL || tempFile->SetCurrentDirectory("\\MT_GlobalData") == 0)
	{
		return 0;
	}

	HANDLE hFile;
	DWORD dwRead;
	fdesc = "Globals";

	if ((hFile = tempFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	tempFile->ReadFile(hFile, &data, sizeof(data), &dwRead, 0);

	maxPlayers = data.maxPlayers;
	
	tempFile->CloseHandle(hFile);
	return 1;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetFileMaxPlayers (U32 maxPlayers)
{
	globalData.maxPlayers = maxPlayers;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetFileDescription (const wchar_t * string)
{
	globalData.gameDescription = string;
}
//----------------------------------------------------------------------------------------------
//
BOOL32 MGlobals::QuickSave (IFileSystem * outFile)
{
	return save(outFile, 0);
}
//----------------------------------------------------------------------------------------------
//
BOOL32 MGlobals::New (void)
{
	//
	// initialize global data structure
	//
	initGlobalData();
	updateGlobalViewer();

	return 1;
}
//----------------------------------------------------------------------------------------------
//
BOOL32 MGlobals::Close (void)
{
	return true;
}
//----------------------------------------------------------------------------------------------
//
static bool isUsed (U32 playerSlots[MAX_PLAYERS], U32 value)
{
	bool result = true;
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
		if (playerSlots[i] == value)
			goto Done;
	result = false;
Done:
	return result;
}
//----------------------------------------------------------------------------------------------
//
static bool hasUnits (U32 playerID)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	bool result = false;

	while (obj)
	{
		if (obj->GetPlayerID() == playerID)
		{
			result = true;
			break;
		}

		obj = obj->nextTarget;
	}

	return result;
}
//----------------------------------------------------------------------------------------------
//
static U8 findUnusedColor (void)
{
	U8 result = 1;
	int i;

	for (i = 1; i <= MAX_PLAYERS; i++)
	{
		if (globalData.colorAssignment[i] == result)
		{
			i = 0;
			result++;
		}
	}

	if (result > MAX_PLAYERS)
		result = 0;

	return result;
}
//----------------------------------------------------------------------------------------------
//
static void __stdcall assignCoOpPlayers (const struct FULLCQGAME & cqgame, U32 playerSlots[MAX_PLAYERS], int slot)
{
	int i;
	const COLOR color = cqgame.slot[slot].color;
	const U32 playerID = playerSlots[slot];

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (playerSlots[i] == 0)	// if unassigned so far
		{
			if (cqgame.slot[i].color == color)		// requested same team
			{
				playerSlots[i] = playerID;
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::AssignThisPlayer (const char * szPlayerName)
{
	CQGAME & cqgame = globalData.gameSettings;
	if (cqgame.activeSlots < 1)
		cqgame.activeSlots = 1;
	cqgame.slot[0].type = HUMAN;
	cqgame.slot[0].state = READY;
//	slot[0].race = TERRAN;
	cqgame.slot[0].color = YELLOW;
	cqgame.slot[0].team = NOTEAM;
	cqgame.slot[0].dpid = PLAYERID;

	_localAnsiToWide(szPlayerName, globalData.playerNames[0], sizeof(globalData.playerNames[0]));
	globalData.playerNames[0][M_MAX_STRING-1] = 0;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsSinglePlayer (void)
{
	return (globalData.scriptLibrary[0] != 0);	//(globalData.gameSettings.gameType == MISSION_DEFINED);
}
//----------------------------------------------------------------------------------------------
// how many different players are actually participating?
static U32 getStartingNumPlayers (const struct FULLCQGAME & game)
{
	U32 numPlayers = 0;

	U32 assignments[MAX_PLAYERS+1];
	memset(assignments, 0, sizeof(assignments));
	S32 i;
	for(i = 0; i < S32(game.activeSlots); ++i)
	{
		if ((game.slot[i].state == READY) || (game.slot[i].state == ACTIVE))
			assignments[game.slot[i].color] = 1;
	}

	for (i = 1; i <= MAX_PLAYERS; i++)
		numPlayers += assignments[i];

	return numPlayers;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::AssignPlayers (const struct FULLCQGAME & cqgame, U32 randomSeed)
{
	U32 playerSlots[MAX_PLAYERS];
	int i, j;
	U32 numStartingPlayers = getStartingNumPlayers(cqgame);

	memset(playerSlots, 0, sizeof(playerSlots));
	srand(randomSeed);
	OPPRINT1("MAP GENERATION SEED = %d\n",randomSeed);
	CQASSERT(IsSinglePlayer() == false && "Winning condition == MISSION_DEFINED!?");

	//
	// try to match things exactly
	//

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerNames[i][0])
		{
			for (j = 0; j < S32(cqgame.activeSlots); j++)
			{
				if (playerSlots[j] == 0) // if unassigned
				{
					if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
					{
						if (cqgame.slot[j].type == HUMAN)
						{
							if (globalData.playerRace[globalData.playerAssignments[i]] == cqgame.slot[j].race && wcscmp(globalData.playerNames[i], cqgame.szPlayerNames[j]) == 0)
							{
								playerSlots[j] = globalData.playerAssignments[i];
								assignCoOpPlayers(cqgame, playerSlots, j);
//								numStartingPlayers++;		// increase range of random number so new players won't be squeezed out
//								numStartingPlayers = __min(numStartingPlayers, MAX_PLAYERS);
								break;
							}
						}
					}
				}
			}
		}
	}

	//
	// try to match by name, race==don't care
	//

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerNames[i][0] && isUsed(playerSlots, globalData.playerAssignments[i])==false)
		{
			for (j = 0; j < S32(cqgame.activeSlots); j++)
			{
				if (playerSlots[j] == 0) // if unassigned
				{
					if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
					{
						if (cqgame.slot[j].type == HUMAN)
						{
							if (wcscmp(globalData.playerNames[i], cqgame.szPlayerNames[j]) == 0)
							{
								playerSlots[j] = globalData.playerAssignments[i];
								assignCoOpPlayers(cqgame, playerSlots, j);
//								numStartingPlayers++;		// increase range of random number so new players won't be squeezed out
//								numStartingPlayers = __min(numStartingPlayers, MAX_PLAYERS);
								break;
							}
						}
					}
				}
			}
		}
	}

	//
	// try to match by race
	//

	{
		RandomNumGenerator generator;
		generator.init(numStartingPlayers);		// generate from 0 to MAX_PLAYERS-1

		while ((i = generator.rand()) >= 0)
		{
			i++;
			if (globalData.playerRace[i] && isUsed(playerSlots, i)==false)
			{
				for (j = 0; j < S32(cqgame.activeSlots); j++)
				{
					if (playerSlots[j] == 0) // if unassigned
					{
						if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
						{
							if (cqgame.slot[j].type == HUMAN)
							{
								if (globalData.playerRace[i] == cqgame.slot[j].race)
								{
									if (hasUnits(i))
									{
										playerSlots[j] = i;
										assignCoOpPlayers(cqgame, playerSlots, j);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//
	// find a match for remaining players that have units
	//
	for (j = 0; j < S32(cqgame.activeSlots); j++)
	{
		if (playerSlots[j] == 0) // if unassigned
		{
			if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
			{
				if (cqgame.slot[j].type == HUMAN)
				{
					RandomNumGenerator generator;
					generator.init(numStartingPlayers);		// generate from 0 to MAX_PLAYERS-1

					while ((i = generator.rand()) >= 0)
					{
						i++;
						if (isUsed(playerSlots, i)==false)
						{
							if (hasUnits(i))
							{
								playerSlots[j] = i;
								assignCoOpPlayers(cqgame, playerSlots, j);
								break;
							}
						}
					}

					if (playerSlots[j] == 0)
						break;		// could not find a player with units
				}
			}
		}
	}

	//
	// find a match for remaining players
	//
	for (j = 0; j < S32(cqgame.activeSlots); j++)
	{
		if (playerSlots[j] == 0) // if unassigned
		{
			if (cqgame.slot[j].state == ACTIVE || cqgame.slot[j].state == READY)
			{
				RandomNumGenerator generator;
				generator.init(numStartingPlayers);		// generate from 0 to MAX_PLAYERS-1

				while ((i = generator.rand()) >= 0)
				{
					i++;
					if (isUsed(playerSlots, i)==false)
					{
						playerSlots[j] = i;
						assignCoOpPlayers(cqgame, playerSlots, j);
						break;
					}
				}
			}
		}
	}

	//
	// update the names, races table to fit run-time data
	//

	memset(globalData.playerNames, 0, sizeof(globalData.playerNames));
	memset(globalData.playerRace, 0, sizeof(globalData.playerRace));
	memset(globalData.colorAssignment, 0, sizeof(globalData.colorAssignment));
	globalData.gameSettings = cqgame;

	for (j = 0; j < S32(cqgame.activeSlots); j++)
	{
		if ((globalData.playerAssignments[j] = playerSlots[j]) != 0)
		{
			globalData.playerNames[j] = cqgame.szPlayerNames[j];
			globalData.playerRace[playerSlots[j]] = static_cast<M_RACE>(cqgame.slot[j].race);
			globalData.colorAssignment[playerSlots[j]] = cqgame.slot[j].color;
		}
	}

/*
	//
	// fix up colors for unassigned slots
	//
	for (j = 0; j < S32(cqgame.activeSlots); j++)
	{
		if (playerSlots[j] != 0) // if assigned
		{
			if (globalData.colorAssignment[playerSlots[j]] == 0)
			{
				globalData.colorAssignment[playerSlots[j]] = findUnusedColor();
			}
		}
	}
*/
	//
	// fill out rest of colors
	//

	for (i = 1; i <= MAX_PLAYERS; i++)
	{
		if (globalData.colorAssignment[i] == 0)
		{
			globalData.colorAssignment[i] = findUnusedColor();
		}
	}


	globalData.currentPlayer = playerSlots[cqgame.localSlot];
	CQASSERT(globalData.currentPlayer>0 && globalData.currentPlayer<=MAX_PLAYERS);

	//
	// set aliances
	// 
	for (i = 0; i < MAX_PLAYERS; i++)
		globalData.allyMask[i] = (1 << i);

	for (j = 0; j < S32(cqgame.activeSlots); j++)
	{
		if (playerSlots[j] != 0) // if assigned
		{
			for (i = 0; i < S32(cqgame.activeSlots); i++)
			{
				if (playerSlots[i] != 0) // if assigned
				{
					if (i==j || (cqgame.slot[i].team != NOTEAM && cqgame.slot[i].team == cqgame.slot[j].team))
					{
						setAlly(playerSlots[i], playerSlots[j]);
					}
				}
			}
		}
	}

	recalcAllyMasks();
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetLastPartID (U32 lastID)
{
	lastID = (lastID & ~(SUBORDID_MASK|ADMIRAL_MASK)) >> 4;
	globalData.lastPartID = __max(globalData.lastPartID, lastID);
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetGroupID (void)
{
	return GROUPID;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetNextSubPartID (U32 dwMissionID)
{
	return dwMissionID + (1 << 24);
}
//---------------------------------------------------------------------------
//
bool MGlobals::CanTroopship (U32 playerID, U32 troopshipID, U32 targetID)
{
	IBaseObject * target = OBJLIST->FindObject(targetID);
	IBaseObject * troopship = OBJLIST->FindObject(troopshipID);
	if(target && troopship)
	{
		MPart targPart(target);
		MPart troopPart(troopship);
		if(targPart.isValid() && troopPart.isValid())
		{
			if(IsTroopship(targPart->mObjClass) || IsFlagship(targPart->mObjClass) || IsJumpPlat(targPart->mObjClass) 
				|| (target->objClass == OC_SPACESHIP && (!(MGlobals::IsFabricator(targPart->mObjClass) || MGlobals::IsHarvester(targPart->mObjClass) || MGlobals::IsSupplyShip(targPart->mObjClass))) ))
				return false;//can't troopship a troopship or flagship of jumpPlat
			S32 type = targPart.pInit->armorData.myArmor;
			if(targPart->mObjClass == M_HQ || 
				targPart->mObjClass == M_ACROPOLIS ||
				targPart->mObjClass == M_COCOON)
			{
				type = HEAVY_ARMOR+1;//these platforms are really tough to take.
			}
			U32 level = troopPart->techLevel.classSpecific;
			if(troopPart.pInit->race == M_TERRAN)
			{
				if(type == NO_ARMOR || type == LIGHT_ARMOR)
					return true;
				else if(type == MEDIUM_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL1)
					return true;
				else if(type == HEAVY_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL2)
					return true;
				else if(type == HEAVY_ARMOR+1 && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL3)
					return true;
			}
			else if(troopPart.pInit->race == M_MANTIS)
			{
				if(type == NO_ARMOR || type == LIGHT_ARMOR || type == MEDIUM_ARMOR)
					return true;
				else if(type == HEAVY_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL1)
					return true;
				else if(type == HEAVY_ARMOR+1 && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL2)
					return true;
			}
			else if(troopPart.pInit->race == M_SOLARIAN)
			{
				if(type == NO_ARMOR)
					return true;
				else if(type == LIGHT_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL1)
					return true;
				else if(type == MEDIUM_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL2)
					return true;
				else if(type == HEAVY_ARMOR && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL3)
					return true;			
				else if(type == HEAVY_ARMOR+1 && level >= MISSION_SAVELOAD::InstanceTechLevel::LEVEL4)
					return true;
			}
		}
	}
	return false;
}

//----------------------------------------------------------------------------------------------
//
TECHNODE MGlobals::GetCurrentTechLevel (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	return globalData.techNode[playerID];
}
//----------------------------------------------------------------------------------------------
//
TECHNODE MGlobals::GetWorkingTechLevel (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	return globalData.workingTechNode[playerID];
}
//----------------------------------------------------------------------------------------------
//
static MT_GlobalData::PlayerTechLevel::TECHLEVEL __fastcall getTechLevel (U32 bitfield)
{
	int result=0;

	while (bitfield > 0)
	{
		bitfield >>= 1;
		result++;
	}

	return static_cast<MT_GlobalData::PlayerTechLevel::TECHLEVEL>(result);
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentTechLevel (struct TECHNODE techNode, U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_TECH1
	bool bChanged = false;
	for(U32 i = 1 ; i < NUM_RACES_PLUS_1; ++i)
	{
		if((techNode.race[i-1].common | globalData.techNode[playerID].race[i-1].common) != (S32)(globalData.techNode[playerID].race[i-1].common))
		{	
			bChanged = true;
			globalData.playerTechLevel[playerID][i].engine   = getTechLevel(techNode.race[i-1].common & 0x0000001F);
			globalData.playerTechLevel[playerID][i].shields  = getTechLevel((techNode.race[i-1].common & 0x000003e0) >> 5);
			globalData.playerTechLevel[playerID][i].hull     = getTechLevel((techNode.race[i-1].common & 0x00007c00) >> 10);
			globalData.playerTechLevel[playerID][i].supplies = getTechLevel((techNode.race[i-1].common & 0x000F8000) >> 15);
			globalData.playerTechLevel[playerID][i].damage   = getTechLevel((techNode.race[i-1].common & 0x01F00000) >> 20);
		}
		if((techNode.race[i-1].common_extra | globalData.techNode[playerID].race[i-1].common_extra) != (S32)(globalData.techNode[playerID].race[i-1].common_extra))
		{
			bChanged = true;
			globalData.playerTechLevel[playerID][i].fleet	  = getTechLevel((techNode.race[i-1].common_extra & 0x0000001F));
			globalData.playerTechLevel[playerID][i].tanker   = getTechLevel((techNode.race[i-1].common_extra & 0x000003e0) >> 5);
			globalData.playerTechLevel[playerID][i].tender   = getTechLevel((techNode.race[i-1].common_extra & 0x00007c00) >> 10);
			globalData.playerTechLevel[playerID][i].sensors  = getTechLevel((techNode.race[i-1].common_extra & 0x000F8000) >> 15);
			globalData.playerTechLevel[playerID][i].fighter  = getTechLevel((techNode.race[i-1].common_extra & 0x01F00000) >> 20);
		}
		if((!bChanged) && (techNode.race[i-1].tech | globalData.techNode[playerID].race[i-1].tech) != (S32)(globalData.techNode[playerID].race[i-1].tech))
		{
			bChanged = true;
		}
	}
	globalData.techNode[playerID] = techNode;
	if(bChanged)
	{
		IBaseObject * obj = OBJLIST->GetTargetList();
		while(obj)
		{
			if(obj->GetPlayerID() == playerID)
				UpgradeMissionObj(obj);
			obj = obj->next;
		}
	}
	ANTIHACK_TECH2
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetWorkingTechLevel (struct TECHNODE techNode, U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	globalData.workingTechNode[playerID] = techNode;
}
//----------------------------------------------------------------------------------------------
//
struct TECHNODE MGlobals::GetTechAvailable()
{
	return globalData.availableTechNode;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetTechAvailable (struct TECHNODE techNode)
{
	globalData.availableTechNode = techNode;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::UpgradeMissionObj(IBaseObject * missionObj)
{
	MPartNC part = missionObj;
	U32 playerID = GetPlayerFromPartID(missionObj->GetPartID());

	if (part.isValid())
	{
		part->techLevel.damage    = __max(part->techLevel.damage,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].damage));
		part->techLevel.engine    = __max(part->techLevel.engine,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].engine));
		part->techLevel.hull      = __max(part->techLevel.hull,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].hull));
		part->techLevel.supplies  = __max(part->techLevel.supplies,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].supplies));
		part->techLevel.targeting = __max(part->techLevel.targeting,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].targeting));
		part->techLevel.shields   = __max(part->techLevel.shields,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].shields));
		part->techLevel.sensors   = __max(part->techLevel.sensors,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].sensors));

		if((part->mObjClass == M_HARVEST) || (part->mObjClass == M_GALIOT) || (part->mObjClass == M_SIPHON))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].tanker));
		else if(IsSupplyShip(part->mObjClass) || IsHQ(part->mObjClass) || IsTenderPlat(part->mObjClass))
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].tender));
		else if(MGlobals::IsCarrier(part->mObjClass) ||
					part->mObjClass == M_SPACESTATION ||
					part->mObjClass == M_PLASMAHIVE)
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].fighter));
		else if(part->mObjClass == M_FLAGSHIP)
			part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,static_cast<MISSION_SAVELOAD::InstanceTechLevel::UPGRADE>(globalData.playerTechLevel[playerID][part->race].fleet));
		else if(part->mObjClass == M_TROOPSHIP)
		{
			if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP3,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
			else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP2,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
			else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_TROOPSHIP1,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
			else
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
		}
		else if(part->mObjClass == M_LEECH)
		{
			if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_LEECH2,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
			else if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_LEECH1,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
			else
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
		}
		else if(part->mObjClass == M_LEGIONAIRE)
		{
			if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION4,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL4);
			else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION3,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
			else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION2,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
			else if(globalData.techNode[playerID].HasTech(M_SOLARIAN,TECHTREE::S_RES_LEGION1,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
			else
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
		}
		else if(part.pInit->displayName == OBJNAMES::MT_MISSILE_CRUISER)
		{
			if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK3,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL3);
			else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK2,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
			else if(globalData.techNode[playerID].HasTech(M_TERRAN,TECHTREE::T_RES_MISSLEPACK1,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
			else
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
		}
		else if(part.pInit->displayName == OBJNAMES::MT_KHAMIR)
		{
			if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM2,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL2);
			else if(globalData.techNode[playerID].HasTech(M_MANTIS,TECHTREE::M_RES_EXPLODYRAM1,0,0,0,0,0))
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL1);
			else
				part->techLevel.classSpecific   = __max(part->techLevel.classSpecific,MISSION_SAVELOAD::InstanceTechLevel::LEVEL0);
		}

		U32 oldHullMax = part->hullPointsMax;
		part->hullPointsMax   = part.pInit->hullPointsMax   * (1.0 + globalValues.techUpgrades[part->race].hull[globalData.playerTechLevel[playerID][part->race].hull]);
		if(oldHullMax && oldHullMax != part->hullPointsMax && THEMATRIX->IsMaster())
		{
			part->hullPoints = (part->hullPoints*part->hullPointsMax)/oldHullMax;
		}
		if((part->mObjClass == M_HARVEST) || (part->mObjClass == M_GALIOT) || (part->mObjClass == M_SIPHON))
			part->supplyPointsMax = part.pInit->supplyPointsMax * (1.0 + globalValues.techUpgrades[part->race].tanker[globalData.playerTechLevel[playerID][part->race].tanker]);
		else
			part->supplyPointsMax = part.pInit->supplyPointsMax * (1.0 + globalValues.techUpgrades[part->race].supplies[globalData.playerTechLevel[playerID][part->race].supplies]);
		part->maxVelocity = part.pInit->maxVelocity * (1.0 + globalValues.techUpgrades[part->race].engine[globalData.playerTechLevel[playerID][part->race].engine]);
		part->sensorRadius = part.pInit->sensorRadius * (1.0 + globalValues.techUpgrades[part->race].sensors[globalData.playerTechLevel[playerID][part->race].sensors]);;
		part->cloakedSensorRadius = part.pInit->cloakedSensorRadius * (1.0 + globalValues.techUpgrades[part->race].sensors[globalData.playerTechLevel[playerID][part->race].sensors]);;

	}
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetHarvestUpgrade(IBaseObject * harvester)
{
	MPart part = harvester;
	if(part.isValid())
		return 1.0 + globalValues.techUpgrades[part->race].tanker[part->techLevel.classSpecific];
	return 1.0;
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetTenderUpgrade(IBaseObject * tender)
{
	MPart part = tender;
	if(part.isValid())
		return 1.0 + globalValues.techUpgrades[part->race].tender[part->techLevel.classSpecific];
	return 1.0;
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetFleetUpgrade(IBaseObject * fleetOfficer)
{
	MPart part = fleetOfficer;
	if(part.isValid())
		return 1.0 + globalValues.techUpgrades[part->race].fleet[part->techLevel.classSpecific];
	return 1.0;
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetFighterUpgrade(U32 playerID, U32 race)
{
	return 1.0 + globalValues.techUpgrades[race].fighter[globalData.playerTechLevel[playerID][race].fighter];
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCurrentGas (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.gas[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentGas (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.gas[playerID] = amount;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCurrentMetal (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.metal[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentMetal (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.metal[playerID] = amount;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCurrentCrew (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.crew[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentCrew (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.crew[playerID] = amount;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCurrentTotalComPts (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.totalCommandPts[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentTotalComPts (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.totalCommandPts[playerID] = amount;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCurrentUsedComPts (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if(IsNightmareAI(playerID))
		return 0;
	return globalData.usedCommandPts[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCurrentUsedComPts (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	if(IsNightmareAI(playerID))
		globalData.usedCommandPts[playerID] = 0;
	else
		globalData.usedCommandPts[playerID] = amount;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetMaxGas (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.gasMax[playerID]+BASE_MAX_GAS;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMaxGas (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.gasMax[playerID] = amount-BASE_MAX_GAS;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetMaxMetal (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.metalMax[playerID]+BASE_MAX_METAL;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMaxMetal (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.metalMax[playerID] = amount-BASE_MAX_METAL;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetMaxCrew (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.crewMax[playerID]+BASE_MAX_CREW;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMaxCrew (U32 playerID, U32 amount)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	ANTIHACK_RES1
	globalData.crewMax[playerID] = amount-BASE_MAX_CREW;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::ResetResourceMax ()
{
	ANTIHACK_RES1
	for(U32 i = 1; i <= MAX_PLAYERS; ++i)
	{
		globalData.crewMax[i] = 0;
		globalData.gasMax[i] = 0;
		globalData.metalMax[i] = 0;
	}
	ANTIHACK_RES2
}

//----------------------------------------------------------------------------------------------
//
enum M_RACE MGlobals::GetPlayerRace (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return globalData.playerRace[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetPlayerRace (const U32 playerID, const enum M_RACE race)
{
	globalData.playerRace[playerID] = race;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetNumUnitsBuilt (U32 playerID)
{
	return globalData.gameStats[playerID].numUnitsBuilt;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetNumUnitsBuilt (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numUnitsBuilt = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetUnitsDestroyed (U32 playerID)
{
	return globalData.gameStats[playerID].numUnitsDestroyed;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetUnitsDestroyed (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numUnitsDestroyed = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetUnitsLost (U32 playerID)
{
	return globalData.gameStats[playerID].numUnitsLost;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetUnitsLost (U32 playerID, U32 newValue)
{
	if (CQFLAGS.bInsidePlayerResign==0)
		globalData.gameStats[playerID].numUnitsLost = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetUnitsConverted (U32 playerID)
{
	return globalData.gameStats[playerID].numUnitsConverted;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetUnitsConverted (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numUnitsConverted = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlatformsConverted (U32 playerID)
{
	return globalData.gameStats[playerID].numPlatformsConverted;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetPlatformsConverted (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numPlatformsConverted = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32  MGlobals::GetNumJumpgatesControlled (U32 playerID)
{
	return globalData.gameStats[playerID].numJumpgatesControlled;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetNumJumpgatesControlled (U32 playerID, U32 newValue)
{
	if (S32(newValue) >= 0)
		globalData.gameStats[playerID].numJumpgatesControlled = newValue;
	else
		CQERROR2("Attempt to set player #%d's \"NumJumpgatesControlled\" to %d", playerID, newValue);
}
//----------------------------------------------------------------------------------------------
//
U32  MGlobals::GetNumAdmiralsBuilt (U32 playerID)
{
	return globalData.gameStats[playerID].numAdmiralsBuilt; 
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetNumAdmiralsBuilt (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numAdmiralsBuilt = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetNumPlatformsBuilt (U32 playerID)
{
	return globalData.gameStats[playerID].numPlatformsBuilt;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetNumPlatformsBuilt (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numPlatformsBuilt = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlatformsDestroyed (U32 playerID)
{
	return globalData.gameStats[playerID].numPlatformsDestroyed;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetPlatformsDestroyed (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numPlatformsDestroyed = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlatformsLost (U32 playerID)
{
	return globalData.gameStats[playerID].numPlatformsLost;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetPlatformsLost (U32 playerID, U32 newValue)
{
	if (CQFLAGS.bInsidePlayerResign==0)
		globalData.gameStats[playerID].numPlatformsLost = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetGasGained (U32 playerID)
{
	return globalData.gameStats[playerID].gasGained;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetGasGained (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].gasGained = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetMetalGained (U32 playerID)
{
	return globalData.gameStats[playerID].metalGained;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMetalGained (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].metalGained = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetCrewGained (U32 playerID)
{
	return globalData.gameStats[playerID].crewGained;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetCrewGained (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].crewGained = newValue;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetResearchCompleted (U32 playerID)
{
	return globalData.gameStats[playerID].numResearchComplete;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetResearchCompleted (U32 playerID, U32 newValue)
{
	globalData.gameStats[playerID].numResearchComplete = newValue;
}
//----------------------------------------------------------------------------------------------
//
SINGLE MGlobals::GetExploredSystemsRatio (U32 playerID)
{
	SINGLE result = SINGLE(globalData.gameStats[playerID].percentSystemsExplored) / 255;
	return result;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetExploredSystemsRatio (U32 playerID, SINGLE newValue)
{
	U32 value = U32(newValue * 255);

	globalData.gameStats[playerID].percentSystemsExplored = value;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetGameStats (U32 playerID, void * buffer)
{
	CQASSERT(playerID && playerID<=MAX_PLAYERS);
	memcpy(buffer, &globalData.gameStats[playerID], sizeof(globalData.gameStats[1]));
	return sizeof(globalData.gameStats[1]);
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetGameStats (U32 playerID, const void * buffer, U32 bufferSize)
{
	CQASSERT(playerID && playerID<=MAX_PLAYERS);
 	CQASSERT(bufferSize==sizeof(globalData.gameStats[1]));
	memcpy(&globalData.gameStats[playerID], buffer, sizeof(globalData.gameStats[1]));
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetGameScores (void * buffer)
{
	memcpy(buffer, &globalData.gameScores[1], sizeof(globalData.gameScores[1])*MAX_PLAYERS);
	return sizeof(globalData.gameScores[1])*MAX_PLAYERS;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetGameScores (const void * buffer, U32 bufferSize)
{
 	CQASSERT(bufferSize==sizeof(globalData.gameScores[1])*MAX_PLAYERS);
	memcpy(&globalData.gameScores[1], buffer, sizeof(globalData.gameScores[1])*MAX_PLAYERS);
}
//----------------------------------------------------------------------------------------------
//
#pragma warning (disable : 4309)	// truncation of constant

void MGlobals::SetPlayerScore (U32 playerID, U32 score)
{
	CQASSERT(IsHost());
	CQASSERT(playerID && playerID<=MAX_PLAYERS);

	globalData.gameScores[playerID] = score;
	if (globalData.gameScores[playerID] != score)
		globalData.gameScores[playerID] = 0xFFFFFFFF;		// set max value instead of truncating
}

#pragma warning (default : 4309)	// truncation of constant
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerScore (U32 playerID)
{
	CQASSERT(playerID && playerID<=MAX_PLAYERS);
	return globalData.gameScores[playerID];
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerNameBySlot (U32 slotID, wchar_t * buffer, U32 bufferSize)
{
	CQASSERT(slotID < MAX_PLAYERS);
	U32 result=0;

	result = wcslen(globalData.playerNames[slotID]);

	if (result+1 > bufferSize/sizeof(wchar_t))
		result = (bufferSize/sizeof(wchar_t)) - 1;

	memcpy(buffer, globalData.playerNames[slotID], (result+1)*sizeof(wchar_t));
	
	return result;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerNameFromDPID (U32 dpid, wchar_t * buffer, U32 bufferSize)
{
	U32 i, result=0;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (dpid == globalData.gameSettings.slot[i].dpid)
		{
			result = wcslen(globalData.playerNames[i]);

			if (result+1 > bufferSize/sizeof(wchar_t))
				result = (bufferSize/sizeof(wchar_t)) - 1;

			memcpy(buffer, globalData.playerNames[i], (result+1)*sizeof(wchar_t));
		}
	}
	
	return result;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerIDFromDPID (U32 dpID)
{
	U32 i = 0;

	while (i < MAX_PLAYERS)
	{
		if (dpID == globalData.gameSettings.slot[i].dpid)
			return globalData.playerAssignments[i];
		i++;
	}

	return 0;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetSlotIDFromDPID (U32 dpID)
{
	U32 i = 0;

	while (i < MAX_PLAYERS)
	{
		if (dpID == globalData.gameSettings.slot[i].dpid)
			return i;
		i++;
	}

	return 0xFFFFFFFF;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetupComputerCharacter (U32 playerID, const wchar_t * szName)
{
	CQGAME & cqgame = globalData.gameSettings;

	// increase the active slots value by 1
	int id = cqgame.activeSlots++;

	cqgame.slot[id].type  = CQGAMETYPES::COMPUTER;
	cqgame.slot[id].compChalange = CQGAMETYPES::AVERAGE_CH;
	cqgame.slot[id].state = CQGAMETYPES::READY;
	cqgame.slot[id].color = static_cast<CQGAMETYPES::COLOR>(playerID);
	
	// set the player assignments
	globalData.playerAssignments[id] = playerID;

	wcscpy(globalData.playerNames[id], szName);
	globalData.playerNames[id][wcslen(szName)] = 0;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsPlayerInGame (U32 playerID)
{
	U32 i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerAssignments[i] == playerID)
		{
			return true;
		}
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetPlayerIDFromSlot (U32 slotID)
{
	CQASSERT(slotID < MAX_PLAYERS);

	return globalData.playerAssignments[slotID];
}
//----------------------------------------------------------------------------------------------
// returns # of DPID's that match the playerID
//
U32 MGlobals::GetPlayerDPIDForPlayerID (U32 playerID, U32 * pDPID)
{
	U32 result = 0;
	U32 i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerAssignments[i] == playerID)
		{
			if (pDPID)
				pDPID[result] = globalData.gameSettings.slot[i].dpid;
			if (globalData.gameSettings.slot[i].dpid)		// don't count people who have dropped
				result++;
		}
	}

	return result;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetSlotIDForPlayerID (U32 playerID, U32 * pSlots)
{
	U32 result = 0;
	U32 i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerAssignments[i] == playerID)
		{
			if (pSlots)
				pSlots[result++] = i;
		}
	}

	return result;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsHostOnlyPlayerLeft (void)
{
	// return true only if there are no other humans (dpids) and there is at least one computer AI enemy left

	const CQGAME & cqgame = MGlobals::GetGameSettings();
	U32 i;

	bool bFoundOtherHuman = false;
	bool bFoundComputerEnemy = false;
					
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		const U32 playID = MGlobals::GetPlayerIDFromSlot(i);

		if (playID && playID != GetThisPlayer())
		{
			// another human around?
			if (globalData.gameSettings.slot[i].dpid != 0)
			{
				bFoundOtherHuman = true;
			}

			// found a computer enemy AI?
			if (MISSION->IsComputerControlled(playID) && AreAllies(playID, GetThisPlayer()) == false)
			{
				bFoundComputerEnemy = true;
			}
		}
		else
		if (playID)		// must be "this" player
		{
			if (HasPlayerResigned(i))
				return false;
		}
	}


	return (bFoundOtherHuman == false) && (bFoundComputerEnemy == true);
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::RemoveDPIDFromPlayerID (U32 dpid, U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	U32 i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (globalData.playerAssignments[i] == playerID)
		{
			if (dpid == globalData.gameSettings.slot[i].dpid)
			{
//				globalData.playerAssignments[i] = 0;
				globalData.gameSettings.slot[i].dpid = 0;
			}
		}
	}
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetZoneSeatFromSlot (U32 slotID)
{
	CQASSERT(slotID < MAX_PLAYERS);

	return globalData.gameSettings.slot[slotID].zoneSeat;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetZoneSeatFromSlot (U32 slotID, U32 dwSeat)
{
	CQASSERT(slotID < MAX_PLAYERS);

	globalData.gameSettings.slot[slotID].zoneSeat = dwSeat;
}
//----------------------------------------------------------------------------------------------
//
const struct CQGAME & MGlobals::GetGameSettings (void)
{
	return globalData.gameSettings;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetRegenMode (bool bEnable)
{
	globalData.gameSettings.regenOn = bEnable;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsGlobalLighting (void)
{
	return globalData.bGlobalLighting;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::EnableGlobalLighting (bool bEnable)
{
//	CQASSERT(DEFAULTS->GetDefaults()->bEditorMode);
	globalData.bGlobalLighting = bEnable;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetMaxControlPoints (U32 playerID)
{
	return globalData.maxComPts[playerID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMaxControlPoints (U32 playerID, U32 newMax)
{
	ANTIHACK_RES1
	globalData.maxComPts[playerID] = newMax;
	ANTIHACK_RES2
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetScriptName (char * buffer, U32 bufferSize)
{
	U32 result = strlen(globalData.scriptLibrary);

	if (result+1 > bufferSize)
		result = bufferSize - 1;

	memcpy(buffer, globalData.scriptLibrary, result+1);
	return result;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetScriptName (const char * buffer)
{
	strncpy(globalData.scriptLibrary, buffer, sizeof(buffer));
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetTerrainFilename (char * buffer, U32 bufferSize)
{
	U32 result = strlen(globalData.baseTerrainMap);

	if (result+1 > bufferSize)
		result = bufferSize - 1;

	memcpy(buffer, globalData.baseTerrainMap, result+1);
	return result;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetTerrainFilename (const char * buffer)
{
	strncpy(globalData.baseTerrainMap, buffer, strlen(buffer)+1);
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetUpdateCount (void)
{
	return globalData.updateCount;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetUpdateCount (U32 count)
{
	ANTIHACK_RES1
	ANTIHACK_TECH1
	globalData.updateCount = count;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsHQ (enum M_OBJCLASS oc)
{
	if ((oc == M_HQ) || (oc == M_COCOON) || (oc == M_ACROPOLIS) || (oc == M_LOCUS))
		return true;
		
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsPlatform (enum M_OBJCLASS oc)
{
	if (((oc >= M_HQ) && (oc <= M_CLOAKSTATION)) || 
		((oc >= M_JUMPPLAT) && (oc <= M_PLASMAHIVE)) ||
		((oc >= M_ACROPOLIS) && (oc <= M_PORTAL)) ||
		((oc >= M_LOCUS) && (oc <= M_CYNOSURE)))
		return true;
		
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsRefinery (enum M_OBJCLASS oc)
{
	if ((oc == M_REFINERY) || (oc == M_HEAVYREFINERY) || (oc == M_SUPERHEAVYREFINERY) ||
		(oc == M_COLLECTOR) || (oc == M_GREATER_COLLECTOR) || (oc == M_OXIDATOR) ||
		(oc == M_COALESCER))
		return true;
		
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsShipyard (enum M_OBJCLASS oc)
{
	if ((oc == M_LIGHTIND) || (oc == M_HEAVYIND) || (oc == M_THRIPID) ||
		(oc == M_NIAD) || (oc == M_PAVILION) || (oc == M_GREATERPAVILION) ||
		(oc == M_COMPILER))
		return true;
		
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsRepairPlat (enum M_OBJCLASS oc)
{
	if ((oc == M_REPAIR) || (oc == M_GREATER_PLANTATION) || (oc == M_EUTROMILL))
		return true;
		
	return false;
}
///----------------------------------------------------------------------------------------------
//
bool MGlobals::IsTenderPlat (enum M_OBJCLASS oc)
{
	if ((oc == M_TENDER) || (oc == M_REPAIR) || 
		(oc == M_PLANTATION) || (oc == M_GREATER_PLANTATION)
		|| (oc == M_EUTROMILL))
		return true;

	// an HQ is not a tender plat, this causes bad shit - SBarton
//	if (IsHQ(oc))
//		return true;
		
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsMilitaryShip (enum M_OBJCLASS oc)
{
	if (((oc >= M_CORVETTE) && (oc <= M_INFILTRATOR)) ||
		((oc >= M_SCOUTCARRIER) && (oc <= M_TIAMAT)) ||
		((oc >= M_TAOS) && (oc <= M_MONOLITH)) ||
		((oc == M_TROOPSHIP) || (oc == M_SPINELAYER)) ||
		((oc >= M_MYSTIC) && (oc <= M_LEVIATHIN)) )
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsObjectThreatening (enum M_OBJCLASS oc)
{
	// is object a gunboat or sight unit or gun plat?
	if (oc == M_LSAT || oc == M_SPACESTATION || oc == M_IONCANNON)
	{
		return true;
	}
	else if (oc == M_PLASMASPLITTER || oc == M_VORAAKCANNON || oc == M_PLASMAHIVE)
	{
		return true;
	}
	else if (oc == M_PROTEUS || oc == M_HYDROFOIL || oc == M_ESPCOIL || oc == M_STARBURST)
	{
		return true;
	}

	if (((oc >= M_CORVETTE) && (oc <= M_INFILTRATOR)) ||
		((oc >= M_SCOUTCARRIER) && (oc <= M_TIAMAT)) ||
		((oc >= M_TAOS) && (oc <= M_MONOLITH)) ||
		((oc == M_TROOPSHIP) || (oc == M_SPINELAYER)) ||
		((oc >= M_MYSTIC) && (oc <= M_LEVIATHIN)))
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsGunboat (enum M_OBJCLASS oc)
{
	if (((oc >= M_CORVETTE) && (oc <= M_INFILTRATOR)) ||
		((oc >= M_SEEKER) && (oc <= M_TIAMAT)) ||
		((oc >= M_TAOS) && (oc <= M_MONOLITH)))
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsSeekerShip (enum M_OBJCLASS oc)
{
	if (oc == M_SEEKER || oc == M_INFILTRATOR || oc == M_ORACLE || oc == M_MOK)
	{
		return true;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsLightGunboat (enum M_OBJCLASS oc)
{
	switch (oc)
	{
	case M_CORVETTE:
	case M_MISSILECRUISER:
	case M_INFILTRATOR:
	
	case M_SCOUTCARRIER:
	case M_KHAMIR:
	case M_SEEKER:
	case M_SPINELAYER:
	
	case M_TAOS:
	case M_AURORA:
	case M_ORACLE:

	case M_VIPER:
	case M_MOK:
	case M_NECTROP:
	case M_ADDER:
		return true;

	default:
		return false;
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsMediumGunboat (enum M_OBJCLASS oc)
{
	switch (oc)
	{
	case M_BATTLESHIP:
	case M_LANCER:

	case M_FRIGATE:
	case M_SCOUTCARRIER:   //both a light and medium gunboat now.  don't think will cause problems -aeh
	case M_HIVECARRIER:
	case M_SCARAB:

	case M_POLARIS:
	case M_TRIREME:

	case M_COBRA:
	case M_BASILISK:
	case M_CROTAL:
	case M_MYSTIC:
		return true;

	default:
		return false;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsHeavyGunboat (enum M_OBJCLASS oc)
{
	switch (oc)
	{
	case M_CARRIER:
	case M_DREADNOUGHT:
	
	case M_TIAMAT:
	
	case M_MONOLITH:

	case M_LEVIATHIN:
		return true;

	default:
		return false;
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsCarrier (enum M_OBJCLASS oc)
{
	if (oc == M_CARRIER || oc == M_SCOUTCARRIER || oc == M_HIVECARRIER || oc == M_TIAMAT)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsTroopship (enum M_OBJCLASS oc)
{
	if (oc == M_TROOPSHIP || oc == M_LEECH || oc == M_LEGIONAIRE || oc == M_ERTRAG)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsMinelayer (enum M_OBJCLASS oc)
{
	if (oc == M_SPINELAYER || oc == M_ATLAS)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsHarvester (const M_OBJCLASS oc)
{
	if (oc == M_HARVEST || oc == M_SIPHON || oc == M_GALIOT || oc == M_AGGREGATOR)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsFlagship (enum M_OBJCLASS oc)
{
	if (oc == M_FLAGSHIP || oc == M_WARLORD || oc == M_HIGHCOUNSEL)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsSupplyShip (enum M_OBJCLASS oc)
{
	if (oc == M_SUPPLY || oc == M_ZORAP || oc == M_STRATUM || oc == M_ANACONDA)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsFabricator (enum M_OBJCLASS oc)
{
	if (oc == M_FABRICATOR || oc == M_WEAVER || oc == M_FORGER || oc == M_SHAPER)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsJumpPlat (enum M_OBJCLASS oclass)
{
	if (oclass == M_JUMPPLAT)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::IsGunPlat (enum M_OBJCLASS oc)
{
	if(oc == M_LSAT || oc == M_SPACESTATION || oc == M_IONCANNON || oc == M_PLASMASPLITTER || 
		oc == M_VORAAKCANNON || oc == M_PROTEUS || oc == M_HYDROFOIL || oc == M_ESPCOIL || 
		oc == M_STARBURST)
		return true;

	return false;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetLastStreamID (void)
{
	return globalData.lastStreamID;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetLastStreamID (U32 id)
{
	globalData.lastStreamID = id;
}
//----------------------------------------------------------------------------------------------
//
U32 MGlobals::GetLastTeletypeID (void)
{
	return globalData.lastTeletypeID;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetLastTeletypeID (U32 id)
{
	globalData.lastTeletypeID = id;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::GetScriptUIControl (void)
{
	return globalData.bScriptUIControl;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetScriptUIControl (bool bScriptControl)
{
	globalData.bScriptUIControl = bScriptControl;
}
//----------------------------------------------------------------------------------------------
//
bool MGlobals::HasPlayerResigned (U32 slotID)
{
	CQASSERT(slotID < MAX_PLAYERS);
	return resignState[slotID];
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetPlayerResignedBySlot (U32 slotID)
{
	CQASSERT(slotID < MAX_PLAYERS);
	resignState[slotID] = true;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMissionName (U32 stringID)
{
	globalData.objectives.mission_name_stringID = stringID;
	EVENTSYS->Send(CQE_OBJECTIVES_CHANGE, 0);
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMissionID (U32 missionID)
{
	globalData.missionID = missionID;
}
//----------------------------------------------------------------------------------------------
//
const U32 MGlobals::GetMissionID (void)
{
	return globalData.missionID;
}
//----------------------------------------------------------------------------------------------
//
const U32 MGlobals::GetMissionName (void)
{
	return globalData.objectives.mission_name_stringID;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::SetMissionDescription (U32 stringID)
{
	globalData.objectives.overview_stringID = stringID;
	EVENTSYS->Send(CQE_OBJECTIVES_CHANGE, 0);
}
//----------------------------------------------------------------------------------------------
//
const U32 MGlobals::GetMissionDescription (void)
{
	return globalData.objectives.overview_stringID;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::AddToObjectiveList (U32 stringID, bool bSecondary)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == 0)
		{
			globalData.objectives.objective_stringID[i] = stringID;

			globalData.objectives.bObjectiveSecondary[i] = bSecondary;
			globalData.objectives.state[i] = MT_GlobalData::OBJECTIVES::Pending;

			EVENTSYS->Send(CQE_OBJECTIVE_ADDED, 0);
			return;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::RemoveFromObjectiveList (U32 stringID)
{
	int i = -1;
	int removeIndex = -1;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			removeIndex = i;
			break;
		}
	}

	// memcpy things around so that we don't have holes in the data...
	int last = MAX_MISSION_OBJECTIVES - 1;

	if (removeIndex != -1)
	{
		if (removeIndex != last)
		{
			int head = removeIndex+1;
			int del  = removeIndex;
			int num  = (MAX_MISSION_OBJECTIVES - head);

			U32 copySize =  num * sizeof(U32);
			memcpy(&globalData.objectives.objective_stringID[del], &globalData.objectives.objective_stringID[head], copySize);

			copySize = num * sizeof(globalData.objectives.state[del]);
			memcpy(&globalData.objectives.state[del], &globalData.objectives.state[head], copySize);
			
			copySize = num * sizeof(bool);
			memcpy(&globalData.objectives.bObjectiveSecondary[del], &globalData.objectives.bObjectiveSecondary[head], copySize);
			
			// zero out the last one
			globalData.objectives.objective_stringID[last] = 0;
			globalData.objectives.state[last] = MT_GlobalData::OBJECTIVES::Pending;
			globalData.objectives.bObjectiveSecondary[last] = 0;
		}
		else
		{
			// just zero the last index out
			globalData.objectives.objective_stringID[last] = 0;
			globalData.objectives.state[last] = MT_GlobalData::OBJECTIVES::Pending;
			globalData.objectives.bObjectiveSecondary[last] = 0;
		}

		EVENTSYS->Send(CQE_OBJECTIVES_CHANGE, 0);
	}
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::MarkObjectiveCompleted (U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			globalData.objectives.state[i] = MT_GlobalData::OBJECTIVES::Complete;
			EVENTSYS->Send(CQE_OBJECTIVES_CHANGE, 0);
			return;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
const bool MGlobals::IsObjectiveSecondary (const U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			return globalData.objectives.bObjectiveSecondary[i];
		}
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
const bool MGlobals::IsObjectiveCompleted (const U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			if (globalData.objectives.state[i] == MT_GlobalData::OBJECTIVES::Complete)
			{
				return true;
			}
			else 
			{
				return false;
			}
		}
	}

	return false;
}
//----------------------------------------------------------------------------------------------
//
void MGlobals::MarkObjectiveFailed (U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			globalData.objectives.state[i] = MT_GlobalData::OBJECTIVES::Failed;
			EVENTSYS->Send(CQE_OBJECTIVES_CHANGE, 0);
			return;
		}
	}
}
//----------------------------------------------------------------------------------------------
//
const bool MGlobals::IsObjectiveFailed (const U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			if (globalData.objectives.state[i] == MT_GlobalData::OBJECTIVES::Failed)
			{
				return true;
			}
			else 
			{
				return false;
			}
		}
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
const bool MGlobals::IsObjectiveInList (const U32 stringID)
{
	int i;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == stringID)
		{
			return true;
		}
	}
	return false;
}
//----------------------------------------------------------------------------------------------
//
const U32 MGlobals::GetNumberObjectives (void)
{
	int i;
	U32 count = 0;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == 0)
		{
			continue;
		}
		count++;
	}

	return count;
}
//----------------------------------------------------------------------------------------------
//
const U32 MGlobals::GetObjectiveStringID (const U32 index)
{	
	int i;
	U32 count = 0;

	for (i=0; i<MAX_MISSION_OBJECTIVES; i++)
	{
		if (globalData.objectives.objective_stringID[i] == 0)
		{
			continue;
		}
		else
		{
			// return the index'th valid string ID
			if (count == index)
			{
				return globalData.objectives.objective_stringID[i];
			}
		}
		count++;
	}

	return 0;
}
//----------------------------------------------------------------------------------------------
//
struct _globalViewer : GlobalComponent
{
	GlobalViewer* viewer;

	virtual void Startup (void)
	{
		viewer = new DAComponent<GlobalViewer>;
		AddToGlobalCleanupList(&viewer);
	}
	
	virtual void Initialize (void)
	{
		viewer->init();
	}
};

static _globalViewer globalViewer;

void updateGlobalViewer (void)
{
	if (globalViewer.viewer->doc1)
	{
		BOOL32 state = 0;
		if (globalViewer.viewer->viewer!=0)
			globalViewer.viewer->viewer->get_display_state(&state);
		if (state==0)
		{
			globalViewer.viewer->viewer.free();
			globalViewer.viewer->doc1.free();
		}
		else
			globalViewer.viewer->doc1->UpdateAllClients();
	}

	if (globalViewer.viewer->doc3==0)
		globalViewer.viewer->createViewer3();
}

//--------------------------------------------------------------------------//
//----------------------------END MGlobals.cpp------------------------------//
//--------------------------------------------------------------------------//
