// EffectEd.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#define SOM_MAIN
#include <commctrl.h>
#include <commdlg.h>
#include <stdlib.h>
#include "MyVertex.h"
#include "TMANAGER.h"

#include "globals.h"
#include "resource.h"
#include <DACOM.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <Engine.h>
#include <RendPipeline.h>
#include <IVertexBufferManager.h>
#include <ITextureLibrary.h>
#include <IParticleManager.h>
#include <IMeshManager.h>
#include "SysManager.h"
#include <IMaterialManager.h>

//#include <SoundSys.h>
//#include <Streamer.h>

#include "IEffectFile.h"
#include "InfoArea.h"
#include "PreviewWin.h"
#include "Startup.h"
#include "IEffectTarget.h"
#include "EventGraph.h"
#include "IEffectAction.h"
#include "IEffectEvent.h"
#include "IEffectParam.h"
#include "Camera.h"
#include "SplitterWnd.h"

SystemManager systemManager;

HIMAGELIST tabImageList;

LONG CALLBACK eventGraphProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
LONG CALLBACK infoAreaProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

#define MAX_BUTTONS 32

#define ICON_CX 16
#define ICON_CY 16

extern IEffectEvent * selectedEvent;//TimeBar.cpp
extern IEffectAction * selectedAction;
extern HWND particleWorkArea; //the work area

HACCEL hAccel = NULL;

//--------------------------------------------------------------------------//
//
#define NUM_CLEANUP_PTRS 64
struct CLEANUP_NODE
{
	struct CLEANUP_NODE * pNext;
	U32 numUsed;
	IDAComponent ** component[NUM_CLEANUP_PTRS];

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};
static CLEANUP_NODE * cleanupList;
//------------------------------------------------------------------------
//
template <> void AddToGlobalCleanupList (IDAComponent ** component)
{
	//
	// find an empty place on the list
	//
	if (cleanupList == 0 || cleanupList->numUsed >= NUM_CLEANUP_PTRS)
	{
		CLEANUP_NODE * node = new CLEANUP_NODE;
		node->pNext = cleanupList;
		cleanupList = node;
	}

	cleanupList->component[cleanupList->numUsed++] = component;
}
//-----------------------------------------------------------------------------
// delete everyone in the cleanup list
//
void CleanupGlobals (void)
{
	S32 numUsed;
	CLEANUP_NODE * node = cleanupList;

	while (node)
	{
		numUsed = node->numUsed;
		while (numUsed-- > 0)
		{
			if (*node->component[numUsed])
				(*node->component[numUsed])->Release();
			*node->component[numUsed] = 0;
		}
		node = node->pNext;
		delete cleanupList;
		cleanupList = node;
	}
}

void fixWindowSize()
{
	RECT clientRect;
	GetClientRect(mainWindow,&clientRect);

	RECT toolRect;
	GetWindowRect(hMenuBar,&toolRect);

	U32 toolHeight = toolRect.bottom-toolRect.top;
	U32 clientHeight = (clientRect.bottom-clientRect.top)-toolHeight;
	
	MoveWindow(hMenuBar,0,0,clientRect.right-clientRect.left,toolHeight,true);
	MoveWindow(splitter,0,toolHeight,clientRect.right-clientRect.left,clientHeight,true);
//	MoveWindow(eventGraph,0,(clientHeight/2)+toolHeight,clientRect.right-clientRect.left,(clientHeight/2),true);
//	MoveWindow(infoArea,0,toolHeight,clientRect.right-clientRect.left,(clientHeight/2),true);
}

void updateTitle()
{
	char title[MAX_PATH] = {"Effect Editor - "};
	strcat( title, EFFECTFILE->GetFileName() );
	::SetWindowText( mainWindow, title );
}

