//--------------------------------------------------------------------------//
//                                                                          //
//                      Teletype.cpp                                        //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//																			//
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

	Control that scrolls informative text to the user			

*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "ITeletype.h"
#include "BaseHotRect.h"
#include "EventPriority.h"
#include "DrawAgent.h"
#include "Startup.h"

#include "frame.h"
#include "hotkeys.h"
#include "Sfx.h"

#include <da_vector>
using namespace da_std;

#define MAX_CHARS 256

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct TeletypeLine
{
	TeletypeLine (wchar_t *szString, RECT rc, U32 _fontID, COLORREF _color, U32 _speed = 20, bool _bMute = false, bool _bCenter = false) 
		:  fontID(_fontID), color(_color), speed(_speed), bMute(_bMute), bCenter(_bCenter)
	{
		reloadFonts(true);
		wcsncpy(string, szString, sizeof(string)/sizeof(wchar_t));
		bDone = false;
		bLast = false;
		blink = 0;
		counter = 0;
		timer = 0;

		pane.window = NULL;
		pane.x0 = rc.left;
		pane.x1 = rc.right;
		pane.y0 = rc.top;
		pane.y1 = rc.bottom;

		paneCaret = pane;
		rcFull = rc;

		// temp - put someplace else
		Setup();
	}

	TeletypeLine (const TeletypeLine& rhs) : fontID(rhs.fontID), color(rhs.color), speed(rhs.speed), bMute(rhs.bMute), bCenter(rhs.bCenter)
	{
		*this = rhs;
	}

	~TeletypeLine (void)
	{
	}

	TeletypeLine& operator = ( const TeletypeLine& rhs )
	{
		wcsncpy(string, rhs.string, wcslen(rhs.string));
		
		pane = rhs.pane;
		paneCaret = rhs.paneCaret;

		counter = rhs.counter;
		timer = rhs.timer;
		blink = rhs.blink;
		bDone = rhs.bDone;
		bLast = rhs.bLast;

		nChars = rhs.nChars;
		height = rhs.height;
		width  = rhs.width;

		fontAgent = rhs.fontAgent;
		backAgent = rhs.backAgent;
		rcFull	  = rhs.rcFull;	
		
		U32 i;
		for (i = 0; i < nChars+1; i++)
		{
			pos[i] = rhs.pos[i];
		}

		return *this;
	}

	COMPTR<IFontDrawAgent> fontAgent;
	COMPTR<IFontDrawAgent> backAgent;
	PANE pane;
	PANE paneCaret;
	
	static HSOUND HsndTeletype;
	static bool bPlaying;
	const bool bMute;
	const bool bCenter;

	wchar_t string[MAX_CHARS];
	S32     pos[MAX_CHARS];

	RECT rcFull;
	const U32  fontID;
	const COLORREF color;
	const int speed;  // in chars per second

	bool bDone;
	bool bLast;
	U32  counter;
	U32  oldcounter;
	U32  timer;
	U32  blink;
	U32  nChars;
	U32  height;
	U32  width;
	U32  xoffset;

	void Setup (void)
	{
		nChars = wcslen(string);
		CQASSERT(nChars <= MAX_CHARS);

		// find all of our positions
		U32 i;
		wchar_t buffer[MAX_CHARS];

		pos[0] = 0;
		for (i = 1; i < nChars+1; i++)
		{
			memset(buffer, 0, sizeof(buffer));
			wcsncpy(buffer, string, i);
			pos[i] = fontAgent->GetStringWidth(buffer);
		}

		height = fontAgent->GetFontHeight();
		width  = fontAgent->GetStringWidth(L"W");

		if (bCenter)
		{
			xoffset = (rcFull.right - rcFull.left) - fontAgent->GetStringWidth(string);
			xoffset /= 2;
		}
		else
		{
			xoffset = 0;
		}

		Update(0);
	}

	void Draw (void)
	{
		backAgent->StringDraw(&pane, 1 + xoffset, 1, string);
		backAgent->StringDraw(&pane, -1 + xoffset , -1, string);
		backAgent->StringDraw(&pane, 1 + xoffset, -1, string);
		backAgent->StringDraw(&pane, -1 + xoffset, 1, string);
		fontAgent->StringDraw(&pane,  xoffset, 0, string);

		if (blink < 500)
		{
			DA::RectangleFill(&paneCaret, pos[counter]+1 + xoffset, 1, pos[counter] + width + xoffset, height, color);
			
			DA::LineDraw(&paneCaret, pos[counter] + xoffset,				0,		pos[counter] + width + xoffset,	0,		0);
			DA::LineDraw(&paneCaret, pos[counter] + width + xoffset,		0,		pos[counter] + width + xoffset,	height,	0);
			DA::LineDraw(&paneCaret, pos[counter] + width + xoffset,		height,	pos[counter] + xoffset,			height, 0);
			DA::LineDraw(&paneCaret, pos[counter] + xoffset,				height, pos[counter] + xoffset,			0,		0);
		}
	}

	void Release (void)
	{
		reloadFonts(false);
	}

	void Reset (void)
	{
		timer = 0;
		blink = 0;
		bDone = false;
	}

	void BumpUp (void)
	{
		OffsetRect(&rcFull, 0, -(pane.y1 - pane.y0));
		pane.y0 = rcFull.top;
		pane.y1 = rcFull.bottom;

		paneCaret.y0 = pane.y0;
		paneCaret.y1 = pane.y1;
	}

	void Update (U32 dt)
	{
		// change the size of the pane to reveal more characters over time
		if (bDone == false)
		{
			timer += dt;
			counter = (timer)*(speed)/1000;

			if (counter > oldcounter && bMute == false)
			{
				SFXMANAGER->Play(HsndTeletype, 0, 0);
			}

			if (counter >= nChars)
			{
				bDone = true;
				counter = nChars;
			}

			pane.x1 = pane.x0 + pos[counter] + xoffset;
			paneCaret.x1 = paneCaret.x0 + pos[counter] + width*2 + xoffset;
			oldcounter = counter;
		}
		else
		{

			// also, blink the caret (below 500 is on, above is off)
			blink = (blink + dt)%1000;

			if (bLast == false)
			{
				// no blinking caret at end of text if it is not the last Teletype line
				blink+=500;
			}
		}
	}

	void reloadFonts(bool bReload)
	{
		if (bReload)
		{
			HFONT hFont;
			COLORREF pen, background;

			pen			= color | 0xFF000000;		
			background	= RGB(0,0,0);
			hFont = CQCreateFont(fontID);

			CreateFontDrawAgent(hFont, 1, pen, background, fontAgent);
			CreateFontDrawAgent(hFont, 0, background | 0xFF000000, background, backAgent);
		}
		else
		{
			fontAgent.free();
			backAgent.free();
		}
	}
};

