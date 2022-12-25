//--------------------------------------------------------------------------//
//                                                                          //
//                               TabControl.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/TabControl.cpp 35    8/22/00 12:44p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#include "pch.h"
#include <globals.h>

#include <TComponent.h>
#include <DTabControl.h>

#include "ITabControl.h"
#include "Frame.h"
#include "IHotButton.h"
#include "IShapeLoader.h"
#include "IImageReader.h"


#include "IButton2.h"
#include "Startup.h"
#include "BaseHotRect.h"
#include "GenData.h"
#include <DButton.h>
#include "DrawAgent.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>


#define TABS_ID_START  0x00ff00ff

#define TABWIDTH  60
#define TABHEIGHT 20

//--------------------------------------------------------------------------//
//
struct TABTYPE
{
	PGENTYPE pArchetype;
	U32 width, height;
	S32 leftMargin, rightMargin;		// skip pixels on the edge (for dropdown arrows)
	bool bBottomTabs;

	COLORREF normalColor, hiliteColor, selectedColor;

	TABTYPE (void)
	{
	}

	~TABTYPE (void)
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
struct DACOM_NO_VTABLE TabControl : BaseHotRect, ITabControl, IKeyboardFocus
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(TabControl)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(ITabControl)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IKeyboardFocus)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	TABTYPE * tabType;
	U32 controlID;					// set by owner
	bool bTabsLoaded;
	int controlHeight;
	int controlWidth;

	COMPTR<ITabButton> tabs[MAX_TABS];
	COMPTR<IDrawAgent> borderDrawAgent;
	int nTabs;
	int iSelected;
	
	//
	// class methods
	//

	TabControl (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~TabControl (void)
	{
		GENDATA->Release(tabType->pArchetype);
	}

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* ITabControl methods  */

	virtual void InitTab (const TABCONTROL_DATA & data, BaseHotRect * parent, IShapeLoader * pLoader);

	virtual GENRESULT __stdcall GetTabMenu (U32 index, struct BaseHotRect ** ppMenu);

	virtual void SetControlID (U32 id);

	virtual U32 GetControlID (void);

	virtual int GetTabCount (void) { return nTabs;}

	virtual int GetCurrentTab (void) { return iSelected; }

	virtual int SetCurrentTab (int tabID) 
	{
/*		if (tabID >= nTabs || tabID < 0)
		{
			// give the parent keyboard focus okay?
		}
		else
		{
*/
		if (tabID >=0 && tabID < nTabs)
		{
			int oldTab = iSelected;
			onLeftHotbutton(tabID + TABS_ID_START);
			return oldTab;
		}

		return -1;
	}

	virtual void SetDefaultControlForTab (int tabID, IDAComponent * component)
	{
		CQASSERT(tabID >= 0 && tabID < nTabs);
		tabs[tabID]->SetDefaultFocusControl(component);
	}

	virtual void EnableKeyboardFocusing (void)
	{
		for (int i = 0; i < nTabs; i++)
		{
			tabs[i]->EnableKeyboardFocusing();
		}
	}


	/* IKeyboardFocus methods */
	virtual bool SetKeyboardFocus (bool bEnable) { return 1;}

	
	/* TabControl methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void draw (void);

	void init (TABTYPE * pTabType);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftHotbutton (int id)
	{
		int tabID = id - TABS_ID_START;

		if (tabID >= 0 && tabID < nTabs)
		{
			// hit a tab
			iSelected = tabID;
			for (int i = 0; i < nTabs; i++)
			{
				tabs[i]->SetPushState(i == tabID);
				tabs[i]->SetTabSelected(i == tabID);
			}
			parent->PostMessage(CQE_TABCONTROL_CHANGE, (void*)iSelected);
		}
		else
		{
			parent->PostMessage(CQE_LHOTBUTTON, (void*)id);
		}
	}

/*	virtual void onSetFocus (bool bFocus)
	{
		parent->onSetFocus(bFocus);
	}
*/
};
//--------------------------------------------------------------------------//
//
void TabControl::InitTab (const TABCONTROL_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQASSERT(data.numTabs && "Need to set the data for a Tab Control, must have a non zero tab");
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	COMPTR<IDAComponent> pBase;
	COMPTR<IImageReader> reader;
	int width, height;
	int space = 0;
	HOTBUTTON_DATA dumbData[MAX_TABS];
	memset(dumbData, 0, sizeof(dumbData));

	U32 baseImage = data.iBaseImage;
	if(!baseImage)
		baseImage = 1;

	// create the HotButtons
	if (bTabsLoaded == false)
	{
		// load the base tab control image
		loader->CreateImageReader(baseImage, reader);
		controlWidth  = reader->GetWidth();
		controlHeight = reader->GetHeight();

		loader->CreateDrawAgent(baseImage, borderDrawAgent, NULL);
	} 

	// set the screen rect
	RECT rcParent;
	_parent->GetRect(&rcParent);

	screenRect.left = rcParent.left + IDEAL2REALX(data.xpos);
	screenRect.right = rcParent.right + IDEAL2REALX(data.xpos + controlWidth);
	screenRect.top = rcParent.top + IDEAL2REALY(data.ypos);
	screenRect.bottom = rcParent.bottom + IDEAL2REALY(data.ypos + controlHeight);

	for (int i = 0; i < data.numTabs; i++)
	{
		loader->CreateImageReader(i*4 + 1 + baseImage, reader);
		width = reader->GetWidth();
		height = reader->GetHeight();

		dumbData[i].baseImage = i*4 + 1 + baseImage;
		dumbData[i].xOrigin = space;
		dumbData[i].yOrigin = (tabType->bBottomTabs) ? controlHeight : -height;

		space += width;

		if (tabs[i] == 0)
		{
			if (data.hotButtonType[0])
			{
				GENDATA->CreateInstance(data.hotButtonType, pBase);
			}
			else
			{
				GENDATA->CreateInstance("HotButton!!Tab", pBase);
			}
			pBase->QueryInterface("ITabButton", tabs[i]);
		}

		tabs[i]->InitTabButton(dumbData[i], this, loader);
		
		if (data.textID[i])
		{
			tabs[i]->SetTextID(data.textID[i]);
			tabs[i]->SetTextColorNormal(tabType->normalColor);
			tabs[i]->SetTextColorHilite(tabType->hiliteColor);
			tabs[i]->SetTextColorSelected(tabType->selectedColor);
		}
		
		tabs[i]->SetControlID(i + TABS_ID_START);
	}

	nTabs = data.numTabs;
	
	onLeftHotbutton(iSelected + TABS_ID_START);

	bTabsLoaded = true;
}
//--------------------------------------------------------------------------//
//
GENRESULT __stdcall TabControl::GetTabMenu (U32 index, struct BaseHotRect ** ppMenu)
{
	CQASSERT(index < (U32)nTabs);
	return tabs[index]->QueryInterface("BaseHotRect", (void **)ppMenu);
}
//--------------------------------------------------------------------------//
//
void TabControl::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 TabControl::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
GENRESULT TabControl::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		draw();
		break;

	case CQE_LHOTBUTTON:
		onLeftHotbutton((U32)param);
		break;