void saveAs()
{
	char buffer[255];
	buffer[0] = 0;
    OPENFILENAME ofn; // common dialog box structure
	memset(&ofn,0,sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = "Effect Files\0*.EFF\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = 255;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
    			OFN_HIDEREADONLY;

    // Display the Open dialog box.

    if (GetSaveFileName(&ofn))
	{
		EFFECTFILE->Save(buffer);
		updateTitle();
	}
}

void load()
{
	char buffer[255];
	buffer[0] = 0;
    OPENFILENAME ofn; // common dialog box structure
	memset(&ofn,0,sizeof(OPENFILENAME));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = mainWindow;
    ofn.lpstrFilter = "Effect Files\0*.EFF\0\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = 255;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
    			OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    // Display the Open dialog box.

    if (GetOpenFileName(&ofn))
	{
		EFFECTFILE->Load(buffer);
		updateTitle();
	}
}

LONG CALLBACK mainWinProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
 	case WM_MOUSEWHEEL:
		{
			S16 xPos = (S16)(LOWORD(lParam));
			S16 yPos = (S16)(HIWORD(lParam));
			POINT point;
			point.x = xPos;
			point.y = yPos;

			ScreenToClient(eventGraph,&point);
			RECT rect;
			GetClientRect(eventGraph,&rect);
			if(point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom)
			{
				SendMessage(eventGraph,message,wParam,lParam);
			}

			point.x = xPos;
			point.y = yPos;
			ScreenToClient(particleWorkArea,&point);
			GetClientRect(particleWorkArea,&rect);
			if(point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom)
			{
				SendMessage(particleWorkArea,message,wParam,lParam);
			}

			return 1;
		};
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDD_FILE_EXIT:
			{
				PostQuitMessage(0);
				break;
			}
		case IDD_FILE_LOAD:
			{
				load();
				EventGraph::InvalidateEventGraph();
				break;
			}
		case IDD_FILE_NEW:
			{
				EFFECTFILE->New();
				EventGraph::InvalidateEventGraph();
				break;
			}
		case IDD_FILE_SAVE:
			{
				if(strcmp(EFFECTFILE->GetFileName(),"Untitled") != 0)
				{
					EFFECTFILE->Save(EFFECTFILE->GetFileName());
					updateTitle();
					break;
				}
				//fall through intentional
			}
		case IDD_FILE_SAVEAS:
			{
				saveAs();
				break;
			}

		case IDD_SELECT_TARGET:
			{
				InfoArea::SelectTargetList();
				break;
			};

		case IDM_SELECT_PARAM:
			{
				InfoArea::SelectParamList();
				break;
			};
		case IDM_PREVIEW_WINDOW:
			{
				PreviewWin::Open();
				break;
			};
		case ID_VIEW_MATERIALMANAGER:
			{
				MATMAN->OpenEditWindow(NULL);
				break;
			};
		case IDM_CUT:
			{
				if(selectedAction)
				{
					EFFECTFILE->CopyAction(selectedAction);
					EFFECTFILE->DeleteAction(selectedAction);
					selectedAction = NULL;
					EventGraph::Deselect();
					InvalidateRect(mainWindow,NULL,false);
				}
				break;
			};
		case IDM_COPY:
			{
				if(selectedAction)
					EFFECTFILE->CopyAction(selectedAction);
				break;
			};
		case IDM_PASTE:
			{
				if(selectedEvent)
				{
					EFFECTFILE->PasteAction(selectedEvent);
					InvalidateRect(mainWindow,NULL,false);
				}
				break;
			};
		}
		break;
	case WM_SIZE:
		{
			fixWindowSize();
			break;
		}
	case WM_CLOSE:
		{
			PostQuitMessage(0);
		}break;
/*	case SOM_STREAMER:
		{
			if(SOUNDSYS)
			{
				SOUNDSYS->StreamerMessage(wParam,lParam);
			}
		}
		break;
*/	}
	return DefWindowProc(hWindow,message,wParam,lParam);
}

