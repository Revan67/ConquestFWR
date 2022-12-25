//--------------------------------------------------------------------------//
//                                                                          //
//                             InProgressAnim.cpp                           //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/InProgressAnim.cpp 26    9/25/00 10:07p Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "InProgressAnim.h"
#include "IAnimate.h"
#include "IStatic.h"
#include "GenData.h"
#include "VideoSurface.h"
#include "DrawAgent.h"
#include "FileSys.h"
#include "BaseHotrect.h"

#include <DStatic.h>
#include <DAnimate.h>

#include <TSmartPointer.h>
#include <EventSys.h>
#include <TComponent.h>
#include <IDDBackDoor.h>

void __stdcall SetPipelineCriticalSection (CRITICAL_SECTION * criticalSection);
void __stdcall SetWindowManagerCriticalSection (CRITICAL_SECTION * criticalSection);
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#define BAR_X 115
#define BAR_Y 220
#define BAR_BASE_COLOR RGB(175, 80, 80)
#define BAR_HILIGHT_COLOR RGB(255, 0, 0)

#define ANIMATION_TYPE  "Animate!!Progress"

//--------------------------------------------------------------------------//
//
/*
inline long __fastcall F2LONG (SINGLE s)
{
	long result;
	__asm  fld	dword ptr[s]
	__asm  fistp dword ptr[result]
	return result;
}*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
struct IPAnim : IPANIM, BaseHotRect
{
	BEGIN_DACOM_MAP_INBOUND(IPAnim)
	DACOM_INTERFACE_ENTRY(IPANIM)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	//  data
	//

	bool bInitialized;
	volatile bool bKillRequested;
	bool bStopped;
	CRITICAL_SECTION criticalSection;
	HANDLE hThread;
	U32 threadID;
	__int64 lastUpdateTick, clockFrequency;


	COMPTR<IEventCallback> background, foreground;
	COMPTR<IFontDrawAgent> pString;
	COMPTR<IDrawAgent> shape;
	S32 stringX, stringY;
	U32 stringResID;
	U16 barWidth, barHeight;

	SINGLE progress;


    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	IPAnim (void) : BaseHotRect ((IDAComponent *)NULL)
	{
	}

	~IPAnim (void)
	{
		reset();
		CQFLAGS.bInProgressAnimActive = 0;
	}

	/* IPANIM methods */

	virtual void Start (void);

	virtual void Stop (void);

	virtual void SetProgress (SINGLE percent);		// 0 to 1

	virtual SINGLE GetProgress (void)
	{
		return progress;
	}
	
	virtual void UpdateString (U32 dwResID);

	/* IPAnim methods */

	void reset (void);		// free all resources

	void init (void);

	void draw_S (void);		// synchronized

	void update_S (void);		// synchronized

	void main (void);		// background thread routine

