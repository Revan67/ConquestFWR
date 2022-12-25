//--------------------------------------------------------------------------//
//                                                                          //
//                               TabButton.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TabButton.cpp 9     8/23/00 9:51p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IHotButton.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "Frame.h"
#include "GenData.h"
#include "ITabControl.h"

#include <DHotButton.h>
#include "DrawAgent.h"
#include "IShapeLoader.h"
#include "Sfx.h"
#include <DSounds.h>

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>


struct TABBUTTONTYPE
{
	PGENTYPE pArchetype;
	PGENTYPE pFontType;

	TABBUTTONTYPE (void)
	{
	}

	~TABBUTTONTYPE (void)
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
struct DACOM_NO_VTABLE TabButton :  BaseHotRect, ITabButton, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(TabButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(ITabButton)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(BaseHotRect)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	TABBUTTONTYPE * pButtonType;
	COMPTR<IFontDrawAgent> pFont;

	HSOUND hSound;
	COMPTR<IDrawAgent> shapes[GTHBSHP_MAX_SHAPES];

	//
	// data for keyboard focus management
	// 
	COMPTR<IKeyboardFocus> focusControl;
	COMPTR<IKeyboardFocus> defaultFocusControl;

	U32 controlID;					// set by owner
	wchar_t szButtonText[256];
	U32 hotkeyID;
	S32 numericValue;

	bool bDisabled:1;
	bool bPushState:1;			// used for selection state
	bool bPostMessageBlocked:1;
	bool bContextBehavior:1;
	bool bHighlight:1;
	bool bTabSelected:1;
	bool bKeyboardFocusEnabled:1;

	int colorIndex;
	COLORREF colorTable[3];
	S16 xText, yText;

	//
	// class methods
	//

	TabButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~TabButton (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ITabButton methods  */

	virtual void InitTabButton (const HOTBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * loader); 

	virtual void EnableButton (bool bEnable);

	virtual bool GetEnableState (void);

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void);
	
	virtual void SetTextID (U32 textID);	// set resource ID for text

	virtual void SetTextString (const wchar_t * szString);

	virtual void SetTextColorNormal (COLORREF color);

	virtual void SetTextColorHilite (COLORREF color);

	virtual void SetTextColorSelected (COLORREF color);

	virtual void SetPushState (bool bPressed);

	virtual bool GetPushState (void);

	virtual void SetControlID (U32 id);

	virtual U32 GetControlID (void);

	virtual void EnableContextMenuBehavior (void)
	{
		bContextBehavior = true;
	}

	virtual void SetHighlightState (bool _bHighlight)
	{
		bHighlight = _bHighlight;
	}

	virtual void SetTabSelected (bool bSelected);

	virtual void SetDefaultFocusControl (struct IDAComponent * component);

	virtual void EnableKeyboardFocusing (void)
	{
		bKeyboardFocusEnabled = true;
	}

	/* BaseHotRect methods */

	virtual bool onTabPressed (void);

	virtual void onLeftPressed (void);

	virtual void onRightPressed (void);

	/* IKeyboardFocus methods */
	virtual bool SetKeyboardFocus (bool bEnable) 
	{  
		return false;
	}
	
	/* HotButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (TABBUTTONTYPE * _pButtonType);

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

	// are the contents (ie. your children) supposed to be invisible?
	bool isInvisibleMenu (void) const
	{
		return !bTabSelected;
	}

	void setFocus (struct IDAComponent * component);

	void nextFocus (void);

	void prevFocus (void);

	CONNECTION_NODE<IKeyboardFocus> * findPrevFocus (CONNECTION_NODE<IKeyboardFocus> * pFirst, IKeyboardFocus * pCurr);
};
//--------------------------------------------------------------------------//
//
TabButton::~TabButton (void)
{
 	GENDATA->Release(pButtonType->pArchetype);

	if (focusControl)
	{
		focusControl.free();
	}
	
	if (defaultFocusControl)
	{
		defaultFocusControl.free();
	}
}
//--------------------------------------------------------------------------//
//
void TabButton::InitTabButton (const HOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

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
}
//--------------------------------------------------------------------------//
//
void TabButton::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bPushState = false;
	}
}
//--------------------------------------------------------------------------//
//
bool TabButton::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void TabButton::SetVisible (bool bVisible)
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
bool TabButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void TabButton::SetTextID (U32 textID)
{
	if (textID)
	{
		wcsncpy(szButtonText, _localLoadStringW(textID), sizeof(szButtonText)/sizeof(wchar_t));

		if (pFont)
		{
			// set the x and y coordinates of the string
			int width  = screenRect.right - screenRect.left;
			int height = screenRect.bottom - screenRect.top;

			yText = S32(height - pFont->GetFontHeight()) / 2;
			xText = S32(width - pFont->GetStringWidth(szButtonText)) / 2;
		}
	}
}
//--------------------------------------------------------------------------//
//
void TabButton::SetTextString (const wchar_t * szString)
{
	wcsncpy(szButtonText, szString, sizeof(szButtonText)/sizeof(wchar_t));

	if (pFont)
	{
		// set the x and y coordinates of the string
		int width  = screenRect.right - screenRect.left;
		int height = screenRect.bottom - screenRect.top;

		yText = S32(height - pFont->GetFontHeight()) / 2;
		xText = S32(width - pFont->GetStringWidth(szButtonText)) / 2;
	}
}
//--------------------------------------------------------------------------//
//
void TabButton::SetTextColorNormal (COLORREF color)
{
	colorTable[0] = color;

	if (pFont)
	{
		pFont->SetFontColor(color | 0xFF000000, 0);
	}
}
//--------------------------------------------------------------------------//
//
void TabButton::SetTextColorHilite (COLORREF color)
{
	colorTable[1] = color;
}
//--------------------------------------------------------------------------//
//
void TabButton::SetTextColorSelected (COLORREF color)
{
	colorTable[2] = color;
}
//--------------------------------------------------------------------------//
//
void TabButton::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool TabButton::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void TabButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 TabButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT TabButton::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

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
		break;

	case CQE_KEYBOARD_FOCUS:
		{
			IDAComponent * focus = (IDAComponent*)param;
			if (focus)
			{
				setFocus(focus);
			}
		}
		break;

/*	case WM_LBUTTONDBLCLK:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			doDblPushAction();
			return GR_GENERIC;
		}
*/		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		onLeftButtonDown();
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;

/*	case WM_RBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (bContextBehavior && (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode))	// do not allow buttons to work in this mode
			bAlert = 0;
		onRightButtonDown();
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;
*/
	case CQE_UPDATE:
		if (isInvisibleMenu())
		{
			// do not pass on update's to tab children
			return GR_OK;
		}
		bPostMessageBlocked = false;
		break;

//	case WM_KEYDOWN:
	case CQE_SLIDER:
	case CQE_LIST_CARET_MOVED:
	case CQE_LIST_SELECTION:
	case CQE_BUTTON:
	case CQE_LHOTBUTTON:
	case CQE_RHOTBUTTON:
		parent->Notify(message, param);		// forward the message upward
		break;

	case CQE_SET_FOCUS:
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
	
	case WM_CHAR:
		if (isInvisibleMenu() == false)
		{
			switch (TCHAR(msg->wParam))
			{
			case '\t':
				if (onTabPressed())
					return GR_GENERIC;		// don't propagate this message
				break;

			default:
				break;
			} 
			return GR_OK;
		}
		break; // end switch WM_CHAR

	case WM_KEYDOWN:
		if (isInvisibleMenu() == false)
		{
			if (BaseHotRect::Notify(message, param) != GR_OK)		// give children a chance first
				return GR_GENERIC;
			else
			switch (msg->wParam)
			{
			case VK_UP:
			case VK_LEFT:
				onLeftPressed();
				break;

			case VK_DOWN:
			case VK_RIGHT:
				onRightPressed();
				break;
			}
			return GR_OK;
		}
		break;
	}