HSOUND TeletypeLine::HsndTeletype;

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
struct TeletypeObj
{
	typedef vector<TeletypeLine*> TTLINE_VECTOR;

	TeletypeObj (const wchar_t *string, U32 _fontID, COLORREF _color, RECT rc, U32 _lifetime = 10000, U32 _speed = 20, 
				 bool _bMute = false, bool _bIgnorePause = false, bool _bCenter = false) 
		: fontID(_fontID), color(_color), bMute(_bMute), bIgnorePause(_bIgnorePause), bCenter(_bCenter)
	{
		lifetime = _lifetime;
		speed = _speed;
		bDead = false;
		nLines = 0;
		timer = 0;

		nChars = (U16)(string[0]-1);
		string = string++;
		text = string;

		rect.left = IDEAL2REALX(rc.left);
		rect.top = IDEAL2REALY(rc.top);
		rect.right = IDEAL2REALX(rc.right);
		rect.bottom = IDEAL2REALY(rc.bottom);

		processString();
	}

	~TeletypeObj()
	{
		U32 i = 0;
		while (lines.size())
		{
			TeletypeLine * ptr = lines[i];
			lines.erase(lines.begin());
			delete ptr;
		}
	}

	TeletypeObj (const TeletypeObj& rhs) 
		: fontID(rhs.fontID), color(rhs.color), bMute(rhs.bMute), bIgnorePause(rhs.bIgnorePause), bCenter(rhs.bCenter)
	{
		*this = rhs;
	}

