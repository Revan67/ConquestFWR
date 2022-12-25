//--------------------------------------------------------------------------//
//                                                                          //
//                               Dropdown.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Dropdown.cpp 25    9/05/00 4:18p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IButton2.h"
#include "IListBox.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DDropdown.h>
#include "DrawAgent.h"
//#include "Hotkeys.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Dropdown : BaseHotRect, IDropdown, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Dropdown)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IListbox)
	DACOM_INTERFACE_ENTRY(IDropdown)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PGENTYPE pArchetype;

	bool bKeyboardFocus;
	bool bDisabled;
	bool bDropped;

	COMPTR<IListbox> list;
	COMPTR<IButton2> button;

	U32 controlID;
	//
	// class methods
	//

	Dropdown (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~Dropdown (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDropdown methods  */

	virtual void InitDropdown (const DROPDOWN_DATA & data, BaseHotRect * parent); 

	virtual void EnableDropdown (bool bEnable);

	virtual void SetSelectionColor (COLORREF color)
	{
		button->SetDefaultColor(color);
	}

	/* IListbox methods  */

	virtual void InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent);
	
	virtual void EnableListbox (bool bEnable)
	{
		list->EnableListbox(bEnable);
	}

	virtual void SetVisible (bool bVisible);

	virtual S32 AddStringToHead (const wchar_t * szString);

	virtual S32 AddString (const wchar_t * szString);	// returns index

	virtual S32 FindString (const wchar_t * szString);	// returns index of matching string prefix

	virtual S32 FindStringExact (const wchar_t * szString);	// returns index of matching string

	virtual void RemoveString (S32 index);	

	virtual U32 GetString (S32 index, wchar_t * buffer, U32 bufferSize);	// returns length of string (in characters)

	virtual S32 SetString (S32 index, const wchar_t * szString);	// changes string value, returns index

	virtual void SetDataValue (S32 index, U32 data);	// set user defined data item

	virtual U32 GetDataValue (S32 index);	// return user defined value, 0 on error

	virtual void SetColorValue (S32 index, COLORREF color);		// set color of text for item

	virtual COLORREF GetColorValue (S32 index);
	
	virtual S32 GetCurrentSelection (void);		// returns index
		
	virtual S32 SetCurrentSelection (S32 newIndex);		// returns index

	virtual S32 GetCaretPosition (void);		// returns index
		
	virtual S32 SetCaretPosition (S32 newIndex);		// returns old caret index

	virtual void ResetContent (void);		// remove all items from list

	virtual U32 GetNumberOfItems (void);

	virtual S32 GetTopVisibleString (void);
	
	virtual S32 GetBottomVisibleString (void);

	virtual void EnsureVisible (S32 index);		// make sure a string is visible

	virtual void ScrollPageUp (void);

	virtual void ScrollPageDown (void);

	virtual void ScrollLineUp (void);

	virtual void ScrollLineDown (void);

	virtual void ScrollHome (void);

	virtual void ScrollEnd (void);

	virtual void CaretPageUp (void);

	virtual void CaretPageDown (void);

	virtual void CaretLineUp (void);

	virtual void CaretLineDown (void);

	virtual void CaretHome (void);

	virtual void CaretEnd (void);

	virtual void SetControlID (U32 id);

	virtual	S32 GetBreakIndex(const wchar_t * szString)
	{
		return -1;
	}

	virtual const bool IsMouseOver (S16 x, S16 y) const
	{
		return false; // not supported
	}
		

	/* IKeyboardFocus methods */

	virtual bool SetKeyboardFocus (bool bEnable);

	virtual U32 GetControlID (void);

	/* BaseHotRect methods */

	virtual void onRequestKeyboardFocus (int x, int y)
	{
	}

	virtual void DrawRect (void)
	{
		if (bDropped)
		{
			U32 color = RGB(205,9,210);		// purple
			//
			// draw the box
			//
			DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.right, screenRect.top, color);
			DA::LineDraw(0, screenRect.right, screenRect.top, screenRect.right, screenRect.bottom, color);
			DA::LineDraw(0, screenRect.right, screenRect.bottom, screenRect.left, screenRect.bottom, color);
			DA::LineDraw(0, screenRect.left, screenRect.bottom, screenRect.left, screenRect.top, color);
		}
	}

	/* Dropdown methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE pArchetype);

	void draw (void);

	bool checkAlertness (S16 x, S16 y);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Dropdown::~Dropdown (void)
{
	GENDATA->Release(pArchetype);
}
//--------------------------------------------------------------------------//
//
void Dropdown::InitDropdown (const DROPDOWN_DATA & data, BaseHotRect * _parent)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());

		//
		// load our sub-components
		//
		COMPTR<IDAComponent> pComp;
		GENDATA->CreateInstance(data.buttonData.buttonType, pComp);
		pComp->QueryInterface("IButton2", button);

		GENDATA->CreateInstance(data.listboxData.listboxType, pComp);
		pComp->QueryInterface("IListbox", list);
	}

	screenRect.left   = IDEAL2REALX(data.screenRect.left) + parent->screenRect.left;
	screenRect.right  = IDEAL2REALX(data.screenRect.right)  + parent->screenRect.left;
	screenRect.top    = IDEAL2REALY(data.screenRect.top)    + parent->screenRect.top;
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom) + parent->screenRect.top;

	button->InitButton(data.buttonData, this);
	list->InitListbox(data.listboxData, this);

	button->SetDropdownBehavior();

/*
	COMPTR<BaseHotRect> hotrect;
	list->QueryInterface("BaseHotRect", hotrect);

	hotrect->parent = this;		// send events to me
	parent->SetCallbackPriority(hotrect, HOTRECT_PRIORITY+1);
*/

	list->SetVisible(bInvisible==false && bDropped);
}
//--------------------------------------------------------------------------//
//
void Dropdown::EnableDropdown (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
		bDisabled = !bEnable;
		button->EnableButton(bEnable);
		if (bDisabled)
		{
			bDropped = false;
			list->SetVisible(false);
			button->SetPushState(false);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Dropdown::InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent)
{
	CQBOMB0("InitListbox() not supported.");
}
//--------------------------------------------------------------------------//
//
void Dropdown::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
	button->SetVisible(bVisible);
	if (bVisible==false)
		bDropped = false;
	list->SetVisible(bVisible && bDropped);
	button->SetPushState(bVisible && bDropped);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::AddStringToHead (const wchar_t * szString)
{
	return list->AddStringToHead(szString);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::AddString (const wchar_t * szString)
{
	return list->AddString(szString);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::FindString (const wchar_t * szString)
{
	return list->FindString(szString);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::FindStringExact (const wchar_t * szString)
{
	return list->FindStringExact(szString);
}
//--------------------------------------------------------------------------//
//
void Dropdown::RemoveString (S32 index)
{
	list->RemoveString(index);
}
//--------------------------------------------------------------------------//
//
U32 Dropdown::GetString (S32 index, wchar_t * buffer, U32 bufferSize)
{
	return list->GetString(index, buffer, bufferSize);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::SetString (S32 index, const wchar_t * szString)
{
	return list->SetString(index, szString);
}
//--------------------------------------------------------------------------//
//
void Dropdown::SetDataValue (S32 index, U32 data)
{
	list->SetDataValue(index, data);
}
//--------------------------------------------------------------------------//
//
U32 Dropdown::GetDataValue (S32 index)
{
	return list->GetDataValue(index);
}
//--------------------------------------------------------------------------//
//
void Dropdown::SetColorValue (S32 index, COLORREF color)
{
	list->SetColorValue(index, color);
}
//--------------------------------------------------------------------------//
//
COLORREF Dropdown::GetColorValue (S32 index)
{
	return list->GetColorValue(index);
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::GetCurrentSelection (void)
{
	return list->GetCurrentSelection();
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::SetCurrentSelection (S32 newIndex)
{
	S32 result = list->SetCurrentSelection(newIndex);
	wchar_t string[256];

	if (list->GetString(newIndex, string, sizeof(string)))
		button->SetTextString(string);

	return result;
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::GetCaretPosition (void)
{
	return list->GetCaretPosition();
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::SetCaretPosition (S32 newIndex)
{
	return list->SetCaretPosition(newIndex);
}
//--------------------------------------------------------------------------//
//
void Dropdown::ResetContent (void)
{
	button->SetTextString(L"");
	list->ResetContent();
}
//--------------------------------------------------------------------------//
//
U32 Dropdown::GetNumberOfItems (void)
{
	return list->GetNumberOfItems();
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::GetTopVisibleString (void)
{
	return list->GetTopVisibleString();
}
//--------------------------------------------------------------------------//
//
S32 Dropdown::GetBottomVisibleString (void)
{
	return list->GetBottomVisibleString();
}
//--------------------------------------------------------------------------//
//
void Dropdown::EnsureVisible (S32 index)
{
	list->EnsureVisible(index);
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollPageUp (void)
{
	list->ScrollPageUp();
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollPageDown (void)
{
	list->ScrollPageDown();
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollLineUp (void)
{
	list->ScrollLineUp();
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollLineDown (void)
{
	list->ScrollLineDown();
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollHome (void)
{
	list->ScrollHome();
}
//--------------------------------------------------------------------------//
//
void Dropdown::ScrollEnd (void)
{
	list->ScrollEnd();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretPageUp (void)
{
	list->CaretPageUp();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretPageDown (void)
{
	list->CaretPageDown();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretLineUp (void)
{
	list->CaretLineUp();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretLineDown (void)
{
	list->CaretLineDown();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretHome (void)
{
	list->CaretHome();
}
//--------------------------------------------------------------------------//
//
void Dropdown::CaretEnd (void)
{
	list->CaretEnd();
}
//--------------------------------------------------------------------------//
//
void Dropdown::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
bool Dropdown::SetKeyboardFocus (bool bEnable)
{
	if (bEnable && (bDisabled||bInvisible))
		return false;
//	if (bEnable != bKeyboardFocus)  // if we are changing state
	{
		bKeyboardFocus = bEnable;
		COMPTR<IKeyboardFocus> res;

		if (bKeyboardFocus)
		{
			if (bDropped)
			{
				button->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(false);
				list->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(true);
			}
			else
			{
				list->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(false);
				button->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(true);
			}
			parent->SetCallbackPriority(this, HOTRECT_PRIORITY+1);	// for draw order
		}
		else
		{
			button->QueryInterface("IKeyboardFocus", res);
			res->SetKeyboardFocus(false);
			list->QueryInterface("IKeyboardFocus", res);
			res->SetKeyboardFocus(false);
			bDropped = false;
			list->SetVisible(false);
			button->SetPushState(false);
			parent->SetCallbackPriority(this, HOTRECT_PRIORITY);
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
U32 Dropdown::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT Dropdown::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_BUTTON:
		if (S32(param) >= 0)		// ignore double-click message
		{
			bDropped = !bDropped;
			list->SetVisible(bDropped);
			button->SetPushState(bDropped);

			if (bKeyboardFocus)
				SetKeyboardFocus(true);
			else
				parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
		}
		break;

	case WM_CHAR:
		switch (TCHAR(msg->wParam))
		{
		case 27:	// ESC pressed
			if (bDropped)
			{
				bDropped = false;
				list->SetVisible(false);
				button->SetPushState(false);
				if (bKeyboardFocus)
					SetKeyboardFocus(true);
				else
					parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
				return GR_GENERIC;
			}
			break; // end case 27
		}
		break; // end case WM_CHAR

	case CQE_LIST_SELECTION:
		{
			wchar_t string[256];

			list->GetString(list->GetCurrentSelection(), string, sizeof(string));
			button->SetTextString(string);
			bDropped = false;
			list->SetVisible(false);
			button->SetPushState(false);
			if (bKeyboardFocus)
				SetKeyboardFocus(true);
			else
				parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
			parent->PostMessage(CQE_LIST_SELECTION, (void *)controlID);
		}
		break;

		// why they hell was this here?  And why did hell did it work for so long? -sb
/*	case WM_LBUTTONDOWN:
		{
			GENRESULT result = BaseHotRect::Notify(message, param);
			if (bDropped && bAlert==0)
			{
				bDropped = false;
				list->SetVisible(false);
				button->SetPushState(false);
				if (bKeyboardFocus)
					SetKeyboardFocus(true);
				else
					parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
				result = GR_GENERIC;	// eat the message
			}
			return result;
		}
*/
	case WM_MOUSEMOVE:
		{
			BaseHotRect::Notify(message, param);

			// we need a better way to determine if this control is alert
			// it is alert if the button or the listbox has the mouse over it
			S16 x = static_cast<S16>(LOWORD(msg->lParam));
			S16 y = static_cast<S16>(HIWORD(msg->lParam)); 
			checkAlertness(x, y);

			// we'll want to eat the message if the listbox is alerted
			if (bAlert)
			{
				if (list->IsMouseOver(x, y))
				{
					// eat the message
					return GR_GENERIC;
				}
			}
		}
		return GR_OK;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
bool Dropdown::checkAlertness (S16 x, S16 y)
{
	bAlert = 0;

	if (bHasFocus && bInvisible == false)
	{
		// is the mouse over the button component?
		if (button->IsMouseOver(x, y))
		{
			bAlert = 1;
		}

		// is the mouse over the listbox component?
		if (list->IsMouseOver(x, y))
		{
			bAlert = 1;
		}
	}

	return bAlert;
}
//--------------------------------------------------------------------------//
//
void Dropdown::draw (void)
{
}
//--------------------------------------------------------------------------//
//
void Dropdown::init (PGENTYPE _pArchetype)
{
	pArchetype = _pArchetype;
}
//--------------------------------------------------------------------------//
//-----------------------Dropdown Factory class-----------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE DropdownFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(DropdownFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	DropdownFactory (void) { }

	~DropdownFactory (void);

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

	/* DropdownFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
DropdownFactory::~DropdownFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void DropdownFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE DropdownFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_DROPDOWN)
	{
		CQASSERT(pArchetype==0);
		pArchetype = _pArchetype;
		
		return (HANDLE) 1;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 DropdownFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT(hArchetype == (HANDLE)1);
	pArchetype = 0;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT DropdownFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT(hArchetype == (HANDLE)1);

	Dropdown * result = new DAComponent<Dropdown>;

	result->init(pArchetype);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _dropdownFactory : GlobalComponent
{
	DropdownFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<DropdownFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _dropdownFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Dropdown.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//
