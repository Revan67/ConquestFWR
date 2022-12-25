//--------------------------------------------------------------------------//
//                                                                          //
//                               Conquest.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Tmauer $

    $Header: /Conquest/App/Src/Conquest.cpp 190   6/12/01 12:49p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "UserDefaults.h"
#include "Resource.h"
#include "Mission.h"
#include "CQTrace.h"
#include "MGlobals.h"
#include "GenData.h"
#include "MScroll.h"
#include "BaseHotRect.h"
#include "CQBatch.h"
#include "Netpacket.h"
#include "OpAgent.h"
#include "CQImage.h"
#include "CommPacket.h"
#include <stdio.h>
#include <MScript.h>

#include <DACOM.h>
#include <HeapObj.h>
#include <GameSys.h>
#include <FileSys.h>
#include <RendPipeline.h>
#include <renderer.h>
#include <IHardPoint.h>
#include <IRenderPrimitive.h>
#include <Streamer.h>
#include <LightMan.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <StringTable.h>
#include <VideoSystem.h>
#include <IMaterialManager.h>
#include <IParticleManager.h>
#include <IMeshManager.h>

#include <DPlay.h>


#include <stdlib.h>
#include <commctrl.h>
#include <objbase.h>
#include <float.h>
#include <locale.h>

// 64 MEGS - CODE - BATCHER

#define HEAP_SIZE  (0x4000000-0xA00000-0x200000)
//--------------------------------------------------------------------------
//-----------------------Game Global instances------------------------------
//--------------------------------------------------------------------------

#define NAME_REG_KEY   "CQPlayerName"

GameSystem GAME;


static bool bCoInitialized;
static bool bInCleanUp;
static bool bStartNewMission;

static M_RACE interfaceRace;

static char cmd_line_file[256];	// filename specified on the command line
static char cmd_line_ini[32] = "Conquest.ini";	// ini file specified on the command line
static char cmd_line_script[32];	// script file specified on the command line

extern "C"
__declspec(dllimport) IHeap * __stdcall GetBatchHeap (void);
HWND __stdcall DoSplashScreen (void);
void __stdcall ShutdownSplash (HWND hwnd);

CQEXTERN SPLASHINFO CQ_SplashInfo;

