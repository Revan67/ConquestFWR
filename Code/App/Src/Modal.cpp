//--------------------------------------------------------------------------//
//                                                                          //
//                                Modal.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Modal.cpp 71    10/20/00 11:24p Jasony $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "ObjList.h"
#include "NetBuffer.h"
#include "DrawAgent.h"
#include "CQTrace.h"
#include "VideoSurface.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "DEffectOpts.h"
#include "Hotkeys.h"
#include "SoundManager.h"
#include "IBackground.h"
#include "Netpacket.h"
#include "Sector.h"

#include <HeapObj.h>
#include <System.h>
#include <EventSys.h>
#include <HKEvent.h>
#include <WindowManager.h>
#include <RendPipeline.h>
#include <IRenderPrimitive.h>
#include <IConnection.h>
#include <TComponent.h>
#include <TSmartPointer.h>


#include <mmsystem.h>

typedef IHeap * (__stdcall * GETBATCHHEAP) (void);
static GETBATCHHEAP GetBatchHeap;
//__declspec(dllimport) IHeap * __stdcall GetBatchHeap (void);
//--------------------------------------------------------------------------
//
static DOUBLE clockPeriod;
static void init_frame_time (void)
{
	__int64 clockFrequency;

 	if (QueryPerformanceFrequency((LARGE_INTEGER *) &clockFrequency) == 0)
 		CQBOMB0("High performance clock not supported on this system.");

 	clockPeriod = 1.0 / ((DOUBLE) clockFrequency);
}
//--------------------------------------------------------------------------
//
static S32 currentFrameTime=0;

#define MAX_FRAMEPERIOD 0.25		// 1/4 second
static S32 get_frame_time (void)
{
	__int64 clockTick;
	static __int64 lastClockTick;
	double elapsedTime;

	QueryPerformanceCounter((LARGE_INTEGER *)&clockTick);

	if (lastClockTick == 0)
		lastClockTick = clockTick;

	elapsedTime = DOUBLE(clockTick - lastClockTick) * clockPeriod;
	if (elapsedTime < 0 || elapsedTime > MAX_FRAMEPERIOD)
		elapsedTime = MAX_FRAMEPERIOD;
	lastClockTick = clockTick;
	
	// comment - the 1024x1000 converts it to micro-seconds, we use 1024 for part of it since most
	// controls shift the time down by 10 - capeche?
	currentFrameTime = S32(elapsedTime * 1024*1000);
	return currentFrameTime;
}

