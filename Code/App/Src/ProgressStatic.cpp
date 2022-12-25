//--------------------------------------------------------------------------//
//                                                                          //
//                               ProgressStatic.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ProgressStatic.cpp 4     10/13/00 10:47a Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include <DProgressStatic.h>
#include <wchar.h>

#include "IProgressStatic.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "DrawAgent.h"
#include "Sfx.h"

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
struct PROGRESS_STATICTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	COMPTR<IDrawAgent> shape;
	U16 width, height;

	GT_PROGRESS_STATIC::COLOR normalText, overText, background, background2;
	GT_PROGRESS_STATIC::drawtype backgroundDraw;
	bool bBackdraw;

	PROGRESS_STATICTYPE (void)
	{
	}

	~PROGRESS_STATICTYPE (void)
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
struct DACOM_NO_VTABLE ProgressStatic : BaseHotRect, IProgressStatic
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(ProgressStatic)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IProgressStatic)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PROGRESS_STATICTYPE * pProgressStaticType;
	COMPTR<IFontDrawAgent> pFont;
	COMPTR<IFontDrawAgent> pOverFont;
	COMPTR<IFontDrawAgent> pFontBackground;

	PRG_STTXT::PROGRESS_STATIC_TEXT staticText;		// can also be used for ID
	S16 xText, yText;			// offset from origin of control for text
	U16 width, height;

	PROGRESS_STATIC_DATA::alignmenttype alignment;

	wchar_t szText[1024];
	U32 rollupValue;
	U32 timer;
	static HSOUND HsndRollup;
	bool bNegative;

	U32 curProgress,curMax;

	//
	// class methods
	//

	ProgressStatic (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~ProgressStatic (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IProgressStatic methods  */

	virtual void InitProgressStatic (const PROGRESS_STATIC_DATA & data, BaseHotRect * parent, bool bHighPriority = false); 

	virtual void SetText (const wchar_t *szText); 

	virtual void SetTextID (const U32 textID);

	virtual int  GetText (wchar_t * szBuffer, int bufferSize);

	virtual void SetVisible (bool bVisible);

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

	virtual void SetProgress(U32 current, U32 max);

	/* ProgressStatic methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PROGRESS_STATICTYPE * _pProgressStaticType);

	void draw (void);

	void update (U32 dt);

	void setTextAlignment (void)
	{

		if (pFont)
		{
			if (alignment == PROGRESS_STATIC_DATA::CENTER)
			{
				yText = S32(height - pFont->GetFontHeight()) / 2;
				xText = S32(width - pFont->GetStringWidth(szText)) / 2;
			}
			else if (alignment == PROGRESS_STATIC_DATA::LEFT)
			{
				xText = IDEAL2REALX(2);
				yText = S32(height - pFont->GetFontHeight()) / 2;
			}
			else if (alignment == PROGRESS_STATIC_DATA::RIGHT)
			{
				// right adjust
				xText = (width - pFont->GetStringWidth(szText)) - IDEAL2REALX(4);
				yText = S32(height - pFont->GetFontHeight()) / 2;
			}
			else if (alignment == PROGRESS_STATIC_DATA::TOPLEFT)
			{
				// good for multi-line fonts
				yText = IDEAL2REALY(2);
				xText = IDEAL2REALX(2);
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
ProgressStatic::~ProgressStatic (void)
{
	GENDATA->Release(pProgressStaticType->pArchetype);

	// close the  sound handles
	if (SFXMANAGER && HsndRollup)
	{
		SFXMANAGER->CloseHandle(HsndRollup);
		HsndRollup = NULL;
	}
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::InitProgressStatic (const PROGRESS_STATIC_DATA & data, BaseHotRect * _parent, bool bHighPriority)
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

	if ((height = IDEAL2REALY(data.height)) == 0)
		if ((height = pProgressStaticType->height) == 0)
			height = parent->screenRect.bottom - parent->screenRect.top + 1;

	if ((width = IDEAL2REALX(data.width)) == 0)
		if ((width = pProgressStaticType->width) == 0)
			width = parent->screenRect.right - parent->screenRect.left + 1;

	CQASSERT(height && width);

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;

	//
	// calculate text offset
	//

	if (staticText != PRG_STTXT::NO_STATIC_TEXT && szText[0] == 0)
		wcsncpy(szText, _localLoadStringW(staticText), sizeof(szText)/sizeof(wchar_t));

	setTextAlignment();
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::SetText (const wchar_t *_szText)
{
	if (_szText == 0)
		szText[0] = 0;
	else
		wcsncpy(szText, _szText, sizeof(szText)/2 - 1);

	setTextAlignment();
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::SetTextID (const U32 textID)
{
	wchar_t buffer[256];
	wcsncpy(buffer, _localLoadStringW(textID), sizeof(buffer)/sizeof(wchar_t));

	SetText(buffer);
}
//--------------------------------------------------------------------------//
//
int ProgressStatic::GetText (wchar_t * szBuffer, int bufferSize)
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
void ProgressStatic::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::SetProgress(U32 current, U32 max)
{
	curProgress = current;
	curMax = max;
}
//--------------------------------------------------------------------------//
//
GENRESULT ProgressStatic::Notify (U32 message, void *param)
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
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::update (U32 dt)
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
void ProgressStatic::draw (void)
{
	SINGLE percent;
	if(curMax)
		percent = ((SINGLE)curProgress)/curMax;
	else
		percent = 1.0;

	if(percent > 1.0)
		percent = 1.0;
	else if(percent < 0)
		percent = 0;

	S32 percentWidth = percent*(screenRect.right-screenRect.left);

	PANE bkPane;
	bkPane.window = 0;
	bkPane.x0 = screenRect.left;
	bkPane.y0 = screenRect.top;
	bkPane.x1 = screenRect.left+percentWidth;
	bkPane.y1 = screenRect.bottom;
	
	if (pProgressStaticType->shape)
	{
		pProgressStaticType->shape->Draw(&bkPane,0,0);
	}
	else
	if (pProgressStaticType->backgroundDraw == GT_PROGRESS_STATIC::FILL)
	{
		const GT_PROGRESS_STATIC::COLOR & color = pProgressStaticType->background;
		DA::RectangleFill(&bkPane, 0, 0, screenRect.right-screenRect.left, screenRect.bottom-screenRect.top, RGB(color.red, color.green, color.blue));
		bkPane.x0= bkPane.x1;
		bkPane.x1 = screenRect.right;
		const GT_PROGRESS_STATIC::COLOR & color2 = pProgressStaticType->background2;
		DA::RectangleFill(&bkPane, -percentWidth, 0, screenRect.right-screenRect.left, screenRect.bottom-screenRect.top, RGB(color2.red, color2.green, color2.blue));
	}
	else
	if (pProgressStaticType->backgroundDraw == GT_PROGRESS_STATIC::HASH)
	{
		const GT_PROGRESS_STATIC::COLOR & color = pProgressStaticType->background;
		DA::RectangleHash(&bkPane, 0, 0, screenRect.right-screenRect.left, screenRect.bottom-screenRect.top, RGB(color.red, color.green, color.blue));
		bkPane.x0= bkPane.x1;
		bkPane.x1 = screenRect.right;
		const GT_PROGRESS_STATIC::COLOR & color2 = pProgressStaticType->background2;
		DA::RectangleHash(&bkPane, -percentWidth, 0, screenRect.right-screenRect.left, screenRect.bottom-screenRect.top, RGB(color2.red, color2.green, color2.blue));
	}

	if (pFont!=0 && szText[0])
	{
		if (pFontBackground)
		{
			PANE pane;
			pane.window = 0;
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

		PANE p1;
		p1.window = 0;
		p1.x0 = screenRect.left+xText;
		p1.x1 = screenRect.left+percentWidth;
		p1.y0 = screenRect.top+yText;
		p1.y1 = screenRect.bottom;
		if(p1.x1 > p1.x0)
		{
			pOverFont->StringDraw(&p1, 0, 0, szText);
		}
		p1.x0 = p1.x1;
		p1.x1 = screenRect.right;
		if(p1.x1 > p1.x0)
		{
			pFont->StringDraw(&p1, -percentWidth+xText, 0, szText);
		}
	}
}
//--------------------------------------------------------------------------//
//
void ProgressStatic::init (PROGRESS_STATICTYPE * _pProgressStaticType)
{
	COMPTR<IDAComponent> pBase;
	pProgressStaticType = _pProgressStaticType;

	if (pProgressStaticType->pFontType)
	{
		GENDATA->CreateInstance(pProgressStaticType->pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
		pFont->SetFontColor(RGB(pProgressStaticType->normalText.red, pProgressStaticType->normalText.green, pProgressStaticType->normalText.blue) | 0xFF000000, 0);

		GENDATA->CreateInstance(pProgressStaticType->pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pOverFont);
		pOverFont->SetFontColor(RGB(pProgressStaticType->overText.red, pProgressStaticType->overText.green, pProgressStaticType->overText.blue) | 0xFF000000, 0);

		// create the background font
		if (pProgressStaticType->bBackdraw)
		{
			GENDATA->CreateInstance(pProgressStaticType->pFontType, pBase);
			CQASSERT(pBase!=0);
			pBase->QueryInterface("IFontDrawAgent", pFontBackground);
			pFontBackground->SetFontColor(0 | 0xFF000000, 0);
		}
	}
}
//--------------------------------------------------------------------------//
//
HSOUND	ProgressStatic::HsndRollup;

//--------------------------------------------------------------------------//
//-----------------------Static Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ProgressStaticFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ProgressStaticFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ProgressStaticFactory (void) { }

	~ProgressStaticFactory (void);

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
ProgressStaticFactory::~ProgressStaticFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ProgressStaticFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ProgressStaticFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_PROGRESS_STATIC)
	{
		GT_PROGRESS_STATIC * data = (GT_PROGRESS_STATIC *) _data;
		PROGRESS_STATICTYPE * result = new PROGRESS_STATICTYPE;

		result->pArchetype = pArchetype;
		if (data->fontName[0])
		{
			result->pFontType = GENDATA->LoadArchetype(data->fontName);
			CQASSERT(result->pFontType);
			GENDATA->AddRef(result->pFontType);
		}

		result->normalText = data->normalText;
		result->overText = data->overText;
		result->background = data->background;
		result->background2 = data->background2;
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
BOOL32 ProgressStaticFactory::DestroyArchetype (HANDLE hArchetype)
{
	PROGRESS_STATICTYPE * type = (PROGRESS_STATICTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ProgressStaticFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	PROGRESS_STATICTYPE * type = (PROGRESS_STATICTYPE *) hArchetype;
	ProgressStatic * result = new DAComponent<ProgressStatic>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _ProgressStaticFactory : GlobalComponent
{
	ProgressStaticFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ProgressStaticFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _ProgressStaticFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End ProgressStaticFactory.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
