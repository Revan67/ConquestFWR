//--------------------------------------------------------------------------//
//                                                                          //
//                               Edit2.cpp                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Edit2.cpp 57    10/18/02 2:36p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IEdit2.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DEdit.h>
#include <DFonts.h>
#include "DrawAgent.h"

#include <hkevent.h>
#include <TComponent.h>
#include <FileSys.h>
#include <imm.h>

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

#define DEF_MAX_EDIT_CHARS 256
//--------------------------------------------------------------------------//
//
struct EDITTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	COMPTR<IDrawAgent> shapes[GTESHP_MAX_SHAPES];
	LOGFONT logFont;
	U16 width, height, justify;
	U32 blinkOffTime;
	U32 blinkOnTime;

	GT_EDIT::COLOR colors[GTETXT_MAX_STATES];

	EDITTYPE (void)
	{
	}

	~EDITTYPE (void)
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
struct DACOM_NO_VTABLE Edit2 : BaseHotRect, IEdit2, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Edit2)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IEdit2)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	U32 MAX_EDIT_CHARS;

	EDITTYPE * pEditType;
	COMPTR<IFontDrawAgent> pFont;

	U32 controlID;					// set by owner
	EDTXT::EDIT_TEXT editText;			
	wchar_t szEditText[DEF_MAX_EDIT_CHARS];
	wchar_t szScratchText[DEF_MAX_EDIT_CHARS];
	wchar_t szIgnoreChars[DEF_MAX_EDIT_CHARS];

	S16 xText, yText;			// offset from origin of edit for text
	S16 width, height;

	bool bKeyboardFocus:1;
	bool bMousePressed:1;			// mouse press occurred inside rect
	bool bDisabled:1;
	bool bOverwriteMode:1;
	bool bPostMessageBlocked:1;
	bool bPostEditMessageBlocked:1;
	bool bToolbarControl:1;
	bool bChatboxControl:1;
	bool bLockedTextControl:1;
	bool bTransparent:1;
	bool bDisableInput:1;
	BOOL bUseShapeFile;

	//
	// caret blinking
	//
	bool bBlinkOn;
	S32  blinkCount;

	U8	 colorState;			// which color to display for text
	
	//
	// data needed for maintaining highlight state
	//
	S32 firstChar, lastChar;		// first<0 means no highlight, last can be less than first
	U16 emptyTextWidth;
	U8 charWidths[DEF_MAX_EDIT_CHARS];
	S32 caretChar;
	S32 firstDisplayChar;			// non-zero if text is longer than display area

	S32 lastX;

	//
	// class methods
	//

	Edit2 (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		colorState = GTETXT_MAX_STATES;		// invalid state
		MAX_EDIT_CHARS = DEF_MAX_EDIT_CHARS;
	}

	virtual ~Edit2 (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEdit2 methods  */

	virtual void InitEdit (const EDIT_DATA & data, BaseHotRect * parent); 

	virtual void EnableEdit (bool bEnable);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void)
	{
		return !bInvisible;
	}

	virtual void SetText (const wchar_t * szText, S32 firstHighlightChar=-1);

	virtual int  GetText (wchar_t * szBuffer, int bufferSize);

	virtual void SetControlID (U32 id);

	virtual void SetMaxChars (U32 maxChars);		// allow user to enter maxChars-1

	virtual void EnableToolbarBehavior (void);			// edit control in the toolbar

	virtual void EnableChatboxBehavior (void);			// edit control is of the chatbox varienty

	virtual void EnableLockedTextBehavior (void);		// edit control is of the chatbox varienty

	virtual void DisableInput (bool _bDisableInput)
	{
		bDisableInput = _bDisableInput;
	}

	virtual void SetTransparentBehavior (bool _bTransparent)
	{
		bTransparent = _bTransparent;
	}

	virtual void SetIgnoreChars (wchar_t * ignoreChars)	// don't allow ignore characters to show up in edit control
	{
		if (ignoreChars != NULL)
		{
			wcsncpy(szIgnoreChars, ignoreChars, sizeof(szIgnoreChars) / sizeof(wchar_t));
		}
		else
		{
			szIgnoreChars[0] = 0;
		}
	}

	virtual const U32 GetEditWidth (void) const
	{
		return width;
	}

	virtual bool IsTextAllVisible (void)
	{
		U32 textWidth = 0;
		U32 index = 0;
		while(szEditText[index])
		{
			textWidth += charWidths[index];
			++index;
		}
		return (textWidth < ((U32)width));
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

	/* Edit2 methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (EDITTYPE * _pEditType);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDblClick (int x, int y)
	{
		if (bDisabled == false)
		{
			bMousePressed = false;
	
			// find a space character to the left and to the right of the postion and highlite the whole damn thing
			caretChar = getCharPos(x-screenRect.left);

			if (szEditText[caretChar] == L' ')
			{
				return;
			}

			while (caretChar > 0)
			{
				if (szEditText[caretChar] == L' ')
				{
					caretChar++;
					break;
				}
				caretChar--;
			}

			firstChar = caretChar;
			caretChar = getCharPos(x-screenRect.left);

			bool bFoundSpace = false;
			while (caretChar < (int)wcslen(szEditText))
			{
				if (bFoundSpace)
				{
					if (szEditText[caretChar] != L' ')
					{
						break;
					}
				}
				else
				{
					if (szEditText[caretChar] == L' ')
					{
						bFoundSpace = true;
					}
				}
				caretChar++;
			}
			
			lastChar = caretChar;

			if (caretChar < (int)wcslen(szEditText))
			{
				lastChar--;
			}
		}
	}

	void onLeftButtonDown (int x, int y)
	{
		if (bDisabled == false && bKeyboardFocus)
		{
			if ((bMousePressed = bAlert) != 0)
			{
				// move the caret, reset the highlight area
				caretChar = getCharPos(x-screenRect.left);
				firstChar = lastChar = -1;
				lastX = x;
			}
		}
	}

	void onLeftButtonUp (int x, int y)
	{
		if (bDisabled == false)
		{
			bMousePressed = false;

			if (bKeyboardFocus && bToolbarControl && bAlert==0)
				doAction();
		}
	}
	
	void onMouseMove (int x, int y)
	{
		if (bMousePressed && x!=lastX)
		{
			// move the caret, reset the highlight area
			lastChar = getCharPos(x-screenRect.left);
			if (firstChar < 0)
				firstChar = lastChar;
			caretChar = lastChar;
			if (lastChar >= firstChar)
				caretChar++;
			lastX = x;
		}
	}

	// x relative to screenRect
	// convert screen point into character index
	int getCharPos (int x)
	{
		int result=firstDisplayChar, sum=0;

		x -= xText;

		while (szEditText[result])
		{
			sum += charWidths[result];
			if (x < sum)
				break;
			result++;
		}

		return result;
	}

	// get left edge of character position
	// convert index into screen position
	// returns 
	int getGuardedIndexPos (int index)
	{
		U32 result = getIndexPos(index);
		result = __min(result, U32(width-xText-xText));
		return result;
	}

		// get left edge of character position
	// convert index into screen position
	// returns 
	int getIndexPos (int index)
	{
		int result = 0;

		if ((index -= firstDisplayChar) < 0)
			index = 0;
		while (index-- > 0)
			result += charWidths[index+firstDisplayChar];

		return result;
	}

	void doAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			parent->PostMessage(CQE_BUTTON, (void*)controlID);
			bPostMessageBlocked = true;
			if (bToolbarControl)
			{
				parent->PostMessage(CQE_LHOTBUTTON, (void*)controlID);
				parent->PostMessage(CQE_KEYBOARD_FOCUS, 0);		// undo focus
			}
		}
	}

	void onEditChange (void)
	{
		if (bPostEditMessageBlocked == false)
		{
			bPostEditMessageBlocked = true;
			parent->PostMessage(CQE_EDIT_CHANGED, (void*)controlID);
		}
	}

	void onCharPressed (const wchar_t letter)
	{
		switch (letter)
		{
		case 13:		// return
		case '\t':		// tab
		case 27:		// ESC
			break;

		case 8:			// backspace
			if (firstChar < 0)
			{ 
				// highlight character to left of caret
				if (caretChar > 0)
				{
					firstChar = lastChar = caretChar-1;
				}
			}
			flushHighlight();
			break;

		default:
			if (isIgnorable(letter) == false)
			{
				flushHighlight();
				addChar(letter);
				ensureCaretVisible();
			}
			else
			{
				// make a beep or something here
			}
			break;
		}
	}

	void setColorState (U32 state)
	{
		U8 newState;

		switch (state)
		{
		case GTESHP_DISABLED:
			newState = GTETXT_DISABLED;
			break;
		case GTESHP_MOUSE_FOCUS:
			newState = GTETXT_HIGHLIGHT;
			break;

		case GTESHP_NORMAL:
		default:
			newState = GTETXT_NORMAL;
			break;
		}

		if (newState != colorState)
		{
			colorState = newState;
			const GT_EDIT::COLOR & color = pEditType->colors[newState];

			pFont->SetFontColor(RGB(color.red, color.green, color.blue) | 0xFF000000, 0);
		}
	}

	// add character at caret, remove highlight, move other letters over (insertion)
	bool addChar (const wchar_t letter);

	void highlightAll (void);

	void flushHighlight (void);

	bool onKeyPressed (int vKey);

	//void onCharPressed (int uniChar);

	void ensureCaretVisible (void);

	void onUpdate (U32 dt)
	{
		blinkCount -= dt;
		if (blinkCount < 0)
		{
			bBlinkOn = !bBlinkOn;
			blinkCount = (bBlinkOn) ? pEditType->blinkOnTime : pEditType->blinkOffTime;
		}
	}

	bool isIgnorable (const wchar_t letter)
	{
		if (szIgnoreChars[0])
		{
			if (wcspbrk(&letter, szIgnoreChars) != NULL)
				return true;

			// also, if we are inputting the first character, make sure it is not space
			if (caretChar == 0 && letter == L' ')
				return true;
		}
		return false;
	}
};
//--------------------------------------------------------------------------//
//
Edit2::~Edit2 (void)
{
	GENDATA->Release(pEditType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void Edit2::InitEdit (const EDIT_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
	}

	if ((editText = data.editText) != 0)
		SetText(_localLoadStringW(editText));

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top +  pEditType->height - 1;
	screenRect.right = screenRect.left +  pEditType->width - 1;

	width = pEditType->width;
	height = pEditType->height;

	// are we using an art file for the dimension data?
	bUseShapeFile = (pEditType->shapes[0] != NULL);

	//
	// calculate text offset (left justify)
	//

	yText = S32(pEditType->height - pFont->GetFontHeight()) / 2;
	xText = IDEAL2REALX(2 + pEditType->justify);
	emptyTextWidth = pFont->GetCharWidth(' ') * 3;
	emptyTextWidth = __min(emptyTextWidth, S32(pEditType->width)-xText-xText);

	if (controlID == 0)
		controlID = editText;
}
//--------------------------------------------------------------------------//
//
void Edit2::EnableEdit (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bMousePressed = false;
	}
}
//--------------------------------------------------------------------------//
//
void Edit2::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
bool Edit2::SetKeyboardFocus (bool bEnable)
{
	if (bEnable && (bDisabled||bInvisible||bDisableInput))
		return false;

	if (bEnable != bKeyboardFocus)  // if we are changing state
	{
		if ((bKeyboardFocus = bEnable) != 0)
		{
			highlightAll();
			if (bToolbarControl)
			{
				EVENTSYS->Send(CQE_KILL_FOCUS, static_cast<BaseHotRect *>(this));
				HOTKEY->Disable();
				DBHOTKEY->Disable();
		
				if (bChatboxControl)
				{
					statusTextID = IDS_ENTERCHATTEXT;
				}
				else
				{
					statusTextID = IDS_ENTERSHIPNAME;
				}
				cursorID = IDC_CURSOR_ARROW;
				desiredOwnedFlags = RF_CURSOR | RF_STATUS;
				grabAllResources();
			}
			if (CQFLAGS.b3DEnabled==0)
			{
				//
				// move composition window next to our rect, set the font
				//
				if (hIMC)
				{
					COMPOSITIONFORM form;

					form.dwStyle = CFS_FORCE_POSITION;
					form.ptCurrentPos.x = screenRect.left;
					form.ptCurrentPos.y = screenRect.top-((pFont->GetFontHeight()*3)/2);
					if (form.ptCurrentPos.y < 0)
						form.ptCurrentPos.y = screenRect.top+((pFont->GetFontHeight()*3)*2);

					ImmAssociateContext(hMainWindow, hIMC);
					ImmSetCompositionFont(hIMC, &pEditType->logFont);
					ImmSetCompositionWindow(hIMC, &form);
				}
			}
		}
		else
		{
			if (CQFLAGS.b3DEnabled==0)
			{
				ImmAssociateContext(hMainWindow, 0);
			}
			if (bToolbarControl)
			{
				EVENTSYS->Send(CQE_SET_FOCUS, static_cast<BaseHotRect *>(this));
				HOTKEY->Enable();
				DBHOTKEY->Enable();
				desiredOwnedFlags = 0;
				releaseResources();
			}
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
U32 Edit2::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT Edit2::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_UPDATE:
		bPostMessageBlocked = false;
		bPostEditMessageBlocked = false;
		onUpdate(U32(param)>>10);
		break;

	case CQE_ENDFRAME:
		draw();
		break;

	case WM_LBUTTONDBLCLK:
		if (!bDisableInput)
		{
			onLeftButtonDblClick(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
		}
		break;

	case WM_LBUTTONDOWN:
		if (!bDisableInput)
		{
			onLeftButtonDown(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
		}
		break;

	case WM_LBUTTONUP:
		if (!bDisableInput)
		{
			onLeftButtonUp(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
		}
		break;

	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);
		if (!bDisableInput)
		{
			onMouseMove(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
		}
		return GR_OK;

	case WM_KEYDOWN:
		if (bKeyboardFocus && !bDisableInput)
		{
			if (onKeyPressed(msg->wParam))
			{
				// we want to eat the message
				return GR_GENERIC;
			}
		}
		break;

	case CQE_UNICHAR:
		if (bKeyboardFocus && !bDisableInput && bHasFocus)
		{
			// eat the message when you're done with it
			onCharPressed(U16(param));
			return GR_GENERIC;
		}
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Edit2::draw (void)
{
	U32 state;
	
	if (bDisabled)
	{
		state = GTESHP_DISABLED;
	}
	else
	{
		state = GTESHP_NORMAL;
		if (bAlert)
			state = GTESHP_MOUSE_FOCUS;
	}

	PANE pane;
	pane.window = 0;
	pane.x0 = screenRect.left;
	pane.y0 = screenRect.top;
	pane.x1 = screenRect.right;
	pane.y1 = screenRect.bottom;


	setColorState(state);

	if (bUseShapeFile)
	{
		pEditType->shapes[state]->Draw(0, screenRect.left, screenRect.top);
	}
	else if (!bTransparent)
	{
		DA::RectangleFill(&pane, 0, 0, width, height, RGB(0,0,0));

		int x1 = 0, y1 = 0;
		int x2 = pane.x1 - pane.x0 - 1;//screenRect.right - screenRect.left;
		int y2 = pane.y1 - pane.y0 - 1;//screenRect.bottom - screenRect.top;

		DA::LineDraw(&pane, x1, y1, x2, y1, RGB(100,100,100));
		DA::LineDraw(&pane, x2, y1, x2, y2, RGB(100,100,100));
		DA::LineDraw(&pane, x1, y2, x2, y2, RGB(100,100,100));
		DA::LineDraw(&pane, x1, y1, x1, y2, RGB(100,100,100));

	}

	if (bKeyboardFocus && bHasFocus)
	{
		if (bUseShapeFile)
		{
			pEditType->shapes[GTESHP_KEYB_FOCUS]->Draw(0, screenRect.left, screenRect.top);
		}
		else if (!bTransparent)
		{
			int x1 = 0, y1 = 0;
			int x2 = screenRect.right - screenRect.left - 1;
			int y2 = screenRect.bottom - screenRect.top - 1;

			DA::LineDraw(&pane, x1, y1, x2, y1, RGB(255,100,100));
			DA::LineDraw(&pane, x2, y1, x2, y2, RGB(255,100,100));
			DA::LineDraw(&pane, x1, y2, x2, y2, RGB(255,100,100));
			DA::LineDraw(&pane, x1, y1, x1, y2, RGB(255,100,100));
		}

		//	highlight text
		if (firstChar >= 0)
		{
			const GT_EDIT::COLOR & color = pEditType->colors[GTETXT_SELECTED];
			int start = __min(firstChar, lastChar);
			int end   = __max(firstChar, lastChar);
			DA::RectangleFill(&pane, xText+getGuardedIndexPos(start), yText, xText+getGuardedIndexPos(end+1)-1, 
				              yText+pFont->GetFontHeight(), RGB(color.red, color.green, color.blue));
		}
		else if (szEditText[0] == 0)
		{
			const GT_EDIT::COLOR & color = pEditType->colors[GTETXT_SELECTED];
			DA::RectangleFill(&pane, xText+0, yText, emptyTextWidth-1, yText+pFont->GetFontHeight(), RGB(color.red, color.green, color.blue));
		}

		if (bBlinkOn)
		{
			const GT_EDIT::COLOR & color = pEditType->colors[GTETXT_CARET];
			DA::RectangleFill(&pane, xText+getGuardedIndexPos(caretChar)-1, yText, xText+getGuardedIndexPos(caretChar)-1, yText+pFont->GetFontHeight(), RGB(color.red, color.green, color.blue));
		}
	}

	pFont->StringDraw(&pane, xText, yText, szEditText+firstDisplayChar);
}
//--------------------------------------------------------------------------//
// set a whole new string
//
void Edit2::SetText (const wchar_t * string, S32 firstHighlightChar)
{
	U32 len = wcslen(string);
	U32 i;

	len = __min(len, MAX_EDIT_CHARS-1);
	wcsncpy(szEditText, string, MAX_EDIT_CHARS-1);
	firstDisplayChar = 0;

	if (len && firstHighlightChar<S32(len))
	{
		firstChar = (firstHighlightChar>=0) ? firstHighlightChar : 0;
		lastChar = len-1;
	}
	else
	{
		firstChar=lastChar=-1;
	}

	caretChar = (firstHighlightChar>=0) ? firstHighlightChar : len;

	for (i = 0; i < len; i++)
		charWidths[i] = pFont->GetCharWidth(string[i]);
	charWidths[i] = 0;

	ensureCaretVisible();
}
//--------------------------------------------------------------------------//
//
int Edit2::GetText (wchar_t * szBuffer, int bufferSize)
{
	if (szBuffer==0)
	{
		return wcslen(szEditText);
	}
	else
	{
		bufferSize /= sizeof(wchar_t);
		int len = wcslen(szEditText);

		len = __min(len+1, bufferSize);
		memcpy(szBuffer, szEditText, len*sizeof(wchar_t));

		return len-1;
	}
}
//--------------------------------------------------------------------------//
//
void Edit2::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
void Edit2::SetMaxChars (U32 maxChars)
{
	CQASSERT(maxChars <= DEF_MAX_EDIT_CHARS);
	MAX_EDIT_CHARS = maxChars;
}
//--------------------------------------------------------------------------//
//
void Edit2::EnableToolbarBehavior (void)
{
	bToolbarControl = true;
}
//--------------------------------------------------------------------------//
//
void Edit2::EnableChatboxBehavior (void)
{
	bChatboxControl = true;
}
//--------------------------------------------------------------------------//
//
void Edit2::EnableLockedTextBehavior (void)
{
	bLockedTextControl = true;
}
//--------------------------------------------------------------------------//
// add character at caret (insertion)
//
bool Edit2::addChar (const wchar_t letter)
{
	// ignore control characters
	if (letter < 31)
		return false;

	wchar_t text[DEF_MAX_EDIT_CHARS];
	U8 widths[DEF_MAX_EDIT_CHARS];
	U8 letterWidth = pFont->GetCharWidth(letter);
	int len = wcslen(szEditText);

	if (U32(caretChar) >= MAX_EDIT_CHARS-1 || (bOverwriteMode==false && U32(len) >= MAX_EDIT_CHARS-1))
	{
		// discard letter, might want a beep here!
		return false;
	}

	// check if the character belongs to our ignore list (if there is a list)
	if (isIgnorable(letter) == true)
	{
		return false;
	}


	memcpy(text, szEditText, sizeof(text));
	memcpy(widths, charWidths, sizeof(widths));

	szEditText[caretChar] = letter;
	charWidths[caretChar] = letterWidth;

	caretChar++;

	if (bOverwriteMode == false)
	{
		memcpy(szEditText+caretChar, text+caretChar-1, sizeof(text)-((caretChar+1)*2));
		memcpy(charWidths+caretChar, widths+caretChar-1, sizeof(widths)-(caretChar+1));
	}
	onEditChange();
	return true;
}
//--------------------------------------------------------------------------//
// remove highlight, move other letters over 
//
void Edit2::flushHighlight (void)
{
	if (firstChar < 0)
		return;	// nothing to do

	int num = lastChar - firstChar;

	if (num >= 0)
	{
		memcpy(szEditText+firstChar, szEditText+lastChar+1, sizeof(szEditText)-((lastChar+1)*2));
		memcpy(charWidths+firstChar, charWidths+lastChar+1, sizeof(charWidths)-(lastChar+1));
		caretChar = firstChar;
	}
	else	// num < 0
	{
		memcpy(szEditText+lastChar, szEditText+firstChar+1, sizeof(szEditText)-((firstChar+1)*2));
		memcpy(charWidths+lastChar, charWidths+firstChar+1, sizeof(charWidths)-(firstChar+1));
		caretChar = lastChar;
	}

	lastChar = firstChar = -1;
	ensureCaretVisible();
	onEditChange();  //don't send this so that combo won't re-insert the same highlighted text
}
//--------------------------------------------------------------------------//
//
void Edit2::highlightAll (void)
{
	U32 len = wcslen(szEditText);

	if (len)
	{
		firstChar = 0;
		lastChar = len-1;
	}
	else
	{
		firstChar=lastChar=-1;
	}

	caretChar = len;
	ensureCaretVisible();
}
//--------------------------------------------------------------------------//
//
//	return true if we want to eat the message
bool Edit2::onKeyPressed (int vKey)
{
	switch (vKey)
	{
	case VK_ESCAPE:
		if (bToolbarControl)
		{
			parent->PostMessage(CQE_RHOTBUTTON, (void*)controlID);
			parent->PostMessage(CQE_KEYBOARD_FOCUS, 0);		// undo focus
		}
		return true;
	
	case VK_RETURN:
		doAction();
		return true;

	case VK_RIGHT:
		if (szEditText[caretChar])
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				if (firstChar<0)
				{
					lastChar=firstChar=caretChar;
					caretChar++;
				}
				else
				{
					if (caretChar==firstChar)
					{
						lastChar = firstChar = -1;
					}
					else
					{
						lastChar++;
					}
					caretChar++;
				}
			}
			else
			if (firstChar>=0)	// if highlighted text
			{
				lastChar = firstChar = -1;
			}
			else
				caretChar++;
			ensureCaretVisible();
		}
		else //already at the end of the line
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				// nothing to do?
			}
			else
				lastChar = firstChar = -1;
		}
		return true;

	case VK_END:
		if (szEditText[caretChar])
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				if (firstChar<0)	// no highlighted text?
				{
					firstChar = caretChar;
					caretChar = wcslen(szEditText);
					lastChar = caretChar-1;
				}
				else
				{
					if (caretChar > firstChar)
					{
						caretChar = wcslen(szEditText);
						lastChar = caretChar-1;
					}
					else
					{
						firstChar++;
						caretChar = wcslen(szEditText);
						if (caretChar == firstChar)	// at the end of the line
						{
							firstChar=lastChar=-1;
						}
						else
						{
							lastChar=caretChar-1;
						}
					}
				}
			}
			else
			if (firstChar>=0)	// if highlighted text
			{
				lastChar = firstChar = -1;
				caretChar = wcslen(szEditText);
			}
			else
				caretChar = wcslen(szEditText);
			ensureCaretVisible();
		}
		else //already at the end of the line
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				// nothing to do?
			}
			else
				lastChar = firstChar = -1;
		}
		return true;

	case VK_HOME:
		if (caretChar > 0)
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				if (firstChar<0)
				{
					if (caretChar>0)
					{
						firstChar=caretChar-1;
						lastChar=caretChar=0;
					}
					else
					{
						lastChar=firstChar=-1;
						caretChar=0;
					}
				}
				else
				{
					if (caretChar > firstChar)
					{
						if (firstChar>0)
						{
							firstChar--;
							lastChar=caretChar=0;
						}
						else
						{
							firstChar=lastChar=-1;
							caretChar=0;
						}
					}
					else
						lastChar=caretChar=0;
				}
			}
			else
			if (firstChar>=0)	// if highlighted text
			{
				lastChar = firstChar = -1;
				caretChar = 0;
			}
			else
				caretChar = 0;
			ensureCaretVisible();
		}
		else //already at the end of the line
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				// nothing to do?
			}
			else
				lastChar = firstChar = -1;
		}
		return true;

	case VK_LEFT:
		if (caretChar > 0)
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				caretChar--;
				if (firstChar<0)
				{
					lastChar=firstChar=caretChar;
				}
				else
				if (caretChar==firstChar)
				{
					lastChar = firstChar = -1;
				}
				else
				{
					lastChar--;
				}
			}
			else
			if (firstChar>=0)	// if highlighted text
			{
				lastChar = firstChar = -1;
			}
			else
				caretChar--;
			ensureCaretVisible();
		}
		else //already at the end of the line
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				// nothing to do?
			}
			else
				lastChar = firstChar = -1;
		}
		break;


	case VK_DELETE:
		if (firstChar >= 0)		// if highlighted text
		{
			if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
			{
				// copy highlighted text to scratch area				
				int start = __min(firstChar, lastChar);
				int end   = __max(firstChar, lastChar);

				int i;
				for (i=0; start <= end; i++, start++)
					szScratchText[i] = szEditText[start];
				szScratchText[i] = 0;
			}
		}
		else
			firstChar = lastChar = caretChar;
		flushHighlight();
		break;

	case VK_INSERT:
		if (HOTKEY->GetVkeyState(VK_LSHIFT) || HOTKEY->GetVkeyState(VK_RSHIFT))
		{
			flushHighlight();
			for (int i=0; szScratchText[i]; i++)
				addChar(szScratchText[i]);
			ensureCaretVisible();
		}
		else
		if (HOTKEY->GetVkeyState(VK_LCONTROL) || HOTKEY->GetVkeyState(VK_RCONTROL))
		{
			// copy highlighted text to scratch area				
			int start = __min(firstChar, lastChar);
			int end   = __max(firstChar, lastChar);

			int i;
			for (i=0; start <= end; i++, start++)
				szScratchText[i] = szEditText[start];
			szScratchText[i] = 0;
		}
		else
			bOverwriteMode = !bOverwriteMode;
		break;

	default:
		return false;
	}

	return true;
}
//--------------------------------------------------------------------------//
//
void Edit2::ensureCaretVisible (void)
{
	//
	// easy case: caret is to the left of visible range
	//
	if (caretChar <= firstDisplayChar)
	{
		firstDisplayChar = caretChar;
	}
	else	// we may have to shift to the right
	{
		const int mwidth = width-xText-xText;
		int pos = getIndexPos(caretChar);

		if (bLockedTextControl && pos > mwidth)
		{
			// we don't want to shift if we have Locked Text Behavior enabled!!

			// erase the character we just added
			firstChar = caretChar-1;
			lastChar = caretChar;		
			flushHighlight();
			return;
		}

		while (pos > mwidth)  // we have to shift
		{
			pos -= charWidths[firstDisplayChar++];
		}
	}
}
//--------------------------------------------------------------------------//
//
void Edit2::init (EDITTYPE * _pEditType)
{
	COMPTR<IDAComponent> pBase;
	pEditType = _pEditType;

	GENDATA->CreateInstance(pEditType->pFontType, pBase);
	pBase->QueryInterface("IFontDrawAgent", pFont);
}

//--------------------------------------------------------------------------//
//-------------------------Edit Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE EditFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(EditFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	EditFactory (void) { }

	~EditFactory (void);

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
EditFactory::~EditFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void EditFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE EditFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_EDIT)
	{
		GT_EDIT * data = (GT_EDIT *) _data;
		EDITTYPE * result = new EDITTYPE;

		result->pArchetype = pArchetype;
		result->pFontType = GENDATA->LoadArchetype(data->fontName);
		CQASSERT(result->pFontType);
		GENDATA->AddRef(result->pFontType);
		// 
		// get logFont info
		//
 		GT_FONT * pFont = (GT_FONT *) GENDATA->GetArchetypeData(result->pFontType);
		CQCreateLogFont(pFont->font, result->logFont, pFont->bNotScaling);

		result->colors[GTETXT_DISABLED] = data->disabledText;
		result->colors[GTETXT_HIGHLIGHT] = data->highlightText;
		result->colors[GTETXT_NORMAL] = data->normalText;
		result->colors[GTETXT_SELECTED] = data->selectedText;
		result->colors[GTETXT_CARET] = data->caret;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				int i;
				for (i = 0; i < GTESHP_MAX_SHAPES; i++)
					CreateDrawAgent((VFX_SHAPETABLE *) pImage, i, result->shapes[i]);
			END_MAPPING(INTERFACEDIR);

			result->shapes[0]->GetDimensions(result->width, result->height);
		}
		else
		{
			result->width = IDEAL2REALX(data->width);
			result->height = IDEAL2REALY(data->height);
		}

		result->justify = data->justify;
		result->blinkOnTime = result->blinkOffTime = GetCaretBlinkTime();
		
		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 EditFactory::DestroyArchetype (HANDLE hArchetype)
{
	EDITTYPE * type = (EDITTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT EditFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	EDITTYPE * type = (EDITTYPE *) hArchetype;
	Edit2 * result = new DAComponent<Edit2>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _editfactory : GlobalComponent
{
	EditFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<EditFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _editfactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Edit2.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//
