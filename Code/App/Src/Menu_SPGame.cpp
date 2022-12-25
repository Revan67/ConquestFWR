//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_SPGame.cpp                           //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_SPGame.cpp 31    11/01/00 1:54p Jasony $
*/
//--------------------------------------------------------------------------//
// New player dialog
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include <DMenu1.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "Mission.h"
#include "MusicManager.h"

#define NAME_REG_KEY	"CQPlayerName"
#define MENUSP_MUSIC	"single_player_menu.wav"

U32 __stdcall CreateMenuLoadSaveSpecial (bool bLoad, bool bSinglePlayerMaps);
U32 __stdcall DoMenu_campaign (Frame * parent, const GT_MENU1 & data, const bool bSkip);
U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const struct SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone);
U32 __stdcall DoMenu_Briefing (Frame * parent, const char * szFileName);

//--------------------------------------------------------------------------//
//
struct Menu_SPGame : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::SINGLEPLAYER_MENU & data;
	const GT_MENU1 & mdata;

	COMPTR<IStatic>  background, staticSingle, staticName;
	COMPTR<IButton2> buttonCampaign, buttonSkirmish, buttonLoad, buttonQBLoad, buttonBack;

	bool bSkipMenu;
	bool bLookForSinglePlayerMaps;
	U32 nSavedGames;
	U32 uUpdateRecursion;

	//
	// instance methods
	//

	Menu_SPGame (Frame * _parent, const GT_MENU1 & _data, const bool _bSkip) : data(_data.singlePlayerMenu), mdata(_data)
	{
		bSkipMenu = _bSkip;
		initializeFrame(_parent);
		init();
	}

	~Menu_SPGame (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}


	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);


	/* Menu_SPGame methods */

	virtual void onUpdate (U32 dt)
	{
		if (bSkipMenu && uUpdateRecursion == 0)
		{
			uUpdateRecursion++;
			onButtonCampaign();
			bSkipMenu = false;
		}
	}

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	void init (void);

	bool setSPSaveDir (void);

	void onButtonCampaign (void);

	U32  countNumberSavedGames (void);

	void addFile (const WIN32_FIND_DATA & data);

	void addFile (HANDLE handle);
};
//----------------------------------------------------------------------------------//
//
Menu_SPGame::~Menu_SPGame (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_SPGame::Notify (U32 message, void *param)
{

	switch (message)
	{
	case CQE_SET_FOCUS:
		if (CQFLAGS.b3DEnabled == 0)
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
void Menu_SPGame::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//
	background->InitStatic(data.background, this);
	staticSingle->InitStatic(data.staticSingle, this);
	staticName->InitStatic(data.staticName, this);
	buttonCampaign->InitButton(data.buttonCampaign, this);
	buttonSkirmish->InitButton(data.buttonSkirmish, this);
	buttonLoad->InitButton(data.buttonLoad, this);
	buttonQBLoad->InitButton(data.buttonQBLoad, this);
	buttonBack->InitButton(data.buttonBack, this);

	buttonCampaign->SetTransparent(true);
	buttonSkirmish->SetTransparent(true);
	buttonLoad->SetTransparent(true);
	buttonQBLoad->SetTransparent(true);

	// get the name of the player out of the registry
	char nameAnsi[128];
	wchar_t nameWide[128];
	DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, nameAnsi, sizeof(nameAnsi)); 
	_localAnsiToWide(nameAnsi, nameWide, sizeof(nameWide));
	staticName->SetText(nameWide);

	setSPSaveDir(); 

	// do we have any single player games available to load?
	bLookForSinglePlayerMaps = true;
	if (countNumberSavedGames() == 0)
	{
		buttonLoad->EnableButton(false);
	}

	// do we have any skrimish games available to load?
	bLookForSinglePlayerMaps = false;
	if (countNumberSavedGames() == 0)
	{
		buttonQBLoad->EnableButton(false);
	}

	MUSICMANAGER->PlayMusic(MENUSP_MUSIC);

	// the demo build will not allow you to play campaign games
#ifdef _DEMO_
	buttonCampaign->EnableButton(false);
	buttonLoad->EnableButton(false);
#endif

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		setFocus(buttonCampaign);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_SPGame::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.staticSingle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSingle);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", staticName);

	GENDATA->CreateInstance(data.buttonCampaign.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonCampaign);

	GENDATA->CreateInstance(data.buttonSkirmish.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonSkirmish);

	GENDATA->CreateInstance(data.buttonLoad.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonLoad);

	GENDATA->CreateInstance(data.buttonQBLoad.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonQBLoad);

	GENDATA->CreateInstance(data.buttonBack.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonBack);

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
bool Menu_SPGame::setSPSaveDir (void)
{
	char name[128];
	char buffer[256];

	U32 result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, name, sizeof(name));
	CQASSERT(result && "We Must have a Single Player Name set by now");
	
	wsprintf(buffer, "SavedGame\\%s", name);

	DAFILEDESC fdesc = buffer;
	if (SAVEDIR)
	{
		SAVEDIR->Release();
		SAVEDIR = 0;
	}
	if (DACOM->CreateInstance(&fdesc, (void **)&SAVEDIR) != GR_OK)
	{
		// the directory was probably destroyed, remake it and try again
		if (!::CreateDirectory(buffer, 0))
		{
			CQERROR1("Could not create directory '%s'", fdesc.lpFileName);
			return false;
		}
		
		fdesc = buffer;
		if (DACOM->CreateInstance(&fdesc, (void **)&SAVEDIR) != GR_OK)
		{
			CQERROR1("Could not create directory  DACOM '%s'", fdesc.lpFileName);
			return false;
		}
	}

	return true;
}
//--------------------------------------------------------------------------//
//
void Menu_SPGame::addFile (const WIN32_FIND_DATA & data)
{
	// ignore directories
	if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		char fileKey;
		if (bLookForSinglePlayerMaps)
		{
			fileKey = 'f';
		}
		else
		{
			fileKey = 'm';
		}

		if (data.cFileName[0] == fileKey)
		{
			nSavedGames++;
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_SPGame::addFile (HANDLE handle)
{
	WIN32_FIND_DATA data;

	if (SAVEDIR->FindNextFile(handle, &data))
	{
		addFile(handle);		// recursion
		addFile(data);
	}
}
//--------------------------------------------------------------------------//
//
U32 Menu_SPGame::countNumberSavedGames (void)
{
	// go though everything in the saved game directory and count the number
	// of single player saved games
	nSavedGames = 0;

	HANDLE handle;
	WIN32_FIND_DATA data;

	if (SAVEDIR == 0 || (handle = SAVEDIR->FindFirstFile("*.mission", &data)) == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	addFile(handle);		// recursion
	addFile(data);

	return nSavedGames;
}
//--------------------------------------------------------------------------//
//
void Menu_SPGame::onButtonCampaign (void)
{
/*	SetVisible(false);

	if (DoMenu_campaign(this, mdata, bSkipMenu))
	{
		endDialog(1);
	}
	else
	{
		SetVisible(true);
		setFocus(buttonCampaign);
	}
	*/
	MISSION->SetSinglePlayerRace(M_TERRAN);

	SetVisible(false);
	Notify(CQE_KILL_FOCUS, 0);
	U32 result = 0;
	result = DoMenu_Briefing(this, "demo_2_1.qmission");

	if (result)
	{
		endDialog(1);
	}
	else
	{
		SetVisible(true);
	}

	Notify(CQE_SET_FOCUS, 0);
}
//--------------------------------------------------------------------------//
//
void Menu_SPGame::onButtonPressed (U32 buttonID)
{
	switch (buttonID)
	{
	case IDS_LOAD_SAVED_GAME:
		Notify(CQE_KILL_FOCUS, 0);
		if (CreateMenuLoadSaveSpecial(true, true))
		{
			MISSION->LoadInterface();
			SetVisible(false);
			endDialog(1);
		}
		Notify(CQE_SET_FOCUS, 0);
		break;

	case IDS_LOAD_QUICKBATTLE:
		Notify(CQE_KILL_FOCUS, 0);
		if (CreateMenuLoadSaveSpecial(true, false))
		{
			MISSION->LoadInterface();
			SetVisible(false);
			endDialog(1);
		}
		Notify(CQE_SET_FOCUS, 0);
		break;		

	case IDS_SELECT_CAMPAIGN:
		onButtonCampaign();
		break;

	case IDS_SKIRMISH:
		{
			wchar_t name[64];
			staticName->GetText(name, sizeof(name));
			SetVisible(false);
			if (DoMenu_mshell(this, mdata, NULL, name, NULL, false, false))
			{
				endDialog(1);
				return;
			}
			setFocus(buttonSkirmish);
			SetVisible(true);
		}
		break;

	case IDS_BACK:
		endDialog(0);
		break;


	default:
		break;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_SPGame (Frame * parent, const GT_MENU1 & data, bool bSkipMenu)
{
	Menu_SPGame * dlg = new Menu_SPGame(parent, data, bSkipMenu);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_SPGame.cpp--------------------------//
//--------------------------------------------------------------------------//