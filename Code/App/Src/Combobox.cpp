//--------------------------------------------------------------------------//
//                                                                          //
//                               Combobox.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Combobox.cpp 27    19/10/01 13:25 Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include "IButton2.h"
#include "IEdit2.h"
#include "IListBox.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DCombobox.h>
#include "DrawAgent.h"
//#include "Hotkeys.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE Combobox : BaseHotRect, ICombobox, IKeyboardFocus, IListbox
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(Combobox)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IListbox)
	DACOM_INTERFACE_ENTRY(ICombobox)
	DACOM_INTERFACE_ENTRY(IEdit2)
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
	COMPTR<IEdit2>   edit;

	int previousSize;
	U32 controlID;
	//
	// class methods
	//

	Combobox (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~Combobox (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEdit2 methods */
	
	virtual void InitEdit (const EDIT_DATA & data, BaseHotRect * parent); 

	virtual void EnableEdit (bool bEnable);

	virtual void SetText (const wchar_t * szText, S32 firstHighlightChar = 0);

	virtual int  GetText (wchar_t * szBuffer, int bufferSize);

	virtual void SetMaxChars (U32 maxChars);		// allow user to enter maxChars-1

	virtual void EnableToolbarBehavior (void);		// edit control in the toolbar

	virtual void EnableChatboxBehavior (void)
	{
	}

	virtual void EnableLockedTextBehavior (void)
	{
	}

	virtual void DisableInput (bool bDisableInput)
	{
		edit->DisableInput(bDisableInput);
	}

	virtual void SetTransparentBehavior (bool bTransparent)
	{
		edit->SetTransparentBehavior(bTransparent);
	}

	virtual void SetIgnoreChars (wchar_t * ignoreChars)
	{
		edit->SetIgnoreChars(ignoreChars);
	}

	virtual const U32 GetEditWidth (void) const
	{
		return edit->GetEditWidth();
	}

	virtual bool IsTextAllVisible (void)
	{
		return true;
	}

	/* ICombobox methods  */

	virtual void InitCombobox (const COMBOBOX_DATA & data, BaseHotRect * parent); 

	virtual void EnableCombobox (bool bEnable);

	/* IListbox methods  */

	virtual void InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent);
	
	virtual void EnableListbox (bool bEnable)
	{
		list->EnableListbox(bEnable);
	}

	virtual void SetVisible (bool bVisible);

	virtual bool GetVisible (void)
	{
		return !bInvisible;
	}

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

	virtual S32 GetBreakIndex(const wchar_t * szString)
	{
		return -1;
	}

	virtual const bool IsMouseOver (S16 x, S16 y) const
	{
		return false;
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

	/* Combobox methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE pArchetype);

	void draw (void);

	bool isAlert (IDAComponent *pComp)
	{
		COMPTR<BaseHotRect> pRect;
		bool result = false;
		if (pComp->QueryInterface("BaseHotRect", pRect)== GR_OK)
		{
			result = pRect->bAlert;
		}
		return result;
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Combobox::~Combobox (void)
{
	GENDATA->Release(pArchetype);
}
//--------------------------------------------------------------------------//
//
void Combobox::InitCombobox (const COMBOBOX_DATA & data, BaseHotRect * _parent)
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

		GENDATA->CreateInstance(data.editData.editType, pComp);
		pComp->QueryInterface("IEdit2", edit);
	}

	screenRect.left   = data.screenRect.left   + parent->screenRect.left;
	screenRect.right  = data.screenRect.right  + parent->screenRect.left;
	screenRect.top    = data.screenRect.top    + parent->screenRect.top;
	screenRect.bottom = data.screenRect.bottom + parent->screenRect.top;

	button->InitButton(data.buttonData, this);
	list->InitListbox(data.listboxData, this);
	edit->InitEdit(data.editData, this);

	button->SetControlID(1);
	list->SetControlID(2);
	edit->SetControlID(3);

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
void Combobox::EnableCombobox (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
		bDisabled = !bEnable;
		
		button->EnableButton(bEnable);
		edit->EnableEdit(bEnable);
		
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
void Combobox::InitListbox (const LISTBOX_DATA & data, BaseHotRect * parent)
{
	CQBOMB0("InitListbox() not supported.");
}
//--------------------------------------------------------------------------//
//
void Combobox::SetVisible (bool bVisible)
{
	bInvisible = !bVisible;
	button->SetVisible(bVisible);
	edit->SetVisible(bVisible);

	if (bVisible==false)
		bDropped = false;
	list->SetVisible(bVisible && bDropped);
	button->SetPushState(bVisible && bDropped);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::AddStringToHead (const wchar_t * szString)
{
	return list->AddStringToHead(szString);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::AddString (const wchar_t * szString)
{
	return list->AddString(szString);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::FindString (const wchar_t * szString)
{
	return list->FindString(szString);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::FindStringExact (const wchar_t * szString)
{
	return list->FindStringExact(szString);
}
//--------------------------------------------------------------------------//
//
void Combobox::RemoveString (S32 index)
{
	list->RemoveString(index);
}
//--------------------------------------------------------------------------//
//
U32 Combobox::GetString (S32 index, wchar_t * buffer, U32 bufferSize)
{
	return list->GetString(index, buffer, bufferSize);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::SetString (S32 index, const wchar_t * szString)
{
	return list->SetString(index, szString);
}
//--------------------------------------------------------------------------//
//
void Combobox::SetDataValue (S32 index, U32 data)
{
	list->SetDataValue(index, data);
}
//--------------------------------------------------------------------------//
//
U32 Combobox::GetDataValue (S32 index)
{
	return list->GetDataValue(index);
}
//--------------------------------------------------------------------------//
//
void Combobox::SetColorValue (S32 index, COLORREF color)
{
	list->SetColorValue(index, color);
}
//--------------------------------------------------------------------------//
//
COLORREF Combobox::GetColorValue (S32 index)
{
	return list->GetColorValue(index);
}
//--------------------------------------------------------------------------//
//
S32 Combobox::GetCurrentSelection (void)
{
	return list->GetCurrentSelection();
}
//--------------------------------------------------------------------------//
//
S32 Combobox::SetCurrentSelection (S32 newIndex)
{
	S32 result = list->SetCurrentSelection(newIndex);
	wchar_t string[256];

	if (list->GetString(newIndex, string, sizeof(string)))
		button->SetTextString(string);

	return result;
}
//--------------------------------------------------------------------------//
//
S32 Combobox::GetCaretPosition (void)
{
	return list->GetCaretPosition();
}
//--------------------------------------------------------------------------//
//
S32 Combobox::SetCaretPosition (S32 newIndex)
{
	return list->SetCaretPosition(newIndex);
}
//--------------------------------------------------------------------------//
//
void Combobox::ResetContent (void)
{
	list->ResetContent();
}
//--------------------------------------------------------------------------//
//
U32 Combobox::GetNumberOfItems (void)
{
	return list->GetNumberOfItems();
}
//--------------------------------------------------------------------------//
//
S32 Combobox::GetTopVisibleString (void)
{
	return list->GetTopVisibleString();
}
//--------------------------------------------------------------------------//
//
S32 Combobox::GetBottomVisibleString (void)
{
	return list->GetBottomVisibleString();
}
//--------------------------------------------------------------------------//
//
void Combobox::EnsureVisible (S32 index)
{
	list->EnsureVisible(index);
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollPageUp (void)
{
	list->ScrollPageUp();
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollPageDown (void)
{
	list->ScrollPageDown();
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollLineUp (void)
{
	list->ScrollLineUp();
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollLineDown (void)
{
	list->ScrollLineDown();
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollHome (void)
{
	list->ScrollHome();
}
//--------------------------------------------------------------------------//
//
void Combobox::ScrollEnd (void)
{
	list->ScrollEnd();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretPageUp (void)
{
	list->CaretPageUp();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretPageDown (void)
{
	list->CaretPageDown();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretLineUp (void)
{
	list->CaretLineUp();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretLineDown (void)
{
	list->CaretLineDown();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretHome (void)
{
	list->CaretHome();
}
//--------------------------------------------------------------------------//
//
void Combobox::CaretEnd (void)
{
	list->CaretEnd();
}
//--------------------------------------------------------------------------//
//
void Combobox::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
bool Combobox::SetKeyboardFocus (bool bEnable)
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
				edit->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(false);
				list->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(true);
			}
			else
			{
				list->QueryInterface("IKeyboardFocus", res);
				res->SetKeyboardFocus(false);
				edit->QueryInterface("IKeyboardFocus", res);
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
			
			edit->QueryInterface("IKeyboardFocus", res);
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
U32 Combobox::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT Combobox::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	if (bInvisible==false)
	switch (message)
	{
	case CQE_BUTTON:
		if (S32(param) >= 0)		// ignore double-click message
		{
			// was the message sent by the edit control?
			if (U32(param) == edit->GetControlID())	
			{
				parent->PostMessage(CQE_BUTTON,  (void*)controlID);
			}
			else
			{
				bDropped = !bDropped;
				list->SetVisible(bDropped);
				button->SetPushState(bDropped);
				if (bKeyboardFocus)
					SetKeyboardFocus(true);
				else
					parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
			}
		}
		break;

	case CQE_EDIT_CHANGED:
		{
			// see if the text in the edit control matches something in the list
			wchar_t string[256];
			int index;
			int sz =  edit->GetText(string, 256);
			CQASSERT(sz <= 256);

			if (sz > previousSize)
			{
				index = list->FindString(string);
				if (index  > -1)
				{
					int strLen;

					// we are going to replace the string in the edit box and higlight past the
					// size of the string originally in the edit control
					wchar_t newString[256];
					list->GetString(index, newString, 256);
					strLen = wcslen(string);

					edit->SetText(newString, strLen);
				}
			}
			previousSize = edit->GetText(0, 0);
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
	
	case CQE_LIST_CARET_MOVED:
		{
			// get index and string for where the caret is, and feed it to the edit control
			wchar_t string[256];

			if (list->GetCurrentSelection() > -1)
			{
				list->GetString(list->GetCurrentSelection(), string, sizeof(string));
				edit->SetText(string);
			}
		}
		break;

	case WM_LBUTTONDOWN:
		if (bDisabled==false)
		{
			GENRESULT result = BaseHotRect::Notify(message, param);
			
			if (bDropped)
			{
				if (isAlert(edit))
				{
					bDropped = !bDropped;
					list->SetVisible(bDropped);
					button->SetPushState(bDropped);
					if (bKeyboardFocus)
						SetKeyboardFocus(true);
					else
						parent->Notify(CQE_KEYBOARD_FOCUS, getBase());
				}
			}
			else if (isAlert(edit))
			{
				if (bKeyboardFocus)
					SetKeyboardFocus(true);
				else
					parent->Notify(CQE_KEYBOARD_FOCUS, getBase());	
			}

			return result;
		}
		else
		{
			return isAlert(edit) ? GR_GENERIC : GR_OK;
		}
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Combobox::draw (void)
{
}
//--------------------------------------------------------------------------//
//
void Combobox::init (PGENTYPE _pArchetype)
{
	pArchetype = _pArchetype;
}

//-----------------------------------------------------------------------------------------//
//
void Combobox::InitEdit (const EDIT_DATA & data, BaseHotRect * parent)
{
	CQBOMB0("InitEdit() not supported.");	
}

//-----------------------------------------------------------------------------------------//
//
void Combobox::EnableEdit (bool bEnable)
{
/*	if (bEnable == bDisabled)	// if we are changing state
	{
		bHasFocus = bEnable;	// keep base class from grabbing keyboard focus
		bDisabled = !bEnable;
		
		edit->EnableEdit(bEnable);
		
		if (bDisabled)
		{
			bDropped = false;
			list->SetVisible(false);
			button->SetPushState(false);
		}
	}
*/
	EnableCombobox(bEnable);

//	edit->EnableEdit(bEnable);
}

//-----------------------------------------------------------------------------------------//
//
void Combobox::SetText (const wchar_t * szText, S32 firstHighlightChar)
{
	edit->SetText(szText, firstHighlightChar);
}

//-----------------------------------------------------------------------------------------//
//
int Combobox::GetText (wchar_t * szBuffer, int bufferSize)
{
	return edit->GetText(szBuffer, bufferSize);
}

//-----------------------------------------------------------------------------------------//
//
void Combobox::SetMaxChars (U32 maxChars)
{
	edit->SetMaxChars(maxChars);
}

//-----------------------------------------------------------------------------------------//
//
void Combobox::EnableToolbarBehavior (void)
{
	edit->EnableToolbarBehavior();
}


//--------------------------------------------------------------------------//
//-----------------------Combobox Factory class-----------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ComboboxFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ComboboxFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ComboboxFactory (void) { }

	~ComboboxFactory (void);

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

	/* ComboboxFactory methods */

	IDAComponent * getBase (void)
	{
		return static_cast<ICQFactory *> (this);
	}
};
//-----------------------------------------------------------------------------------------//
//
ComboboxFactory::~ComboboxFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ComboboxFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ComboboxFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_COMBOBOX)
	{
		CQASSERT(pArchetype==0);
		pArchetype = _pArchetype;
		
		return (HANDLE) 1;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ComboboxFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT(hArchetype == (HANDLE)1);
	pArchetype = 0;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ComboboxFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT(hArchetype == (HANDLE)1);

	Combobox * result = new DAComponent<Combobox>;

	result->init(pArchetype);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _ComboboxFactory : GlobalComponent
{
	ComboboxFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ComboboxFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};


static _ComboboxFactory startup;
//-----------------------------------------------------------------------------------------//
//----------------------------------End Combobox.cpp----------------------------------------//
//-----------------------------------------------------------------------------------------//
