//--------------------------------------------------------------------------//
//                                                                          //
//                               Icon.cpp                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Icon.cpp 5     9/01/00 7:24p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IIcon.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DIcon.h>
#include "DrawAgent.h"
#include "IShapeLoader.h"

//#include "VideoSurface.h"

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

//--------------------------------------------------------------------------//
//
struct ICONTYPE
{
	PGENTYPE pArchetype;

	ICONTYPE (void)
	{
	}

	~ICONTYPE (void)
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
struct DACOM_NO_VTABLE Icon : BaseHotRect, IIcon
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Icon)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IIcon)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	ICONTYPE * pIconType;
	COMPTR<IDrawAgent> shape;

	U32 tooltip;

	//
	// class methods
	//

	Icon (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		tooltip = 0;
	}

	virtual ~Icon (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IIcon methods  */

	virtual void InitIcon (const ICON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void SetVisible (bool bVisible);

	/* BaseHotRect methods */
	
	virtual void setStatus (void);

	/* Icon methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (ICONTYPE * _pIconType);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Icon::~Icon (void)
{
	GENDATA->Release(pIconType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void Icon::InitIcon (const ICON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, 0x40000000);
	}

	U32 baseImage = data.baseImage;
	if(baseImage == 0)
		++baseImage;
	loader->CreateDrawAgent(baseImage, shape);

	tooltip = data.tooltip;

	U16 width,height;
	shape->GetDimensions(width,height);

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;
}
//--------------------------------------------------------------------------//
//
void Icon::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
void Icon::setStatus (void)
{
	wchar_t szToolText[256];
	
	if(tooltip)
	{
		wcsncpy(szToolText, _localLoadStringW(tooltip), sizeof(szToolText)/sizeof(wchar_t));

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
GENRESULT Icon::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;
	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert && tooltip)
		{
			desiredOwnedFlags = (RF_STATUS);
			grabAllResources();
		}
		else
		if (actualOwnedFlags)
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Icon::draw (void)
{
	if (shape)
		shape->Draw(0, screenRect.left, screenRect.top);
}
//--------------------------------------------------------------------------//
//
void Icon::init (ICONTYPE * _pIconType)
{
	COMPTR<IDAComponent> pBase;
	pIconType = _pIconType;
}

//--------------------------------------------------------------------------//
//-----------------------Icon Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IconFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(IconFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	IconFactory (void) { }

	~IconFactory (void);

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

	/* IconFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
IconFactory::~IconFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void IconFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE IconFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_ICON)
	{
//		GT_ICON * data = (GT_ICON *) _data;
		ICONTYPE * result = new ICONTYPE;

		result->pArchetype = pArchetype;

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 IconFactory::DestroyArchetype (HANDLE hArchetype)
{
	ICONTYPE * type = (ICONTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT IconFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	ICONTYPE * type = (ICONTYPE *) hArchetype;
	Icon * result = new DAComponent<Icon>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _IconFactory : GlobalComponent
{
	IconFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<IconFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _IconFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Icon.cpp-----------------------------------------//
//-----------------------------------------------------------------------------------------//