	TeletypeObj& operator = ( const TeletypeObj& rhs )
	{
		lines = rhs.lines;

		bDead		= rhs.bDead;
		timer		= rhs.timer;
		lifetime	= rhs.lifetime;
		speed		= rhs.speed;
		nLines		= rhs.nLines;
		lineHeight	= rhs.lineHeight;

		rect	= rhs.rect;
		text	= rhs.text;
		nChars	= rhs.nChars;

		return *this;
	}

	COMPTR<IFontDrawAgent> testAgent;

	bool bDead;
	U32 timer;
	U32 lifetime;
	U32 speed;			// in letters per sec
	U32 nLines; 
	U32 lineHeight;		// the height of one line of text
	U32 teletypeID;

	const U32 fontID;
	const COLORREF color;
	const bool bMute;
	const bool bIgnorePause;
	const bool bCenter;

	RECT rect;			// the all-encompassing rectangle

	const wchar_t *text;		// the counted wide string
	U16 nChars;			// number of characters in the whole string
	TTLINE_VECTOR lines;

	void Draw (void)
	{
		if (!CQFLAGS.bGamePaused || bIgnorePause)
		{
			U32 i;
			for (i = 0; i < nLines; i++)
			{
				if (lines[i]->rcFull.top >= rect.top)
					lines[i]->Draw();
				if (lines[i]->bDone == false)
					break;
			}
		}
	}

	void Update (U32 dt)
	{
		if (!CQFLAGS.bGamePaused || bIgnorePause)
		{
			timer += dt;
			if (lifetime && (timer > lifetime))
			{
				bDead = true;
			}

			U32 i;
			for (i = 0; i < nLines; i++)
			{
				lines[i]->Update(dt);

				// do we have to bump up the text?
				if (lines[i]->rcFull.bottom > rect.bottom)
				{
					bumpUpLines();
				}

				if (lines[i]->bDone == false)
				{
					break;
				}
			}
		}
	}

	void bumpUpLines (void)
	{
		U32 i;
		for (i = 0; i < lines.size(); i++)
		{
			lines[i]->BumpUp();
		}
	}

	void createTestAgent (void)
	{
		if (testAgent == NULL)
		{
			HFONT hFont;
			COLORREF pen, background;

			pen			= color | 0xFF000000;		
			background	= RGB(0,0,0);
			hFont = CQCreateFont(fontID);

			CreateFontDrawAgent(hFont, 1, pen, background, testAgent);
		}
	}

	void reloadFonts (bool bReload)
	{
		if (bReload)
		{
			createTestAgent();
		}
		else
		{
			if (testAgent)
			{
				testAgent.free();
			}
		}

		U32 i;
		for (i = 0; i < lines.size(); i++)
		{
			lines[i]->reloadFonts(bReload);
		}
	}

	void processString (void)
	{
		wchar_t buffer[MAX_CHARS];

		createTestAgent();
		lineHeight = testAgent->GetFontHeight();

		// divy the string up into seperate lines and add each line to the vector
		RECT rc;
		rc.left   = rect.left;
		rc.top    = rect.top;
		rc.right  = rect.right;
		rc.bottom = rect.top + lineHeight;

		int idx = 0;
		int next  = 0;
		int cnt   = 0;
		
		do 
		{
			// take away any newline characters
			if (text[idx] == 13 || text[idx] == 10)
			{
				idx++;
			}

			next = getNextLineLength(&text[idx], nChars-idx);

			memset(buffer, 0, sizeof(buffer));
			wcsncpy(buffer, &text[idx], next);
			
			if (cnt)
			{
				OffsetRect(&rc, 0, lineHeight);
			}
			TeletypeLine *tl = new TeletypeLine(buffer, rc, fontID, color, speed, bMute, bCenter);

			lines.push_back(tl);

			if (next >= nChars - idx)// wcslen(&text[idx]))
			{
				// this is the last one we're adding
				break;
			}
			else
			{
				idx += next;
				cnt++;
			}
		} while (next > 0);

		nLines = lines.size();
		lines[nLines-1]->bLast = true;
	}

	
	// return number of characters to print in the next line.
	// look for newlines, spaces as clues to where to break
	int getNextLineLength (const wchar_t *string, int lastchar)
	{
		int width = 0;
		int maxWidth = rect.right - rect.left;
		int lastSpace = -1;		// index of last space character found
		int index = 0;
		
		wchar_t letter = string[index];

		while (letter && width <= maxWidth && index <= lastchar)  
		{
			if (letter == 13 || letter==10)		// newline character?
			{
				break;
			}
			else
			if (letter == ' ')
				lastSpace = index;
			
			wchar_t let[2];
			let[0] = letter;
			let[1] = 0;

			width += testAgent->GetStringWidth(let);
			letter = string[++index];
		}

		// back up until
		if (width > maxWidth)
		{
			if (lastSpace>=0 && index - lastSpace < 16)		// did we find a break in last 15 characters?
				index = lastSpace + 1;
			else
			if (index>1)
				index--;
		}
		
		return index;
	}
};


