//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu1.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu1.cpp 155   7/15/02 1:32p Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <stdio.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IAnimate.h"
#include <DMenu1.h>
#include <DStatic.h>
#include "UserDefaults.h"
#include "Mission.h"
#include "SoundManager.h"
#include "MusicManager.h"
#include "hotkeys.h"
#include "IGameProgress.h"

#include <dplay.h>
#include "ZoneLobby.h"

#define NAME_REG_KEY		"CQPlayerName"
#define WIDTH_REG_KEY		"CQWindowWidth"
#define HEIGHT_REG_KEY		"CQWindowHeight"
#define DEPTH_REG_KEY		"CQPixelDepth"
#define RENDERDEV_REG_KEY	"RenderDevice"
#define S_GAME_DIR_A		"SavedGame\\"
#define M_GAME_DIR_A		"SavedGame\\" 

#define MENU1_MUSIC		"Main_menu_screen.wav"

U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const struct SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone);
U32 __stdcall MovieScreen (Frame * parent, const char * filename);
U32 __stdcall CreateOptionsMenu (const bool bFocusing = true);
U32 __stdcall DoMenu_newplayer (Frame * parent, wchar_t * szOldName, wchar_t * bufferNewName, int numChars);
U32 __stdcall DoMenu_SPGame (Frame * parent, const GT_MENU1 & data, bool bSkipToMenu);
U32 __stdcall DoMenu_nc (Frame * parent, const GT_MENU1 & data);
U32 __stdcall DoMenu_help (Frame * parent, const GT_MENU1 & data);

#define ZONE_QUIT 10 //also defined in menu_zone.cpp

Frame * Frame::g_pFocusFrame;

static bool g_bSkipIntroMovies = false;

//--------------------------------------------------------------------------//
//
struct dummy_menu1 : public Frame
{
	BEGIN_DACOM_MAP_INBOUND(dummy_menu1)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()
};
//--------------------------------------------------------------------------//
//
struct Menu1 : public DAComponent<dummy_menu1>
{
	//
	// data items
	//
	GT_MENU1 data;
	COMPTR<IButton2> single, multi, options, intro, help, quit;
	COMPTR<IStatic> background;
	COMPTR<IStatic> staticSingle, staticMulti, staticOptions, staticIntro, staticHelp;
	COMPTR<IAnimate> animSingle, animMulti, animOptions, animQuestion;
	COMPTR<BaseHotRect> gameroom;
	COMPTR<IStatic> staticLegal;

#ifndef _DEMO_
	COMPTR<IAnimate> animMedia;
#endif

	bool bLobbyInited;
	bool bAlreadyLoaded;
	bool bSkipToMission;
	bool bInsideZoneLaunched;

	U32 uUpdateRecursion;

	//
	// instance methods
	//

	Menu1 (const bool bSkip)
	{
		bSkipToMission = bSkip;
		initializeFrame(NULL);
		init();
	}

	~Menu1 (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm);


	/* IEventCallback methods */

	GENRESULT __stdcall Notify (U32 message, void *param);


	/* Menu1 methods */

	virtual void onUpdate (U32 dt)
	{
		if (childFrame == 0)
		{
			if (CQFLAGS.bInsideOutZoneLaunch)
			{
				if (bInsideZoneLaunched==false)
				{
					bInsideZoneLaunched = true;
					onButtonPressed(IDS_MULTI_PLAYER);
				}
			}
			else
			if (CQFLAGS.bDPLobby)
			{
				if (bLobbyInited==false)
					initLobby();
			}
			else
			{
				if (bAlreadyLoaded == false)
				{
					bAlreadyLoaded = true;
					verifyPlayerNames();
				}
				if (bSkipToMission && uUpdateRecursion == 0)
				{
					uUpdateRecursion++;
					onButtonPressed(IDS_SINGLE_PLAYER);
					bSkipToMission = false;
				}
			}
		}
	}

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	void onCancelPressed (void)
	{
		if (bInvisible == false && CQMessageBox(IDS_CONFIRM_QUIT, IDS_CONFIRM, MB_OKCANCEL, this))
		{
//			DoMenu_SlideShow("VFXShape!!ExitGame",8,true);
			endDialog(0);
		}
	}

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_SINGLE_PLAYER:
			if (checkForName())
			{
				DEFAULTS->GetDefaults()->bChoseMultiplayer = 0;
				U32 result;
				setMenuVisible(false);
				animSingle->SetVisible(true);
				result = DoMenu_SPGame(this, data, bSkipToMission);

				if (result)
				{
					endDialog(1);
				}
				else
				{
					setMenuVisible(true);
					setFocus(single);
				}
			}
			break;

