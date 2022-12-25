//--------------------------------------------------------------------------//
//                                                                          //
//                           Menu_options.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_options.cpp 101   8/23/01 9:12a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <DEffectOpts.h>
#include <Digoptions.h>
#include <wchar.h>
#include <Rpul\\RPUL_Misc.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IListbox.h"
#include "ISlider.h"
#include "ITabControl.h"
#include "IShapeLoader.h"

#include "IGammaControl.h"
#include "SoundManager.h"
#include "Menu.h"
#include "IGameProgress.h"

#include "Mission.h"
#include "Hotkeys.h"

#include <IProfileParser.h>
#include <stdio.h>

// turn off "global" optimizations
#ifndef _DEBUG
#pragma optimize( "ga", off )
#endif

#define NAME_REG_KEY   "CQPlayerName"
#define WIDTH_REG_KEY  "CQWindowWidth"
#define HEIGHT_REG_KEY "CQWindowHeight"
#define DEPTH_REG_KEY  "CQPixelDepth"
#define RENDERDEV_REG_KEY "RenderDevice"

#define TAB_VFX_OPTIONS		"VFXShape!!TabOptions"

#define MAX_CHARS 256
#define MAX_PLAYER_CHARS 32

#define LIST_PLAYERS 12000

#define PUSH_MUSIC 1000
#define PUSH_SFX   1001
#define PUSH_COMM  1002
#define PUSH_CHAT  1003

#define SLIDER_MUSIC 1004
#define SLIDER_SFX   1005
#define SLIDER_COMM  1006
#define SLIDER_CHAT  1007

#define PUSH_TRAILS 	3000
#define PUSH_EMISSIVE	3001
#define PUSH_DETAIL		3002
#define SLIDER_DRAWBACK	3003
#define SLIDER_SHIPS3D	3004
#define PUSH_HARDWARE	3005

#define SLIDER_GAMMA	5000

#define PUSH_STATUS		2001
#define PUSH_MAP		2002
#define PUSH_ROLLOVER	2003
#define PUSH_RIGHTCLICK	2004
#define PUSH_DINPUT     2005
#define PUSH_SUBTITLES	2006

#define SLIDER_GAME		4000
#define SLIDER_SCROLL	4001
#define SLIDER_MOUSE	4002

#define DROP_DEVICES    7001

#define GAME_DIR_ANSI "SavedGame\\"

/*
struct RPDDDEVICEINFO
{
	U32			 vendor_id_val;
	U32			 device_id_val;
	RPDEVICEID	 device_id;
};

RPDDDEVICEINFO rp_dd_device_ids[] =
{
	{ 0x12D2, 0x0018, RP_D_RIVA128			},
	{ 0x12D2, 0x0019, RP_D_RIVA128			},
	{ 0x10DE, 0x0020, RP_D_RIVATNT			},
	{ 0x10DE, 0x0028, RP_D_RIVATNT2			},
	{ 0x10DE, 0x0029, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002A, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002B, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002C, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002D, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002E, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002F, RP_D_RIVATNT2			},
	{ 0x121A, 0x0001, RP_D_VOODOO_1			},
	{ 0x121A, 0x0002, RP_D_VOODOO_2			},
	{ 0x121A, 0x0003, RP_D_VOODOO_BANSHEE	},
	{ 0x121A, 0x0005, RP_D_VOODOO_3			},
	{ 0x1142, 0x643D, RP_D_VOODOO_RUSH		},
	{ 0x10D9, 0x8626, RP_D_VOODOO_RUSH		},
	{ 0x3d3d, 0x0009, RP_D_PERMEDIA_2		},

	{      0,      0, RP_D_GENERIC			}
};
*/


U32 __stdcall DoMenu_newplayer (Frame * parent, wchar_t * szOldName, wchar_t * bufferNewName, int numChars);

//--------------------------------------------------------------------------//
//
struct NameList
{
	wchar_t szName[MAX_PLAYER_CHARS];
	wchar_t szOldName[MAX_PLAYER_CHARS];
	struct NameList * pNext;

	NameList (void)
	{
		memset(szName, 0, sizeof(szName));
		memset(szOldName, 0, sizeof(szOldName));
	}
};

