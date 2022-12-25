//-----------------------------------------------------------------------------------------------------
// DACOM stuff
// todo(aaj-4/12/2004): could be moved to its own CPP file
//-----------------------------------------------------------------------------------------------------

#include "stdafx.h"

#define CQ2_MAIN
#include "globals.h"

#include "Editor.h"
#include "Camera.h"
#include "Undo.h"
#include "Mode.h"

#include <GameSys.h>
#include <renderer.h>
#include <IHardPoint.h>
#include <IRenderPrimitive.h>
#include <LightMan.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <EventSys.h>
#include <IAnim.h>
#include <FileSys.h>
#include <StringTable.h>
#include <IPython.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool InitDacom(HWND _hWnd)
{
	ZeroMemory( &CQFLAGS, sizeof(CQFLAGS) );

	CQFLAGS.b3DEnabled         = false;
	CQFLAGS.bPrimaryDevice     = true;
	CQFLAGS.bFPUExceptions     = false;
	CQFLAGS.bFullScreen        = false;
	CQFLAGS.bNoGDI             = false;
	CQFLAGS.bFrameLockEnabled  = false;
	CQFLAGS.bWindowModeAllowed = true;
	CQFLAGS.bHardwareGeometry  = true;
	CQFLAGS.bExtCameraZoom     = true;
	CQFLAGS.bGameActive        = false;

	ZeroMemory( &CQRENDERFLAGS, sizeof(CQRENDERFLAGS) );

	CQRENDERFLAGS.b32BitTextures    = true;
	CQRENDERFLAGS.bNoPerVertexAlpha = true;
	CQRENDERFLAGS.bSoftwareRenderer = false;
	CQRENDERFLAGS.bMultiTexture     = true;
	CQRENDERFLAGS.bHardwareGeometry = true;
	CQRENDERFLAGS.bStallPipeline    = false;
	CQRENDERFLAGS.bFSAA             = false;
	CQRENDERFLAGS.bBackground       = true;

	DACOM = DACOM_Acquire();
	if( !DACOM )
	{
		return false;
	}

	hMainWindow = _hWnd;
	hMainDC     = GetDC( hMainWindow );
	hMainInst   = AfxGetInstanceHandle();

	if( DACOM->SetINIConfig("Editor.ini") != GR_OK )
	{
		return false;
	}

	// creating & loading system components

	void __stdcall RegisterContainerFactory (void);
	RegisterContainerFactory();

	AGGDESC adesc = "ISystemContainer";
	if (DACOM->CreateInstance(&adesc, (void **) &SYSTEM) != GR_OK)
	{
		return false;
	}
	SYSTEM->LoadSystemComponents();

	SYSTEM->QueryInterface(IID_IRenderPipeline,(void **) &PIPE);
	SYSTEM->QueryInterface(IID_IRenderPrimitive,(void **) &BATCH);
	SYSTEM->QueryInterface(IID_ITextureLibrary, (void **) &TEXLIB);
	SYSTEM->QueryInterface(IID_ILightManager,(void **)&LIGHT);
	SYSTEM->QueryInterface(IID_IVertexBufferManager,(void **)&VB_MANAGER);
	SYSTEM->QueryInterface(IID_IEventSystem,(void**)&EVENTSYS);
	SYSTEM->QueryInterface(IID_IStringTable,(void**)&STRINGTABLE);
	SYSTEM->QueryInterface(IID_IPython,(void**)&PYTHON);

	GS = SYSTEM;

	// creating and loading engine components

	DACOMDESC desc = "IEngine";
	if (DACOM->CreateInstance(&desc, (void **)&ENGINE) == GR_OK)
	{
	}
	ENGINE->load_engine_components(SYSTEM);

	ENGINE->QueryInterface(IID_IRenderer,  (void **)&REND);
	ENGINE->QueryInterface(IID_IAnimation,(void **)&ANIM);
	ENGINE->QueryInterface(IID_IHardpoint,(void **)&HARDPOINT);

	AddToGlobalCleanupList(&REND);
	AddToGlobalCleanupList(&BATCH);
	AddToGlobalCleanupList(&PIPE);
	AddToGlobalCleanupList(&TEXLIB);
	AddToGlobalCleanupList(&LIGHT);
	AddToGlobalCleanupList(&VB_MANAGER);
	AddToGlobalCleanupList(&ANIM);
	AddToGlobalCleanupList(&HARDPOINT);
	AddToGlobalCleanupList(&EVENTSYS);
	AddToGlobalCleanupList(&PYTHON);
	AddToGlobalCleanupList(&STRINGTABLE);

	// create directories
	{
		// TODO: figure out why "..\\..\\Data\\Objects" does not work

		DAFILEDESC fdesc;
		fdesc = "Z:\\CQ2\\DATA\\OBJECTS";

		DACOM->CreateInstance(&fdesc, (void **)&OBJECTDIR);
		AddToGlobalCleanupList(&OBJECTDIR);
	}

	ENGINE->set_search_path2(OBJECTDIR);

	SetCleanUpAtExit();

	// in cleanup.cpp
	void __stdcall CreateGlobalComponents (void);
	CreateGlobalComponents();

	// look in RenderSystem.cpp
	void __stdcall Enable3DMode (bool bEnable);
	Enable3DMode(true);

	PIPE->set_window( hMainWindow, 0, 0, 640, 480 );
	PIPE->set_pipeline_state(RP_CLEAR_COLOR,0xff000000);  // a,r,g,b
	PIPE->set_render_state(D3DRS_ZENABLE,TRUE);

	if( PIPE->create_buffers( hMainWindow, 640, 480 ) != GR_OK )
	{
		return 0;	
	}

	// PB.SetIRenderPrimitive(BATCH);
	PB.SetPipeline(PIPE);

	// loads the global string table from Conquest

	hStringTable = NULL;
	CString libString = "Globals.dll";
	HMODULE libStringTable = LoadLibrary(libString);
	if( !libStringTable )
	{
		libString = "Z:\\CQ2\\Code\\App\\Src\\Debug\\Globals.dll";
		libStringTable = LoadLibrary(libString);
	}
	if( libStringTable )
	{
		hStringTable = GetModuleHandle(libString);
	}

	// set up the texture library
	if( TEXLIB )
	{
		// set the texture's LOD state to 1.0
		SINGLE dummy = 1.0f;
		TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD, TRUE );
		TEXLIB->set_library_state( ITL_STATE_TEXTURE_LOD_LOAD_SCALE, *((U32 *)&dummy));

		// enable point level mipping
		PIPE->set_sampler_state( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
		PIPE->set_sampler_state( 1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
	}

	CQEDITORMODE = EM_CAMPAIGN;
	MODE_CAMPAIGN->Start();

	// ready to go
	CQFLAGS.b3DEnabled = true;

	return true;
}

/*

IID_IRenderPipeline
IID_IRenderPrimitive
IID_ITextureLibrary
IID_ILightManager
IID_IVertexBufferManager
IID_IEventSystem
IID_IStringTable
IID_IPython
IID_IRenderer
IID_IAnimation
IID_IHardpoint

*/