		case IDS_MULTI_PLAYER:
			if (checkForName())
			{
				DEFAULTS->GetDefaults()->bChoseMultiplayer = 1;
				U32 result;
				setMenuVisible(false);
				animMulti->SetVisible(true);
				result = DoMenu_nc(this, data);

				if(result == ZONE_QUIT)
				{
					endDialog(0);
				}
				else if (result)
				{
					endDialog(1);
				}
				else
				{
					setMenuVisible(true);
					setFocus(multi);
					StopNetConnection(true);	// failed. shutdown multiplayer
				}
			}
			break;

		case IDS_OPTIONS:
			if (checkForName())
			{
				quit->SetVisible(false);
				staticLegal->SetVisible(false);

				CreateOptionsMenu();

				quit->SetVisible(true);
				staticLegal->SetVisible(true);
				setFocus(options);
			}
			break;


		case IDS_INTRODUCTION:
/*			setMenuVisible(false);
			background->SetVisible(false);
			if (bHasFocus)
				MovieScreen(this, "cq_intro.mpg", 640, 272);
			setMenuVisible(true);
			background->SetVisible(true);
			setFocus(intro);
*/			break;

		case IDS_STATIC_HELP:
			if (bHasFocus)
			{
				quit->SetVisible(false);
				staticLegal->SetVisible(false);
				DoMenu_help(this, data);
				quit->SetVisible(true);
				staticLegal->SetVisible(true);
				setFocus(help);
			}
			break;