static RPDEVICESTATS stats;
//--------------------------------------------------------------------------
//
static void slam2Dcomponents (S32 frame_time, bool bUseLocking)
{
	S32 fps;
#ifndef FINAL_RELEASE	
	C8  buffer[256];
#endif

	if (frame_time)
		fps = ((10000000 / frame_time) + 5) / 10;
	else
		fps = RENDER_FRAMERATE;
	if (fps + 1 == RENDER_FRAMERATE || fps - 1 == RENDER_FRAMERATE)
		fps = RENDER_FRAMERATE;
	
	if (bUseLocking)
	{
		if (SURFACE->Lock() == false)
			return;
	}
	
	if (CQFLAGS.bFullScreenMap==0)
		OBJLIST->DrawHighlightedList();
	
	BATCH->set_state(RPR_BATCH,false);
	BATCH->flush(RPR_OPAQUE | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
	
	EVENTSYS->Send(CQE_ENDFRAME);
	
#ifndef FINAL_RELEASE	
	if (CheckHotkeyPressed (IDH_DEBUG_PRINT))
	{
		CQFLAGS.debugPrint++;
	}
	
	if (CQFLAGS.debugPrint!=DBG_NONE)
	{
		if (CQFLAGS.debugPrint == DBG_ALL)
		{	
			IHeap *batchHeap = (*GetBatchHeap)();
			U32 batchMem = batchHeap->GetHeapSize()-batchHeap->GetAvailableMemory();
			
			wsprintf(buffer,"FPS: %2d, TEX: %d, VB: %d, SND: %d, HEAP: %d, BATCH: %d, POLYS: %3d%s", 
				fps, TEXMEMORYUSED, VBMEMORYUSED, SNDMEMORYUSED, HEAP->GetLargestBlock(), batchMem, stats.num_dp_primitives+stats.num_dip_primitives, stats.is_thrashing ? " TEXTURE THRASHING" : "");
			
			DEBUGFONT->StringDraw(0, 8, 14, buffer, RGB(200,200,200));
			
		}
		else
		{
			wsprintf(buffer,"FPS: %2d, HEAP: %d",
			fps, HEAP->GetLargestBlock());

			DEBUGFONT->StringDraw(0, 8, 14, buffer, RGB(200,200,200));
		}

#ifdef _ROB
		wsprintf(buffer,"DP: %3d DIP: %3d  TEXTURES_IN_SCENE: %3d",stats.num_dp_calls,stats.num_dip_calls,stats.num_texture_activated);
			
		DEBUGFONT->StringDraw(0, 8, 1, buffer, RGB(200,200,200));
#endif // end _ROB

		if (CQFLAGS.bGameActive)
			OBJLIST->DEBUG_print();	// print debugging info, if any
		else
			NETPACKET->DEBUG_print(DEBUGFONT);
	}
#endif  // end !FINAL_RELEASE

	if (bUseLocking)
		SURFACE->Unlock();
}
//--------------------------------------------------------------------------
//
#ifdef _DEBUG
static void readJoystick (void)
{
	JOYINFOEX joyinfo;
	
	joyinfo.dwSize = sizeof(joyinfo);
	joyinfo.dwFlags =  JOY_RETURNBUTTONS | JOY_RETURNPOV | JOY_RETURNX | JOY_RETURNY;

	if (joyGetPosEx(JOYSTICKID1, &joyinfo) == JOYERR_NOERROR)
	{
		if (joyinfo.dwButtons)
		{
			joyinfo.dwButtons++;
			joyinfo.dwButtons--;
		}

		DBHOTKEY->SystemMessage((S32)hMainWindow, CQE_JOYSTICK, 0, (LONG)&joyinfo);
	}
}
#endif
//--------------------------------------------------------------------------
//
struct DACOM_NO_VTABLE ModalEventCallback : public IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(ModalEventCallback)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 eventHandle;
	IDAConnectionPoint * parentConnection;
	bool bResultSet;
	U32 dwResult;

	static ModalEventCallback * root;
	ModalEventCallback * next;

	ModalEventCallback (void)
	{
		eventHandle = 0;
		parentConnection = 0;
		bResultSet = false;
		next = root;
		root = this;
	}

	~ModalEventCallback (void)
	{
		unhook();
		root = next;
	}
	
	DEFMETHOD(Notify) (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_DLG_RESULT:
			if (root == this)
			{
				bResultSet = true;
				dwResult = U32(param);
				return GR_GENERIC;
			}
		}

		return GR_OK;
	}

	/* ModalEventCallback methods */

	void unhook (void)
	{
		if (parentConnection && eventHandle)
			parentConnection->Unadvise(eventHandle);
		eventHandle = 0;
	}

	void init (IDAConnectionPoint * _parent)
	{
		parentConnection = _parent;
		parentConnection->Advise(this, &eventHandle);
	}

};
static bool bResultWasReturned;
ModalEventCallback * ModalEventCallback::root;

bool bInScene=0;

