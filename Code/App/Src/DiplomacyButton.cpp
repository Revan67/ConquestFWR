//--------------------------------------------------------------------------//
//                                                                          //
//                               DiplomacyButton.cpp                        //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/DiplomacyButton.cpp 2     9/08/00 7:14p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IDiplomacyButton.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include "DrawAgent.h"

#include <DDiplomacyButton.h>

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
struct DIPBUTTONTYPE
{
	PGENTYPE pArchetype;
	COMPTR<IDrawAgent> shapes[GT_DIPL_MAX_SHAPES];
	U16 width, height;
	S16 leftMargin, topMargin;		// skip pixels on the edge (for dropdown arrows)

	DIPBUTTONTYPE (void)
	{
	}

	~DIPBUTTONTYPE (void)
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
struct DACOM_NO_VTABLE DiplomacyButton : BaseHotRect, IDiplomacyButton, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(DiplomacyButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDiplomacyButton)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	DIPBUTTONTYPE * pDipButtonType;

	U32 controlID;					// set by owner
	S16 xText, yText;			// offset from origin of button for text

	bool bKeyboardFocus:1;
	bool bMousePressed:1;			// mouse press occurred inside rect
	bool bKeyboardPressed:1;		// keyboard used to press button
	bool bDisabled:1;
	bool bPostMessageBlocked:1;
	bool bFirstState:1;
	bool bSecondState:1;

	//
	// class methods
	//

	DiplomacyButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~DiplomacyButton (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDiplomacyButton methods  */

	virtual void InitDiplomacyButton (const DIPLOMACYBUTTON_DATA & data, BaseHotRect * parent); 

	virtual void EnableDiplomacyButton (bool bEnable);

	virtual const bool GetEnableState (void) const;

	virtual void SetVisible (bool bVisible);

	virtual const bool GetVisible (void) const;
	
	virtual void SetControlID (U32 id);

	virtual void SetFirstState (const bool bState);

	virtual void SetSecondState (const bool bState);

	virtual const bool GetFirstState (void) const;

	virtual const bool GetSecondState (void) const;


	/* IKeyboardFocus methods */

	virtual bool SetKeyboardFocus (bool bEnable);

	virtual U32 GetControlID (void);

	/* BaseHotRect methods */

	virtual void onRequestKeyboardFocus (int x, int y)
	{
		if (bDisabled==false)
			BaseHotRect::onRequestKeyboardFocus(x, y);
	}

	/* DiplomacyButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (DIPBUTTONTYPE * _pButtonType);

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
			if (bAlert && bMousePressed)
			{
				bMousePressed = bKeyboardPressed = false;
				doReleaseAction();
			}
			else
				bMousePressed = false;
		}
	}

	void doReleaseAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			parent->PostMessage(CQE_DIPLOMACYBUTTON, (void*)controlID);
			bPostMessageBlocked = true;
		}
	}
};
//--------------------------------------------------------------------------//
//
DiplomacyButton::~DiplomacyButton (void)
{
	GENDATA->Release(pDipButtonType->pArchetype);
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::InitDiplomacyButton (const DIPLOMACYBUTTON_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		parent->SetCallbackPriority(this, EVENT_PRIORITY_BUTTON);		
	}

	//
	// calculate screen rect. 
	//
	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.right = screenRect.left + pDipButtonType->width - 1;
	screenRect.bottom = screenRect.top + pDipButtonType->height - 1;
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::EnableDiplomacyButton (bool bEnable)
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
const bool DiplomacyButton::GetEnableState (void) const
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
}
//--------------------------------------------------------------------------//
//
const bool DiplomacyButton::GetVisible (void) const
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::SetFirstState (const bool bState)
{
	bFirstState = bState;
}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::SetSecondState (const bool bState)
{
	bSecondState = bState;
}
//--------------------------------------------------------------------------//
//
const bool DiplomacyButton::GetFirstState (void) const
{
	return bFirstState;
}
//--------------------------------------------------------------------------//
//
const bool DiplomacyButton::GetSecondState (void) const
{
	return bSecondState;
}
//--------------------------------------------------------------------------//
//
U32 DiplomacyButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
bool DiplomacyButton::SetKeyboardFocus (bool bEnable)
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
GENRESULT DiplomacyButton::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

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
				doReleaseAction();
				bKeyboardPressed = true;
				bMousePressed = false;
				BaseHotRect::Notify(message, param);
				return GR_GENERIC;
			}
			break;
		} // end switch (wParam)
		break; // end switch WM_KEYDOWN

	case CQE_UPDATE:
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
void DiplomacyButton::draw (void)
{
	// draw the proper state
	U32 state;

	if (bDisabled)
	{
		state = GT_DIPL_DISABLED;
	}
	else
	{
		state = GT_DIPL_NORMAL;
		if ((bAlert && bMousePressed) || bKeyboardPressed)
		{
			state = GT_DIPL_DEPRESSED;
		}
		else if (bAlert)
		{
			state = GT_DIPL_MOUSE_FOCUS;
		}
	}

	pDipButtonType->shapes[state]->Draw(0, screenRect.left, screenRect.top);

	if (bKeyboardFocus)
	{
		pDipButtonType->shapes[GT_DIPL_KEYB_FOCUS]->Draw(0, screenRect.left, screenRect.top);
	}
	
	// draw the proper handshake
	int diplomacy;

	if (bFirstState && bSecondState)
	{
		diplomacy = 3;
	}
	else if (bFirstState && bSecondState == false)
	{
		diplomacy = 1;
	}
	else if (bFirstState == false && bSecondState == false)
	{
		diplomacy = 0;
	}
	else
	{
		diplomacy = 2;
	}

	diplomacy += GT_DIPL_STATE0;
	pDipButtonType->shapes[diplomacy]->Draw(0, screenRect.left + pDipButtonType->leftMargin,
										   screenRect.top + pDipButtonType->topMargin);

}
//--------------------------------------------------------------------------//
//
void DiplomacyButton::init (DIPBUTTONTYPE * _pButtonType)
{
	COMPTR<IDAComponent> pBase;
	pDipButtonType = _pButtonType;
}
//--------------------------------------------------------------------------//
//----------------------Diplomacy Button Factory class----------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE DiplomacyButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(DiplomacyButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	DiplomacyButtonFactory (void) { }

	~DiplomacyButtonFactory (void);

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
DiplomacyButtonFactory::~DiplomacyButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void DiplomacyButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE DiplomacyButtonFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_DIPLOMACYBUTTON)
	{
		GT_DIPLOMACYBUTTON * data = (GT_DIPLOMACYBUTTON *) _data;
		DIPBUTTONTYPE * result = new DIPBUTTONTYPE;

		result->pArchetype = pArchetype;

		//
		// now load all of the shapes from the data file
		//
		if (data->shapeFile[0])
		{
			BEGIN_MAPPING(INTERFACEDIR, data->shapeFile);
				int i;
				for (i = 0; i < GT_DIPL_MAX_SHAPES; i++)
					CreateDrawAgent((VFX_SHAPETABLE *) pImage, i, result->shapes[i]);
			END_MAPPING(INTERFACEDIR);

			result->shapes[0]->GetDimensions(result->width, result->height);
		}

		result->leftMargin = IDEAL2REALX(data->leftMargin);
		result->topMargin = IDEAL2REALX(data->topMargin);

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 DiplomacyButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	DIPBUTTONTYPE * type = (DIPBUTTONTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT DiplomacyButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	DIPBUTTONTYPE * type = (DIPBUTTONTYPE *) hArchetype;
	DiplomacyButton * result = new DAComponent<DiplomacyButton>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _dipbuttonfactory : GlobalComponent
{
	DiplomacyButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<DiplomacyButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _dipbuttonfactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End DiplomacyButton.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//
