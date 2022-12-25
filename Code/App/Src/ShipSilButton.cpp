//--------------------------------------------------------------------------//
//                                                                          //
//                            ShipSilButton.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ShipSilButton.cpp 24    9/28/01 1:45p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IShipSilButton.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DShipSilButton.h>
#include "DrawAgent.h"
#include "IShapeLoader.h"
#include "MPart.h"
#include "Objlist.h"

#include <TComponent.h>
#include <FileSys.h>

#define SHIPSIL_MAX_SHAPES 3

#define SHIPSIL_RED 2
#define SHIPSIL_YELLOW 1
#define SHIPSIL_GREEN 0

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ShipSilButton : BaseHotRect, IShipSilButton
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(ShipSilButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IShipSilButton)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PGENTYPE pArchetype;
	SHIPSILBUTTON_DATA * pData;
	COMPTR<IDrawAgent> shapes[SHIPSIL_MAX_SHAPES];
	COMPTR<IShapeLoader> shapeLoader;
	GT_SHIPSILBUTTON * gtData;
	OBJPTR<IBaseObject> ship;
	bool bPressing:1;
//	bool bPostMessageBlocked:1;

	U32 controlID;					// set by owner


	bool bPostMessageBlocked;

	//
	// class methods
	//

	ShipSilButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~ShipSilButton (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IShipSilButton methods  */

	virtual void InitShipSilButton (SHIPSILBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void SetShip(IBaseObject * ship);

	virtual IBaseObject * GetShip();

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);
	
	virtual void SetControlID (U32 id);

	virtual U32 GetControlID (void);

	/* BaseHotRect methods */
	virtual void setStatus (void);
	
	/* HotButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE _pArchetype, GT_SHIPSILBUTTON * _data);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDown (void)
	{
		if(bAlert)
		{
			bPressing = true;
			
		}
	}

	void onLeftButtonUp (void)
	{
		if(bAlert && bPressing)
		{
			if(ship->bSelected)
			{
				bPressing = false;
				if((HOTKEY->GetVkeyState(VK_LSHIFT) == HKEF_PRESSED) ||
					(HOTKEY->GetVkeyState(VK_RSHIFT) == HKEF_PRESSED))
				{
					OBJLIST->UnselectObject(ship);
				}
				else
				{
					IBaseObject * obj = OBJLIST->GetSelectedList();
					while(obj)
					{
						IBaseObject * nextObj = obj->nextSelected;
						if(obj != ship)
						{
							OBJLIST->UnselectObject(obj);
						}
						obj = nextObj;
					}
				}
			}
		}
		else if(bPressing)//I need to release now but take no action.
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		bPressing = false;
	}

};
//--------------------------------------------------------------------------//
//
ShipSilButton::~ShipSilButton (void)
{
	GENDATA->Release(pArchetype);
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::InitShipSilButton (SHIPSILBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	pData = &data;

	shapeLoader = loader;
	controlID = 0;
	bPressing = 0;
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::SetShip(IBaseObject * _ship)
{
	if(ship != 0)
	{
		int i;
		for (i = 0; i < SHIPSIL_MAX_SHAPES; i++)
			shapes[i] = 0;

	}
	if (_ship)
		_ship->QueryInterface(IBaseObjectID, ship, NONSYSVOLATILEPTR);
	else
		ship=0;
	//
	// load the shapes
	//
	MPart part = ship;
	if(part.isValid())
	{
		int base;
		if ((base = part.pInit->silhouetteImage) == 0)
			base++;
		
		int i;
		for (i = 0; i < SHIPSIL_MAX_SHAPES; i++)
			shapeLoader->CreateDrawAgent(i+base, shapes[i]);
	
		U16 width, height;
		if (shapes[0])
			shapes[0]->GetDimensions(width, height);
		else
		{
			width = height = 50;
		}
	
		screenRect.left = IDEAL2REALX(pData->xOrigin) + parent->screenRect.left;
		screenRect.top = IDEAL2REALY(pData->yOrigin) + parent->screenRect.top;
		screenRect.bottom = screenRect.top + height - 1;
		screenRect.right = screenRect.left + width - 1;

	}
	bPressing = false;
	desiredOwnedFlags = 0;
	releaseResources();
		
}
//--------------------------------------------------------------------------//
//
IBaseObject * ShipSilButton::GetShip()
{
	return ship;
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;

	if (bInvisible)
	{
		desiredOwnedFlags = 0;
		releaseResources();
	}
}
//--------------------------------------------------------------------------//
//
bool ShipSilButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 ShipSilButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT ShipSilButton::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==0 && (ship != 0))
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bAlert)
		{
			desiredOwnedFlags = (RF_STATUS|RF_CURSOR);
			grabAllResources();
		}
		else
		if (actualOwnedFlags && !bPressing)
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		else
		if (bPressing)
		{
			desiredOwnedFlags = RF_CURSOR;
			releaseResources();
		}
		break;

	case WM_LBUTTONUP:
		BaseHotRect::Notify(message,param);
		onLeftButtonUp();
		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		onLeftButtonDown();
		return GR_OK;

	case CQE_UPDATE:
		bPostMessageBlocked = false;
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::draw (void)
{
	U32 state;
	MPart part = ship;
	if(part->hullPoints < part->hullPointsMax * gtData->redYellowBreak)
	{
		state = SHIPSIL_RED;
	}
	else if(part->hullPoints < part->hullPointsMax * gtData->yellowGreenBreak)
	{
		state = SHIPSIL_YELLOW;
	}
	else
		state = SHIPSIL_GREEN;
	if (shapes[state])
		shapes[state]->Draw(0, screenRect.left, screenRect.top);

	U32 barHeight = IDEAL2REALX(2);
	U32 barOffset = IDEAL2REALX(6);
	if(part->hullPointsMax)
	{
		DA::RectangleHash(NULL,screenRect.left+barOffset,screenRect.top,screenRect.right-barOffset,screenRect.top+barHeight,0);
		U32 hWidth = (part->hullPoints*((screenRect.right-screenRect.left)-(barOffset*2)))/part->hullPointsMax;
		DA::RectangleFill(NULL,screenRect.left+barOffset,screenRect.top,screenRect.left+barOffset+hWidth,screenRect.top+barHeight,RGB(255,0,0));
	}

	if(part->supplyPointsMax && (!(part->mObjClass == M_HARVEST || part->mObjClass == M_SIPHON || part->mObjClass == M_GALIOT)))
	{
		DA::RectangleHash(NULL,screenRect.left+barOffset,screenRect.bottom-barHeight,screenRect.right-barOffset,screenRect.bottom,0);
		U32 hWidth = (part->supplies*((screenRect.right-screenRect.left)-(barOffset*2)))/part->supplyPointsMax;
		if(ship->fieldFlags.suppliesLocked())
			DA::RectangleFill(NULL,screenRect.left+barOffset,screenRect.bottom-barHeight,screenRect.left+barOffset+hWidth,screenRect.bottom,RGB(180,180,255));
		else
			DA::RectangleFill(NULL,screenRect.left+barOffset,screenRect.bottom-barHeight,screenRect.left+barOffset+hWidth,screenRect.bottom,RGB(0,0,255));
	}
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::setStatus (void)
{
	if (ship != 0)
	{
		wchar_t buffer[256];
		MPart part = ship;
		if(part->bShowPartName)
			_localAnsiToWide(part->partName,buffer,sizeof(buffer));
		else
		{
			wchar_t * ptr;
			wcsncpy(buffer, _localLoadStringW(part.pInit->displayName), sizeof(buffer)/sizeof(wchar_t));

			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				*ptr = 0;
			}
			if ((ptr = wcschr(buffer, '(')) != 0)
			{
				*ptr = 0;
			}
		}

		wchar_t * namePtr = buffer;
		if(namePtr[0] == '#')
		{
			++namePtr;
			if ((namePtr = wcschr(namePtr,'#')) != 0)
				++namePtr;
			else
				namePtr = buffer;
		}
				
		STATUS->SetTextString(namePtr, STM_NAME);
	}
	else
	if (statusTextID)
		STATUS->SetText(statusTextID, STM_DEFAULT);
	else
		STATUS->SetDefaultState();

	STATUS->SetRect(screenRect);
}
//--------------------------------------------------------------------------//
//
void ShipSilButton::init (PGENTYPE _pArchetype, GT_SHIPSILBUTTON * _data)
{
	gtData = _data;
	pArchetype = _pArchetype;
	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//-----------------------ShipSilButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ShipSilButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ShipSilButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ShipSilButtonFactory (void) { }

	~ShipSilButtonFactory (void);

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
ShipSilButtonFactory::~ShipSilButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ShipSilButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ShipSilButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_SHIPSILBUTTON)
	{
		GT_SHIPSILBUTTON * data = (GT_SHIPSILBUTTON *) _data;
	
		CQASSERT(pArchetype == 0);

		pArchetype = _pArchetype;

		return (HANDLE) data;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ShipSilButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT((U32)hArchetype != 0 && pArchetype != 0);

	pArchetype = 0;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ShipSilButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT(pArchetype != 0);
	ShipSilButton * result = new DAComponent<ShipSilButton>;

	result->init(pArchetype,(GT_SHIPSILBUTTON *)hArchetype);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _shipSilbuttonfactory : GlobalComponent
{
	ShipSilButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ShipSilButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _shipSilbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End ShipSilButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