//--------------------------------------------------------------------------
// special custom version of heap interface
struct DACOM_NO_VTABLE IHeapBackdoor : IDAComponent
{
	virtual void __stdcall SetHeapFlags (U32 flags) = 0;
};

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//
static void clean_up (void)
{
	if (bInCleanUp)
		return;
	bInCleanUp = 1;
	
	{
		COMPTR<IHeapBackdoor> door;

		if (HEAP->QueryInterface("IHeapBackdoor", door) == GR_OK)
		{
			U32 flags = HEAP->GetHeapFlags();

			flags &= ~DAHEAPFLAG_DEBUGFILL_SNAN;		// don't fill memory
			flags |= DAHEAPFLAG_NOBESTFIT|DAHEAPFLAG_NOVERIFYPTR;

			door->SetHeapFlags(flags);
		}
	}

	CQFLAGS.bGameActive = 0;	// prevent sending game packets
	if (DPLAY)
		DPLAY->Close();			// make sure we close the connection

	VB_MANAGER->cleanup();

	// clean up objects, remove all archetypes
	EVENTSYS->Send(WM_CLOSE, 0);		// force toolbar, etc to close
	FULLSCREEN->Notify(CQE_UPDATE, 0);	// force one last update
	
	if (MISSION)
		MISSION->Close();

	CleanupGlobals();

	if (hMainDC)
	{
		ReleaseDC(hMainWindow, hMainDC);
		hMainDC = 0;
	}

	PB.~PB();			// force early destruction of PB (JY)
	ENGINE = 0;
	GS = 0;

	GAME.shut_down();

	if (bCoInitialized)
	{
		bCoInitialized = 0;
		CoUninitialize();
	}

	if (DACOM)
	{
		if (CQIMAGE)
			CQIMAGE->MemoryReport(HEAP);
		else
			PrintHeap(HEAP);

		IHeap *batchHeap = GetBatchHeap();
		if (CQIMAGE)
			CQIMAGE->MemoryReport(batchHeap);
		else
			PrintHeap(batchHeap);
		DACOM->ShutDown();
		DACOM=0;
	}
}
//--------------------------------------------------------------------------
//
static LONG __stdcall ___UnhandledExceptionFilter (_EXCEPTION_POINTERS *ExceptionInfo)
{
	if (CQFLAGS.bExceptionHappened==0)
	{
		CQFLAGS.bExceptionHappened = 1;
	
		int result = CQIMAGE->Exception(ExceptionInfo);

		if (result == IDIGNORE)
		{
			CQFLAGS.bExceptionHappened = 0;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		else
		if (result == IDABORT)
		{
			clean_up();
			SetErrorMode(SEM_NOGPFAULTERRORBOX);
		}
	}

	if (prevExceptionHandler)
		return prevExceptionHandler(ExceptionInfo);
	return EXCEPTION_CONTINUE_SEARCH;
} 

//--------------------------------------------------------------------------
//forces the defaults to good values and creates a player name
//should only be used for demo that bypasses menu
#define WIDTH_REG_KEY		"CQWindowWidth"
#define HEIGHT_REG_KEY		"CQWindowHeight"
#define DEPTH_REG_KEY		"CQPixelDepth"
#define RENDERDEV_REG_KEY	"RenderDevice"

static void forceDefaults()
{
	char name[128];
	U32 result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, name, sizeof(name));
	if(result == 0)
	{
		char * szNameAnsi = "Blackwell";

		DEFAULTS->SetStringInRegistry(NAME_REG_KEY, szNameAnsi);

		// the new path +directory
		char szPathAnsi[256];
		wsprintf(szPathAnsi, "%s%s", "SavedGame\\", szNameAnsi);

		::CreateDirectory("SavedGame", 0);
		::CreateDirectory(szPathAnsi, 0);

		//find3DCard
		HANDLE hSection;
		COMPTR<IProfileParser> parser;
		
		char buffer[MAX_PATH];
		char regValue[MAX_PATH];
		
		bool bPrimary = false;
		bool bFound3DCard = false;
		int i;

		if (DEFAULTS->GetStringFromRegistry(RENDERDEV_REG_KEY, regValue, sizeof(regValue)) == 0)
			strcpy(regValue, "{00000000-0000-0000-0000-000000000000}");

		DACOM->QueryInterface(IID_IProfileParser, parser);

		// find the first card with 3D capabilites
		for (i = 0; i < 4; i++)
		{
			sprintf(buffer, "Rend%d", i);
			
			if ((hSection = parser->CreateSection(buffer)) != 0)
			{
				if (parser->ReadKeyValue(hSection, "DeviceId", buffer, sizeof(buffer)) != 0)
				{
					bool bIsNull = (strncmp(buffer, "{00000000-0000-0000-0000-000000000000}", strlen("{00000000-0000-0000-0000-000000000000}")) == 0);
					
					if (bPrimary == false || bIsNull == false)
					{
						if (bPrimary == false)
							bPrimary = bIsNull;

						// does the card have 3D capabilites?
						if (TestDeviceFor3D(buffer))
						{
							bFound3DCard = true;
						}
					}
				}

				parser->CloseSection(hSection);

				if (bFound3DCard)
				{
					break;
				}
			}
		}

		// whether we find a 3D card or not, we're still going to start off in default video mode
		// the default resoulition is 640x480x16
		U32 width = 1024;
		U32 height = 768;
		U32 depth = 32;

		Set3DVarialbes(width, height, depth);

		// right the vars to the registry
		DEFAULTS->SetDataInRegistry(WIDTH_REG_KEY, &width, sizeof(U32));
		DEFAULTS->SetDataInRegistry(HEIGHT_REG_KEY, &height, sizeof(U32));
		DEFAULTS->SetDataInRegistry(DEPTH_REG_KEY, &depth, sizeof(U32));
		
		// get the guid and save it off
		DEFAULTS->SetStringInRegistry(RENDERDEV_REG_KEY, buffer);

		// did we dind a 3D card good enough for us?
		if (bFound3DCard)
		{
			DEFAULTS->GetDefaults()->bHardwareRender = true;
		}
		else
		{
			// if we didn't find a card then return false
			DEFAULTS->GetDefaults()->bHardwareRender = false;
		}
	}
}