//	case WM_KEYDOWN:
	case CQE_BUTTON:
	case CQE_SLIDER:
	case CQE_LIST_CARET_MOVED:
	case CQE_LIST_SELECTION:
		parent->PostMessage(message, param);		// forward the message upward
		break;

	case CQE_SET_FOCUS:
		if (bInvisible)
		{
			onSetFocus(true);
			return GR_OK;
		}
		else
			SetCurrentTab(GetCurrentTab());
		break;

	case CQE_KILL_FOCUS:
		if (bInvisible)
		{
			onSetFocus(false);
			return GR_OK;
		}
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void TabControl::init (TABTYPE * pTabType)
{
	tabType = pTabType;
}
//--------------------------------------------------------------------------//
//
void TabControl::draw (void)
{
	if (borderDrawAgent)
	{
		BATCH->set_state(RPR_BATCH,FALSE);
		borderDrawAgent->Draw(NULL, screenRect.left, screenRect.top);
	}
}
//--------------------------------------------------------------------------//
//-----------------------TabControl Factory class---------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE TabControlFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(TabControlFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	TabControlFactory (void) { }

	~TabControlFactory (void);

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
TabControlFactory::~TabControlFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void TabControlFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE TabControlFactory::CreateArchetype (PGENTYPE pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_TABCONTROL)
	{

		GT_TABCONTROL * data = (GT_TABCONTROL *) _data;
		TABTYPE * result = new TABTYPE;

		result->pArchetype = pArchetype;
		result->leftMargin = 20;
		result->rightMargin = 10;
		result->bBottomTabs = true;
		result->normalColor = RGB(data->normalColor.red, data->normalColor.green, data->normalColor.blue);
		result->hiliteColor = RGB(data->hiliteColor.red, data->hiliteColor.green, data->hiliteColor.blue);
		result->selectedColor = RGB(data->selectedColor.red, data->selectedColor.green, data->selectedColor.blue);

		return result;
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 TabControlFactory::DestroyArchetype (HANDLE hArchetype)
{
	TABTYPE * type = (TABTYPE *) hArchetype;
	delete type;
	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT TabControlFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	TABTYPE * type = (TABTYPE *) hArchetype;
	TabControl * result = new DAComponent<TabControl>;

	result->init(type);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _tabcontrolfactory : GlobalComponent
{
	TabControlFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<TabControlFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _tabcontrolfactory startup;





//-----------------------------------------------------------------------------------------//
//----------------------------------End TabControl.cpp-------------------------------------//
//-----------------------------------------------------------------------------------------//
