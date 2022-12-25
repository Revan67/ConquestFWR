//--------------------------------------------------------------------------//
//                                                                          //
//                               Hintbox.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Hintbox.cpp 13    10/18/01 4:32p Tmauer $

		                 Resource manager for the hint system
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "resource.h"
#include "TResource.h"
#include "Hintbox.h"
#include "BaseHotRect.h"
#include "EventPriority.h"
#include "DrawAgent.h"
#include "Startup.h"
#include "UserDefaults.h"
#include "VideoSurface.h"

#include <TComponent.h>
#include <IConnection.h>
#include <TConnContainer.h>
#include <TConnPoint.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>


#include <commctrl.h>

#define CHARCOUNT(x) (sizeof(x)/sizeof(x[0]))

#define HINTBOX_HEIGHT 150
#define HINTBOX_WIDTH 273
#define HINTBOX_GAP 8

#define HB_LEADING_PAUSE  600				// milliseconds
#define HB_TRAILING_PAUSE 200		

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HintResource : public Resource<HintResource,IHintResource>, 
								  ConnectionPointContainer<HintResource>,
								  IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(HintResource)
	DACOM_INTERFACE_ENTRY(IResource)
	DACOM_INTERFACE_ENTRY(IHintResource)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	U32 handle;		// connection handle
	U32 dwHeight;		// height of the control, in pixels
	U32 dwTextID;
	U32 leadTime, trailTime;		// in milliseconds
	wchar_t szText[256];
	BOOL32 bInMenuLoop;
	U32 toolbarHeight;
	HFONT hFont;

