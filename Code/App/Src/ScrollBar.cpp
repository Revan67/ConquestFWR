//--------------------------------------------------------------------------//
//                                                                          //
//                               ScrollBar.cpp                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ScrollBar.cpp 18    11/06/00 12:10p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IScrollBar.h"
#include "IButton2.h"
#include "DButton.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "DrawAgent.h"

#include <TComponent.h>
#include <FileSys.h>
#include <DScrollBar.h>

#include <stdlib.h>

//id's for the buttons
#define TOP_LEFT_SCROLL_BUTTON_ID 1
#define BOTTOM_RIGHT_SCROLL_BUTTON_ID 2

//multiplyer for the thumb highlight and shadow color
#define THUMB_HIGHLIGHT_DIF 1.5f
#define THUMB_SHADOW_DIF 0.5f

//constants for the thumb
#define MIN_SCROLL_THUMB_AREA 10
#define MIN_SCROLL_THUMB_SIZE 6

//breakoff distance for sliding thumb
#define SCROLL_BREAKOFF_DIST 40

//speed in miliseconds
#define REPEATER_SCROLL_PAGE_SPEED 200

#define MAX_SCROLLBAR_SHAPES 2

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
struct SCROLLBARTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pTopLeftButton;
	PGENTYPE pBottomRightButton;

	char topLeftButtonType[GT_PATH];
	char bottomRightButtonType[GT_PATH];

	U32 thumbColor;
	U32 thumbColorBright;
	U32 thumbColorDark;
	U32 backgroundColor;
	U32 disabledColor;
	U32 buttonHeight;
	U32 buttonWidth;

	bool bHorizontal:1;

	COMPTR<IDrawAgent> shapes[MAX_SCROLLBAR_SHAPES];

	SCROLLBARTYPE (void)
	{
	}

	~SCROLLBARTYPE (void)
	{
		if (pTopLeftButton)
			GENDATA->Release(pTopLeftButton);
		if (pBottomRightButton)
			GENDATA->Release(pBottomRightButton);
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
struct DACOM_NO_VTABLE ScrollBar : BaseHotRect, IScrollBar
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(ScrollBar)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IScrollBar)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	SCROLLBARTYPE * pScrollBarType;
	RECT slideArea;
	RECT thumbRect;

	S32 scrollPosition;
	S32 scrollRange;
	S32 viewRange;
	S32 slideOffset;
	S32 oldPosition;

	U32 leftOverTime;

	COMPTR<IButton2> topLeftButton;
	COMPTR<IButton2> bottomRightButton;
	IScrollBarOwner * owner;			// no extra reference, same as parent object

	bool bActive:1;
	bool bDisabled;
	bool bSliding:1;
	bool bSlideCancel:1;
	bool bPostMessageBlocked:1;
	bool bThumbActive:1;
	bool bPageMoveUp:1; //used for page up an down
	bool bPaging:1;		//true when in paging mode
	bool bPagingActive:1;//true when mouse over scroll in paging mode

	U32 controlID;

	//
	// class methods
	//

	ScrollBar (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~ScrollBar (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IScrollBar methods  */

	virtual void InitScrollBar (const SCROLLBAR_DATA & data, BaseHotRect * parent); 

	virtual void EnableScrollBar (bool bEnable)
	{
		if (bEnable == bDisabled)	// if we are changing state
		{
			bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
			bDisabled = !bEnable;
		}
	}

	virtual void SetVisible (bool bVisible);

	virtual const bool GetVisible (void) const;

	virtual S32 GetScrollPosition (void);		// returns index
		
	virtual S32 SetScrollPosition (S32 newIndex);		// returns old thumb index

	virtual void SetScrollRange (S32 bottomIndex);		// sets the scrollable range from 0
	
	virtual void SetViewRange (S32 viewSize);		// sets the view range

	virtual void SetControlID (U32 id);

	virtual U32 GetControlID (void);

	virtual void GetDimensions (U32 & width, U32 & height);

	/* ScrollBar methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (SCROLLBARTYPE * _pScrollBarType);

	void draw (void);

	void onMouseMove (S32 x, S32 y);

	bool onLeftButtonDown (S32 x, S32 y);

	bool onLeftButtonUp (S32 x, S32 y);

	void createThumbRect();

	void doPagingRepeat(U32 miliElapsed)
	{
		miliElapsed += leftOverTime;
		while(miliElapsed > REPEATER_SCROLL_PAGE_SPEED)
		{
			if(bPageMoveUp)
				owner->ScrollPageUp(controlID);
			else
				owner->ScrollPageDown(controlID);

			miliElapsed -= REPEATER_SCROLL_PAGE_SPEED;
		}
		leftOverTime = miliElapsed;		
	}
	
	virtual void DrawRect (void)
	{
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void setActive (bool _bActive)
	{
		bActive = _bActive;
		topLeftButton->SetVisible(bActive && !bInvisible);
		bottomRightButton->SetVisible(bActive && !bInvisible);
	}
};
//--------------------------------------------------------------------------//
//
ScrollBar::~ScrollBar (void)
{
	GENDATA->Release(pScrollBarType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void ScrollBar::InitScrollBar (const SCROLLBAR_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());

		//create scroll Buttons
		COMPTR<IDAComponent> pComp;

		GENDATA->CreateInstance(pScrollBarType->pTopLeftButton, pComp);
		pComp->QueryInterface("IButton2", topLeftButton);
		
		topLeftButton->SetControlID(TOP_LEFT_SCROLL_BUTTON_ID);
		GENDATA->CreateInstance(pScrollBarType->pBottomRightButton, pComp);
		
		pComp->QueryInterface("IButton2", bottomRightButton);
		bottomRightButton->SetControlID(BOTTOM_RIGHT_SCROLL_BUTTON_ID);
		
		// get size info
		if (pScrollBarType->buttonHeight == 0)
			bottomRightButton->GetDimensions(pScrollBarType->buttonWidth, pScrollBarType->buttonHeight);

		//info that will be set by owner
		scrollPosition = 0;
		scrollRange = 0;
		viewRange = 0;
		bSliding = false;
		bSlideCancel = false;
		bInvisible = true;

		if (_parent->QueryInterface("IScrollBarOwner", (void **) & owner) == GR_OK)
			owner->Release();	// release the reference count early
	}

	//find rectangles for scrollbar and for the scroll area
	if(pScrollBarType->bHorizontal)
	{
		screenRect.left = parent->screenRect.left;
		screenRect.top = parent->screenRect.bottom - pScrollBarType->buttonHeight+1;
		screenRect.bottom = parent->screenRect.bottom;
		screenRect.right = parent->screenRect.right;

		slideArea.top = screenRect.top;
		slideArea.bottom = screenRect.bottom;
		slideArea.left = screenRect.left+pScrollBarType->buttonWidth;
		slideArea.right = screenRect.right-pScrollBarType->buttonWidth;

	}
	else	//vertical
	{
		screenRect.left = parent->screenRect.right - pScrollBarType->buttonWidth+1;
		screenRect.top = parent->screenRect.top;
		screenRect.bottom = parent->screenRect.bottom;
		screenRect.right = parent->screenRect.right;

		slideArea.top = screenRect.top+pScrollBarType->buttonHeight;
		slideArea.bottom = screenRect.bottom-pScrollBarType->buttonHeight;
		slideArea.left = screenRect.left;
		slideArea.right = screenRect.right;
	}

	//Initialize the buttons
	BUTTON_DATA buttonData;
	memset(&buttonData, 0, sizeof(buttonData));
	buttonData.buttonText = BTNTXT::NOTEXT;
	strcpy(buttonData.buttonType,pScrollBarType->topLeftButtonType);
	buttonData.xOrigin = 0;
	buttonData.yOrigin = 0;
	topLeftButton->InitButton(buttonData, this);

	strcpy(buttonData.buttonType,pScrollBarType->bottomRightButtonType);
	if(pScrollBarType->bHorizontal)
		buttonData.xOrigin = REAL2IDEALX((screenRect.right-screenRect.left)-pScrollBarType->buttonWidth+1);
	else
		buttonData.yOrigin = REAL2IDEALY((screenRect.bottom-screenRect.top)-pScrollBarType->buttonHeight+1);
	bottomRightButton->InitButton(buttonData, this);

	//test to see if the thumb shoud be active
	if(pScrollBarType->bHorizontal)
		bThumbActive = (slideArea.right - slideArea.left) > MIN_SCROLL_THUMB_AREA;
	else
		bThumbActive = (slideArea.bottom - slideArea.top) > MIN_SCROLL_THUMB_AREA;
	
	SetVisible(true);
}
//--------------------------------------------------------------------------//
//
void ScrollBar::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
	topLeftButton->SetVisible(bVisible);
	bottomRightButton->SetVisible(bVisible);

	if (bVisible && scrollPosition < 0)
	{
		scrollPosition = 0;
		createThumbRect();
	}
}
//--------------------------------------------------------------------------//
//
const bool ScrollBar::GetVisible (void) const
{
	return (bActive && !bInvisible);
}
//--------------------------------------------------------------------------//
//
S32 ScrollBar::GetScrollPosition ()
{
	return scrollPosition;
}
		
//--------------------------------------------------------------------------//
//
S32 ScrollBar::SetScrollPosition (S32 newIndex) 	// returns position
{
	if (newIndex > scrollRange-viewRange) 
	{
		newIndex = scrollRange - viewRange;
	}
	if (newIndex < 0)
	{
		newIndex = 0;
	}
	
	S32 temp = scrollPosition;
	scrollPosition = newIndex;
	
	if ((!bSliding) || bSlideCancel)
	{
		createThumbRect();
	}
	return temp;
}

//--------------------------------------------------------------------------//
//
void ScrollBar::SetScrollRange (S32 bottomIndex)		// sets the scrollable range from 0
{
	scrollRange = bottomIndex;

	setActive(scrollRange > viewRange);

	if (bDisabled)
	{
		scrollPosition = 0;
	}
	else
	{
		if(scrollPosition >= scrollRange-viewRange)
		{	
			scrollPosition = scrollRange-viewRange-1;
		}
	}

	if (scrollPosition < 0)
	{
		scrollPosition = 0;
	}
	createThumbRect();
}
	
//--------------------------------------------------------------------------//
//
void ScrollBar::SetViewRange (S32 viewSize)		// sets the view range
{
	viewRange = viewSize;

	setActive(scrollRange > viewRange);

	if(bDisabled)
	{
		scrollPosition = 0;
	}
	else
	{
		if(scrollPosition >= scrollRange-viewRange)
		{	
			scrollPosition = scrollRange-viewRange-1;
		}
	}

	if (scrollPosition < 0)
	{
		scrollPosition = 0;
	}
	createThumbRect();
}
//--------------------------------------------------------------------------//
//
GENRESULT ScrollBar::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false && bActive == true)
	switch (message)
	{
	case CQE_BUTTON:
		if (((int)param) == TOP_LEFT_SCROLL_BUTTON_ID)
		{
			if (!bPostMessageBlocked)
			{
				owner->ScrollLineUp(controlID);
				bPostMessageBlocked = true;
			}
			return GR_GENERIC;
		}
		else if (((int)param) == BOTTOM_RIGHT_SCROLL_BUTTON_ID)
		{
			if (!bPostMessageBlocked)
			{
				owner->ScrollLineDown(controlID);
				bPostMessageBlocked = true;
			}
			return GR_GENERIC;
		}
		break;

	case CQE_UPDATE:
		if(bPagingActive)
			doPagingRepeat(((U32)param) >> 10);
		else
			leftOverTime = 0;
		bPostMessageBlocked = false;
		break;

	case CQE_ENDFRAME:
		draw();
		break;

	case WM_MOUSEMOVE:
		if (BaseHotRect::Notify(message, param) == GR_OK)
		{
			if (bSliding || bPaging)
			{
				onMouseMove(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam)));
				return GR_GENERIC;
			}
			return (bAlert) ? GR_GENERIC : GR_OK;
		}
		else
		{
			return GR_GENERIC;
		}
		break;

	case WM_LBUTTONUP:
		// if we're in sliding mode, than don't pass on the message to our clients
		// we won't get the BaseHotRect::Notify call if bSliding is true
		if (bSliding || BaseHotRect::Notify(message, param) == GR_OK)
		{
			if (onLeftButtonUp(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam))) || bAlert)
				return GR_GENERIC;
			else
				return GR_OK;
		}
		else
		{
			return GR_GENERIC;
		}
		break;

	case WM_LBUTTONDOWN:
		if (BaseHotRect::Notify(message, param) == GR_OK)
		{
			if (onLeftButtonDown(S16(LOWORD(msg->lParam)), S16(HIWORD(msg->lParam))) || bAlert)
				return GR_GENERIC;
			else
				return GR_OK;
		}
		else
			return GR_GENERIC;
		break;

	case WM_LBUTTONDBLCLK:
		if (BaseHotRect::Notify(message, param) == GR_OK)
			return (bAlert) ? GR_GENERIC : GR_OK;
		else
			return GR_GENERIC;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void ScrollBar::draw (void)
{
	if (bDisabled)
	{
		DA::RectangleFill(0,slideArea.left,slideArea.top,slideArea.right,slideArea.bottom,
			pScrollBarType->disabledColor);
	}
	else 
	{
		if (pScrollBarType->shapes[0])
		{
			pScrollBarType->shapes[0]->Draw(0, slideArea.left, slideArea.top);
		}
		else
		{
			DA::RectangleFill(0,slideArea.left,slideArea.top,slideArea.right,slideArea.bottom,
				pScrollBarType->backgroundColor);
		}

		if (bThumbActive)
		{
			if (pScrollBarType->shapes[0])
			{
					PANE pane;
					pane.window = 0;
					pane.x0 = thumbRect.left;
					pane.y0 = thumbRect.top;
					pane.x1 = thumbRect.right;
					pane.y1 = thumbRect.bottom;

				pScrollBarType->shapes[1]->Draw(&pane, 0, 0);
			}
			else
			{
				//draw highlight
				DA::LineDraw(0,thumbRect.left,thumbRect.top,thumbRect.right-1,thumbRect.top,pScrollBarType->thumbColorBright);
				DA::LineDraw(0,thumbRect.left,thumbRect.top+1,thumbRect.right-2,thumbRect.top+1,pScrollBarType->thumbColorBright);
				DA::LineDraw(0,thumbRect.left,thumbRect.top+2,thumbRect.left,thumbRect.bottom-1,pScrollBarType->thumbColorBright);
				DA::LineDraw(0,thumbRect.left+1,thumbRect.top+2,thumbRect.left+1,thumbRect.bottom-2,pScrollBarType->thumbColorBright);

				//draw shadow
				DA::LineDraw(0,thumbRect.left,thumbRect.bottom,thumbRect.right,thumbRect.bottom,pScrollBarType->thumbColorDark);
				DA::LineDraw(0,thumbRect.left+1,thumbRect.bottom-1,thumbRect.right,thumbRect.bottom-1,pScrollBarType->thumbColorDark);
				DA::LineDraw(0,thumbRect.right,thumbRect.top,thumbRect.right,thumbRect.bottom-2,pScrollBarType->thumbColorDark);
				DA::LineDraw(0,thumbRect.right-1,thumbRect.top+1,thumbRect.right-1,thumbRect.bottom-2,pScrollBarType->thumbColorDark);

				DA::RectangleFill(0,thumbRect.left+2,thumbRect.top+2,
					thumbRect.right-2,thumbRect.bottom-2,
					pScrollBarType->thumbColor);
			}
		}
	}

	if (DEFAULTS->GetDefaults()->bDrawHotrects)
	{
		U32 color = RGB(0, 255, 0);
		//
		// draw the box
		//
		DA::LineDraw(0, slideArea.left, slideArea.top, slideArea.left, slideArea.bottom, color);
		DA::LineDraw(0, slideArea.right, slideArea.top, slideArea.right, slideArea.bottom, color);
		DA::LineDraw(0, slideArea.left, slideArea.top, slideArea.right, slideArea.top, color);
		DA::LineDraw(0, slideArea.left, slideArea.bottom, slideArea.right, slideArea.bottom, color);

		//draw the thumb
		color = RGB(0,0,255);
		DA::LineDraw(0, thumbRect.left, thumbRect.top, thumbRect.left, thumbRect.bottom, color);
		DA::LineDraw(0, thumbRect.right, thumbRect.top, thumbRect.right, thumbRect.bottom, color);
		DA::LineDraw(0, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.top, color);
		DA::LineDraw(0, thumbRect.left, thumbRect.bottom, thumbRect.right, thumbRect.bottom, color);

	}

}
//--------------------------------------------------------------------------//
//
void ScrollBar::init (SCROLLBARTYPE * _pScrollBarType)
{
	pScrollBarType = _pScrollBarType;
}
//--------------------------------------------------------------------------//
//
U32 ScrollBar::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void ScrollBar::GetDimensions (U32 & width, U32 & height)
{
	width = screenRect.right - screenRect.left;
	height = screenRect.bottom - screenRect.top;
}
//--------------------------------------------------------------------------//
//
void ScrollBar::SetControlID (U32 id)
{
	controlID = id;
}

//--------------------------------------------------------------------------//
//
void ScrollBar::onMouseMove (S32 x, S32 y)
{
	if (bSliding)
	{
		if (pScrollBarType->bHorizontal)
		{
			if(slideOffset != x-thumbRect.left)
			{
				if(((y < slideArea.top-SCROLL_BREAKOFF_DIST) || (y > slideArea.bottom+SCROLL_BREAKOFF_DIST))
						&& (scrollPosition != oldPosition))
					owner->SetScrollPosition(controlID,oldPosition);
				else{
					int slideDist = slideArea.right - slideArea.left;
					int thumbWidth = thumbRect.right-thumbRect.left;
					float scrollDelta = ((float)(slideDist-thumbWidth))/((float)(scrollRange-viewRange));
					
					int newPos = ((x-slideOffset)-slideArea.left)/scrollDelta;
					if(newPos > scrollRange-viewRange)
						newPos = scrollRange-viewRange;
					if(newPos < 0) newPos = 0;

					if(newPos != scrollPosition)
						owner->SetScrollPosition(controlID, newPos);

					thumbRect.left = x-slideOffset;
					if(thumbRect.left < slideArea.left) 
						thumbRect.left = slideArea.left;
					if(thumbRect.left > slideArea.right - thumbWidth) 
						thumbRect.left = slideArea.right - thumbWidth;
					thumbRect.right = thumbRect.left + thumbWidth;
				}

			}
		}
		else if (slideOffset != y-thumbRect.top)
		{
			if ((x < slideArea.left-SCROLL_BREAKOFF_DIST) || (x > slideArea.right+SCROLL_BREAKOFF_DIST))
			{
				bSlideCancel = true;
				owner->SetScrollPosition(controlID,oldPosition);
			}
			else
			{
				bSlideCancel = false;
				int slideDist = slideArea.bottom - slideArea.top;
				int thumbHeight = thumbRect.bottom-thumbRect.top;
				float scrollDelta = ((float)(slideDist-thumbHeight))/((float)(scrollRange-viewRange));
				
				int newPos = ((y-slideOffset)-slideArea.top)/scrollDelta;
				if(newPos > scrollRange-viewRange)
					newPos = scrollRange-viewRange;
				if(newPos < 0) newPos = 0;

				if(newPos != scrollPosition)
					owner->SetScrollPosition(controlID, newPos);

				thumbRect.top = y-slideOffset;
				if(thumbRect.top < slideArea.top) 
					thumbRect.top = slideArea.top;
				if(thumbRect.top > slideArea.bottom - thumbHeight) 
					thumbRect.top = slideArea.bottom - thumbHeight;
				thumbRect.bottom = thumbRect.top + thumbHeight;
			}

		}
	}
	else if (bPaging)
	{
		if ((x >= slideArea.left) && (x <= slideArea.right) && (y >= slideArea.top) && (y <= slideArea.bottom))
		{
			bPagingActive = true;
		}
		else
		{
			bPagingActive = false;
		}
	}
}

//--------------------------------------------------------------------------//
//
bool ScrollBar::onLeftButtonDown(S32 x, S32 y)
{
	if ((!bThumbActive) || bDisabled) 
	{
		return false;
	}
	
	if ((x >= thumbRect.left) && (x <= thumbRect.right) && (y >= thumbRect.top) && (y <= thumbRect.bottom))
	{
		oldPosition = scrollPosition;
		bSliding = true;
		bSlideCancel = false;
		
		if (pScrollBarType->bHorizontal)
		{
			slideOffset = x-thumbRect.left;
		}
		else
		{
			slideOffset = y-thumbRect.top;
		}
		
		return true;
	}
	else if ((x >= slideArea.left) && (x <= slideArea.right) && (y >= slideArea.top) && (y <= slideArea.bottom))
	{
		leftOverTime = 0;
		bPaging = true;
		bPagingActive = true;
		
		if (pScrollBarType->bHorizontal)
		{
			if (x < thumbRect.left)
			{
				bPageMoveUp = true;
				owner->ScrollPageUp(controlID);
			}
			else
			{
				bPageMoveUp = false;
				owner->ScrollPageDown(controlID);
			}
		}
		else
		{
			if (y < thumbRect.top)
			{
				bPageMoveUp = true;
				owner->ScrollPageUp(controlID);
			}
			else
			{
				bPageMoveUp = false;
				owner->ScrollPageDown(controlID);
			}
		}
		return true;
	}
	return false;
}

//--------------------------------------------------------------------------//
//
bool ScrollBar::onLeftButtonUp(S32 x, S32 y)
{
	if (bSliding)
	{
		bSliding = false;
		bSlideCancel = false;
		return true;
	}
	else if (bPaging)
	{
		bPaging = false;
		bPagingActive = false;
	}
	return false;
}

//--------------------------------------------------------------------------//
//
void ScrollBar::createThumbRect()
{
	if ((!bThumbActive) || bDisabled) 
	{
		return;
	}
	
	if (pScrollBarType->bHorizontal)
	{
		thumbRect.top = screenRect.top;
		thumbRect.bottom = screenRect.bottom;
		int slideDist = slideArea.right - slideArea.left;
		
		int thumbWidth = (scrollRange > 0) ? ((int)(slideDist*(((float)viewRange)/((float)scrollRange)))) : 0;
		
		if (thumbWidth < MIN_SCROLL_THUMB_SIZE)
		{
			thumbWidth = MIN_SCROLL_THUMB_SIZE;
		}
		float scrollDelta = (scrollRange - viewRange > 0) ? (((float)(slideDist-thumbWidth))/((float)(scrollRange-viewRange))) : 0;
		thumbRect.left = slideArea.left + (scrollPosition*scrollDelta);
		thumbRect.right = thumbRect.left + thumbWidth;
		
		// re-calculate the thumbRect top and bottom?
	}
	else
	{
		// what's the width of the scrollbar slider?
		if (pScrollBarType->shapes[0])
		{
			U16 swidth, sheight;
			pScrollBarType->shapes[1]->GetDimensions(swidth, sheight);

			int diff = ((screenRect.right - screenRect.left) - swidth) / 2;
			thumbRect.right = screenRect.right - diff;
			thumbRect.left = screenRect.left + diff;
		}
		else
		{
			thumbRect.right = screenRect.right;
			thumbRect.left = screenRect.left;
		}
		int slideDist = slideArea.bottom - slideArea.top;
		
		int thumbHeight = (scrollRange > 0) ? ((int)(slideDist*(((float)viewRange)/((float)scrollRange)))) : 0;
		
		if (thumbHeight < MIN_SCROLL_THUMB_SIZE)
		{
			thumbHeight = MIN_SCROLL_THUMB_SIZE;
		}
		float scrollDelta = (scrollRange - viewRange > 0) ? (((float)(slideDist-thumbHeight))/((float)(scrollRange-viewRange))):0;
		
		thumbRect.top = slideArea.top+(scrollPosition*scrollDelta);
		thumbRect.bottom = thumbRect.top + thumbHeight;
	}
}

//--------------------------------------------------------------------------//
//-----------------------ScrollBar Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ScrollBarFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ScrollBarFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ScrollBarFactory (void) { }

	~ScrollBarFactory (void);

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

	/* ScrollBarFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
ScrollBarFactory::~ScrollBarFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ScrollBarFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ScrollBarFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_SCROLLBAR)
	{
		GT_SCROLLBAR * data = (GT_SCROLLBAR *) _data;
		SCROLLBARTYPE * result = new SCROLLBARTYPE;

		result->pArchetype = pArchetype;

		result->thumbColor = RGB(data->thumbColor.red,
						data->thumbColor.green,
						data->thumbColor.blue);

		U32 redColor = data->thumbColor.red*THUMB_HIGHLIGHT_DIF;
		if(redColor >255) redColor = 255;
		U32 greenColor = data->thumbColor.green*THUMB_HIGHLIGHT_DIF;
		if(greenColor >255) greenColor = 255;
		U32 blueColor = data->thumbColor.blue*THUMB_HIGHLIGHT_DIF;
		if(blueColor >255) blueColor = 255;
		result->thumbColorBright = RGB(redColor,greenColor,blueColor);

		redColor = data->thumbColor.red*THUMB_SHADOW_DIF;
		if(redColor >255) redColor = 255;
		greenColor = data->thumbColor.green*THUMB_SHADOW_DIF;
		if(greenColor >255) greenColor = 255;
		blueColor = data->thumbColor.blue*THUMB_SHADOW_DIF;
		if(blueColor >255) blueColor = 255;
		result->thumbColorDark = RGB(redColor,greenColor,blueColor);

		result->backgroundColor = RGB(data->backgroundColor.red,
									  data->backgroundColor.green,
									  data->backgroundColor.blue);
		result->disabledColor = RGB(data->disabledColor.red,
									data->disabledColor.green,
									data->disabledColor.blue);
		
		if ((result->pTopLeftButton = GENDATA->LoadArchetype(data->upButtonType)) != 0)
			GENDATA->AddRef(result->pTopLeftButton);
		if ((result->pBottomRightButton = GENDATA->LoadArchetype(data->downButtonType)) != 0)
			GENDATA->AddRef(result->pBottomRightButton);
		
		strcpy(result->topLeftButtonType,data->upButtonType);
		strcpy(result->bottomRightButtonType,data->downButtonType);

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				int i;
				for (i = 0; i < MAX_SCROLLBAR_SHAPES; i++)
					CreateDrawAgent((VFX_SHAPETABLE *) pImage, i, result->shapes[i]);
			END_MAPPING(INTERFACEDIR);
		}

		result->bHorizontal = data->bHorizontal;
		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ScrollBarFactory::DestroyArchetype (HANDLE hArchetype)
{
	SCROLLBARTYPE * type = (SCROLLBARTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ScrollBarFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	SCROLLBARTYPE * type = (SCROLLBARTYPE *) hArchetype;
	ScrollBar * result = new DAComponent<ScrollBar>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _ScrollBarFactory : GlobalComponent
{
	ScrollBarFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ScrollBarFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _ScrollBarFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Listbox.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