//--------------------------------------------------------------------------//
//
bool item_in_list (NameList * list, wchar_t * szName, bool bCheckOld = false)
{
	NameList * p = list;

	while (p)
	{
		if (bCheckOld)
		{
			if (p->szOldName && wcscmp(p->szOldName, szName) == 0)
			{
				return true;
			}
		}
		else
		{
			if (wcscmp(p->szName, szName) == 0)
			{
				return true;
			}
		}
		p = p->pNext;
	}

	return false;
}
//--------------------------------------------------------------------------//
//
void add_name_to_list (NameList *& list, wchar_t * szNewName, wchar_t * szOldName = NULL)
{
	// do not add the name to the list if it is already there
	if (szOldName)
	{
		if (item_in_list(list, szNewName))
		{
			return;
		}
	}

	// the new entry, it always gets added to the end of the list
	NameList * pEntry = new NameList;
	wcsncpy(pEntry->szName, szNewName, wcslen(szNewName));

	if (szOldName)
	{
		wcsncpy(pEntry->szOldName, szOldName, wcslen(szOldName));
	}

	pEntry->pNext = NULL;

	// if list is NULL, then add the stringID to the begining of the list
	if (list == NULL)
	{
		list = pEntry;
	}
	else
	{
		// add the object to the end of the list
		NameList * p = list;
		
		while (p)
		{
			if (p->pNext == NULL)
			{
				p->pNext = pEntry;
				break;
			}
			p = p->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
void delete_list (NameList * list)
{
	NameList * p = list;
	NameList * pNext = NULL;
	
	while (p)
	{
		pNext = p->pNext;
		delete p;
		p = pNext;
	}
}
//--------------------------------------------------------------------------//
//
void delete_item_from_list (NameList *& list, wchar_t * szName)
{
	NameList * p = list;
	NameList * pPrev = NULL;

	while (p)
	{
		if (wcscmp(p->szName, szName) == 0)
		{
			// delete the item from the list
			if (pPrev == NULL)
			{
				// we are deleting the first item in the list
				list = p->pNext;
				delete p;
			}
			else
			{
				NameList * pDelete = p;
				pPrev->pNext = p->pNext;
				delete pDelete;
			}
			break;
		}
		
		pPrev = p;
		p = p->pNext;
	}
}

//--------------------------------------------------------------------------//
//
struct ResEnum
{
	U32 regWidth, regHeight, regDepth;
	U32 regIndex;
	IListbox * list;
	DDDEVICEIDENTIFIER2 * ident2;
	int numModes;
};
//--------------------------------------------------------------------------//
//
struct PackedRes
{
	U32 dwWidth:12;
	U32 dwHeight:12;
	U32 dwDepth:8;

	operator U32 (void) const 
	{
		const U32 * _this = (const U32 *) this;

		return *_this;
	}
};

//--------------------------------------------------------------------------//
//
struct dummy_options : public Frame
{
	BEGIN_DACOM_MAP_INBOUND(dummy_options)
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
struct Menu_options : public DAComponent<dummy_options>
{
	//
	// data items
	//
	GT_OPTIONS data;

	COMPTR<IStatic> background, title;

	// stuff for player name controls
	struct subStruct
	{
		COMPTR<IStatic>		staticName;
		COMPTR<IListbox>	listNames;
		COMPTR<IButton2>	buttonNew, buttonChange, buttonDelete;

		// stuff for the slider controls
		COMPTR<IStatic>		staticSound, staticMusic, staticComm, staticChat, staticSpeed, staticScroll, staticMouse;
		COMPTR<ISlider>		sliderSound, sliderMusic, sliderComm, sliderChat, sliderSpeed, sliderScroll, sliderMouse;
		COMPTR<IButton2>	pushSound, pushMusic, pushComm;

		// stuff for the check boxes
		COMPTR<IStatic>		staticDInput, staticStatus, staticRollover, staticSectorMap, staticRightClick, staticSubtitles;
		COMPTR<IButton2>	pushDInput, pushStatus, pushRollover, pushSectorMap, pushRightClick, pushSubtitles;

		// stuff for the graphics
		COMPTR<IStatic>		staticResolution, staticGamma;
		COMPTR<IDropdown>	dropResolution;
		COMPTR<ISlider>		sliderGamma, slideDrawBack, sliderShips3D;
		COMPTR<IStatic>		staticShips3D, staticTrails, staticEmissive, staticDetail, staticDrawBack;
		COMPTR<IButton2>	pushTrails, pushEmissive, pushDetail;

		COMPTR<IStatic>		staticDevice;
		COMPTR<IDropdown>	dropDevice;
		COMPTR<IStatic>		static3DHardware;
		COMPTR<IButton2>    push3DHardware;
	} tb;

	COMPTR<IButton2> buttonOk, buttonCancel;

	// the tab control
	COMPTR<ITabControl> tabControl;

	COMPTR<IFileSystem> fileSys;

	// variables for remember who to select, delete, and add
	NameList * listAdd;
	NameList * listChange;
	NameList * listDelete;
	NameList * listCurrent;
	wchar_t szDefaultName[MAX_PLAYER_CHARS];
	
	struct DefaultInfo
	{
		S32  nDefaultGamma;
		bool bDefaultTrails;
		bool bDefaultEmissive;
		bool bDefaultDetail;
		bool bDefaultTexture;
		S32 nDefaultDrawBack;
		S32 n3DShips;

		bool bDefaultEffectsMuted;
		bool bDefaultMusicMuted;
		bool bDefaultCommsMuted;
		bool bDefaultNetChatMuted;
		S32  nDefaultEffects;
		S32  nDefaultMusic;
		S32  nDefaultComms;
		S32  nDefaultNetChat;

		bool bDefaultTooltips;
		bool bDefaultStatus;
		bool bDefaultMap;
		bool bDefaultRollover;
		bool bDefaultRightClick;
		bool bDefaultSubtitles;
		bool bDefaultDInput;

		S32 nDefaultGame;
		S32 nDefaultMouse;
		SINGLE fDefaultScrollRate;
	};

	DefaultInfo currentInfo;
	DefaultInfo defaultInfo;

	DDDEVICEIDENTIFIER2 ident2;		// valid after calling fillDeviceList()
	bool b3DCardFound;

	//
	// instance methods
	//

	Menu_options (void)
	{
		eventPriority = EVENT_PRIORITY_IG_OPTIONS;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~Menu_options (void);

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

	/* Menu_options methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		return false;
	}

	virtual bool onNextPressed (void)
	{
		return false;
	}

	virtual bool onPrevPressed (void)
	{
		return false;
	}

	virtual void onButtonPressed (U32 buttonID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonCancel();
		return true;
	}

	virtual void onSetFocus (bool bFocus)
	{
		if (bFocus)
		{
			setFocus(buttonOk);
		}
		else
		{
			setFocus(NULL);
		}

		BaseHotRect::onSetFocus(bFocus);
	}


	void addFolder (HANDLE handle)
	{
		WIN32_FIND_DATA data;

		if (fileSys->FindNextFile(handle, &data))
		{
			addFolder(handle);		// recursion
			addFolder(data);
		}
	}
	
	void addFolder (WIN32_FIND_DATA data)
	{
		// interested in directories only
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (data.cFileName[0] != '.'))
		{
			wchar_t buffer[256];

			_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));
			add_name_to_list(listCurrent, buffer, NULL);
		}
	}

	void doPushButton (IButton2 *pButton)
	{
		pButton->SetPushState(!pButton->GetPushState());
	}

	void checkState(IButton2 *button, ISlider *slider)
	{
		button->SetPushState(!button->GetPushState());
		slider->EnableSlider(button->GetPushState());
	}

	virtual void onSliderPressed (U32 sliderID);

	virtual void onListCaretMove (S32 listID);

	virtual void onListSelection (S32 listID);

	void onButtonOk (void);

	void onButtonCancel (void);

	void onButtonNew (void);

	void onButtonDelete (void);

	void onButtonChange (void);

	void initNameList (void);

	void redoNameList (void);

	void init (void);

	void initFileSys (void);

	void resetDefaults (DefaultInfo & info);

	void getGUIDFromIndex (int index, char * pGUID, U32 bufferSize);

	static bool isNullGUID (const char * pGUID)
	{
		return (strncmp(pGUID, "{00000000-0000-0000-0000-000000000000}", strlen("{00000000-0000-0000-0000-000000000000}")) == 0);
	}

	static int __stdcall fillResolutionList (const char * pGUID, IListbox * list, DDDEVICEIDENTIFIER2 * ident2);

	static HRESULT __stdcall resCallback (struct _DDSURFACEDESC2 * ddesc, LPVOID lpContext);

	int fillDeviceList (IListbox * devList, IListbox * resList);

//	void deleteAddedFolders (void);

	void deleteMarkedNames (void);

	void changeMarkedFolders (void);

	void addMarkedFolders (void);

//	void changeBackFolders (void);

	void createFolder (wchar_t * szName);

	void changeFolderName (wchar_t * szOldNameWide, wchar_t * szNewNameWide);

	void deleteInfoForPlayer (wchar_t * szFolderName);

	void onButton3DHardware (void);
};
//----------------------------------------------------------------------------------//
//
Menu_options::~Menu_options (void)
{
	tb.~subStruct();

	// delete everything in our name lists if we must
	delete_list(listAdd);
	delete_list(listDelete);
	delete_list(listChange);
	delete_list(listCurrent);
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_options::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
/*
void Menu_options::deleteAddedFolders (void)
{
	NameList * p = listAdd;

	while (p)
	{
		// get the name of the folder that was created and destroy it now
		deleteInfoForPlayer(p->szName);
		p = p->pNext;
	}
}
*/
//----------------------------------------------------------------------------------//
//
void Menu_options::deleteMarkedNames (void)
{
	NameList * p = listDelete;

	while (p)
	{
		deleteInfoForPlayer(p->szName);
		p = p->pNext;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_options::addMarkedFolders (void)
{
	// this function may well fail - we're just making sure the SavedGame directory is there
	::CreateDirectory("SavedGame\\", 0);

	NameList * p = listAdd;

	while (p)
	{
		createFolder(p->szName);

		// place the name into the registry
		char nameAnsi[MAX_PLAYER_CHARS];
		_localWideToAnsi(p->szName, nameAnsi, sizeof(nameAnsi));
		DEFAULTS->SetStringInRegistry(NAME_REG_KEY, nameAnsi);

		// the newly selected player will have a different game progress, reinitialize everything
		GAMEPROGRESS->ReInitialize();

		p = p->pNext;
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_options::changeMarkedFolders (void)
{
	NameList * p = listChange;

	while (p)
	{
		changeFolderName(p->szOldName, p->szName);
		p = p->pNext;
	}
}
/*
//----------------------------------------------------------------------------------//
//
void Menu_options::changeBackFolders (void)
{
	NameList * p = listChange;

	while (p)
	{
		changeFolderName(p->szName, p->szOldName);
		p = p->pNext;
	}
}*/
//--------------------------------------------------------------------------//
//
void Menu_options::deleteInfoForPlayer (wchar_t * szFolderNameWide)
{
	COMPTR<IFileSystem> file;
	char szFolderNameAnsi[MAX_PLAYER_CHARS];
	char szFolderPathAnsi[MAX_CHARS];

	_localWideToAnsi(szFolderNameWide, szFolderNameAnsi, sizeof(szFolderNameAnsi));

	// delete the single player filename folder and its contents
	wsprintf(szFolderPathAnsi, "%s%s", GAME_DIR_ANSI, szFolderNameAnsi);

	DAFILEDESC fdesc = szFolderPathAnsi;
	if (DACOM->CreateInstance(&fdesc, file) == GR_OK)
	{
		RecursiveDelete(file);

		// now delete the directory
		file.free();
		::RemoveDirectory(szFolderPathAnsi);
	}

	DEFAULTS->DeletePlayerDefaults(szFolderNameAnsi);
}
//--------------------------------------------------------------------------//
//
void Menu_options::createFolder (wchar_t * szNameWide)
{
	char szNameAnsi[128];
	_localWideToAnsi(szNameWide, szNameAnsi, sizeof(szNameAnsi)); 

	// the new path +directory
	char szPathAnsi[256];
	wsprintf(szPathAnsi, "%s%s", GAME_DIR_ANSI, szNameAnsi);

	::CreateDirectory(szPathAnsi, 0);
}
//--------------------------------------------------------------------------//
//
void Menu_options::changeFolderName (wchar_t * szOldNameWide, wchar_t * szNewNameWide)
{
	char szNewNameAnsi[128];
	_localWideToAnsi(szNewNameWide, szNewNameAnsi, sizeof(szNewNameAnsi)); 

	char szOldNameAnsi[128];
	_localWideToAnsi(szOldNameWide, szOldNameAnsi, sizeof(szOldNameAnsi));
	
	// the old path + directory
	char szOldPathAnsi[256];
	wsprintf(szOldPathAnsi, "%s%s", GAME_DIR_ANSI, szOldNameAnsi);

	// the new path +directory
	char szNewPathAnsi[256];
	wsprintf(szNewPathAnsi, "%s%s", GAME_DIR_ANSI, szNewNameAnsi);


	// now, switch the name, man
	::MoveFile(szOldPathAnsi, szNewPathAnsi);

	// is there anymore cleanup to do here?
	// does the registry stuff change properly
}
//----------------------------------------------------------------------------------//
//
void Menu_options::resetDefaults (DefaultInfo & info)
{
	DEFAULTS->LoadDefaults();
	DEFAULTS->GetDefaults()->bEditorMode = 0;
	MENU->InitPreferences();

	USER_DEFAULTS * pUserDefaults = DEFAULTS->GetDefaults();	
	
	info.nDefaultGamma		= pUserDefaults->gammaCorrection;
	info.bDefaultTrails		= CQEFFECTS.bWeaponTrails != 0;
	info.bDefaultEmissive	= CQEFFECTS.bEmissiveTextures != 0; 
	info.bDefaultDetail		= CQEFFECTS.bExpensiveTerrain != 0;
	info.bDefaultTexture	= CQEFFECTS.bTextures != 0;
	info.n3DShips			= CQEFFECTS.nFlatShipScale;

	info.nDefaultDrawBack	= (CQEFFECTS.bHighBackground ? 2 : (CQEFFECTS.bBackground ? 1 : 0));

	SOUNDMANAGER->GetSfxVolumeLevel(info.nDefaultEffects, info.bDefaultEffectsMuted);
	SOUNDMANAGER->GetMusicVolumeLevel(info.nDefaultMusic, info.bDefaultMusicMuted);
	SOUNDMANAGER->GetCommVolumeLevel(info.nDefaultComms, info.bDefaultCommsMuted);
	SOUNDMANAGER->GetChatVolumeLevel(info.nDefaultNetChat, info.bDefaultNetChatMuted);

	info.bDefaultTooltips	= pUserDefaults->bNoTooltips == 0;
	info.bDefaultStatus		= pUserDefaults->bNoStatusBar == 0;
	info.bDefaultMap		= pUserDefaults->bSectormapRotates;
	info.bDefaultRollover	= pUserDefaults->bNoHints == 0;
	info.bDefaultRightClick = pUserDefaults->bRightClickOption;
	info.bDefaultSubtitles  = pUserDefaults->bSubtitles;
	info.bDefaultDInput		= pUserDefaults->bHardwareCursor == 0;

	info.nDefaultGame		= pUserDefaults->gameSpeed;
	info.nDefaultMouse		= pUserDefaults->mouseSpeed;
	info.fDefaultScrollRate = pUserDefaults->scrollRate;

	tb.sliderSound->SetPosition(info.nDefaultEffects);
	tb.sliderSound->EnableSlider(info.bDefaultEffectsMuted == 0);
	tb.pushSound->SetPushState(info.bDefaultEffectsMuted == 0);

	tb.sliderMusic->SetPosition(info.nDefaultMusic);
	tb.sliderMusic->EnableSlider(info.bDefaultMusicMuted == 0);
	tb.pushMusic->SetPushState(info.bDefaultMusicMuted == 0);

	tb.sliderComm->SetPosition(info.nDefaultComms);
	tb.sliderComm->EnableSlider(info.bDefaultCommsMuted == 0);
	tb.pushComm->SetPushState(info.bDefaultCommsMuted == 0);

	tb.sliderChat->SetPosition(info.nDefaultNetChat);

	tb.sliderMouse->SetPosition(info.nDefaultMouse);
	tb.pushRollover->SetPushState(info.bDefaultRollover);

	tb.sliderSpeed->SetPosition(info.nDefaultGame);
	tb.sliderScroll->SetPosition(info.fDefaultScrollRate*2.0f - 1);

	tb.pushSectorMap->SetPushState(info.bDefaultMap);
	tb.pushRightClick->SetPushState(info.bDefaultRightClick);
	tb.pushSubtitles->SetPushState(info.bDefaultSubtitles);
	tb.pushStatus->SetPushState(info.bDefaultStatus);
	tb.pushDInput->SetPushState(info.bDefaultDInput);

	tb.sliderGamma->SetPosition(info.nDefaultGamma);

	tb.sliderShips3D->SetPosition(info.n3DShips);
	tb.slideDrawBack->SetPosition(info.nDefaultDrawBack);

	tb.pushEmissive->SetPushState(info.bDefaultEmissive);
	tb.pushDetail->SetPushState(info.bDefaultDetail);
	tb.pushTrails->SetPushState(info.bDefaultTrails);

	tb.sliderMouse->EnableSlider(tb.pushDInput->GetPushState());
}
//----------------------------------------------------------------------------------//
//
void Menu_options::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);
	
	// if the game is not active, then we want to position this screen for the 2D 800x600 front end system
	if (CQFLAGS.bGameActive == false)
	{
		int width  = screenRect.right - screenRect.left;
		int height = screenRect.bottom - screenRect.top;
		screenRect.left = (SCREENRESX-width)/2;
		screenRect.top = (SCREENRESY-height)/2;
		screenRect.right = screenRect.left + width;
		screenRect.bottom = screenRect.top + height;
	}

	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);

	// load up the tab control - man I hate doing this
	COMPTR<IDAComponent> pBase;
	COMPTR<IShapeLoader> loader;

	GENDATA->CreateInstance(TAB_VFX_OPTIONS, pBase);
	pBase->QueryInterface("IShapeLoader", loader);

	// init the tab control
	tabControl->InitTab(data.tab, this, loader);
	COMPTR<BaseHotRect> base;

	//
	// put everything into the player tab (tab 1)
	//
	tabControl->GetTabMenu(0, base);

	tb.staticName->InitStatic(data.staticName, base);
	tb.listNames->InitListbox(data.listNames, base);
	tb.listNames->SetControlID(LIST_PLAYERS);

	tb.buttonNew->InitButton(data.buttonNew, base);
	tb.buttonNew->SetTransparent(true);
	tb.buttonChange->InitButton(data.buttonChange, base);
	tb.buttonChange->SetTransparent(true);
	tb.buttonDelete->InitButton(data.buttonDelete, base);
	tb.buttonDelete->SetTransparent(true);

	tb.staticMouse->InitStatic(data.staticMouse, base);
	tb.staticSpeed->InitStatic(data.staticSpeed, base);
	tb.staticScroll->InitStatic(data.staticScroll, base);
	tb.staticStatus->InitStatic(data.staticStatus, base);
	tb.staticRollover->InitStatic(data.staticRollover, base);
	tb.staticSectorMap->InitStatic(data.staticSectorMap, base);
	tb.staticRightClick->InitStatic(data.staticRightClick, base);
	tb.staticSubtitles->InitStatic(data.staticSubtitles, base);
	tb.staticDInput->InitStatic(data.staticDInput, base);

	tb.pushDInput->InitButton(data.pushDInput, base);
	tb.pushDInput->SetControlID(PUSH_DINPUT);

	tb.sliderMouse->InitSlider(data.sliderMouse, base);
	tb.sliderMouse->SetControlID(SLIDER_MOUSE);
	tb.sliderMouse->SetRangeMin(-10);
	tb.sliderMouse->SetRangeMax(10);

	tb.sliderSpeed->InitSlider(data.sliderSpeed, base);
	tb.sliderSpeed->SetControlID(SLIDER_GAME);
	tb.sliderSpeed->SetRangeMin(-5);
	tb.sliderSpeed->SetRangeMax(5);

	tb.sliderScroll->InitSlider(data.sliderScroll, base);
	tb.sliderScroll->SetControlID(SLIDER_SCROLL);
	tb.sliderScroll->SetRangeMin(0);
	tb.sliderScroll->SetRangeMax(5);

	tb.pushStatus->InitButton(data.pushStatus, base);
	tb.pushStatus->SetControlID(PUSH_STATUS);

	tb.pushRollover->InitButton(data.pushRollover, base);
	tb.pushRollover->SetControlID(PUSH_ROLLOVER);

	tb.pushSectorMap->InitButton(data.pushSectorMap, base);
	tb.pushSectorMap->SetControlID(PUSH_MAP);

	tb.pushRightClick->InitButton(data.pushRightClick, base);
	tb.pushRightClick->SetControlID(PUSH_RIGHTCLICK);

	tb.pushSubtitles->InitButton(data.pushSubtitles, base);
	tb.pushSubtitles->SetControlID(PUSH_SUBTITLES);

	//
	// the second tab (Graphics)
	//
	tabControl->GetTabMenu(1, base);
	
	tb.staticDevice->InitStatic(data.staticDevice, base);
	tb.staticResolution->InitStatic(data.staticResolution, base);
	tb.staticGamma->InitStatic(data.staticGamma, base);
	tb.staticTrails->InitStatic(data.staticTrails, base);
	tb.staticEmissive->InitStatic(data.staticEmissive, base);
	tb.staticDetail->InitStatic(data.staticDetail, base);
	tb.staticDrawBack->InitStatic(data.staticDrawBack, base);
	tb.staticShips3D->InitStatic(data.staticShips3D, base);

	tb.dropDevice->InitDropdown(data.dropDevice, base);
	tb.dropDevice->SetControlID(DROP_DEVICES);

	tb.dropResolution->InitDropdown(data.dropResolution, base);
	
	tb.sliderGamma->InitSlider(data.sliderGamma, base);
	tb.sliderGamma->SetControlID(SLIDER_GAMMA);
	tb.sliderGamma->SetRangeMin(-9);
	tb.sliderGamma->SetRangeMax(20);

	tb.slideDrawBack->InitSlider(data.slideDrawBack, base);
	tb.slideDrawBack->SetControlID(SLIDER_DRAWBACK);
	tb.slideDrawBack->SetRangeMin(0);
	tb.slideDrawBack->SetRangeMax(2);

	tb.sliderShips3D->InitSlider(data.sliderShips3D, base);
	tb.sliderShips3D->SetControlID(SLIDER_SHIPS3D);
	tb.sliderShips3D->SetRangeMin(0);
	tb.sliderShips3D->SetRangeMax(4);

	tb.pushTrails->InitButton(data.pushTrails, base);
	tb.pushTrails->SetControlID(PUSH_TRAILS);

	tb.pushEmissive->InitButton(data.pushEmissive, base);
	tb.pushEmissive->SetControlID(PUSH_EMISSIVE);

	tb.pushDetail->InitButton(data.pushDetail, base);
	tb.pushDetail->SetControlID(PUSH_DETAIL);

	tb.static3DHardware->InitStatic(data.static3DHardware, base);
	tb.push3DHardware->InitButton(data.push3DHardware, base);
	tb.push3DHardware->SetControlID(PUSH_HARDWARE);

	//
	// the 3rd Tab (Sounds)
	tabControl->GetTabMenu(2, base);
  	tb.staticSound->InitStatic(data.staticSound, base);
	tb.staticMusic->InitStatic(data.staticMusic, base);
	tb.staticComm->InitStatic(data.staticComm, base);
	tb.staticChat->InitStatic(data.staticChat, base);

	tb.pushSound->InitButton(data.pushSound, base);
	tb.pushSound->SetControlID(PUSH_SFX);

	tb.sliderSound->InitSlider(data.sliderSound, base);
	tb.sliderSound->SetControlID(SLIDER_SFX);
	tb.sliderSound->SetRangeMin(0);
	tb.sliderSound->SetRangeMax(10);
	
	tb.pushMusic->InitButton(data.pushMusic, base);
	tb.pushMusic->SetControlID(PUSH_MUSIC);

	tb.sliderMusic->InitSlider(data.sliderMusic, base);
	tb.sliderMusic->SetControlID(SLIDER_MUSIC);
	tb.sliderMusic->SetRangeMin(0);
	tb.sliderMusic->SetRangeMax(10);

	tb.pushComm->InitButton(data.pushComm, base);
	tb.pushComm->SetControlID(PUSH_COMM);

	tb.sliderComm->InitSlider(data.sliderComm, base);
	tb.sliderComm->SetControlID(SLIDER_COMM);
	tb.sliderComm->SetRangeMin(0);
	tb.sliderComm->SetRangeMax(10);

	tb.sliderChat->InitSlider(data.sliderChat, base);
	tb.sliderChat->SetControlID(SLIDER_CHAT);
	tb.sliderChat->SetRangeMin(0);
	tb.sliderChat->SetRangeMax(10);

	buttonOk->InitButton(data.buttonOk, this);
	buttonCancel->InitButton(data.buttonCancel, this);

	// set up all the buddy buttons for our static controls
	tb.staticDInput->SetBuddyControl(tb.pushDInput);
	tb.staticStatus->SetBuddyControl(tb.pushStatus);
	tb.staticRollover->SetBuddyControl(tb.pushRollover);
	tb.staticSectorMap->SetBuddyControl(tb.pushSectorMap);
	tb.staticRightClick->SetBuddyControl(tb.pushRightClick);
	tb.staticSubtitles->SetBuddyControl(tb.pushSubtitles);

	tb.staticTrails->SetBuddyControl(tb.pushTrails);
	tb.staticEmissive->SetBuddyControl(tb.pushEmissive);
	tb.staticDetail->SetBuddyControl(tb.pushDetail);
	tb.static3DHardware->SetBuddyControl(tb.push3DHardware);
	
	tb.staticSound->SetBuddyControl(tb.pushSound);
	tb.staticMusic->SetBuddyControl(tb.pushMusic);
	tb.staticComm->SetBuddyControl(tb.pushComm);

	// keep track of the current and default user settings
	// god dammit this design sucks major ass!
	resetDefaults(defaultInfo);
	currentInfo = defaultInfo;

	// fill all of the devices into the drop box
	fillDeviceList(tb.dropDevice, tb.dropResolution);

	// if no 3D card was found, than force software mode and disable the appropriate controls
	if (b3DCardFound == false)
	{
		tb.push3DHardware->EnableButton(false);
		tb.dropDevice->EnableDropdown(false);
		tb.dropResolution->EnableDropdown(false);
	}
	else
	{
		// go by our default setting.  A 3D card may be available, but the user may be partial to software mode
		if (DEFAULTS->GetDefaults()->bHardwareRender == true)
		{
			onButton3DHardware();
		}
		else
		{
			// act as if we hit the enable 3D hardware button twice
			// once to enable 3D mode and sync up the dropdown boxes with the push box
			// and a second time to switch the hardware off (now that the dropdown boxes are n'sync
			onButton3DHardware();
			onButton3DHardware();
		}
	}

	// put all of the names into the list
	initNameList();

	if (CQFLAGS.bGameActive)
	{
		// cannot set the multitexture flag if our card doesn't support it
		if (CQRENDERFLAGS.bMultiTexture == FALSE)
		{
			tb.pushEmissive->EnableButton(false);
		}

		tb.pushDetail->EnableButton(false);

		tb.listNames->EnableListbox(false);
		tb.buttonNew->EnableButton(false);
		tb.buttonChange->EnableButton(false);
		tb.buttonDelete->EnableButton(false);

		tb.dropDevice->EnableDropdown(false);
		tb.dropResolution->EnableDropdown(false);
		tb.push3DHardware->EnableButton(false);

		if (PLAYERID!=0)
		{
			tb.sliderSpeed->EnableSlider(false);
		}
	}

	// set the default controls for the tabs
	tabControl->EnableKeyboardFocusing();

	if (CQFLAGS.bGameActive)
	{
		tabControl->SetDefaultControlForTab(0, tb.sliderMouse);
		tabControl->SetDefaultControlForTab(1, tb.sliderGamma);
	}
	else
	{
		tabControl->SetDefaultControlForTab(0, tb.listNames);
		tabControl->SetDefaultControlForTab(1, tb.dropDevice);
	}
	tabControl->SetDefaultControlForTab(2, tb.pushSound);

	setFocus(tabControl);
	tabControl->SetCurrentTab(0);
}
//--------------------------------------------------------------------------//
//
void Menu_options::init (void)
{
	data = 	*((GT_OPTIONS *) GENDATA->GetArchetypeData("MenuOptions"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	// tab contol stuff
	GENDATA->CreateInstance(data.tab.tabControlType, pComp);
	pComp->QueryInterface("ITabControl", tabControl);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticName);

	GENDATA->CreateInstance(data.listNames.listboxType, pComp);
	pComp->QueryInterface("IListbox", tb.listNames);

	GENDATA->CreateInstance(data.buttonNew.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.buttonNew);

	GENDATA->CreateInstance(data.buttonChange.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.buttonChange);

	GENDATA->CreateInstance(data.buttonDelete.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.buttonDelete);

	GENDATA->CreateInstance(data.staticDInput.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticDInput);

	GENDATA->CreateInstance(data.staticMouse.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticMouse);

	GENDATA->CreateInstance(data.staticSpeed.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticSpeed);

	GENDATA->CreateInstance(data.staticScroll.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticScroll);

	GENDATA->CreateInstance(data.staticStatus.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticStatus);

	GENDATA->CreateInstance(data.staticRollover.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticRollover);

	GENDATA->CreateInstance(data.staticSectorMap.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticSectorMap);

	GENDATA->CreateInstance(data.staticRightClick.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticRightClick);

	GENDATA->CreateInstance(data.staticSubtitles.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticSubtitles);

	GENDATA->CreateInstance(data.pushDInput.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushDInput);

	GENDATA->CreateInstance(data.sliderMouse.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderMouse);

	GENDATA->CreateInstance(data.sliderSpeed.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderSpeed);

	GENDATA->CreateInstance(data.sliderScroll.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderScroll);

	GENDATA->CreateInstance(data.pushStatus.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushStatus);

	GENDATA->CreateInstance(data.pushRollover.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushRollover);

	GENDATA->CreateInstance(data.pushSectorMap.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushSectorMap);

	GENDATA->CreateInstance(data.pushRightClick.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushRightClick);

	GENDATA->CreateInstance(data.pushSubtitles.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushSubtitles);

	GENDATA->CreateInstance(data.staticDevice.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticDevice);

	GENDATA->CreateInstance(data.staticResolution.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticResolution);

	GENDATA->CreateInstance(data.staticGamma.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticGamma);

	GENDATA->CreateInstance(data.staticTrails.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticTrails);

	GENDATA->CreateInstance(data.staticEmissive.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticEmissive);

	GENDATA->CreateInstance(data.staticDetail.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticDetail);

	GENDATA->CreateInstance(data.staticDrawBack.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticDrawBack);

	GENDATA->CreateInstance(data.staticShips3D.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticShips3D);

	GENDATA->CreateInstance(data.dropDevice.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", tb.dropDevice);

	GENDATA->CreateInstance(data.dropResolution.dropdownType, pComp);
	pComp->QueryInterface("IDropdown", tb.dropResolution);

	GENDATA->CreateInstance(data.sliderGamma.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderGamma);

	GENDATA->CreateInstance(data.pushTrails.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushTrails);

	GENDATA->CreateInstance(data.pushEmissive.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushEmissive);

	GENDATA->CreateInstance(data.pushDetail.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushDetail);

	GENDATA->CreateInstance(data.slideDrawBack.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.slideDrawBack);

	GENDATA->CreateInstance(data.sliderShips3D.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderShips3D);

	GENDATA->CreateInstance(data.staticSound.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticSound);

	GENDATA->CreateInstance(data.staticMusic.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticMusic);

	GENDATA->CreateInstance(data.staticComm.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticComm);

	GENDATA->CreateInstance(data.staticChat.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.staticChat);

	GENDATA->CreateInstance(data.pushSound.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushSound);
	
	GENDATA->CreateInstance(data.sliderSound.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderSound);

	GENDATA->CreateInstance(data.pushMusic.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushMusic);

	GENDATA->CreateInstance(data.sliderMusic.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderMusic);

	GENDATA->CreateInstance(data.pushComm.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.pushComm);

	GENDATA->CreateInstance(data.sliderComm.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderComm);

	GENDATA->CreateInstance(data.sliderChat.sliderType, pComp);
	pComp->QueryInterface("ISlider", tb.sliderChat);

	GENDATA->CreateInstance(data.buttonOk.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonOk);

	GENDATA->CreateInstance(data.buttonCancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonCancel);

	GENDATA->CreateInstance(data.static3DHardware.staticType, pComp);
	pComp->QueryInterface("IStatic", tb.static3DHardware);

	GENDATA->CreateInstance(data.push3DHardware.buttonType, pComp);
	pComp->QueryInterface("IButton2", tb.push3DHardware);

	DEFAULTS->StoreDefaults();		// store the current defaults
	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;
	desiredOwnedFlags = RF_CURSOR;
	grabAllResources();

	initFileSys();
}
//--------------------------------------------------------------------------//
//
void Menu_options::getGUIDFromIndex (int index, char * pGUID, U32 bufferSize)
{
	CQASSERT(index != -1);
	HANDLE hSection;
	char buffer[MAX_PATH];
	
	pGUID[0] = 0;
	int data = tb.dropDevice->GetDataValue(index);

	COMPTR<IProfileParser> parser;
	DACOM->QueryInterface(IID_IProfileParser, parser);

	sprintf(buffer, "Rend%d", data);
	if ((hSection = parser->CreateSection(buffer)) != 0)
	{
		parser->ReadKeyValue(hSection, "DeviceId", pGUID, bufferSize);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonOk (void)
{
	// set all of the graphic properties
	CQEFFECTS.bWeaponTrails = GlobalEffectsOptions::OPTVAL(tb.pushTrails->GetPushState());
	CQEFFECTS.bEmissiveTextures = GlobalEffectsOptions::OPTVAL(tb.pushEmissive->GetPushState());
	CQEFFECTS.bExpensiveTerrain = GlobalEffectsOptions::OPTVAL(tb.pushDetail->GetPushState());
	CQEFFECTS.nFlatShipScale = tb.sliderShips3D->GetPosition();

	CQEFFECTS.bBackground = GlobalEffectsOptions::OPTVAL(tb.slideDrawBack->GetPosition() > 0);
	CQEFFECTS.bHighBackground = GlobalEffectsOptions::OPTVAL(tb.slideDrawBack->GetPosition() > 1);

	USER_DEFAULTS * pUserDefaults = DEFAULTS->GetDefaults();

	// set the width, height, and bit depth variables for 3D Mode
	if (!CQFLAGS.bGameActive)
	{
		const int b3DHardware = tb.push3DHardware->GetPushState();

		U32 width;
		U32 height;
		U32 depth;

			//hardcoded solution for now, until enumeration is working again
/*		if (b3DHardware)
		{
			S32 index = tb.dropResolution->GetCurrentSelection();
			CQASSERT(index>=0);

			// get the data item from the selection
			U32 data = tb.dropResolution->GetDataValue(index);

			PackedRes  * resData = (PackedRes*)&data;

			width = resData->dwWidth;
			height = resData->dwHeight;
			depth = resData->dwDepth;

			// get the guid and save it off
			char buffer[MAX_PATH];
			getGUIDFromIndex(tb.dropDevice->GetCurrentSelection(), buffer, sizeof(buffer));
			DEFAULTS->SetStringInRegistry(RENDERDEV_REG_KEY, buffer);

			pUserDefaults->bHardwareRender = true;

			
		}
		else
*/		{
			width = 1024;
			height = 768;
			depth = 32;

			pUserDefaults->bHardwareRender = true;
		}

		Set3DVarialbes(width, height, depth);

		// right the vars to the registry
		DEFAULTS->SetDataInRegistry(WIDTH_REG_KEY, &width, sizeof(U32));
		DEFAULTS->SetDataInRegistry(HEIGHT_REG_KEY, &height, sizeof(U32));
		DEFAULTS->SetDataInRegistry(DEPTH_REG_KEY, &depth, sizeof(U32));
	}

	// set the game speed, cursor speed, and scroll rate
	pUserDefaults->gameSpeed = tb.sliderSpeed->GetPosition();
	pUserDefaults->mouseSpeed = tb.sliderMouse->GetPosition();
	pUserDefaults->scrollRate = SINGLE(tb.sliderScroll->GetPosition()+1) * 0.5f;

	// get the in game settings
	pUserDefaults->bNoStatusBar = !tb.pushStatus->GetPushState();
	pUserDefaults->bSectormapRotates = tb.pushSectorMap->GetPushState();
	pUserDefaults->bNoHints = !tb.pushRollover->GetPushState();
	pUserDefaults->bRightClickOption = tb.pushRightClick->GetPushState();
	pUserDefaults->bSubtitles = tb.pushSubtitles->GetPushState();
	pUserDefaults->bHardwareCursor = !tb.pushDInput->GetPushState();

	// delete all of the names that were marked for deletion
	// delete their folders and delete them from the registry
	// note - order is important here!!
	deleteMarkedNames();
	changeMarkedFolders();
	addMarkedFolders();

	// select the player
	tb.listNames->SetCurrentSelection(tb.listNames->GetCurrentSelection());

	DEFAULTS->StoreDefaults();		// store the current defaults
	MENU->InitPreferences();

	endDialog(1);
	setFocus(tabControl);
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonCancel (void)
{
	// redo the name list with only the current (ie. default) list
	tb.listNames->ResetContent();
	NameList * p = listCurrent;
	
	// add everything in the current list
	while (p)
	{
		tb.listNames->AddStringToHead(p->szName);
		p = p->pNext;
	}

	// reset to the default player name
	int selection = tb.listNames->FindStringExact(szDefaultName);
	if (selection != -1)
	{
		tb.listNames->SetCurrentSelection(selection);
	}

	USER_DEFAULTS * pUserDefaults = DEFAULTS->GetDefaults();

	// reset the gamma value back to the default
	COMPTR<IGammaControl> gamma;
	GS->QueryInterface(IID_IGammaControl, gamma);
	
	pUserDefaults->gammaCorrection = defaultInfo.nDefaultGamma;
	SINGLE gamma_value = SINGLE(defaultInfo.nDefaultGamma + 10)/SINGLE(10);
	gamma->set_gamma_function(IGC_ALL, gamma_value, 0, 1.0, 0);

	// reset all of the sound controls back to their defaults
	SOUNDMANAGER->SetSfxVolumeLevel(defaultInfo.nDefaultEffects, defaultInfo.bDefaultEffectsMuted);
	SOUNDMANAGER->SetMusicVolumeLevel(defaultInfo.nDefaultMusic, defaultInfo.bDefaultMusicMuted);
	SOUNDMANAGER->SetCommVolumeLevel(defaultInfo.nDefaultComms, defaultInfo.bDefaultCommsMuted);
	SOUNDMANAGER->SetChatVolumeLevel(defaultInfo.nDefaultNetChat, defaultInfo.bDefaultNetChatMuted);

	pUserDefaults->bHardwareCursor = !defaultInfo.bDefaultDInput;

	CURSOR->SetCursorSpeed(defaultInfo.nDefaultMouse);
	MENU->InitPreferences();

	endDialog(0);
	setFocus(tabControl);
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonNew (void)
{
	wchar_t newName[MAX_PLAYER_CHARS];
	U32 result;

	Notify(CQE_KILL_FOCUS, 0);
	result = DoMenu_newplayer(this, NULL, newName, sizeof(newName)/sizeof(wchar_t));
	Notify(CQE_SET_FOCUS, 0);

	const bool bCurrent = item_in_list(listCurrent, newName);
	const bool bAdd		= item_in_list(listAdd, newName);
	const bool bDelete  = item_in_list(listDelete, newName);
	const bool bChange  = item_in_list(listChange, newName, true);

	// if the result was a duplicate, than check if it is part of the delete list before we inform the user
	// in this case, take the name off of the delete list
	if (result)
	{
		if (bCurrent)
		{
			if (bDelete == false && bChange == false)
			{
				CQMessageBox(IDS_PLAYER_EXISTS, IDS_ERROR, MB_OK);
				return;
			}
		}

		// don't add it twice to our add list
		if (bAdd == false)
		{
			add_name_to_list(listAdd, newName, NULL);
			redoNameList();

			if (tb.listNames->GetNumberOfItems())
			{
				U32 sel = tb.listNames->FindStringExact(newName);
				if (sel != -1)
				{
					tb.listNames->SetCurrentSelection(sel);
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonDelete (void)
{
	// if we are trying to delete the last guy, then tell the user they have to change the name instead
	if (tb.listNames->GetNumberOfItems() == 1)
	{
		CQMessageBox(IDS_CANT_DELETE, IDS_APP_NAMETM, MB_OK, this);
		return;
	}

	S32 sel = tb.listNames->GetCurrentSelection();
	if (sel == -1)
	{
		return;
	}

	COMPTR<IFileSystem> file;
	wchar_t nameWide[MAX_CHARS-20];

	tb.listNames->GetString(sel, nameWide, (MAX_CHARS-20)*sizeof(wchar_t));

	// check if we really want to do this
	wchar_t ask[MAX_CHARS];
	swprintf(ask, _localLoadStringW(IDS_ASK_DELETE), nameWide);
	if (CQMessageBox(ask, IDS_HELP_DELETE, MB_OKCANCEL, this) == 0)
	{
		return;
	}

	// add the currently selected player to the delete list
	add_name_to_list(listDelete, nameWide);

	// delete the name from the add list if it is in there
	delete_item_from_list(listAdd, nameWide);

	redoNameList();

	// force an update of the current selection
	tb.listNames->SetCurrentSelection(0);
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonChange (void)
{
	wchar_t nameOld[MAX_PLAYER_CHARS];
	wchar_t nameNew[MAX_PLAYER_CHARS];
	U32 sel = tb.listNames->GetCurrentSelection();
	U32 result;

	if (sel == -1)
	{
		return;
	}

	tb.listNames->GetString(sel, nameOld, sizeof(nameOld));

	Notify(CQE_KILL_FOCUS, 0);
	result = DoMenu_newplayer(this, nameOld, nameNew, sizeof(nameNew)/sizeof(wchar_t));
	Notify(CQE_SET_FOCUS, 0);

	if (result)
	{
		const bool bCurrent		= item_in_list(listCurrent, nameNew);
		const bool bAdd			= item_in_list(listAdd, nameNew);
		const bool bDelete		= item_in_list(listDelete, nameNew);
		const bool bChange		= item_in_list(listChange, nameNew);
		const bool bChangeOld	= item_in_list(listChange, nameNew, true); 

		// the name can be our current list as long as it's in our delete list...
		if (bCurrent && (bDelete || bChangeOld))
		{
		}
		else
		{
			// do not add the name to the list if it already exits in any of our lists
			if (bAdd || bChange || bCurrent)
			{
				CQMessageBox(IDS_PLAYER_EXISTS, IDS_ERROR, MB_OK);
				return;
			}
		}

		add_name_to_list(listChange, nameNew, nameOld);
		redoNameList();
		sel = tb.listNames->FindStringExact(nameNew);
		tb.listNames->SetCurrentSelection(sel);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::initFileSys (void)
{
	if (fileSys == NULL)
	{
		DAFILEDESC fdesc;
		fdesc = "SavedGame";
		DACOM->CreateInstance(&fdesc, (void**)&fileSys);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::redoNameList (void)
{
	tb.listNames->ResetContent();

	// only the first time through...
	if (listCurrent == NULL)
	{
		// we need to look for a bunch of folders
		HANDLE handle;
		WIN32_FIND_DATA data;
		data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;

		initFileSys();

		// add the folder to the single player dir
		if (fileSys == NULL || (handle = fileSys->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		{
			return;
		}
		addFolder(handle);
		addFolder(data);
		fileSys->FindClose(handle);
	}

	NameList * p = listCurrent;
	
	// add everything in the current list
	while (p)
	{
		tb.listNames->AddString(p->szName);
		p = p->pNext;
	}

	// change everything that is in the change list
	p = listChange;
	while (p)
	{
		int selection = tb.listNames->FindStringExact(p->szOldName);
		if (selection != -1)
		{
			tb.listNames->SetString(selection, p->szName);
		}
		p = p->pNext;
	}

	// remove everything that is in the delete list
	p = listDelete;
	while (p)
	{
		int selection = tb.listNames->FindStringExact(p->szName);
		if (selection != -1)
		{
			tb.listNames->RemoveString(selection);
		}

		p = p->pNext;
	}

	// add everything from the add list
	p = listAdd;
	while (p)
	{
		tb.listNames->AddStringToHead(p->szName);
		p = p->pNext;
	}

	if (tb.listNames->GetNumberOfItems() > 0)
	{
		tb.buttonDelete->EnableButton(true);
		tb.buttonChange->EnableButton(true);
		buttonOk->EnableButton(true);
	}
	else
	{
		tb.buttonDelete->EnableButton(false);
		tb.buttonChange->EnableButton(false);
		buttonOk->EnableButton(false);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::initNameList (void)
{
	redoNameList();

	if (tb.listNames->GetNumberOfItems())
	{
		// set the current selection to what player is in the registry
		char nameAnsi[MAX_PLAYER_CHARS];
		U32 result;
	
		result = DEFAULTS->GetStringFromRegistry(NAME_REG_KEY, nameAnsi, sizeof(nameAnsi));
		if (result && nameAnsi[0])
		{
			wchar_t nameWide[MAX_PLAYER_CHARS];
			int sel;

			// save off the old name...
			wcsncpy(szDefaultName, nameWide, sizeof(szDefaultName)/sizeof(wchar_t));

			_localAnsiToWide(nameAnsi, nameWide, sizeof(nameWide));
			sel = tb.listNames->FindStringExact(nameWide);
			if (sel > -1)
			{
				tb.listNames->SetCurrentSelection(sel);
			}
			else
			{
				tb.listNames->SetCurrentSelection(0);
			}
		}

//		onListCaretMove(LIST_PLAYERS);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onListCaretMove (S32 listID)
{
	if (listID == LIST_PLAYERS)
	{
		wchar_t playerPlace[64];
		wcsncpy(playerPlace, _localLoadStringW(IDS_ENTER_PLAYER_NAME), sizeof(playerPlace)/sizeof(wchar_t));
		if (tb.listNames->GetNumberOfItems() && tb.listNames->GetCurrentSelection() != -1)
		{
			wchar_t playerName[MAX_PLAYER_CHARS];
			wchar_t fullString[256];

			tb.listNames->GetString(tb.listNames->GetCurrentSelection(), playerName, sizeof(playerName));

			swprintf(fullString, L"%s  %s", playerPlace, playerName);
			tb.staticName->SetText(fullString);		

			// place the name into the registry
			char nameAnsi[MAX_PLAYER_CHARS];
			_localWideToAnsi(playerName, nameAnsi, sizeof(nameAnsi));
			DEFAULTS->SetStringInRegistry(NAME_REG_KEY, nameAnsi);
			
			resetDefaults(currentInfo);
		}
		else
		{
			tb.staticName->SetText(playerPlace);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onListSelection (S32 listID)
{
	if (listID == DROP_DEVICES)
	{
		// get the guid and re-fill the resolution listbox
		int sel = tb.dropDevice->GetCurrentSelection();
		CQASSERT(sel>=0);

		char buffer[MAX_PATH];
		getGUIDFromIndex(sel, buffer, sizeof(buffer));
		fillResolutionList(buffer, tb.dropResolution, &ident2);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onSliderPressed (U32 sliderID)
{
	switch (sliderID)
	{
	case SLIDER_GAMMA:
		{
			COMPTR<IGammaControl> gamma;
			GS->QueryInterface(IID_IGammaControl, gamma);

			S32 value = tb.sliderGamma->GetPosition();
			SINGLE gamma_value = SINGLE(value + 10)/SINGLE(10);

			gamma->set_gamma_function(IGC_ALL, gamma_value, 0, 1.0, 0);
			DEFAULTS->GetDefaults()->gammaCorrection = value;
		}
		break;

	case SLIDER_SFX:
		SOUNDMANAGER->SetSfxVolumeLevel(tb.sliderSound->GetPosition(), !tb.pushSound->GetPushState());
		break;

	case SLIDER_MUSIC:
		SOUNDMANAGER->SetMusicVolumeLevel(tb.sliderMusic->GetPosition(), !tb.pushMusic->GetPushState());
		break;

	case SLIDER_COMM:
		SOUNDMANAGER->SetCommVolumeLevel(tb.sliderComm->GetPosition(), !tb.pushComm->GetPushState());
		break;

	case SLIDER_CHAT:
		SOUNDMANAGER->SetChatVolumeLevel(tb.sliderChat->GetPosition(), true);//!tb.pushChat->GetPushState());
		break;

	case SLIDER_MOUSE:
		CURSOR->SetCursorSpeed(tb.sliderMouse->GetPosition());
		break;

	default:
		break;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButtonPressed (U32 buttonID)
{
	switch (buttonID)
	{
	case IDS_OK:
		// put all of the settings into effect
		onButtonOk();
		break;

	case IDS_CANCEL:
		// close the menu and reset all the settings
		onButtonCancel();
		break;

	case IDS_NEW_PLAYER:
		onButtonNew();
		break;

	case IDS_DELETE_PLAYER:
		onButtonDelete();
		break;

	case IDS_CHANGE_NAME:
		onButtonChange();
		break;

	case PUSH_TRAILS:
		doPushButton(tb.pushTrails);
		break;
	
	case PUSH_EMISSIVE:
		doPushButton(tb.pushEmissive);
		break;

	case PUSH_DETAIL:
		doPushButton(tb.pushDetail);
		break;

	case PUSH_HARDWARE:
		onButton3DHardware();
		break;

	case PUSH_SFX:
		checkState(tb.pushSound, tb.sliderSound);
		SOUNDMANAGER->SetSfxVolumeLevel(tb.sliderSound->GetPosition(), !tb.pushSound->GetPushState());
		break;

	case PUSH_MUSIC:
		checkState(tb.pushMusic, tb.sliderMusic);
		SOUNDMANAGER->SetMusicVolumeLevel(tb.sliderMusic->GetPosition(), !tb.pushMusic->GetPushState());
		break;

	case PUSH_COMM:
		checkState(tb.pushComm, tb.sliderComm);
		SOUNDMANAGER->SetCommVolumeLevel(tb.sliderComm->GetPosition(), !tb.pushComm->GetPushState());
		break;

	case PUSH_STATUS:
		doPushButton(tb.pushStatus);
		break;

	case PUSH_MAP:
		doPushButton(tb.pushSectorMap);
		break;

	case PUSH_ROLLOVER:
		doPushButton(tb.pushRollover);
		break;

	case PUSH_RIGHTCLICK:
		doPushButton(tb.pushRightClick);
		break;

	case PUSH_SUBTITLES:
		doPushButton(tb.pushSubtitles);
		break;

	case PUSH_DINPUT:
		doPushButton(tb.pushDInput);
		tb.sliderMouse->EnableSlider(tb.pushDInput->GetPushState());
		currentInfo.bDefaultDInput = !tb.pushDInput->GetPushState();
		DEFAULTS->GetDefaults()->bHardwareCursor = !tb.pushDInput->GetPushState();
		break;

	default:
		break;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_options::onButton3DHardware (void)
{
	// toggle the push button
	doPushButton(tb.push3DHardware);

	// if hardware mode is disabled, than we can't monkey around with the device and resolution drop boxes
	const bool b3DEnabled = tb.push3DHardware->GetPushState();

	tb.dropDevice->EnableDropdown(b3DEnabled);
	tb.dropResolution->EnableDropdown(b3DEnabled);
}
//--------------------------------------------------------------------------//
//
HRESULT Menu_options::resCallback (struct _DDSURFACEDESC2 * ddesc, LPVOID lpContext)
{
	ResEnum * pResEnum = (ResEnum *) lpContext;

	unsigned int maxBitDepth=16;

extern bool bSupports32Bit;

	if (bSupports32Bit)
		maxBitDepth=32;

	if ((ddesc->dwFlags & (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT)) == (DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT))
	{
		if ((CQFLAGS.bLimitResolutions==0 || (ddesc->dwWidth >= 640 && ddesc->dwHeight >= 480)) && ddesc->ddpfPixelFormat.dwRGBBitCount >= 16)
		{
			if (CQFLAGS.bLimitResolutions==0 || (ddesc->dwWidth <= 1024 && ddesc->dwHeight <= 768 && ddesc->ddpfPixelFormat.dwRGBBitCount <= maxBitDepth))
			{
				// limit the resolution for Voodoo2 cards
				if (pResEnum->ident2->dwVendorId != 0x121A || pResEnum->ident2->dwDeviceId != 0x0002 || ddesc->dwWidth <= 800)
				{
					if (pResEnum->list)
					{
						wchar_t buffer[64];
						swprintf(buffer, L"%ux%ux%u", ddesc->dwWidth, ddesc->dwHeight, ddesc->ddpfPixelFormat.dwRGBBitCount);
						PackedRes packedRes;
						packedRes.dwWidth = ddesc->dwWidth;
						packedRes.dwHeight = ddesc->dwHeight;
						packedRes.dwDepth = ddesc->ddpfPixelFormat.dwRGBBitCount;
						U32 index = pResEnum->list->AddString(buffer);
						pResEnum->list->SetDataValue(index,  packedRes);

						// does this resolution match the defaults?
						if (pResEnum->regWidth == ddesc->dwWidth && pResEnum->regHeight == ddesc->dwHeight && pResEnum->regDepth == ddesc->ddpfPixelFormat.dwRGBBitCount)
						{
							pResEnum->regIndex = index;
						}
					}
		
					pResEnum->numModes++;
				}
			}
		}
	}

	return DDENUMRET_OK;
//	return DDENUMRET_CANCEL;		// testing!!!
}
//--------------------------------------------------------------------------//
//
int Menu_options::fillResolutionList (const char * pGUID, IListbox * list, DDDEVICEIDENTIFIER2 * ident2)
{
	ResEnum resEnum;
	memset(&resEnum, 0, sizeof(resEnum));

	if (list)
	{
		list->ResetContent();
		DEFAULTS->GetDataFromRegistry(WIDTH_REG_KEY, &resEnum.regWidth, sizeof(U32));
		DEFAULTS->GetDataFromRegistry(HEIGHT_REG_KEY, &resEnum.regHeight, sizeof(U32));
		DEFAULTS->GetDataFromRegistry(DEPTH_REG_KEY, &resEnum.regDepth, sizeof(U32));
		resEnum.list = list;
	}

	resEnum.ident2 = ident2;

	//EnumVideoModes(pGUID, resCallback, &resEnum);

	if (list)
	{
		list->SetCurrentSelection(resEnum.regIndex);
		CQASSERT(resEnum.numModes==0 || list->GetCurrentSelection() >= 0);
	}

	return resEnum.numModes;
}
//--------------------------------------------------------------------------//
//
int Menu_options::fillDeviceList (IListbox * devlist, IListbox * resList)
{
	HANDLE hSection;
	COMPTR<IProfileParser> parser;
	int result = 0;
	bool bPrimary=false;
	char buffer[MAX_PATH];
	char regValue[MAX_PATH];
	wchar_t tmp[MAX_PATH];
	S32 regIndex = -1;
	int i;

	devlist->ResetContent();
	resList->ResetContent();

	if (DEFAULTS->GetStringFromRegistry(RENDERDEV_REG_KEY, regValue, sizeof(regValue)) == 0)
		strcpy(regValue, "{00000000-0000-0000-0000-000000000000}");

	DACOM->QueryInterface(IID_IProfileParser, parser);

	for (i = 0; i < 4; i++)
	{
		sprintf(buffer, "Rend%d", i);
		if ((hSection = parser->CreateSection(buffer)) != 0)
		{
			if (parser->ReadKeyValue(hSection, "DeviceId", buffer, sizeof(buffer)) != 0)
			{
				bool bIsNull = isNullGUID(buffer);
				if (bPrimary==false || bIsNull==false)
				{
					if (bPrimary==false)
						bPrimary = bIsNull;

					// is this device fully capable to play our game in 3D?
					if (TestDeviceFor3D(buffer))
					{
						b3DCardFound = true;
					}

//					if (GetDisplayDeviceIDFromGUID(buffer, &ident2) == DD_OK)
					{
						CQTRACE13("CONFIG INFO: Render Device: %s, VendorID=0x%X, DeviceID=0x%X", 
							ident2.szDescription, ident2.dwVendorId, ident2.dwDeviceId);
						if (fillResolutionList(buffer, NULL, &ident2))
						{
							_localAnsiToWide(ident2.szDescription, tmp, sizeof(tmp));

							// don't add the string if it's already in the listbox
							if (devlist->FindStringExact(tmp) == -1)
							{
								U32 index = devlist->AddString(tmp);
								devlist->SetDataValue(index, i);

								result++;
								if (strcmp(buffer, regValue) == 0)		// guid matches regkey
								{
									regIndex = index;
								}
							}
						}
					}
				}
			}

			parser->CloseSection(hSection);
		}
	}

	if (result)
	{
		if (regIndex>=0)
			devlist->SetCurrentSelection(regIndex);
		else
			devlist->SetCurrentSelection(result-1);

		onListSelection(DROP_DEVICES);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
U32 __stdcall CreateOptionsMenu (const bool bFocusing)
{
	static U8 recurse;

	if (recurse==0)		// might be called recursively because cheat hackers
	{
		recurse++;
		Menu_options * menu = new Menu_options;
		menu->createViewer("\\GT_OPTIONS\\MenuOptions", "GT_OPTIONS", IDS_VIEWOPTIONS);
		menu->beginModalFocus();

		const BOOL32 bOldPause = CQFLAGS.bGamePaused;

		if (bFocusing)
		{
			// pause the game if in single player mode
			if (PLAYERID == 0)
			{
				CQFLAGS.bGamePaused = TRUE;
				EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
			}
		}

		U32 result = CQDoModal(menu);
		delete menu;

		if (bFocusing)
		{
			// unpause the game, if we must
			if (PLAYERID == 0)
			{
				CQFLAGS.bGamePaused = bOldPause;
				EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
			}
		}

		recurse--;
		return result;
	}

	return 0;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_options.cpp-------------------------//
//--------------------------------------------------------------------------//