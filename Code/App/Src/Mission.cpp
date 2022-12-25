//--------------------------------------------------------------------------//
//                                                                          //
//                                Mission.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Mission.cpp 322   10/18/02 2:36p Tmauer $

   Game state management
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "Mission.h"
#include "Resource.h"
#include "UserDefaults.h"
#include "ObjList.h"
#include "Sector.h"
#include "NetFileTransfer.h"
#include "netpacket.h"
#include "Startup.h"
#include "Menu.h"
#include "CQTrace.h"
#include "Cursor.h"
#include "Hotkeys.h"
#include "CQGame.h"
#include "BaseHotrect.h"
#include "MGlobals.h"
#include <DBaseData.h>
#include "OpAgent.h"
#include "IObject.h"
#include "Camera.h"
#include "MusicManager.h"
#include "CQLight.h"
#include "FogOfWar.h"
#include "MPart.h"
#include "ScrollingText.h"
#include "MapGen.h"
#include <MScript.h>
#include "Commpacket.h"
#include "IMovieCamera.h"
#include "INugget.h"
#include "IFabricator.h"
#include "ISPlayerAI.h"
#include "IGameProgress.h"
#include "InProgressAnim.h"
#include "IAnimate.h"
#include "IStatic.h"
#include "GenData.h"
#include "SysMap.h"
#include "DrawAgent.h"
#include "IShapeLoader.h"

#include <dplay.h>
#include <dplobby.h>
#include "ZoneLobby.h"
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <ViewCnst.h>
#include <WindowManager.h>
#include <MemFile.h>
#include <IUTFWriter.h>

#include <DAnimate.h>
#include <DStatic.h>

#include <stdio.h>

#define NAME_REG_KEY   "CQPlayerName"
#define DEFERRED_MISSION_END_TIME 5000

using namespace CQGAMETYPES;

static IViewConstructor2 * parser;

static bool g_bCheatsOn = false;

struct ENDGAME_PACKET : BASE_PACKET
{
	U8 allyMask;

	ENDGAME_PACKET (void)
	{
		type = PT_ENDGAME;
		dwSize = sizeof(*this);
	}
};

CQEXTERN SPLASHINFO CQ_SplashInfo; // in case bDeferredMissionEndNonFail is set to "true", do a splash screen

//--------------------------------------------------------------------------//
//
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

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Mission : public IMission, 
										IEventCallback, 
										ISystemEventCallback,
										IResourceClient,
										IFileTransferCallback
{
	BEGIN_DACOM_MAP_INBOUND(Mission)
	DACOM_INTERFACE_ENTRY(IMission)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(ISystemEventCallback)
	DACOM_INTERFACE_ENTRY(IFileTransferCallback)
	END_DACOM_MAP()

	//--------------------------------
	// data items go here
	//--------------------------------
	
	U32 eventHandle, systemEventHandle, fileTransferHandle;		// connection handles
	
  	//
	// mission save / load
	//
	COMPTR<IFileSystem> inFile;
	BOOL32 bUnsavedData, bDownloadNeeded, bRuseEnabled;
	HANDLE hPrevSymbols;
	FTCHANNEL * downloadChannel;		// remote download of mission file
	FTSTATUS downloadStatus;
	BOOL32 bHasFocus;
	BOOL32 bInterfaceLoaded;
	//
	// sinple player ai
	//
	COMPTR<ISPlayerAI> playerAI[MAX_PLAYERS];
	COMPTR<IPANIM> ipAnim; // the progress animation
	SINGLE baseObjPercent;		// starting percent for loading objlist
	SINGLE totalObjPercent;		// total percent for loading objlist


	U32    endCheckedCount;			// update count when last checked for end-of-game

	U32    deferredMissionWinners;	// ally mask of winning team
	S32    deferredMissionEndTime;
	BOOL32 bDeferredMissionEndNonFail;

	USER_DEFAULTS initialDefaults;	// check against for cheating
	bool bNetloadProgressAnimation;
	bool bOnlyHumanPlayerMsgNeeded;

	//--------------------------------
	// class methods
	//--------------------------------
	
	Mission (void);

	~Mission (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	/* IMission methods  */
	
	DEFMETHOD_(BOOL32,Load) (const C8 * fileName = 0, struct IComponentFactory * pParent = 0);

	DEFMETHOD_(BOOL32,Save) (const C8 * fileName = 0, struct IComponentFactory * pParent = 0);

	DEFMETHOD_(BOOL32,LoadFromDescription) (const wchar_t * fileDesc = 0, struct IComponentFactory * pParent = 0);

	DEFMETHOD_(BOOL32,SaveByDescription) (const wchar_t * fileDesc = 0, struct IComponentFactory * pParent = 0, const char * szOverwrite = 0, U32 saveKey = 0);

	virtual BOOL32 __stdcall LoadBriefing (const char * fileName);

	DEFMETHOD_(BOOL32,QuickSave) (const C8 * fileName = 0, struct IComponentFactory * pParent = 0);

	DEFMETHOD_(BOOL32,DynamicSave) (const C8 * fileName = 0, struct IComponentFactory * pParent = 0);

	DEFMETHOD_(BOOL32,QuickSaveFile) (void);

	DEFMETHOD_(BOOL32,QuickLoadFile) (void);

	DEFMETHOD_(BOOL32,New) ();
	
	DEFMETHOD_(void,Close) (BOOL32 bForceIt);

	DEFMETHOD_(BOOL32,SetUnsavedData) (void);
	
	DEFMETHOD(CorrelateSymbol) (const C8 *pSymbolName, void *pOldData, void *pNewData);

	DEFMETHOD_(void,Update) (void);

	DEFMETHOD_(void,SetWindowTitle) (void);

	DEFMETHOD_(void,StartNetDownload) (void);

	DEFMETHOD_(void,CancelNetDownload) (void);

	DEFMETHOD_(enum FTSTATUS,GetDownloadStatus) (void);

	virtual GENRESULT __stdcall GetFileSystem (struct IFileSystem **ppFile);

	virtual void __stdcall SetFileSystem (struct IFileSystem * pFile);

	virtual BOOL32 __stdcall Reload (void);	// reload current file

	virtual BOOL32 __stdcall ReloadMission (void);	// reload current file

	virtual void __stdcall InitializeNetGame (const struct FULLCQGAME & cqgame, U32 randomSeed);

	virtual void __stdcall GenerateMultiplayerMap (const struct FULLCQGAME & cqgame, U32 randomSeed);

	virtual void __stdcall EndMission (U32 allyMask, bool bNonWinLose);

	virtual U32 __stdcall GetNumAIPlayers (void);		// for accounting

	virtual U32 __stdcall GetFileDescription (const C8 * fileName, wchar_t * string, U32 numMaxChars);
	
	virtual U32 __stdcall GetFileDescription (IFileSystem * fileDir, const C8 * fileName, wchar_t * string, U32 numMaxChars);

	virtual U32 __stdcall GetFileMaxPlayers (IFileSystem * fileDir, const C8 * fileName, U32 & maxPlayers);

	virtual U32 __stdcall GetFileMaxPlayers (const C8 * fileName, U32 & maxPlayers);

	virtual void LoadInterface (void);

	virtual void __stdcall SetAIResign (U32 playerID)
	{
		CQASSERT(THEMATRIX->IsMaster());
		CQASSERT(IsComputerControlled(playerID));		// must be computer controlled

		AIRESIGN_PACKET packet;
		packet.dwSize = sizeof(packet);
		packet.playerID = playerID;
		NETPACKET->Send(0, NETF_ALLREMOTE | NETF_ECHO, &packet);
	}

	//AI CONTROLLING METHODS FOR MISSION SCRIPTING

	virtual bool __stdcall IsComputerControlled (U32 playerID);

	virtual void __stdcall EnableEnemyAI(U32 playerID, bool bOn, enum M_RACE race);
	
	virtual void __stdcall EnableEnemyAI(U32 playerID, bool bOn, const char * szPlayerAIType);

	virtual void __stdcall SetEnemyAIRules(U32 playerID, const struct AIPersonality & rules);

	virtual void __stdcall SetEnemyAITarget(U32 playerID, IBaseObject *obj, U32 range, U32 systemID = 0);

	virtual void __stdcall LaunchOffensive (U32 playerID, UNIT_STANCE stance);

	virtual void __stdcall SetInitialCheatState (void)
	{
		initialDefaults = *DEFAULTS->GetDefaults();
		memset(&initialDefaults, 0, daoffsetofmember(USER_DEFAULTS, gammaCorrection)+dasizeofmember(USER_DEFAULTS, gammaCorrection));
	}

	virtual bool __stdcall IsSinglePlayerGame (void)
	{
		return MGlobals::GetGameSettings().gameType == CQGAMETYPES::MISSION_DEFINED;
	}

	virtual void __stdcall SetFileDescription(const wchar_t * desc);

	virtual bool __stdcall IsMouseEnabled (void);

	virtual void __stdcall CancelProgressAnim (void)
	{
		ipAnim.free();
		bNetloadProgressAnimation = false;
	}

	virtual void __stdcall StartProgressAnim (struct IPANIM ** ppAnim);

	virtual void __stdcall SetObjlistLoadProgress (SINGLE percentLoaded)
	{
		if (percentLoaded >= 1.0)
			percentLoaded = 1;

		if (ipAnim!=0 && totalObjPercent!=0)
		{
			percentLoaded = (totalObjPercent*percentLoaded) + baseObjPercent;
			ipAnim->SetProgress(percentLoaded);
		}
	}

	virtual const M_RACE __stdcall GetSinglePlayerRace (void) const;

	virtual void __stdcall SetSinglePlayerRace (const enum M_RACE race);

	virtual const struct CQGAME & GetGameSettings (void);

	virtual void __stdcall ToggleCheating (void);

	virtual const bool _stdcall GetCheatsEnabled (void) const;

	virtual COLORREF __stdcall GetPlayerColorForDPID (U32 dpid);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IResourceClient methods */

	DEFMETHOD_(BOOL32,Notify) (struct IResource *res, U32 message, void *parm=0);

	/* ISystemEventCallback methods */

	DEFMETHOD_(BOOL32,OnAppClose) (void);

	/* IFileTransferCallback methods */

	DEFMETHOD(RemoteFileRequest) (const C8 *fileName, IFileSystem ** file);
	
	/* Mission methods  */

	void SetMissionMRUList (HMENU hMenu);

	BOOL32 SaveAs (const C8 * fileName=0, struct IComponentFactory * pParent = 0);

	BOOL32 QuickSaveAs (const C8 * fileName=0, struct IComponentFactory * pParent = 0);

	BOOL32 DynamicSaveAs (const C8 * fileName=0, struct IComponentFactory * pParent = 0);

	void LoadFromFilter();

	BOOL32 LoadMission (U32 id);

	BOOL32 SaveParseData (void);

	BOOL32 LoadParseData (IFileSystem * inFile);

	BOOL32 close (void);

	void updateNetFile (void);
	
	void unloadInterface (void);

	void enableSpectatorMode (void);

	void checkEndGame (void);
	
	void reloadGametypes (void);
	
	void destroyPlayerAI (void);
	
	void startPlayerAI (void);
	
	bool savePlayerAI (struct IFileSystem * outFile);
	
	void loadPlayerAI (struct IFileSystem * inFile);
	
	void loadPlayerAI (struct IFileSystem * inFile, U32 playerID);

	void createPlayerAI (M_RACE race, U32 playerID, ISPlayerAI ** ppPlayerAI);
	void calculateCtrlO_Scores (void);
	void explodePlayerBombs (void);

	void setAIResign (U32 playerID);

	void setDefaultCheatSettings (void);

	bool getDiscFreeSpace (LPCTSTR lpszPath, DWORDLONG* pnFree);

	IDAComponent * GetBase (void)
	{
		return (IMission *) this;
	}
};
//--------------------------------------------------------------------------
//
inline static void * GetPreprocessData (int & resourceSize)
{
	HRSRC hRes;

	if ((hRes = FindResource(hResource, MAKEINTRESOURCE(IDR_PARSER1), "PARSER")) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(hResource, hRes)) != 0)
		{
			void * ptr = LockResource(hGlobal);
			resourceSize = SizeofResource(hResource, hRes);
			return ptr;
		}
	}
	return 0;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