//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Teletype : public IEventCallback, ITeletype
{
	BEGIN_DACOM_MAP_INBOUND(Teletype)
	DACOM_INTERFACE_ENTRY(ITeletype)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	typedef vector<TeletypeObj*> TTOBJ_VECTOR;

	U32 handle;			// connection handle
	static U32 teletypeIDCounter;
	
	HSOUND hsndTeletype;

	TTOBJ_VECTOR teletypes;


	Teletype (void);

	~Teletype (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ITeletype methods */

	virtual void Flush (void);

	virtual U32 CreateTeletypeDisplay (const wchar_t *_string, RECT rc, U32 fontID, COLORREF color, U32 _lifetime = 10000, U32 _textTime = 5000, bool bMute = false, bool bCenter = false);

	virtual U32 CreateTeletypeDisplayEx (const wchar_t *_string, RECT rc, U32 fontID, COLORREF color, U32 _lifetime = 10000, U32 _textTime = 5000, bool bMute = false, bool bIgnorePause = false);

	virtual BOOL32 IsPlaying (U32 teletypeID);

	virtual U32 GetLastTeletypeID (void)
	{
		return teletypeIDCounter;
	}
	virtual void SetLastTeletypeID (U32 id)
	{
		teletypeIDCounter = id;
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* Teletype methods */

	void reloadFonts (bool bLoad);

	void draw (void);

	IDAComponent * GetBase (void)
	{
		return (ITeletype *) this;
	}

	void onUpdate (U32 dt)			// dt is milliseconds
	{
		U32 i;			
		for (i = 0; i < teletypes.size(); i++)
		{
			if (teletypes[i]->bDead)
			{
				TeletypeObj * ptr = teletypes[i];
				teletypes.erase(teletypes.begin()+i);
				delete ptr;
				break;
			}
		}

		for (i = 0; i < teletypes.size(); i++)
		{
			teletypes[i]->Update(dt);
		}
	}
};
U32	Teletype::teletypeIDCounter = 0;

//--------------------------------------------------------------------------//
//
Teletype::Teletype (void)
{
}
//--------------------------------------------------------------------------//
//
Teletype::~Teletype (void)
{
	if (TELETYPE)
	{
		TELETYPE->Flush();
	}

	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(handle);
	}

	// close the  sound handles
	if (SFXMANAGER && hsndTeletype)
	{
		SFXMANAGER->CloseHandle(hsndTeletype);
		hsndTeletype = NULL;
	}
}
//--------------------------------------------------------------------------//
//
U32 Teletype::CreateTeletypeDisplay (const wchar_t *_string, RECT rc, U32 _fontID, COLORREF color, U32 _lifetime, U32 _textTime, bool bMute, bool bCenter)
{
	U32 result;

	if (hsndTeletype == NULL)
	{
		// load the sounds
		hsndTeletype = SFXMANAGER->Open(SFX::TELETYPE);
		TeletypeLine::HsndTeletype = hsndTeletype;
	}

	U32 speed = U32(_string[0])*1000/_textTime;

	TeletypeObj *to = new TeletypeObj(_string, _fontID, color, rc, _lifetime, speed, bMute, false, bCenter);
	result = to->teletypeID = ++teletypeIDCounter;
	teletypes.push_back(to);

	return result;
}
//--------------------------------------------------------------------------//
//
U32 Teletype::CreateTeletypeDisplayEx (const wchar_t *_string, RECT rc, U32 _fontID, COLORREF color, U32 _lifetime, U32 _textTime, bool bMute, bool bIgnorePause)
{
	U32 result;

	if (hsndTeletype == NULL)
	{
		// load the sounds
		hsndTeletype = SFXMANAGER->Open(SFX::TELETYPE);
		TeletypeLine::HsndTeletype = hsndTeletype;
	}

	U32 speed = U32(_string[0])*1000/_textTime;

	TeletypeObj *to = new TeletypeObj(_string, _fontID, color, rc, _lifetime, speed, bMute, bIgnorePause);
	result = to->teletypeID = ++teletypeIDCounter;
	teletypes.push_back(to);

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL32 Teletype::IsPlaying (U32 teletypeID)
{
	// go through all the teletypes, see if there is a matching ID
	U32 i;
	for (i = 0; i < teletypes.size(); i++)
	{
		if (teletypes[i]->teletypeID == teletypeID)
		{
			return TRUE;
		}
	}

	return FALSE;
}
//--------------------------------------------------------------------------//
//
void Teletype::draw (void)
{
	bool bFrameLocked = (CQFLAGS.bFrameLockEnabled != 0);

	if (bFrameLocked)
	{
		if (SURFACE->Lock() == false)
		{
			return;
		}
	}

	U32 i;
	for (i = 0; i < teletypes.size(); i++)
	{
		teletypes[i]->Draw();
	}
		
	if (bFrameLocked)
	{
		SURFACE->Unlock();
	}
}
//--------------------------------------------------------------------------//
//
void Teletype::Flush (void)
{
	// goddamn STL bit me in the ass again!  Have to delete the pointers stored ourselves....
	while (teletypes.size())
	{
		TeletypeObj * ptr = teletypes[0];
		teletypes.erase(teletypes.begin());
		delete ptr;
	}
	
	// Dump any Teletype sound playing by closing the sound handles
	if (SFXMANAGER && hsndTeletype)
	{
		SFXMANAGER->CloseHandle(hsndTeletype);
		hsndTeletype = NULL;
	}
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Teletype::Notify (U32 message, void *param)
{
//	wchar_t string[] = L" Here is the test teletype string, what the hell is the problem here anyway, I don't see anything.";
//	string[0] = wchar_t(wcslen(string)-1);
//	RECT rc;
//	::SetRect(&rc, 20, 20, 200, 200);

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		Flush();
		break;

	case CQE_START3DMODE:
		reloadFonts(true);
		break;
	case CQE_END3DMODE:
		reloadFonts(false);
		break;

	case CQE_ENDFRAME:
		draw();
		break;

//	case SOMETHING_OR_OTHER:
//		Flush();
//		CreateTeletypeDisplay (string, rc, IDS_STATUSBAR_FONT, RGB(255,0,0)); 
//		break;

	case CQE_UPDATE:
//		if (!CQFLAGS.bGamePaused)
			onUpdate(S32(param) >> 10);
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void Teletype::reloadFonts (bool bLoad)
{
	U32 i;
	for (i = 0; i < teletypes.size(); i++)
	{
		teletypes[i]->reloadFonts(bLoad);
	}
}

//--------------------------------------------------------------------------//
//
struct _teletype : GlobalComponent
{
	Teletype * TTYPE;
	
	virtual void Startup (void)
	{
		TELETYPE = TTYPE = new DAComponent<Teletype>;
		AddToGlobalCleanupList((IDAComponent **) &TELETYPE);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
	
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(TTYPE->GetBase(), &TTYPE->handle);
			FULLSCREEN->SetCallbackPriority(TTYPE, EVENT_PRIORITY_TEXTCHAT);
		}
	}
};

static _teletype startup;

//--------------------------------------------------------------------------//
//--------------------------End Teletype.cpp--------------------------------//
//--------------------------------------------------------------------------//