		case IDS_QUIT:
			if (bHasFocus)
			{
//				onCancelPressed();
//				DoMenu_SlideShow("VFXShape!!ExitGame",8,true);

				endDialog(0);
			}
			break;
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		if (childFrame==0)
		{
			onCancelPressed();
			return true;
		}
		return false;
	}

	void setMenuVisible (bool bVisible)
	{
		SetVisible(bVisible);
	}

	void init (void);

	void initLobby (void);

	void getSessionNames (wchar_t szPlayerName[64], wchar_t szSessionName[128]);

	const bool checkForName (void);

	void verifyPlayerNames (void);

	bool find3DCard (void);

	void createFolder (wchar_t * szName);

	static bool isNullGUID (const char * pGUID)
	{
		return (strncmp(pGUID, "{00000000-0000-0000-0000-000000000000}", strlen("{00000000-0000-0000-0000-000000000000}")) == 0);
	}
};
//----------------------------------------------------------------------------------//
//
Menu1::~Menu1 (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu1::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu1::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case WM_MOUSEMOVE:
		if (!bInvisible && bHasFocus && animSingle!=0)
		{
			S16 x = static_cast<S16> (LOWORD(msg->lParam));
			S16 y =	static_cast<S16> (HIWORD(msg->lParam));
			
#ifndef _DEMO_
			animMedia->SetVisible(intro->IsMouseOver(x, y));
#endif
			animSingle->SetVisible(single->IsMouseOver(x, y));
			animMulti->SetVisible(multi->IsMouseOver(x, y));
			animOptions->SetVisible(options->IsMouseOver(x, y));
			animQuestion->SetVisible(help->IsMouseOver(x, y));
		}
		break;

	case CQE_SET_FOCUS:
		if (CQFLAGS.b3DEnabled == 0)
		{
			MUSICMANAGER->PlayMusic(MENU1_MUSIC);
		}
		break;

/*	case CQE_HOTKEY:
		if (U32(param) == IDH_CHATTEAM)
		{
			if (bHasFocus)
			{
				SOUNDMANAGER->PlayAnimatedMessage("TH_test.wav", "Animate!!TH_test", 40, 40);
//				MUSICMANAGER->PlayMusic("network_game_winner.wav");
			}
		}
		break;
*/
	}
	return Frame::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Menu1::createFolder (wchar_t * szNameWide)
{
	char szNameAnsi[128];
	_localWideToAnsi(szNameWide, szNameAnsi, sizeof(szNameAnsi)); 

	DEFAULTS->SetStringInRegistry(NAME_REG_KEY, szNameAnsi);

	// the new path +directory
	char szPathAnsi[256];
	wsprintf(szPathAnsi, "%s%s", S_GAME_DIR_A, szNameAnsi);

	::CreateDirectory(szPathAnsi, 0);
}
//----------------------------------------------------------------------------------//
//
const bool Menu1::checkForName (void)
{
	// if we don't have a name in the regsisty then tell the dumb-ass user to input a name
	char buffer[128];
	U32 result;
	
	result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, buffer, sizeof(buffer));

	if (!result || !buffer[0])
	{
		// nothing in the registry - do we have a folder in the SavedGame directory perhaps?
		// this would happen if the player unistalled Conquest but kept the saved games around

		// just find the first directory within the SavedGame diretory and use that for the players name...
		WIN32_FIND_DATA findFileData;
		HANDLE hFind;
		
		findFileData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		
		hFind = ::FindFirstFile("SavedGame\\*.*", &findFileData);

		if (hFind != INVALID_HANDLE_VALUE)
		{
			do 
			{
				if (findFileData.cFileName[0] != '.')
				{
					break;
				}
			} while (::FindNextFile(hFind, &findFileData));

			::FindClose(hFind);
		}

		if (hFind != INVALID_HANDLE_VALUE && findFileData.cFileName[0] != '.')
		{
			strcpy(buffer, findFileData.cFileName);
			result = DEFAULTS->SetStringInRegistry(NAME_REG_KEY, buffer);
		}
		else
		if (CQFLAGS.bDPLobby && PLAYERID!=0)
		{
			U32 size = 0;
			DPLAY->GetPlayerName(PLAYERID, NULL, &size);

			if (size)
			{
				DPNAME * pName = (DPNAME *) malloc(size);
				if (DPLAY->GetPlayerName(PLAYERID, pName, &size) == DP_OK)
				{
					createFolder(pName->lpszShortName);
					_localWideToAnsi(pName->lpszShortName, buffer, sizeof(buffer));
				}
				::free(pName);
				result = 1;
			}
		}
	}

	if (!result || !buffer[0])
	{
		// tell the user to input a name
		// open the player name dlg
		wchar_t nameWide[128];
		
		result = DoMenu_newplayer(this, NULL, nameWide, sizeof(nameWide)/sizeof(wchar_t));

		// put that new name into the registry
		if (result)
		{
			// create the new folder
			createFolder(nameWide);
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}
//----------------------------------------------------------------------------------//
//
void Menu1::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect = data.opening.screenRect;

	background->InitStatic(data.opening.background, parent);

	intro->InitButton(data.opening.intro, this);
	single->InitButton(data.opening.single, this); 
	multi->InitButton(data.opening.multi, this); 
	options->InitButton(data.opening.options, this); 
	help->InitButton(data.opening.help, this);
	quit->InitButton(data.opening.quit, this); 

	staticSingle->InitStatic(data.opening.staticSingle, this, true);
	staticMulti->InitStatic(data.opening.staticMulti, this, true);
	staticIntro->InitStatic(data.opening.staticIntro, this, true);
	staticOptions->InitStatic(data.opening.staticOptions, this, true);
	staticHelp->InitStatic(data.opening.staticHelp, this, true);
	
	staticLegal->InitStatic(data.opening.staticLegal, this);

#ifndef _DEMO_	
	animMedia->InitAnimate(data.opening.animMedia, this, NULL, 0);
	animMedia->SetVisible(false);
#endif

	animSingle->InitAnimate(data.opening.animSingle, this, NULL, 0);
	animSingle->SetVisible(false);

	animMulti->InitAnimate(data.opening.animMulti, this, NULL, 0);
	animMulti->SetVisible(false);

	animOptions->InitAnimate(data.opening.animOptions, this, NULL, 0);
	animOptions->SetVisible(false);

	animQuestion->InitAnimate(data.opening.animQuestion, this, NULL, 0);
	animQuestion->SetVisible(false);

	single->SetControlID(IDS_SINGLE_PLAYER);
	intro->SetControlID(IDS_INTRODUCTION);
	multi->SetControlID(IDS_MULTI_PLAYER);
	options->SetControlID(IDS_OPTIONS);
	help->SetControlID(IDS_STATIC_HELP);
	quit->SetControlID(IDS_QUIT);

	intro->EnableButton(false);
	staticIntro->SetTextColor(RGB(100,100,100));

	DEFAULTS->GetDefaults()->bEditorMode = 0;
	MENU->InitPreferences();

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else 
	if (CQFLAGS.bDPLobby==0)
	{
		if (DEFAULTS->GetDefaults()->bChoseMultiplayer)
		{
			setFocus(multi);
		}
		else
		{
			setFocus(single);
		}
	}
	else
		SetVisible(false);		// don't show this menu if using lobby

}
//--------------------------------------------------------------------------//
//
bool Menu1::find3DCard (void)
{
	HANDLE hSection;
	COMPTR<IProfileParser> parser;
	
	char buffer[MAX_PATH];
	char regValue[MAX_PATH];
	
	bool bPrimary = false;
	bool bFound3DCard = false;
	int i;

	if (DEFAULTS->GetStringFromRegistry(RENDERDEV_REG_KEY, regValue, sizeof(regValue)) == 0)
		strcpy(regValue, "{00000000-0000-0000-0000-000000000000}");

	DACOM->QueryInterface(IID_IProfileParser, parser);

	// find the first card with 3D capabilites
	for (i = 0; i < 4; i++)
	{
		sprintf(buffer, "Rend%d", i);
		
		if ((hSection = parser->CreateSection(buffer)) != 0)
		{
			if (parser->ReadKeyValue(hSection, "DeviceId", buffer, sizeof(buffer)) != 0)
			{
				bool bIsNull = isNullGUID(buffer);
				
				if (bPrimary == false || bIsNull == false)
				{
					if (bPrimary == false)
						bPrimary = bIsNull;

					// does the card have 3D capabilites?
					if (TestDeviceFor3D(buffer))
					{
						bFound3DCard = true;
					}
				}
			}

			parser->CloseSection(hSection);

			if (bFound3DCard)
			{
				break;
			}
		}
	}

	// whether we find a 3D card or not, we're still going to start off in default video mode
	// the default resoulition is 640x480x16
	U32 width = 1024;
	U32 height = 768;
	U32 depth = 32;

	Set3DVarialbes(width, height, depth);

	// right the vars to the registry
	DEFAULTS->SetDataInRegistry(WIDTH_REG_KEY, &width, sizeof(U32));
	DEFAULTS->SetDataInRegistry(HEIGHT_REG_KEY, &height, sizeof(U32));
	DEFAULTS->SetDataInRegistry(DEPTH_REG_KEY, &depth, sizeof(U32));
	
	// get the guid and save it off
	DEFAULTS->SetStringInRegistry(RENDERDEV_REG_KEY, buffer);

	// did we dind a 3D card good enough for us?
	if (bFound3DCard)
	{
		DEFAULTS->GetDefaults()->bHardwareRender = true;
		return true;
	}

	// if we didn't find a card then return false
	DEFAULTS->GetDefaults()->bHardwareRender = false;
	return false;
}
//--------------------------------------------------------------------------//
//
void Menu1::verifyPlayerNames (void)
{
	// these functions should fail 99% of the time - we're just making sure the SavedGame directory is around
	::CreateDirectory("SavedGame\\", 0);

	// make sure that we have the proper directories set up in the single player and multiplayer folders
	char szName[128];
	char szDirectory[256];
	U32 result;
	
	result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, szName, sizeof(szName));

	if (!result || !szName[0])
	{
		// no string available, must be first time through, should be okay

		// start the music so we have something to listen to as we input our name
		MUSICMANAGER->PlayMusic(MENU1_MUSIC);

		// initilize which 3D card we are going to use for 3D play
		if (find3DCard() == 0)
		{
			// aack!  No 3D card was found!
			// tell the user that software rendering is going to be used
			CQMessageBox(IDS_HELP_NO3DCARD, IDS_ERROR, MB_OK, this);
		}

		// ask the player to input a name
		checkForName();
		return;
	}
	else
	{
		// make sure you start the music anyhow
		MUSICMANAGER->PlayMusic(MENU1_MUSIC);
	}

	// we've got a name so make sure the directories are set up and good (create them if you have to)
	wsprintf(szDirectory, "%s%s", S_GAME_DIR_A, szName);
	::CreateDirectory(szDirectory, 0);
	wsprintf(szDirectory, "%s%s", M_GAME_DIR_A, szName);
	::CreateDirectory(szDirectory, 0);

	// now we are going to check that there is always a match between what's in SinglePlayer and what's in
	// multiplayer.  We'll create directories if we have to
}
//--------------------------------------------------------------------------//
//
void Menu1::init (void)
{
	data = 	*((GT_MENU1 *) GENDATA->GetArchetypeData("Menu1"));

	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.opening.single.buttonType, pComp);
	pComp->QueryInterface("IButton2", single);

	GENDATA->CreateInstance(data.opening.multi.buttonType, pComp);
	pComp->QueryInterface("IButton2", multi);

	GENDATA->CreateInstance(data.opening.options.buttonType, pComp);
	pComp->QueryInterface("IButton2", options);
	
	GENDATA->CreateInstance(data.opening.intro.buttonType, pComp);
	pComp->QueryInterface("IButton2", intro);

	GENDATA->CreateInstance(data.opening.help.buttonType, pComp);
	pComp->QueryInterface("IButton2", help);

	GENDATA->CreateInstance(data.opening.quit.buttonType, pComp);
	pComp->QueryInterface("IButton2", quit);

	GENDATA->CreateInstance(data.opening.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.opening.staticSingle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSingle);

	GENDATA->CreateInstance(data.opening.staticMulti.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMulti);

	GENDATA->CreateInstance(data.opening.staticIntro.staticType, pComp);
	pComp->QueryInterface("IStatic", staticIntro);

	GENDATA->CreateInstance(data.opening.staticOptions.staticType, pComp);
	pComp->QueryInterface("IStatic", staticOptions);

	GENDATA->CreateInstance(data.opening.staticHelp.staticType, pComp);
	pComp->QueryInterface("IStatic", staticHelp);

