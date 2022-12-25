//--------------------------------------------------------------------------//
//                                                                          //
//                               HotButton.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/HotButton.cpp 43    9/25/00 9:55p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IHotButton.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DHotButton.h>
#include "DrawAgent.h"
#include "IShapeLoader.h"
#include "Sfx.h"
#include "IInterfaceManager.h"
#include <DSounds.h>

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HotButton : BaseHotRect, IHotButton 
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(HotButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IHotButton)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PGENTYPE pArchetype;
	HSOUND hSound;
	COMPTR<IDrawAgent> shapes[GTHBSHP_MAX_SHAPES];

	U32 controlID;					// set by owner
	HBTNTXT::BUTTON_TEXT buttonText;	
	wchar_t *szButtonText;
	U32 hotkeyID;
	S32 numericValue;

	bool bDisabled:1;
	bool bPushState:1;			// used for selection state
	bool bPostMessageBlocked:1;
	bool bContextBehavior:1;
	bool bHighlight:1;

	//
	// class methods
	//

	HotButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~HotButton (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IHotButton methods  */

	virtual void InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void EnableButton (bool bEnable);

	virtual bool GetEnableState (void);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);
	
	virtual void SetTextID (U32 textID);	// set resource ID for text

	virtual void SetTextString (const wchar_t * szString);

	virtual void SetPushState (bool bPressed);

	virtual bool GetPushState (void);

	virtual void SetControlID (U32 id);

	virtual U32 GetControlID (void);

	virtual void SetNumericValue (S32 value);

	virtual S32 GetNumericValue (void);

	virtual void EnableContextMenuBehavior (void)
	{
		bContextBehavior = true;
	}

	virtual void SetHighlightState (bool _bHighlight)
	{
		bHighlight = _bHighlight;
	}

	/* BaseHotRect methods */
	virtual void setStatus (void);


	/* HotButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE _pArchetype, HSOUND _hSound);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDown (void)
	{
		if (bDisabled == false)
		{
			if (bAlert)
				doLPushAction();
		}
	}

	void onRightButtonDown (void)
	{
		if (bDisabled == false)
		{
			if (bAlert)
				doRPushAction();
		}
	}

	void doLPushAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			if (hotkeyID != 0)
				EVENTSYS->Post(CQE_HOTKEY, (void *)hotkeyID);
			parent->PostMessage(CQE_LHOTBUTTON, (void*)controlID);
			SFXMANAGER->Play(hSound);
			bPostMessageBlocked = true;
		}
	}

	void doRPushAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			parent->PostMessage(CQE_RHOTBUTTON, (void*)controlID);
			bPostMessageBlocked = true;
		}
	}

	void doDblPushAction (void)
	{
		parent->PostMessage(CQE_LDBLHOTBUTTON, (void*)controlID);
	}
};
//--------------------------------------------------------------------------//
//
HotButton::~HotButton (void)
{
	GENDATA->Release(pArchetype);
	delete szButtonText;
}
//--------------------------------------------------------------------------//
//
void HotButton::InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	if ((buttonText = data.buttonText) != 0)
		szButtonText = _wcsdup(_localLoadStringW(buttonText));
	//	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));

	hotkeyID = data.hotkey;
	statusTextID = data.buttonInfo;
	hintboxID = data.buttonHint;

	EnableButton(data.bDisabled==false);

	//
	// load the shapes
	//
	int base;
	if ((base = data.baseImage) == 0)
		base++;
	
	int i;
	for (i = 0; i < GTHBSHP_MAX_SHAPES; i++)
		loader->CreateDrawAgent(i+base, shapes[i]);

	U16 width, height;
	if (shapes[0])
		shapes[0]->GetDimensions(width, height);
	else
	{
		width = height = 50;
	}

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;
	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;

	//
	// calculate text offset
	//

	if (controlID == 0)
		controlID = buttonText;
}
//--------------------------------------------------------------------------//
//
void HotButton::InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void HotButton::InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void HotButton::InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void HotButton::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bPushState = false;
	}
}
//--------------------------------------------------------------------------//
//
bool HotButton::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void HotButton::SetVisible (bool bVisible)
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
bool HotButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void HotButton::SetTextID (U32 textID)
{
	buttonText = static_cast<HBTNTXT::BUTTON_TEXT>(textID);
//	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));
	if (szButtonText)
	{
		delete szButtonText;
	}
	szButtonText = _wcsdup(_localLoadStringW(buttonText));
}
//--------------------------------------------------------------------------//
//
void HotButton::SetTextString (const wchar_t * szString)
{
	buttonText = HBTNTXT::NOTEXT;
//	wcsncpy(szButtonText, szString, sizeof(szButtonText)/sizeof(wchar_t));
	if (szButtonText)
	{
		delete szButtonText;
		szButtonText = 0;
	}
	if (szString && szString[0])
		szButtonText = _wcsdup(szString);
}
//--------------------------------------------------------------------------//
//
void HotButton::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool HotButton::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void HotButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 HotButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void HotButton::SetNumericValue (S32 value)
{
	numericValue = value;	
}
//--------------------------------------------------------------------------//
//
S32 HotButton::GetNumericValue (void)
{
	return numericValue;
}
//--------------------------------------------------------------------------//
//
GENRESULT HotButton::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		draw();
		break;

	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			desiredOwnedFlags = (RF_CURSOR|RF_STATUS|RF_HINTBOX);
			grabAllResources();
		}
		else
		if (actualOwnedFlags)
		{
			desiredOwnedFlags = 0;
			releaseResources();
		}
		break;

	case WM_LBUTTONDBLCLK:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			doDblPushAction();
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		onLeftButtonDown();
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;

	case WM_RBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		onRightButtonDown();
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;

	case CQE_UPDATE:
		bPostMessageBlocked = false;
		break;

//	case WM_KEYDOWN:
//	case CQE_SLIDER:
//	case CQE_LIST_CARET_MOVED:
//	case CQE_LIST_SELECTION:
//	case CQE_BUTTON:
	case CQE_LHOTBUTTON:
	case CQE_RHOTBUTTON:
		parent->Notify(message, param);		// forward the message upward
		break;

/*	case CQE_SET_FOCUS:
		if (isInvisibleMenu())
		{
			onSetFocus(true);
			return GR_OK;
		}
		break;

	case CQE_KILL_FOCUS:
		if (isInvisibleMenu())
		{
			onSetFocus(false);
			return GR_OK;
		}
		break;
*/
	}

