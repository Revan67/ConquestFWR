//--------------------------------------------------------------------------//
//                                                                          //
//                               HotStatic.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/HotStatic.cpp 15    7/31/00 2:56p Rmarr $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "Startup.h"
#include "IHotStatic.h"
#include "DHotStatic.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "IShapeLoader.h"
#include "DrawAgent.h"


#define STATICTECH_STRLEN 256

struct HotStaticArchetype
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	~HotStaticArchetype()
	{
		if(pFontType)
			GENDATA->Release(pFontType);
	}
};

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HotStatic : BaseHotRect, IHotStatic
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(HotStatic)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IHotStatic)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PGENTYPE pArchetype;

	wchar_t szTechText[STATICTECH_STRLEN];
	U32 lastText;
	S16 xText, yText;
	U16 width, height;

	S16 barStartX,barStartY;
	U16 barWidth,barHeight,barSpacing;

	COMPTR<IFontDrawAgent> pFont;
	COMPTR<IDrawAgent> fullShape, emptyShape;


	U32 techLevel;
	U32 numTechLevels;

	//
	// class methods
	//

	HotStatic (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~HotStatic (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IHotStatic methods  */

	virtual void InitHotStatic (const HOTSTATIC_DATA & data, BaseHotRect * _parent, IShapeLoader * loader);

	virtual U32 GetImageLevel();

	virtual void SetImageLevel(U32 newTechLevel, U32 newNumTechLevels);

	virtual void SetTextString(HSTTXT::STATIC_TEXT text);

	virtual void SetVisible(bool bVisible);

	/* BaseHotRect methods */
	
	/*HotStatic methods*/
	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (HotStaticArchetype * archetype);

	void draw();

};
//--------------------------------------------------------------------------//
//
HotStatic::~HotStatic (void)
{
	GENDATA->Release(pArchetype);
}
//--------------------------------------------------------------------------//
//
void HotStatic::InitHotStatic (const HOTSTATIC_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	wcsncpy(szTechText, _localLoadStringW(data.text), STATICTECH_STRLEN);
	lastText = data.text;

	U32 baseImage = data.baseImage;
	if(baseImage == 0)
		baseImage++;
	loader->CreateDrawAgent(baseImage, fullShape);
	loader->CreateDrawAgent(baseImage+1, emptyShape);

	fullShape->GetDimensions(barWidth,barHeight);
	barWidth = barWidth;
	barHeight = barHeight;
	barSpacing = IDEAL2REALX(data.barSpacing);

	width = IDEAL2REALX(data.width);
	height = IDEAL2REALY(data.height);

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;

	barStartX = screenRect.left+IDEAL2REALX(data.barStartX);
	barStartY = screenRect.top+(((S32)height)-((S32)barHeight))/2;

	techLevel = 0;
	numTechLevels = 1;

	if(pFont)
	{
		pFont->SetFontColor(RGB(data.textColor.red, data.textColor.green, data.textColor.blue) | 0xFF000000, 0);

		yText = S32(height - pFont->GetFontHeight()) / 2;
		xText = IDEAL2REALX(2);
	}

}

U32 HotStatic::GetImageLevel()
{
	return techLevel;
}

void HotStatic::SetImageLevel(U32 newTechLevel, U32 newNumTechLevels)
{
	CQASSERT(techLevel >= 0);
	techLevel = newTechLevel;
	numTechLevels = newNumTechLevels;
}

void HotStatic::SetTextString(HSTTXT::STATIC_TEXT text)
{
	if(text != (S32)lastText)
	{
		lastText = text;
		if( text != HSTTXT::NOTEXT)
			wcsncpy(szTechText, _localLoadStringW(text), STATICTECH_STRLEN);
		else
			szTechText[0] = 0;
	}
}

void HotStatic::SetVisible(bool bVisible)
{
	bInvisible = !bVisible;
}

GENRESULT HotStatic::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	}

	return BaseHotRect::Notify(message, param);
}

void HotStatic::init(HotStaticArchetype * archetype)
{
	pArchetype = archetype->pArchetype;
	if (archetype->pFontType)
	{
		COMPTR<IDAComponent> pBase;
		GENDATA->CreateInstance(archetype->pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
	}

}

void HotStatic::draw()
{
	if((numTechLevels >1) && lastText != HSTTXT::NOTEXT)
	{
		for(U32 i = 0; i < numTechLevels-1; ++i)
		{
			if(i < techLevel)
				fullShape->Draw(0,barStartX+((barWidth+barSpacing)*i),barStartY);
			else
				emptyShape->Draw(0,barStartX+((barWidth+barSpacing)*i),barStartY);
		}
		if (pFont!=0 && szTechText[0])
		{
			PANE pane;

			pane.window = 0;
			pane.x0 = screenRect.left+xText;
			pane.x1 = screenRect.right;
			pane.y0 = screenRect.top+yText;
			pane.y1 = screenRect.bottom;

			pFont->StringDraw(&pane, 0, 0, szTechText);
		}
	}
}

//--------------------------------------------------------------------------//
//-----------------------HotStatic Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HotStaticFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(HotStaticFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	HotStaticFactory (void) { }

	~HotStaticFactory (void);

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
HotStaticFactory::~HotStaticFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void HotStaticFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE HotStaticFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTSTATIC)
	{
		GT_HOTSTATIC * data = (GT_HOTSTATIC *) _data;
		HotStaticArchetype * archetype = new HotStaticArchetype;
		if (data->fontType[0])
		{
			archetype->pFontType = GENDATA->LoadArchetype(data->fontType);
			CQASSERT(archetype->pFontType);
			GENDATA->AddRef(archetype->pFontType);
		}
		archetype->pArchetype = _pArchetype;
		return archetype;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 HotStaticFactory::DestroyArchetype (HANDLE hArchetype)
{
	delete (HotStaticArchetype *)hArchetype;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT HotStaticFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	HotStatic * result = new DAComponent<HotStatic>;

	result->init((HotStaticArchetype *)hArchetype);
	*pInstance = result->GetBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _HotStaticfactory : GlobalComponent
{
	HotStaticFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<HotStaticFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _HotStaticfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End HotButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