//--------------------------------------------------------------------------
//  
void createMenuBar(HWND hWindow)
{
	TBBUTTON tbArray[MAX_BUTTONS];

	hMenuBar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|TBSTYLE_TRANSPARENT|TBSTYLE_TOOLTIPS|CCS_ADJUSTABLE,
								0, 0, 0, 0, hWindow, NULL, hMainInst, NULL);

    SendMessage(hMenuBar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
	SendMessage(hMenuBar, TB_SETMAXTEXTROWS, 1, 0L);
	SendMessage(hMenuBar, TB_SETBITMAPSIZE, 0, (LPARAM)MAKELONG(ICON_CX, ICON_CY));

	tabImageList = ImageList_Create(ICON_CX,ICON_CY,ILC_COLOR8,0,15);
	ImageList_Add(tabImageList, LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_NEWBUTTON)), NULL);
	ImageList_Add(tabImageList, LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_LOADBUTTON)), NULL);
	ImageList_Add(tabImageList, LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_SAVEBUTTON)), NULL);
	ImageList_Add(tabImageList, LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_TARGET_LIST)), NULL);
	ImageList_Add(tabImageList, LoadBitmap(hMainInst,MAKEINTRESOURCE(IDB_PARAMETER_LIST)), NULL);

	SendMessage(hMenuBar, TB_SETIMAGELIST, 0, (LPARAM)tabImageList);

	// fill the array of TBBUTTON structures.
	int i = 0;
	tbArray[i].iBitmap   = 0;
	tbArray[i].idCommand = IDD_FILE_NEW;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_BUTTON;
	tbArray[i].dwData    = 0;
	tbArray[i].iString   = i;
	i++;

	tbArray[i].iBitmap   = 0;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_SEP;
	i++;

	tbArray[i].iBitmap   = 1;
	tbArray[i].idCommand = IDD_FILE_LOAD;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_BUTTON;
	tbArray[i].dwData    = 0;
	tbArray[i].iString   = i;
	i++;

	tbArray[i].iBitmap   = 2;
	tbArray[i].idCommand = IDD_FILE_SAVE;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_BUTTON;
	tbArray[i].dwData    = 0;
	tbArray[i].iString   = i;
	i++;

	tbArray[i].iBitmap   = 0;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_SEP;
	i++;

	tbArray[i].iBitmap   = 3;
	tbArray[i].idCommand = IDD_SELECT_TARGET;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_BUTTON;
	tbArray[i].dwData    = 0;
	tbArray[i].iString   = i;
	i++;

	tbArray[i].iBitmap   = 4;
	tbArray[i].idCommand = IDM_SELECT_PARAM;
	tbArray[i].fsState   = TBSTATE_ENABLED;
	tbArray[i].fsStyle   = TBSTYLE_BUTTON;
	tbArray[i].dwData    = 0;
	tbArray[i].iString   = i;
	i++;

	SendMessage(hMenuBar, TB_ADDBUTTONS, (UINT)i, (LPARAM)tbArray);

	ShowWindow(hMenuBar,true);
}

struct PositionListener: public IGamePositionCallback
{
	PositionListener()
	{
	}

	//IGamePositionCallback
	virtual bool GetObjectTransform(U32 objectID, U32 hardpoint, DWORD context, Transform & trans)
	{
		IEffectTarget * search = EFFECTFILE->GetFirstTarget();
		while(search)
		{
			if(search->GetTargetID() == objectID)
			{
				search->GetHardPointTransform(hardpoint,trans);
				return true;
			}
			search = search->GetNextTarget();
		}
		return false;
	};

	virtual bool GetObjectTransform(U32 objectID, const char * hpName, DWORD context, Transform & trans)
	{
		IEffectTarget * search = EFFECTFILE->GetFirstTarget();
		while(search)
		{
			if(search->GetTargetID() == objectID)
			{
				search->GetHardPointTransform(search->GetHardPointIndex(hpName),trans);
				return true;
			}
			search = search->GetNextTarget();
		}
		return false;
	}