//	if (isInvisibleMenu() && message == CQE_ENDFRAME)
//		return GR_OK;

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void HotButton::draw (void)
{
	BATCH->set_state(RPR_BATCH,FALSE);
	U32 state;

	bool bFlash = INTERMAN->RenderHotkeyFlashing(hotkeyID);
	
	if(bFlash)
	{
		state = GTHBSHP_MOUSE_FOCUS;
	}
	else
	if (bDisabled)
	{
		state = GTHBSHP_DISABLED;
	}
	else
	{
		state = GTHBSHP_NORMAL;
		if (bAlert)
			state = GTHBSHP_MOUSE_FOCUS;
	}

	if (shapes[state])
		shapes[state]->Draw(0, screenRect.left, screenRect.top);
	if ((bPushState || bHighlight) && shapes[GTHBSHP_SELECTED]!=0)
		shapes[GTHBSHP_SELECTED]->Draw(0, screenRect.left, screenRect.top);
}
//--------------------------------------------------------------------------//
//
void HotButton::setStatus (void)
{
	wchar_t buffer[256];
	wchar_t ext[128];
	char keytext[128];
	wchar_t * ptr;
	int len;

	if (szButtonText)
	{
		ptr = szButtonText;
		len = wcslen(ptr);
		CQASSERT1(len < 128, "String too long: \"%S\"", ptr);
		memcpy(buffer, ptr, (len+1)*sizeof(*ptr));
		
		if (hotkeyID != 0 && (ptr = wcschr(buffer, '!')) != 0)
		{
			int i;

			wcscpy(ext, ptr+1);		// copy everything after the mark
			len = HOTKEY->GetHotkeyText(hotkeyID, keytext, sizeof(keytext));
			i = MultiByteToWideChar(CP_ACP, 0, keytext, len, ptr, sizeof(buffer)/sizeof(buffer[0]));
			ptr += i;
			wcscpy(ptr, ext);
			CQASSERT(wcslen(buffer) < sizeof(buffer)/sizeof(buffer[0]));
		}

		if ((ptr = wcschr(buffer, '#')) != 0)
		{
//			int i=0;

			wcscpy(ext, ptr+1);		// copy everything after the mark
			_ltow(numericValue, ptr, 10);
			wcscat(ptr, ext);
		}

		STATUS->SetTextString(buffer, STM_TOOLTIP);
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
void HotButton::init (PGENTYPE _pArchetype, HSOUND _hSound)
{
	pArchetype = _pArchetype;
	hSound = _hSound;
	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//-----------------------HotButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE HotButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	HSOUND hSound;		// sound played when hotbutton is pressed

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(HotButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	HotButtonFactory (void) { }

	~HotButtonFactory (void);

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
HotButtonFactory::~HotButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void HotButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE HotButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTBUTTON)
	{
		GT_HOTBUTTON * data = (GT_HOTBUTTON *) _data;
	
		if (data->buttonType == HOTBUTTONTYPE::HOTBUTTON)
		{
			CQASSERT(pArchetype == 0);

			pArchetype = _pArchetype;
			hSound = SFXMANAGER->Open(SFXMANAGER->GetGlobalSounds().defaultButton);

			return (HANDLE) 1;
		}
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 HotButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);

	pArchetype = 0;
	SFXMANAGER->CloseHandle(hSound);
	hSound = 0;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT HotButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);
	HotButton * result = new DAComponent<HotButton>;

	result->init(pArchetype, hSound);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _hotbuttonfactory : GlobalComponent
{
	HotButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<HotButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _hotbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End HotButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
