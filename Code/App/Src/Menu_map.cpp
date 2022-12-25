//--------------------------------------------------------------------------//
//                                                                          //
//                               Menu_map.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_map.cpp 110   10/18/02 2:36p Tmauer $
*/
//--------------------------------------------------------------------------//
// Multiplayer load map
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <DMenu1.h>

#include "Frame.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IButton2.h"
#include "ISlider.h"
#include "CQGame.h"
#include "Mission.h"
#include "MapGen.h"
#include "NetBuffer.h"

#include <wchar.h>
#include <dplay.h>

#define MT_RANDOM 11001
#define MT_MULTI  11002
#define MT_USER   11003

#define SLIDER_GAME_SPEED 16000
#define SLIDER_COMMAND_POINTS 16001

#define PUSH_SPECTATOR		87000
#define PUSH_DIPLOMACY		87001
#define PUSH_REGENERATION	87002
#define PUSH_EASY			87003
#define PUSH_AVERAGE		87004
#define PUSH_HARD			87005

#define PUSH_LOCKSETTINGS	88000

void  __stdcall DoMenu_slots (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame);
U32 __stdcall DoMenu_MapSelect (BaseHotRect * parent, ICQGame & cqgame, wchar_t * szFileName, const int numChars);

using namespace CQGAMETYPES;
//--------------------------------------------------------------------------//
//
struct Menu_map : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::MAP & data;
	const GT_MENU1 & menu1;
	ICQGame & cqgame;
	bool bInitialized;

	COMPTR<IDropdown> dropGameType, dropMoney;
	COMPTR<IDropdown> dropSize, dropTerrain, dropUnits;
	COMPTR<IDropdown> dropVisibility;
	COMPTR<IDropdown> bandWidthOption;
	COMPTR<IButton2>  buttonMapType;

	COMPTR<IStatic> staticGameType, staticSpeed, staticMoney, staticUnits;
	COMPTR<IStatic> staticMapType, staticSize, staticTerrain;
	COMPTR<IStatic> staticVisibility, staticBandwidth;
	COMPTR<ISlider> sliderSpeed,sliderCmdPoints;

	COMPTR<IStatic> staticSpectator, staticDiplomacy;
	COMPTR<IButton2> pushSpectator, pushDiplomacy;

	COMPTR<IStatic> staticLockSettings;
	COMPTR<IButton2> pushLockSettings;

	COMPTR<IStatic>   staticSystems;
	COMPTR<IDropdown> dropSystems;

	COMPTR<IButton2> pushEasy, pushAverage, pushHard;

	COMPTR<IStatic> staticDifficulty, staticEasy, staticAverage, staticHard;
	COMPTR<IStatic> staticCmdPoints,staticCmdPointsDisplay;

	Frame * mshellFrame;
	Frame * slotsFrame;

	U32 oldNumberOfPlayers;

	U32 maxPlayers;

	bool bInternet;

	//
	// instance methods
	//

	Menu_map (Frame * _parent, const GT_MENU1 & _data, ICQGame & _cqgame, bool _bInternet) 
		: data(_data.map), menu1(_data), cqgame(_cqgame)
	{
		bInternet = _bInternet;
		initializeFrame(_parent);
		init();
	}

	~Menu_map (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	virtual void onSliderPressed (U32 sliderID);

	virtual void onButtonPressed (U32 buttonID);

	virtual void onSetDefaultFocus (bool bFromPrevious)
	{
		// if we are not the host, than move the message along 
		if (HOSTID != PLAYERID)
		{
			if (bFromPrevious)
			{
				if (mshellFrame)
				{
					mshellFrame->onSetDefaultFocus(true);
				}
			}
			else
			{
				if (slotsFrame)
				{
					slotsFrame->onSetDefaultFocus(false);
				}
			}
			return;
		}

		if (focusControl == NULL)
		{
			// if lock settings is turned on then go to that control
			if (pushLockSettings->GetPushState())
			{
				setFocus(pushLockSettings);
			}
			else 
			{
				setFocus(pushSpectator);				
			}
			return;
		}

		Frame::onSetDefaultFocus(bFromPrevious);
	}

	virtual bool onTabPressed (void)
	{
		if (focusControl == NULL)
		{
			return false;
		}
		
		if (onGroupTabPressed())
		{
			// let the frame do its work
			return Frame::onTabPressed();
		}
		else
		{
			// switch focus to the next control group (menu_final)
			if (mshellFrame)
			{
				mshellFrame->onSetDefaultFocus(true);
				return true;
			}
		}

		return Frame::onTabPressed();
	}

	virtual bool onShiftTabPressed (void)
	{
		if (focusControl == NULL)
		{
			return false;
		}

		if (onGroupShiftTabPressed())
		{
			// let the frame do its work
			return Frame::onShiftTabPressed();
		}
		else
		{
			if (slotsFrame)
			{
				slotsFrame->setLastFocus();
				return true;
			}
		}

		return Frame::onShiftTabPressed();
	}

	/* Menu_map methods */

	virtual void setStateInfo (void);

	virtual void onListSelection (S32 listID);		// user has selected a list item

	void init (void);

	void initDropdowns (void);

	void fillMapDescription (void);

	void setMaxPlayers (void);

	virtual GENRESULT __stdcall Notify (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_KILL_FOCUS:
			if (childFrame)
			{
				childFrame->Notify(CQE_KILL_FOCUS, 0);
			}
			break;

		case CQE_SET_FOCUS:
			if (childFrame)
			{
				childFrame->Notify(CQE_SET_FOCUS, 0);
			}
			break;

		case CQE_GET_MAX_PLAYERS:
			if(!maxPlayers)
				*((U32 *)param) = MAX_PLAYERS;
			else
				*((U32 *)param) = maxPlayers;
			return GR_OK;

		default:
			break;
		} 

		return Frame::Notify(message, param);
	}

	void doPushButton (IButton2 *pButton)
	{
		pButton->SetPushState(!pButton->GetPushState());
	}

	CQGAMETYPES::RANDOM_TEMPLATE randomTypeFromStringID (U32 stringID)
	{
		CQGAMETYPES::RANDOM_TEMPLATE value = CQGAMETYPES::TEMPLATE_RANDOM;;

		if (stringID == IDS_MAPTYPE_RANDOM)
		{
			value = CQGAMETYPES::TEMPLATE_RANDOM;
		}
		else if (stringID == IDS_MAPTYPE_RING)
		{
			value = CQGAMETYPES::TEMPLATE_RING;
		}
		else if (stringID == IDS_MAPTYPE_STAR)
		{
			value = CQGAMETYPES::TEMPLATE_STAR;
		}
		else if (stringID == IDS_MAPTYPE_NEW_RANDOM)
		{
			value = CQGAMETYPES::TEMPLATE_NEW_RANDOM;
		}
		return value;
	}

	void setDifficultyButtonState (void);

	U32 getNumberPlayers (void);
};