//	static void testBuffersLost (void);

	static DWORD WINAPI threadMain (LPVOID lpThreadParameter)
	{
		((IPAnim *)lpThreadParameter)->main();
		return 0;
	}

};
//-------------------------------------------------------------------------------
//
void IPAnim::Start (void)
{
	bStopped = false;
}
//-------------------------------------------------------------------------------
//
void IPAnim::Stop (void)
{
	bStopped = true;
}
//-------------------------------------------------------------------------------
//
/*
void IPAnim::SetBackground (IDAComponent * _background)
{
	EnterCriticalSection(&criticalSection);

	{
		if (_background==0)
			background = 0;
		else
			_background->QueryInterface("IEventCallback", background);
	}

	LeaveCriticalSection(&criticalSection);
}
//-------------------------------------------------------------------------------
//
void IPAnim::SetForeground (IDAComponent * _foreground)
{
	EnterCriticalSection(&criticalSection);

	{
		if (_foreground==0)
			foreground = 0;
		else
			_foreground->QueryInterface("IEventCallback", foreground);
	}

	LeaveCriticalSection(&criticalSection);
}
*/
//-------------------------------------------------------------------------------
//
void IPAnim::SetProgress (SINGLE percent)
{
	progress = percent;
}
//-------------------------------------------------------------------------------
//
/*
void IPAnim::SetString (IFontDrawAgent * pAgent, S32 x, S32 y, U32 dwResID)
{
	EnterCriticalSection(&criticalSection);
	
	pString = pAgent;
	stringX = IDEAL2REALX(x);
	stringY = IDEAL2REALY(y);
	stringResID = dwResID;

	draw_S();

	LeaveCriticalSection(&criticalSection);
}
*/
//-------------------------------------------------------------------------------
//
void IPAnim::UpdateString (U32 dwResID)
{
	EnterCriticalSection(&criticalSection);
	
	stringResID = dwResID;

	draw_S();

	LeaveCriticalSection(&criticalSection);
}
//-------------------------------------------------------------------------------
//
void IPAnim::reset (void)
{
	if (bInitialized)
	{
		bKillRequested = true;
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread = 0;
		
		SetPipelineCriticalSection(NULL);
		SetWindowManagerCriticalSection(NULL);

		DeleteCriticalSection(&criticalSection);
		bInitialized = false;
	}
}
//-------------------------------------------------------------------------------
//
void IPAnim::init (void)
{
	// initialize all of our controls before we create the damn thread
	COMPTR<IAnimate> animate;
	COMPTR<IStatic> back;
	COMPTR<IDAComponent> pComp;

	CQASSERT(CQFLAGS.bInProgressAnimActive==0);
	CQFLAGS.bInProgressAnimActive = 1;

	// add the background image
	if (GENDATA->CreateInstance("Static!!Progress", pComp) == GR_OK)
	{
		pComp->QueryInterface("IStatic", back);

		if (back)
		{

			STATIC_DATA sdata;
			memset(&sdata, 0, sizeof(sdata));
			strncpy(sdata.staticType, "Static!!Progress", sizeof(sdata.staticType));
			sdata.xOrigin = 100;
			sdata.yOrigin = 90;
			sdata.staticText = STTXT::LOAD_MISSION_DATA;
			sdata.alignment = STATIC_DATA::TENBYSIX;

			back->InitStatic(sdata, this);
			back->QueryInterface("IEventCallback", background);
		}
	}

	// add the animation
	if (GENDATA->CreateInstance(ANIMATION_TYPE, pComp) == GR_OK)
	{
		pComp->QueryInterface("IAnimate", animate);

		if (animate)
		{
			ANIMATE_DATA adata;
			strncpy(adata.animateType, ANIMATION_TYPE, sizeof(adata.animateType));
			adata.dwTimer = 0;
			adata.xOrigin = 114;
			adata.yOrigin = 120;
			adata.bFuzzEffect = false;

			animate->InitAnimate(adata, this, NULL, 0);
			animate->QueryInterface("IEventCallback", foreground);
		}
	}

	// create a font draw agent to pass in for our fonts
	HFONT hFont;
	COLORREF penFront, penBack;
	penFront = RGB_NTEXT | 0xFF000000;		
	penBack	 = RGB(0,0,0);	   

	hFont = CQCreateFont(IDS_TOOLBAR_MONEY_FONT);
	CreateFontDrawAgent(hFont, 1, penFront, penBack, pString);

	stringX = IDEAL2REALX(270);
	stringY = IDEAL2REALY(190);

	CreateDrawAgent("progress.shp", INTERFACEDIR, DA::UNKTYPE, 0, shape);
	CQASSERT(shape);
	shape->GetDimensions(barWidth, barHeight);

	// draw once to make sure everyone has init'ed themselves properly

	draw_S();

	// now do the multi-threaded stuff
	QueryPerformanceFrequency((LARGE_INTEGER *) &clockFrequency);
	InitializeCriticalSection(&criticalSection);
	SetPipelineCriticalSection(&criticalSection);
	SetWindowManagerCriticalSection(&criticalSection);
	hThread = CreateThread(0,4096, (LPTHREAD_START_ROUTINE) threadMain, (LPVOID)this, 0, &threadID);
	CQASSERT(hThread);


	bInitialized = true;
}
#if 0
//-------------------------------------------------------------------------------
//
void IPAnim::testBuffersLost (void)
{
	COMPTR<IDDBackDoor> pBackDoor;
	COMPTR<IUnknown> pDD1;
	COMPTR<IDirectDraw7> pDD7;
	COMPTR<IDirectDrawSurface7> pDDSurface;
	HRESULT hr;

	hr = PIPE->QueryInterface(IID_IDDBackDoor, pBackDoor);
	if (hr != GR_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_DIRECTDRAW, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDraw7, pDD7);    
	if (hr != DD_OK)
		goto Done;
	hr = pBackDoor->get_dd_provider(DDBD_P_PRIMARYSURFACE, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDrawSurface7, pDDSurface);
	if (hr != GR_OK)
		goto Done;
	if (pDDSurface->IsLost() != DD_OK)
		CQTRACE10("Primary surface is lost!");

	hr = pBackDoor->get_dd_provider(DDBD_P_BACKSURFACE, pDD1);
	if (hr != GR_OK)
		goto Done;
	hr = pDD1->QueryInterface(IID_IDirectDrawSurface7, pDDSurface);
	if (hr != GR_OK)
		goto Done;
	if (pDDSurface->IsLost() != DD_OK)
		CQTRACE10("Back surface is lost!");

Done:
	return;
}
#endif

