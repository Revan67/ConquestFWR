//--------------------------------------------------------------------------//
//                                                                          //
//                             BuildButton.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/BuildButton.cpp 52    9/24/00 12:45p Tmauer $
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
#include "ObjList.h"
#include <DSpaceship.h>
#include <DPlatform.h>

#include <TComponent.h>
#include <FileSys.h>
#include <HKEvent.h>

#include <stdio.h>

namespace BBUTTONSTATE
{
	enum STATE
	{
		ACTIVE,
		GREY,
		INVISIBLE
	};
};

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE BuildButton : BaseHotRect, IHotButton, IActiveButton
{
	//
	// incoming interface map
	//
	BEGIN_DACOM_MAP_INBOUND(BuildButton)
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
	U32 stringID;
	U32 greyStringID;

	COMPTR<IDrawAgent> shapes[GTHBSHP_MAX_SHAPES];
	COMPTR<IDrawAgent> noMoneyShape;
	COMPTR<IFontDrawAgent> pFont;
	S32 yText, xText;
	U32 buildID;
	U32 stall;

	U32 controlID;					// set by owner
	ResourceCost buildCost;
	S32 numericValue;				// build cue
	SINGLE_TECHNODE nodeDepend, nodeGrey, activeTech;
	BBUTTONSTATE::STATE state;
	HOTKEYS::HOTKEY hotkey;

	bool bDisabled:1;
	bool bPushState:1;			// used for selection state

	bool bDrawingPercent:1;
	bool bModeOn:1;  //set when this is the vurrent build mode
	bool bTechActive:1;
	bool bButtonDown:1;
	bool bHighlight:1;

	U32 queueNumber;
	SINGLE percentDone;

	//
	// class methods
	//

	BuildButton (void) : BaseHotRect ( (IDAComponent *)NULL )
	{
	}

	virtual ~BuildButton (void);

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

	virtual void SetTechNode(TECHNODE node,TECHNODE allowed,TECHNODE working);

	virtual void SetUpgradeLevel(S32 level){};

	virtual void SetQueueNumber (U32 number);

	virtual U32 GetBuildArchetype (void);

	virtual void UpdateBuild(SINGLE percent, U32 stallType);

	virtual void ResetActiveButton();

	virtual void SetBuildModeOn(bool value);

	virtual void DrawAt(U32 xpos,U32 ypos);

	virtual void GetShape(IDrawAgent ** shape);

	virtual HOTKEYS::HOTKEY GetHotkey();

	virtual bool IsActive();

	/* BaseHotRect methods */
	virtual void setStatus (void);
	
	/* BuildButton methods */

	virtual GENRESULT __stdcall	Notify (U32 message, void *param);

	void init (PGENTYPE _pArchetype, HSOUND _hSound, PGENTYPE _pFontType, U32 fontColor);

	void draw (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IEventCallback *> (this);
	}

	void onLeftButtonDown (void)
	{
		if ((bDisabled == false) && (state == BBUTTONSTATE::ACTIVE))
		{
			if (bAlert)
				bButtonDown = true;
		}
	}

	void onLeftDblClick (void)
	{
		if ((bDisabled == false) && (state == BBUTTONSTATE::ACTIVE))
		{
			doDblPushAction();
		}
	}

	void onLeftButtonUp (void)
	{
		if ((bDisabled == false) && (state == BBUTTONSTATE::ACTIVE))
		{
			if (bAlert && bButtonDown)
				doLPushAction();
		}
		bButtonDown = false;
	}

	void onRightButtonDown (void)
	{
		if ((bDisabled == false) && (state == BBUTTONSTATE::ACTIVE))
		{
			if (bAlert)
				doRPushAction();
		}
	}

	void doLPushAction (void)
	{
		parent->PostMessage(CQE_LHOTBUTTON, (void*)controlID);
		SFXMANAGER->Play(hSound);
	}

	void doRPushAction (void)
	{
		parent->PostMessage(CQE_RHOTBUTTON, (void*)controlID);
	}

	void doDblPushAction (void)
	{
//		parent->PostMessage(CQE_LHOTBUTTON, (void*)controlID);
//		SFXMANAGER->Play(hSound);
	}
};
//--------------------------------------------------------------------------//
//
BuildButton::~BuildButton (void)
{
	GENDATA->Release(pArchetype);
	if(pFontType)
		GENDATA->Release(pFontType);
}
//--------------------------------------------------------------------------//
//
void BuildButton::InitHotButton (const HOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void BuildButton::InitBuildButton (const BUILDBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	if (parent == 0)
	{
		parent = _parent;
		lateInitialize(_parent->GetBase());
		resPriority = _parent->resPriority+1;
	}

	hintboxID = data.buildInfo;
	bModeOn = false;
	nodeDepend = data.techDependency;
	nodeGrey = data.techGreyed;
	buildID = 0;
	hotkey = data.hotkey;
	if(data.rtArchetype[0])
	{
		buildID = ARCHLIST->GetArchetypeDataID(data.rtArchetype);
		BASIC_DATA * workData = (BASIC_DATA *)(ARCHLIST->GetArchetypeData(buildID));
		activeTech.build = (TECHTREE::BUILDNODE)0;
		activeTech.tech = (TECHTREE::TECHUPGRADE)0;
		activeTech.raceID = (M_RACE)M_TERRAN;
		activeTech.common = (TECHTREE::COMMON)0;
		activeTech.cq2Vars1 = (TECHTREE::CQ2_VARS_1)0;
		activeTech.cq2Vars2 = (TECHTREE::CQ2_VARS_2)0;

		if(workData)
		{
			if(workData->objClass == OC_PLATFORM)
			{
				BASE_PLATFORM_DATA * platData = (BASE_PLATFORM_DATA *) workData;
				activeTech = platData->techActive;
			}
			else if(workData->objClass == OC_SPACESHIP)
			{
				BASE_SPACESHIP_DATA * shipData = (BASE_SPACESHIP_DATA * ) workData;
				activeTech = shipData->techActive;
			}
		}
	}
	state = BBUTTONSTATE::ACTIVE;
	queueNumber = 0;

	greyStringID = data.greyedTooltip;

	EnableButton(data.bDisabled==false);
	//
	// get the archetype data (display name, build cost)
	//
	BASIC_DATA * basic = (BASIC_DATA *) ARCHLIST->GetArchetypeData(data.rtArchetype);
	buildCost.crew = 0;

	if (basic)
	{
		switch (basic->objClass)
		{
		case OC_SPACESHIP:
			{
				BASE_SPACESHIP_DATA * data = (BASE_SPACESHIP_DATA *) basic;
				buildCost = data->missionData.resourceCost;
				stringID = data->missionData.displayName;
				if (controlID == 0)
					controlID = data->missionData.displayName;
			}
			break;

		case OC_PLATFORM:
			{
				BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *) basic;
				buildCost = data->missionData.resourceCost;
				stringID = data->missionData.displayName;
				if (controlID == 0)
					controlID = data->missionData.displayName;
			}
			break;
		}

	}

	//
	// load the shapes
	//
	int base;
	if ((base = data.baseImage) == 0)
		base++;

	int i;
	for (i = 0; i < GTHBSHP_MAX_SHAPES; i++)
		loader->CreateDrawAgent(i+base, shapes[i]);

	if(data.noMoneyImage)
		loader->CreateDrawAgent(data.noMoneyImage,noMoneyShape);


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
}
//--------------------------------------------------------------------------//
//
void BuildButton::InitResearchButton (const RESEARCHBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void BuildButton::InitMultiHotButton (const MULTIHOTBUTTON_DATA & data, BaseHotRect * _parent, IShapeLoader * loader)
{
	CQBOMB0("Wrong init called.");
}
//--------------------------------------------------------------------------//
//
void BuildButton::EnableButton (bool bEnable)
{
	if (bEnable == bDisabled)	// if we are changing state
	{
		bDisabled = !bEnable;
		bPushState = false;
	}
}
//--------------------------------------------------------------------------//
//
bool BuildButton::GetEnableState (void)
{
	return !bDisabled;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetVisible (bool bVisible)
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
bool BuildButton::GetVisible (void)
{
	return !bInvisible;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetTextID (U32 textID)
{
//	wcsncpy(szToolText, _localLoadStringW(textID), sizeof(szToolText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetTextString (const wchar_t * szString)
{
//	wcsncpy(szToolText, szString, sizeof(szToolText)/sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetPushState (bool bPressed)
{
	bPushState = bPressed;
}
//--------------------------------------------------------------------------//
//
bool BuildButton::GetPushState (void)
{
	return bPushState;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetControlID (U32 id)
{
	controlID = id;
}
//--------------------------------------------------------------------------//
//
U32 BuildButton::GetControlID (void)
{
	return controlID;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetNumericValue (S32 value)
{
	numericValue = value;	
}
//--------------------------------------------------------------------------//
//
S32 BuildButton::GetNumericValue (void)
{
	return numericValue;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetTechNode (TECHNODE node, TECHNODE allowed, TECHNODE working)
{
	if((activeTech.raceID != M_NO_RACE) && (!(allowed.HasTech(activeTech))))
	{
		state = BBUTTONSTATE::INVISIBLE;
		SetVisible(false);
	}
	else if(node.HasTech(nodeDepend))
	{
		state = BBUTTONSTATE::ACTIVE;
		SetVisible(true);
	}
	else if(node.HasTech(nodeGrey))
	{
		state = BBUTTONSTATE::GREY;
		SetVisible(true);
	}
	else
	{
		state = BBUTTONSTATE::GREY; //set to invisible to change back to the old way
		SetVisible(true);
	}
	if(activeTech.raceID != M_NO_RACE && node.HasTech(activeTech))
		bTechActive = true;
	else
		bTechActive = false;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetQueueNumber (U32 number)
{
	queueNumber = number;
}
//--------------------------------------------------------------------------//
//
U32 BuildButton::GetBuildArchetype (void)
{
	return buildID;
}
//--------------------------------------------------------------------------//
//
void BuildButton::UpdateBuild(SINGLE percent, U32 stallType)
{
	percentDone = percent;
	bDrawingPercent = true;
	stall = stallType;
}
//--------------------------------------------------------------------------//
//
void BuildButton::ResetActiveButton()
{
	bDrawingPercent = false;
	percentDone = 0;
	bModeOn = false;
	stall = NO_STALL;
}
//--------------------------------------------------------------------------//
//
void BuildButton::SetBuildModeOn(bool value)
{
	bModeOn = value;
}
//--------------------------------------------------------------------------//
//
GENRESULT BuildButton::Notify (U32 message, void *param)
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
			onLeftDblClick();
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

	case WM_LBUTTONUP:
		BaseHotRect::Notify(message, param);		// switch focus, if needed
		if (CQFLAGS.bGamePaused || DEFAULTS->GetDefaults()->bEditorMode)	// do not allow buttons to work in this mode
			bAlert = 0;
		onLeftButtonUp();
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
		break;
	}

	return BaseHotRect::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void BuildButton::DrawAt(U32 xpos,U32 ypos)
{
	if (shapes[GTHBSHP_NORMAL])
		shapes[GTHBSHP_NORMAL]->Draw(0, xpos, ypos);
}
//--------------------------------------------------------------------------//
//
void BuildButton::GetShape(IDrawAgent ** shape)
{
	shape[GTHBSHP_NORMAL] = shapes[GTHBSHP_NORMAL];
	shape[GTHBSHP_MOUSE_FOCUS] = shapes[GTHBSHP_MOUSE_FOCUS];
	shape[GTHBSHP_DISABLED] = shapes[GTHBSHP_DISABLED];
	shape[GTHBSHP_SELECTED] = shapes[GTHBSHP_SELECTED];
}
//--------------------------------------------------------------------------//
//
HOTKEYS::HOTKEY BuildButton::GetHotkey()
{
	return hotkey;
}
//--------------------------------------------------------------------------//
//
bool BuildButton::IsActive()
{
	return (state == BBUTTONSTATE::ACTIVE);
}
//--------------------------------------------------------------------------//
//
void BuildButton::draw (void)
{
	U32 drawState;
	
	if(state == BBUTTONSTATE::INVISIBLE)
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
			drawState = GTHBSHP_DISABLED;
		}
		else
		{
			if(state == BBUTTONSTATE::ACTIVE)
			{
				drawState = GTHBSHP_NORMAL;
				if (bAlert)
					drawState = GTHBSHP_MOUSE_FOCUS;
			}else 
				drawState = GTHBSHP_DISABLED;
		}

		if (shapes[drawState])
			shapes[drawState]->Draw(0, screenRect.left, screenRect.top);
		if ((bPushState || bModeOn || bHighlight) && shapes[GTHBSHP_SELECTED]!=0)
			shapes[GTHBSHP_SELECTED]->Draw(0, screenRect.left, screenRect.top);
	}

	// draw a 3D border around the build button
/*	if (bButtonDown == false)
	{
		DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.right, screenRect.top, RGB(200,240,180));
		DA::LineDraw(0, screenRect.left, screenRect.top, screenRect.left, screenRect.bottom, RGB(200,240,180));
	}
	else
	{
		DA::LineDraw(0, screenRect.left, screenRect.bottom, screenRect.right, screenRect.bottom, RGB(200,240,180));
		DA::LineDraw(0, screenRect.right, screenRect.bottom, screenRect.right, screenRect.top, RGB(200,240,180));
	}
*/
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

	if(bTechActive)
	{
		U32 num = screenRect.right-screenRect.left;
		num = num/3;
		U32 num2 = num/3;
		DA::RectangleFill(0, screenRect.left+num2, screenRect.top+num, screenRect.left+num2, screenRect.bottom-num, RGB(64,200,64));
		DA::RectangleFill(0, screenRect.left+num, screenRect.top+num2, screenRect.right-num, screenRect.top+num2, RGB(64,200,64));
		DA::RectangleFill(0, screenRect.right-num2, screenRect.top+num, screenRect.right-num2, screenRect.bottom-num, RGB(64,200,64));
		DA::RectangleFill(0, screenRect.left+num, screenRect.bottom-num2, screenRect.right-num, screenRect.bottom-num2, RGB(64,200,64));
	}

	if((stall == NO_MONEY) && noMoneyShape != 0)
	{
		noMoneyShape->Draw(0, screenRect.left, screenRect.top);
	}
}
//--------------------------------------------------------------------------//
//
void BuildButton::setStatus (void)
{
	wchar_t szToolText[256];
	
	wchar_t buffer[256];
	wchar_t ext[256];
	wchar_t dollar[256];
	char keytext[256];
	wchar_t * ptr;
	int len;

	if(state == BBUTTONSTATE::GREY)
	{
		wcsncpy(szToolText, _localLoadStringW(greyStringID), sizeof(szToolText)/sizeof(wchar_t));

		STATUS->SetTextString(szToolText, STM_BUILD_DENIED);
	}
	else
	{
		wcsncpy(szToolText, _localLoadStringW(stringID), sizeof(szToolText)/sizeof(wchar_t));
		if (szToolText[0])
		{
			ptr = szToolText;
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
			STATUS->SetDefaultState();
	}
	STATUS->SetRect(screenRect);
}
//--------------------------------------------------------------------------//
//
void BuildButton::init (PGENTYPE _pArchetype, HSOUND _hSound, PGENTYPE _pFontType, U32 fontColor)
{
	pArchetype = _pArchetype;
	hSound = _hSound;
	pFontType = _pFontType;
	if(pFontType)
		GENDATA->AddRef(pFontType);

	cursorID = IDC_CURSOR_DEFAULT;
	
	if(pFontType)
	{
		COMPTR<IDAComponent> pBase;
		GENDATA->CreateInstance(pFontType, pBase);
		CQASSERT(pBase!=0);
		pBase->QueryInterface("IFontDrawAgent", pFont);
		if(pFont)
			pFont->SetFontColor(fontColor, 0);
	}
}
//--------------------------------------------------------------------------//
//-----------------------BuildButton Factory class-------------------------------//
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE BuildButtonFactory : public ICQFactory
{
	//----------------------------------------------------------------------------
	//
	U32 factoryHandle;		// handles to callback

	PGENTYPE pArchetype;
	HSOUND hSound;		// sound played when BuildButton is pressed
	PGENTYPE pFontType;
	U32 fontColor;

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(BuildButtonFactory)
	DACOM_INTERFACE_ENTRY(ICQFactory)
	END_DACOM_MAP()

	BuildButtonFactory (void) { }

	~BuildButtonFactory (void);

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
BuildButtonFactory::~BuildButtonFactory (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA && GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Unadvise(factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
void BuildButtonFactory::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GENDATA->QueryOutgoingInterface("ICQFactory", connection) == GR_OK)
		connection->Advise(this, &factoryHandle);
}
//-----------------------------------------------------------------------------------------//
//
HANDLE BuildButtonFactory::CreateArchetype (PGENTYPE _pArchetype, GENBASE_TYPE objClass, void *_data)
{
	if (objClass == GBT_HOTBUTTON)
	{
		GT_HOTBUTTON * data = (GT_HOTBUTTON *) _data;
		if (data->buttonType == HOTBUTTONTYPE::BUILD)
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
BOOL32 BuildButtonFactory::DestroyArchetype (HANDLE hArchetype)
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
GENRESULT BuildButtonFactory::CreateInstance (HANDLE hArchetype, IDAComponent ** pInstance)
{
	CQASSERT((U32)hArchetype == 1 && pArchetype != 0);
	BuildButton * result = new DAComponent<BuildButton>;

	result->init(pArchetype, hSound, pFontType, fontColor);
	*pInstance = result->getBase();
	return GR_OK;
}
//-----------------------------------------------------------------------------------------//
//
struct _buildbuttonfactory : GlobalComponent
{
	BuildButtonFactory * factory;

	virtual void Startup (void)
	{
		factory = new DAComponent<BuildButtonFactory>;
		AddToGlobalCleanupList(&factory);
	}

	virtual void Initialize (void)
	{
		factory->init();
	}
};

static _buildbuttonfactory startup;

//-----------------------------------------------------------------------------------------//
//----------------------------------End BuildButton.cpp--------------------------------------//
//-----------------------------------------------------------------------------------------//
