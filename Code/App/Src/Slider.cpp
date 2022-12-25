//--------------------------------------------------------------------------//
//                                                                          //
//                               Slider.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Slider.cpp 25    10/05/00 1:43p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "ISlider.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DSlider.h>
#include "DrawAgent.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#define TAB_WIDTH			10
#define MAX_SLIDER_SHAPES	8

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
struct SLIDERTYPE
{
	PGENTYPE pArchetype;

	GT_SLIDER::COLOR colors[GTS_MAX_STATES];
	bool bVertical;
	COMPTR<IDrawAgent> shapes[MAX_SLIDER_SHAPES];
	U16 tabWidth;
	U32 indent;

	SLIDERTYPE (void)
	{
	}

	~SLIDERTYPE (void)
	{
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
struct DACOM_NO_VTABLE Slider : BaseHotRect, ISlider, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Slider)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(ISlider)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	SLIDERTYPE * pSliderType;

	U32 controlID;					// set by owner

	bool bKeyboardFocus;
	bool bMousePressed;			// mouse press occurred inside rect
	bool bKeyboardPressed;		// keyboard used to press button
	bool bDisabled;
	bool bPostMessageBlocked;
	bool bVertical;

	S32 rangeMax, rangeMin;
	S32 position;
	short logicalMin, logicalMax;
	RECT rcTab;
	POINT ptPosition;
	short int positionOffset;

	bool bEndOnClick;

	//
	// class methods
	//

	Slider (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		rangeMax = 10;
		rangeMin = 0;
		position = 0;
	}

