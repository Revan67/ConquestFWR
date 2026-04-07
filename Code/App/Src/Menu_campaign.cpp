//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_campaign.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_campaign.cpp 45    5/15/01 1:53p Tmauer $
*/
//--------------------------------------------------------------------------//
// Single player options
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>

static FILE *g_mc_f = NULL;
#define MC_LOG(s) do { if(!g_mc_f) g_mc_f=fopen("campaign_diag.txt","w"); if(g_mc_f){fputs((s),g_mc_f);fflush(g_mc_f);} } while(0)
#define MC_LOGF(fmt,...) do { if(!g_mc_f) g_mc_f=fopen("campaign_diag.txt","w"); if(g_mc_f){fprintf(g_mc_f,(fmt),__VA_ARGS__);fflush(g_mc_f);} } while(0)

#include <DMenu1.h>

#include "IShapeLoader.h"
#include "DrawAgent.h"
#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include "Mission.h"
#include "NetConnectBuffers.h"
#include "NetBuffer.h"
#include "IGameProgress.h"
#include "MusicManager.h"

#define MENUSP_MUSIC	"single_player_menu.wav"
#define NAME_REG_KEY	"CQPlayerName"

U32 __stdcall DoMenu_mission (Frame * parent, const GT_MENU1 & data, bool bSkipMenu);
U32 __stdcall MovieScreen (Frame * parent, const char * filename);
U32 __stdcall DoMenu_Briefing (Frame * parent, const char * szFileName);

#define MAX_CHARS 256