Mission::Mission (void)
{
	// nothing else to init so far?
	bHasFocus = 1;
}
//--------------------------------------------------------------------------//
//
Mission::~Mission (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS && GS->QueryOutgoingInterface("ISystemEventCallback", connection) == GR_OK)
		connection->Unadvise(systemEventHandle);
	
	if (FULLSCREEN && FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	if (FILETRANSFER && FILETRANSFER->QueryOutgoingInterface("IFileTransferCallback", connection) == GR_OK)
		connection->Unadvise(fileTransferHandle);

	if (parser)
	{
		if (hPrevSymbols)
			parser->DestroySymbols(hPrevSymbols);
		hPrevSymbols = 0;
		parser->Release();
	}
	parser = 0;

	if (downloadChannel && FILETRANSFER)
		FILETRANSFER->CloseChannel(downloadChannel);
	downloadChannel = 0;
}
//--------------------------------------------------------------------------//
//
void Mission::LoadInterface (void)
{
	if (bInterfaceLoaded==0)
	{
		CQASSERT(CQFLAGS.b3DEnabled!=0);

		M_RACE race;
		race = MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());

		bInterfaceLoaded = 1;
		EVENTSYS->Send(CQE_LOAD_TOOLBAR, &race);
	}
}
//-------------------------------------------------------------------
//
void Mission::unloadInterface (void)
{
	if (bInterfaceLoaded)
	{
		bInterfaceLoaded = 0;
		EVENTSYS->Send(CQE_LOAD_TOOLBAR, NULL);
	}
}
//-------------------------------------------------------------------
//
void Mission::destroyPlayerAI (void)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++)
		playerAI[i].free();
}
//-------------------------------------------------------------------
//
static bool isComputerPlayerAssigned (const struct CQGAME & cqgame, U32 playerID)
{
	int i, j = cqgame.activeSlots;

	for (i = 0; i < j; i++)
	{
		if (cqgame.slot[i].type == COMPUTER && MGlobals::GetPlayerIDFromSlot(i) == playerID)
			return true;
	}

	return false;
}
//-------------------------------------------------------------------
// assumes that cqgame struct has been initialized
// must delete some computer players that aren't needed (if they were loaded from disk)
//
void Mission::startPlayerAI (void)
{
	int i, j;
	const struct CQGAME & cqgame = MGlobals::GetGameSettings();
	j = cqgame.activeSlots;

	if (CQFLAGS.bAITest)		// for AI into all READY slots
	{
		for (i = 0; i < j; i++)
		{
			int race = cqgame.slot[i].race;
			if (cqgame.slot[i].state == READY)
			{
				U32 playerID = MGlobals::GetPlayerIDFromSlot(i);

				if (playerID && playerID <= MAX_PLAYERS && playerAI[playerID-1]==0)
				{
					createPlayerAI((M_RACE)race, playerID, playerAI[playerID-1]);
				}
			}
		}
	}
	else
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (playerAI[i] != 0)
			{
				if (isComputerPlayerAssigned(cqgame, i+1) == false)
				{
					playerAI[i].free();
				}
			}
		}

		for (i = 0; i < j; i++)
		{
			int race = cqgame.slot[i].race;
			if (cqgame.slot[i].state == READY && cqgame.slot[i].type == COMPUTER)
			{
				U32 playerID = MGlobals::GetPlayerIDFromSlot(i);

				if (playerID && playerID <= MAX_PLAYERS && playerAI[playerID-1]==0)
				{
					createPlayerAI((M_RACE)race, playerID, playerAI[playerID-1]);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------
//
void Mission::loadPlayerAI (struct IFileSystem * inFile, U32 playerID)
{
	char buffer[MAX_PATH];
	CQASSERT(playerID && playerID <= MAX_PLAYERS);

	sprintf(buffer, "\\PLAYERAI\\Player%02d", playerID);

	if (inFile->SetCurrentDirectory(buffer))
	{
		WIN32_FIND_DATA data;
		HANDLE hFF;

		if ((hFF = inFile->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
		{
			if (inFile->SetCurrentDirectory(data.cFileName))
			{
				ISPlayerAI::CreateInstance(data.cFileName, playerID, playerAI[playerID-1]);
				if (playerAI[playerID-1])
				{
					if (playerAI[playerID-1]->Load(inFile) == 0)
					{
						playerAI[playerID-1].free();
					}
				}
#ifdef _DEBUG
				if (playerAI[playerID-1] == 0)
				{
					const struct CQGAME & cqgame = MGlobals::GetGameSettings();
					if (cqgame.gameType == MISSION_DEFINED)
						CQERROR0("Failed load on PlayerAI");
				}
#endif
			}

			inFile->FindClose(hFF);
		}
	}
}
//--------------------------------------------------------------------------
//
void Mission::loadPlayerAI (struct IFileSystem * inFile)
{
	const struct CQGAME & cqgame = MGlobals::GetGameSettings();

	// single player mission
	if (cqgame.gameType == MISSION_DEFINED)
	{
		for(U32 i = 2; i <=MAX_PLAYERS; ++i)
		{
			loadPlayerAI(inFile, i);
		}
	}
	else
	{
		int i, j;
		const struct CQGAME & cqgame = MGlobals::GetGameSettings();
		j = cqgame.activeSlots;

		for (i = 0; i < j; i++)
		{
			int race = cqgame.slot[i].race;
			if (cqgame.slot[i].type == COMPUTER)
			{
				U32 playerID = MGlobals::GetPlayerIDFromSlot(i);

				if (playerID && playerID <= MAX_PLAYERS && playerAI[playerID-1]==0)
				{
					loadPlayerAI(inFile, playerID);
					if (playerAI[playerID-1]==0)		// if load failed
						createPlayerAI((M_RACE)race, playerID, playerAI[playerID-1]);
				}
			}
		}
	}

}
//--------------------------------------------------------------------------
//
bool Mission::savePlayerAI (struct IFileSystem * outFile)
{
	if (PLAYERID==0 && GetNumAIPlayers())
	{
		char buffer[MAX_PATH];

		outFile->CreateDirectory("\\PLAYERAI");
		
		for (U32 i = 0; i < MAX_PLAYERS; i++)
		{		
			if (playerAI[i] != 0)
			{
				sprintf(buffer, "\\PLAYERAI\\Player%02d", i+1);
				if (outFile->CreateDirectory(buffer) == 0)
				{
					return false;
				}
				if (outFile->SetCurrentDirectory(buffer) == 0)
				{
					return false;
				}

				if (outFile->CreateDirectory(playerAI[i]->getSaveLoadName()) == 0)
				{
					return false;
				}
				if (outFile->SetCurrentDirectory(playerAI[i]->getSaveLoadName()) == 0)
				{
					return false;
				}

				if (playerAI[i]->Save(outFile) == 0)
				{
					return false;
				}
			}
		}
	}

	return true;
}
//-------------------------------------------------------------------
//
BOOL32 Mission::SetUnsavedData (void)
{
	bUnsavedData = 1;
	return 1;
}
//-------------------------------------------------------------------
//
GENRESULT Mission::CorrelateSymbol (const C8 *pSymbolName, void *pOldData, void *pNewData)
{
	SYMBOL oldSymbol, newSymbol;

	oldSymbol = parser->GetSymbol(hPrevSymbols, pSymbolName);
	newSymbol = parser->GetSymbol(0, pSymbolName);

	if (oldSymbol && newSymbol)
	{
		parser->CorrelateSymbol(oldSymbol, pOldData, newSymbol, pNewData);
		return GR_OK;
	}
	else
		return GR_GENERIC;
}
//-------------------------------------------------------------------
//
BOOL32 Mission::Load (const C8 * fileName, IComponentFactory * pParent)
{
	BOOL32 result=0;
	char buffer[MAX_PATH+4];
	DAFILEDESC fdesc = buffer;

	if (pParent == 0)
		pParent = DACOM;
	fdesc.lpImplementation = "UTF";
	buffer[0] = 0;
	if (fileName)
		strcpy(buffer, fileName);
	if (buffer[0] || DEFAULTS->GetInputFilename(buffer, IDS_MISSION_FILTER))
	{
		if (close() == 0)
			goto Done;

		if (pParent->CreateInstance(&fdesc, inFile) == GR_OK)
		{
			result = Reload();
		}
		else
			SetWindowTitle();

		bUnsavedData=0;
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
bool Mission::getDiscFreeSpace (LPCTSTR lpszPath, DWORDLONG* pnFree)
{
	// the typedef for the function pointer to get the available disk space
	typedef bool (CALLBACK* MYFUNCPTR)(LPCTSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER); 

	bool bRet = false;

	// We need to determine whether GetDiskFreeSpaceEx is available by calling LoadLibrary
	// or LoadLibraryEx, to load Kernel32.DLL, and then calling the GetProcAddress to
	// obtain an address for GetDiskFreeSpaceEx.  If GetProcAddress fails, or if
	// GetDiskFreeSpaceEx fails with the ERROR_CALL_NOT_IMPLEMENTED code, we use the
	// GetDiskFreeSpace function instead.	
	HINSTANCE hInstance = LoadLibrary("KERNEL32.DLL");

	if (hInstance)	// If we got the library
	{		
		MYFUNCPTR lpfnDLLProc = NULL;
		
		lpfnDLLProc = (MYFUNCPTR)GetProcAddress(hInstance, "GetDiskFreeSpaceExA");
		
		if (lpfnDLLProc)	// If we got an address to the function
		{	
			ULARGE_INTEGER nTotalBytes, nTotalFreeBytes, nTotalAvailable;

			if (lpfnDLLProc(lpszPath, &nTotalAvailable, &nTotalBytes, &nTotalFreeBytes))
			{
				*pnFree = nTotalAvailable.QuadPart;
				bRet = true;
			}		
		}

		FreeLibrary(hInstance);
	}

	if (!bRet)	// We have to try and use GetDiskFreeSpace()	
	{
		ULONG secpercluster, bytespersec, nooffreeclusters, totalnoofclusters;	

		if(GetDiskFreeSpace(lpszPath, &secpercluster, &bytespersec,
			&nooffreeclusters, &totalnoofclusters))
		{
			*pnFree = (nooffreeclusters * secpercluster * bytespersec);
			bRet = true;
		}				
	}

	return bRet;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::Save (const C8 * fileName, struct IComponentFactory * pParent)
{
	// make sure we have enough memory
	DWORDLONG dwlDiskSpace=0;
	if (getDiscFreeSpace(NULL, &dwlDiskSpace))
	{
		// we need to have at least one meg of memory around in order to save our game...
		if (dwlDiskSpace < 1024*1024)
		{
			// tell the user that they don't have enough available memory to save the game
			CQMessageBox(IDS_HELP_NODISCSPACE, IDS_ERROR, MB_OK);
			return FALSE;
		}
	}
	
	BOOL32 result = 0;
	COMPTR<IUTFWriter> pWriter;
	char buffer[MAX_PATH+4];

	if (inFile)
		inFile->GetFileName(buffer, sizeof(buffer));
	else
		buffer[0] = 0;
	
	if (inFile == 0 || (fileName != 0 && stricmp(fileName,buffer)!=0))	// no current file, or it's a different file
		return SaveAs(fileName, pParent);

	if (strstr(buffer, ".qmission"))
		return SaveAs();

	CURSOR->SetBusy(1);

	MGlobals::SetTerrainFilename("");

	if ((inFile->GetAccessType() & GENERIC_WRITE) == 0)
	{
		// file is not writeable
		COMPTR<IFileSystem> parentFile;
		if (inFile->GetParentSystem(parentFile) == GR_OK)
		{
			COMPTR<IComponentFactory> factory;
			inFile.free();
			DAFILEDESC fdesc = buffer;
			fdesc.lpImplementation = "UTF";
			fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			if (buffer[1] == ':') // file was opened through GDI 
				factory = DACOM;
			else
				factory = parentFile;
			parentFile.free();

			if (factory->CreateInstance(&fdesc, inFile) != GR_OK)
			{
				CQERROR0("Failed to create save file.");
				goto Done;		// failed
			}
		}
		else
		{
			CQERROR0("Failed to create get parent file system.");
			goto Done;		// failed
		}
	}

	inFile->SetCurrentDirectory("\\");
	RecursiveDelete(inFile);
#ifdef _DEBUG
	if (SaveParseData() == 0)
	{
		CQERROR0("Failed write to save game");
	}
#endif

	// any one of these saves can fail, no?
	SECTOR->Save(inFile);
	savePlayerAI(inFile);
	CAMERAMANAGER->Save(inFile);
	NUGGETMANAGER->Save(inFile);
	OBJLIST->Save(inFile);
	THEMATRIX->Save(inFile);
	MUSICMANAGER->Save(inFile);
	FOGOFWAR->Save(inFile);
	MScript::Save(inFile);
	MGlobals::Save(inFile);		// needs to save last, load first because it's referenced elsewhere


	if (inFile!=0 && inFile->QueryInterface("IUTFWriter", pWriter)==GR_OK)
	{
		pWriter->Flush();
		pWriter.free();
	}

	// re-open the file for read-only
	{
		COMPTR<IFileSystem> parentFile;
		if (inFile->GetParentSystem(parentFile) == GR_OK)
		{
			COMPTR<IComponentFactory> factory;
			inFile.free();
			DAFILEDESC fdesc = buffer;
			if (buffer[1] == ':') // file was opened through GDI 
				factory = DACOM;
			else
				factory = parentFile;
			parentFile.free();

			if (factory->CreateInstance(&fdesc, inFile) != GR_OK)
			{
				CQERROR0("Failed to re-open save file.");
				goto Done;		// failed
			}
		}
		else
		{
			CQERROR0("Failed to create get parent file system.");
			goto Done;		// failed
		}
	}


	SetWindowTitle();
	bUnsavedData=0;
	result=1;
Done:
	CURSOR->SetBusy(0);
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::LoadFromDescription (const wchar_t * fileDesc, IComponentFactory *pParent)
{
	return TRUE;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::SaveByDescription (const wchar_t * fileDesc, IComponentFactory *pParent, const char * szOverwrite, U32 saveKey)
{
	// set the new game description
	MGlobals::SetFileDescription(fileDesc);

	if (szOverwrite == NULL)
	{
		char szFile[MAX_PATH];

		if (IsSinglePlayerGame())
		{
			wsprintf(szFile, "f%07d", saveKey);
		}
		else
		{
			wsprintf(szFile, "m%07d", saveKey);			
		}
		
		return Save(szFile, pParent);
	}
	else
	{
		return Save(szOverwrite, pParent);
	}
}
//--------------------------------------------------------------------------//
//
U32 Mission::GetFileDescription (const C8 * fileName, wchar_t * string, U32 numMaxChars)
{
	return MGlobals::GetFileDescription(fileName, string, numMaxChars);
}
//--------------------------------------------------------------------------//
//
U32 Mission::GetFileDescription (IFileSystem * fileDir, const C8 * fileName, wchar_t * string, U32 numMaxChars)
{
	return MGlobals::GetFileDescription(fileDir, fileName, string, numMaxChars);
}
//--------------------------------------------------------------------------//
//
U32 Mission::GetFileMaxPlayers (IFileSystem * fileDir, const C8 * fileName, U32 & maxPlayers)
{
	return MGlobals::GetFileMaxPlayers (fileDir,fileName,maxPlayers);
}
//--------------------------------------------------------------------------//
//
U32 Mission::GetFileMaxPlayers (const C8 * fileName, U32 & maxPlayers)
{
	return MGlobals::GetFileMaxPlayers (fileName,maxPlayers);
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::QuickSave (const C8 * fileName, struct IComponentFactory * pParent)
{
	BOOL32 result = 0;
	COMPTR<IUTFWriter> pWriter;
 
	if (inFile == 0 || fileName != 0)
		return QuickSaveAs(fileName, pParent);

	CURSOR->SetBusy(1);

	if ((inFile->GetAccessType() & GENERIC_WRITE) == 0)
	{
		// file is not writeable
		COMPTR<IFileSystem> parentFile;
		if (inFile->GetParentSystem(parentFile) == GR_OK)
		{
			COMPTR<IComponentFactory> factory;
			char buffer[MAX_PATH+4];
			inFile->GetFileName(buffer, sizeof(buffer));
			inFile.free();
			DAFILEDESC fdesc = buffer;
			fdesc.lpImplementation = "UTF";
			fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			if (buffer[1] == ':') // file was opened through GDI 
				factory = DACOM;
			else
				factory = parentFile;
			parentFile.free();

			if (factory->CreateInstance(&fdesc, inFile) != GR_OK)
			{
				CQERROR0("Failed to create save file.");
				goto Done;		// failed
			}
		}
		else
		{
			CQERROR0("Failed to create get parent file system.");
			goto Done;		// failed
		}
	}

	inFile->SetCurrentDirectory("\\");
	RecursiveDelete(inFile);
	if (SaveParseData() == 0)
	{
		CQERROR0("Failed write to save game");
		goto Done;
	}
	SECTOR->QuickSave(inFile);
	OBJLIST->QuickSave(inFile);
	MUSICMANAGER->Save(inFile);
	MGlobals::QuickSave(inFile);

	if (inFile!=0 && inFile->QueryInterface("IUTFWriter", pWriter)==GR_OK)
		pWriter->Flush();

	SetWindowTitle();
	bUnsavedData=0;
	result=1;
Done:
	CURSOR->SetBusy(0);
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::QuickSaveFile (void)
{
	BOOL32 result;

	if (IsSinglePlayerGame())
	{
		SetFileDescription(_localLoadStringW(IDS_SAVE_QSMISSION));
		result = Save("f0000000.mission", SAVEDIR);
	}
	else
	{
		SetFileDescription(_localLoadStringW(IDS_SAVE_QSQUICKBATTLE));
		result = Save("m0000000.mission", SAVEDIR);
	}

	if (result == TRUE)
	{
		SCROLLTEXT->SetText(IDS_QUICKSAVE_SUCCESS);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::QuickLoadFile (void)
{
	// are we quick loading a quick battle or a single player game?
	char szDescription[32];
	memset(szDescription, 0, sizeof(szDescription));

	if (IsSinglePlayerGame())
	{
		strcpy(szDescription, "f0000000.mission");
	}
	else
	{
		strcpy(szDescription, "m0000000.mission");
	}
	
	// first make sure that the file exists!
	WIN32_FIND_DATA data;
	if (SAVEDIR && SAVEDIR->FindFirstFile(szDescription, &data) == INVALID_HANDLE_VALUE)
	{
		SCROLLTEXT->SetText(IDS_QUICKLOAD_FAILED);
		return FALSE;
	}
	else
	{
//		SCROLLTEXT->SetText(IDS_QUICKLOAD_WAIT);
		BOOL32 result = MISSION->Load(szDescription, SAVEDIR);
		SCROLLTEXT->SetText(IDS_QUICKLOAD_SUCCESS);
		return result;
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::DynamicSave (const C8 * fileName, struct IComponentFactory * pParent)
{
	BOOL32 result = 0;
	COMPTR<IUTFWriter> pWriter;
 
	if (inFile == 0 || fileName != 0)
		return DynamicSaveAs(fileName, pParent);

	CURSOR->SetBusy(1);

	if ((inFile->GetAccessType() & GENERIC_WRITE) == 0)
	{
		// file is not writeable
		COMPTR<IFileSystem> parentFile;
		if (inFile->GetParentSystem(parentFile) == GR_OK)
		{
			COMPTR<IComponentFactory> factory;
			char buffer[MAX_PATH+4];
			inFile->GetFileName(buffer, sizeof(buffer));
			inFile.free();
			DAFILEDESC fdesc = buffer;
			fdesc.lpImplementation = "UTF";
			fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			if (buffer[1] == ':') // file was opened through GDI 
				factory = DACOM;
			else
				factory = parentFile;
			parentFile.free();

			if (factory->CreateInstance(&fdesc, inFile) != GR_OK)
			{
				CQERROR0("Failed to create save file.");
				goto Done;		// failed
			}
		}
		else
		{
			CQERROR0("Failed to create get parent file system.");
			goto Done;		// failed
		}
	}

	inFile->SetCurrentDirectory("\\");
	RecursiveDelete(inFile);
	if (SaveParseData() == 0)
	{
		CQERROR0("Failed write to save game");
		goto Done;
	}
	OBJLIST->QuickSave(inFile,true);
	MGlobals::QuickSave(inFile);

	if (inFile!=0 && inFile->QueryInterface("IUTFWriter", pWriter)==GR_OK)
		pWriter->Flush();

	SetWindowTitle();
	bUnsavedData=0;
	result=1;
Done:
	CURSOR->SetBusy(0);
	return result;
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT Mission::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_GAME_ACTIVE:
		setDefaultCheatSettings();
		break;

	case CQE_NETPACKET:
		if (CQFLAGS.bGameActive)
		{
			BASE_PACKET * packet = (BASE_PACKET *) param;
			if (packet->type == PT_ENDGAME && packet->fromID == HOSTID)
			{
				ENDGAME_PACKET * p = (ENDGAME_PACKET *) packet;
				deferredMissionEndTime = DEFERRED_MISSION_END_TIME;
				deferredMissionWinners = p->allyMask;
			}
			else
			if (packet->type == PT_AIRESIGN)
			{
				AIRESIGN_PACKET * p = (AIRESIGN_PACKET *) packet;

				setAIResign(p->playerID);
			}
			else
			if (packet->type == PT_RESIGN)
			{
				U32 playerID = MGlobals::GetPlayerIDFromDPID(packet->fromID);
				U32 slotID = MGlobals::GetSlotIDFromDPID(packet->fromID);

				if (slotID < MAX_PLAYERS)
				{
					// tell the user that someone has resigned
					wchar_t msg[255];
					wchar_t name[64];

					// if the player is already marked as 'resigned' then don't print any messages
					// they've already been labelled has resigned if they have been defeated
					if (MGlobals::HasPlayerResigned(slotID) == false)
					{
						MGlobals::GetPlayerNameFromDPID(packet->fromID, name, sizeof(name));
						swprintf(msg, _localLoadStringW(IDS_FORMAT_PLAYERRESIGNED), name);
						SCROLLTEXT->SetTextString(msg);
						CQTRACE11("RESIGNATION -- %S", msg);
					}

					if (slotID < MAX_PLAYERS)
						MGlobals::SetPlayerResignedBySlot(slotID);
					MGlobals::RemoveDPIDFromPlayerID(packet->fromID, playerID);

					if (ZONESCORE)
					{
						U32 slots[MAX_PLAYERS];
						U32 numSlots = MGlobals::GetSlotIDForPlayerID(playerID, slots);
						U32 allResigning = (MGlobals::GetPlayerDPIDForPlayerID(playerID) == 0);
						U32 flags;
						U32 i;

						for (i = 0; i < numSlots; i++)
						{
							if (allResigning || slotID == slots[i])
							{
								flags = ZONESCORE_RESIGNED|ZONESCORE_LOSER;
								if (slotID != slots[i])   // assume computer player
									flags |= ZONESCORE_COMPUTERPLAYER;
								ZONESCORE->SetPlayerFlags(MGlobals::GetZoneSeatFromSlot(slots[i]), flags);
							}
						}
					}

					if (THEMATRIX->IsMaster())
					{
						if (MGlobals::GetPlayerDPIDForPlayerID(playerID) == 0)	// no more humans for this player
							THEMATRIX->TerminatePlayerUnits(playerID);
					}
				}
			}
			else
			if (packet->type == PT_PLAYERLOST && packet->fromID == HOSTID)
			{
				PLAYERLOST_PACKET * p;
				p = (PLAYERLOST_PACKET *) packet;

				// scrolling text message
				wchar_t msg[255];
				wchar_t name[64];

				// arrrgh!  Go through all the slots, see if there is a match for the playerID, get the
				// name(s) and post the message
				const CQGAME & cqgame = MGlobals::GetGameSettings();
				for (U32 i = 0; i < cqgame.activeSlots; i++)
				{
					U32 id = MGlobals::GetPlayerIDFromSlot(i);

					if (id == p->playerID)
					{
						U32 result = MGlobals::GetPlayerNameBySlot(i, name, sizeof(name));

						// if there was no name, then the player must be a computer
						if (result == 0)
						{
							wcsncpy(name, _localLoadStringW(IDS_COMPUTER_NAME), 32);
						}

						swprintf(msg, _localLoadStringW(IDS_FORMAT_PLAYEROUT), name);
						SCROLLTEXT->SetTextString(msg);
						CQTRACE11("LOSING -- %S", msg);

						MGlobals::SetPlayerResignedBySlot(i);

						if (ZONESCORE)
						{
							U32 flags = ZONESCORE_LOSER;
							if (result == 0 || cqgame.slot[i].dpid==0)
								flags |= ZONESCORE_COMPUTERPLAYER;
							ZONESCORE->SetPlayerFlags(MGlobals::GetZoneSeatFromSlot(i), flags);
						}
					}
				}
				
				// if you are not spectating, then leave the game, otherwise have some nice big teletype text come
				// up telling the user that he lost so he knows for sure that he's a loser and is just spectating
				if (p->playerID == MGlobals::GetThisPlayer())
				{
					if (IsSinglePlayerGame() == false)
					{
						// if we are allowed to spectate, then spectate dammit
						if (DEFAULTS->GetDefaults()->bSpectatorModeAllowed)
						{
							enableSpectatorMode();
						}
						else
						{
							// end the game for us
							deferredMissionEndTime = DEFERRED_MISSION_END_TIME;
							deferredMissionWinners = ~MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
						}
					}
					else
					{
						// we are playing a single player game, end the game
						deferredMissionWinners = ~MGlobals::GetAllyMask(MGlobals::GetThisPlayer());
						deferredMissionEndTime = 1;
					}
				}
			}
		}
		break;
	case CQE_KILL_FOCUS:
		if ((Mission *)param != this)
			bHasFocus = FALSE;
		break;
	case CQE_SET_FOCUS:
		bHasFocus = TRUE;
		break;

	case CQE_UPDATE:
		// is it time to put a an endmission message box?
		if (CQFLAGS.bGamePaused == false && deferredMissionEndTime!=0 && bHasFocus)
		{
			deferredMissionEndTime -= S32(param)>>10;
			if (deferredMissionEndTime <= 0)
			{
				deferredMissionEndTime = 0;
				bOnlyHumanPlayerMsgNeeded = 0;
				EndMission(deferredMissionWinners,bDeferredMissionEndNonFail != false);
			}
		}
		updateNetFile();
		if (CQFLAGS.bGameActive)
		{
			const U32 updateCount = MGlobals::GetUpdateCount();
			const U32 updateDiff = updateCount - endCheckedCount;			// number of times we have updated AI
			if (updateDiff > U32(RENDER_FRAMERATE)*5)
			{
				endCheckedCount = updateCount;
				checkEndGame();
			}
			if (PLAYERID!=0)
			{
				if (updateCount > ((U32)RENDER_FRAMERATE)*60*10)		// 10 minutes
				{
					const USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();
							// uh-oh! someone's illegally cheating!
							// make game not-so-good
					if (defaults.fogMode != initialDefaults.fogMode ||
						(defaults.bSpectatorModeOn==0 && defaults.bVisibilityRulesOff != initialDefaults.bVisibilityRulesOff) ||
						defaults.gameSpeed != initialDefaults.gameSpeed ||
						defaults.bCheatsEnabled != initialDefaults.bCheatsEnabled || 
						defaults.bLockDiplomacy != initialDefaults.bLockDiplomacy ||
						defaults.bSpectatorModeAllowed != initialDefaults.bSpectatorModeAllowed ||
						(defaults.bSpectatorModeOn && initialDefaults.bSpectatorModeAllowed==0) )
					{
						EVENTSYS->Post(CQE_HOTKEY, (void *)IDH_GAME_OPTIONS);
					}
				}
			}
			if (bOnlyHumanPlayerMsgNeeded && bHasFocus)
			{
				bOnlyHumanPlayerMsgNeeded = false;
				// notify the player he is the only human left playing
				if (deferredMissionEndTime==0)		// don't do this if mission is ending
					CQMessageBox(IDS_HELP_NOHUMANS, IDS_APP_NAMETM, MB_OK);
			}
		}
		break;


	case CQE_HOTKEY:
		switch (U32(param))
		{
		case IDH_WIN_CLOSE:
			PostMessage(hMainWindow, WM_CLOSE, 0, 0);
			break;
		} 
		break; // end case CQE_HOTKEY

	case WM_CLOSE:
		ipAnim.free();		// make sure background drawing is shutdown
		break;

	case CQE_ENTERING_INGAMEMODE:
		EVENTSYS->Send(CQE_LOAD_TOOLBAR, 0);		// unload interface
		break;

	case CQE_DELETEPLAYER:
		if (U32(param) != PLAYERID)		// ignore booting case
		{
			U32 playerID = MGlobals::GetPlayerIDFromDPID(U32(param));
			if (playerID)
			{
				// notify everyone that a player has left the building
				// has left the game
				// scrolling text message

				U32 slotID = MGlobals::GetSlotIDFromDPID(U32(param));
				if (slotID < MAX_PLAYERS)
				{
					wchar_t msg[255];
					wchar_t name[64];

					U32 nPlayers;
					nPlayers = MGlobals::GetPlayerDPIDForPlayerID(playerID, NULL);

					U32 result = MGlobals::GetPlayerNameBySlot(slotID, name, sizeof(name));

					CQASSERT(result);		// cannot be a computer player here!
					swprintf(msg, _localLoadStringW(IDS_FORMAT_PLAYERLEFT), name);
					SCROLLTEXT->SetTextString(msg);
					CQTRACE11("QUITTING -- %S", msg);

					MGlobals::RemoveDPIDFromPlayerID(U32(param), playerID);
					if (MGlobals::HasPlayerResigned(slotID) == false)
					{
						if (ZONESCORE)
							ZONESCORE->SetPlayerFlags(MGlobals::GetZoneSeatFromSlot(slotID), ZONESCORE_DISCONNECTED);
						
						if (nPlayers == 1)
						{
							if (ZONESCORE)
							{
								//
								// add a new player for the zone to track
								//
								char buffer[64];
								U32 dwSeat;
								strncpy(buffer, _localLoadString(IDS_COMPUTER_NAME), sizeof(buffer));
								ZONESCORE->AddNewPlayer(buffer, 0, MGlobals::GetAllyMask(playerID), ZONESCORE_COMPUTERPLAYER, &dwSeat);
								MGlobals::SetZoneSeatFromSlot(slotID, dwSeat);
							}
							// no more human players, create a computer player
							createPlayerAI(MGlobals::GetPlayerRace(playerID), playerID, playerAI[playerID-1]);
						}
					}
				}
			}
			// are there any human oppopents left?
			if (CQFLAGS.bGameActive && deferredMissionEndTime==0 && MGlobals::IsHostOnlyPlayerLeft())
			{
				bOnlyHumanPlayerMsgNeeded = true;
			}
		}
		break;

	case CQE_MISSION_PROG_ENDING:
		deferredMissionEndTime = DEFERRED_MISSION_END_TIME;
		deferredMissionWinners = U32(param);
		break;

	case CQE_MISSION_ENDING_SPLASH:
		deferredMissionEndTime = 500; // end 1/2 second latter
		deferredMissionWinners = 0;
		bDeferredMissionEndNonFail = true;
		memcpy( &CQ_SplashInfo, (SPLASHINFO*)param, sizeof(CQ_SplashInfo) );
		break;

	case CQE_PLAYER_RESIGN:
		EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_RESIGN);			// go back to shell
		EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_RESIGN);		// send result code
		destroyPlayerAI();
		break;
	case CQE_PLAYER_QUIT:
		EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_QUIT);			// go back to shell
		EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_QUIT);		// send result code
		destroyPlayerAI();
		break;

	case WM_CHAR:
		if (bHasFocus)
		switch (TCHAR(msg->wParam))
		{
		case 27:		//ESC
			if (CQFLAGS.bGameActive)
			{
				U32 igResult = CreateMenu_igoptions();
				if(igResult)		// player wants to resign
				{
					if(igResult == 1)
					{
						EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_RESIGN);			// go back to shell
						EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_RESIGN);		// send result code
						destroyPlayerAI();
					}
					else
					{
						EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_QUIT);			// go back to shell
						EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_QUIT);		// send result code
						destroyPlayerAI();
					}
				}
			}
			break;
		}
		break;  // end case WM_CHAR

	case WM_INITMENUPOPUP:
		if (HIWORD(msg->lParam)==0)
		{
			if (LOWORD(msg->lParam) == MENUPOS_MRU_MISSION)
				SetMissionMRUList((HMENU)msg->wParam);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(msg->wParam))
		{
		case IDM_RELOAD_DATABASE:
			reloadGametypes();
			break;

		case IDM_MRU0_MISSION:
		case IDM_MRU1_MISSION:
		case IDM_MRU2_MISSION:
		case IDM_MRU3_MISSION:
			EVENTSYS->Send(CQE_NEW_SELECTION, this);
			LoadMission(LOWORD(msg->wParam));
			break;
		
		case IDM_OPEN_MISSION:
			EVENTSYS->Send(CQE_NEW_SELECTION, this);
			if (Load()==0)
				New();
			break;
		
		case IDM_LOAD_TERRAIN:
			EVENTSYS->Send(CQE_NEW_SELECTION, this);
			if (Load()==0)
				New();
			if (inFile)
			{
				char buffer[MAX_PATH+4];
				inFile->GetFileName(buffer,sizeof(buffer));
				char * ptr = strrchr(buffer,'\\');
				if(!ptr)
					ptr = buffer;
				MGlobals::SetTerrainFilename(ptr);
				inFile.free();
				SetWindowTitle();
			}
			break;

		/*
		case IDM_CLOSE_MISSION:
			EVENTSYS->Send(CQE_NEW_SELECTION, this);
			close();
			break;
		*/

		case IDM_NEW_MISSION:
			EVENTSYS->Send(CQE_NEW_SELECTION, this);
			if (close())
			{
				New();
			}
			break;

		case IDM_ENABLE_RUSE:
			if (CQFLAGS.bGameActive && QuickSave() && inFile!=0)
			{
				bRuseEnabled = 1;
				CQFLAGS.bGameActive = 0;
				ActivateRUSE(inFile);
			}
			break;

 		case IDM_RUSE_EXITMAPEDITOR:
			if (bRuseEnabled && DeactivateRUSE())
			{
				bRuseEnabled = 0;
				CQFLAGS.bGameActive = 1;
				Reload();
//				SendMessage(hMainWindow, WM_COMMAND, IDM_MRU0_MISSION, 0);
			}
			break;
		
		case IDM_SAVE_MISSION:
			Save();
			break;

		case IDM_SAVEAS_MISSION:
			SaveAs();
			break;

		case IDM_QUICK_SAVE:
			{
				char buffer[MAX_PATH+4];
				MGlobals::GetTerrainFilename(buffer,sizeof(buffer));
				if(buffer[0])
					DynamicSave();
				else
					QuickSave();
			}
			break;

		case IDM_QUICK_SAVE_AS:
			{
				char buffer[MAX_PATH+4];
				MGlobals::GetTerrainFilename(buffer,sizeof(buffer));
				if(buffer[0])
					DynamicSaveAs();
				else
					QuickSaveAs();
			}
			break;

		case IDM_LOAD_FILTER:
			LoadFromFilter();
			break;

		case IDM_ALL_TECH:
			{
				TECHNODE node;
				memset(&node, 0, sizeof(node));
				node.InitLevel(TECHTREE::FULL_TREE);
				for(U32 i = 1; i <=MAX_PLAYERS; ++i)
				{
					MGlobals::SetCurrentTechLevel(node,i);
				}
			}
			break;

		case IDM_FIX_CP:
			{
				for(U32 i = 1; i <= MAX_PLAYERS; ++i)
				{
					MGlobals::SetCurrentUsedComPts(i,0);
					if(MGlobals::GetMaxControlPoints(i) == 0)
						MGlobals::SetMaxControlPoints(i,100);
				}
				IBaseObject * obj = OBJLIST->GetTargetList();
				while(obj)
				{
					MPart part(obj);
					if(part.isValid() && part->bUnderCommand)
					{
						U32 playerID = obj->GetPlayerID();
						if(playerID)
							MGlobals::SetCurrentUsedComPts(playerID,MGlobals::GetCurrentUsedComPts(playerID)+part.pInit->resourceCost.commandPt);
					}
					obj = obj->nextTarget;
				}
			}
			break;
		}
		break;

	}

	MScript::Notify(message, param);

	return GR_OK;
}
//-------------------------------------------------------------------
//
void Mission::setAIResign (U32 playerID)
{
	U32 slots[MAX_PLAYERS];
	U32 numSlots = MGlobals::GetSlotIDForPlayerID(playerID, slots);

	if (numSlots)
	{
		U32 index;
		const struct CQGAME & cqgame = MGlobals::GetGameSettings();

		for (index = 0; index < numSlots; index++)
		{
			if (MGlobals::HasPlayerResigned(slots[index]) == 0)
			{
				// tell the user that someone has resigned
				wchar_t msg[255];
				wchar_t name[64];

				MGlobals::GetPlayerNameBySlot(slots[index], name, sizeof(name));
				swprintf(msg, _localLoadStringW(IDS_FORMAT_PLAYERRESIGNED), name);
				SCROLLTEXT->SetTextString(msg);
				CQTRACE11("RESIGNATION -- %S", msg);

				MGlobals::SetPlayerResignedBySlot(slots[index]);
				MGlobals::RemoveDPIDFromPlayerID(cqgame.slot[slots[index]].dpid, playerID);
			}
		}

		if (THEMATRIX->IsMaster())
		{
			THEMATRIX->TerminatePlayerUnits(playerID);
		}

		playerAI[playerID-1].free();
	}
}
//-------------------------------------------------------------------
//
void Mission::setDefaultCheatSettings (void)
{
	g_bCheatsOn = false;
	CQFLAGS.bInstantBuilding = false;
	CQFLAGS.bEverythingFree = false;
}
//-------------------------------------------------------------------
// receive notifications from resources
//
BOOL32 Mission::Notify (struct IResource *res, U32 message, void *parm)
{
	return GR_OK;
}

//-------------------------------------------------------------------
//
void Mission::Update()
{
	if(CQFLAGS.bGamePaused==0 && CQFLAGS.bRTGamePaused==0)
	{
		__int64 pretick, posttick;
		QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
		THEMATRIX->Update();
		QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
		OBJLIST->AddTimerTicks(TIMING_OPAGENT, posttick-pretick);
		MScript::Update();
		if (THEMATRIX->IsMaster())
			calculateCtrlO_Scores();


		if (THEMATRIX->IsMaster())
		{
			int i;
			QueryPerformanceCounter((LARGE_INTEGER *)&pretick);
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (playerAI[i])
					playerAI[i]->Update();
			}
			QueryPerformanceCounter((LARGE_INTEGER *)&posttick);
			OBJLIST->AddTimerTicks(TIMING_PLAYERAI, posttick-pretick);
		}
	}
	else if(CQFLAGS.bGamePaused==0 && CQFLAGS.bRTGamePaused!=0)
	{
		MScript::Update();
	}
}
//-------------------------------------------------------------------
//
BOOL32 Mission::OnAppClose (void)
{
	USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();
	BOOL32 result=0;

//	if (bHasFocus==0)
//		return 0;
	if (CQFLAGS.bInProgressAnimActive)
		return TRUE;
	if (CQFLAGS.bFullScreenMovie)
		return FALSE;			// always exit without comment during a movie

	if (bRuseEnabled)
	{
		return (DeactivateRUSE() == 0);		// cancel if ruse did not deactivate
	}

	if (CQFLAGS.bNoExitConfirm == 0)
	{
		static bool bRecursion;
		if (bRecursion)
			return 1;						// cancel if recursion
		bRecursion = true;
		if (CQMessageBox(IDS_CONFIRM_QUIT, IDS_CONFIRM, MB_OKCANCEL) == 0)
		{
			bRecursion = false;
			return 1;							// cancel if user changed his mind
		}
		bRecursion = false;
	}

	if (CQFLAGS.bGameActive && PLAYERID)
	{
		CQFLAGS.bGameActive = 0;
		EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)0);
		DoMenu_nul(false);	// do net unloading
	}

	if (defaults->bNoAutoSave)
	{
		bUnsavedData = false;
		goto Done;
	}

Done:
	result = (close() == 0);		// do not end program if save failed
	return result;
}
//--------------------------------------------------------------------------//
//
GENRESULT Mission::RemoteFileRequest (const C8 *fileName, IFileSystem ** file)
{
	GENRESULT result = GR_GENERIC;

	if (strcmp(fileName, "??MISSION") == 0)
	{
		if (inFile!=0)
		{
			MEMFILEDESC mdesc = "??MISSION";
			DWORD dwRead;
			mdesc.dwDesiredAccess  = GENERIC_READ;
			mdesc.dwCreationDistribution = OPEN_EXISTING;

			mdesc.dwBufferSize = inFile->GetFileSize();
			mdesc.lpBuffer = malloc(mdesc.dwBufferSize);
			mdesc.dwFlags = CMF_DONT_COPY_MEMORY | CMF_OWN_MEMORY;
			inFile->SetFilePointer(0,0);
			inFile->ReadFile(0, mdesc.lpBuffer, mdesc.dwBufferSize, &dwRead, 0);
			if ((result = DACOM->CreateInstance(&mdesc, (void **) file)) != GR_OK)
				free(mdesc.lpBuffer);
		}
	}
	return result;
}
//--------------------------------------------------------------------------//
//
void Mission::SetMissionMRUList (HMENU hMenu)
{
	MENUITEMINFO minfo;
	DWORD i;
	char buffer[MAX_PATH+4];

	for (i = 0; i < 4; i++)
	{
		::RemoveMenu(hMenu, IDM_MRU0_MISSION+i, MF_BYCOMMAND);
	}

	for (i = 0; i < 4; i++)
	{
		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDM_MRU0_MISSION+i;
		minfo.dwTypeData = buffer;

		if (DEFAULTS->GetNameInMRU(buffer, IDS_MISSION_FILTER, i) == 0)
			break;

		minfo.cch = strlen(buffer);
		
		InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);
	}
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::SaveAs (const C8 * fileName, struct IComponentFactory * pParent)
{
	BOOL32 result = 0;
	COMPTR<IFileSystem> oldFile = inFile;
	char buffer[MAX_PATH+4];

	buffer[0] = 0;
	if (fileName)
		strcpy(buffer, fileName);
	if (pParent == 0)
		pParent = DACOM;
	if (buffer[0] || DEFAULTS->GetOutputFilename(buffer, IDS_MISSION_FILTER))
	{
		DAFILEDESC fdesc = buffer;

		if (strchr(buffer, '.') == 0)
		{
			strcat(buffer, ".mission");
			DEFAULTS->SetNameInMRU(buffer, IDS_MISSION_FILTER, 0);
		}
		fdesc.lpImplementation = "UTF";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (pParent->CreateInstance(&fdesc, inFile) != GR_OK)
		{
			inFile = oldFile;
			CQERROR0("Failed to create save file.");
			goto Done;
		}
		else
		if ((result=Save(0,pParent))==0)
		{
			inFile = oldFile;
			goto Done;
		}

	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::QuickSaveAs (const C8 * fileName, struct IComponentFactory * pParent)
{
	BOOL32 result = 0;
	COMPTR<IFileSystem> oldFile = inFile;
	char buffer[MAX_PATH+4];

	buffer[0] = 0;
	if (fileName)
		strcpy(buffer, fileName);
	if (pParent == 0)
		pParent = DACOM;
	if (buffer[0] || DEFAULTS->GetOutputFilename(buffer, IDS_QUICK_MISSION_FILTER))
	{
		DAFILEDESC fdesc = buffer;

		if (strchr(buffer, '.') == 0)
		{
			strcat(buffer, ".qmission");
			DEFAULTS->SetNameInMRU(buffer, IDS_QUICK_MISSION_FILTER, 0);
		}
		fdesc.lpImplementation = "UTF";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (pParent->CreateInstance(&fdesc, inFile) != GR_OK)
		{
			inFile = oldFile;
			CQERROR0("Failed to create save file.");
			goto Done;
		}
		else
		if ((result=QuickSave(0,pParent))==0)
		{
			inFile = oldFile;
			goto Done;
		}
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::DynamicSaveAs (const C8 * fileName, struct IComponentFactory * pParent)
{
	BOOL32 result = 0;
	COMPTR<IFileSystem> oldFile = inFile;
	char buffer[MAX_PATH+4];

	buffer[0] = 0;
	if (fileName)
		strcpy(buffer, fileName);
	if (pParent == 0)
		pParent = DACOM;
	if (buffer[0] || DEFAULTS->GetOutputFilename(buffer, IDS_DYNAMIC_MISSION_FILTER))
	{
		DAFILEDESC fdesc = buffer;

		if (strchr(buffer, '.') == 0)
		{
			strcat(buffer, ".dmission");
			DEFAULTS->SetNameInMRU(buffer, IDS_DYNAMIC_MISSION_FILTER, 0);
		}
		fdesc.lpImplementation = "UTF";
		fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
		fdesc.dwShareMode = 0;		// no sharing, faster reads/ writes
		fdesc.dwCreationDistribution = CREATE_ALWAYS;

		if (pParent->CreateInstance(&fdesc, inFile) != GR_OK)
		{
			inFile = oldFile;
			CQERROR0("Failed to create save file.");
			goto Done;
		}
		else
		if ((result=DynamicSave(0,pParent))==0)
		{
			inFile = oldFile;
			goto Done;
		}
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void Mission::LoadFromFilter()
{
	char buffer[MAX_PATH+4];
	DAFILEDESC fdesc = buffer;

	fdesc.lpImplementation = "UTF";
	buffer[0] = 0;
	if (DEFAULTS->GetInputFilename(buffer, IDS_MISSION_FILTER))
	{
		COMPTR<IFileSystem> filterFile;
		if (DACOM->CreateInstance(&fdesc, filterFile) == GR_OK)
		{
			baseObjPercent = 0;
			totalObjPercent = 0;
			OBJLIST->Load(filterFile,false,true);
			OBJLIST->ResolveAssociations();
		}
	}

}
//--------------------------------------------------------------------------//
//
void Mission::SetWindowTitle (void)
{
	// setup program needs to search on the official title
	if (CQFLAGS.bNoGDI==0)
	{
		char buffer[MAX_PATH+4+36];
		char *ptr;
		int len;

		::GetWindowText(hMainWindow, buffer, sizeof(buffer));

		len = strlen(buffer);
		if ((ptr = strchr(buffer, '[')) == 0)
		{
			buffer[len++] = ' ';
			ptr = buffer+len;
			buffer[len++] = '[';
			buffer[len] = 0;
		}

		ptr++;
		*ptr=0;

		if (inFile)
			inFile->GetFileName(ptr, sizeof(buffer)-(ptr-buffer));
		strcat(buffer, "] ");

		::SetWindowText(hMainWindow, buffer);
	}
}
//--------------------------------------------------------------------------//
//
void Mission::StartNetDownload (void)
{
	CQASSERT(HOSTID != PLAYERID);		//can't download if we are the host
	CQASSERT(downloadChannel==0);

	if (close() == 0)
	{
  		CQBOMB0("Close failed!?");
	}
	bDownloadNeeded = 1;
	downloadStatus = FTS_INITIALIZING;
}
//--------------------------------------------------------------------------//
//
void Mission::CancelNetDownload (void)
{
	if (downloadChannel && FILETRANSFER)
		FILETRANSFER->CloseChannel(downloadChannel);
	downloadStatus = FTS_INVALIDCHANNEL;
	downloadChannel = 0;
	bDownloadNeeded = 0;
}	
//--------------------------------------------------------------------------//
//
enum FTSTATUS Mission::GetDownloadStatus (void)
{
	return downloadStatus;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::close (void)
{
	BOOL32 result = 0;

	// todo(aaj-5/6/2004): big hack to get the first splash screen up first
	if (bDeferredMissionEndNonFail)
	{
		// do the loading
		COMPTR<IShapeLoader> loader;
		COMPTR<IDAComponent> pComp;
		COMPTR<IDrawAgent> slide;

		GENDATA->CreateInstance(CQ_SplashInfo.vfxName, pComp);
		pComp->QueryInterface("IShapeLoader", loader);

		if( loader->CreateDrawAgent(0,slide) == GR_OK )
		{
			// start the drawing
			PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
			PIPE->begin_scene();

			// do the drawing
			if(slide)
			{
				PANE src;
				src.window = NULL; // VFX_WINDOW*
				PIPE->get_viewport( (int*)&src.x0, (int*)&src.y0, (int*)&src.x1, (int*)&src.y1 );

				U16 width, height;
				slide->GetDimensions(width,height);

				PANE dst;
				dst.window = NULL; // VFX_WINDOW*
				dst.x0 = 0;
				dst.y0 = 0;
				dst.x1 = width;
				dst.y1 = height;

				slide->Draw( &src, &dst );
			}

			// end the drawing
			PIPE->end_scene();
			PIPE->swap_buffers();
		}

		// do the cleanup
		loader.free();
		slide.free();
	}

	if (bUnsavedData && CQFLAGS.bNoGDI==0)
	{
		switch (MessageBox(hMainWindow, "Save mission data?", "Unsaved Data", MB_YESNOCANCEL|MB_ICONEXCLAMATION))
		{
		case IDYES:
			if (Save() == 0)
				return close();
			break;
		case IDCANCEL:
			goto Done;
		}
	}
	
	EVENTSYS->Send(CQE_MISSION_CLOSE, 0);

	inFile.free();
	SetWindowTitle();
	MENU->EnableMenuItem(IDM_CLOSE_MISSION, MF_BYCOMMAND | MF_GRAYED); 		

	destroyPlayerAI();
	SECTOR->Close();
	OBJLIST->Close();
	THEMATRIX->Close();
	MScript::Close();
	MGlobals::Close();

	bUnsavedData=0;
	endCheckedCount = 0;
	deferredMissionWinners = 0;
	deferredMissionEndTime = 0;
	bOnlyHumanPlayerMsgNeeded = 0;
	bDeferredMissionEndNonFail = false;

	memcpy(COLORTABLE, DEFCOLORTABLE, sizeof(COLORTABLE));		// restore the color table

	EVENTSYS->Send(CQE_MISSION_CLOSE, (void *)1);

	unloadInterface();

	result=1;

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void Mission::Close (BOOL32 bForceIt)
{
	if (bForceIt)
		bUnsavedData = 0;
	close();
}
//--------------------------------------------------------------------------//
//
void Mission::reloadGametypes (void)
{
	if (PLAYERID==0 && CQFLAGS.bGameActive)		// can't do this during a network session
	{
		const USER_DEFAULTS * const defaults = DEFAULTS->GetDefaults();
		if (defaults->bNoAutoSave)
			bUnsavedData = false;
		if (bUnsavedData && Save()==0)
			return;
		COMPTR<IFileSystem> file = inFile;
		if (close())
		{
			OBJLIST->DEBUG_reloadDatabase();
			inFile = file;
			if (file)
				Reload();
			else
				New();
		}
	}
	else
		CQERROR0("Can't reload database right now.");
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
BOOL32 Mission::LoadMission (U32 id)
{
	char buffer[MAX_PATH+4];
	BOOL32 result=0;

	if (DEFAULTS->GetNameInMRU(buffer, IDS_MISSION_FILTER, id-IDM_MRU0_MISSION))
	{
		DEFAULTS->RemoveNameFromMRU(IDS_MISSION_FILTER, id-IDM_MRU0_MISSION);

		if ((result = Load(buffer)) != 0)
		{
			DEFAULTS->InsertNameIntoMRU(buffer, IDS_MISSION_FILTER);
		}
		else
			New();
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::New (void)
{
	BOOL32 result;

	if ((result = close()) != 0)
	{
		MGlobals::New();
		//warning, stupid code follows.  I didn't write it.  -rmarr
		result = SECTOR->New();
		result = OBJLIST->New();
		result = THEMATRIX->New();
		result = LIGHTS->New();
		result = FOGOFWAR->New();

		LoadInterface();

		EVENTSYS->Send(CQE_MISSION_LOAD_COMPLETE, 0);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Mission::SaveParseData (void)
{
	BOOL32 result = 0;
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "Data.h";
	DWORD len, dwWritten;

	inFile->CreateDirectory("\\ParseData");
	if (inFile->SetCurrentDirectory("\\ParseData") == 0)
		goto Done;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (inFile->CreateInstance(&fdesc, file) != GR_OK)
		goto Done;

	char * ptr2;
	{
		void * pPreprocessBlock;
		int length=0;

		if ((pPreprocessBlock = ::GetPreprocessData(length)) == 0)
			goto Done;

		ptr2 = (char *) malloc(length + 1);
		memcpy(ptr2, pPreprocessBlock, length);
		ptr2[length] = 0;
		len = length;
	}
	
	file->WriteFile(0, ptr2, len+1, &dwWritten, 0);
	::free(ptr2);

	inFile->SetCurrentDirectory("..");
	result = 1;
Done:
	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 Mission::LoadParseData (IFileSystem * inFile)
{
	BOOL32 result = 0;
	DAFILEDESC fdesc = "\\ParseData\\Data.h";
	DWORD len, dwRead;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	C8 * pTemp = 0;

	if (hPrevSymbols)
		parser->DestroySymbols(hPrevSymbols);
	hPrevSymbols = 0;
	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;
	len = inFile->GetFileSize(hFile);
	if ((pTemp = (C8 *) malloc (len+1)) == 0)
		goto Done;
	if (inFile->ReadFile(hFile, (void *)pTemp, len, &dwRead, 0) == 0)
		goto Done;
	pTemp[len] = 0;

	hPrevSymbols = parser->ParseNewMemory(pTemp);

	result=1;
Done:
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);
	::free(pTemp);

	return result;
}
//--------------------------------------------------------------------------
//
void Mission::updateNetFile (void)
{
	if (downloadChannel || (bDownloadNeeded && HOSTID && HOSTID != PLAYERID))
	{
		if (bDownloadNeeded)
		{
			if (downloadChannel)		// remote download of mission file
				FILETRANSFER->CloseChannel(downloadChannel);
			downloadChannel = FILETRANSFER->GetFile(HOSTID, "??MISSION");
		}

		if (downloadChannel)
		{
			U32 bytesReceived=0, totalBytes=0;
			COMPTR<IFileSystem> file;

			bDownloadNeeded = 0;
			FILETRANSFER->GetTransferProgress(downloadChannel, &bytesReceived, &totalBytes);

			switch (downloadStatus = FILETRANSFER->CreateFile(downloadChannel, file))
			{
			case FTS_INITIALIZING:
			case FTS_INPROGRESS:
				break;		// do nothing, connection still pending

			case FTS_SUCCESS:
				FILETRANSFER->CloseChannel(downloadChannel);
				downloadChannel = 0;
				bUnsavedData = 0;
				close();
				inFile = file;

				LoadParseData(inFile);
				MGlobals::Load(inFile);
				SECTOR->Load(inFile);
				SYSMAP->Load(inFile);
				CAMERAMANAGER->Load(inFile);
				NUGGETMANAGER->Load(inFile);
				baseObjPercent=0;
				totalObjPercent=0;
				OBJLIST->Load(inFile);
				THEMATRIX->Load(inFile);
				FOGOFWAR->Load(inFile);
				MUSICMANAGER->Load(inFile);
				LIGHTS->Load(inFile);
				MScript::Load(inFile);
				OBJLIST->ResolveAssociations();
				MUSICMANAGER->LoadFinish();
				EVENTSYS->Send(CQE_MISSION_LOAD_COMPLETE, 0);

				SetWindowTitle();
				inFile.free();
				bUnsavedData = 0;
				break;

			default:	// else some sort of error occurred
				switch (downloadStatus)
				{
				case FTS_INVALIDFILE:
					CQERROR0("Remote side has nothing loaded.");
					break;
				case FTS_INVALIDCHANNEL:
					if (PLAYERID)
						CQERROR0("An error occurred in data file transfer!");
					break;
				case FTS_CLOSED:				// remote side closed the connection
					CQERROR0("FTP Channel closed by host machine.");
					break;
				}

				FILETRANSFER->CloseChannel(downloadChannel);
				downloadChannel = 0;
				break;
			} // end switch
		}
	}
}
//--------------------------------------------------------------------------
//
GENRESULT Mission::GetFileSystem (struct IFileSystem **ppFile)
{
	if ((*ppFile = inFile) != 0)
	{
		inFile->AddRef();
		return GR_OK;
	}
	return GR_GENERIC;
}
//--------------------------------------------------------------------------
//
void Mission::SetFileSystem (struct IFileSystem * pFile)
{
	inFile = pFile;
}
//--------------------------------------------------------------------------
//
BOOL32 Mission::ReloadMission (void)
{
	// must be in single player mode to call this.
	U32 uMissionID = MGlobals::GetMissionID();

	char fileName[32];
	memset(fileName, 0, sizeof(fileName));

	if (uMissionID == 0)
	{
		// either were are about to reload mission 1 or one of the training missions
		enum M_RACE race = MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());
		if (race == M_TERRAN)
		{
			// reload mission 1
			strcpy(fileName, "T_Mission1");
		}
		else if (race == M_MANTIS)
		{
			// reload the mantis training mission
			strcpy(fileName, "Mantis_Train");
		}
		else if (race == M_SOLARIAN)
		{
			// reload the solarian training mission
			strcpy(fileName, "Sol_Train");
		}
	}
	else
	{
		// loading on of the missions 2 - 16
		wsprintf(fileName, "T_Mission%d", uMissionID+1); 
	}

	char fileNameEx[32];
	memset(fileNameEx, 0, sizeof(fileNameEx));

	// okay, first see if we can load a *.dmission file
	// if that fails, load a *.qmission file
	wsprintf(fileNameEx, "%s%s", fileName, ".dmission");

	DAFILEDESC fdesc = fileNameEx;
	if (SPMAPDIR->CreateInstance(&fdesc, inFile) != GR_OK)
	{
		// we couldn't find the dmission, load the qmission instead
		memset(fileNameEx, 0, sizeof(fileNameEx));
		wsprintf(fileNameEx, "%s%s", fileName, ".qmission");

		if (SPMAPDIR->CreateInstance(&fdesc, inFile) != GR_OK)
		{
			CQBOMB0("Could not restart the mission");
			return FALSE;
		}
	}

	return Reload();
}
//--------------------------------------------------------------------------
//
BOOL32 Mission::Reload (void)
{
	COMPTR<IFileSystem> temp = inFile;
	BOOL32 result = 0;
	bool bMouseBusy = false;

	if (close() == 0)
		goto Done;

	CURSOR->SetBusy(1);
	bMouseBusy = 1;

	inFile = temp;		// restore original file
	CQASSERT(inFile != 0);

	if (DEFAULTS->GetDefaults()->bSpectatorModeOn)
	{
		DEFAULTS->GetDefaults()->bSpectatorModeOn = false;
		DEFAULTS->GetDefaults()->bVisibilityRulesOff = false;
	}

	// don't create the progress animation if its already around
	if (ipAnim == 0)
	{
		CreateInProgressAnim(ipAnim);
	}

	ipAnim->UpdateString(IDS_PROG_INITMAP);
	LoadParseData(inFile);
	ipAnim->SetProgress(0.05f);

	MGlobals::Load(inFile);
	ipAnim->SetProgress(0.08f);

	if (PLAYERID==0 && MGlobals::IsSinglePlayer())		// we may want to store this data in the map!
	{
		DEFAULTS->GetDefaults()->fogMode = FOGOWAR_NORMAL;
	}
		
	if (MGlobals::GetGameSettings().version && MGlobals::GetGameSettings().version != GetBuildVersion())
	{
#ifdef _DEBUG
		CQERROR0("Mission file is from an incompatible version. I'll let it go, this time...");
#else
		CQERROR0("Mission file is from an incompatible version. DO NOT LOG BUGS FOR THIS SESSION!");
#endif
	}
	
	//detect if it is a dynamic load
	char buffer[MAX_PATH+4];
	MGlobals::GetTerrainFilename(buffer,sizeof(buffer));

	if (buffer[0])
	{
		COMPTR<IFileSystem> terrainFile;

		DAFILEDESC fdesc = buffer;

		fdesc.lpImplementation = "UTF";
	
		if (SPMAPDIR->CreateInstance(&fdesc, terrainFile) == GR_OK)
		{
			HANDLE _hPrevSymbols = hPrevSymbols;		// save old parse info
			hPrevSymbols = NULL;
			
			LoadParseData(terrainFile);

			ipAnim->SetProgress(0.15f);
			SECTOR->Load(terrainFile);
			SYSMAP->Load(terrainFile);

			ipAnim->SetProgress(0.20f);
			ipAnim->UpdateString(IDS_PROG_CAMERA);
			CAMERAMANAGER->Load(terrainFile);

			ipAnim->SetProgress(0.22f);
			ipAnim->UpdateString(IDS_PROG_OBJECTS);
			NUGGETMANAGER->Load(terrainFile);

			ipAnim->SetProgress(0.25f);
			baseObjPercent=ipAnim->GetProgress();
			totalObjPercent=0.40-baseObjPercent;
			OBJLIST->Load(terrainFile,true);

			ipAnim->SetProgress(0.40f);
			ipAnim->UpdateString(IDS_PROG_MATRIX);
			THEMATRIX->Load(terrainFile);

			ipAnim->SetProgress(0.43f);
			FOGOFWAR->Load(terrainFile);

			ipAnim->SetProgress(0.47f);
			MUSICMANAGER->Load(terrainFile);

			ipAnim->SetProgress(0.50f);
			LIGHTS->Load(terrainFile);

			ipAnim->SetProgress(0.53f);
			if (hPrevSymbols)
			{
				parser->DestroySymbols(hPrevSymbols);
			}
			hPrevSymbols = _hPrevSymbols;
		}
		
		ipAnim->UpdateString(IDS_PROG_OBJECTS);
		baseObjPercent=ipAnim->GetProgress();
		totalObjPercent=0.80 - baseObjPercent;
		OBJLIST->Load(inFile);

		ipAnim->SetProgress(0.80f);
		ipAnim->UpdateString(IDS_PROG_SCRIPTS);
		MScript::Load(NULL);		// should not have any save/load info here anyway
		ipAnim->SetProgress(0.90f);
	}
	else
	{
		// initialize the load
		SECTOR->Load(inFile);
		SYSMAP->Load(inFile);
		ipAnim->SetProgress(0.10f);

		// load the artificial intelligence
		ipAnim->UpdateString(IDS_PROG_CREATEAI);
		loadPlayerAI(inFile);
		ipAnim->SetProgress(0.15f);

		ipAnim->UpdateString(IDS_PROG_CAMERA);
		CAMERAMANAGER->Load(inFile);
		ipAnim->SetProgress(0.20f);

		ipAnim->UpdateString(IDS_PROG_OBJECTS);
		NUGGETMANAGER->Load(inFile);
		ipAnim->SetProgress(0.25f);

		baseObjPercent=ipAnim->GetProgress();
		totalObjPercent=0.80 - baseObjPercent;
		OBJLIST->Load(inFile);
		ipAnim->SetProgress(0.80f);

		ipAnim->UpdateString(IDS_PROG_MATRIX);
		THEMATRIX->Load(inFile);
		ipAnim->SetProgress(0.82f);

		FOGOFWAR->Load(inFile);
		ipAnim->SetProgress(0.84f);

		MUSICMANAGER->Load(inFile);
		ipAnim->SetProgress(0.86f);

		LIGHTS->Load(inFile);
		ipAnim->SetProgress(0.88f);

		ipAnim->UpdateString(IDS_PROG_SCRIPTS);
		MScript::Load(inFile);
		ipAnim->SetProgress(0.90f);
	}

	OBJLIST->ResolveAssociations();
	MUSICMANAGER->LoadFinish();
	EVENTSYS->Send(CQE_MISSION_LOAD_COMPLETE, 0);

	SetWindowTitle();
	bUnsavedData=0;
	
	ipAnim->UpdateString(IDS_PROG_LOADINTERFACE);
	if (PLAYERID==0 && MGlobals::IsSinglePlayer())
	{
		LoadInterface();
		DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, buffer, sizeof(buffer));
		MGlobals::AssignThisPlayer(buffer);
	}
//	else if (CQFLAGS.bGameActive)
else // AAJ - the proper way fix this is to have IsSinglePlayer() report correctly for Non-Mission DLL scripted missions
	{
		LoadInterface();
	}

	ipAnim->SetProgress(1.00f);

	if (hPrevSymbols)
	{
		parser->DestroySymbols(hPrevSymbols);
		hPrevSymbols = 0;
	}

	result = 1;

	if (CQFLAGS.bGameActive)
	{
		EVENTSYS->Send(CQE_GAME_ACTIVE, (void *)1);
	}

Done:
	if (bMouseBusy)
		CURSOR->SetBusy(0);

	// do not free the animation if we are going to call initnetgame
	if (ipAnim != NULL && bNetloadProgressAnimation == false)
	{
		ipAnim->SetProgress(1.0f);
		ipAnim.free();
		ipAnim = NULL;
	}
	return result;
}
//--------------------------------------------------------------------------
//
BOOL32 Mission::LoadBriefing (const char * fileName)
{
	BOOL32 result = 0;
	DAFILEDESC fdesc = fileName;

	if (close() == 0)
		goto Done;

	if (SPMAPDIR->CreateInstance(&fdesc, inFile) != GR_OK)
		goto Done;
		
	LoadParseData(inFile);
	MGlobals::Load(inFile);
	if (PLAYERID==0 && MGlobals::IsSinglePlayer())		// we may want to store this data in the map!
		DEFAULTS->GetDefaults()->fogMode = FOGOWAR_NORMAL;
		
	if (MGlobals::GetGameSettings().version && MGlobals::GetGameSettings().version != GetBuildVersion())
	{
#ifdef _DEBUG
		CQERROR0("Mission file is from an incompatible version. I'll let it go, this time...");
#else
		CQERROR0("Mission file is from an incompatible version. DO NOT LOG BUGS FOR THIS SESSION!");
#endif
	}

	MScript::LoadScript();
	SetWindowTitle();
	bUnsavedData=0;
	result = 1;

Done:
	return result;
}
//--------------------------------------------------------------------------
//
void Mission::InitializeNetGame (const struct FULLCQGAME & cqgame, U32 randomSeed)
{
	// if this gets called you know you are playing a quickbattle or multiplayer game
	// the animation is already going
	CQASSERT(ipAnim);

	ipAnim->UpdateString(IDS_PROG_ASSIGNPLAYERS);
	MGlobals::AssignPlayers(cqgame, randomSeed);
	ipAnim->SetProgress(0.8f);

	ipAnim->UpdateString(IDS_PROG_LOADINTERFACE);
	LoadInterface();
	
	//
	// look at this player's ships
	//
	ipAnim->SetProgress(0.9f);
	ipAnim->UpdateString(IDS_PROG_INIT3D);
	IBaseObject * obj = OBJLIST->GetTargetList(), *end=0;
	const U32 playerID = MGlobals::GetThisPlayer();

	while (obj)
	{
		if (obj->GetPlayerID() == playerID)
			break;
		obj = obj->nextTarget;
	}

	// set camera to look at obj
	if (obj)
	{
		SECTOR->SetCurrentSystem(obj->GetSystemID());
		CAMERA->SetLookAtPosition(obj->GetTransform().translation);
	}

	explodePlayerBombs();

	ipAnim->SetProgress(0.95f);
	
	//
	// recalculate visibility
	//
	obj=OBJLIST->GetObjectList();

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

	ipAnim->SetProgress(0.98f);
	ipAnim->UpdateString(IDS_PROG_STARTAI);
	startPlayerAI();

	//
	// initialize money
	//
	if (MGlobals::GetUpdateCount() == 0)
	{
		U32 gas = 0;
		U32 metal = 0;
		U32 crew = 0;
		U32 maxControlPoints = 100;
#ifdef _DEMO
		if (cqgame.commandLimit == COMMAND_LOW)
			maxControlPoints = 50;
		else
		if (cqgame.commandLimit == COMMAND_MID)
			maxControlPoints = 150;
		else
		if (cqgame.commandLimit == COMMAND_HIGH)
			maxControlPoints = 200;
#else
		if (cqgame.commandLimit == COMMAND_NORMAL)
			maxControlPoints = 150;
		else
		if (cqgame.commandLimit == COMMAND_MID)
			maxControlPoints = 200;
		else
		if (cqgame.commandLimit == COMMAND_HIGH)
			maxControlPoints = 300;		
#endif

		if(cqgame.money == LOW_MONEY)
		{
			gas = atoi(_localLoadString(IDS_GAS_LOW));
			metal = atoi(_localLoadString(IDS_METAL_LOW));
			crew = atoi(_localLoadString(IDS_CREW_LOW));
		}
		else if(cqgame.money == MEDIUM_MONEY)
		{
			gas = atoi(_localLoadString(IDS_GAS_MEDIUM));
			metal = atoi(_localLoadString(IDS_METAL_MEDIUM));
			crew = atoi(_localLoadString(IDS_CREW_MEDIUM));
		}
		else if(cqgame.money == HIGH_MONEY)
		{
			gas = atoi(_localLoadString(IDS_GAS_HIGH));
			metal = atoi(_localLoadString(IDS_METAL_HIGH));
			crew = atoi(_localLoadString(IDS_CREW_HIGH));
		}
		
		for (U32 i = 1; i <= MAX_PLAYERS; i++)
		{
			MGlobals::SetCurrentCrew(i,crew);
			MGlobals::SetCurrentGas(i,gas);
			MGlobals::SetCurrentMetal(i,metal);
			MGlobals::SetMaxControlPoints(i, maxControlPoints);
		}
	}

	THEMATRIX->ResetSyncItems();

	if (cqgame.mapType == RANDOM_MAP)
	{
		MUSICMANAGER->InitPlaylist(MGlobals::GetPlayerRace(playerID));
		EVENTSYS->Send(CQE_MISSION_LOAD_COMPLETE, 0);
	}

	// destroy what we need to
	ipAnim.free();
	ipAnim = NULL;
}
//--------------------------------------------------------------------------
//
void Mission::GenerateMultiplayerMap (const struct FULLCQGAME & cqgame, U32 randomSeed)
{
	// get ready for the load animation...
	if (ipAnim==0)
		CreateInProgressAnim(ipAnim);

	bUnsavedData = 0;
	close();
	MGlobals::New();		// doesn't have a close() method

	CAMERA->SetCameraToDefaults();		// always use default values for camera
	OBJLIST->New();
	THEMATRIX->New();
	LIGHTS->New();
	FOGOFWAR->New();
	
	// generate the map
	MAPGEN->GenerateMap(cqgame, randomSeed, ipAnim);
}
//--------------------------------------------------------------------------
// start up a menu that says -"hey you lost" and it has an okay button to let you continue
// DO NOT CALL THIS IN RESPONSE TO A NETWORK PACKET OR YOU WILL STALL OTHER PLAYERS!
// if top bit of allyMask is set, local player has lost, but game is to continue....
//
void Mission::EndMission (U32 allyMask, bool bNonWinLose)
{
	if (ZONESCORE && ((allyMask & 0x80000000) == 0))
	{
		const CQGAME & cqgame = MGlobals::GetGameSettings();
		U32 i;

		for (i = 0; i < cqgame.activeSlots; i++)
		{
			U32 playerID = MGlobals::GetPlayerIDFromSlot(i);

			if (playerID && playerID <= MAX_PLAYERS)
			{
				if ((allyMask & (1 << (playerID-1))) != 0)		// is player among the winners
				{
					U32 flags = ZONESCORE_WINNER;
					if (cqgame.slot[i].dpid==0)
					{
						if (MGlobals::HasPlayerResigned(i))
							flags |= ZONESCORE_RESIGNED;
						else
							flags |= ZONESCORE_COMPUTERPLAYER;
					}
					ZONESCORE->SetPlayerFlags(MGlobals::GetZoneSeatFromSlot(i), flags);
				}
			}
		}

		ZONESCORE->SendGameState(DPLOBBY, ZSTATE_END);
		ZONESCORE->SendFinalScore(DPLOBBY);
	}

	bool bWon = ((allyMask & (1 << (MGlobals::GetThisPlayer()-1))) != 0);		// are we allied with winners?
	U32 result = bWon ? MISSION_END_WON : MISSION_END_LOST;

	if (bNonWinLose)
	{
		result = MISSION_END_SPLASH;
	}
	
	// if recursing, this will close the previous dialog message!
	EVENTSYS->Send(CQE_DLG_RESULT, (void *)result);			// go back to shell
	EVENTSYS->Send(CQE_MISSION_ENDING, (void *)result);		// send result code

	if (THEMATRIX->IsMaster())
	{
		THEMATRIX->FlushSyncData();	// called by Host before shutdown of net session
		if (((allyMask & 0x80000000) == 0))		// is the game really ending!?
		{
			ENDGAME_PACKET packet;
			packet.allyMask = allyMask;
			NETPACKET->Send(0, NETF_ALLREMOTE, &packet);
		}
	}

	BOOL bOldGamePaused = CQFLAGS.bGamePaused;
	// only do the pause thing if the game is really ending!
	// otherwise the clients will be paused for no good reason
	if (bOldGamePaused==0 && ((allyMask & 0x80000000) == 0))
	{
		CQFLAGS.bGamePaused = TRUE;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}

	if (result == MISSION_END_WON)
	{
		CQMessageBox(IDS_MISSION_PRESSOK, IDS_MISSION_SUCCESS, MB_OK);
	}
	else if(result == MISSION_END_LOST)
	{
		CQMessageBox(IDS_MISSION_PRESSOK, IDS_MISSION_FAILURE, MB_OK);
	}

	if (bOldGamePaused==0 && ((allyMask & 0x80000000) == 0))
	{
		CQFLAGS.bGamePaused = FALSE;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}
}
//--------------------------------------------------------------------------
//
U32 Mission::GetNumAIPlayers (void)
{
	U32 result = 0;
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (playerAI[i] != 0)
			result++;
	}

	return result;
}

//--------------------------------------------------------------------------
// AI CONTROLLING METHODS FOR MISSION SCRIPTING
//--------------------------------------------------------------------------
//
bool Mission::IsComputerControlled (U32 playerID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	return (playerAI[playerID-1]);
}
//--------------------------------------------------------------------------
//
void Mission::EnableEnemyAI (U32 playerID, bool bOn, enum M_RACE race)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if (bOn)
	{  
		if (!playerAI[playerID-1])
			createPlayerAI(race, playerID, playerAI[playerID-1]);
		
		playerAI[playerID-1]->Activate(true);
	}
	else
	{
		if (playerAI[playerID-1])
			playerAI[playerID-1]->Activate(false);
	}
}
//--------------------------------------------------------------------------
//
void Mission::EnableEnemyAI (U32 playerID, bool bOn, const char * szPlayerAIType)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if (bOn)
	{  
		if (!playerAI[playerID-1])
			ISPlayerAI::CreateInstance(szPlayerAIType, playerID, playerAI[playerID-1]);
		
		playerAI[playerID-1]->Activate(true);
	}
	else
	{
		if (playerAI[playerID-1])
			playerAI[playerID-1]->Activate(false);
	}
}
//--------------------------------------------------------------------------
//
void Mission::SetEnemyAIRules (U32 playerID, const struct AIPersonality & rules)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if (playerAI[playerID-1])
	{
		playerAI[playerID-1]->SetPersonality(rules);
	}
}
//--------------------------------------------------------------------------
//
void Mission::SetEnemyAITarget (U32 playerID, IBaseObject *obj, U32 range, U32 systemID)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if (playerAI[playerID-1])
	{
		playerAI[playerID-1]->SetStrategicTarget(obj, range, systemID);
	}
}
//--------------------------------------------------------------------------
//
void Mission::SetFileDescription(const wchar_t * desc)
{
	MGlobals::SetFileDescription(desc);
}
//--------------------------------------------------------------------------
//
static void playerHasLost (U32 playerID)
{
	PLAYERLOST_PACKET packet;

	packet.playerID = playerID;
	packet.dwSize = sizeof(packet);

	NETPACKET->Send(0, NETF_ALLREMOTE|NETF_ECHO, &packet);
}
//--------------------------------------------------------------------------
//
static U32 getTeamRemaining (const CQGAME & cqgame)
{
	U32 result = 0;
	U32 i;

	for (i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == READY && MGlobals::HasPlayerResigned(i)==false)
		{
			U32 playerID = MGlobals::GetPlayerIDFromSlot(i);
			result |= (1 << playerID);
		}
	}

	result >>= 1;

	return result;
}
//--------------------------------------------------------------------------
// 
void Mission::enableSpectatorMode (void)
{
	DEFAULTS->GetDefaults()->bSpectatorModeOn = true;
	DEFAULTS->GetDefaults()->bVisibilityRulesOff = true;
}
//--------------------------------------------------------------------------
// called periodically
//
void Mission::checkEndGame (void)
{
	const CQGAME & cqgame = MGlobals::GetGameSettings();
	if (cqgame.gameType != MISSION_DEFINED && DEFAULTS->GetDefaults()->bNoWinningConditions == 0 && THEMATRIX->IsMaster())
	{
		U32 playerMask = 0;
		MPart part = OBJLIST->GetTargetList();
		BOOL32 bEnd = false;
		U32 numUnits[MAX_PLAYERS+1];
		U32 numHQs[MAX_PLAYERS+1];
		U32 numPlats[MAX_PLAYERS+1];
		U32 numFabs[MAX_PLAYERS+1];
		VOLPTR(IPlatform) pPlatform;

		memset(numUnits, 0, sizeof(numUnits));
		memset(numHQs, 0, sizeof(numHQs));
		memset(numPlats, 0, sizeof(numPlats));
		memset(numFabs, 0, sizeof(numFabs));

		while (part.obj)
		{
			CQASSERT(part.isValid());

			if (part->bReady)
			{
				const U32 playerID = part->playerID;

				if (playerID && playerID <= MAX_PLAYERS)
				{
					if (part.obj->objClass != OC_PLATFORM || (pPlatform=part.obj)==0 || pPlatform->IsReallyDead()==0)
					{
						numUnits[playerID]++;

						if (MGlobals::IsHQ(part->mObjClass))
							numHQs[playerID]++;
						if (part.obj->objClass == OC_PLATFORM && MGlobals::IsGunPlat(part->mObjClass)==0)
							numPlats[playerID]++;
						if (MGlobals::IsFabricator(part->mObjClass))
							numFabs[playerID]++;
					}
				}
			}

			part = part.obj->nextTarget;
		}

		if (cqgame.gameType == KILL_UNITS)
		{
			U32 i;
			U32 losers=0;
			for (i = 0; i < cqgame.activeSlots; i++)
			{
				if (cqgame.slot[i].state == READY && MGlobals::HasPlayerResigned(i)==false)
				{
					U32 playerID = MGlobals::GetPlayerIDFromSlot(i);
					if (numUnits[playerID] == 0)
					{
						losers |= (1 << playerID);
						MGlobals::SetPlayerResignedBySlot(i);
					}
				}
			}

			losers>>=1;
			for (i = 1; i <= MAX_PLAYERS; i++, losers>>=1)
			{
				if (losers & 1)
				{
					playerHasLost(i);
				}
			}

			playerMask = getTeamRemaining(cqgame);
			if (playerMask)
			{
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					if ((playerMask & (1 << i)) != 0)
					{
						if ((MGlobals::GetAllyMask(i+1) & playerMask) != playerMask)		// remaining players are not allied
							goto Done;	// keep playing
					}
				}
			}
		}
		else  // last platform
		if (cqgame.gameType == KILL_HQ_PLATS)
		{
			U32 i;
			U32 losers=0;
			for (i = 0; i < cqgame.activeSlots; i++)
			{
				if (cqgame.slot[i].state == READY && MGlobals::HasPlayerResigned(i)==false)
				{
					U32 playerID = MGlobals::GetPlayerIDFromSlot(i);
					if (numHQs[playerID] == 0)
					{
						losers |= (1 << playerID);
						MGlobals::SetPlayerResignedBySlot(i);
					}
				}
			}

			losers>>=1;
			for (i = 1; i <= MAX_PLAYERS; i++, losers>>=1)
			{
				if (losers & 1)
				{
					playerHasLost(i);
					THEMATRIX->TerminatePlayerUnits(i);
				}
			}

			playerMask = getTeamRemaining(cqgame);
			if (playerMask)
			{
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					if ((playerMask & (1 << i)) != 0)
					{
						if ((MGlobals::GetAllyMask(i+1) & playerMask) != playerMask)		// remaining players are not allied
							goto Done;	// keep playing
					}
				}
			}
		}
		else  // last platform
		if (cqgame.gameType == KILL_PLATS_FABS)
		{
			U32 i;
			U32 losers=0;
			for (i = 0; i < cqgame.activeSlots; i++)
			{
				if (cqgame.slot[i].state == READY && MGlobals::HasPlayerResigned(i)==false)
				{
					U32 playerID = MGlobals::GetPlayerIDFromSlot(i);
					if (numPlats[playerID]+numFabs[playerID] == 0)
					{
						losers |= (1 << playerID);
						MGlobals::SetPlayerResignedBySlot(i);
					}
				}
			}

			losers>>=1;
			for (i = 1; i <= MAX_PLAYERS; i++, losers>>=1)
			{
				if (losers & 1)
				{
					playerHasLost(i);
					THEMATRIX->TerminatePlayerUnits(i);
				}
			}

			playerMask = getTeamRemaining(cqgame);
			if (playerMask)
			{
				for (i = 0; i < MAX_PLAYERS; i++)
				{
					if ((playerMask & (1 << i)) != 0)
					{
						if ((MGlobals::GetAllyMask(i+1) & playerMask) != playerMask)		// remaining players are not allied
							goto Done;	// keep playing
					}
				}
			}
		}

		bEnd = true;
Done:
		if (bEnd)
		{
			if (deferredMissionEndTime==0)
				deferredMissionEndTime = DEFERRED_MISSION_END_TIME;
			deferredMissionWinners = playerMask;
		}
	}
}
//--------------------------------------------------------------------------
//
void Mission::StartProgressAnim (struct IPANIM ** ppAnim)
{
	bNetloadProgressAnimation = true;
	if (ipAnim == 0)
	{
		CreateInProgressAnim(ipAnim);
	}
	ipAnim->AddRef();
	*ppAnim = ipAnim;
}
//--------------------------------------------------------------------------
//
const M_RACE Mission::GetSinglePlayerRace (void) const
{
	return MGlobals::GetPlayerRace(MGlobals::GetThisPlayer());
}
//--------------------------------------------------------------------------
//
void Mission::SetSinglePlayerRace (const enum M_RACE race)
{
	MGlobals::SetPlayerRace(MGlobals::GetThisPlayer(), race);
}
//--------------------------------------------------------------------------
//
const struct CQGAME & Mission::GetGameSettings (void)
{
	return MGlobals::GetGameSettings();
}
//--------------------------------------------------------------------------
//
void Mission::ToggleCheating (void)
{
	g_bCheatsOn = !g_bCheatsOn;

	if (g_bCheatsOn == false)
	{
		setDefaultCheatSettings();
	}
}
//--------------------------------------------------------------------------
//
const bool Mission::GetCheatsEnabled (void) const
{
	return g_bCheatsOn;
}
//--------------------------------------------------------------------------
//
bool Mission::IsMouseEnabled (void)
{
	return !MGlobals::GetScriptUIControl();
}
//--------------------------------------------------------------------------
//
void Mission::createPlayerAI (M_RACE race, U32 playerID, ISPlayerAI ** ppPlayerAI)
{
	ISPlayerAI::CreateInstance(race, playerID, ppPlayerAI);
}
//--------------------------------------------------------------------------
//
void Mission::LaunchOffensive (U32 playerID, UNIT_STANCE stance)
{
	CQASSERT(playerID && playerID <= MAX_PLAYERS);
	if (playerAI[playerID-1])
			playerAI[playerID-1]->LaunchOffensive(stance);
}
//--------------------------------------------------------------------------
//
COLORREF Mission::GetPlayerColorForDPID (U32 dpid)
{
	U32 playerID = MGlobals::GetPlayerIDFromDPID(dpid);
	COLORREF color = RGB(255,255,255);

	if (playerID && playerID <= MAX_PLAYERS)
		color = COLORTABLE[MGlobals::GetColorID(playerID)];

	return color;
}

//--------------------------------------------------------------------------
// points should add up to around 256
#define CALC_PLAT_POINTS 60
#define CALC_SHIP_POINTS 60
#define CALC_EXPLORED_POINTS 10
#define CALC_JUMPGATE_POINTS 100 
#define CALC_MONEY_POINTS 30

#define MAX_POSSIBLE_UNITS 50
#define MAX_POSSIBLE_MONEY 750

//--------------------------------------------------------------------------
//
void Mission::calculateCtrlO_Scores (void)
{
	U32 playerID;

	for (playerID = 1; playerID <= MAX_PLAYERS; playerID++)
	{
		if (MGlobals::IsPlayerInGame(playerID))
		{
			S32 total = 0;
			// add up plat units
			S32 score = MGlobals::GetNumPlatformsBuilt(playerID);
			score -= MGlobals::GetPlatformsLost(playerID);
			if (score < 0)
				score = 0;
			else
			if (score < MAX_POSSIBLE_UNITS)
				score = (score * CALC_PLAT_POINTS) / MAX_POSSIBLE_UNITS;
			else
				score = CALC_PLAT_POINTS;
			total += score;

			// add up plat units
			score = MGlobals::GetNumUnitsBuilt(playerID);
			score -= MGlobals::GetUnitsLost(playerID);
			if (score < 0)
				score = 0;
			else
			if (score < MAX_POSSIBLE_UNITS)
				score = (score * CALC_SHIP_POINTS) / MAX_POSSIBLE_UNITS;
			else
				score = CALC_SHIP_POINTS;

			total += score;
			
			// add up exploration
			score = (MGlobals::GetExploredSystemsRatio(playerID) * CALC_EXPLORED_POINTS);
			total += score;

			// add up jumpgate control for allies of player
			score = 0;
			{
				U32 allyMask = MGlobals::GetAllyMask(playerID) << 1;
				U32 i;
				for (i = 1; i <= MAX_PLAYERS; i++)
				{
					if ((allyMask & (1 << i)) != 0)
						score += MGlobals::GetNumJumpgatesControlled(i);
				}
			}
			if (score < SECTOR->GetNumSystems())
				score = (score * CALC_JUMPGATE_POINTS) / SECTOR->GetNumSystems();
			else
				score = CALC_JUMPGATE_POINTS;
			total += score;

			// add up money
			score = MGlobals::GetCurrentCrew(playerID);
			score += MGlobals::GetCurrentMetal(playerID);
			score += MGlobals::GetCurrentGas(playerID);

			if (score < MAX_POSSIBLE_MONEY)
				score = (score * CALC_MONEY_POINTS) / MAX_POSSIBLE_MONEY;
			else
				score = CALC_MONEY_POINTS;
			total += score;

			MGlobals::SetPlayerScore(playerID, total);
		}
	}
}
//--------------------------------------------------------------------------
//
void Mission::explodePlayerBombs (void)
{
	IBaseObject * obj = OBJLIST->GetTargetList();

	while (obj)
	{
		VOLPTR(IPlayerBomb) pBomb = obj;

		if (pBomb)
			pBomb->ExplodePlayerBomb();
		
		obj = obj->nextTarget;
	}

}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
static BOOL32 CreateParser (void)
{
	if (GS->QueryInterface("IViewConstructor", (void **)&PARSER) == GR_OK)
	{
		AddToGlobalCleanupList((IDAComponent **) &PARSER);
		return 1;
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
static BOOL32 ParseDefFile (void)
{
	BOOL32 result=0;

	if (PARSER->QueryInterface("IViewConstructor2", (void **) &parser) == GR_OK)
	{
		void * pPreprocessBlock;
		char * ptr2;
		int len=0;

		if ((pPreprocessBlock = ::GetPreprocessData(len)) == 0)
			goto Done;

		ptr2 = (char *) malloc(len + 1);
		memcpy(ptr2, pPreprocessBlock, len);
		ptr2[len] = 0;

		result = (parser->ParseMemory(ptr2) == GR_OK);
		::free(ptr2);
	}
	else
		result = 0;

Done:
	return result;
}
extern "C" void _heap_init (void);
//--------------------------------------------------------------------------//
//
struct _mission : GlobalComponent
{
	Mission * mission;
	HINSTANCE hInstance;

	virtual void Startup (void)
	{
		_heap_init();

		if (CreateParser() == 0 || ParseDefFile() == 0)
			CQBOMB0("Failed parse of the Data definition file: ..\\dinclude\\Data.h");

		memcpy(COLORTABLE, DEFCOLORTABLE, sizeof(COLORTABLE));		// init the color table
		
		MISSION = mission = new DAComponent<Mission>;
		AddToGlobalCleanupList((IDAComponent **) &MISSION);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		DWORD dwLen;
		char buffer[260];
		
		dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));

		if (GS->QueryOutgoingInterface("ISystemEventCallback", connection) == GR_OK)
			connection->Advise(MISSION, &mission->systemEventHandle);
		if (FILETRANSFER->QueryOutgoingInterface("IFileTransferCallback", connection) == GR_OK)
			connection->Advise(MISSION, &mission->fileTransferHandle);
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(MISSION, &mission->eventHandle);
			FULLSCREEN->SetCallbackPriority(mission, EVENT_PRIORITY_MISSION);
		}

		SendMessage(hMainWindow, CQE_DATA_CHECKSUM, 3, CreateZBufferAnalog(buffer));
	}
};

static _mission mission;

void main (void)
{
}
//--------------------------------------------------------------------------
//  
static void SetDllHeapMsg (HINSTANCE hInstance)
{
	DWORD dwLen;
	char buffer[260];
	
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
	mission.hInstance = hInstance;
 
	while (dwLen > 0)
	{
		if (buffer[dwLen] == '\\')
		{
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg(buffer+dwLen);
}
//--------------------------------------------------------------------------//
//
BOOL WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hInstance);
		}
		break;
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
void __stdcall MissionResetHeap (IHeap * heap)
{
	HEAP = heap;
}
//--------------------------------------------------------------------------//
//
//------------------------------------------------------------------------------------//
//---------------------------------------End Mission.cpp------------------------------//
//------------------------------------------------------------------------------------//