	virtual SINGLE GetParameter(const char * name, U32 objectID,DWORD context)
	{
//		if(objectID == -1)
//		{
			IEffectParam * param = EFFECTFILE->GetFirstParam();
			while(param)
			{
				if(strcmp(param->GetName(),name) == 0)
				{
					return param->GetValue();
				}
				param = param->GetNextParam();
			}
//		}
		return 0;
	};

	void ObjectPostionCallback(U32 objectID,DWORD context, char * string, Transform & trans)
	{
		//I don't care.....
	}

	virtual bool TestCollision(Vector p1,Vector p2,Vector & collisionPoint,Vector & finalPoint,bool bTerrain,bool bTinkerToys,bool bWater,bool bUnits)
	{
		if(bTerrain || bTinkerToys || bWater)
		{
			if((p1.z >= 0 && p2.z < 0) || (p1.z <= 0 && p2.z > 0))
			{
				finalPoint.x = p2.x;
				finalPoint.y = p2.y;
				finalPoint.z = -p2.z;
				
				SINGLE t = (-p1.z)/(p2.z-p1.z);
				collisionPoint = (t*(p2-p1))+p1;
				return true;
			}
		}
		//Units??
		return false;
	}

	virtual void ShakeCamera(SINGLE durration, SINGLE power)
	{
		CAMERA->CameraShake(durration,power);
	}

	virtual IMeshInstance * GetObjectMesh(U32 objectID, DWORD context)
	{
		IEffectTarget * search = EFFECTFILE->GetFirstTarget();
		while(search)
		{
			if(search->GetTargetID() == objectID)
			{
				return search->GetMesh();
			}
			search = search->GetNextTarget();
		}
		return NULL;
	}
};

PositionListener posList;

