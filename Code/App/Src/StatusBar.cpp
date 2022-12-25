//--------------------------------------------------------------------------//
//                                                                          //
//                              StatusBar.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

		                 Resource manager for the status bar

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "resource.h"
#include "TResource.h"
#include "StatusBar.h"
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

// #define USE_WINDOWS_CTRL 
// #define STATUS_Y 358
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE StatusBarResource : public Resource<StatusBarResource,IStatusBarResource>, 
								  ConnectionPointContainer<StatusBarResource>,
								  IEventCallback
{
#ifdef USE_WINDOWS_CTRL
	HWND hStatusBar;	
#endif

	BEGIN_DACOM_MAP_INBOUND(StatusBarResource)
	DACOM_INTERFACE_ENTRY(IResource)
	DACOM_INTERFACE_ENTRY(IStatusBarResource)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 handle;		// connection handle
	U32 dwHeight;		// height of the control, in pixels
	STATUSTEXTMODE mode;
#ifndef USE_WINDOWS_CTRL
	U32 dwTextID;
	wchar_t szText[256];
	wchar_t szName[256];
	BOOL32 bInMenuLoop;
	U32 toolbarHeight;


	COMPTR<IFontDrawAgent> font;
	COMPTR<IFontDrawAgent> shadowFont;

	COMPTR<IFontDrawAgent> fontName;
	COMPTR<IFontDrawAgent> shadowFontName;
#endif

	StatusBarResource (void);

	~StatusBarResource (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IStatusBarResource methods */

	DEFMETHOD(SetText) (U32 dwResourceID, STATUSTEXTMODE mode);

	DEFMETHOD(SetTextString) (const wchar_t *string, STATUSTEXTMODE mode);

	DEFMETHOD(SetTextString2) (const wchar_t *name, const wchar_t *text);

	DEFMETHOD_(U32,GetText) (void);

	DEFMETHOD_(U32,GetHeight) (void);

	DEFMETHOD_(void,SetToolbarHeight) (U32 height);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* StatusBarResource methods */

	void reloadFonts (bool bLoad);

	void unDraw (void);

	void OnNoOwner (void)
	{
		SetText(0,STM_DEFAULT);		// empty text for now
	}

	void redraw (void);

	IDAComponent * GetBase (void)
	{
		return (IStatusBarResource *) this;
	}
};
//--------------------------------------------------------------------------//
//
StatusBarResource::StatusBarResource (void)
{
		// nothing to init so far?
}
//--------------------------------------------------------------------------//
//
StatusBarResource::~StatusBarResource (void)
{
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}
}
//--------------------------------------------------------------------------//
//
void StatusBarResource::redraw (void)
{
#ifndef USE_WINDOWS_CTRL

	//
	// NOTE:
	// might not want to lock the surface during the menu loop because of WIN16 trouble,
	// especially now that we are use GDI to draw the fonts
	//
	
//	if (glLockFrameBufferEXT && bInMenuLoop)
//		return;

	if (CQFLAGS.bGameActive && DEFAULTS->GetDefaults()->bNoStatusBar==0)
	{
		S32 font_height = (toolbarHeight)? 0 : font->GetFontHeight();
		const S32 y0 = 	SCREENRESY-toolbarHeight-IDEAL2REALY(4)-font_height;
		S32 x0 = 0;

		bool bFrameLocked=(CQFLAGS.bFrameLockEnabled!=0);

		if (bFrameLocked)
			if (SURFACE->Lock() == false)
				return;

		U32 color;
		switch (mode)
		{
		default:
		case STM_TOOLTIP:
		case STM_DEFAULT:
		case STM_DEFAULT_NAME:
			color = RGB_GOLD | 0xFF000000;
			break;

		case STM_BUILD_DENIED:
			color = RGB(100,100,100) | 0xFF000000;
			break;

		case STM_BUILD:
		case STM_NAME:
			color = RGB(200,200,200) | 0xFF000000;
			break;
		}

		if (mode==STM_DEFAULT_NAME)
		{
			shadowFontName->SetFontColor(0 | 0xFF000000, 0);
			fontName->SetFontColor(RGB(200,200,200) | 0xFF000000, 0);

			if (szName[0])
			{
				shadowFontName->StringDraw(0, x0+IDEAL2REALX(2)+IDEAL2REALX(1), y0+IDEAL2REALY(1), szName);
				fontName->StringDraw(0, x0+IDEAL2REALX(2), y0, szName);
				x0 += fontName->GetStringWidth(szName);
				x0 += IDEAL2REALX(10);
			}
		}

		shadowFont->SetFontColor(0 | 0xFF000000, 0);

		if (dwTextID)
			shadowFont->StringDraw(0, x0+IDEAL2REALX(2)+IDEAL2REALX(1), y0+IDEAL2REALY(1), dwTextID);
		else 
		if (szText[0])
			shadowFont->StringDraw(0, x0+IDEAL2REALX(2)+IDEAL2REALX(1), y0+IDEAL2REALY(1), szText);

		font->SetFontColor(color, 0);

		if (dwTextID)
			font->StringDraw(0, x0+IDEAL2REALX(2), y0, dwTextID);
		else 
		if (szText[0])
			font->StringDraw(0, x0+IDEAL2REALX(2), y0, szText);

		if (bFrameLocked)
			SURFACE->Unlock();
	}

#endif
}
//--------------------------------------------------------------------------//
// undraw the previous text (this operation is slow
void StatusBarResource::unDraw (void)
{
	if (CQFLAGS.bGameActive && DEFAULTS->GetDefaults()->bNoStatusBar==0)
	{
		S32 font_height = font->GetFontHeight();
		const S32 y0 = 	SCREENRESY-toolbarHeight-font_height-IDEAL2REALY(4);
		bool bFrameLocked=(CQFLAGS.bFrameLockEnabled!=0);

		if (bFrameLocked)
			if (SURFACE->Lock() == false)
				return;

		{
			S32 x0;
			S32 x1;
			x0 = 0;
			x1 = SCREENRESX-1;

			DA::RectangleFill(0, x0, y0, x1, y0+font_height, 0);
		}
		
		if (bFrameLocked)
			SURFACE->Unlock();
	}
}
//--------------------------------------------------------------------------//
//
U32 StatusBarResource::GetText (void)
{
	return dwTextID;
}
//--------------------------------------------------------------------------//
//
GENRESULT StatusBarResource::SetText (U32 dwResourceID, STATUSTEXTMODE _mode)
{
#ifdef USE_WINDOWS_CTRL
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM) _localLoadString(dwResourceID)); 
	return GR_OK;