//----------------------------------------------------------------------------------//
//
Menu_map::~Menu_map (void)
{
}
//----------------------------------------------------------------------------------//
//
void Menu_map::onButtonPressed (U32 buttonID)
{
	if (buttonID == IDS_BUTTON_MAPTYPE)
	{
		CQGAMETYPES::MAPTYPE type;
		wchar_t szFileName[MAX_PATH];

		// initialize our values
		type = cqgame.mapType;
		wcsncpy(szFileName, cqgame.szMapName, sizeof(szFileName)/sizeof(wchar_t));

		// the user wants to pick a new map to be displayed...
		U32 result = DoMenu_MapSelect(this, cqgame, szFileName, sizeof(szFileName)/sizeof(wchar_t));
		type = cqgame.mapType;

		if (result)
		{
			if (type == CQGAMETYPES::RANDOM_MAP)
			{
				cqgame.SetMapType(CQGAMETYPES::RANDOM_MAP);
				cqgame.SetMapTemplateType(randomTypeFromStringID(result));
			}
			else if (type == CQGAMETYPES::SELECTED_MAP)
			{
				cqgame.SetMapType(CQGAMETYPES::SELECTED_MAP);

				// which map are we using?
				cqgame.SetMapName(szFileName);
			}
			else if (type == CQGAMETYPES::USER_MAP)
			{
				cqgame.SetMapType(CQGAMETYPES::USER_MAP);

				// which map are we using?
				cqgame.SetMapName(szFileName);
			}
		}

		fillMapDescription();
		setMaxPlayers();
		cqgame.ForceUpdate();
	}
	else if (buttonID == PUSH_SPECTATOR)
	{
		doPushButton(pushSpectator);
		cqgame.SetSpectatorState(pushSpectator->GetPushState());
	}
	else if (buttonID == PUSH_DIPLOMACY)
	{
		doPushButton(pushDiplomacy);
		cqgame.SetLockDiplomacy(pushDiplomacy->GetPushState());
	}
	else if (buttonID == PUSH_LOCKSETTINGS)
	{
		doPushButton(pushLockSettings);
		cqgame.SetHostBusy(!pushLockSettings->GetPushState());
	}
	else if (buttonID == PUSH_EASY)
	{
		cqgame.SetDifficulty(EASY);
	}
	else if (buttonID == PUSH_AVERAGE)
	{
		cqgame.SetDifficulty(AVERAGE);
	}
	else if (buttonID == PUSH_HARD)
	{
		cqgame.SetDifficulty(HARD);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_map::onSliderPressed (U32 sliderID)
{
	if (sliderID == SLIDER_GAME_SPEED)
	{
		// the game speed is a number between -10 and 10
		cqgame.SetGameSpeed(sliderSpeed->GetPosition());
	}
	else if (sliderID == SLIDER_COMMAND_POINTS)
	{
		cqgame.SetCommandLimit((CQGAMETYPES::COMMANDLIMIT)(sliderCmdPoints->GetPosition()));
		S32 nCmdPts;

		switch (cqgame.commandLimit)
		{
#ifdef _DEMO
		case COMMAND_LOW:
			nCmdPts = 50;
			break;

		case COMMAND_MID:
			nCmdPts = 150;
			break;

		case COMMAND_HIGH:
			nCmdPts = 200;
			break;

		default:
			nCmdPts = 100;
			break;
#else
		case COMMAND_LOW:
			nCmdPts = 100;
			break;

		case COMMAND_MID:
			nCmdPts = 200;
			break;

		case COMMAND_HIGH:
			nCmdPts = 300;
			break;

		default:
			nCmdPts = 150;
			break;

#endif
		}

		wchar_t szCmdPts[256];
		swprintf(szCmdPts, L"%d", nCmdPts);
		staticCmdPointsDisplay->SetText(szCmdPts);
		staticCmdPointsDisplay->SetVisible(true);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_map::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//

	staticSpectator->InitStatic(data.staticSpectator, this);
	staticDiplomacy->InitStatic(data.staticDiplomacy, this);
	pushSpectator->InitButton(data.pushSpectator, this);
	pushDiplomacy->InitButton(data.pushDiplomacy, this);
	pushEasy->InitButton(data.pushEasy, this);
	pushAverage->InitButton(data.pushAverage, this);
	pushHard->InitButton(data.pushHard, this);

	buttonMapType->InitButton(data.mapType, this);
	buttonMapType->SetTransparent(true);

	staticGameType->InitStatic(data.staticGameType, this);
	staticSpeed->InitStatic(data.staticSpeed, this);
	staticMoney->InitStatic(data.staticMoney, this);
	staticUnits->InitStatic(data.staticUnits, this);
	staticMapType->InitStatic(data.staticMapType, this);
	staticSize->InitStatic(data.staticSize, this);
	staticTerrain->InitStatic(data.staticTerrain, this);
	staticVisibility->InitStatic(data.staticVisibility, this);
	staticBandwidth->InitStatic(data.staticBandwidth, this);
	staticLockSettings->InitStatic(data.staticLockSettings, this);
	staticSystems->InitStatic(data.staticSystems, this);

	staticDifficulty->InitStatic(data.staticDifficulty, this);
	staticEasy->InitStatic(data.staticEasy, this);
	staticAverage->InitStatic(data.staticAverage, this);
	staticHard->InitStatic(data.staticHard, this);

	staticCmdPoints->InitStatic(data.staticCmdPoints, this);
	staticCmdPointsDisplay->InitStatic(data.staticCmdPointsDisplay, this);

	sliderSpeed->InitSlider(data.sliderSpeed, this);
	sliderCmdPoints->InitSlider(data.sliderCmdPoints,this);
	dropMoney->InitDropdown(data.dropMoney, this);
	dropGameType->InitDropdown(data.dropGameType, this);
	dropSystems->InitDropdown(data.dropSystems, this);
	dropSize->InitDropdown(data.dropSize, this);
	dropTerrain->InitDropdown(data.dropTerrain, this);
	dropVisibility->InitDropdown(data.dropVisibility, this);
	bandWidthOption->InitDropdown(data.bandWidthOption,this);
	dropUnits->InitDropdown(data.dropUnits, this);
	pushLockSettings->InitButton(data.pushLockSettings, this);

	pushSpectator->SetControlID(PUSH_SPECTATOR);
	pushDiplomacy->SetControlID(PUSH_DIPLOMACY);
	pushLockSettings->SetControlID(PUSH_LOCKSETTINGS);
	pushEasy->SetControlID(PUSH_EASY);
	pushAverage->SetControlID(PUSH_AVERAGE);
	pushHard->SetControlID(PUSH_HARD);

	// set up the buddy controls...
	staticSpectator->SetBuddyControl(pushSpectator);
	staticDiplomacy->SetBuddyControl(pushDiplomacy);
	staticLockSettings->SetBuddyControl(pushLockSettings);

	staticEasy->SetBuddyControl(pushEasy);
	staticAverage->SetBuddyControl(pushAverage);
	staticHard->SetBuddyControl(pushHard);

	sliderSpeed->EnableClickBehavior(true);
	sliderCmdPoints->EnableClickBehavior(true);

	if (bInitialized == false)
	{
		initDropdowns();

		sliderSpeed->SetControlID(SLIDER_GAME_SPEED);
		sliderSpeed->SetRangeMin(-5);
		sliderSpeed->SetRangeMax(5);

		sliderCmdPoints->SetControlID(SLIDER_COMMAND_POINTS);
		sliderCmdPoints->SetRangeMin(-2);
		sliderCmdPoints->SetRangeMax(1);
	}
		
	//
	// set state of dropdowns
	//
	if (dropGameType->GetNumberOfItems() > 1)
	{
		if (cqgame.gameType == KILL_PLATS_FABS)
		{
			dropGameType->SetCurrentSelection(2);
		}
		else
		{
			dropGameType->SetCurrentSelection(cqgame.gameType-KILL_UNITS);
		}
	}
	else
	{
		dropGameType->SetCurrentSelection(0);
	}

	bool bEnableControls = (HOSTID == PLAYERID && cqgame.bHostBusy);
	bool bEnableDemoControls = (HOSTID == PLAYERID && cqgame.bHostBusy);

#ifdef _DEMO_
		bEnableControls = false;
#endif

	dropMoney->SetCurrentSelection(cqgame.money-LOW_MONEY);
	dropSize->SetCurrentSelection(cqgame.mapSize-SMALL_MAP);
	dropTerrain->SetCurrentSelection(cqgame.terrain-LIGHT_TERRAIN);
	dropVisibility->SetCurrentSelection(cqgame.visibility - VISIBILITY_NORMAL);
	bandWidthOption->SetCurrentSelection(DEFAULTS->GetDefaults()->bNetworkBandwidth);
	dropUnits->SetCurrentSelection(cqgame.units-UNITS_MINIMAL);
	
	sliderSpeed->SetPosition(cqgame.gameSpeed);
	sliderCmdPoints->SetPosition(cqgame.commandLimit);

	if (cqgame.mapType == CQGAMETYPES::RANDOM_MAP && CQFLAGS.bLimitMapSettings)
		buttonMapType->EnableButton(false);
	else
		buttonMapType->EnableButton(bEnableControls);

	dropMoney->EnableDropdown(bEnableControls);
	dropSize->EnableDropdown(bEnableControls);
	dropTerrain->EnableDropdown(bEnableControls);
	dropVisibility->EnableDropdown(bEnableControls);
	bandWidthOption->EnableDropdown(true);
	bandWidthOption->SetVisible(true);
	staticBandwidth->SetVisible(bInternet);
	dropUnits->EnableDropdown(bEnableControls);
	dropGameType->EnableDropdown(bEnableControls);
	dropSystems->EnableDropdown(bEnableDemoControls);

	sliderSpeed->EnableSlider(bEnableDemoControls);
	sliderCmdPoints->EnableSlider(bEnableDemoControls);
	pushSpectator->EnableButton(bEnableDemoControls);
	pushDiplomacy->EnableButton(bEnableDemoControls);

	if (PLAYERID == 0)
	{
		// spectator and diplomacy buttons/text should not be available in quickbattle
		pushSpectator->SetVisible(false);
		pushDiplomacy->SetVisible(false);
		staticSpectator->SetVisible(false);
		staticDiplomacy->SetVisible(false);
	}

	// still need the lock settings in demo build because of the speed slider
	pushLockSettings->EnableButton(HOSTID==PLAYERID);
	pushLockSettings->SetVisible(HOSTID == PLAYERID && PLAYERID);
	staticLockSettings->SetVisible(HOSTID == PLAYERID && PLAYERID);

//#ifdef _DEMO_
	bool bShowDifficulty = false;
//#else
//	bool bShowDifficulty = (PLAYERID==0);
//#endif
	pushEasy->SetVisible(bShowDifficulty);
	pushAverage->SetVisible(bShowDifficulty);
	pushHard->SetVisible(bShowDifficulty);
	staticDifficulty->SetVisible(bShowDifficulty);
	staticEasy->SetVisible(bShowDifficulty);
	staticAverage->SetVisible(bShowDifficulty);
	staticHard->SetVisible(bShowDifficulty);
	
	fillMapDescription();

	// find out how many command points we're using in this game...
	// 0 = normal (100), -1 = low (50), 1 = high (200)
	S32 nCmdPts;

	switch (cqgame.commandLimit)
	{
#ifdef _DEMO
	case COMMAND_LOW:
		nCmdPts = 50;
		break;

	case COMMAND_MID:
		nCmdPts = 150;
		break;

	case COMMAND_HIGH:
		nCmdPts = 200;
		break;

	default:
		nCmdPts = 100;
		break;
#else
	case COMMAND_LOW:
		nCmdPts = 100;
		break;

	case COMMAND_MID:
		nCmdPts = 200;
		break;

	case COMMAND_HIGH:
		nCmdPts = 300;
		break;

	default:
		nCmdPts = 150;
		break;

#endif
	}

	wchar_t szCmdPts[256];
	swprintf(szCmdPts, L"%s", _localLoadStringW(IDS_STATIC_CMDPOINTS));
	staticCmdPoints->SetText(szCmdPts);
	staticCmdPoints->SetVisible(true);

	swprintf(szCmdPts, L"%d", nCmdPts);
	staticCmdPointsDisplay->SetText(szCmdPts);
	staticCmdPointsDisplay->SetVisible(true);

	if (childFrame == 0)
	{
		DoMenu_slots(this, menu1, cqgame);
	}
	else if (childFrame)
	{
		childFrame->setStateInfo();
	}

	// get a pointer to the mshell frame, this is your parents (menu_final) parent
	// get a pointer to menu_slots, this is your child frame
	mshellFrame = parentFrame->parentFrame;
	slotsFrame = childFrame;

	// set the group behavior so we do the right tab thing
	setGroupBehavior();

	setMaxPlayers();
}
//--------------------------------------------------------------------------//
//
void Menu_map::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.staticSpectator.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSpectator);
	
	GENDATA->CreateInstance(data.staticDiplomacy.staticType, pComp);
	pComp->QueryInterface("IStatic", staticDiplomacy);

	GENDATA->CreateInstance(data.pushSpectator.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushSpectator);
	
	GENDATA->CreateInstance(data.pushDiplomacy.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushDiplomacy);

	GENDATA->CreateInstance(data.mapType.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonMapType);

	GENDATA->CreateInstance(data.staticGameType.staticType, pComp);
	pComp->QueryInterface("IStatic", staticGameType);
	
	GENDATA->CreateInstance(data.staticSpeed.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSpeed);
	
	GENDATA->CreateInstance(data.staticMoney.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMoney);
	
	GENDATA->CreateInstance(data.staticUnits.staticType, pComp);
	pComp->QueryInterface("IStatic", staticUnits);
	
	GENDATA->CreateInstance(data.staticMapType.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMapType);
	
	GENDATA->CreateInstance(data.staticSize.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSize);

	GENDATA->CreateInstance(data.staticTerrain.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTerrain);

	GENDATA->CreateInstance(data.staticSystems.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSystems);

	GENDATA->CreateInstance(data.staticVisibility.staticType, pComp);
	pComp->QueryInterface("IStatic", staticVisibility);

	GENDATA->CreateInstance(data.staticBandwidth.staticType, pComp);
	pComp->QueryInterface("IStatic", staticBandwidth);

	GENDATA->CreateInstance(data.dropGameType.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropGameType);

	GENDATA->CreateInstance(data.dropMoney.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropMoney);

	GENDATA->CreateInstance(data.dropSize.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropSize);
	
	GENDATA->CreateInstance(data.dropTerrain.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropTerrain);

	GENDATA->CreateInstance(data.dropVisibility.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropVisibility);

	GENDATA->CreateInstance(data.bandWidthOption.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", bandWidthOption);

	GENDATA->CreateInstance(data.dropUnits.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropUnits);

	GENDATA->CreateInstance(data.dropSystems.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", dropSystems);

	GENDATA->CreateInstance(data.sliderSpeed.sliderType, pComp);
	pComp->QueryInterface("ISlider", sliderSpeed);

	GENDATA->CreateInstance(data.sliderCmdPoints.sliderType, pComp);
	pComp->QueryInterface("ISlider", sliderCmdPoints);

	GENDATA->CreateInstance(data.staticLockSettings.staticType, pComp);
	pComp->QueryInterface("IStatic", staticLockSettings);

	GENDATA->CreateInstance(data.pushLockSettings.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushLockSettings);

	GENDATA->CreateInstance(data.pushEasy.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushEasy);

	GENDATA->CreateInstance(data.pushAverage.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushAverage);

	GENDATA->CreateInstance(data.pushHard.buttonType, pComp);
	pComp->QueryInterface("IButton2", pushHard);

	GENDATA->CreateInstance(data.staticDifficulty.staticType, pComp);
	pComp->QueryInterface("IStatic", staticDifficulty);	

	GENDATA->CreateInstance(data.staticEasy.staticType, pComp);
	pComp->QueryInterface("IStatic", staticEasy);

	GENDATA->CreateInstance(data.staticAverage.staticType, pComp);
	pComp->QueryInterface("IStatic", staticAverage);

	GENDATA->CreateInstance(data.staticHard.staticType, pComp);
	pComp->QueryInterface("IStatic", staticHard);

	GENDATA->CreateInstance(data.staticCmdPoints.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCmdPoints);

	GENDATA->CreateInstance(data.staticCmdPointsDisplay.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCmdPointsDisplay);

	setStateInfo();
	bInitialized = true;
}
//--------------------------------------------------------------------------//
//
void Menu_map::setDifficultyButtonState (void)
{
	DIFFICULTY difficulty = cqgame.GetDifficulty();
	pushEasy->SetPushState(difficulty==EASY);
	pushAverage->SetPushState(difficulty==AVERAGE);
	pushHard->SetPushState(difficulty==HARD);
}
//--------------------------------------------------------------------------//
//
U32 Menu_map::getNumberPlayers (void)
{
	U32 count = 0;

	for (U32 i = 0; i < cqgame.activeSlots; i++)
	{
		if (cqgame.slot[i].state == ACTIVE || cqgame.slot[i].state == READY)
		{
			count++;
		}
	}

	return count;
}
//--------------------------------------------------------------------------//
//
void Menu_map::fillMapDescription (void)
{
	// are we a random map type?
	if (cqgame.mapType == CQGAMETYPES::RANDOM_MAP)
	{
		if (cqgame.templateType == CQGAMETYPES::TEMPLATE_RANDOM)
		{
			buttonMapType->SetTextString(_localLoadStringW(IDS_MAPTYPE_RANDOM));
		}
		else if (cqgame.templateType == CQGAMETYPES::TEMPLATE_RING)
		{
			buttonMapType->SetTextString(_localLoadStringW(IDS_MAPTYPE_RING));
		}
		else if (cqgame.templateType == CQGAMETYPES::TEMPLATE_STAR)
		{
			buttonMapType->SetTextString(_localLoadStringW(IDS_MAPTYPE_STAR));
		}
		else 
		{
			buttonMapType->SetTextString(_localLoadStringW(IDS_MAPTYPE_NEW_RANDOM));
		}
	}
	else if (HOSTID == PLAYERID)
	{
		// either we are playing a map supplied by DA or a saved game...
		// set the text to the file description (if one exists)
		wchar_t szDesc[MAX_PATH];
		char fileAnsi[MAX_PATH];
		U32 result;

		_localWideToAnsi(cqgame.szMapName, fileAnsi, sizeof(fileAnsi));
		result = MISSION->GetFileDescription(fileAnsi, szDesc, sizeof(szDesc)/sizeof(wchar_t));

		if(!result)
			result = MISSION->GetFileDescription(MPMAPDIR,fileAnsi, szDesc, sizeof(szDesc)/sizeof(wchar_t));
		
		if (result)
		{
			buttonMapType->SetTextString(szDesc);
		}
		else
		{
			buttonMapType->SetTextString(cqgame.szMapName);
		}
	}
	else if (cqgame.mapType == CQGAMETYPES::SELECTED_MAP)
	{
		// if the file exists on our computer, than lets use it's description, otherwise just tell the fucker
		// that it's a saved game from the host
		wchar_t szDesc[MAX_PATH];
		char fileAnsi[MAX_PATH];
		U32 result;

		_localWideToAnsi(cqgame.szMapName, fileAnsi, sizeof(fileAnsi));
		result = MISSION->GetFileDescription(fileAnsi, szDesc, sizeof(szDesc)/sizeof(wchar_t));

		if(!result)
			result = MISSION->GetFileDescription(MPMAPDIR,fileAnsi, szDesc, sizeof(szDesc)/sizeof(wchar_t));

		if (result)
		{
			buttonMapType->SetTextString(szDesc);
		}
		else
		{
			buttonMapType->SetTextID(IDS_HOST_SAVEDGAME);
		}
	}
	else if (cqgame.mapType == CQGAMETYPES::USER_MAP)
	{
		buttonMapType->SetTextID(IDS_HOST_SAVEDGAME);
	}

	U32 oldsystems = cqgame.numSystems;

	dropSystems->ResetContent();

	if (cqgame.mapType == CQGAMETYPES::RANDOM_MAP)
	{
		// okay, let's refil the box with the number of systems allowed...
		// make sure that the old value is put in for kicks
		U32 list[16], num, i;
		num = MAPGEN->GetPosibleSystemNumbers (cqgame, list);

 		// everything that we need in the list is there
		for (i = 0; i < num; i++)
		{
			int index = dropSystems->AddString(_localLoadStringW(IDS_NUMBERS_0 + list[i]));
			dropSystems->SetDataValue(index, list[i]);
		}


		// only change the number of systems here if the number of players has changed and this is not a skirmish game
		U32 newNumberOfPlayers = getNumberPlayers();

		// set the current selection to numplayers * 1.5
		SINGLE numsystems = static_cast<SINGLE>(newNumberOfPlayers) * 1.5f;
		numsystems = ceil(numsystems);
		U32 newsystems = 0;
		if (PLAYERID != 0)
		{
			// a multiplayer game
			if (oldNumberOfPlayers != newNumberOfPlayers)
			{
				oldNumberOfPlayers = newNumberOfPlayers;
				if (oldsystems < numsystems)
				{
					newsystems = numsystems;
				}
				else
				{
					newsystems = oldsystems;
				}
			}
			else
			{
				newsystems = oldsystems;
			}
		}
		else
		{
			// a skirmish game, don't change settings
			newsystems = oldsystems;
		}

		// now go through the all the number of available systems and set the value accordingly
		bool bSetValue = false;
		for (i = 0; i < dropSystems->GetNumberOfItems(); i++)
		{
			U32 data = dropSystems->GetDataValue(i);
			if (data >= newsystems)
			{
				bSetValue = true;
				dropSystems->SetCurrentSelection(i);
				cqgame.SetNumSystems(dropSystems->GetDataValue(i));
				break;
			}
		}
		
		if (bSetValue == false)
		{
			dropSystems->SetCurrentSelection(0);
			cqgame.SetNumSystems(dropSystems->GetDataValue(0));
		}

		// make sure the dropdowns are enabled because we may have disabled it last time we were here
		bool bEnableControls = (HOSTID == PLAYERID && cqgame.bHostBusy);

		dropSystems->EnableDropdown(bEnableControls);
#ifdef _DEMO_
		bEnableControls = false;
#endif
		dropMoney->EnableDropdown(bEnableControls);
		dropSize->EnableDropdown(bEnableControls);
		dropTerrain->EnableDropdown(bEnableControls);
	}
	else
	{
		// we are not playing a random map so we need to disable some of the controls
		dropSystems->AddString(L"--");
		dropSystems->SetCurrentSelection(0);
		dropSystems->EnableDropdown(false);
		dropMoney->EnableDropdown(false);
		dropSize->EnableDropdown(false);
		dropTerrain->EnableDropdown(false);
	}

	// if we've got a saved game selected then we don't want to messing with the number of starting units and bases
	bool bEnableUnits = (HOSTID == PLAYERID && cqgame.bHostBusy) && (cqgame.mapType != CQGAMETYPES::USER_MAP);

#ifdef _DEMO_
		bEnableUnits = false;
#endif
	
	dropUnits->EnableDropdown(bEnableUnits);

	pushSpectator->SetPushState(cqgame.spectatorsOn);
	pushDiplomacy->SetPushState(cqgame.lockDiplomacyOn);
	pushLockSettings->SetPushState(cqgame.bHostBusy==0);
	setDifficultyButtonState();
}
//--------------------------------------------------------------------------//
//
void Menu_map::setMaxPlayers (void)
{
	if(cqgame.mapType == CQGAMETYPES::RANDOM_MAP)
	{
		maxPlayers = MAX_PLAYERS;
	}
	else if(cqgame.mapType == CQGAMETYPES::SELECTED_MAP || cqgame.mapType == CQGAMETYPES::USER_MAP)
	{
		char fileAnsi[MAX_PATH];
		U32 result;

		_localWideToAnsi(cqgame.szMapName, fileAnsi, sizeof(fileAnsi));
		result = MISSION->GetFileMaxPlayers(fileAnsi, maxPlayers);

		if(!result)
			result = MISSION->GetFileMaxPlayers(MPMAPDIR,fileAnsi, maxPlayers);

		if(!result)
			maxPlayers = MAX_PLAYERS;
	}
}

//--------------------------------------------------------------------------//
//
void Menu_map::initDropdowns (void)
{
	int idx;
	dropGameType->SetControlID(IDS_GAMETYPE_KILL_UNITS);
	
	if (CQFLAGS.bLimitGameTypes == false || cqgame.gameType == KILL_UNITS || cqgame.gameType == KILL_HQ_PLATS)
	{
		idx = dropGameType->AddString(_localLoadStringW(IDS_GAMETYPE_KILL_UNITS));
		dropGameType->SetDataValue(idx, KILL_UNITS);

		idx = dropGameType->AddString(_localLoadStringW(IDS_GAMETYPE_KILL_HQ_PLATS));
		dropGameType->SetDataValue(idx, KILL_HQ_PLATS);

		idx = dropGameType->AddString(_localLoadStringW(IDS_GAMETYPE_KILL_PLATFAB));
		dropGameType->SetDataValue(idx, KILL_PLATS_FABS);
	}
	else
	{
		idx = dropGameType->AddString(_localLoadStringW(IDS_GAMETYPE_KILL_PLATFAB));
		dropGameType->SetDataValue(idx, KILL_PLATS_FABS);
	}

	dropMoney->SetControlID(IDS_MONEY_LOW);
	dropMoney->AddString(_localLoadStringW(IDS_MONEY_LOW));
	dropMoney->AddString(_localLoadStringW(IDS_MONEY_MEDIUM));
	dropMoney->AddString(_localLoadStringW(IDS_MONEY_HIGH));

	dropSize->SetControlID(IDS_MAPSIZE_SMALL);
	dropSize->AddString(_localLoadStringW(IDS_MAPSIZE_SMALL));
	dropSize->AddString(_localLoadStringW(IDS_MAPSIZE_MEDIUM));
	dropSize->AddString(_localLoadStringW(IDS_MAPSIZE_LARGE));

	dropTerrain->SetControlID(IDS_TERRAIN_LIGHT);
	dropTerrain->AddString(_localLoadStringW(IDS_TERRAIN_LIGHT));
	dropTerrain->AddString(_localLoadStringW(IDS_TERRAIN_MEDIUM));
	dropTerrain->AddString(_localLoadStringW(IDS_TERRAIN_HEAVY));

	dropVisibility->SetControlID(IDS_VISIBILITY_NORMAL);
	dropVisibility->AddString(_localLoadStringW(IDS_VISIBILITY_NORMAL));
	dropVisibility->AddString(_localLoadStringW(IDS_VISIBILITY_EXPLORED));
	dropVisibility->AddString(_localLoadStringW(IDS_VISIBILITY_NONE));

	bandWidthOption->SetControlID(IDS_BANDWIDTH_NORMAL);
	bandWidthOption->AddString(_localLoadStringW(IDS_BANDWIDTH_NORMAL));
	bandWidthOption->AddString(_localLoadStringW(IDS_BANDWIDTH_BROADBAND));

	dropUnits->SetControlID(IDS_UNITS_MINIMAL);
	dropUnits->AddString(_localLoadStringW(IDS_UNITS_MINIMAL));
	dropUnits->AddString(_localLoadStringW(IDS_UNITS_MEDIUM));
	dropUnits->AddString(_localLoadStringW(IDS_UNITS_HEAVY));

	dropSystems->SetControlID(IDS_NUMBERS_0);
}
//--------------------------------------------------------------------------//
//
void Menu_map::onListSelection (S32 listID)		// user has selected a list item
{
	switch (listID)
	{
	case IDS_GAMETYPE_KILL_UNITS:
		cqgame.SetGameType(static_cast<GAMETYPE>(dropGameType->GetDataValue(dropGameType->GetCurrentSelection())));
		if (cqgame.gameType == MISSION_DEFINED)
		{
			cqgame.SetGameType(CQGAMETYPES::GAMETYPE(cqgame.gameType+1));
		}
		break;

	case IDS_MONEY_LOW:
		cqgame.SetMoney(static_cast<MONEY>(dropMoney->GetCurrentSelection()+LOW_MONEY));
		break;

	case IDS_MAPSIZE_SMALL:
		cqgame.SetMapSize(static_cast<MAPSIZE>(dropSize->GetCurrentSelection()+SMALL_MAP));
		break;

	case IDS_TERRAIN_LIGHT:
		cqgame.SetTerrain(static_cast<TERRAIN>(dropTerrain->GetCurrentSelection()+LIGHT_TERRAIN));
		break;
	
	case IDS_UNITS_MINIMAL:
		cqgame.SetUnits(static_cast<STARTING_UNITS>(dropUnits->GetCurrentSelection()+UNITS_MINIMAL));
		break;

	case IDS_VISIBILITY_NORMAL:
		cqgame.SetVisibility(static_cast<VISIBILITYMODE>(dropVisibility->GetCurrentSelection()+VISIBILITY_NORMAL));
		break;

	case IDS_BANDWIDTH_NORMAL:
		DEFAULTS->GetDefaults()->bNetworkBandwidth = (bandWidthOption->GetCurrentSelection() != 0);
		NETBUFFER->SetTCPNetworkPerformance(DEFAULTS->GetDefaults()->bNetworkBandwidth);
		EVENTSYS->Send(CQE_RESET_NETWORK_PERFORMANCE, 0);
		break;
	
	case IDS_NUMBERS_0:
		cqgame.SetNumSystems(dropSystems->GetDataValue(dropSystems->GetCurrentSelection()));
		break;

	default:
		CQBOMB0("Unknown control");
		break;  
	} 

	setDifficultyButtonState();
}
//--------------------------------------------------------------------------//
//
void  __stdcall DoMenu_map (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, bool _bInternet)
{
	new Menu_map(parent, data, cqgame,_bInternet);
}

//--------------------------------------------------------------------------//
//----------------------------End Menu_map.cpp------------------------------//
//--------------------------------------------------------------------------//