BOOL32 EndModal(SINGLE dt)
{
	bool bNoSwap = false;		// true if viewing a full-screen movie
	__int64 time1,time2;
	bool bUseLocking = (CQFLAGS.bFrameLockEnabled!=0);
	QueryPerformanceCounter((LARGE_INTEGER *)&time1);
	
	if (bInScene)
	{
		CQASSERT(CQFLAGS.bInProgressAnimActive==0);   // should not be drawing when progress anim is running!
		
		BATCH->flush(RPR_OPAQUE | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
		QueryPerformanceCounter((LARGE_INTEGER *)&time2);
		
		OBJLIST->AddTimerTicks(TIMING_FLUSH,time2-time1);
		time1=time2;
		
		BATCH->set_state(RPR_BATCH,FALSE);
		
		if (bUseLocking)
		{
			if (CQFLAGS.b3DEnabled)
			{
				//	PrimitiveBuilder PB(PIPE);
				// glEnable(GL_BLEND);
				DisableTextures();
				PB.Color3ub(0,0,0);
				PB.Begin(PB_TRIANGLES);
				PB.Vertex3f(0,0,0);
				PB.Vertex3f(0,0,0);
				PB.Vertex3f(0,0,0);
				PB.End();
				//	BATCH->flush(RPR_OPAQUE | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
				//			BATCH->flush(RPR_OPAQUE);
				//	BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
				//	BATCH->flush(RPR_TRANSLUCENT_UNSORTED_ONLY);
				if (PIPE->end_scene() != GR_OK)
					CQTRACE10("Error in end_scene - probably lost surfaces");
			}
			bInScene = 0;
		}
		
		// when the game is active, draw movies before 2D components
		// when the game is not active (ie. front end menus) then movies are always on top.
		if (CQFLAGS.bGameActive)
		{
			SOUNDMANAGER->BltMovies();
			slam2Dcomponents(dt, bUseLocking);		// lock frame buffer, write 2D stuff, unlock
		}
		else 
/*			if (CQFLAGS.bFullScreenMovie)
			{
				SOUNDMANAGER->BltMovies();
				bNoSwap = true;		// viewing a full-screen movie directly to the front buffer
			}
			else
*/			{
				// okay, we're in the front end, and we are not drawing a movie, so render the 2D stuff
				slam2Dcomponents(dt, bUseLocking);		// lock frame buffer, write 2D stuff, unlock
				SOUNDMANAGER->BltMovies();
			}
			
			if (bUseLocking==0)
			{
				if (CQFLAGS.b3DEnabled)
				{
					//	PrimitiveBuilder PB(PIPE);
					// glEnable(GL_BLEND);
					PB.Color3ub(0,0,0);
					PB.Begin(PB_TRIANGLES);
					PB.Vertex3f(0,0,0);
					PB.Vertex3f(0,0,0);
					PB.Vertex3f(0,0,0);
					PB.End();
					//BATCH->flush(RPR_OPAQUE | RPR_TRANSLUCENT_UNSORTED_ONLY | RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
					//			BATCH->flush(RPR_OPAQUE);
					BATCH->flush(RPR_TRANSLUCENT_DEPTH_SORTED_ONLY);
					BATCH->flush(RPR_TRANSLUCENT_UNSORTED_ONLY);
					if (PIPE->end_scene() != GR_OK)
						CQTRACE10("Error in end_scene - probably lost surfaces");
				}
				bInScene = 0;
			}
			
			QueryPerformanceCounter((LARGE_INTEGER *)&time2);
			
			OBJLIST->AddTimerTicks(TIMING_RENDER2D,time2-time1);
			
			time1=time2;
	}

	memset(&stats, 0, sizeof(stats));
	PIPE->get_device_stats(&stats);

	if (bNoSwap==false)
	{
		//PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);
		PIPE->swap_buffers();
		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL);
	}
	
	QueryPerformanceCounter((LARGE_INTEGER *)&time2);

	OBJLIST->AddTimerTicks(TIMING_SWAPBUFFERS,time2-time1);

	if (CQFLAGS.bGameActive)
		OBJLIST->Update();

	QueryPerformanceCounter((LARGE_INTEGER *)&time1);
	((ISystemContainer *)GS)->Update();			// service windows message queue, etc.
	QueryPerformanceCounter((LARGE_INTEGER *)&time2);
	OBJLIST->AddTimerTicks(TIMING_GAMESYSTEM,time2-time1);

	OBJLIST->EndFrame();	// frame timing stuff, do delay if needed

	return TRUE;
}
//--------------------------------------------------------------------------
//
U32 __stdcall CQDoModal (BaseHotRect * pDialog)
{
	CQASSERT(bInScene == 0);
	__int64 time1,time2;

	DAComponent<ModalEventCallback> modalEvent;

	if (pDialog)
	{
		CQASSERT(pDialog->parentEventConnection!=0);
		modalEvent.init(pDialog->parentEventConnection);
	}
	else
	{
		modalEvent.init(FULLSCREEN->parentEventConnection);
	}

	while (modalEvent.bResultSet==false)
	{
		S32 dt = get_frame_time();

		QueryPerformanceCounter((LARGE_INTEGER *)&time1);

		SECTOR->PrepareTexture();

		EVENTSYS->Send(CQE_UPDATE,(void *)dt);	// update components
		if (modalEvent.bResultSet)
			break;		// break early ( for end of game, don't update objlist last time )
		QueryPerformanceCounter((LARGE_INTEGER *)&time2);
		OBJLIST->AddTimerTicks(TIMING_UPDATE2D,time2-time1);

		OBJLIST->BeginFrame();	// frame timing stuff
		BATCH->set_viewport(0,0,SCREENRESX,SCREENRESY);

		if (CQFLAGS.bGameActive)
		{
			QueryPerformanceCounter((LARGE_INTEGER *)&time1);
			PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
			QueryPerformanceCounter((LARGE_INTEGER *)&time2);
			OBJLIST->AddTimerTicks(TIMING_CLEARBUFFERS,time2-time1);
		}
		
		bInScene = 1;
		if (CQFLAGS.b3DEnabled)
		{
			bInScene = 0;
			if(PIPE->begin_scene() == GR_OK)
				bInScene = 1;
		}
		
		if (bInScene)
		{
			if (CQFLAGS.bGameActive) 
			{
				QueryPerformanceCounter((LARGE_INTEGER *)&time1);
				if (CQFLAGS.bFullScreenMap==0)
					BACKGROUND->RenderNeb();
				BATCH->set_state(RPR_BATCH,TRUE);
				QueryPerformanceCounter((LARGE_INTEGER *)&time2);
				OBJLIST->AddTimerTicks(TIMING_RENDERBACKGROUND, time2-time1);
				if (CQFLAGS.bFullScreenMap==0)
					OBJLIST->Render();
			}
#ifdef _DEBUG
			readJoystick();
#endif
			
			QueryPerformanceCounter((LARGE_INTEGER *)&time1);
			EVENTSYS->Send(CQE_RENDER_LAST3D,(void *)dt);
			QueryPerformanceCounter((LARGE_INTEGER *)&time2);
			OBJLIST->AddTimerTicks(TIMING_RENDER2D, time2-time1);
		}
		EndModal(currentFrameTime);

		QueryPerformanceCounter((LARGE_INTEGER *)&time1);
		EVENTSYS->Send(CQE_FLUSHMESSAGES,(void *)dt);
		QueryPerformanceCounter((LARGE_INTEGER *)&time2);
		OBJLIST->AddTimerTicks(TIMING_UPDATE2D, time2-time1);
	}

	bResultWasReturned = true;

	return modalEvent.dwResult;
}
//--------------------------------------------------------------------------
//
U32 __stdcall CQNonRenderingModal (BaseHotRect * pDialog)
{
	CQASSERT(bInScene == 0);

	DAComponent<ModalEventCallback> modalEvent;

	if (pDialog)
	{
		CQASSERT(pDialog->parentEventConnection!=0);
		modalEvent.init(pDialog->parentEventConnection);
	}
	else
	{
		modalEvent.init(FULLSCREEN->parentEventConnection);
	}

	while (modalEvent.bResultSet==false)
	{
		S32 dt = get_frame_time();

		EVENTSYS->Send(CQE_UPDATE,(void *)dt);	// update components
		if (modalEvent.bResultSet)
			break;		// break early ( for end of game, don't update objlist last time )
		((ISystemContainer *)GS)->Update();			// service windows message queue, etc.
		EVENTSYS->Send(CQE_FLUSHMESSAGES,(void *)dt);
	}

	bResultWasReturned = true;

	return modalEvent.dwResult;
}
//-----------------------------------------------------------------------------------------//
//
struct _modalfactory : GlobalComponent
{
	virtual void Startup (void)
	{
		HINSTANCE hInstance = GetModuleHandle("ZBatcher.dll");
		
		if (hInstance)
			GetBatchHeap = (GETBATCHHEAP) GetProcAddress(hInstance, "_GetBatchHeap@0");
	}

	virtual void Initialize (void)
	{
		init_frame_time();
	}
};

static _modalfactory startup;
//----------------------------------------------------------------------------------
//------------------------END Modal.cpp---------------------------------------------
//----------------------------------------------------------------------------------