	virtual ~Slider (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "Slider");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}

	/* ISlider methods  */

	virtual void InitSlider (const SLIDER_DATA & data, BaseHotRect * parent); 

	virtual void EnableSlider (bool bEnable);

	virtual bool GetEnableState (void);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);
	
	virtual void SetControlID (U32 id);

	virtual void GetDimensions (U32 & width, U32 & height);

	virtual void SetRangeMax (const S32& max)
	{
		rangeMax = max;
	}

	virtual void SetRangeMin (const S32& min)
	{
		rangeMin = min;
	}

	virtual const S32& GetRangeMax (void)
	{
		return rangeMax;
	}

	virtual const S32& GetRangeMin (void)
	{
		return rangeMin;
	}

	virtual void SetPosition (const S32& pos)
	{
		position = pos;
		setLogicalPosition();
		parent->PostMessage(CQE_SLIDER, (void*)controlID);
	}
	
	virtual const S32& GetPosition (void)
	{
		return position;
	}

	virtual void EnableClickBehavior (bool bEnableClick)
	{
		bEndOnClick = bEnableClick;
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

	/* Slider methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (SLIDERTYPE * _pSliderType);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDown (void)
	{
		if (bDisabled == false)
		{
			bKeyboardPressed = false;
			bMousePressed = bAlert;
		}
	}

	void onLeftButtonUp (void)
	{
		if (bDisabled == false)
		{
			if (bMousePressed)
			{
				bMousePressed = bKeyboardPressed = false;

				// send the event here man!!
				if (bEndOnClick)
				{
					parent->PostMessage(CQE_SLIDER, (void*)controlID);
				}
			}
			else
				bMousePressed = false;
		}
	}

	void setLogicalPosition (void);

	void onMouseMove (short xpos, short ypos);
};
//--------------------------------------------------------------------------//
//
Slider::~Slider (void)
{
	GENDATA->Release(pSliderType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void Slider::InitSlider (const SLIDER_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, EVENT_PRIORITY_SLIDER);		// set to low priority	
	}

	if (pSliderType->shapes[0])
	{
		U16 width, height;
		pSliderType->shapes[0]->GetDimensions(width, height);
		
		screenRect.left   = IDEAL2REALX(data.screenRect.left) + parent->screenRect.left;
		screenRect.top    = IDEAL2REALY(data.screenRect.top) + parent->screenRect.top;
		screenRect.right  = screenRect.left + width;// + parent->screenRect.left;
		screenRect.bottom = screenRect.top + height;// + parent->screenRect.top;
	}
	else
	{
		screenRect = data.screenRect;
		screenRect.left   = IDEAL2REALX(screenRect.left) + parent->screenRect.left;
		screenRect.top    = IDEAL2REALY(screenRect.top) + parent->screenRect.top;
		screenRect.right  = IDEAL2REALX(screenRect.right) + parent->screenRect.left;
		screenRect.bottom = IDEAL2REALY(screenRect.bottom) + parent->screenRect.top;
	}

	rcTab.left   = 0;
	rcTab.top    = 0;

	if (pSliderType->bVertical)
	{
		rcTab.right = screenRect.right - screenRect.left - 1;
		rcTab.bottom = pSliderType->tabWidth;
	}
	else
	{
		rcTab.right  = pSliderType->tabWidth;
		rcTab.bottom = screenRect.bottom - screenRect.top - 1;
	}

	OffsetRect(&screenRect, IDEAL2REALX(data.xOrigin), IDEAL2REALY(data.yOrigin));

	if (pSliderType->bVertical)
	{
		ptPosition.x =  screenRect.left;
		ptPosition.y = screenRect.top + pSliderType->tabWidth/2 + pSliderType->indent;

		logicalMin = screenRect.bottom - pSliderType->tabWidth/2 - pSliderType->indent;
		logicalMax = ptPosition.y;
	}
	else
	{
		ptPosition.x = screenRect.left + pSliderType->tabWidth/2 + pSliderType->indent;
		ptPosition.y = screenRect.top;

		logicalMin = ptPosition.x;
		logicalMax = screenRect.right - pSliderType->tabWidth/2 - pSliderType->indent;
	}

	SetPosition(position);
}
//--------------------------------------------------------------------------//
//
void Slider::EnableSlider (bool bEnable)
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
bool Slider::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void Slider::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
bool Slider::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void Slider::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
void Slider::GetDimensions (U32 & width, U32 & height)
{
	// take this out???
}
//--------------------------------------------------------------------------//
//
bool Slider::SetKeyboardFocus (bool bEnable)
{
	if (bEnable && (bDisabled||bInvisible))
	{
		return false;
	}
	if (bEnable != bKeyboardFocus)  // if we are changing state
	{
		if ((bKeyboardFocus = bEnable) == 0)  // if loosing focus
		{
			bKeyboardPressed = false;
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
U32 Slider::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT Slider::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible)
	{
		return BaseHotRect::Notify(message, param);
	}
	
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case WM_LBUTTONDBLCLK:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bAlert)
		{
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		onLeftButtonDown();
		onMouseMove(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;

	case WM_LBUTTONUP:
		onLeftButtonUp();
		break;

	case WM_MOUSEMOVE:
		onMouseMove(short(LOWORD(msg->lParam)), short(HIWORD(msg->lParam)));
		break;

	case WM_KEYDOWN:
		if (bKeyboardFocus && bKeyboardPressed==false && bHasFocus)
		switch (msg->wParam)
		{
		// move the slider
		case VK_LEFT:
			if (bVertical == false)
			{
				BaseHotRect::Notify(message, param);
				SetPosition(GetPosition()-1);
				return GR_GENERIC;
			}
			break;

		case VK_RIGHT:
			if (bVertical == false)
			{
				BaseHotRect::Notify(message, param);
				SetPosition(GetPosition()+1);
				return GR_GENERIC;
			}
			break;

		case VK_UP:
			if (bVertical)
			{
				BaseHotRect::Notify(message, param);
				SetPosition(GetPosition()-1);
				return GR_GENERIC;
			}
			break;

		case VK_DOWN:
			if (bVertical)
			{
				BaseHotRect::Notify(message, param);
				SetPosition(GetPosition()+1);
				return GR_GENERIC;
			}
			break;

		} 
		break; 

	case CQE_UPDATE:
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Slider::onMouseMove (short xpos, short ypos)
{
	if (bMousePressed)
	{
		if (pSliderType->bVertical)
		{
			positionOffset += (ypos - ptPosition.y);
			ptPosition.y = ypos;
		}
		else
		{
			positionOffset += (xpos - ptPosition.x);
			ptPosition.x = xpos;
		}
	
		int step =  (logicalMax - logicalMin) / (rangeMax - rangeMin);

		if (abs(positionOffset) >= step)
		{
			if (positionOffset > 0)
			{
				SetPosition(GetPosition()+1);
			}
			else
			{
				SetPosition(GetPosition()-1);
			}

			if (bEndOnClick == false)
			{
				parent->PostMessage(CQE_SLIDER, (void*)controlID);
			}
			positionOffset = 0;
		}

/*		short pos;
		
		// the movement is not continueous, make it snap into place
		int step =  (logicalMax - logicalMin) / (rangeMax - rangeMin);

		if (pSliderType->bVertical)
		{
			pos = (ypos / step) * step;

			// if the position is outside the screen rect, then forget about it
			if (pos < logicalMax)
			{
				pos = logicalMax;
			}
			else if (pos > logicalMin)
			{
				pos = logicalMin;
			}
		}
		else
		{
			pos = (xpos / step) * step;

			// if the position is outside the screen rect, then forget about it
			if (pos > logicalMax)
			{
				pos = logicalMax;
			}
			else if (pos < logicalMin)
			{
				pos = logicalMin;
			}
		}

		// move the tab 
		int xdiff = 0, ydiff = 0;

		if (pSliderType->bVertical)
		{
			ydiff = (pos - ptPosition.y);
		}
		else
		{
			xdiff = (pos - ptPosition.x);
		}

		OffsetRect(&rcTab, xdiff, ydiff);

		// make sure that we are within the boundaries
		if (rcTab.left < 0)
		{
			OffsetRect(&rcTab, -rcTab.left, 0);
		}
		else if (rcTab.right >= screenRect.right - screenRect.left)
		{
			OffsetRect(&rcTab, (screenRect.right - screenRect.left) - rcTab.right, 0);
		}
		if (rcTab.top < 0)
		{
			OffsetRect(&rcTab, 0, -rcTab.top);
		}
		else if (rcTab.bottom >= screenRect.bottom - screenRect.top)
		{
			OffsetRect(&rcTab, 0, (screenRect.bottom - screenRect.top) - rcTab.bottom);
		}

		if (pSliderType->bVertical)
		{
			ptPosition.y = pos;
		}
		else
		{
			ptPosition.x = pos;
		}

		SINGLE ratio = SINGLE(pos - logicalMin)/SINGLE(logicalMax - logicalMin);

		if (ratio < 0) ratio = 0;
		if (ratio > 1) ratio = 1;

		position = rangeMin + ratio*(rangeMax - rangeMin);

		if (bEndOnClick == false)
		{
			parent->PostMessage(CQE_SLIDER, (void*)controlID);
		}
*/
	}
}
//--------------------------------------------------------------------------//
//
void Slider::setLogicalPosition (void)
{
 	SINGLE ratio = SINGLE(position - rangeMin)/SINGLE(rangeMax - rangeMin);
	short pos;

	if (ratio < 0)
	{
		ratio = 0;
		position = rangeMin;
		pos = logicalMin;
	}
	else if (ratio > 1)
	{
		ratio = 1;
		position = rangeMax;
		pos = logicalMax;
	}
	else
	{
		pos =  logicalMin + ratio*(logicalMax - logicalMin);
	}

	// place the tab rect correctly
	if (pSliderType->bVertical)
	{
		::SetRect(&rcTab, screenRect.left, (pos - pSliderType->tabWidth/2), screenRect.right, (pos + pSliderType->tabWidth/2));
		OffsetRect(&rcTab, -screenRect.left, -screenRect.top);
		ptPosition.y = pos;
	}
	else
	{
		::SetRect(&rcTab, pos - pSliderType->tabWidth/2, screenRect.top, pos + pSliderType->tabWidth/2, screenRect.bottom);
		OffsetRect(&rcTab, -screenRect.left, -screenRect.top);
		ptPosition.x = pos;
	}
}
//--------------------------------------------------------------------------//
//
void Slider::draw (void)
{
	COLORREF color;
	PANE pane;
	pane.window = 0;
	pane.x0 = screenRect.left;
	pane.y0 = screenRect.top;
	pane.x1 = screenRect.right;
	pane.y1 = screenRect.bottom;

	if (pSliderType->shapes[0] == NULL)
	{
		// ugly drawing using primitives set up by programmers
		// draw the backgound rect
		DA::RectangleFill(&pane, 0, 0, screenRect.right, screenRect.bottom, RGB(0,0,0));

		// draw the bounding rect outline
		int x1 = 0, y1 = 0;
		int x2 = screenRect.right - screenRect.left - 1;
		int y2 = screenRect.bottom - screenRect.top - 1;

		if (bDisabled)
		{
			color = RGB(pSliderType->colors[0].red, pSliderType->colors[0].green, pSliderType->colors[0].blue);
		}
		else if (bKeyboardFocus && bHasFocus)
		{
			color = RGB(pSliderType->colors[2].red, pSliderType->colors[2].green, pSliderType->colors[2].blue);
		}
		else if (bAlert)
		{
			color = RGB(pSliderType->colors[3].red, pSliderType->colors[3].green, pSliderType->colors[3].blue);
		}
		else
		{
			color = RGB(pSliderType->colors[1].red, pSliderType->colors[1].green, pSliderType->colors[1].blue);
		}

		// draw the border around the rect
		DA::LineDraw(&pane, x1, y1, x2, y1, color);
		DA::LineDraw(&pane, x2, y1, x2, y2, color);
		DA::LineDraw(&pane, x2, y2, x1, y2, color);
		DA::LineDraw(&pane, x1, y2, x1, y1, color);

		// draw the tab
		DA::RectangleHash(&pane, rcTab.left, rcTab.top, rcTab.right, rcTab.bottom, color);
		DA::LineDraw(&pane, rcTab.left, rcTab.top, rcTab.right, rcTab.top, color);
		DA::LineDraw(&pane, rcTab.right, rcTab.top, rcTab.right, rcTab.bottom, color);
		DA::LineDraw(&pane, rcTab.left, rcTab.bottom, rcTab.right, rcTab.bottom, color);
		DA::LineDraw(&pane, rcTab.left, rcTab.top, rcTab.left, rcTab.bottom, color);
	}
	else
	{
		// which shape id are we using here?
		int id;
		if (bDisabled)
		{
			// disabled 
			id = 0;
		}
		else if (bKeyboardFocus && bHasFocus)
		{
			// highlight
			id = 2;
		}
		else if (bAlert)
		{
			// mouse over
			id = 3;
		}
		else
		{
			// normal
			id = 1;
		}

		// drawing is specified by the art guys
		// draw the border
		pSliderType->shapes[id]->Draw(NULL, screenRect.left, screenRect.top);

		// draw the tab
		pSliderType->shapes[id+4]->Draw(&pane, rcTab.left, rcTab.top);
	}
}
//--------------------------------------------------------------------------//
//
void Slider::init (SLIDERTYPE * _pSliderType)
{
	COMPTR<IDAComponent> pBase;
	pSliderType = _pSliderType;
}