//--------------------------------------------------------------------------
// return true if mission loaded
static bool init_mission (void)
{
	bool missionLoaded=0;

	if (cmd_line_file[0])	// open the last mission file
	{
		char buffer[MAX_PATH+4];

		strcpy(buffer, cmd_line_file);

		if (buffer[0] || DEFAULTS->GetNameInMRU(buffer, IDS_MISSION_FILTER, 0))
		{
			forceDefaults();
			ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);
			missionLoaded = (MISSION->Load(buffer) != 0);
		}
	}

	if (!missionLoaded)
	{
		if (bStartNewMission)
		{
			ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);
			MISSION->New();
			missionLoaded = 1;
		}
		else
		{
			ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
		}
	}

	return missionLoaded;
}
//--------------------------------------------------------------------------//
//
static void create_bogus_savedir (void)
{
	char buffer[256];
	char name[128];

	U32 result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, name, sizeof(name));
	CQASSERT(result && "We Must have a Multi Player Name set by now");
	
	wsprintf(buffer, "SavedGame\\%s", name);

	DAFILEDESC fdesc = buffer;
	if (DACOM->CreateInstance(&fdesc, (void **)&SAVEDIR) != GR_OK)
	{
		CQERROR1("Could not create save directory '%s'", fdesc.lpFileName);
	}
}
//--------------------------------------------------------------------------
//
static void parse_command_line (const char *lpszCmdLine)
{
	const char * tmp;

	if (strstr(lpszCmdLine, "/w"))
		DEFAULTS->GetDefaults()->bWindowMode=1;

	if ((tmp = strstr(lpszCmdLine, ".mission")) != 0)
	{
		const char * tmp2;
		int len;
		// count back until we get to the begining of the name
		tmp2 = tmp;
		do
		{	
			--tmp;
		} while (tmp >= lpszCmdLine && isspace(*tmp)==0);
		
		tmp++;
		len = tmp2 - tmp;
		memcpy(cmd_line_file, tmp, len+8);
	}

	if ((tmp = strstr(lpszCmdLine, ".qmission")) != 0)
	{
		const char * tmp2;
		int len;
		// count back until we get to the begining of the name
		tmp2 = tmp;
		do
		{	
			--tmp;
		} while (tmp >= lpszCmdLine && isspace(*tmp)==0);
		
		tmp++;
		len = tmp2 - tmp;
		memcpy(cmd_line_file, tmp, len+9);
		cmd_line_file[len+9] = 0;
	}

	if ((tmp = strstr(lpszCmdLine, "/ini=")) != 0)
	{
		const char * tmp2;
		int len;
		tmp += 5; // skip switch string
		while (isspace(*tmp))
			tmp++;
		tmp2 = tmp;
		do
		{	
			tmp++;
		} while (*tmp && isspace(*tmp)==0);
		
		len = tmp - tmp2;
		memcpy(cmd_line_ini, tmp2, len);
		cmd_line_ini[len] = 0;
	}
	if ((tmp = strstr(lpszCmdLine, "/script=")) != 0)
	{
		const char * tmp2;
		int len;
		tmp += 8; // skip switch string
		while (isspace(*tmp))
			tmp++;
		tmp2 = tmp;
		do
		{	
			tmp++;
		} while (*tmp && isspace(*tmp)==0);
		
		len = tmp - tmp2;
		memcpy(cmd_line_script, tmp2, len);
		cmd_line_script[len] = 0;
	}
	if ((tmp = strstr(lpszCmdLine, "/playback")) != 0)
	{
		strcpy(cmd_line_script, "client");
		CQFLAGS.bHostRecordMode = 0;
	}
	if ((tmp = strstr(lpszCmdLine, "/record")) != 0)
	{
		CQFLAGS.bHostRecordMode = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/trace=")) != 0)
	{
		tmp += 7; // skip switch string
		while (isspace(*tmp))
			tmp++;
		TRACELEVEL = atoi(tmp) + SEV_TRACE_1;
	}

	if ((tmp = strstr(lpszCmdLine, "/dumpwin")) != 0)
	{
		CQFLAGS.bDumpWindow = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/dumpfile")) != 0)
	{
		CQFLAGS.bDumpFile = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/nodumpfile")) != 0)
	{
		CQFLAGS.bDumpFile = 0;
	}
	if ((tmp = strstr(lpszCmdLine, "/mdump")) != 0)
	{
		CQFLAGS.bTraceMission = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/ndump")) != 0)
	{
		CQFLAGS.bTraceNetwork = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/rdump")) != 0)
	{
		CQFLAGS.bTraceRules = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/netblocking")) != 0)
	{
		CQFLAGS.bNetBlocking = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/debugprint")) != 0)
	{
		CQFLAGS.debugPrint = DBG_ALL;
	}
	if ((tmp = strstr(lpszCmdLine, "/pdump")) != 0)
	{
		CQFLAGS.bTracePerformance = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/new")) != 0 ||
        (tmp = strstr(lpszCmdLine, "/terran")) != 0)
	{
		bStartNewMission = 1;
		interfaceRace = M_TERRAN;
	}
	if ((tmp = strstr(lpszCmdLine, "/mantis")) != 0)
	{
		bStartNewMission = 1;
		interfaceRace = M_MANTIS;
	}
	if ((tmp = strstr(lpszCmdLine, "/solarian")) != 0)
	{
		bStartNewMission = 1;
		interfaceRace = M_SOLARIAN;
	}
	if ((tmp = strstr(lpszCmdLine, "/vyrium")) != 0)
	{
		bStartNewMission = 1;
		interfaceRace = M_VYRIUM;
	}

	if ((tmp = strstr(lpszCmdLine, "/lobby")) != 0)
	{
		CQFLAGS.bDPLobby = 1;
	}

	if ((tmp = strstr(lpszCmdLine, "/fpu")) != 0)
	{
		CQFLAGS.bFPUExceptions = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/dplay")) != 0)
	{
		CQFLAGS.bStoreDPlayInfo = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/noconfirm")) != 0)
	{
		CQFLAGS.bNoExitConfirm = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/aitest")) != 0)
	{
		CQFLAGS.bAITest = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/skipmovies")) != 0)
	{
		CQFLAGS.bSkipMovies = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/skipintro")) != 0)
	{
		CQFLAGS.bSkipIntroMovie = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/latedelivery")) != 0)
	{
		CQFLAGS.bForceLateDelivery = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/earlydelivery")) != 0)
	{
		CQFLAGS.bForceEarlyDelivery = 1;
	}
	if ((tmp = strstr(lpszCmdLine, "/dshowlog")) != 0)
	{
		CQFLAGS.bDShowLog = 1;
	}
}
//--------------------------------------------------------------------------//
//
static bool hasEnoughSwapDiskSpace (void)
{
	// return true if we have enough swap disk space available to play the game
	// or if we choose to play the game regardless of the rescriction
	MEMORYSTATUS memory;
	::GlobalMemoryStatus(&memory);

	// need a fifty meg swap file
	const U32 swapsize = 1024 * 1024 * 120;
	const U32 available = memory.dwTotalPageFile;

	CQTRACE11("Total page file = %d", memory.dwTotalPageFile);

	if (available < swapsize)
	{
		wchar_t name[64];
		wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
		if (MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_NOSWAP), name, MB_YESNO|MB_ICONEXCLAMATION) == IDNO)
			return false;
	}

	return true;
}
//--------------------------------------------------------------------------
//
static void alloc_failed (void)
{
	wchar_t name[64];
	wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
	MessageBoxW(hMainWindow, _localLoadStringW(IDS_HELP_OUTOFMEMORY), name, MB_OK|MB_ICONSTOP);
}
//--------------------------------------------------------------------------
//
static void check_cqbatch (void)
{
	if (BATCH->QueryInterface(IID_ICQBatch,(void **)&CQBATCH) == GR_OK)
		CQFLAGS.bCQBatcher = TRUE;
	else
	{
		CQBOMB0("No CQBatcher - you will probably crash");
		CQFLAGS.bCQBatcher = FALSE;
	}
}
//--------------------------------------------------------------------------
//
static void dochecksum (HINSTANCE hInstance)
{
	DWORD dwLen;
	char buffer[260];
		
	dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
	SendMessage(hMainWindow, CQE_DATA_CHECKSUM, 2, CreateZBufferAnalog(buffer));
}
//--------------------------------------------------------------------------
//
int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	TRACELEVEL = SEV_TRACE_1;
	hResource = GetModuleHandle("Globals.dll");
	DACOM = DACOM_Acquire();
	SetErrorMode(SEM_FAILCRITICALERRORS);
	cqExceptionHandler = ___UnhandledExceptionFilter;
	setlocale( LC_ALL, "" ); 
	WNDCLASSEX wc;

	HWND hSplash = DoSplashScreen();

	if (hasEnoughSwapDiskSpace() == 0)
		goto Done;

	InitCommonControls();  	// This MUST be called once per instance
                          	// to register the common controls
	//
	// initialize OLE
	//
	CoInitialize(0); bCoInitialized = 1;

	if (CreateUserDefaults() == 0)
		goto Done;
	DEFAULTS->LoadDefaults();

//	ubisoft EULA code, see UserDefaults.cpp
//	if (DEFAULTS->UbiEula() == 0)
//		goto Done;

	// note to self: change this later

#ifndef FINAL_RELEASE
	CQFLAGS.bDumpFile = 1;
	CQFLAGS.bHostRecordMode = 1;
#ifdef _DEBUG
	CQFLAGS.bSkipIntroMovie = 1;
#endif	// end _DEBUG
#endif  // end !FINAL_RELEASE
	parse_command_line(lpCmdLine);

#ifndef FINAL_RELEASE
	if (CQFLAGS.bTraceNetwork==0 && CQFLAGS.bTracePerformance==0)
		CQFLAGS.bTraceMission = 1;
#endif  // end !FINAL_RELEASE


	InitRendSections(cmd_line_ini);

	DWORD messages = DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_GROWHEAP|DAHEAPFLAG_NOHEAPEXPANDMSG;
#ifdef NDEBUG
	messages |=	DAHEAPFLAG_NOMSGS;
#endif

	if (InitializeDAHeap(0x10000, 0x4000, messages)==0)
	{
		alloc_failed();
		goto Done;
	}

	HEAP->SetErrorHandler(ICQImage::STANDARD_DUMP);
	TrimResetHeap(HEAP);
	MissionResetHeap(HEAP);
	GetBatchHeap()->SetErrorHandler(ICQImage::STANDARD_DUMP);

	CQIMAGE->SetMessagesEnabled(false);

	if (DACOM->SetINIConfig(cmd_line_ini) != GR_OK)
	{
		CQIMAGE->SetMessagesEnabled(true);
		CQBOMB1("Open failed on INI file '%s'", cmd_line_ini);
	}

	CQIMAGE->SetMessagesEnabled(true);

	//link up extra images
#ifdef _DEBUG
	CQIMAGE->LoadSymTable(GetModuleHandle("Channel.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("DAHotkey.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("Docuview.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("DOSFile.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("Anim.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("Engine.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("PolyMesh.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("RenderMgr.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("x86math.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("optics.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("Hardpoint.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("Streamer.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("LightManager.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("VertexBufferManager.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("StringTable.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("VideoSystem.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("MaterialManager.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("ParticleEffect.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("MeshManager.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("D3DRenderPipe.dll"));
	CQIMAGE->LoadSymTable(GetModuleHandle("TextureManager.dll"));

#endif

	RegisterContainerFactory();
	RegisterWindowManager();

	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.hInstance     = _hInstance;
	wc.hIcon         = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = 0; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH); 
	char appName[256];
	strncpy(appName,_localLoadString(IDS_APP_NAME),255);
	appName[255] = 0;
	wc.lpszClassName = appName; // "Conquest";

	if (GAME.startup(_hInstance, wc.lpszClassName, exit, 0, &wc) == 0)
		goto Done;

	if (GAME.ENG->QueryInterface(IID_IEngine, (void **) &ENGINE) != GR_OK)
	{
		if (GAME.ENG->QueryInterface("IEngine", (void **) &ENGINE) != GR_OK)
		{
			CQBOMB0("DA Library did not load. Check the INI file.");
		}
		else
			CQBOMB0("Incompatilble IEngine interface");
	}
	else
		ENGINE->Release();		// release the extra reference
	GS = GAME.GS;

	// system container
	GAME.GS->QueryInterface(IID_IWindowManager,(void **) &WM);
	AddToGlobalCleanupList(&WM);
	GAME.GS->QueryInterface(IID_IRenderPipeline,(void **) &PIPE);
	AddToGlobalCleanupList(&PIPE);
	GAME.GS->QueryInterface(IID_IRenderPrimitive,(void **) &BATCH);
	AddToGlobalCleanupList(&BATCH);
	GAME.GS->QueryInterface("IEventSystem",  (void **) &EVENTSYS);
	AddToGlobalCleanupList(&EVENTSYS);
	GAME.GS->QueryInterface(IID_IStreamer,     (void **) &STREAMER);
	AddToGlobalCleanupList(&STREAMER);
	GAME.ENG->QueryInterface(IID_IRenderer,  (void **) &REND);
	AddToGlobalCleanupList(&REND);
	GAME.GS->QueryInterface(IID_ITextureLibrary, (void **) &TEXLIB);
	AddToGlobalCleanupList(&TEXLIB);
	GAME.GS->QueryInterface(IID_ILightManager,(void **)&LIGHT);
	AddToGlobalCleanupList(&LIGHT);
	GAME.GS->QueryInterface(IID_IVertexBufferManager,(void **)&VB_MANAGER);
	AddToGlobalCleanupList(&VB_MANAGER);
	GAME.GS->QueryInterface(IID_IStringTable,(void **) &STRINGTABLE);
	AddToGlobalCleanupList(&STRINGTABLE);
	GAME.GS->QueryInterface(IID_IVideoSystem,(void **) &VIDEOSYS);
	AddToGlobalCleanupList(&VIDEOSYS);
	GAME.GS->QueryInterface(IID_IMaterialManager,(void **) &MATMAN);
	AddToGlobalCleanupList(&MATMAN);
	GAME.GS->QueryInterface(IID_IParticleManager,(void **) &PARTMAN);
	AddToGlobalCleanupList(&PARTMAN);
	GAME.GS->QueryInterface(IID_IMeshManager,(void **) &MESHMAN);
	AddToGlobalCleanupList(&MESHMAN);
	GAME.GS->QueryInterface(IID_ITManager,(void **)&TMANAGER);
	AddToGlobalCleanupList(&TMANAGER);
	
	// engine
	ENGINE->QueryInterface("IAnimation",(void **)&ANIM);
	AddToGlobalCleanupList(&ANIM);
	ENGINE->QueryInterface(IID_IHardpoint,(void **)&HARDPOINT);
	AddToGlobalCleanupList(&HARDPOINT);

	AddToGlobalCleanupList(&DPLAY);
	AddToGlobalCleanupList(&DPLOBBY);
	AddToGlobalCleanupList(&ZONESCORE);
	AddToGlobalCleanupList(&DEBUGFONT);

	ENGINE->AddRef();
	ENGINE->Release();		

	CQASSERT(WM);
	CQASSERT(PIPE);
	CQASSERT(BATCH);
	CQASSERT(EVENTSYS);
	CQASSERT(STREAMER);
	CQASSERT(REND);
	CQASSERT(TEXLIB);
	CQASSERT(LIGHT);
	CQASSERT(ANIM);
	CQASSERT(HARDPOINT);
	CQASSERT(VB_MANAGER);
	CQASSERT(STRINGTABLE);
	CQASSERT(VIDEOSYS);
	CQASSERT(MATMAN);
	CQASSERT(PARTMAN);
	CQASSERT(MESHMAN);

	check_cqbatch();

	if ((hMainWindow = WM->GetWindowHandle()) == 0)
		goto Done;
	if ((hMainDC = GetDC(hMainWindow)) == 0)
		goto Done;

	ReadRenderOptions();
	SetupWindowCallback();
	
	ParseVideoINI();

	// add an extra 500000 if GDI is used, because viewers, documents use that much!
	if (InitializeDAHeap(HEAP_SIZE+((CQFLAGS.bNoGDI)?0:500000), 0x800000, messages)==0)
	{
		alloc_failed();
		goto Done;
	}

	{
		// make sure we have enough memory to play the game
		void * pMem = VirtualAlloc(0, 120*1024*1024 - HEAP_SIZE, MEM_COMMIT, PAGE_NOACCESS);
		if (pMem==0)
		{
			alloc_failed();
			goto Done;
		}
		VirtualFree(pMem, 0, MEM_RELEASE);
	}


	MarkAllocatedBlocks(HEAP);
	MarkAllocatedBlocks(GetBatchHeap());

	CreateGlobalComponents();

	IParticleManager::InitInfo pInfo;
	pInfo.camera = MAINCAM;
	pInfo.posCallback = GAMEPOSCALLBACK;
	pInfo.MATMAN = MATMAN;
	pInfo.MESHMAN = MESHMAN;
	PARTMAN->InitParticles(pInfo);

	IMeshManager::InitInfo mInfo;
	mInfo.ENGINE = ENGINE;
	mInfo.ANIM = ANIM;
	mInfo.OBJDIR = OBJECTDIR;
	mInfo.REND = REND;
	mInfo.PIPE = PIPE;
	mInfo.CAMERA = MAINCAM;
	mInfo.HARDPOINT = HARDPOINT;
	mInfo.MATMAN = MATMAN;
	MESHMAN->InitManager(mInfo);

	ITManager::InitInfo tInfo;
	tInfo.PIPE = PIPE;
	TMANAGER->Initialize(tInfo);

	dochecksum(_hInstance);

	memset( &CQ_SplashInfo, 0, sizeof(CQ_SplashInfo) );

	atexit(clean_up);		// don't set clean_up until now -- only clean up if started up properly, avoids screwing up registry (jy)

	//PB.SetPipeline(PIPE);
	PB.SetIRenderPrimitive(BATCH);

	if (DEFAULTS->GetDefaults()->bWindowMode==0)
		CQFLAGS.bFullScreen = 1;
	
	Start3DMode();

	IMaterialManager::InitInfo matInfo;
	matInfo.PIPE = PIPE;
	matInfo.TMANAGER = TMANAGER;
	matInfo.MATDIR = MATERIALDIR;
	MATMAN->Initialize(matInfo);
	VIDEOSYS->Initialize(MOVIEDIR,PIPE);

	if (!DEFAULTS->GetDefaults()->bWindowMode)
	{
		DEVMODE dm;
		dm.dmSize = sizeof(dm);
		dm.dmPelsHeight = SCREENRESY;
		dm.dmPelsWidth = SCREENRESX;
		dm.dmFields = DM_PELSHEIGHT | DM_PELSWIDTH;
		ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
		WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_FULL_SCREEN);	
		PIPE->destroy_buffers();
	}
	else
	{
		ChangeDisplaySettings(NULL, 0);
		WM->SetWindowPos(SCREENRESX, SCREENRESY, WMF_CENTER);	
		PIPE->destroy_buffers();
	}

	if (cmd_line_script[0])
	{
		create_bogus_savedir();
		ShutdownSplash(hSplash);
		hSplash = NULL;
		RunTestScript(cmd_line_script);
	}
	else
	if (init_mission())
	{
		create_bogus_savedir();
		CQFLAGS.bGameActive = 1;
		EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)1);
		MSCROLL->SetActive(1);
		ShutdownSplash(hSplash);
		hSplash = NULL;
		U32 missionResult = CQDoModal(0);
		if (missionResult==MISSION_END_SPLASH)
		{
			// flushing game systems
			CQFLAGS.bGameActive = 0;
			EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)0);
			if (PLAYERID)
				DoMenu_nul(missionResult==MISSION_END_RESIGN);	// do net unloading
			StopNetConnection(true);
			MISSION->Close();
			FULLSCREEN->Notify(CQE_UPDATE, 0);	// give all menus a chance to destruct
			GENDATA->FlushUnusedArchetypes();
			HEAP->HeapMinimize();
			ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);

			if (CQ_SplashInfo.vfxName[0])
				DoMenu_Splash(CQ_SplashInfo);
		}
	}
	else
	{
		ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
		ShutdownSplash(hSplash);
		hSplash = NULL;
		bool bSkipMenus = false;
		while (1)
		{
			if (CreateMenu1(bSkipMenus)==0)
				break;
			bSkipMenus = false;
			FULLSCREEN->Notify(CQE_UPDATE, 0);	// give all menus a chance to destruct
			GENDATA->FlushUnusedArchetypes();
			CQFLAGS.bGameActive = 1;
			EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)1);
			MSCROLL->SetActive(1);
			U32 missionResult = CQDoModal(0);
			if (missionResult==0)
				break;
			else
			if (missionResult==MISSION_END_RESIGN)
			{
				if (PLAYERID != 0)		// network session?
				{
					// destroy units if I am the only one on the team
					if (PLAYERID == HOSTID && MGlobals::GetPlayerDPIDForPlayerID(MGlobals::GetThisPlayer()) <= 1)
					{
						// can't blow up units if not the master!
						if (THEMATRIX->IsMaster() == false)
							missionResult = MISSION_END_QUIT;
						else
							THEMATRIX->TerminatePlayerUnits(MGlobals::GetThisPlayer());
					}
				}
			}
			CQFLAGS.bGameActive = 0;
			EVENTSYS->Send(CQE_GAME_ACTIVE, (void*)0);
			if (PLAYERID)
				DoMenu_nul(missionResult==MISSION_END_RESIGN);	// do net unloading
			StopNetConnection(true);
			MISSION->Close();
			ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
			FULLSCREEN->Notify(CQE_UPDATE, 0);	// give all menus a chance to destruct
			GENDATA->FlushUnusedArchetypes();
			HEAP->HeapMinimize();

			switch (missionResult)
			{
			case MISSION_END_QUIT:
			case MISSION_END_RESIGN:
				if (MISSION->IsSinglePlayerGame() == false)
				{
					CreateMenuEndGame(false);
				}
				bSkipMenus = MGlobals::IsSinglePlayer();
				break;
			case MISSION_END_WON:
				CreateMenuEndGame(true);
				bSkipMenus = MGlobals::IsSinglePlayer();
				break;
			case MISSION_END_LOST:
				CreateMenuEndGame(false);
				bSkipMenus = MGlobals::IsSinglePlayer();
				break;
			case MISSION_END_SPLASH:
				DoMenu_Splash(CQ_SplashInfo);
				bSkipMenus = MGlobals::IsSinglePlayer();
				break;
			}
			GENDATA->FlushUnusedArchetypes();
			MSCROLL->SetActive(0);
			if (CQFLAGS.bDPLobby)
				break;
		}
	}
	Shutdown3DMode();

Done:
	if (EVENTSYS)
		EVENTSYS->Send(WM_CLOSE, 0);		// force toolbar, etc to close
	if (FULLSCREEN)
		FULLSCREEN->Notify(CQE_UPDATE, 0);	// force one last update

	return 0;
}

//----------------------------------------------------------------------------------
//------------------------END Conquest.cpp------------------------------------------
//----------------------------------------------------------------------------------
