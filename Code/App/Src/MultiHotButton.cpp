//--------------------------------------------------------------------------//
//                                                                          //
//                               MultiHotButton.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MultiHotButton.cpp 11    9/13/00 11:39a Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IHotButton.h"
#include "IMultiHotButton.h"
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
struct DACOM_NO_VTABLE MultiHotButton : BaseHotRect, IHotButton, IMultiHotButton
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(MultiHotButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IHotButton)
	DACOM_INTERFACE_ENTRY(IMultiHotButton)
	DACOM_INTERFACE_ENTRY(IEventCallback)
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
	wchar_t szButtonText[256];
	U32 hotkeyID;
	S32 numericValue;
	COMPTR<IShapeLoader> myLoader;
	ResourceCost buildCost;

	bool bDisabled:1;
	bool bPushState:1;			// used for selection state
	bool bPostMessageBlocked:1;
	bool bContextBehavior:1;
	bool bWantInvisible:1;
	bool bHaveImageState:1;
	bool bHighlight:1;
	bool bSingleShape:1;
	bool bHasBuildCost:1;


	//
	// class methods
	//

	MultiHotButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
		bHasBuildCost = false;
	}

	virtual ~MultiHotButton (void);

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

	virtual void SetTabBehavior (void) {}

	virtual void SetTabSelected (bool bSelected) {}


	/* IMultiHotButton */

	virtual void SetState(U32 _baseImage, HBTNTXT::BUTTON_TEXT _buttonText, HBTNTXT::HOTBUTTONINFO _statusTextID,  HBTNTXT::MULTIBUTTONINFO _buttonInfo);

	virtual void NullState();

	virtual void GetShape(IDrawAgent ** shape);

	virtual void SetBuildCost(const ResourceCost & cost);

	/* BaseHotRect methods */
	virtual void setStatus (void);
	
	virtual void setHintbox (void);
	
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
			if (hotkeyID != 0xFFFFFFFF)
				EVENTSYS->Post(CQE_HOTKEY, (void *)hotkeyID);
			parent->PostMessage(CQE_LHOTBUTTON, (void*)buttonText);
			SFXMANAGER->Play(hSound);
			bPostMessageBlocked = true;
		}
	}

	void doRPushAction (void)
	{
		if (bPostMessageBlocked==false)
		{
			parent->PostMessage(CQE_RHOTBUTTON, (void*)buttonText);
			bPostMessageBlocked = true;
		}
	}

	void doDblPushAction (void)
	{
		parent->PostMessage(CQE_LDBLHOTBUTTON, (void*)buttonText);
	}
};
//--------------------------------------------------------------------------//
//
MultiHotButton::~MultiHotButton (void)
{
	GENDATA->Release(pArchetype);
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	myLoader = loader;
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	hintboxID = 0;
	hotkeyID = data.hotkey;
	statusTextID = 0;
	bSingleShape = data.bSingleShape;

	EnableButton(data.bDisabled==false);

	screenRect.left = IDEAL2REALX(data.xOrigin) + parent->screenRect.left;
	screenRect.top = IDEAL2REALY(data.yOrigin) + parent->screenRect.top;

}
//--------------------------------------------------------------------------//
//
void MultiHotButton::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bPushState = false;
	}
}
//--------------------------------------------------------------------------//
//
bool MultiHotButton::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetVisible (bool bVisible)
{
	bWantInvisible = !bVisible;
	bInvisible =  bWantInvisible || (!bHaveImageState);
	if (bInvisible)
	{
		desiredOwnedFlags = 0;
		releaseResources();
	}
}
//--------------------------------------------------------------------------//
//
bool MultiHotButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetTextID (U32 textID)
{
	buttonText = static_cast<HBTNTXT::BUTTON_TEXT>(textID);
	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetTextString (const wchar_t * szString)
{
	buttonText = HBTNTXT::NOTEXT;
	wcsncpy(szButtonText, szString, sizeof(szButtonText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool MultiHotButton::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 MultiHotButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetNumericValue (S32 value)
{
	numericValue = value;	
}
//--------------------------------------------------------------------------//
//
S32 MultiHotButton::GetNumericValue (void)
{
	return numericValue;
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetState(U32 _baseImage, HBTNTXT::BUTTON_TEXT _buttonText, HBTNTXT::HOTBUTTONINFO _statusTextID, HBTNTXT::MULTIBUTTONINFO _buttonInfo)
{
	hintboxID = _buttonInfo;
	statusTextID = _statusTextID;

	if ((buttonText = _buttonText) != 0)
		wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));	

	int base;
	if ((base = _baseImage) == 0)
		base++;
	
	int i;
	if(bSingleShape)
	{
		myLoader->CreateDrawAgent(base, shapes[0]);
	}
	else
	{
		for (i = 0; i < GTHBSHP_MAX_SHAPES; i++)
			myLoader->CreateDrawAgent(i+base, shapes[i]);
	}

	U16 width, height;
	if (shapes[0])
		shapes[0]->GetDimensions(width, height);
	else
	{
		width = height = 50;
	}

	screenRect.bottom = screenRect.top + height - 1;
	screenRect.right = screenRect.left + width - 1;

	bHaveImageState = true;

	bInvisible =  bWantInvisible || (!bHaveImageState);
	if (bInvisible)
	{
		desiredOwnedFlags = 0;
		releaseResources();
	}
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::NullState()
{
	bHaveImageState = false;
	bInvisible =  bWantInvisible || (!bHaveImageState);
	if (bInvisible)
	{
		desiredOwnedFlags = 0;
		releaseResources();
	}
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::GetShape(IDrawAgent ** shape)
{
	shape[GTHBSHP_NORMAL] = shapes[GTHBSHP_NORMAL];
	shape[GTHBSHP_MOUSE_FOCUS] = shapes[GTHBSHP_MOUSE_FOCUS];
	shape[GTHBSHP_DISABLED] = shapes[GTHBSHP_DISABLED];
	shape[GTHBSHP_SELECTED] = shapes[GTHBSHP_SELECTED];
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::SetBuildCost(const ResourceCost & cost)
{
	bHasBuildCost = true;
	buildCost = cost;
}
//--------------------------------------------------------------------------//
//
GENRESULT MultiHotButton::Notify (U32 message, void *param)
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
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::draw (void)
{
	if(bHaveImageState)
	{
		if(bSingleShape)
		{
			if (shapes[0])
				shapes[0]->Draw(0, screenRect.left, screenRect.top);
		}
		else
		{
			U32 state;
			
			bool bFlash = INTERMAN->RenderHotkeyFlashing(hotkeyID);
			
			if (bHighlight)
			{
				state = GTHBSHP_MOUSE_FOCUS;
			}
			else if (bDisabled)
			{
				state = GTHBSHP_DISABLED;
			}
			else
			{
				state = GTHBSHP_NORMAL;
				if (bAlert)
					state = GTHBSHP_MOUSE_FOCUS;
			}

			if(bFlash)
			{
				if(state == GTHBSHP_MOUSE_FOCUS)
					state = GTHBSHP_NORMAL;
				else 
					state = GTHBSHP_MOUSE_FOCUS;
			}

			if (shapes[state])
				shapes[state]->Draw(0, screenRect.left, screenRect.top);
			if ((bPushState || bHighlight) && shapes[GTHBSHP_SELECTED]!=0)
				shapes[GTHBSHP_SELECTED]->Draw(0, screenRect.left, screenRect.top);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::setStatus (void)
{
	wchar_t buffer[256];
	wchar_t ext[256];
	char keytext[64];
	wchar_t * ptr;
	int len;

	if (szButtonText[0])
	{
		ptr = szButtonText;
		len = wcslen(ptr);
		CQASSERT(len < 64);
		memcpy(buffer, ptr, (len+1)*sizeof(*ptr));
		
	/*	if (hotkeyID != 0xFFFFFFFF && (ptr = wcschr(buffer, '!')) != 0)
		{
			int i;

			wcscpy(ext, ptr+1);		// copy everything after the mark
			len = HOTKEY->GetHotkeyText(hotkeyID, keytext, sizeof(keytext));
			i = MultiByteToWideChar(CP_ACP, 0, keytext, len, ptr, sizeof(buffer)/sizeof(buffer[0]));
			ptr += i;
			wcscpy(ptr, ext);
			CQASSERT(wcslen(buffer) < sizeof(buffer)/sizeof(buffer[0]));
		}*/

		if(bHasBuildCost)
		{
			wchar_t dollar[256];

			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				if(buildCost.metal)
				{
					wcscpy(dollar, _localLoadStringW(IDS_METAL_SIGN));
					len = wcslen(dollar);
					wcscpy(ext, ptr+1);		// copy everything after the mark
					memcpy(ptr, dollar, len * sizeof(wchar_t));
					ptr+=len;
					_ltow(buildCost.metal*METAL_MULTIPLIER, ptr, 10);
					wcscat(ptr, ext);
				}
				else
				{
					memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
				}
			}
			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				if(buildCost.gas)
				{
					wcscpy(dollar, _localLoadStringW(IDS_GAS_SIGN));
					len = wcslen(dollar);
					wcscpy(ext, ptr+1);		// copy everything after the mark
					memcpy(ptr, dollar, len * sizeof(wchar_t));
					ptr+=len;
					_ltow(buildCost.gas*GAS_MULTIPLIER, ptr, 10);
					wcscat(ptr, ext);
				}
				else
				{
					memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
				}
			}
			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				if(buildCost.crew)
				{
					wcscpy(dollar, _localLoadStringW(IDS_CREW_SIGN));
					len = wcslen(dollar);
					wcscpy(ext, ptr+1);		// copy everything after the mark
					memcpy(ptr, dollar, len * sizeof(wchar_t));
					ptr+=len;
					_ltow(buildCost.crew*CREW_MULTIPLIER, ptr, 10);
					wcscat(ptr, ext);
				}
				else
				{
					memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
				}
			}
			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				if(buildCost.commandPt)
				{
					wcscpy(dollar, _localLoadStringW(IDS_COMMAND_SIGN));
					len = wcslen(dollar);
					wcscpy(ext, ptr+1);		// copy everything after the mark
					memcpy(ptr, dollar, len * sizeof(wchar_t));
					ptr+=len;
					_ltow(buildCost.commandPt, ptr, 10);
					wcscat(ptr, ext);
				}
				else
				{
					memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
				}
			}
			
			STATUS->SetTextString(buffer, STM_BUILD);
		}
		else
		{
			if ((ptr = wcschr(buffer, '#')) != 0)
			{
				ptr[0] = 0;
			}
			STATUS->SetTextString(buffer, STM_TOOLTIP);
		}
	}
	else
	if (statusTextID)
	{
		if(bHasBuildCost)
		{
			wchar_t szToolText[256];
	
			wchar_t buffer[256];
			wchar_t ext[256];
			wchar_t dollar[256];
			char keytext[256];
			wchar_t * ptr;
			int len;

			wcsncpy(szToolText, _localLoadStringW(statusTextID), sizeof(szToolText)/sizeof(wchar_t));
			if (szToolText[0])
			{
				ptr = szToolText;
				len = wcslen(ptr);
				CQASSERT(len < 256);
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
					if(buildCost.metal)
					{
						wcscpy(dollar, _localLoadStringW(IDS_METAL_SIGN));
						len = wcslen(dollar);
						wcscpy(ext, ptr+1);		// copy everything after the mark
						memcpy(ptr, dollar, len * sizeof(wchar_t));
						ptr+=len;
						_ltow(buildCost.metal*METAL_MULTIPLIER, ptr, 10);
						wcscat(ptr, ext);
					}
					else
					{
						memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
					}
				}
				if ((ptr = wcschr(buffer, '#')) != 0)
				{
					if(buildCost.gas)
					{
						wcscpy(dollar, _localLoadStringW(IDS_GAS_SIGN));
						len = wcslen(dollar);
						wcscpy(ext, ptr+1);		// copy everything after the mark
						memcpy(ptr, dollar, len * sizeof(wchar_t));
						ptr+=len;
						_ltow(buildCost.gas*GAS_MULTIPLIER, ptr, 10);
						wcscat(ptr, ext);
					}
					else
					{
						memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
					}
				}
				if ((ptr = wcschr(buffer, '#')) != 0)
				{
					if(buildCost.crew)
					{
						wcscpy(dollar, _localLoadStringW(IDS_CREW_SIGN));
						len = wcslen(dollar);
						wcscpy(ext, ptr+1);		// copy everything after the mark
						memcpy(ptr, dollar, len * sizeof(wchar_t));
						ptr+=len;
						_ltow(buildCost.crew*CREW_MULTIPLIER, ptr, 10);
						wcscat(ptr, ext);
					}
					else
					{
						memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
					}
				}
				if ((ptr = wcschr(buffer, '#')) != 0)
				{
					if(buildCost.commandPt)
					{
						wcscpy(dollar, _localLoadStringW(IDS_COMMAND_SIGN));
						len = wcslen(dollar);
						wcscpy(ext, ptr+1);		// copy everything after the mark
						memcpy(ptr, dollar, len * sizeof(wchar_t));
						ptr+=len;
						_ltow(buildCost.commandPt, ptr, 10);
						wcscat(ptr, ext);
					}
					else
					{
						memmove(ptr,ptr+1,wcslen(ptr)*sizeof(wchar_t));
					}
				}
				
				STATUS->SetTextString(buffer, STM_BUILD);
			}
			else
				STATUS->SetDefaultState();
		}
		else
			STATUS->SetText(statusTextID, STM_DEFAULT);
	}
	else
		STATUS->SetDefaultState();

	STATUS->SetRect(screenRect);
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::setHintbox (void)
{
	if (hintboxID)
		HINTBOX->SetText(hintboxID);
}
//--------------------------------------------------------------------------//
//
void MultiHotButton::init (PGENTYPE _pArchetype, HSOUND _hSound)
{
	pArchetype = _pArchetype;
	hSound = _hSound;
	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//-----------------------MultiHotButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE MultiHotButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	HSOUND hSound;		// sound played when hotbutton is pressed

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(MultiHotButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	MultiHotButtonFactory (void) { }

	~MultiHotButtonFactory (void);

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
MultiHotButtonFactory::~MultiHotButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void MultiHotButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE MultiHotButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTBUTTON)
	{
		GT_HOTBUTTON * data = (GT_HOTBUTTON *) _data;
	
		if (data->buttonType == HOTBUTTONTYPE::MULTI)
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
BOOL32 MultiHotButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);

	pArchetype = 0;
	SFXMANAGER->CloseHandle(hSound);
	hSound = 0;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT MultiHotButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);
	MultiHotButton * result = new DAComponent<MultiHotButton>;

	result->init(pArchetype, hSound);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _multihotbuttonfactory : GlobalComponent
{
	MultiHotButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<MultiHotButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _multihotbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End MultiHotButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
