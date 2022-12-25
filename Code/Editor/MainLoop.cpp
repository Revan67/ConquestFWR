//
// The main loop
//

#include "stdafx.h"
#include "globals.h"

#include "Camera.h"
#include "vfx.h"
#include "CQTrace.h"
#include "Mode.h"
#include "Editor.h"

#include <Startup.h>
#include <EventSys.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <System.h>
#include <IConnection.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// todo(aaj-4/21/2004): note, need to break these tasks into their different editor modes: Campaign, Senerio, Sector, and System

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// MainLoop Event System listener
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

struct MainLoopListener : public IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(MainLoopListener)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32    eventHandle;
	DOUBLE clockPeriod;
	DWORD  dwTickCount;

	MainLoopListener()
	{
		eventHandle = 0;
		clockPeriod = 0;
		dwTickCount = 0;
	}

	DEFMETHOD(Notify) (U32 message, void *param = 0)
	{
		if( message == CQE_WINDOW_RESIZE )
		{
			if( hMainWindow )
			{
				CRect rect;
				::GetClientRect( hMainWindow, &rect );

				PIPE->destroy_buffers();
				PIPE->create_buffers( hMainWindow, rect.Width(), rect.Height() );

				PIPE->set_window(hMainWindow, rect.TopLeft().x, rect.TopLeft().y, rect.Width(), rect.Height() );

				PANE p;
				p.window = NULL;
				p.x0 = rect.left;
				p.y0 = rect.top;
				p.x1 = rect.right - 1;
				p.y1 = rect.bottom - 1;
				CAMERA->SetPane( &p );

				// set up the visible area of the camera's pane
				PANE* pane = CAMERA->GetPane();
				if( pane )
				{
					int x = pane->x0;
					int y = pane->y0;
					int w = pane->x1 - x;
					int h = pane->y1 - y;
					PIPE->set_viewport(x,y,w,h);
				}
				else
					PIPE->set_viewport(0,0,SCREENRESX,SCREENRESY);

				SCREENRESX = p.x1 - p.x0;
				SCREENRESY = p.y1 - p.y0;
			}
		}

		return GR_OK;
	}

	//--------------------------------------------------------------------------
	//

	void init_frame_time (void)
	{
		__int64 clockFrequency;

 		if (QueryPerformanceFrequency((LARGE_INTEGER *) &clockFrequency) == 0)
		{
 			CQBOMB0("High performance clock not supported on this system.");
		}

 		clockPeriod = 1.0 / ((DOUBLE) clockFrequency);

		dwTickCount = ::GetTickCount();
	}

	//--------------------------------------------------------------------------
	//
	S32 get_frame_time (void)
	{
		#define MAX_FRAMEPERIOD 0.25		// 1/4 second
		static S32 currentFrameTime=0;

		__int64 clockTick;
		static __int64 lastClockTick;
		double elapsedTime;

		QueryPerformanceCounter((LARGE_INTEGER *)&clockTick);

		if (lastClockTick == 0)
		{
			lastClockTick = clockTick;
		}

		elapsedTime = DOUBLE(clockTick - lastClockTick) * clockPeriod;
		if (elapsedTime < 0 || elapsedTime > MAX_FRAMEPERIOD)
		{
			elapsedTime = MAX_FRAMEPERIOD;
		}
		lastClockTick = clockTick;
		
		// comment - the 1024x1000 converts it to micro-seconds, we use 1024 for part of it since most
		// controls shift the time down by 10 - capeche?
		currentFrameTime = S32(elapsedTime * 1024 * 1000);
		return currentFrameTime;
	}

	//--------------------------------------------------------------------------
	//
	void update()
	{
		// update using timer
		S32 dt = get_frame_time();

		// figure normal delta time
		Editor::deltaTime = ::GetTickCount() - dwTickCount;
		Editor::deltaTime /= 1000; // change to seconds
		dwTickCount = ::GetTickCount();

		// update components
		EVENTSYS->Send(CQE_UPDATE,(void *)dt);

		// update the current mode
		getMode()->Update();
	}

	//--------------------------------------------------------------------------
	//
	void draw()
	{
		getMode()->Draw();
	}

	//--------------------------------------------------------------------------
	//
	inline IMode* getMode()
	{
		switch( CQEDITORMODE )
		{
			case EM_CAMPAIGN:	return MODE_CAMPAIGN;
			case EM_SCENARIO:	return MODE_SCENARIO;
			case EM_SECTOR:		return MODE_SECTOR;
			case EM_SYSTEM:		return MODE_SYSTEM;
		}
		return MODE_SYSTEM;
	}
};

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// GlobalComponent
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

struct _mainlooplistener : GlobalComponent
{
	MainLoopListener * mainLoopListener;

	virtual void Startup (void)
	{
		mainLoopListener = new DAComponent<MainLoopListener>;
		AddToGlobalCleanupList((IDAComponent **) &mainLoopListener);
	}

	virtual void Initialize (void)
	{
		mainLoopListener->init_frame_time();

		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(mainLoopListener, &mainLoopListener->eventHandle);
		}
	}
};
static _mainlooplistener __mainlooplistener;

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
// MainLoop
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

void MainLoop()
{
	if( CQFLAGS.b3DEnabled )
	{
		__mainlooplistener.mainLoopListener->update();
		__mainlooplistener.mainLoopListener->draw();
	}
}
