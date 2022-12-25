//--------------------------------------------------------------------------//
//                                                                          //
//                             POSTOOL.CPP                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "DBHotKeys.h"
#include "BaseHotrect.h"
#include "resource.h"
#include "UserDefaults.h"
#include "Camera.h"
#include "Startup.h"

#include <heapobj.h>
#include <TComponent.h>
#include <EventSys.h>
#include <stdio.h>

struct PosTool : public BaseHotRect
{
	BOOL32 bActive;

	PosTool (IDAComponent * _parent);
	~PosTool();

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	// IEventCallback method */
	
	DEFMETHOD(Notify) (U32 message, void *param = 0);

	void PopUp();
	void Hide();

};

PosTool::PosTool (IDAComponent * _parent) : BaseHotRect(_parent)
{
	bActive = FALSE;
}

PosTool::~PosTool()
{}

GENRESULT 
PosTool::Notify(U32 message, void *param)
{
	MSG * msg = (MSG *)param;
	wchar_t buffer[64];

	switch (message)
	{
	case WM_MOUSEMOVE:
		if (bActive && ownsResources())
		{
			S32 mouseX,mouseY;
			mouseX = S16(LOWORD(msg->lParam));
			mouseY = S16(HIWORD(msg->lParam));

			if (DEFAULTS->GetDefaults()->bPosToolWorld)
			{
				SINGLE x, y;

				x = (SINGLE) mouseX;
				y = (SINGLE) mouseY;
				CAMERA->ScreenToPoint(x, y, 0);
				mouseX = (S32) x;
				mouseY = (S32) y;
			}

			swprintf(buffer,L"%i,%i",mouseX,mouseY);
			STATUS->SetTextString(buffer);
		}
		break;

	case WM_RBUTTONUP:
		Hide();
		break;
	
	case CQE_DEBUG_HOTKEY:
		switch ((U32)param)
		{
		case IDH_POSTOOL:
			PopUp();
			break;
		}
		break;
	}

	return GR_OK;
}

void PosTool::PopUp()
{
	desiredOwnedFlags = RF_CURSOR | RF_STATUS;
	grabAllResources();
	bActive = TRUE;
}

void PosTool::Hide()
{
	desiredOwnedFlags = 0;
	releaseResources();
	bActive = FALSE;
}

struct _postool : GlobalComponent
{
	PosTool *postool;

	virtual void Startup (void)
	{
		postool = new PosTool(0);		// delay initialization until later
		AddToGlobalCleanupList((IDAComponent **) &postool);
	}

	virtual void Initialize (void)
	{
		postool->lateInitialize(GS);		// initialize now
		postool->resPriority = RES_PRIORITY_HIGH+6;
		postool->toolTextID = 0;//IDS_HKEDIT;
		postool->cursorID = IDC_CURSOR_POSTOOL;

		memset(&postool->screenRect, -1, sizeof(postool->screenRect));
	}
};

static _postool startup;

//---------------------------------------------------------------------------
//--------------------------End PosTool.cpp----------------------------------
//---------------------------------------------------------------------------