	if (isInvisibleMenu() && message == CQE_ENDFRAME)
		return GR_OK;

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void TabButton::draw (void)
{
	BATCH->set_state(RPR_BATCH,FALSE);
	U32 state;
	
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

	if ((pFont != NULL) && szButtonText[0])
	{
		int index = 0;
		
		if (bPushState)
		{
			index = 2;
		}
		else if (bAlert)
		{
			index = 1;
		}

		if (index != colorIndex)
		{
			colorIndex = index;
			pFont->SetFontColor(colorTable[colorIndex] | 0xFF000000, 0);
		}

		pFont->StringDraw(NULL, screenRect.left + xText, screenRect.top + yText, szButtonText);
	}
}
//----------------------------------------------------------------------------------//
//
bool TabButton::onTabPressed (void)
{
	if (bKeyboardFocusEnabled)
	{
		SHORT shiftDown = GetAsyncKeyState(VK_SHIFT) >> 1;

		// if the shift button is down, then we want to go back a tab
		if (shiftDown == 0)
		{
			COMPTR<ITabControl> tabControl;
			parent->QueryInterface("ITabControl", tabControl);
		
			int nextTab = (tabControl->GetCurrentTab() + 1)%tabControl->GetTabCount();
			tabControl->SetCurrentTab(nextTab);
			return true;
		}
		else 
		{
			COMPTR<ITabControl> tabControl;
			parent->QueryInterface("ITabControl", tabControl);

			int prevTab = tabControl->GetCurrentTab() - 1;
			if (prevTab < 0)
			{
				prevTab = tabControl->GetTabCount() - 1;
			}
			tabControl->SetCurrentTab(prevTab);
			return true;
		}
	}
	return false;
}
//----------------------------------------------------------------------------------//
//
void TabButton::onLeftPressed (void)
{
	prevFocus();
}
//----------------------------------------------------------------------------------//
//
void TabButton::onRightPressed (void)
{
	nextFocus();
}
//----------------------------------------------------------------------------------//
//
void TabButton::SetTabSelected (bool bSelected)
{
	bTabSelected = bSelected;
	
	if (bSelected==false)
	{
		bool bOrigFocus = bHasFocus;
		BaseHotRect::Notify(CQE_KILL_FOCUS, 0);
		bHasFocus = bOrigFocus;
	}
	else
	{
		if (bHasFocus)
			BaseHotRect::Notify(CQE_SET_FOCUS, 0);

		// set our default focus control if there is no ordinary focus control
		if (bKeyboardFocusEnabled && (focusControl == NULL) && (defaultFocusControl != NULL))
		{
			setFocus(defaultFocusControl);
		}
	}

	// send a mouse update
	{
		MSG msg;
		S32 x, y;

		WM->GetCursorPos(x, y);

		msg.hwnd = hMainWindow;
		msg.message = WM_MOUSEMOVE;
		msg.wParam = 0;
		msg.lParam = MAKELPARAM(x,y);
		Notify(WM_MOUSEMOVE, &msg);
	}
}
//----------------------------------------------------------------------------------//
//
void TabButton::SetDefaultFocusControl (struct IDAComponent * component)
{
	if (bKeyboardFocusEnabled)
	{
		COMPTR<IKeyboardFocus> res;
		
		if (component)
		{
			component->QueryInterface("IKeyboardFocus", res);
			CQASSERT(res!=0);

			defaultFocusControl = res;
		}
		else 
		{
			defaultFocusControl.free();
			defaultFocusControl = NULL;
		}
	}
}
//----------------------------------------------------------------------------------//
//
void TabButton::setFocus (struct IDAComponent * component)
{
	if (bKeyboardFocusEnabled == false)
		return;

	COMPTR<IKeyboardFocus> res;
	if (component)
	{
		component->QueryInterface("IKeyboardFocus", res);
		CQASSERT(res!=0);
	}

	if (focusControl != res)
	{
		if (focusControl!=0)
		{
			focusControl->SetKeyboardFocus(false);
			focusControl.free();
		}

		if (res)
		{
			if (res->SetKeyboardFocus(true))
			{
				focusControl = res;
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
void TabButton::nextFocus (void)
{
	if (bKeyboardFocusEnabled == false)
		return;

	CONNECTION_NODE<IKeyboardFocus> *node = point2.pClientList;

	if (focusControl == 0)
	{
		if (node == 0)
			return;
		focusControl = node->client;

		if (focusControl->SetKeyboardFocus(true) == false)
		{
			nextFocus();
		}
		return;
	}

	//
	// find the next control after current focus
	//
	while (node)
	{
		if (node->client == focusControl)
		{
			if ((node = node->pNext) == 0)
				node = point2.pClientList;
			break;
		}
		node = node->pNext;
	}

	if (node && node->client != focusControl)
	{
		focusControl->SetKeyboardFocus(false);
		while (node->client != focusControl)
		{
			if (node->client->SetKeyboardFocus(true))
			{
				focusControl = node->client;
				return;
			}

			if ((node = node->pNext) == 0)
				node = point2.pClientList;
		}
		focusControl->SetKeyboardFocus(true);	// failed to find another control to switch to
	}
}
//----------------------------------------------------------------------------------//
//
CONNECTION_NODE<IKeyboardFocus> * TabButton::findPrevFocus (CONNECTION_NODE<IKeyboardFocus> * pFirst, IKeyboardFocus * pCurr)
{
	// go through everything in the list until pNext == pCurr
	CONNECTION_NODE<IKeyboardFocus> *node = pFirst;

	if (pCurr == NULL)
	{
		pCurr = focusControl;
	}

	while (node)
	{
		if (node->pNext && (node->pNext->client == pCurr))
		{
			return node;
		}
		node = node->pNext;
	}

	// if node was null then pick the last control in the list
	if (node == NULL)
	{
		node = pFirst;
		while (node->pNext)
		{
			node = node->pNext;
		}
	}

	return node;
}
//----------------------------------------------------------------------------------//
//
void TabButton::prevFocus (void)
{
	CONNECTION_NODE<IKeyboardFocus> *node = point2.pClientList;
	CONNECTION_NODE<IKeyboardFocus> *first = point2.pClientList;

	if (focusControl == 0)
	{
		if (node == 0)
			return;
		focusControl = node->client;
		if (focusControl->SetKeyboardFocus(true) == false)
		{
			prevFocus();
		}
		return;
	}

	//
	// find the prev control before current focus
	//
	node = findPrevFocus(first, NULL);

	if (node && node->client != focusControl)
	{
		focusControl->SetKeyboardFocus(false);
		while (node->client != focusControl)
		{
			if (node->client->SetKeyboardFocus(true))
			{
				focusControl = node->client;
				return;
			}

			if ((node = findPrevFocus(first, node->client)) == 0)
				node = point2.pClientList;
		}
		focusControl->SetKeyboardFocus(true);	// failed to find another control to switch to
	}
}
//--------------------------------------------------------------------------//
//
void TabButton::init (TABBUTTONTYPE * _pButtonType)
{
	pButtonType = _pButtonType;

	if (pButtonType->pFontType)
	{
		COMPTR<IDAComponent> pBase;

		GENDATA->CreateInstance(pButtonType->pFontType, pBase);
		pBase->QueryInterface("IFontDrawAgent", pFont);
	}
}
//--------------------------------------------------------------------------//
//-----------------------TabButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE TabButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TabButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	TabButtonFactory (void) { }

	~TabButtonFactory (void);

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
TabButtonFactory::~TabButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void TabButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE TabButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTBUTTON)
	{
		GT_HOTBUTTON * data = (GT_HOTBUTTON *) _data;
	
		if (data->buttonType == HOTBUTTONTYPE::TABBUTTON)
		{
			TABBUTTONTYPE * result = new TABBUTTONTYPE;

			result->pArchetype = _pArchetype;

			if (data->fontType[0])
			{
				result->pFontType = GENDATA->LoadArchetype(data->fontType);
				CQASSERT(result->pFontType);
				GENDATA->AddRef(result->pFontType);
			}

			return result;
		}
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 TabButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	TABBUTTONTYPE * type = (TABBUTTONTYPE *) hArchetype;

	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT TabButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	TABBUTTONTYPE * type = (TABBUTTONTYPE *) hArchetype;
	TabButton * result = new DAComponent<TabButton>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _tabbuttonfactory : GlobalComponent
{
	TabButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<TabButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _tabbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End HotButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//