//--------------------------------------------------------------------------//
//-----------------------Slider Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE SliderFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(SliderFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	SliderFactory (void) { }

	~SliderFactory (void);

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
SliderFactory::~SliderFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void SliderFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE SliderFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_SLIDER)
	{
		GT_SLIDER * data = (GT_SLIDER *) _data;
		SLIDERTYPE * result = new SLIDERTYPE;

		result->pArchetype = pArchetype;

		result->colors[GTS_DISABLED] = data->disabledColor;
		result->colors[GTS_HIGHLIGHT] = data->highlightColor;
		result->colors[GTS_NORMAL] = data->normalColor;
		result->colors[GTS_ALERT] = data->alertColor;
		result->bVertical = data->bVertical;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				int i;
				for (i = 0; i < MAX_SLIDER_SHAPES; i++)
					CreateDrawAgent((VFX_SHAPETABLE *) pImage, i, result->shapes[i]);
			END_MAPPING(INTERFACEDIR);

			// the tab width is defined by art
			U16 dummyDimension;
			if (data->bVertical == false)
			{
				result->shapes[4]->GetDimensions(result->tabWidth, dummyDimension);
			}
			else
			{
				result->shapes[4]->GetDimensions(dummyDimension, result->tabWidth);
			}

			result->indent = IDEAL2REALX(data->indent);
		}
		else
		{
			// the tab width is defined by our macro
			if (data->bVertical == false)
			{
				result->tabWidth = IDEAL2REALX(TAB_WIDTH);
			}
			else
			{
				result->tabWidth = IDEAL2REALY(TAB_WIDTH);
			}
			result->indent = 0;
		}

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 SliderFactory::DestroyArchetype (HANDLE hArchetype)
{
	SLIDERTYPE * type = (SLIDERTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT SliderFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	SLIDERTYPE * type = (SLIDERTYPE *) hArchetype;
	Slider * result = new DAComponent<Slider>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _sliderfactory : GlobalComponent
{
	SliderFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<SliderFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _sliderfactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Slider.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//