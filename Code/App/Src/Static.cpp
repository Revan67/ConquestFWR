//--------------------------------------------------------------------------//
//                                                                          //
//                               Static.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Static.cpp 43    10/22/00 10:26p Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include <DStatic.h>
#include <wchar.h>

#include "IStatic.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "DrawAgent.h"
#include "Sfx.h"
#include "IButton2.h"

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


// rollup should only take n thousand milliseconds to complete
#define MAX_ROLLUP_TIMER 8000	

//--------------------------------------------------------------------------//
//
struct STATICTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	COMPTR<IDrawAgent> shape;
	U16 width, height;

	GT_STATIC::COLOR normalText, background;
	GT_STATIC::drawtype backgroundDraw;
	bool bBackdraw;

	STATICTYPE (void)
	{
	}

	~STATICTYPE (void)
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
struct DACOM_NO_VTABLE Static : BaseHotRect, IStatic
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Static)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IStatic)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	STATICTYPE * pStaticType;
	COMPTR<IFontDrawAgent> pFont;
	COMPTR<IFontDrawAgent> pFontBackground;

	STTXT::STATIC_TEXT staticText;		// can also be used for ID
	S16 xText, yText;			// offset from origin of control for text
	U16 width, height;

	STATIC_DATA::alignmenttype alignment;

	wchar_t *szText;
	U32 rollupValue;
	U32 timer;
	static HSOUND HsndRollup;
	bool bNegative;

	// our buddy control...
	COMPTR<IButton2> buddyControl;
	bool bLeftDown;


	U32 staticTooltip;

	//
	// class methods
	//

	Static (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		staticTooltip = 0;
	}

	virtual ~Static (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IStatic methods  */

	virtual void InitStatic (const STATIC_DATA & data, BaseHotRect * parent, bool bHighPriority = false); 

	virtual void SetText (const wchar_t *szText); 

	virtual void SetTextID (const U32 textID);

	virtual int  GetText (wchar_t * szBuffer, int bufferSize);

	virtual void SetVisible (bool bVisible);

	virtual void SetTextColor (U8 red, U8 green, U8 blue);

	virtual void SetTextColor (COLORREF color);

	virtual void EnableRollupBehavior (int value)	// the control will display text rolling up to the value given
	{
		if (value == 0)
		{
			SetText(L"0");
		}
		if (value < 0)
		{
			bNegative = true;
			rollupValue = -value;
		}
		else
		{
			bNegative = false;
			rollupValue = value;
		}
	}

	virtual void SetBuddyControl (IButton2 * buddy)
	{
		buddyControl = buddy;
	}

	virtual const U32  GetStringWidth (void) const; // get the width of the string that is being used by this control

	/* BaseHotRect methods */
	
	virtual void setStatus (void);

	/* Static methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (STATICTYPE * _pStaticType);

	void draw (void);

	void update (U32 dt);

	void setTextAlignment (void)
	{

		if ((pFont!=0) && (szText!=0))
		{
			if (alignment == STATIC_DATA::CENTER)
			{
				if (pFont->IsMultiLine() == false || pFont->GetStringWidth(szText) <= width)
				{
					yText = S32(height - pFont->GetFontHeight()) / 2;
					xText = S32(width - pFont->GetStringWidth(szText)) / 2;
				}
				else
				{
					xText = IDEAL2REALX(2);	

					// how may rows of text will we need here?
					if (width != 0)
					{
						PANE pane;
						pane.x0 = 0;
						pane.x1 = width;
						pane.y0 = 0;
						pane.y1 = height;
						//U32 stringWidth = pFont->GetStringWidth(szText);
						U32 nRows = pFont->GetNumRows(szText, &pane); //stringWidth/width;
						yText = (height - nRows * pFont->GetFontHeight()) / 2;
					}
					else
					{
						yText = IDEAL2REALY(2);
					}
				}
			}
			else if (alignment == STATIC_DATA::LEFT)
			{
				xText = IDEAL2REALX(2);
				yText = S32(height - pFont->GetFontHeight()) / 2;
			}
			else if (alignment == STATIC_DATA::RIGHT)
			{
				// right adjust
				xText = (width - pFont->GetStringWidth(szText)) - IDEAL2REALX(4);
				yText = S32(height - pFont->GetFontHeight()) / 2;
			}
			else if (alignment == STATIC_DATA::TOPLEFT)
			{
				// good for multi-line fonts
				yText = IDEAL2REALY(2);
				xText = IDEAL2REALX(2);
			}
			else if (alignment == STATIC_DATA::TENBYSIX)
			{
				// not good for much, but needed anyway
				yText = IDEAL2REALY(6);
				xText = IDEAL2REALX(10);				
			}
			else if (alignment == STATIC_DATA::LEFTSIX)
			{
				yText = S32(height - pFont->GetFontHeight()) / 2;
				xText = IDEAL2REALX(6);				
			}
		}
		else
		{
			xText = IDEAL2REALX(4);
			yText = IDEAL2REALY(2);		// make this a data item?
		}
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Static::~Static (void)
{
	GENDATA->Release(pStaticType->pArchetype);

	// close the  sound handles
	if (SFXMANAGER && HsndRollup)
	{
		SFXMANAGER->CloseHandle(HsndRollup);
		HsndRollup = NULL;
	}

	::free(szText);
}
//--------------------------------------------------------------------------//
//
void Static::InitStatic (const STATIC_DATA & data, BaseHotRect * _parent, bool bHighPriority)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		if (bHighPriority)
		{
			parent->SetCallbackPriority(this, EVENT_PRIORITY_STATIC_HIGH);
		}
		else
		{
			parent->SetCallbackPriority(this, EVENT_PRIORITY_STATIC);
		}
	}

	staticText = data.staticText;		// can be 0
	alignment = data.alignment;
	staticTooltip = data.staticTooltip;
	hintboxID = data.staticHintbox;

	if ((height = IDEAL2REALY(data.height)) == 0)
		if ((height = pStaticType->height) == 0)
			height = parent->screenRect.bottom - parent->screenRect.top + 1;

	if ((width = IDEAL2REALX(data.width)) == 0)
		if ((width = pStaticType->width) == 0)
			width = parent->screenRect.right - parent->screenRect.left + 1;

	CQASSERT(height && width);

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;

	//
	// calculate text offset
	//

	//if (staticText != STTXT::NO_STATIC_TEXT && szText[0] == 0)
	//	wcsncpy(szText, _localLoadStringW(staticText), sizeof(szText)/sizeof(wchar_t));
	if (staticText != STTXT::NO_STATIC_TEXT && szText == 0)
		szText = _wcsdup(_localLoadStringW(staticText));

	setTextAlignment();
}
//--------------------------------------------------------------------------//
//
void Static::SetText (const wchar_t *_szText)
{
	if (_szText == 0 || _szText[0] == 0)
	{
		::free(szText);
		szText = 0;
	}
	else
	{
		::free(szText);
		szText = _wcsdup(_szText);
		//wcsncpy(szText, _szText, sizeof(szText)/2 - 1);
	}

	setTextAlignment();
}
//--------------------------------------------------------------------------//
//
void Static::SetTextID (const U32 textID)
{
	wchar_t buffer[256];
	wcsncpy(buffer, _localLoadStringW(textID), sizeof(buffer)/sizeof(wchar_t));

	SetText(buffer);
}
//--------------------------------------------------------------------------//
//
int Static::GetText (wchar_t * szBuffer, int bufferSize)
{
	if (szBuffer==0)
	{
		return wcslen(szText);
	}
	else
	{
		bufferSize /= sizeof(wchar_t);
		int len = wcslen(szText);

		len = __min(len+1, bufferSize);
		memcpy(szBuffer, szText, len*sizeof(wchar_t));

		return len-1;
	}
}
//--------------------------------------------------------------------------//
//
void Static::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
void Static::SetTextColor (U8 red, U8 green, U8 blue)
{
	if (pFont)
		pFont->SetFontColor(RGB(red, green, blue) | 0xFF000000, 0);
	
	if (pFontBackground)
		pFontBackground->SetFontColor(0, 0);
}
//--------------------------------------------------------------------------//
//
const U32 Static::GetStringWidth (void) const // get the width of the string that is being used by this control
{
	// sean is here
	if (pFont && szText)
	{
		return pFont->GetStringWidth(szText);
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
void Static::setStatus (void)
{
	wchar_t szToolText[256];
	
	if(staticTooltip)
	{
		wcsncpy(szToolText, _localLoadStringW(staticTooltip), sizeof(szToolText)/sizeof(wchar_t));

		STATUS->SetTextString(szToolText, STM_TOOLTIP);
	}
	else
	{
		STATUS->SetDefaultState();
	}
	STATUS->SetRect(screenRect);
}
//--------------------------------------------------------------------------//
//
void Static::SetTextColor (COLORREF color)
{
	if (pFont)
		pFont->SetFontColor(color | 0xFF000000, 0);
	
	if (pFontBackground)
		pFontBackground->SetFontColor(0, 0);
}
//--------------------------------------------------------------------------//
//
GENRESULT Static::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case CQE_UPDATE:
		if (rollupValue)
		{
			update(U32(param) >> 10);
		}
		break;

	case CQE_SET_FOCUS:
		if (rollupValue)
		{
		}
		break;

	case CQE_KILL_FOCUS:
		if (rollupValue)
		{
		}
		break;

	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert && staticTooltip)
		{
			desiredOwnedFlags = (RF_STATUS|RF_HINTBOX);
			grabAllResources();
		}
		else
		if (actualOwnedFlags)
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;

	case WM_LBUTTONDOWN:
		if (bAlert && buddyControl != 0)
		{
			COMPTR<IEventCallback> event;
			buddyControl->QueryInterface("IEventCallback", event);
			
			if (event)
			{
				bLeftDown = true;
				buddyControl->ForceAlertState(true);
				GENRESULT result = event->Notify(message, param);
				buddyControl->ForceAlertState(false);
				return result;
			}
		}
		break;

	case WM_LBUTTONUP:
		if (bAlert && buddyControl != 0 && bLeftDown)
		{
			COMPTR<IEventCallback> event;
			buddyControl->QueryInterface("IEventCallback", event);
			
			if (event)
			{
				buddyControl->ForceAlertState(true);
				buddyControl->ForceMouseDownState(true);
				GENRESULT result = event->Notify(message, param);
				buddyControl->ForceAlertState(false);
				bLeftDown = false;
				return result;
			}
		}
		bLeftDown = false;
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Static::update (U32 dt)
{
	CQASSERT(rollupValue);
	timer += dt;

	// play the sound
	if (SFXMANAGER)
	{
		if (HsndRollup == NULL)
		{
			// load the sounds
			HsndRollup = SFXMANAGER->Open(SFX::TELETYPE);
//			SFXMANAGER->Play(HsndRollup, 0, 0, SFXPLAYF_LOOPING|SFXPLAYF_NORESTART);
		}
		SFXMANAGER->Play(HsndRollup, 0, 0);
	}

	// as soon the displayed value reaches the rollupValue than set rollupValue back down to zero to stop updating
	wchar_t szDisplay[16];
	U32 valueDisplay = 0;

	// can we rollup at a constant speed of 1000 units/second or do we have to go faster?
	SINGLE speed = 1.0f;
	if (rollupValue > MAX_ROLLUP_TIMER)
	{
		// need a faster speed
		speed = float(rollupValue)/float(MAX_ROLLUP_TIMER);
	}

	// calculate the value to be displayed
	valueDisplay = timer*speed;

	if (valueDisplay >= rollupValue)
	{
		valueDisplay = rollupValue;

		// if this is the control with the highest value then stop the sound
//		if (rollupValue == HighestValue)
//		{
//			SFXMANAGER->Stop(HsndRollup);
//		}
		
		// this ends the rolling up
		rollupValue = 0;
	}

	// okay, we have the value.  Translate it into a string
	if (bNegative)
	{
		swprintf(szDisplay, L"-%u", valueDisplay);
	}
	else
	{
		swprintf(szDisplay, L"%u", valueDisplay);
	}
	SetText(szDisplay);
}
//--------------------------------------------------------------------------//
//
void Static::draw (void)
{
	if (pStaticType->shape)
	{
		pStaticType->shape->Draw(0, screenRect.left, screenRect.top);
	}
	else if (pStaticType->backgroundDraw == GT_STATIC::FILL)
	{
		const GT_STATIC::COLOR & color = pStaticType->background;
		DA::RectangleFill(0, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom, RGB(color.red, color.green, color.blue));
	}
	else if (pStaticType->backgroundDraw == GT_STATIC::HASH)
	{
		const GT_STATIC::COLOR & color = pStaticType->background;
		DA::RectangleHash(0, screenRect.left, screenRect.top, screenRect.right, screenRect.bottom, RGB(color.red, color.green, color.blue));
	}

	if (pFont!=0 && szText)
	{
		PANE pane;
		pane.window = 0;

		if (pFontBackground)
		{
			int dx = IDEAL2REALX(2);
			int dy = IDEAL2REALY(2);

			pane.x0 = screenRect.left+xText + dx;
			pane.x1 = screenRect.right + dx;
			pane.y0 = screenRect.top+yText + dy;
			pane.y1 = screenRect.bottom + dy;

			pFontBackground->StringDraw(&pane, 0, 0, szText);
			pane.x0 -= dx;
			pane.x1 -= dx;
			pane.y0 -= dy;
			pane.y1 -= dy;
		}
		else
		{
			pane.x0 = screenRect.left+xText;
			pane.x1 = screenRect.right;
			pane.y0 = screenRect.top+yText;
			pane.y1 = screenRect.bottom;
		}

		pFont->StringDraw(&pane, 0, 0, szText);
	}
}
//--------------------------------------------------------------------------//
//
void Static::init (STATICTYPE * _pStaticType)
{
	COMPTR<IDAComponent> pBase;
	pStaticType = _pStaticType;

	if (pStaticType->pFontType)
	{
		GENDATA->CreateInstance(pStaticType->pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
		pFont->SetFontColor(RGB(pStaticType->normalText.red, pStaticType->normalText.green, pStaticType->normalText.blue) | 0xFF000000, 0);

		// create the background font
		if (pStaticType->bBackdraw)
		{
			GENDATA->CreateInstance(pStaticType->pFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pFontBackground);
			pFontBackground->SetFontColor(0 | 0xFF000000, 0);
		}
	}
}
//--------------------------------------------------------------------------//
//
HSOUND	Static::HsndRollup;

//--------------------------------------------------------------------------//
//-----------------------Static Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE StaticFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(StaticFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	StaticFactory (void) { }

	~StaticFactory (void);

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

	/* StaticFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
StaticFactory::~StaticFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void StaticFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE StaticFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_STATIC)
	{
		GT_STATIC * data = (GT_STATIC *) _data;
		STATICTYPE * result = new STATICTYPE;

		result->pArchetype = pArchetype;
		if (data->fontName[0])
		{
			result->pFontType = GENDATA->LoadArchetype(data->fontName);
			CQASSERT(result->pFontType);
			GENDATA->AddRef(result->pFontType);
		}

		result->normalText = data->normalText;
		result->background = data->background;
		result->backgroundDraw = data->backgroundDraw;
		result->bBackdraw = data->bBackdraw;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			CreateDrawAgent(data->shapeFile, INTERFACEDIR, DA::UNKTYPE, 0, result->shape);
			result->shape->GetDimensions(result->width, result->height);
		}

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 StaticFactory::DestroyArchetype (HANDLE hArchetype)
{
	STATICTYPE * type = (STATICTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT StaticFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	STATICTYPE * type = (STATICTYPE *) hArchetype;
	Static * result = new DAComponent<Static>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _StaticFactory : GlobalComponent
{
	StaticFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<StaticFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _StaticFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Static.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