#else

	dwTextID = dwResourceID;
	szText[0] = 0;
	mode = _mode;

	if (bInMenuLoop && CQFLAGS.bFrameLockEnabled)
	{
		unDraw();
		redraw();
		InvalidateRect(hMainWindow, 0, 0);
	}
	
	return GR_OK;
#endif
}
//--------------------------------------------------------------------------//
//
GENRESULT StatusBarResource::SetTextString (const wchar_t *string, STATUSTEXTMODE _mode)
{
#ifdef USE_WINDOWS_CTRL
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM) string); 
	return GR_OK;
#else
	wcsncpy(szText, string, CHARCOUNT(szText)-1);
	dwTextID = 0;
	mode = _mode;
	return GR_OK;
#endif
}
//--------------------------------------------------------------------------//
//
GENRESULT StatusBarResource::SetTextString2 (const wchar_t *name, const wchar_t *text)
{
	wcsncpy(szText, text, CHARCOUNT(szText)-1);
	wcsncpy(szName, name, CHARCOUNT(szName)-1);

	dwTextID = 0;
	mode = STM_DEFAULT_NAME;
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
U32 StatusBarResource::GetHeight (void)
{
	return dwHeight;
}
//--------------------------------------------------------------------------//
//
void StatusBarResource::SetToolbarHeight (U32 height)
{
	toolbarHeight = height;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT StatusBarResource::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
#ifdef USE_WINDOWS_CTRL
	case WM_SIZE:
		{
			WORD wWidth, wHeight;

			wWidth = LOWORD(msg->lParam);
			wHeight = HIWORD(msg->lParam);

			MoveWindow(hStatusBar, 0, 0, wWidth, 0, 1);
		}
		break;
#else
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
#endif
	case CQE_ENDFRAME:
		redraw();
		break;

	case CQE_UPDATE:
		updateResource();
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
void StatusBarResource::reloadFonts (bool bLoad)
{
	if (bLoad)
	{
		HFONT hFont;
		COLORREF pen, background;

		pen			= RGB_GOLD		   | 0xFF000000;		// white	(pen color)
		background	= RGB(0,0,0);	   //| 0xFF000000;		// black	(background color)
		hFont = CQCreateFont(IDS_STATUSBAR_FONT);

		CreateFontDrawAgent(hFont, 1, pen, background, font);
		font->CreateDuplicate(shadowFont);

		font->CreateDuplicate(shadowFontName);
		font->CreateDuplicate(fontName);
	}
	else
	{
		font.free();
		shadowFont.free();
		fontName.free();
		shadowFontName.free();
	}
}
//--------------------------------------------------------------------------//
//
struct _status : GlobalComponent
{
	StatusBarResource * SBAR;
	
	virtual void Startup (void)
	{
#ifdef USE_WINDOWS_CTRL
		HWND hStatus = CreateStatusWindow(WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VISIBLE | SBS_SIZEGRIP,
						"Ready", hMainWindow, ID_STATUSBAR);

		RECT rect;

		STATUS = SBAR = new DAComponent<StatusBarResource>);
		AddToGlobalCleanupList((IDAComponent **) &STATUS);

		SBAR->hStatusBar = hStatus;

		GetWindowRect(hStatus, &rect);
		SBAR->dwHeight = rect.bottom - rect.top + 1;
#else
		STATUS = SBAR = new DAComponent<StatusBarResource>;
		AddToGlobalCleanupList((IDAComponent **) &STATUS);
#endif
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(STATUS, &SBAR->handle);
			FULLSCREEN->SetCallbackPriority(SBAR, EVENT_PRIORITY_STATUS);
		}
	
		SBAR->OnNoOwner();
	}
};

static _status startup;

//--------------------------------------------------------------------------//
//-----------------------------End StatusBar.cpp----------------------------//
//--------------------------------------------------------------------------//
