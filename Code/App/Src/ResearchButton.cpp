//--------------------------------------------------------------------------//
//                                                                          //
//                               ResearchButton.cpp                         //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ResearchButton.cpp 44    9/01/00 4:36p Jasony $
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
#include "IActiveButton.h"
#include <DSounds.h>
#include "DResearch.h"
#include "ObjList.h"

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>
#include <stdio.h>

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ResearchButton : BaseHotRect, IHotButton, IActiveButton
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(ResearchButton)
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IHotButton)
	DACOM_INTERFACE_ENTRY(IActiveButton)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()

	//
	// data items
	// 
	PGENTYPE pArchetype;
	PGENTYPE pFontType;
	HSOUND hSound;
	COMPTR<IDrawAgent> shapes[GTHBSHP_MAX_SHAPES];
	COMPTR<IDrawAgent> noMoneyShape;
	int baseImage;
	COMPTR<IShapeLoader> loader;
	COMPTR<IFontDrawAgent> pFont;
	S16 yText, xText;
//	char researchArchetypeName[GT_PATH];

	U32 controlID;					// set by owner
	HBTNTXT::RESEARCH_TEXT buttonText;	
	S32 numericValue;
	SINGLE_TECHNODE nodeDepend, nodeAchieved;
	TECHNODE currentNode;
	S32 currentUpgrade;
	U32 researchID;
	S32 upgradeID;
	ResourceCost buildCost;
	U32 stall;
	HOTKEYS::HOTKEY hotkey;

	bool bReseachState:1;

	bool bDisabled:1;
	bool bPushState:1;			// used for selection state
	bool bPostMessageBlocked:1;

	bool bDrawingPercent:1;

	bool bHighlight:1;

	U32 queueNumber;
	SINGLE percentDone;

	//
	// class methods
	//

	ResearchButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~ResearchButton (void);

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
		// nothing to do, always uses context behavior
	}

	virtual void SetHighlightState (bool _bHighlight)
	{
		bHighlight = _bHighlight;
	}

	virtual void SetTabBehavior (void) {}

	virtual void SetTabSelected (bool bSelected) {}


	/* IActiveButton methods */

	virtual void SetTechNode(TECHNODE node,TECHNODE allowed,TECHNODE workingNode);

	virtual void SetUpgradeLevel(S32 upgradeLevel);

	virtual void SetQueueNumber (U32 number);

	virtual U32 GetBuildArchetype (void);

	virtual void UpdateBuild(SINGLE percent, U32 stallType);

	virtual void ResetActiveButton();

	virtual void SetBuildModeOn (bool value){};

	virtual void DrawAt(U32 xpos,U32 ypos);

	virtual void GetShape(IDrawAgent ** shape);

	virtual HOTKEYS::HOTKEY GetHotkey();

	virtual bool IsActive();

	/* BaseHotRect methods */
	virtual void setStatus (void);
	
	/* HotButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE _pArchetype, HSOUND _hSound, PGENTYPE pFontType, U32 fontColor);

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
ResearchButton::~ResearchButton (void)
{
	GENDATA->Release(pArchetype);
	if(pFontType)
		GENDATA->Release(pFontType);
}
//--------------------------------------------------------------------------//
//
void ResearchButton::InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * _loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void ResearchButton::InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * parent, IShapeLoader * _loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void ResearchButton::InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * _loader)
{
	hintboxID = data.researchInfo;
	loader = _loader;
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	buttonText = data.tooltip;

	EnableButton(data.bDisabled==false);

	buildCost.gas = 0;
	researchID = 0;
	currentUpgrade = -1;
	hotkey = data.hotkey;
//	strcpy(researchArchetypeName,data.rtArchetype);
	if(data.rtArchetype[0])
	{
		BASE_RESEARCH_DATA * resData = (BASE_RESEARCH_DATA *) (ARCHLIST->GetArchetypeData(data.rtArchetype));
		if(resData)
		{
			if(resData->type == RESEARCH_TECH)
			{
				BT_RESEARCH * research = (BT_RESEARCH *) resData;
				if(research)
				{
					upgradeID = -2;
					nodeDepend = research->dependancy;
					nodeAchieved = research->researchTech;
					buildCost = research->cost;
					researchID = ARCHLIST->GetArchetypeDataID(data.rtArchetype);
				}
			}
			else if(resData->type == RESEARCH_ADMIRAL)
			{
				BT_ADMIRAL_RES * admiral = (BT_ADMIRAL_RES *) resData;
				upgradeID = -2;
				nodeDepend = admiral->dependancy;
				nodeAchieved = admiral->researchTech;
				buildCost = admiral->cost;
				researchID = ARCHLIST->GetArchetypeDataID(data.rtArchetype);
			}
			else
			{
				BT_UPGRADE * upgrade = (BT_UPGRADE *) resData;
				upgradeID = upgrade->extensionID;
				nodeDepend = upgrade->dependancy;
				nodeAchieved = SINGLE_TECHNODE();
//				nodeAchieved.raceID = M_NO_RACE;
				buildCost = upgrade->cost;
				researchID = ARCHLIST->GetArchetypeDataID(data.rtArchetype);
			}
		}
	}
	bReseachState = true;
	bDrawingPercent = false;
	queueNumber = 0;

	//
	// load the shapes
	//
	baseImage;
	if ((baseImage = data.baseImage) == 0)
		baseImage++;

	if(data.noMoneyImage)
		loader->CreateDrawAgent(data.noMoneyImage,noMoneyShape);
	
	int i;
	for (i = 0; i < GTHBSHP_MAX_SHAPES; i++)
		loader->CreateDrawAgent(i+baseImage, shapes[i]);

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

	if(pFont)
	{
		yText = S32(height - pFont->GetFontHeight()) / 2;
		xText = S32(width - pFont->GetStringWidth(L"10")) / 2;
	}
	//
	// calculate text offset
	//

	if (controlID == 0)
		controlID = buttonText;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void ResearchButton::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bPushState = false;
	}
}
//--------------------------------------------------------------------------//
//
bool ResearchButton::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetVisible (bool bVisible)
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
bool ResearchButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetTextID (U32 textID)
{
//	buttonText = static_cast<HBTNTXT::RESEARCH_TEXT>(textID);
//	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetTextString (const wchar_t * szString)
{
//	buttonText = HBTNTXT::NORESEARCHTEXT;
//	wcsncpy(szButtonText, szString, sizeof(szButtonText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool ResearchButton::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 ResearchButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetNumericValue (S32 value)
{
	numericValue = value;	
}
//--------------------------------------------------------------------------//
//
S32 ResearchButton::GetNumericValue (void)
{
	return numericValue;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetTechNode (TECHNODE node,TECHNODE allowed,TECHNODE workingNode)
{
//	if(!researchID)
//	{
//		if(researchArchetypeName[0])
//		{
//			PARCHETYPE rArch = ARCHLIST->GetArchetype(researchArchetypeName);
//	
//			if(rArch)
//			{
//				BT_RESEARCH * research = (BT_RESEARCH *) (ARCHLIST->GetArchetypeData(rArch));
//				nodeDepend = research->dependancy;
//				nodeAchieved = research->researchTech;
//				researchID = ARCHLIST->GetArchetypeDataID(rArch);
//			}
//		}
//	}
	
	currentNode = node;

	if(nodeAchieved.raceID != M_NO_RACE && (!(allowed.HasTech(nodeAchieved))))
	{
		bReseachState = false;
		SetVisible(false);
	}
	else if(nodeAchieved.raceID == 0)
	{
		if( (node.HasTech(nodeDepend)) && (currentUpgrade < upgradeID))
		{
			bReseachState = true;
			SetVisible(true);
		}
		else
		{
			bReseachState = false;
			SetVisible(false);
		}
	}
	else if(  (node.HasTech(nodeDepend))  &&
		(!(node.HasTech(nodeAchieved))) &&
		(!(workingNode.HasTech(nodeAchieved))))
	{
		bReseachState = true;
		SetVisible(true);
	}
	else
	{
		bReseachState = false;
		SetVisible(false);
	}
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetUpgradeLevel(S32 upgradeLevel)
{
	currentUpgrade = upgradeLevel;
	if(nodeAchieved.raceID == 0)
	{
		if(  (currentNode.HasTech(nodeDepend))  &&
			(currentUpgrade < upgradeID) )
		{
			bReseachState = true;
			SetVisible(true);
		}
		else
		{
			bReseachState = false;
			SetVisible(false);
		}
	}
}
//--------------------------------------------------------------------------//
//
void ResearchButton::SetQueueNumber (U32 number)
{
	queueNumber = number;
}
//--------------------------------------------------------------------------//
//
U32 ResearchButton::GetBuildArchetype (void)
{
	return researchID;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::UpdateBuild(SINGLE percent, U32 stallType)
{
	percentDone = percent;
	bDrawingPercent = true;
	stall = stallType;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::ResetActiveButton()
{
	bDrawingPercent = false;
	percentDone = 0;
	stall = NO_STALL;
}
//--------------------------------------------------------------------------//
//
GENRESULT ResearchButton::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	if (bInvisible==0)
	switch (message)
	{
	case CQE_ENDFRAME:
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		draw();
		break;

	case WM_MOUSEMOVE:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			desiredOwnedFlags = (RF_STATUS|RF_CURSOR|RF_HINTBOX);
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
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		if (bAlert)
		{
			doDblPushAction();
			return GR_GENERIC;
		}
		return GR_OK;

	case WM_LBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		onLeftButtonDown();
		if (bAlert)
			return GR_GENERIC;
		return GR_OK;

	case WM_RBUTTONDOWN:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
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
void ResearchButton::DrawAt(U32 xpos,U32 ypos)
{
	if (shapes[GTHBSHP_NORMAL])
		shapes[GTHBSHP_NORMAL]->Draw(0, xpos, ypos);
}
//--------------------------------------------------------------------------//
//
void ResearchButton::GetShape(IDrawAgent ** shape)
{
	shape[GTHBSHP_NORMAL] = shapes[GTHBSHP_NORMAL];
	shape[GTHBSHP_MOUSE_FOCUS] = shapes[GTHBSHP_MOUSE_FOCUS];
	shape[GTHBSHP_DISABLED] = shapes[GTHBSHP_DISABLED];
	shape[GTHBSHP_SELECTED] = shapes[GTHBSHP_SELECTED];
}
//--------------------------------------------------------------------------//
//
HOTKEYS::HOTKEY ResearchButton::GetHotkey()
{
	return hotkey;
}
//--------------------------------------------------------------------------//
//
bool ResearchButton::IsActive()
{
	return bReseachState;
}
//--------------------------------------------------------------------------//
//
void ResearchButton::draw (void)
{
	U32 state;
	
	if(!bReseachState)
		return;

	if(bDrawingPercent)
	{
		S32 width = (screenRect.right-screenRect.left);
		S32 midPoint = percentDone*width;
		
		PANE pane;
		pane.window = 0;
		pane.x0 = screenRect.left;
		pane.x1 = screenRect.left+midPoint;
		pane.y0 = screenRect.top;
		pane.y1 = screenRect.bottom;
		if(pane.x0 < pane.x1)
		{
			if (shapes[GTHBSHP_MOUSE_FOCUS])
				shapes[GTHBSHP_MOUSE_FOCUS]->Draw(&pane, 0, 0);
		}
		pane.x0 = screenRect.left+midPoint;
		pane.x1 = screenRect.right;
		if(pane.x0 < pane.x1)
		{
			if (shapes[GTHBSHP_DISABLED])
				shapes[GTHBSHP_DISABLED]->Draw(&pane, -midPoint, 0);
		}
		if(shapes[GTHBSHP_SELECTED]!=0)
			shapes[GTHBSHP_SELECTED]->Draw(0, screenRect.left, screenRect.top);
	}
	else
	{
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
	if(queueNumber && (pFont != 0))
	{
		PANE pane;

		pane.window = 0;
		pane.x0 = screenRect.left+xText;
		pane.x1 = screenRect.right;
		pane.y0 = screenRect.top+yText;
		pane.y1 = screenRect.bottom;

		wchar_t buffer[10];
		swprintf(buffer,L"%d",queueNumber);
		pFont->StringDraw(&pane, 0, 0,buffer);
	}
	if((stall == NO_MONEY) && (noMoneyShape != 0))
	{
		noMoneyShape->Draw(0, screenRect.left, screenRect.top);
	}
}
//--------------------------------------------------------------------------//
//
void ResearchButton::setStatus (void)
{
	if(!buttonText)
		return;

	wchar_t szButtonText[256];

	wchar_t buffer[256];
	wchar_t ext[256];
	char keytext[256];
	wchar_t dollar[256];
	wchar_t * ptr;
	int len;

	wcsncpy(szButtonText, _localLoadStringW(buttonText), sizeof(szButtonText)/sizeof(wchar_t));

	if (szButtonText[0])
	{
		ptr = szButtonText;
		len = wcslen(ptr);
		CQASSERT(len < 256);
		memcpy(buffer, ptr, (len+1)*sizeof(*ptr));
		
		if (hotkey != 0 && (ptr = wcschr(buffer, '!')) != 0)
		{
			int i;

			wcscpy(ext, ptr+1);		// copy everything after the mark
			len = HOTKEY->GetHotkeyText(hotkey, keytext, sizeof(keytext));
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
	if (statusTextID)
		STATUS->SetTextString(buffer, STM_DEFAULT);
	else
		STATUS->SetDefaultState();

	STATUS->SetRect(screenRect);
}
//--------------------------------------------------------------------------//
//
void ResearchButton::init (PGENTYPE _pArchetype, HSOUND _hSound, PGENTYPE _pFontType, U32 fontColor)
{
	pArchetype = _pArchetype;
	hSound = _hSound;
	pFontType = _pFontType;
	if(pFontType)
		GENDATA->AddRef(pFontType);

	if(pFontType)
	{
		COMPTR<IDAComponent> pBase;
		GENDATA->CreateInstance(pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
		if(pFont)
			pFont->SetFontColor(fontColor, 0);

	}

	cursorID = IDC_CURSOR_DEFAULT;
}
//--------------------------------------------------------------------------//
//-----------------------ResearchButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ResearchButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	HSOUND hSound;		// sound played when hotbutton is pressed
	PGENTYPE pFontType;
	U32 fontColor;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ResearchButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	ResearchButtonFactory (void) { }

	~ResearchButtonFactory (void);

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
ResearchButtonFactory::~ResearchButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void ResearchButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE ResearchButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTBUTTON)
	{
		GT_HOTBUTTON * data = (GT_HOTBUTTON *) _data;
	
		if (data->buttonType == HOTBUTTONTYPE::RESEARCH)
		{
			CQASSERT(pArchetype == 0);

			pArchetype = _pArchetype;
			hSound = SFXMANAGER->Open(SFXMANAGER->GetGlobalSounds().defaultButton);
			if (data->fontType[0])
			{
				pFontType = GENDATA->LoadArchetype(data->fontType);
				CQASSERT(pFontType);
				GENDATA->AddRef(pFontType);
			}

			fontColor = RGB(data->textColor.red, data->textColor.green, data->textColor.blue) | 0xFF000000;
			return (HANDLE) 1;
		}
	}

	return 0;
}
//-----------------------------------------------------------------------------------------//
//
BOOL32 ResearchButtonFactory::DestroyArchetype (HANDLE hArchetype)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);

	pArchetype = 0;
	SFXMANAGER->CloseHandle(hSound);
	hSound = 0;
	if(pFontType)
		GENDATA->Release(pFontType);	
	pFontType = 0;

	return 1;
}
//-----------------------------------------------------------------------------------------//
//
GENRESULT ResearchButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);
	ResearchButton * result = new DAComponent<ResearchButton>;

	result->init(pArchetype, hSound, pFontType,fontColor);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _researchbuttonfactory : GlobalComponent
{
	ResearchButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<ResearchButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _researchbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End ResearchButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