#ifndef _DEMO_
	GENDATA->CreateInstance(data.opening.animMedia.animateType, pComp);
	pComp->QueryInterface("IAnimate", animMedia);
#endif

	GENDATA->CreateInstance(data.opening.animSingle.animateType, pComp);
	pComp->QueryInterface("IAnimate", animSingle);

	GENDATA->CreateInstance(data.opening.animMulti.animateType, pComp);
	pComp->QueryInterface("IAnimate", animMulti);

	GENDATA->CreateInstance(data.opening.animOptions.animateType, pComp);
	pComp->QueryInterface("IAnimate", animOptions);

	GENDATA->CreateInstance(data.opening.animQuestion.animateType, pComp);
	pComp->QueryInterface("IAnimate", animQuestion);

	GENDATA->CreateInstance(data.opening.staticLegal.staticType, pComp);
	pComp->QueryInterface("IStatic", staticLegal);
}
//--------------------------------------------------------------------------//
// command line switch included /lobby
//
void Menu1::initLobby (void)
{
	S32 lobbied;
	DPCAPS dpcaps;

	bLobbyInited = true;
	
	CURSOR->SetBusy(1);
	StartNetConnection(lobbied);
	CURSOR->SetBusy(0);

	if (lobbied == 0)
	{
		StopNetConnection();
		CQFLAGS.bDPLobby = 0;
		CQMessageBox(IDS_HELP_LOBBYFAILED, IDS_APP_NAMETM, MB_OK, this);
		endDialog(0);
		return;
	}

	dpcaps.dwSize = sizeof(dpcaps);
	DPLAY->GetPlayerCaps(PLAYERID, &dpcaps, 0);

	if (dpcaps.dwFlags & DPCAPS_ISHOST)
	{
		HOSTID = PLAYERID;
		EVENTSYS->Send(CQE_NEWHOST, 0);		// notify locals that host has changed
	}
	EVENTSYS->Send(CQE_NETSTARTUP, 0);		// annouce to everyone that we are starting a net session

	//
	// start the gameroom menu
	//
	wchar_t sname[128];
	wchar_t pname[64];
	getSessionNames(pname, sname);

	verifyPlayerNames();

	if (DoMenu_mshell(this, data, NULL, pname, sname, false, true) == 0)
	{
		PostMessage(CQE_BUTTON, (void *)IDS_QUIT);		// post an extra quit message just to be sure
		endDialog(0);
	}
	else
	{
		endDialog(1);
	}
}
//--------------------------------------------------------------------------//
//
void Menu1::getSessionNames (wchar_t szPlayerName[64], wchar_t szSessionName[128])
{
	DPSESSIONDESC2 *pDesc;
	U32 size=0;
	HRESULT hr;

	szSessionName[0] = szPlayerName[0] = 0;

	DPLAY->GetSessionDesc(NULL, &size);
	pDesc = (DPSESSIONDESC2 *) malloc(size);
	hr = DPLAY->GetSessionDesc(pDesc, &size);
	CQASSERT(hr == DP_OK);

	wcsncpy(szSessionName, pDesc->lpszSessionName, 128 / sizeof(wchar_t));
	free(pDesc);
	pDesc = 0;
	
	//
	// now get player name
	//
	
	size = 0;
	DPLAY->GetPlayerName(PLAYERID, NULL, &size);

	if (size)
	{
		DPNAME * pName = (DPNAME *) malloc(size);
		if (DPLAY->GetPlayerName(PLAYERID, pName, &size) == DP_OK)
			wcsncpy(szPlayerName, pName->lpszShortName, 64 / sizeof(wchar_t));
		::free(pName);
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateMenu1 (bool bSkipMenus)
{
#ifdef _DEMO_
	if(g_bSkipIntroMovies == false && CQFLAGS.bDPLobby==0)
		DoMenu_SlideShow("VFXShape!!DemoIntro",5,true);
#else
	// have to play the same 2 logo movies every time
	if (g_bSkipIntroMovies == false && CQFLAGS.bSkipIntroMovie==0 && CQFLAGS.bDPLobby==0)
	{
		// no more Microsoft intro movie, but we'll want to put our new publisher here someday
//		MovieScreen(NULL, "UbiLogo.avi", 640, 480);
//		if(findUserLang() == MAKELANGID(LANG_KOREAN,SUBLANG_KOREAN))
//			MovieScreen(NULL, "JoyOn.avi", 720, 480);
//		MovieScreen(NULL, "fps_logo.avi", 420, 176);
	}
#endif

	g_bSkipIntroMovies = true;

	// don't show intro movie if we're in lobby mode

#ifndef _DEMO_
	
	/*if (CQFLAGS.bDPLobby == 0 && CQFLAGS.bSkipMovies==0)
	{
		char buffer[128];
		U32 result;
		
		result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, buffer, sizeof(buffer));

		// since this is the first time through, play our intro movie
		if (result == 0)
		{
			MovieScreen(NULL, "cq_intro.mpg", 640, 272);
			GAMEPROGRESS->ForcedIntroMovie();
		}
	}*/

#endif

	Menu1 * menu = new Menu1(bSkipMenus);

	GENDATA->FlushUnusedArchetypes();  //get rid of menu shapes I no longer need

	menu->createViewer("\\GT_MENU1\\Menu1", "GT_MENU1", IDS_VIEWMENU1);
	menu->beginModalFocus();

	CURSOR->SetDefaultCursor();

	U32 result = CQDoModal(menu);
	delete menu;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu1.cpp--------------------------------//
//--------------------------------------------------------------------------//