//--------------------------------------------------------------------------//
//
struct Menu_campaign : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::SELECT_CAMPAIGN & data;
	const GT_MENU1 & mdata;

	COMPTR<IStatic>  background, title;
	COMPTR<IStatic>  staticName;
	COMPTR<IButton2> buttonTerran, buttonMantis, buttonSolarian;
	COMPTR<IButton2> buttonBack;

	bool bSkipMenu;
	bool bModalUp;
	bool bSkipToMisisonOne;
	U32 uUpdateRecursion;

	INSTANCE_INDEX	terran1ID;

	//
	// instance methods
	//

	Menu_campaign (Frame * _parent, const GT_MENU1 & _data, const bool bSkip) : data(_data.selectCampaign), mdata(_data)
	{
		bSkipMenu = bSkip;
		initializeFrame(_parent);
		init();
	}

	~Menu_campaign (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);


	/* Menu_campaign methods */

	virtual void onUpdate (U32 dt)
	{
		if (bSkipMenu && uUpdateRecursion == 0)
		{
			uUpdateRecursion++;

			const M_RACE race = MISSION->GetSinglePlayerRace();

			if (race == M_TERRAN || race == M_NO_RACE)
			{
				onButtonTerran();
			}
			bSkipMenu = false;
		}
	}

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);
		endDialog(0);
		return true;
	}

	void init (void);

	void onButtonTerran (void);

	void onButtonTraining (wchar_t *wch_buf, const enum M_RACE race);
};
//----------------------------------------------------------------------------------//
//
Menu_campaign::~Menu_campaign (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_campaign::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_SET_FOCUS:
		if (CQFLAGS.b3DEnabled == 0 && bSkipToMisisonOne == false)
		{
			MUSICMANAGER->PlayMusic(MENUSP_MUSIC);
		}
		break;

	default:
		break;
	}
	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_campaign::setStateInfo (void)
{
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALX(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//
	if (background)  background->InitStatic(data.background, this);
	if (title)       title->InitStatic(data.title, this);
	if (staticName)  staticName->InitStatic(data.staticName, this);
	if (buttonTerran)   buttonTerran->InitButton(data.buttonTerran, this);
	if (buttonMantis)   buttonMantis->InitButton(data.buttonMantis, this);
	if (buttonSolarian) buttonSolarian->InitButton(data.buttonSolarian, this);
	if (buttonBack)     buttonBack->InitButton(data.buttonBack, this);

	if (buttonTerran)   buttonTerran->SetTransparent(true);
	if (buttonMantis)   buttonMantis->SetTransparent(true);
	if (buttonSolarian) buttonSolarian->SetTransparent(true);

	// get the name of the player out of the registry
	char nameAnsi[128];
	wchar_t nameWide[128];
	DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, nameAnsi, sizeof(nameAnsi));
	_localAnsiToWide(nameAnsi, nameWide, sizeof(nameWide));
	if (staticName) staticName->SetText(nameWide);

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		const M_RACE race = MISSION->GetSinglePlayerRace();

		switch (race)
		{
		case M_MANTIS:
			if (buttonMantis) setFocus(buttonMantis);
			break;

		case M_SOLARIAN:
			if (buttonSolarian) setFocus(buttonSolarian);
			break;

		default:
			if (buttonTerran) setFocus(buttonTerran);
			break;
		}
	}

	// kludge - send a benign mouse move message to the system
	// otherwise the user has to click twice on the terran campaign button if fullscreen mode
	POINT pt;
	WM->GetCursorPos(pt.x, pt.y);
	WM->SetCursorPos(pt.x, pt.y);
}
//--------------------------------------------------------------------------//
//
void Menu_campaign::init (void)
{
	//
	// create members
	//
	MC_LOGF("Menu_campaign::init: background.staticType='%s'\n", data.background.staticType);
	MC_LOGF("Menu_campaign::init: title.staticType='%s'\n", data.title.staticType);
	MC_LOGF("Menu_campaign::init: buttonTerran.buttonType='%s'\n", data.buttonTerran.buttonType);

	COMPTR<IDAComponent> pComp;
	if (GENDATA->CreateInstance(data.background.staticType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IStatic", background);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for background '%s'\n", data.background.staticType);

	if (GENDATA->CreateInstance(data.title.staticType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IStatic", title);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for title '%s'\n", data.title.staticType);

	if (GENDATA->CreateInstance(data.staticName.staticType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IStatic", staticName);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for staticName '%s'\n", data.staticName.staticType);

	if (GENDATA->CreateInstance(data.buttonTerran.buttonType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IButton2", buttonTerran);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for buttonTerran '%s'\n", data.buttonTerran.buttonType);

	if (GENDATA->CreateInstance(data.buttonMantis.buttonType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IButton2", buttonMantis);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for buttonMantis '%s'\n", data.buttonMantis.buttonType);

	if (GENDATA->CreateInstance(data.buttonSolarian.buttonType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IButton2", buttonSolarian);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for buttonSolarian '%s'\n", data.buttonSolarian.buttonType);

	if (GENDATA->CreateInstance(data.buttonBack.buttonType, pComp) == GR_OK && pComp)
		pComp->QueryInterface("IButton2", buttonBack);
	else
		MC_LOGF("Menu_campaign::init: CreateInstance failed for buttonBack '%s'\n", data.buttonBack.buttonType);

	MC_LOG("Menu_campaign::init: calling setStateInfo\n");
	setStateInfo();
	MC_LOG("Menu_campaign::init: done\n");
}
//--------------------------------------------------------------------------//
//
void Menu_campaign::onButtonTerran (void)
{
	MISSION->SetSinglePlayerRace(M_TERRAN);

	// is this our first time playing the game as this player??
	U32 movieBits = GAMEPROGRESS->GetMoviesSeen();
	U32 missionsSeen = GAMEPROGRESS->GetMissionsSeen();
	bool bSkipMissionMenu = false;

	if (movieBits == 0 && missionsSeen == 0)
	{
		bSkipToMisisonOne = true;
		MUSICMANAGER->PlayMusic(NULL);
//		MovieScreen(this, "cq_intro.mpg", 640, 272);
		GAMEPROGRESS->SetMovieSeen(0);
		bSkipMissionMenu = true;
	}
	else
	{
		// if we are here, then we must of seen the movie by form of introduction
		// we do not want to force the player to see the movie again
		GAMEPROGRESS->SetMovieSeen(0);
	}

	U32 result = 0;
	bModalUp = true;

	SetVisible(false);
	Notify(CQE_KILL_FOCUS, 0);
	result = DoMenu_mission(this, mdata, bSkipMissionMenu);

	if (result)
	{
		endDialog(1);
	}
	else
	{
		SetVisible(true);
		setFocus(buttonTerran);
	}
	Notify(CQE_SET_FOCUS, 0);
	bModalUp = false;
}
//--------------------------------------------------------------------------//
//
void Menu_campaign::onButtonPressed (U32 buttonID)
{
	wchar_t szMission[64];

	switch (buttonID)
	{
	case IDS_BACK:
		endDialog(0);
		break;

	case IDS_TERRAN_CAMPAIGN:
		onButtonTerran();
		break;

	case IDS_MANTIS_TRAINING:	
		swprintf(szMission, L"Mantis_Train.dmission");
		onButtonTraining(szMission, M_MANTIS);
		break;

	case IDS_SOLARIAN_TRAINING:	
		swprintf(szMission, L"Sol_Train.dmission");
		onButtonTraining(szMission, M_SOLARIAN);
		break;		
	}
}
//--------------------------------------------------------------------------//
// 
void Menu_campaign::onButtonTraining(wchar_t *wch_buf, const enum M_RACE race)
{
	MISSION->SetSinglePlayerRace(race);

	char abuffer[MAX_PATH];
	U32 result = 0;
	
	_localWideToAnsi(wch_buf, abuffer, sizeof(abuffer));

	bModalUp = true;
	SetVisible(false);
	Notify(CQE_KILL_FOCUS, 0);
	result = DoMenu_Briefing(this, abuffer);

	if (result)
	{
		endDialog(1);
	}
	else
	{
		SetVisible(true);
		if (race == M_MANTIS)
		{
			setFocus(buttonMantis);
		}
		else
		{
			setFocus(buttonSolarian);
		}
	}

	Notify(CQE_SET_FOCUS, 0);
	bModalUp = false;
}


//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_campaign (Frame * parent, const GT_MENU1 & data, const bool bSkip)
{
	Menu_campaign * dlg = new Menu_campaign(parent, data, bSkip);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_campaign.cpp------------------------------//
//--------------------------------------------------------------------------//