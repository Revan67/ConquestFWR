f//--------------------------------------------------------------------------//
//                                                                          //
//                               Listbox.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Listbox.cpp 65    9/14/00 5:50p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IListbox.h"
#include "IScrollBar.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DListbox.h>
#include <DScrollBar.h>
#include "DrawAgent.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#include <stdlib.h>

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

//--------------------------------------------------------------------------//
//
struct LISTBOXTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	PGENTYPE pScrollBarType;
	COMPTR<IDrawAgent> shape;
	U16 width, height;		// shape bounds

	GT_LISTBOX::COLOR colors[GTLBTXT_MAX_STATES];

	LISTBOXTYPE (void)
	{
	}

	~LISTBOXTYPE (void)
	{
		if (pFontType)
			GENDATA->Release(pFontType);
		if (pScrollBarType)
			GENDATA->Release(pScrollBarType);
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
struct LISTITEM
{
	LISTITEM *	pNext;
	U32			userData;
	COLORREF	color;
	wchar_t *	szString;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~LISTITEM (void)
	{
		free(szString);
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Listbox : BaseHotRect, IListbox, IKeyboardFocus, IScrollBarOwner
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Listbox)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IListbox)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IScrollBarOwner)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	LISTBOXTYPE * pListboxType;
	COMPTR<IFontDrawAgent> * ppFont;		// variable array of fontDrawAgents
	COMPTR<IScrollBar> pScrollBar;
	RECT textArea;
	int  leadingHeight, fontHeight;

	LISTITEM * pList, * pEndList;
	S32 selectedItem;	// index of highlighted item
	U32 numItems;
	U32 textLines, pageLines;		// max number of lines of text, # lines to move on a page command
	S32 topLine;		// index of first line
	S32 mouseHighlight;		// index ( -1 to textLines-1 )  box where mouse is inside, -1 = not inside any box

	bool bKeyboardFocus;
	bool bStatic;
	bool bSingleClick;
	bool bPostMessageBlocked;		// limit posting to once per update
	bool bScrollVisible;
	bool bSolidBackground;
	bool bDisabled;
	bool bNoBorder;
	bool bDisableMouseSelect;
	
	U32 controlID;

	//
	// class methods
	//

	Listbox (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		selectedItem = -1;
	}

	virtual ~Listbox (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IListbox methods  */

	virtual void InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent); 

	virtual void EnableListbox (bool bEnable);

	virtual void SetVisible (bool bVisible);

	virtual S32 AddStringToHead (const wchar_t * szString);

	virtual S32 AddString (const wchar_t * szString);	// returns index

	virtual S32 FindString (const wchar_t * szString);	// returns index of matching string prefix

	virtual S32 FindStringExact (const wchar_t * szString);	// returns index of matching string

	virtual void RemoveString (S32 index);	

	virtual U32 GetString (S32 index, wchar_t * buffer, U32 bufferSize);	// returns number of characters written

	virtual S32 SetString (S32 index, const wchar_t * szString);	// changes string value, returns index

	virtual void SetDataValue (S32 index, U32 data);	// set user defined data item

	virtual U32 GetDataValue (S32 index);	// return user defined value, 0 on error

	virtual void SetColorValue (S32 index, COLORREF color);		// set color of text for item

	virtual COLORREF GetColorValue (S32 index);
	
	virtual S32 GetCurrentSelection (void);		// returns index
		
	virtual S32 SetCurrentSelection (S32 newIndex);		// returns index

	virtual S32 GetCaretPosition (void);		// returns index
		
	virtual S32 SetCaretPosition (S32 newIndex);		// returns old caret index

	virtual void ResetContent (void);		// remove all items from list

	virtual U32 GetNumberOfItems (void);

	virtual S32 GetTopVisibleString (void);
	
	virtual S32 GetBottomVisibleString (void);

	virtual void EnsureVisible (S32 index);		// make sure a string is visible

	virtual void ScrollPageUp (void); //old?

	virtual void ScrollPageDown (void);//old?

	virtual void ScrollLineUp (void);//old?

	virtual void ScrollLineDown (void);//old?

	virtual void ScrollHome (void);//old?

	virtual void ScrollEnd (void);//old?

	virtual void CaretPageUp (void);

	virtual void CaretPageDown (void);

	virtual void CaretLineUp (void);

	virtual void CaretLineDown (void);

	virtual void CaretHome (void);

	virtual void CaretEnd (void);

	virtual void SetControlID (U32 id);

	virtual	S32 GetBreakIndex(const wchar_t * szString);

	virtual const bool IsMouseOver (S16 x, S16 y) const;

	/* IKeyboardFocus methods */

	virtual bool SetKeyboardFocus (bool bEnable);

	virtual U32 GetControlID (void);

	
	/* IScrollBar methods */

	virtual void ScrollPageUp (U32 scrollId); 

	virtual void ScrollPageDown (U32 scrollId);

	virtual void ScrollLineUp (U32 scrollId);

	virtual void ScrollLineDown (U32 scrollId);

	virtual void SetScrollPosition (U32 scrollId,S32 scrollPosition);

	/* BaseHotRect methods */

	virtual void onRequestKeyboardFocus (int x, int y)
	{
		if (bStatic==false)
		{
			//
			// if mouse is within text bounds
			//
			if (pListboxType->shape)
				BaseHotRect::onRequestKeyboardFocus(x, y);
			else
			{
				x -= screenRect.left;
				y -= screenRect.top;
				if (x >= textArea.left && x <= textArea.right && y >= textArea.top && y <= textArea.bottom)
					BaseHotRect::onRequestKeyboardFocus(x, y);
			}
		}
	}


	/* Listbox methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (LISTBOXTYPE * _pListboxType);

	void draw (void);

	LISTITEM * getItem (S32 index);

	void drawListRect (bool bDebug);

	bool onKeyPressed (int vKey);

	void onMouseMove (S32 x, S32 y);

	void onMouseWheel (S32 zDelta);

	void onLeftButtonDown (S32 x, S32 y)
	{
		if (bHasFocus && bDisabled == false)
		{
			if (U32(mouseHighlight) < textLines)
			{
				selectedItem = topLine + mouseHighlight;
				notifyCaretPos();
			}
		}
	}

	void onLeftButtonUp (S32 x, S32 y)
	{
		if (bDisabled == false)
		{
			// make sure we are within the listbox
			POINT pt = { x , y};
			if (!PtInRect(&screenRect, pt))
			{
				return;
			}

			if (bHasFocus)
			{
				if (U32(mouseHighlight) < textLines)
				{
					if (bSingleClick)
					{
						doAction();
					}
				}
			}
		}
	}

	void doAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			parent->PostMessage(CQE_LIST_SELECTION, (void*)controlID);
			bPostMessageBlocked=true;
		}
	}
	
	void notifyCaretPos (void)
	{
		if (parent) 
			parent->PostMessage(CQE_LIST_CARET_MOVED, (void*)controlID);
	}

	virtual void DrawRect (void)
	{
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Listbox::~Listbox (void)
{
	ResetContent();
	GENDATA->Release(pListboxType->pArchetype);
	if (ppFont)
	{
		delete [] ppFont;
		ppFont = NULL;
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::InitListbox (const LISTBOX_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, EVENT_PRIORITY_HOTRECT);		// set to low priority	
	}

	//
	// calculate text area
	//
	textArea.left   = IDEAL2REALX(data.textArea.left);
	textArea.right  = IDEAL2REALX(data.textArea.right+1)-1;
	textArea.top    = IDEAL2REALY(data.textArea.top);
	textArea.bottom = IDEAL2REALY(data.textArea.bottom+1)-1;


	if (textArea.right == 0)
		textArea.right = pListboxType->width + textArea.left - 1;
	if (textArea.bottom == 0)
		textArea.bottom = pListboxType->height + textArea.top - 1;

	//
	// calculate screen rect. If no shape, rect == text area
	//

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;

	if (pListboxType->height)
		screenRect.bottom = screenRect.top + pListboxType->height - 1;
	else
		screenRect.bottom = screenRect.top + textArea.bottom;

	if (pListboxType->width)
		screenRect.right = screenRect.left + pListboxType->width - 1;
	else
		screenRect.right = screenRect.left + textArea.right;

	leadingHeight = IDEAL2REALY(data.leadingHeight);			// extra space between lines

	//
	// calculate number of lines of text we will have
	//
	COMPTR<IDAComponent> pBase;
	COMPTR<IFontDrawAgent> pFont;

	if (ppFont!=0 && ppFont[0]!=0)
	{
		pFont = ppFont[0];
	}
	else
	{
		GENDATA->CreateInstance(pListboxType->pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
	}
	fontHeight = pFont->GetFontHeight();

	U32 tHeight = fontHeight + leadingHeight;
	U32 totalHeight = textArea.bottom - textArea.top + 1;
	U32 rem;

	textLines = totalHeight / tHeight;
	rem   = totalHeight % tHeight;

	if (rem >= U32(fontHeight))
		textLines++;

	if (textLines < 4)
		pageLines = textLines;
	else
		pageLines = textLines - 2;

	if (ppFont)
	{
		delete [] ppFont;
		ppFont = 0;
	}

	if (textLines>0 && pListboxType->pFontType)
	{
		ppFont = new COMPTR<IFontDrawAgent>[textLines];
		ppFont[0] = pFont;

		for (U32 i = 1; i < textLines; i++)
		{
			pFont->CreateDuplicate(ppFont[i]);
		}
	}

	bStatic = data.bStatic;
	bSingleClick = data.bSingleClick;
	bScrollVisible = data.bScrollbar;
	bSolidBackground = data.bSolidBackground;
	bNoBorder = data.bNoBorder;
	bDisableMouseSelect = data.bDisableMouseSelect;

	mouseHighlight = -1;

	if (pListboxType->pScrollBarType)
	{
		if (pScrollBar == 0)
		{
			GENDATA->CreateInstance(pListboxType->pScrollBarType, pBase);
			pBase->QueryInterface("IScrollBar", pScrollBar);
		}

		SCROLLBAR_DATA dumbData;
		
		pScrollBar->InitScrollBar(dumbData,this);
		pScrollBar->SetScrollRange(numItems);
		pScrollBar->SetViewRange(textLines);
		pScrollBar->SetScrollPosition(0);
		pScrollBar->SetVisible(bScrollVisible && numItems>textLines);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::EnableListbox (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
		bDisabled = !bEnable;
	}

	if (pScrollBar != NULL)
	{
		pScrollBar->EnableScrollBar(bEnable);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
	if(pScrollBar != 0)
	{
		if (bScrollVisible && bVisible)
		{
			pScrollBar->SetScrollRange(numItems);
		}

		pScrollBar->SetVisible(bScrollVisible&&bVisible&&numItems>textLines);
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT Listbox::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_UPDATE:
		bPostMessageBlocked = false;
		break;

	case CQE_ENDFRAME:
		draw();
		break;

	case WM_KEYDOWN:
		if (bKeyboardFocus && bHasFocus && bDisabled == false)
		{
			if (onKeyPressed(msg->wParam))
			{
				// we want to eat the message
				return GR_GENERIC;
			}
		}
		break;

	case WM_MOUSEWHEEL:
		if (bKeyboardFocus && bHasFocus && bDisabled == false)
		{
			onMouseWheel(short(HIWORD(msg->wParam)));
			if (bAlert)
				return GR_GENERIC;		// eat the event
		}
		break;

	case WM_MOUSEMOVE:
		if (bDisabled == false)
		{
			GENRESULT result = BaseHotRect::Notify(message, param);
			if (result == GR_OK)
			{
				onMouseMove(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
			}
		}
		return GR_OK;

	case WM_LBUTTONUP:
		if (bDisabled == false)
		{
			if (BaseHotRect::Notify(message, param) == GR_OK)
			{
				onLeftButtonUp(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
				if (bAlert)
				{
					return GR_GENERIC;		// eat the event
				}
			}
		}
		return GR_OK;

	case WM_LBUTTONDOWN:
		if (bDisabled == false)
		{
			if (BaseHotRect::Notify(message, param) == GR_OK)
			{
				onLeftButtonDown(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
				if (bAlert)
				{
					return GR_GENERIC;		// eat the event
				}
			}
		}
		return GR_OK;

	case WM_LBUTTONDBLCLK:
		if (bDisabled == false)
		{
			if (BaseHotRect::Notify(message, param) == GR_OK)
			{
				if (bHasFocus && selectedItem >= 0 && U32(mouseHighlight) < textLines)
					doAction();
				if (bAlert)
					return GR_GENERIC;		// eat the event
			}
			else
				return GR_GENERIC;
		}
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Listbox::drawListRect (bool bDebug)
{
	const U32 color = (bDebug) ? RGB(255, 0, 0) : RGB(140, 140, 160);
	//
	// draw the box
	//
	DA::LineDraw(0, screenRect.left+textArea.left, screenRect.top+textArea.top, screenRect.left+textArea.right, screenRect.top+textArea.top, color);
	DA::LineDraw(0, screenRect.left+textArea.right, screenRect.top+textArea.top, screenRect.left+textArea.right, screenRect.top+textArea.bottom, color);
	DA::LineDraw(0, screenRect.left+textArea.right, screenRect.top+textArea.bottom, screenRect.left+textArea.left, screenRect.top+textArea.bottom, color);
	DA::LineDraw(0, screenRect.left+textArea.left, screenRect.top+textArea.bottom, screenRect.left+textArea.left, screenRect.top+textArea.top, color);
}
//--------------------------------------------------------------------------//
//
void Listbox::draw (void)
{
	const U32 lineHeight = fontHeight + leadingHeight;

	if (pListboxType->shape)
	{
		pListboxType->shape->Draw(0, screenRect.left, screenRect.top);
	}
	else if (bSolidBackground && !bNoBorder)
	{
		DA::RectangleFill(0, screenRect.left+textArea.left,
							 screenRect.top+textArea.top,
							 screenRect.left+textArea.right,
							 screenRect.top+textArea.bottom, RGB(0,0,0));
	}
	else if (!bNoBorder)
	{
		DA::RectangleHash(0, screenRect.left+textArea.left,
							 screenRect.top+textArea.top,
							 screenRect.left+textArea.right,
							 screenRect.top+textArea.bottom, RGB(0,0,0));

		drawListRect(false);
	}

	//
	// draw the highlight
	//
	if (bStatic == false)
	{
		U32 line = selectedItem - topLine;
		
		if (line < textLines)
		{
			const GT_LISTBOX::COLOR & color = pListboxType->colors[(bKeyboardFocus)?GTLBTXT_SELECTED:GTLBTXT_SELECTED_GRAYED];

			line *= lineHeight;

			if (pScrollBar != 0 && pScrollBar->GetVisible())
			{
				U32 swidth, sheight;
				pScrollBar->GetDimensions(swidth, sheight);
				swidth+=IDEAL2REALX(3);

				DA::RectangleHash(0, screenRect.left+textArea.left, 
								 screenRect.top+textArea.top+line,
								 screenRect.left + textArea.right - swidth,
								 screenRect.top+textArea.top+line+fontHeight+0, 
								 RGB(color.red, color.green, color.blue));
			}
			else
			{
				DA::RectangleHash(0, screenRect.left+textArea.left, 
								 screenRect.top+textArea.top+line,
								 screenRect.left+textArea.right,
								 screenRect.top+textArea.top+line+fontHeight+0, 
								 RGB(color.red, color.green, color.blue));
			}
		}
		else if (selectedItem < 0)		// no selection
		{
			const GT_LISTBOX::COLOR & color = pListboxType->colors[(bKeyboardFocus)?GTLBTXT_SELECTED:GTLBTXT_SELECTED_GRAYED];

			if (pScrollBar != NULL && pScrollBar->GetVisible())
			{
				U32 swidth, sheight;
				pScrollBar->GetDimensions(swidth, sheight);
				swidth+=IDEAL2REALX(3);

				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.right - swidth, 
								   screenRect.top+textArea.top,
								   RGB(color.red, color.green, color.blue));

				DA::LineDraw(0, screenRect.left+textArea.right - swidth, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.right - swidth, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));

				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top+fontHeight+0, 
								   screenRect.left+textArea.right - swidth, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));
				
				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.left, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));
				
			}
			else
			{

				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.right, 
								   screenRect.top+textArea.top,
								   RGB(color.red, color.green, color.blue));

				DA::LineDraw(0, screenRect.left+textArea.right, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.right, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));

				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top+fontHeight+0, 
								   screenRect.left+textArea.right, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));
				
				DA::LineDraw(0, screenRect.left+textArea.left, 
								   screenRect.top+textArea.top, 
								   screenRect.left+textArea.left, 
								   screenRect.top+textArea.top+fontHeight+0,
								   RGB(color.red, color.green, color.blue));
			}
		}
	}

	if (numItems > 0)
	{
		S32 i = topLine;
		S32 j = GetBottomVisibleString();
		PANE pane;
		LISTITEM * pNode=getItem(i);

		pane.window = 0;
		pane.x0   = screenRect.left+textArea.left;
		pane.x1  = screenRect.left+textArea.right;
		pane.y0    = screenRect.top+textArea.top;
		pane.y1 = screenRect.top+textArea.top+lineHeight-1;

		while (i <= j)
		{
			if (pNode->color)
			{
				ppFont[i-topLine]->SetFontColor(pNode->color, 0);
			}
			else
			{
				const GT_LISTBOX::COLOR & color = pListboxType->colors[(i-topLine==mouseHighlight) ? GTLBTXT_HIGHLIGHT : GTLBTXT_NORMAL];
				ppFont[i-topLine]->SetFontColor(RGB(color.red, color.green, color.blue) | 0xFF000000, 0);
			}
			ppFont[i-topLine]->StringDraw(&pane, 2, 0, pNode->szString);

			i++;
			pane.y0 += lineHeight;
			pane.y1 += lineHeight;
			pNode = pNode->pNext;
		}
	}

	//
	//	draw border around the control
	//
	if (!bNoBorder && pListboxType->shape == NULL)
	{
		DA::LineDraw(0, screenRect.left+textArea.left, screenRect.top+textArea.top, screenRect.left+textArea.right, screenRect.top+textArea.top, RGB(100,100,100));
		DA::LineDraw(0, screenRect.left+textArea.right, screenRect.top+textArea.top, screenRect.left+textArea.right, screenRect.top+textArea.bottom, RGB(100,100,100));
		DA::LineDraw(0, screenRect.left+textArea.right, screenRect.top+textArea.bottom, screenRect.left+textArea.left, screenRect.top+textArea.bottom, RGB(100,100,100));
		DA::LineDraw(0, screenRect.left+textArea.left, screenRect.top+textArea.bottom, screenRect.left+textArea.left, screenRect.top+textArea.top, RGB(100,100,100));
	}

	if (DEFAULTS->GetDefaults()->bDrawHotrects)
	{
		drawListRect(true);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::init (LISTBOXTYPE * _pListboxType)
{
	pListboxType = _pListboxType;
}
//--------------------------------------------------------------------------//
//
LISTITEM * Listbox::getItem (S32 index)
{
	CQASSERT(index>=0);
	LISTITEM * pNode = pList;

	while (pNode && index>0)
	{
		pNode=pNode->pNext;
		index--;
	}

	return pNode;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::AddStringToHead (const wchar_t * szString)
{
	LISTITEM * pNode = new LISTITEM;

	pNode->szString = wcsdup(szString);

	// always add the string to the head!
	pNode->pNext = pList;
	pList = pNode;

	++numItems;
	if(pScrollBar != 0)
	{
		pScrollBar->SetScrollRange(numItems);
		pScrollBar->SetVisible(bScrollVisible&&bInvisible==0&&numItems>textLines);
	}

	// the index is always zero
	return 0;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::AddString (const wchar_t * szString)
{
	LISTITEM * pNode = new LISTITEM;

	pNode->szString = wcsdup(szString);
	if (pList==0)
	{
		pList = pEndList = pNode;
	}
	else
	{
		pEndList->pNext = pNode;
		pEndList = pNode;
	}

	++numItems;
	if(pScrollBar != 0)
	{
		pScrollBar->SetScrollRange(numItems);
		pScrollBar->SetVisible(bScrollVisible&&bInvisible==0&&numItems>textLines);
	}
	return numItems-1;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::FindString (const wchar_t * szString)
{
	int len = wcslen(szString);
	LISTITEM * pNode = pList;
	S32 result = -1;

	while (pNode)
	{
		result++;
		if (wcsncmp(szString, pNode->szString, len) == 0)
			break;
		pNode = pNode->pNext;
	}

	return (pNode) ? result : -1;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::FindStringExact (const wchar_t * szString)
{
	LISTITEM * pNode = pList;
	S32 result = -1;

	while (pNode)
	{
		result++;
		if (wcscmp(szString, pNode->szString) == 0)
			break;
		pNode = pNode->pNext;
	}

	return (pNode) ? result : -1;
}
//--------------------------------------------------------------------------//
//
void Listbox::RemoveString (S32 index)
{
	if (index == 0)
	{
		LISTITEM * pNode;
		if ((pNode = pList) != 0)
		{
			if ((pList = pList->pNext) == 0)
				pEndList = 0;	// list is empty
	
			delete pNode;
			--numItems;
			if(pScrollBar != 0)
				pScrollBar->SetScrollRange(numItems);
			if (U32(selectedItem) == numItems)
			{
				selectedItem--;
				notifyCaretPos();
			}
		}	
	}
	else
	{
		LISTITEM * pPrev;
		
		if ((pPrev = getItem(index-1)) != 0)
		{
			LISTITEM * pNode;
			
			if ((pNode = pPrev->pNext) != 0)
			{
				if ((pPrev->pNext = pNode->pNext) == 0)
					pEndList = pPrev;	// removing the end of the list

				delete pNode;
				--numItems;
				if(pScrollBar != 0)
					pScrollBar->SetScrollRange(numItems);
				if (U32(selectedItem) == numItems)
				{
					selectedItem--;
					notifyCaretPos();
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
U32 Listbox::GetString (S32 index, wchar_t * buffer, U32 bufferSize)
{
	LISTITEM * pNode = getItem(index);
	if (pNode == 0)
		return 0;
	int len = wcslen(pNode->szString);
	
	bufferSize /= sizeof(wchar_t);
	bufferSize = __min(bufferSize, U32(len+1));

	memcpy(buffer, pNode->szString, bufferSize*2);

	return len;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::SetString (S32 index, const wchar_t * szString)
{
	LISTITEM * pNode = getItem(index);
	if (pNode == 0)
		return 0;

	free(pNode->szString);
	pNode->szString = wcsdup(szString);

	return index;
}
//--------------------------------------------------------------------------//
//
void Listbox::SetDataValue (S32 index, U32 data)
{
	LISTITEM * pNode = getItem(index);

	if (pNode)
	{
		pNode->userData = data;
	}
}
//--------------------------------------------------------------------------//
//
U32 Listbox::GetDataValue (S32 index)
{
	LISTITEM * pNode = getItem(index);

	if (pNode)
		return pNode->userData;
	else
		return 0;
}
//--------------------------------------------------------------------------//
//
void Listbox::SetColorValue (S32 index, COLORREF color)
{
	LISTITEM * pNode = getItem(index);

	if (pNode)
	{
		pNode->color = color | 0xFF000000;
	}
}
//--------------------------------------------------------------------------//
//
COLORREF Listbox::GetColorValue (S32 index)
{
	LISTITEM * pNode = getItem(index);

	if (pNode)
		return pNode->color & ~0xFF000000;
	else
		return 0;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::GetCurrentSelection (void)
{
	return selectedItem;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::SetCurrentSelection (S32 newIndex)
{
	S32 result = selectedItem;

	if (U32(newIndex) < numItems)
	{
		selectedItem = newIndex;
		notifyCaretPos();
		EnsureVisible(selectedItem);
	}
	else
	if (newIndex == -1)
	{
		selectedItem = newIndex;
		notifyCaretPos();
		EnsureVisible(0);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::GetCaretPosition (void)
{
	return GetCurrentSelection();
}
//--------------------------------------------------------------------------//
//
S32 Listbox::SetCaretPosition (S32 newIndex)
{
	return SetCurrentSelection(newIndex);
}
//--------------------------------------------------------------------------//
//
void Listbox::ResetContent (void)
{
	LISTITEM * pNode=pList;

	pEndList = 0;
	selectedItem = -1;
	notifyCaretPos();
	numItems=0;
	topLine = 0;
	if(pScrollBar != 0)
	{
		pScrollBar->SetScrollRange(numItems);
		pScrollBar->SetScrollPosition(topLine);
	}

	while ((pList = pNode) != 0)
	{
		pNode = pNode->pNext;
		delete pList;
	}
}
//--------------------------------------------------------------------------//
//
U32 Listbox::GetNumberOfItems (void)
{
	return numItems;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::GetTopVisibleString (void)
{
	if (numItems <= 0)
		return -1;
	return topLine;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::GetBottomVisibleString (void)
{
	if (numItems <= 0)
		return -1;
	if (numItems-topLine <= textLines)
		return numItems - 1;
	return topLine + textLines - 1;
}
//--------------------------------------------------------------------------//
//
void Listbox::EnsureVisible (S32 index)
{
	if (index < 0)
		return;

	if (textLines >= numItems)
	{
		return;
	}

	if (U32(index) >= numItems)
		index = numItems-1;

	//
	// is index above current top?
	//
	if (index <= topLine)
	{
		topLine = index;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);// scroll up
	}
	else // else we must scroll down
	{
		const int limit = numItems - textLines;

		if (limit <=  1)
		{
			topLine = 1;
			if(pScrollBar != 0)
				pScrollBar->SetScrollPosition(topLine);
		}
		else
		{
			if (U32(index) > topLine + pageLines)	// is it below visible range?
			{
				if ((topLine = index - pageLines) > limit)
				{
					topLine = limit;
				}
				if(pScrollBar != 0)
					pScrollBar->SetScrollPosition(topLine);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
bool Listbox::SetKeyboardFocus (bool bEnable)
{
	if (bStatic == false && bDisabled == false)
	{
		if (bEnable != bKeyboardFocus)  // if we are changing state
		{
			bKeyboardFocus = bEnable;
		}

		return true;
	}
	return false;	
}
//--------------------------------------------------------------------------//
//
U32 Listbox::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollPageUp (void)
{
	if ((topLine -= pageLines) < 0)
		topLine = 0;
	if(pScrollBar != 0)
		pScrollBar->SetScrollPosition(topLine);
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollPageDown (void)
{
	const int limit = numItems - textLines + 1;

	if (limit > 0)
	{
		if ((topLine += pageLines) > limit)
			topLine = limit;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollLineUp (void)
{
	if (topLine > 0)
		topLine--;
	if(pScrollBar != 0)
		pScrollBar->SetScrollPosition(topLine);
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollLineDown (void)
{
	const int limit = numItems - textLines + 1;

	if (limit > 0)
	{
		if ((topLine += 1) > limit)
			topLine = limit;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollPageUp (U32 scrollId)
{
	if ((topLine -= pageLines+1) < 0)
		topLine = 0;
	if(pScrollBar != 0)
		pScrollBar->SetScrollPosition(topLine);
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollPageDown (U32 scrollId)
{
	const int limit = numItems - textLines + 1;

	if (limit > 0)
	{
		if ((topLine += pageLines+1) > limit)
			topLine = limit;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollLineUp (U32 scrollId)
{
	if (topLine > 0)
		topLine--;
	if(pScrollBar != 0)
		pScrollBar->SetScrollPosition(topLine);
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollLineDown (U32 scrollId)
{
	if (textLines + topLine >= numItems)
	{
		return;
	}

	const int limit = numItems - textLines + 1;

	if (limit > 0)
	{
		if ((topLine += 1) > limit)
			topLine = limit;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::SetScrollPosition (U32 scrollId,S32 scrollPosition)
{
	if(topLine < scrollPosition)
	{
		if(scrollPosition > 0)
			topLine = scrollPosition;
		else
			topLine = 0;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}else if(topLine > scrollPosition)
	{
		const int limit = numItems - textLines + 1;
		if(limit > 0)
		{
			if ((topLine = scrollPosition) > limit)
				topLine = limit;
			if(pScrollBar != 0)
				pScrollBar->SetScrollPosition(topLine);
		}
	}
		
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollHome (void)
{
	topLine = 0;
	if(pScrollBar != 0)
		pScrollBar->SetScrollPosition(topLine);
}
//--------------------------------------------------------------------------//
//
void Listbox::ScrollEnd (void)
{
	const int limit = numItems - textLines + 1;

	if (limit > 0)
	{
		topLine = limit;
		if(pScrollBar != 0)
			pScrollBar->SetScrollPosition(topLine);
	}
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretPageUp (void)
{
	if ((selectedItem -= pageLines) < 0)
		selectedItem = 0;
	if (U32(selectedItem) >= numItems)
		selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretPageDown (void)
{
	if (selectedItem < 0)
		selectedItem = pageLines;
	else
		selectedItem += pageLines;

	if (U32(selectedItem) >= numItems)
		selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretLineUp (void)
{
	if (--selectedItem < 0)
		selectedItem = 0;
	if (U32(selectedItem) >= numItems)
		selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretLineDown (void)
{
	if (selectedItem < 0)
		selectedItem = 1;
	else
		selectedItem++;

	if (U32(selectedItem) >= numItems)
		selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretHome (void)
{
	selectedItem = 0;
	if (U32(selectedItem) >= numItems)
		selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::CaretEnd (void)
{
	selectedItem = numItems - 1;
	notifyCaretPos();
	EnsureVisible(selectedItem);
}
//--------------------------------------------------------------------------//
//
void Listbox::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
S32 Listbox::GetBreakIndex(const wchar_t * szString)
{
	U32 width = textArea.right - textArea.left - 10;

	// can the whole buffer fit into the list box?
	if (ppFont[0]->GetStringWidth(szString) < width)
	{
		return -1;
	}

	// find the last space character that can fit if the list box
	const wchar_t* lastpos = szString;
	wchar_t	substr[256];
	int	index = 0;
	int trail = 0;
	int length = wcslen(szString);

	do
	{
		lastpos = wcschr(lastpos, L' ');

		if (lastpos)
		{
			lastpos++;
			index = (lastpos - szString);
			wcsncpy(substr, szString, index);
			substr[index] = '0';
			trail = index;
		}
	} while (lastpos && ppFont[0]->GetStringWidth(substr) < width);

	// if there was a viable space character found, then return the index of that character
	if (trail)
	{
		return trail;
	}
	else
	{
		// there were no space characters found, find the last character that can possibly fit in the listbox
		int i;

		// brute force method, go through each character seqentially
		for (i = 0; i < length; i++)
		{
			substr[i] = szString[i];
			substr[i+1] = '\0';
			
			if (ppFont[0]->GetStringWidth(substr) >= width)
			{
				return i-1;
			}
		}

	}

	return -1;
}
//--------------------------------------------------------------------------//
//
const bool Listbox::IsMouseOver (S16 x, S16 y) const
{
	if (bInvisible == false)
	{
		if (x >= screenRect.left && x <= screenRect.right && y >= screenRect.top && y <= screenRect.bottom)
		{
			return true;
		}
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool Listbox::onKeyPressed (int vKey)
{
	switch (vKey)
	{
	case VK_RETURN:
		if (selectedItem >= 0)
			doAction();
		break;
	case VK_HOME:
		CaretHome();
		break;
	case VK_END:
		CaretEnd();
		break;
	case VK_PRIOR:
		CaretPageUp();
		break;
	case VK_NEXT:
		CaretPageDown();
		break;
	case VK_UP:
		CaretLineUp();
		break;
	case VK_DOWN:
		CaretLineDown();
		break;

	default:
		return false;
	}

	// we want to eat the message
	return true;
}
//--------------------------------------------------------------------------//
//
void Listbox::onMouseMove (S32 x, S32 y)
{
	if (bDisabled == false)
	{
		mouseHighlight = -1;

		if (bAlert && bStatic==false)
		{
			RECT rect;
			U32 i;
			const U32 lineHeight = fontHeight + leadingHeight;
			const U32 actualLines = GetBottomVisibleString() - topLine + 1;

			rect.left   = screenRect.left+textArea.left;
			rect.right  = screenRect.left+textArea.right;
			rect.top    = screenRect.top+textArea.top;
			rect.bottom = screenRect.top+textArea.top+fontHeight-1;

			for (i = 0; i < actualLines; i++)
			{
				if (x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom)
				{
					mouseHighlight = i;
					if (bDisableMouseSelect == false)
					{
						if (bSingleClick && selectedItem != topLine + mouseHighlight)
						{
							selectedItem = topLine + mouseHighlight;
							notifyCaretPos();
						}
					}
					break;
				}

				rect.top    += lineHeight;
				rect.bottom += lineHeight;
			}
		}
	}
}
//--------------------------------------------------------------------------//
// -zDelta = rolled toward user, +zDelta = rolled away from user
//
void Listbox::onMouseWheel (S32 zDelta)
{
	if (bDisabled == false)
	{
		if (zDelta >= 0)
			ScrollLineUp();
		else
			ScrollLineDown();
	}
}
//--------------------------------------------------------------------------//
//-----------------------Listbox Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ListboxFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ListboxFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ListboxFactory (void) { }

	~ListboxFactory (void);

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

	/* ListboxFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
ListboxFactory::~ListboxFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ListboxFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ListboxFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_LISTBOX)
	{
		GT_LISTBOX * data = (GT_LISTBOX *) _data;
		LISTBOXTYPE * result = new LISTBOXTYPE;

		result->pArchetype = pArchetype;
		if (data->fontName[0])
		{
			result->pFontType = GENDATA->LoadArchetype(data->fontName);
			CQASSERT(result->pFontType);
			GENDATA->AddRef(result->pFontType);
		}

		result->colors[GTLBTXT_DISABLED] = data->disabledText;
		result->colors[GTLBTXT_HIGHLIGHT] = data->highlightText;
		result->colors[GTLBTXT_NORMAL] = data->normalText;
		result->colors[GTLBTXT_SELECTED] = data->selectedText;
		result->colors[GTLBTXT_SELECTED_GRAYED] = data->selectedTextGrayed;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				CreateDrawAgent((VFX_SHAPETABLE *) pImage, 0, result->shape);
			END_MAPPING(INTERFACEDIR);

			result->shape->GetDimensions(result->width, result->height);
		}

		if (data->scrollBarType[0])
		{
			if ((result->pScrollBarType = GENDATA->LoadArchetype(data->scrollBarType)) != 0)
				GENDATA->AddRef(result->pScrollBarType);
		}

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ListboxFactory::DestroyArchetype (HANDLE hArchetype)
{
	LISTBOXTYPE * type = (LISTBOXTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ListboxFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	LISTBOXTYPE * type = (LISTBOXTYPE *) hArchetype;
	Listbox * result = new DAComponent<Listbox>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _ListboxFactory : GlobalComponent
{
	ListboxFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ListboxFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _ListboxFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Listbox.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