bool setup(HINSTANCE hInstance)
{
	hMainInst = hInstance;
	InitCommonControls();
//set up main window and its' children
	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = mainWinProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor       = 0; // LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL; //(HBRUSH)(COLOR_APPWORKSPACE+1); // GetStockObject(BLACK_BRUSH); 
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "Effect Editor";

	if(RegisterClassEx(&wc) == 0)
		return false;

	mainWindow = CreateWindowEx(
		0,
		wc.lpszClassName,
		"Effect Editor",
		WS_OVERLAPPEDWINDOW, 
		0,
		0,
		800,
		600,
		NULL,
		LoadMenu(NULL, MAKEINTRESOURCE(IDR_MAINMENU)),
		hInstance,
		NULL);

	if(!mainWindow)
		return false;

	SplitterWnd_RegisterClass(hInstance);
	RECT clientRect;
	GetClientRect(mainWindow,&clientRect);
	splitter = SplitterWnd_Create (mainWindow, hInstance, NULL , SWS_HORIZONTAL, LoadCursor(NULL, IDC_SIZENS), &clientRect);
	
	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = eventGraphProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1); // GetStockObject(BLACK_BRUSH); 
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "EventGraph";

	if(RegisterClassEx(&wc) == 0)
		return false;

	eventGraph = CreateWindowEx(
		0,
		wc.lpszClassName,
		"EventGraph",
		WS_CHILDWINDOW|WS_HSCROLL|WS_VSCROLL, 
		0,
		300,
		800,
		300,
		splitter,
		NULL/*LoadMenu(hResource, MAKEINTRESOURCE(IDR_MENU1))*/,
		hInstance,
		NULL);

	if(!eventGraph)
		return false;

	memset(&wc, 0, sizeof(wc));
	wc.cbSize		 = sizeof(wc);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = infoAreaProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1); // GetStockObject(BLACK_BRUSH); 
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "InfoArea";

	if(RegisterClassEx(&wc) == 0)
		return false;

	infoArea = CreateWindowEx(
		0,
		wc.lpszClassName,
		"InfoArea",
		WS_CHILDWINDOW, 
		0,
		50,
		800,
		250,
		splitter,
		NULL/*LoadMenu(hResource, MAKEINTRESOURCE(IDR_MENU1))*/,
		hInstance,
		NULL);

	if(!infoArea)
		return false;

	SplitterWnd_SetPanes(splitter,infoArea,eventGraph);

	createMenuBar(mainWindow);

	ShowWindow(infoArea,true);
	ShowWindow(eventGraph,true);

	fixWindowSize();

	SplitterWnd_SetSplitPos(splitter,300);

	ShowWindow(mainWindow,true);

	DACOM = DACOM_Acquire();
	if(!DACOM)
		return false;

	//init the heap with a little memory to get everything going
	if (InitializeDAHeap(0x4000000, 0x800000, DAHEAPFLAG_DEBUGFILL_SNAN|DAHEAPFLAG_GROWHEAP/*|DAHEAPFLAG_NOMSGS*/)==0)
	{
		return false;
	}

	if (DACOM->SetINIConfig("EffectEd.ini") != GR_OK)
		return false;
	
	if (systemManager.startup() == 0)
		return false;

	if (systemManager.engine->QueryInterface(IID_IEngine, (void **) &ENGINE) != GR_OK)
	{
		return false;
	}
	else
		ENGINE->Release();		// release the extra reference

	ENGINE->QueryInterface("IAnimation",(void **)&ANIM);
	AddToGlobalCleanupList(&ANIM);
	ENGINE->QueryInterface("IRenderer",(void **)&REND);
	AddToGlobalCleanupList(&REND);
	ENGINE->QueryInterface("IHardpoint",(void **)&HARDPOINT);
	AddToGlobalCleanupList(&HARDPOINT);

	systemManager.container->QueryInterface(IID_IRenderPipeline,(void **) &PIPE);
	AddToGlobalCleanupList(&PIPE);
	systemManager.container->QueryInterface(IID_ITextureLibrary, (void **) &TEXLIB);
	AddToGlobalCleanupList(&TEXLIB);
	systemManager.container->QueryInterface(IID_IVertexBufferManager,(void **)&VB_MANAGER);
	AddToGlobalCleanupList(&VB_MANAGER);
	systemManager.container->QueryInterface(IID_IParticleManager,(void **)&PARTMAN);
	AddToGlobalCleanupList(&PARTMAN);
	systemManager.container->QueryInterface(IID_IMeshManager,(void **)&MESHMAN);
	AddToGlobalCleanupList(&MESHMAN);
	systemManager.container->QueryInterface(IID_IMaterialManager,(void **)&MATMAN);
	AddToGlobalCleanupList(&MATMAN);
	systemManager.container->QueryInterface(IID_ITManager,(void **)&TMANAGER);
	AddToGlobalCleanupList(&TMANAGER);

	DAFILEDESC fdesc = "C:\\tmauer\\CQ2_copy\\Data\\Textures";
	DACOM->CreateInstance(&fdesc, (void **)&TEXTURESDIR);
	AddToGlobalCleanupList(&TEXTURESDIR);

	fdesc = "C:\\tmauer\\CQ2_copy\\Data\\Objects";
	DACOM->CreateInstance(&fdesc, (void **)&OBJECTS);
	AddToGlobalCleanupList(&OBJECTS);

	fdesc = "C:\\tmauer\\CQ2_copy\\Data\\SFX";
	DACOM->CreateInstance(&fdesc, (void **)&SOUNDDIR);
	AddToGlobalCleanupList(&SOUNDDIR);

	fdesc = "C:\\tmauer\\CQ2_copy\\Data\\mat";
	DACOM->CreateInstance(&fdesc, (void **)&MATERIALDIR);
	AddToGlobalCleanupList(&MATERIALDIR);

	PB.SetPipeline(PIPE);

	IMaterialManager::InitInfo matInfo;
	matInfo.PIPE = PIPE;
	matInfo.TMANAGER = TMANAGER;
	matInfo.MATDIR = MATERIALDIR;
	MATMAN->Initialize(matInfo);

	ITManager::InitInfo tInfo;
	tInfo.PIPE = PIPE;
	TMANAGER->Initialize(tInfo);

