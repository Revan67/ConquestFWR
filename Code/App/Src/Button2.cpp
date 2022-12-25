//--------------------------------------------------------------------------//
//                                                                          //
//                               Button2.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Button2.cpp 64    9/15/00 2:13p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IButton2.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DButton.h>
#include "DrawAgent.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#define BEGIN_MAPPING(parent, name) \
	{								\
		HANDLE hFile, hMapping;		\
		DAFILEDESC fdesc = name;	\
		void *pImage;				\
		if ((hFile = parent->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)	\
			CQFILENOTFOUND(fdesc.lpFileName);	\
		hMapping = parent->CreateFileMapping(hFile);	\
		pImage = parent->MapViewOfFile(hMapping)

#define END_MAPPING(parent) \
		parent->UnmapViewOfFile(pImage); \
		parent->CloseHandle(hMapping); \
		parent->CloseHandle(hFile); }

//speed in miliseconds
#define REPEATER_BUTTON_SPEED 100

//--------------------------------------------------------------------------//
//
struct BUTTONTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	COMPTR<IDrawAgent> shapes[GTBSHP_MAX_SHAPES];
	U16 width, height;
	S16 leftMargin, rightMargin;		// skip pixels on the edge (for dropdown arrows)
	bool bDropdown;

	GT_BUTTON::COLOR colors[GTBTXT_MAX_STATES];
	GT_BUTTON::BUTTON_TYPE buttonType;

	BUTTONTYPE (void)
	{
	}

	~BUTTONTYPE (void)
	{
		if (pFontType)
			GENDATA->Release(pFontType);
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Button2 : BaseHotRect, IButton2, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Button2)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IButton2)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	BUTTONTYPE * pButtonType;
	COMPTR<IFontDrawAgent> pFont;

	U32 controlID;					// set by owner
	BTNTXT::BUTTON_TEXT buttonText;	
	wchar_t szButtonText[256];
	U32 leftOverTime;
	S16 xText, yText;			// offset from origin of button for text
	S16 m_width, m_height;

	bool bKeyboardFocus:1;
	bool bMousePressed:1;			// mouse press occurred inside rect
	bool bKeyboardPressed:1;		// keyboard used to press button
	bool bDisabled:1;
	bool bPushState:1;			// used for pushpin style buttons
	bool bPostMessageBlocked:1;
	bool bDropdown:1;				// if this button is part of a drop box, then draw the dropdown triangle
	bool bTransparent:1;
	bool bColorLocked:1;

	BOOL bUseShapeFile;


	U8	 colorState;			// which color to display for text

	RECT  buttonArea;
	
	//
	// class methods
	//

	Button2 (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		colorState = GTBTXT_MAX_STATES;		// invalid state
	}

	virtual ~Button2 (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IButton2 methods  */

	virtual void InitButton (const BUTTON_DATA & data, BaseHotRect * parent); 

	virtual void EnableButton (bool bEnable);

	virtual bool GetEnableState (void);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);
	
	virtual void SetTextID (U32 textID);	// set resource ID for text

	virtual void SetTextString (const wchar_t * szString);

	virtual int  GetText (wchar_t * szBuffer, int bufferSize);

	virtual void SetPushState (bool bPressed);

	virtual bool GetPushState (void);

	virtual void SetControlID (U32 id);

	virtual void GetDimensions (U32 & width, U32 & height);

	virtual void GetPosition (U32 &xpos, U32 &ypos)
	{
		xpos = screenRect.left;
		ypos = screenRect.top;
	}

	virtual const bool IsMouseOver (S16 x, S16 y) const
	{
		if (bHasFocus && bInvisible==false && x >= screenRect.left && x <= screenRect.right && y >= screenRect.top && y <= screenRect.bottom)
		{
			return true;
		}
		return false;
	}

	virtual const bool IsButtonDown (void) const
	{
		if (bMousePressed || bKeyboardPressed)
		{
			return true;
		}
		return false;
	}

	virtual void SetTransparent (bool _bTransparent)
	{
		if (bUseShapeFile == false)
		{
			bTransparent = _bTransparent;
		}
		else
		{
			bTransparent = false;
		}
	}

	virtual void SetDropdownBehavior (void)
	{
		bDropdown = true;
	}

	virtual void SetDefaultColor (COLORREF color)
	{
		// kind of a kludge, but oh well
		bColorLocked = true;

		if (pFont)
		{
			pFont->SetFontColor(color | 0xFF000000, 0);
		}
	}


	/* IKeyboardFocus methods */

	virtual bool SetKeyboardFocus (bool bEnable);

	virtual U32 GetControlID (void);

	/* BaseHotRect methods */

	virtual void onRequestKeyboardFocus (int x, int y)
	{
		if (bDisabled==false)
			BaseHotRect::onRequestKeyboardFocus(x, y);
	}

	virtual void ForceAlertState (bool _bAlert)
	{
		if (_bAlert)
		{
			bAlert = _bAlert;
		}
		else
		{
			// we want the true alert state
			S32 x, y;
			WM->GetCursorPos(x, y);
			CheckRect(x, y);
		}
	}

	virtual void ForceMouseDownState (bool bMouseDown)
	{
		bMousePressed = true;
	}

	/* Button2 methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (BUTTONTYPE * _pButtonType);

	void draw (void);

	void drawByPrimitives (void);

	void drawByShapeFile (U32 state);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDown (void)
	{
		if (bDisabled == false)
		{
			if (bAlert)
			{
				doPushAction();
			}

			bKeyboardPressed = false;
			bMousePressed = bAlert;
		}
	}

	void onLeftButtonUp (void)
	{
		if (bDisabled == false)
		{
			if (bAlert && bMousePressed)
			{
				bMousePressed = bKeyboardPressed = false;
				doReleaseAction();
			}
			else
				bMousePressed = false;
		}
	}

	void doPushAction (void)
	{
		if (pButtonType->buttonType != GT_BUTTON::DEFAULT && pButtonType->buttonType != GT_BUTTON::CHECKBOX)
		{
			if (bPostMessageBlocked==false || pButtonType->bDropdown)
			{
				parent->PostMessage(CQE_BUTTON, (void*)controlID);
				bPostMessageBlocked = true;
			}
		}
	}

	void doDblPushAction (void)
	{
		if (pButtonType->buttonType != GT_BUTTON::DEFAULT)
		{
			parent->PostMessage(CQE_BUTTON, (void*)(controlID|0x80000000));
		}
	}

	void doReleaseAction (void)
	{
		if (pButtonType->buttonType == GT_BUTTON::DEFAULT || pButtonType->buttonType == GT_BUTTON::CHECKBOX)
		{
			if (bPostMessageBlocked==false)
			{
				parent->PostMessage(CQE_BUTTON, (void*)controlID);
				bPostMessageBlocked = true;
			}
		}
	}

	void doRepeatButton(U32 miliElapsed)
	{
		if((!bDisabled) &&((bAlert && bMousePressed) || bKeyboardPressed || (pButtonType->buttonType != GT_BUTTON::DEFAULT && bPushState)))
		{
			miliElapsed += leftOverTime;
			while(miliElapsed > REPEATER_BUTTON_SPEED)
			{
				parent->PostMessage(CQE_BUTTON, (void*)controlID);			
				miliElapsed -= REPEATER_BUTTON_SPEED;
			}
			leftOverTime = miliElapsed;
		}else 
			leftOverTime = 0;
	}

	void setColorState (U32 state)
	{
		U8 newState;

		switch (state)
		{
		case GTBSHP_DISABLED:
			newState = GTBTXT_DISABLED;
			break;
		case GTBSHP_MOUSE_FOCUS:
			newState = GTBTXT_HIGHLIGHT;
			break;
		case GTBSHP_DEPRESSED:
			newState = (bAlert) ? GTBTXT_HIGHLIGHT : GTBTXT_NORMAL;
			break;
		case GTBSHP_KEYB_FOCUS:
			newState = GTBTXT_KEYFOCUS;
			break;
		case GTBSHP_NORMAL:
		default:
			newState = GTBTXT_NORMAL;
			break;
		}

		if (newState != colorState && bColorLocked == false)
		{
			colorState = newState;
			const GT_BUTTON::COLOR & color = pButtonType->colors[newState];

			pFont->SetFontColor(RGB(color.red, color.green, color.blue) | 0xFF000000, 0);
		}
	}

	void calculateTextOffest (void);
};
//--------------------------------------------------------------------------//
//
Button2::~Button2 (void)
{
	GENDATA->Release(pButtonType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void Button2::InitButton (const BUTTON_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, EVENT_PRIORITY_BUTTON);		
	}

	if ((buttonText = data.buttonText) != 0)
		wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));

	// are we using an art file for the dimension data?
	bUseShapeFile = (pButtonType->shapes[0] != NULL);

	if (!bUseShapeFile)
	{
		m_width = IDEAL2REALX(data.buttonArea.right-data.buttonArea.left);
		m_height = IDEAL2REALY(data.buttonArea.bottom-data.buttonArea.top);
	}

	//
	// calculate text area
	//
	buttonArea.left   = IDEAL2REALX(data.buttonArea.left);
	buttonArea.right  = IDEAL2REALX(data.buttonArea.right+1)-1;
	buttonArea.top    = IDEAL2REALY(data.buttonArea.top);
	buttonArea.bottom = IDEAL2REALY(data.buttonArea.bottom+1)-1;


	if (buttonArea.right == 0)
	{
		buttonArea.right = pButtonType->width + buttonArea.left - 1;
	}
	if (buttonArea.bottom == 0)
	{
		buttonArea.bottom = pButtonType->height + buttonArea.top - 1;
	}

	//
	// calculate screen rect. If no shape, rect == button area
	//
	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;

	if (pButtonType->height)
		screenRect.bottom = screenRect.top + pButtonType->height - 1;
	else
		screenRect.bottom = screenRect.top + buttonArea.bottom;

	if (pButtonType->width)
		screenRect.right = screenRect.left + pButtonType->width - 1;
	else
		screenRect.right = screenRect.left + buttonArea.right;

	//
	// calculate text offset
	//
	calculateTextOffest();

	if (controlID == 0)
		controlID = buttonText;
}
//--------------------------------------------------------------------------//
//
void Button2::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
		bDisabled = !bEnable;
		bMousePressed = bKeyboardPressed = false;
	}
}
//--------------------------------------------------------------------------//
//
bool Button2::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void Button2::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
bool Button2::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void Button2::SetTextID (U32 textID)
{
	buttonText = static_cast<BTNTXT::BUTTON_TEXT>(textID);
	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));

	calculateTextOffest();
}
//--------------------------------------------------------------------------//
//
void Button2::SetTextString (const wchar_t * szString)
{
	buttonText = BTNTXT::NOTEXT;
	wcsncpy(szButtonText, szString, sizeof(szButtonText)/sizeof(wchar_t));

	calculateTextOffest();
}
//--------------------------------------------------------------------------//
//
int Button2::GetText (wchar_t * szBuffer, int bufferSize)
{
	if (szBuffer==0)
	{
		return wcslen(szButtonText);
	}
	else
	{
		bufferSize /= sizeof(wchar_t);
		int len = wcslen(szButtonText);

		len = __min(len+1, bufferSize);
		memcpy(szBuffer, szButtonText, len*sizeof(wchar_t));

		return len-1;
	}
}
//--------------------------------------------------------------------------//
//
void Button2::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool Button2::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void Button2::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
void Button2::GetDimensions (U32 & width, U32 & height)
{
	width  = pButtonType->width;
	height = pButtonType->height;
}
//--------------------------------------------------------------------------//
//
bool Button2::SetKeyboardFocus (bool bEnable)
{
	if (bEnable && (bDisabled||bInvisible))
		return false;
	if (bEnable != bKeyboardFocus)  // if we are changing state
	{
		if ((bKeyboardFocus = bEnable) == 0)  // if loosing focus
			bKeyboardPressed = false;
	}
	return true;
}
//--------------------------------------------------------------------------//
//
U32 Button2::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT Button2::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case WM_LBUTTONDBLCLK:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bAlert)
		{
			doDblPushAction();
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		onLeftButtonDown();
		if (bAlert)
		{
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_LBUTTONUP:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		onLeftButtonUp();
		if (bAlert)
		{
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_KEYDOWN:
		if (bKeyboardFocus && bKeyboardPressed==false && bHasFocus)
		switch (msg->wParam)
		{
		case VK_RETURN:
		case VK_SPACE:
			if ((msg->lParam & 0x40000000) == 0)		// previous state == up
			{
				doPushAction();
				bKeyboardPressed = true;
				bMousePressed = false;
				BaseHotRect::Notify(message, param);
				return GR_GENERIC;
			}
			break;
		} // end switch (wParam)
		break; // end switch WM_KEYDOWN

	case CQE_UPDATE:
		if(pButtonType->buttonType == GT_BUTTON::REPEATER)
			doRepeatButton(U32(param)>>10);
		bPostMessageBlocked = false;
		if (bKeyboardFocus)
		{
			if (bKeyboardPressed && HOTKEY->GetVkeyState(VK_SPACE) == 0 && HOTKEY->GetVkeyState(VK_RETURN) == 0)
			{
				bKeyboardPressed = false;
				doReleaseAction();
			}
		}
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Button2::calculateTextOffest (void)
{
	if (bUseShapeFile)
	{
		yText = S32(pButtonType->height - pFont->GetFontHeight()) / 2;
		xText = pButtonType->leftMargin + (S32(pButtonType->width - pButtonType->leftMargin - pButtonType->rightMargin - pFont->GetStringWidth(szButtonText)) / 2);
	}
	else
	{
		yText = S32(m_height - pFont->GetFontHeight()) / 2;
		xText = pButtonType->leftMargin + (S32(m_width - pButtonType->leftMargin - pButtonType->rightMargin - pFont->GetStringWidth(szButtonText)) / 2);
	}

	if (xText < pButtonType->leftMargin)
	{
		xText = pButtonType->leftMargin;

		// if the text is too wide, then we should cut the string short with '...'
		U32 max_width = buttonArea.right - buttonArea.left - 2*pButtonType->leftMargin;
		U32 string_width = pFont->GetStringWidth(szButtonText);

		if (string_width > max_width)
		{
			// replace last three chars with "..."
			int len = wcslen(szButtonText);

			if (len > 3)
			{
				do
				{
					// take away a character from the end until we fit nicely
					szButtonText[len-1] = 0;
					string_width = pFont->GetStringWidth(szButtonText);
					len = wcslen(szButtonText);
				} while (string_width > max_width && len > 3);

				int last_char = len-1;

				for (int i = last_char-2; i <= last_char; i++)
				{
					szButtonText[i] = L'.';
				}
				szButtonText[len] = 0;
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Button2::draw (void)
{
	U32 state;
	
	if (bDisabled)
	{
		state = GTBSHP_DISABLED;
	}
	else
	{
		state = GTBSHP_NORMAL;
		if ((bAlert && bMousePressed) || bKeyboardPressed || 
			(pButtonType->buttonType != GT_BUTTON::DEFAULT && bPushState))
			state = GTBSHP_DEPRESSED;
		else
		if (bAlert)
			state = GTBSHP_MOUSE_FOCUS;

		if ((bTransparent || bUseShapeFile == false) && bKeyboardFocus)
			state = GTBSHP_KEYB_FOCUS;
	}

	// it is possible (for a checkbox) for the state to be depressed, but the push state is false
	if (state == GTBSHP_DEPRESSED && bPushState == false)
	{
		state = GTBSHP_NORMAL;

		if (bAlert)
			state = GTBSHP_MOUSE_FOCUS;

		if ((bTransparent || !bUseShapeFile) && bKeyboardFocus)
			state = GTBSHP_KEYB_FOCUS;		
	}

	setColorState(state);

	if (bUseShapeFile)
	{
		drawByShapeFile(state);
	}
	else if (bTransparent == false)
	{
		drawByPrimitives();
	}

	// draw the text, same for both rendering cases
	PANE pane;
	pane.window = NULL;
	pane.x0 = screenRect.left;// + pButtonType->leftMargin;
	pane.x1 = screenRect.right;
	pane.y0 = screenRect.top;
	pane.y1 = screenRect.bottom;

	if (state == GTBSHP_DEPRESSED && pButtonType->buttonType != GT_BUTTON::CHECKBOX)
		pFont->StringDraw(&pane, xText + U32(IDEAL2REALX(1.5F)), yText+U32(IDEAL2REALY(1.5F)), szButtonText);
	else
		pFont->StringDraw(&pane, xText, yText, szButtonText);

//	DA::LineDraw(NULL, screenRect.left + pButtonType->leftMargin, screenRect.top, screenRect.left + pButtonType->leftMargin, screenRect.bottom, RGB(255,255,0));
}
//--------------------------------------------------------------------------//
//
void Button2::drawByPrimitives (void)
{
	BATCH->set_state(RPR_BATCH,FALSE);

	GT_BUTTON::COLOR buttonColor;
	COLORREF color;
	PANE pane;
	pane.window = 0;
	pane.x0 = screenRect.left;
	pane.y0 = screenRect.top;
	pane.x1 = screenRect.right;
	pane.y1 = screenRect.bottom;

	// draw the backgound rect
	DA::RectangleFill(&pane, 0, 0, m_width, m_height, RGB(0,0,0));

	// draw the bounding rect outline
	int x1 = 0, y1 = 0;
	int x2 = screenRect.right - screenRect.left;
	int y2 = screenRect.bottom - screenRect.top;

	if (bDisabled)
	{
		buttonColor = pButtonType->colors[GTBTXT_DISABLED];
	}
	else if (bKeyboardFocus && bHasFocus)
	{
		buttonColor = pButtonType->colors[GTBTXT_KEYFOCUS];
	}
	else if (bAlert)
	{
		buttonColor = pButtonType->colors[GTBTXT_HIGHLIGHT];
	}
	else
	{
		buttonColor = pButtonType->colors[GTBTXT_NORMAL];
	}

	color = RGB(buttonColor.red, buttonColor.green, buttonColor.blue);

	// draw the border around the rect
	DA::LineDraw(&pane, x1, y1, x2, y1, color);
	DA::LineDraw(&pane, x2, y1, x2, y2, color);
	DA::LineDraw(&pane, x2, y2, x1, y2, color);
	DA::LineDraw(&pane, x1, y2, x1, y1, color);

	// if we are a checkbox and we are checked, than draw an 'X'
	if (pButtonType->buttonType == GT_BUTTON::CHECKBOX && bPushState)
	{
		x1+=3;
		x2-=3;
		y1+=3;
		y2-=3;

		DA::LineDraw(&pane, x1, y1, x2, y2, RGB(255,255,0));
		DA::LineDraw(&pane, x1, y2, x2, y1, RGB(255,255,0));
	}


	if (bDropdown)
	{
		// draw a  triangle
		POINT pt[3];
		RECT rc = { 0, 0, screenRect.right-screenRect.left-1, screenRect.bottom-screenRect.top-1};
		InflateRect(&rc, -3, -3);
		
		pt[0].x = rc.left;
		pt[0].y = rc.top;
		pt[1].x = rc.left + 10;
		pt[1].y = rc.top;
		pt[2].x = rc.left + 5;
		pt[2].y = rc.bottom;
		
		int i;
		int j;
		for (i = 0; i < 3; i++)
		{
			j = (i+1)%3;
			DA::LineDraw(&pane, pt[i].x, pt[i].y, pt[j].x, pt[j].y, color);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Button2::drawByShapeFile (U32 state)
{	
	BATCH->set_state(RPR_BATCH,FALSE);

	// are we a checkbox?
	if (pButtonType->buttonType == GT_BUTTON::CHECKBOX)
	{
		if (bDisabled)
		{
			pButtonType->shapes[GTBSHP_DISABLED]->Draw(0, screenRect.left, screenRect.top);
		}
		else if (bAlert)
		{
			pButtonType->shapes[GTBSHP_MOUSE_FOCUS]->Draw(0, screenRect.left, screenRect.top);
		}
		else
		{
			pButtonType->shapes[GTBSHP_NORMAL]->Draw(0, screenRect.left, screenRect.top);
		}

		if (bPushState)
		{
			pButtonType->shapes[GTBSHP_DEPRESSED]->Draw(0, screenRect.left, screenRect.top);
		}
	}
	else
	{
		// all our other buttons
		pButtonType->shapes[state]->Draw(0, screenRect.left, screenRect.top);
	}

	if (bKeyboardFocus && bHasFocus)
	{
		pButtonType->shapes[GTBSHP_KEYB_FOCUS]->Draw(0, screenRect.left, screenRect.top);
	}
}
//--------------------------------------------------------------------------//
//
void Button2::init (BUTTONTYPE * _pButtonType)
{
	COMPTR<IDAComponent> pBase;
	pButtonType = _pButtonType;

	GENDATA->CreateInstance(pButtonType->pFontType, pBase);
	pBase->QueryInterface("IFontDrawAgent", pFont);
}
//--------------------------------------------------------------------------//
//-----------------------Button Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ButtonFactory (void) { }

	~ButtonFactory (void);

	void init (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ICQFactory methods */

	virtual HANDLE CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *data);

	virtual BOOL32 DestroyArchetype (HANDLE hArchetype);

	virtual GENRESULT CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance);

	/* FontFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
ButtonFactory::~ButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ButtonFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_BUTTON)
	{
		GT_BUTTON * data = (GT_BUTTON *) _data;
		BUTTONTYPE * result = new BUTTONTYPE;

		result->pArchetype = pArchetype;

		result->pFontType = GENDATA->LoadArchetype(data->fontName);
		CQASSERT(result->pFontType);
		GENDATA->AddRef(result->pFontType);

		result->colors[GTBTXT_DISABLED] = data->disabledText;
		result->colors[GTBTXT_HIGHLIGHT] = data->highlightText;
		result->colors[GTBTXT_NORMAL] = data->normalText;

/*		// the last color is formed algorithmically
		result->colors[GTBTXT_KEYFOCUS].red = (result->colors[GTBTXT_NORMAL].red + 50)%255;
		result->colors[GTBTXT_KEYFOCUS].green = (result->colors[GTBTXT_NORMAL].green + 50)%255;
		result->colors[GTBTXT_KEYFOCUS].blue = (result->colors[GTBTXT_NORMAL].blue + 50)%255;
*/
		result->colors[GTBTXT_KEYFOCUS].red = 181;
		result->colors[GTBTXT_KEYFOCUS].green = 218;
		result->colors[GTBTXT_KEYFOCUS].blue = 240;

		result->buttonType = data->buttonType;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				int i;
				for (i = 0; i < GTBSHP_MAX_SHAPES; i++)
					CreateDrawAgent((VFX_SHAPETABLE *) pImage, i, result->shapes[i]);
			END_MAPPING(INTERFACEDIR);

			result->shapes[0]->GetDimensions(result->width, result->height);
		}

		result->leftMargin = IDEAL2REALX(data->leftMargin);
		result->rightMargin = IDEAL2REALX(data->rightMargin);
		result->bDropdown = (result->rightMargin || result->leftMargin);

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	BUTTONTYPE * type = (BUTTONTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	BUTTONTYPE * type = (BUTTONTYPE *) hArchetype;
	Button2 * result = new DAComponent<Button2>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _buttonfactory : GlobalComponent
{
	ButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _buttonfactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Button2.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//