#define MAX_HISTORY 2
	struct HistoryNode
	{
		COMPTR<IFontDrawAgent> font;
		U32 dwTextID;
	} historyNode[MAX_HISTORY];

	U32 lastHistoryNode;		// index of one used the last time

	HintResource (void);

	~HintResource (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IHintResource methods */

	virtual void __stdcall SetText (U32 dwResourceID);

	virtual void __stdcall SetTextString (const wchar_t *string);

	virtual U32 __stdcall GetText (void);

	virtual U32 __stdcall GetHeight (void);

	virtual void __stdcall SetToolbarHeight (U32 height);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* HintResource methods */

	void reloadFonts (bool bLoad);

	void unDraw (void);

	void OnNoOwner (void)
	{
		SetText(0);		// empty text for now
	}

	U32 findHistoryNode (U32 textID);

	void redraw (void);

	void onUpdate (U32 dt)			// dt is milliseconds
	{
		if (szText[0] || dwTextID)
		{
			if (leadTime < HB_LEADING_PAUSE)
				leadTime += dt;
			trailTime = 0;
		}
		else
		{
			if (trailTime < HB_TRAILING_PAUSE)
				trailTime += dt;
			else
				leadTime = 0;
		}

	}



	IDAComponent * getBase (void)
	{
		return (IHintResource *) this;
	}
};
//--------------------------------------------------------------------------//
//
HintResource::HintResource (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
HintResource::~HintResource (void)
{
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}

	if (hFont)
	{
		DeleteObject(hFont);
		hFont = 0;
	}
}
//--------------------------------------------------------------------------//
//
void HintResource::redraw (void)
{
	//
	// NOTE:
	// might not want to lock the surface during the menu loop because of WIN16 trouble,
	// especially now that we are use GDI to draw the fonts
	//
	
//	if (glLockFrameBufferEXT && bInMenuLoop)
//		return;

	if (CQFLAGS.bGameActive && DEFAULTS->GetDefaults()->bNoHints==0)
	{
		bool bFrameLocked=(CQFLAGS.bFrameLockEnabled!=0);
		IFontDrawAgent * font;

		if (bFrameLocked)
			if (SURFACE->Lock() == false)
				return;
		
		PANE pane;
		pane.window = 0;
		//pane.x0 = SCREENRESX-IDEAL2REALX(HINTBOX_WIDTH+HINTBOX_GAP);
		//pane.x1 = SCREENRESX-IDEAL2REALX(HINTBOX_GAP);
		pane.x0 = IDEAL2REALX(HINTBOX_GAP);
		pane.x1 = IDEAL2REALX(HINTBOX_WIDTH+HINTBOX_GAP);
		pane.y0 = SCREENRESY-toolbarHeight-IDEAL2REALY(HINTBOX_HEIGHT)-1;
		pane.y1 = SCREENRESY-toolbarHeight-1;

		lastHistoryNode = findHistoryNode(dwTextID);
		font = historyNode[lastHistoryNode].font;

		if (dwTextID && leadTime >= HB_LEADING_PAUSE)
		{
			DA::RectangleHash(0, pane.x0, pane.y0, pane.x1, pane.y1, RGB(0, 0, 10));
			font->StringDraw(&pane, 0,0, dwTextID);
		}
		else if (szText && *szText)
		{
			DA::RectangleHash(0, pane.x0, pane.y0, pane.x1, pane.y1, RGB(0, 0, 10));
			font->StringDraw(&pane, 0,0, szText);
		}

		historyNode[lastHistoryNode].dwTextID = dwTextID;

		if (bFrameLocked)
			SURFACE->Unlock();
	}
}
//--------------------------------------------------------------------------//
//
U32 HintResource::findHistoryNode (U32 textID)
{
	int i, blank=-1;

	for (i = 0; i < MAX_HISTORY; i++)
	{
		if (textID == historyNode[i].dwTextID)
			return i;
		if (historyNode[i].dwTextID == 0)
			blank = i;
	}

	if (historyNode[lastHistoryNode].dwTextID == 0)
		return lastHistoryNode;
	if (blank == -1)
		return (lastHistoryNode + 1) % MAX_HISTORY;

	return blank;
}
//--------------------------------------------------------------------------//
// undraw the previous text (this operation is slow
void HintResource::unDraw (void)
{
	if (CQFLAGS.bGameActive && DEFAULTS->GetDefaults()->bNoHints==0)
	{
		bool bFrameLocked=(CQFLAGS.bFrameLockEnabled!=0);

		if (bFrameLocked)
			if (SURFACE->Lock() == false)
				return;

		{
			int x0 = SCREENRESX-IDEAL2REALX(HINTBOX_WIDTH+HINTBOX_GAP);
			int x1 = SCREENRESX-IDEAL2REALX(HINTBOX_GAP);
			int y0 = SCREENRESY-toolbarHeight-IDEAL2REALY(HINTBOX_HEIGHT)-1;
			int y1 = SCREENRESY-toolbarHeight-1;


			DA::RectangleFill(0, x0, y0, x1, y1, 0);
		}
		
		if (bFrameLocked)
			SURFACE->Unlock();
	}
}
//--------------------------------------------------------------------------//
//
U32 HintResource::GetText (void)
{
	return dwTextID;
}
//--------------------------------------------------------------------------//
//
void HintResource::SetText (U32 dwResourceID)
{
	dwTextID = dwResourceID;
	szText[0] = 0;

	if (leadTime < HB_LEADING_PAUSE)
	{
		leadTime = 0;
	}

	if (bInMenuLoop && CQFLAGS.bFrameLockEnabled)
	{
		unDraw();
		redraw();
		InvalidateRect(hMainWindow, 0, 0);
	}
}
//--------------------------------------------------------------------------//
//
void HintResource::SetTextString (const wchar_t *string)
{
	wcsncpy(szText, string, CHARCOUNT(szText)-1);
	dwTextID = 0;

	if (leadTime < HB_LEADING_PAUSE)
	{
		leadTime = 0;
	}
}
//--------------------------------------------------------------------------//
//
U32 HintResource::GetHeight (void)
{
	return dwHeight;
}
//--------------------------------------------------------------------------//
//
void HintResource::SetToolbarHeight (U32 height)
{
	toolbarHeight = height;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT HintResource::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_START3DMODE:
		reloadFonts(true);
		break;
	case CQE_END3DMODE:
		reloadFonts(false);
		break;
	case WM_ENTERMENULOOP:
		bInMenuLoop=1;
		break;
	case WM_EXITMENULOOP:
		bInMenuLoop=0;
		break;
	case CQE_ENDFRAME:
		redraw();
		break;

	case CQE_UPDATE:
		updateResource();
		onUpdate(S32(param) >> 10);
		break;

	case WM_MOUSEMOVE:
		// if we are not in trailing mode, than set the lead timer to zero
		if (leadTime < HB_LEADING_PAUSE)
		{
			leadTime = 0;
		}
		break;

#if 0
	case WM_CHAR:
		if ((TCHAR) msg->wParam == 'a')
		{
			char *ptr = 0;

			if (*ptr == 0)
				*ptr = 1;
		}
		else
		if ((TCHAR) msg->wParam == 'b')
		{
			int i = 0;

			if ((50 / i) == 0)		// divide by zero
				return GR_OK;
		}
		else
		if ((TCHAR) msg->wParam == 'c')
		{
			InvalidateRect(0, 0, 1);
		}
		break;
#endif
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void HintResource::reloadFonts (bool bLoad)
{
	int i;

	if (bLoad)
	{
		COLORREF pen, background;

		pen			= RGB(200,200,200)		   | 0xFF000000;		// white	(pen color)
		background	= RGB(0,0,0);	   //| 0xFF000000;		// black	(background color)

		CQASSERT(hFont==0);
		hFont = CQCreateFont(IDS_HINTBOX_FONT);

		CreateMultilineFontDrawAgent(0,hFont, pen, background, historyNode[0].font);
		for (i = 1; i < MAX_HISTORY; i++)
			historyNode[0].font->CreateDuplicate(historyNode[i].font);
	}
	else
	{
		for (i = 0; i < MAX_HISTORY; i++)
			historyNode[i].font.free();

		if (hFont)
		{
			DeleteObject(hFont);
			hFont = 0;
		}
	}
}
//--------------------------------------------------------------------------//
//
struct _hintbox : GlobalComponent
{
	HintResource * hint;
	
	virtual void Startup (void)
	{
		HINTBOX = hint = new DAComponent<HintResource>;
		AddToGlobalCleanupList((IDAComponent **) &HINTBOX);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(HINTBOX, &hint->handle);
			FULLSCREEN->SetCallbackPriority(hint, EVENT_PRIORITY_STATUS);
		}

		hint->OnNoOwner();
	}
};

static _hintbox startup;

//--------------------------------------------------------------------------//
//-----------------------------End Hintbox.cpp----------------------------//
//--------------------------------------------------------------------------//