extern bool bInScene;
//-------------------------------------------------------------------------------
//
void IPAnim::draw_S (void)
{
	CQASSERT(bInScene == false);

	const bool bUseLocking = (CQFLAGS.bFrameLockEnabled!=0);
	const bool bUsing3D = (bUseLocking==false && CQFLAGS.b3DEnabled!=0);

	GENRESULT gr = PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
	if (gr != GR_OK)
		CQTRACE10("Failed to clear buffers");
	
	if ((bUsing3D==0) || (PIPE->begin_scene()==GR_OK))
	{
		if (bUsing3D || SURFACE->Lock())
		{
			// give the background and foreground animations a chance to draw themselves
			if (background!=0)
			{
				background->Notify(CQE_ENDFRAME, 0);
			}

			if (foreground!=0)
			{
				foreground->Notify(CQE_ENDFRAME, 0);
			}

			// draw the progress bar
			if (progress > 0)
			{
				S32 progressWidth = (barWidth);
				
				if (progress <= 1.0)
				{
					progressWidth *= progress;
				}

				PANE pane;
				pane.window = NULL;
				pane.x0 = IDEAL2REALX(BAR_X);
				pane.y0 = IDEAL2REALY(BAR_Y);
				pane.x1 = pane.x0 + progressWidth;
				pane.y1 = pane.y0 + barHeight+4;
				shape->Draw(&pane, 0, 0); 
			}
			
			if (pString != 0 && stringResID)
			{
				pString->StringDraw(NULL, stringX, stringY, stringResID);
			}

			// end the scene, flush everything
			if (bUsing3D)
			{
				PB.Color3ub(0,0,0);
				PB.Begin(PB_TRIANGLES);
				PB.Vertex3f(0,0,0);
				PB.Vertex3f(0,0,0);
				PB.Vertex3f(0,0,0);
				PB.End();
				BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
				BATCH->flush(RPR_TRANSLUCENT_UNSORTED_ONLY);
				if (PIPE->end_scene() != GR_OK)
					CQTRACE10("Error in draw_S end_scene - probably lost surfaces");
			}
			else
			{
				SURFACE->Unlock();
			}
		}
	}
	PIPE->swap_buffers();
}
//-------------------------------------------------------------------------------
//
void IPAnim::update_S (void)
{
	__int64 diff, clockTick;
	
	QueryPerformanceCounter((LARGE_INTEGER *)&clockTick);
	if (lastUpdateTick==0)
		lastUpdateTick = clockTick;
	diff = clockTick - lastUpdateTick;

	lastUpdateTick = clockTick;

	if (diff > clockFrequency / 4)
		diff = clockFrequency / 4;

	U32 time = (diff * 1024 * 1000) / clockFrequency;

	if (time)
	{
		if (background!=0)
		{
			background->Notify(CQE_UPDATE, (void *) time);
		}
		if (foreground!=0)
		{
			foreground->Notify(CQE_UPDATE, (void *) time);
		}
	}
}
//-------------------------------------------------------------------------------
//
void IPAnim::main (void)
{
	while (bKillRequested==false)
	{
		EnterCriticalSection(&criticalSection);
		if (bStopped == false)
			update_S();
		draw_S();
		LeaveCriticalSection(&criticalSection);

		Sleep(1000 / 15);		// wait for about 1/15 of a second
	}
}

//-------------------------------------------------------------------------------
//
void __stdcall CreateInProgressAnim (IPANIM ** ppAnim)
{
	IPAnim * result = new DAComponent<IPAnim>;
	result->init();
	*ppAnim = result;
}
//-------------------------------------------------------------------------------
//-----------------------END InProgressAnim.cpp----------------------------------
//-------------------------------------------------------------------------------