/*	AGGDESC soundDesc("ISoundSystem");
	IDAComponent * pOuter;
	DACOM->CreateInstance(&soundDesc,(void **) &pOuter);
	pOuter->QueryInterface("IAggregateComponent",(void **)&AGG_SOUNDSYS);
	AddToGlobalCleanupList(&AGG_SOUNDSYS);
	pOuter->QueryInterface("ISystemComponent",(void **)&SYS_SOUNDSYS);
	AddToGlobalCleanupList(&SYS_SOUNDSYS);
	pOuter->QueryInterface("ISoundSystem",(void **)&SOUNDSYS);
	AddToGlobalCleanupList(&SOUNDSYS);
	AGG_SOUNDSYS->Initialize();

	AGGDESC streamDesc("IStreamer");
	DACOM->CreateInstance(&streamDesc,(void **) &pOuter);
	pOuter->QueryInterface("IAggregateComponent",(void **)&AGG_STREAMER);
	AddToGlobalCleanupList(&AGG_STREAMER);
	pOuter->QueryInterface("IStreamer",(void **)&STREAMER);
	AddToGlobalCleanupList(&STREAMER);
	AGG_STREAMER->Initialize();
*/
	CreateGlobalComponents();

	IParticleManager::InitInfo pInfo;
	pInfo.camera = MAINCAM;
	pInfo.posCallback = &posList;
	pInfo.MATMAN = MATMAN;
	pInfo.MESHMAN = MESHMAN;
	PARTMAN->InitParticles(pInfo);

	IMeshManager::InitInfo mInfo;
	mInfo.ENGINE = ENGINE;
	mInfo.ANIM = ANIM;
	mInfo.OBJDIR = OBJECTS;
	mInfo.REND = REND;
	mInfo.PIPE = PIPE;
	mInfo.CAMERA = MAINCAM;
	mInfo.HARDPOINT = HARDPOINT;
	mInfo.MATMAN = MATMAN;
	MESHMAN->InitManager(mInfo);

	PreviewWin::Create();

//	SOUNDSYS->Initialize(previewWin,32,SOUNDCALLBACK);
	
/*	STREAMERDESC sDesc;
	sDesc.lpDSound = SOUNDSYS->GetDirectSound();
	sDesc.hMainWindow = previewWin;
	sDesc.uMsg = SOM_STREAMER;
	sDesc.readBufferTime = 4.0;		// in seconds, 0 = default setting (4.0 is a reasonable value)
	sDesc.soundBufferTime = 0.25;		// in seconds, 0 = default setting (0.25 is a reasonable value)

	STREAMER->Init(&sDesc);
	SOUNDSYS->SetStreamer(STREAMER);
*/

	hAccel = ::LoadAccelerators( hMainInst, MAKEINTRESOURCE(IDR_ACCELERATOR) );

	return true;
};

void runMessageCue()
{
	while(1)
	{
		MSG msg;
		//making sure there is a message on the queue before moving on
		while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
		{
			//using peek message b/c getmessage was eating certain mouse clicks
			if(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE) && msg.message == WM_QUIT)
			{
				return;
			}

			if( !TranslateAccelerator(msg.hwnd,hAccel,&msg) )
			{
				TranslateMessage(&msg); 
				DispatchMessage(&msg);
			}
		}

		systemManager.container->Update();

		PreviewWin::Update();
	}
};

void shutdown()
{
	PB.SetPipeline(NULL);

	PreviewWin::Close();

	CleanupGlobals();

	systemManager.shut_down();

	DACOM->ShutDown();
	DACOM=0;
};

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	if(!setup(hInstance))
		return 0;//failed setup;

	runMessageCue();

	shutdown();

	return 0;
}



