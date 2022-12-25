//--------------------------------------------------------------------------//
//                                                                          //
//                      Subtitle.cpp                                        //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//																			//
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

	Control that scrolls informative text to the user			

*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "ISubtitle.h"
#include "BaseHotRect.h"
#include "EventPriority.h"
#include "DrawAgent.h"
#include "Startup.h"
#include "SoundManager.h"

#include "frame.h"

#define SUB_POSX1 150
#define SUB_POSX2 635
#define SUB_POSY1 110
#define SUB_POSY2 350

#define B_SUB_POSX1 70
#define B_SUB_POSX2 745
#define B_SUB_POSY1 320
#define B_SUB_POSY2 550

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Subtitle : public IEventCallback, ISubtitle
{
	BEGIN_DACOM_MAP_INBOUND(Subtitle)
	DACOM_INTERFACE_ENTRY(ISubtitle)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 handle;			// connection handle
	
	U32 soundHandle;
	wchar_t textBuffer[1024];

	bool bBriefing;

	COMPTR<IFontDrawAgent> fontAgent;

	Subtitle (void);

	~Subtitle (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ISubtitle methods */

	virtual void NewSubtitle(wchar_t * subtitle,U32 _soundHandle);

	virtual void NewBriefingSubtitle(wchar_t * subtitle,U32 _soundHandle);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* Subtitle methods */

	void reloadFonts (bool bLoad);

	void draw (void);

	IDAComponent * GetBase (void)
	{
		return (ISubtitle *) this;
	}

	void onUpdate (U32 dt)			// dt is milliseconds
	{
		if(soundHandle && (!SOUNDMANAGER->IsPlaying(soundHandle)))
		{
			soundHandle = 0;
		}

	}
};

//--------------------------------------------------------------------------//
//
Subtitle::Subtitle (void)
{
	soundHandle = 0;
}
//--------------------------------------------------------------------------//
//
Subtitle::~Subtitle (void)
{
	fontAgent.free();
}
//--------------------------------------------------------------------------//
//
void Subtitle::NewBriefingSubtitle(wchar_t * subtitle,U32 _soundHandle)
{
	if(subtitle)
	{
		CQASSERT(subtitle[0]-1 < 1024);
		soundHandle = _soundHandle;
		wcsncpy(textBuffer,subtitle+1,min(subtitle[0],1023));
		textBuffer[min(subtitle[0],1023)] = 0;

		if(!fontAgent)
			reloadFonts(true);

		PANE pane;
		pane.window = NULL;
		pane.x0 = IDEAL2REALX(B_SUB_POSX1);
		pane.x1 = IDEAL2REALX(B_SUB_POSX2);
		pane.y0 = IDEAL2REALY(B_SUB_POSY1);
		pane.y1 = IDEAL2REALY(B_SUB_POSY2);
		
		bBriefing = true;
	}
}
//--------------------------------------------------------------------------//
//
void Subtitle::NewSubtitle(wchar_t * subtitle,U32 _soundHandle)
{
	if(subtitle)
	{
		CQASSERT(subtitle[0]-1 < 1024);
		soundHandle = _soundHandle;
		wcsncpy(textBuffer,subtitle+1,min(subtitle[0],1023));
		textBuffer[min(subtitle[0],1023)] = 0;

		if(!fontAgent)
			reloadFonts(true);

		PANE pane;
		pane.window = NULL;
		pane.x0 = IDEAL2REALX(SUB_POSX1);
		pane.x1 = IDEAL2REALX(SUB_POSX2);
		pane.y0 = IDEAL2REALY(SUB_POSY1);
		pane.y1 = IDEAL2REALY(SUB_POSY2);
		
		bBriefing = false;
	}
}
//--------------------------------------------------------------------------//
//
void Subtitle::draw (void)
{
	if(soundHandle && DEFAULTS->GetDefaults()->bSubtitles && !CQFLAGS.bGamePaused)
	{
		bool bFrameLocked = (CQFLAGS.bFrameLockEnabled != 0);

		if (bFrameLocked)
		{
			if (SURFACE->Lock() == false)
			{
				return;
			}
		}

		if(bBriefing)
		{
			PANE pane;
			pane.window = NULL;

			pane.x0 = IDEAL2REALX(B_SUB_POSX1);
			pane.x1 = IDEAL2REALX(B_SUB_POSX2);
			pane.y0 = IDEAL2REALY(B_SUB_POSY1);
			pane.y1 = IDEAL2REALY(B_SUB_POSY2);
			fontAgent->StringDraw(&pane,  0, 0, textBuffer);
		}
		else
		{
			PANE pane;
			pane.window = NULL;

			pane.x0 = IDEAL2REALX(SUB_POSX1);
			pane.x1 = IDEAL2REALX(SUB_POSX2);
			pane.y0 = IDEAL2REALY(SUB_POSY1);
			pane.y1 = IDEAL2REALY(SUB_POSY2);
			fontAgent->StringDraw(&pane,  0, 0, textBuffer);
		}

		if (bFrameLocked)
		{
			SURFACE->Unlock();
		}
	}
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Subtitle::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_START3DMODE:
		reloadFonts(true);
		break;
	case CQE_END3DMODE:
		reloadFonts(false);
		break;

	case CQE_ENDFRAME:
		draw();
		break;


	case CQE_UPDATE:
		onUpdate(S32(param) >> 10);
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void Subtitle::reloadFonts (bool bLoad)
{
	if (bLoad)
	{
		HFONT hFont;
		COLORREF pen, background;

		pen			= RGB(255,255,255) | 0xFF000000;		
		background	= RGB(0,0,0);
		hFont = CQCreateFont(IDS_SUBTITLE_FONT);

		CreateMultilineFontDrawAgent(0,hFont, pen, background, fontAgent);
	}
	else
	{
		fontAgent.free();
	}
}

//--------------------------------------------------------------------------//
//
struct _subtitle : GlobalComponent
{
	Subtitle * SSubTitle;
	
	virtual void Startup (void)
	{
		SUBTITLE = SSubTitle = new DAComponent<Subtitle>;
		AddToGlobalCleanupList((IDAComponent **) &SUBTITLE);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(SSubTitle->GetBase(), &SSubTitle->handle);
			FULLSCREEN->SetCallbackPriority(SSubTitle, EVENT_PRIORITY_TEXTCHAT);
		}
	}
};

static _subtitle sub_startup;

//--------------------------------------------------------------------------//
//--------------------------End Subtitle.cpp--------------------------------//
//--------------------------------------------------------------------------//