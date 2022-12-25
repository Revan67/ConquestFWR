//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_mshell.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_mshell.cpp 172   10/18/02 2:36p Tmauer $
*/
//--------------------------------------------------------------------------//
// Multiplayer shell
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IStatic.h"
#include "IButton2.h"
#include <DMenu1.h>
#include "CQGame.h"
#include "GRPackets.h"
#include "Mission.h"
#include "IEdit2.h"
#include "IListbox.h"
#include "NetConnectBuffers.h"
#include "NetBuffer.h"
#include "OpAgent.h"
#include "MusicManager.h"

#include <stdio.h>
#include <dplay.h>
#include <dplobby.h>
#include "zonelobby.h"

#define CHATMSGSIZE 128

#define EDIT_CHAT_ID 1790023

void  __stdcall DoMenu_final (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, bool _bInternet);
U32 __stdcall DoMenu_nl (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, U32 checkSum, U32 randomSeed);

using namespace CQGAMETYPES;

//--------------------------------------------------------------------------//
//
struct CQGAME_PACKET : GR_PACKET, CQGAME
{
	CQGAME_PACKET (void) : GR_PACKET(GAMEDESC)
	{
		dwSize = sizeof(*this);
	}

	CQGAME_PACKET & operator = (const CQGAME & cpy)
	{
		*static_cast<CQGAME *>(this) = cpy;
		return *this;
	}
};
//--------------------------------------------------------------------------//
//
struct MAP_PACKET : GR_PACKET
{
	wchar_t szMapName[MAPNAMESIZE];

	MAP_PACKET (void) : GR_PACKET(MAPNAME)
	{
		dwSize = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
//
struct CLIENTSETTING_PACKET : GR_PACKET
{
	STATE state;
	RACE race;
	COLOR color;
	TEAM team;

	CLIENTSETTING_PACKET (void) : GR_PACKET(CLIENT)
	{
		dwSize = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
//
struct GRCHAT_PACKET : GR_PACKET
{
	wchar_t szMsg[CHATMSGSIZE];

	GRCHAT_PACKET (void) : GR_PACKET(GRCHAT)
	{
		dwSize = sizeof(*this);
	}
};
//--------------------------------------------------------------------------//
//
struct Menu_mshell : public DAComponent<Frame>, ICQGame
{
	//
	// data items
	//
	const GT_MENU1::MSHELL & data;
	const GT_MENU1 & menu1;
	const SAVED_CONNECTION * conn;
	const wchar_t * const szPlayerName;
	const wchar_t * const szSessionName;

	bool bTCP, bLAN, bZone;

	COMPTR<IStatic> background;
	COMPTR<IStatic> title;
	COMPTR<IStatic> ipaddress;
//	COMPTR<IStatic> session, sessionName;
	COMPTR<IStatic> enterChat; // , chatBox;
	COMPTR<IEdit2> editChat;
	COMPTR<IListbox> listChat;

	//
	// data related to multithreading
	//
	volatile bool bThreadCancel;		// request to kill thread (set by default thread)
	volatile bool bMessageReceived;		// true if background thread has posted a message
	volatile bool bUpdateHandled;		// set true every time update is handled
	HANDLE hThread;

	enum SUBMENU
	{
		EDITGAME,
		FINAL
	} submenu;

	//
	// variables related to state changes in setup screens
	//
	bool bLocalDataChanged;		// send needed (only used for the host)
	bool bMapNameChanged;		// resend map name needed
	bool bOutgoingData;			// true if we need to send outgoing (client) data
	bool bLoadInProgress;
	bool bDownloadEnabled;		// host has said "go to next screen, please"

	U32  remoteCheckSum;		// valid after receiving STARTDOWNLOAD packet
	U32  remoteRandomSeed;		// valid after receiving STARTDOWNLOAD packet
	S32 timer;

	Frame * finalFrame;
	Frame * mapFrame;

	U32 readyCheckTime;		

	//
	// instance methods
	//

	Menu_mshell (Frame * _parent, const GT_MENU1 & _data, const SAVED_CONNECTION * _conn, const wchar_t * _szPlayerName, const wchar_t * _szSessionName, bool _bLAN, bool _bZone) : data(_data.mshell), 
																				menu1(_data),
																				conn(_conn),
																				szPlayerName(_szPlayerName),
																				szSessionName(_szSessionName),
																				bTCP(_conn && IsEqualGUID(_conn->guidSP,DPSPGUID_TCPIP)!=0),
																				bLAN(_bLAN),
																				bZone(_bZone)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_mshell (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* Frame overrides */

	virtual void onSetDefaultFocus (bool bFromPrevious)
	{
		// if the edit box is disabled, than move focus on to menu_final
		if (editChat->GetVisible() == false)
		{
			if (bFromPrevious)
			{
				if (finalFrame)
				{
					finalFrame->onSetDefaultFocus(true);
				}
			}
			else
			{
				if (mapFrame)
				{
					mapFrame->onSetDefaultFocus(false);
				}
			}

			return;
		}

		setFocus(editChat);
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
			if (finalFrame)
			{
				finalFrame->onSetDefaultFocus(true);
				return true;
			}
		}

		// switch focus to the next control group (menu_final)
/*		if (finalFrame)
		{
			finalFrame->onSetDefaultFocus(true);
			return true;
		}
*/
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
			if (mapFrame)
			{
				mapFrame->setLastFocus();
				return true;
			}
		}
		return Frame::onShiftTabPressed();
	}


	/* ICQGame methods */

	virtual void SetMapName (const wchar_t * szName);

	virtual void SetType (U32 slot, TYPE type, bool bUpdate);

	virtual void SetCompChalange (U32 slot, CQGAMETYPES::COMP_CHALANGE compChalange, bool bUpdate);

	virtual void SetState (U32 slot, STATE state, bool bUpdate);

	virtual void SetRace (U32 slot, RACE race, bool bUpdate);

	virtual void SetColor (U32 slot, COLOR color, bool bUpdate);

	virtual void SetTeam (U32 slot, TEAM team, bool bUpdate);

	virtual void SetGameType (GAMETYPE type);

	virtual void SetGameSpeed (S8 speed);

	virtual void SetMoney (MONEY money);

	virtual void SetMapType (MAPTYPE type);

	virtual void SetMapTemplateType (CQGAMETYPES::RANDOM_TEMPLATE rndTemp);

	virtual void SetMapSize (MAPSIZE size);

	virtual void SetTerrain (TERRAIN terrain);

	virtual void SetUnits (STARTING_UNITS units);

	virtual void SetResourceRegen (BOOL32 bRegen);

	virtual void SetSpectatorState (BOOL32 bSpectatorsOn);

	virtual void SetLockDiplomacy (BOOL32 bLockOn);

	virtual void SetVisibility (VISIBILITYMODE fog);

	virtual void SetNumSystems (U32 _numSystems);

	virtual void SetHostBusy (BOOL32 _bHostBusy);

	virtual void SetCommandLimit (CQGAMETYPES::COMMANDLIMIT commandSetting);

	virtual void SetDifficulty (CQGAMETYPES::DIFFICULTY difficulty, bool bUpdate);

	virtual CQGAMETYPES::DIFFICULTY GetDifficulty (void);

	virtual void ForceUpdate (void);

	/* Menu_mshell methods */

	virtual GENRESULT __stdcall Notify (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_RESET_NETWORK_PERFORMANCE:
			NETBUFFER->SetMaxBandwidth((conn) ? conn->guidSP : DPSPGUID_TCPIP, bLAN);
			break;
		case CQE_ADDPLAYER:
			onAddPlayer(DPID(param));
			break;
		case CQE_DELETEPLAYER:
			onDeletePlayer(DPID(param));
			break;
		case CQE_NEWHOST:
			onNewHost();
			break;
		case CQE_DPLAY_MSGWAITING:
			bMessageReceived = true;		
			onLocalUpdate();
			break;
		case CQE_NETPACKET:
			onNetPacket((const BASE_PACKET *)param);
			break;

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

		case WM_ACTIVATEAPP:
			if (bLoadInProgress || bDownloadEnabled)
			{
				// eeeiiikkk! (load is already starting!!!)
				MSG * msg = (MSG *) param;
				if (msg->wParam == 0)	// if deactivating
				{
					if (HOSTID != PLAYERID)
					{
						STATE oldState = slot[localSlot].state;
						slot[localSlot].state = ACTIVE;		// don't do this during load process!
						bLocalDataChanged = true;		
						sendClientPacket(true);
						slot[localSlot].state = oldState;	// restore state so we don't get out of sync
					}
					else
						killLoadProcess();
				}
			}
			else
				SetState(localSlot, ACTIVE, true);
			break;

		} // end switch (message)


		return Frame::Notify(message, param);
	}


	virtual void onUpdate (U32 dt)
	{
		bUpdateHandled = true;
		if (HOSTID==PLAYERID)
			updateReadyCount(dt);
		onLocalUpdate();
	}

	virtual void setStateInfo (void);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		return true;     // might need some additional processing here
	}

	void onLocalUpdate (void);   	// do periodic work
	
	void onAddPlayer (DPID id);
	
	void onDeletePlayer (DPID id);
	
	void onNewHost (void);
	
	void onNetPacket (const BASE_PACKET *packet);
	
	void sendClientPacket (bool bAlwaysSend=false);
	
	void receiveRemoteDefaults (const USER_DEFAULTS & userDefaults);
	
	S32 findOpenSlot (void);

	void init (void);

	void startPollingThread (void);
	
	void stopPollingThread (void);
	
	void pollingThreadMain (void);
	
	void initcqgame (void);
	
	void clearAllReadyFlags (void);
	
	bool checkReadyState (void);
	
	void killLoadProcess (void);
	
	void sendChatMsg (void);
	
	void onReceiveChatPacket (const GRCHAT_PACKET *packet);
	
	void chatAnnouncePlayer (U32 dpid, bool bEntering);
	
	void setNamesForSlots (void);
	
	void recalculateJoinState (void);		// are there open slots? and not loading game?
	
	void addToChatBox (U32 dpid, const wchar_t * string);
	
	void updateChatColors (void);
	
	void uploadZoneInfo (void);

	void resetcqgame (U32 mversion);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case EDIT_CHAT_ID:		// edit control
			sendChatMsg();
			break;
		
		case IDS_CANCEL:
			endDialog(0);
			break;
		}
	}

	static DWORD WINAPI threadProc (LPVOID lpParameter)
	{
		struct Menu_mshell * _this = (Menu_mshell *) lpParameter;

		_this->pollingThreadMain();
		return 0;
	}

	void clearReady (void);

	S32 getSlot (DPID id)
	{
		S32 i;

		for (i = 0; i < S32(activeSlots); i++)
		{
			if (slot[i].dpid == id)
				return i;
		}

		return -1;
	}

	COLOR findUnusedColor (void)
	{
		COLOR color = static_cast<COLOR>(1);
		S32 i=0;

		while (i < S32(activeSlots))
		{
			if ((slot[i].state==ACTIVE||slot[i].state==READY) && slot[i].color == color)
			{
				color = static_cast<COLOR>(1 + color);
				i = 0;
				continue;
			}
			i++;
		}

		return color;
	}

	bool isAvailable (COLOR color, U32 row)
	{
		U32 i=0;

		while (i < activeSlots)
		{
			if (i != row && slot[i].color == color && (slot[i].state==ACTIVE||slot[i].state==READY))
			{
				return false;
			}
			i++;
		}

		return true;
	}

	static U32 getPlayerName (DPID id, wchar_t szName[PLAYERNAMESIZE])
	{
		//
		// get player name from the id
		//
		DWORD size = 0;
		szName[0] = 0;
		if (id)
			DPLAY->GetPlayerName(id, NULL, &size);
		if (size)
		{
			DPNAME * pName = (DPNAME *) malloc(size);
			if (DPLAY->GetPlayerName(id, pName, &size) == DP_OK)
				wcsncpy(szName, pName->lpszShortName, PLAYERNAMESIZE-1);
			::free(pName);
		}
		return wcslen(szName);
	}

	void enableJoin (bool bEnable)
	{
		if (DPLAY)
		{
			DPSESSIONDESC2 *pDesc;
			U32 size=0;
			HRESULT hr;

			CQASSERT(HOSTID==PLAYERID);

			DPLAY->GetSessionDesc(NULL, &size);
			pDesc = (DPSESSIONDESC2 *) malloc(size);
			hr = DPLAY->GetSessionDesc(pDesc, &size);
			CQASSERT(hr == DP_OK);
			if (bEnable)
				pDesc->dwFlags &= ~DPSESSION_JOINDISABLED;
			else
				pDesc->dwFlags |= DPSESSION_JOINDISABLED;
			DPLAY->SetSessionDesc(pDesc,0);
			free(pDesc);
		}
	}

	void updateSessDesc (void)
	{
		if (DPLAY)
		{
			DPSESSIONDESC2 *pDesc;
			U32 size=0;
			HRESULT hr;
			OPTIONS * pOptions;

			CQASSERT(HOSTID==PLAYERID);

			DPLAY->GetSessionDesc(NULL, &size);
			pDesc = (DPSESSIONDESC2 *) malloc(size);
			hr = DPLAY->GetSessionDesc(pDesc, &size);
			CQASSERT(hr == DP_OK);
			
			pOptions = (OPTIONS *) &(pDesc->dwUser1);
			CQASSERT(sizeof(OPTIONS) <= sizeof(DWORD)*2);

			if (*pOptions != *this)
			{
				*pOptions = *this;
				DPLAY->SetSessionDesc(pDesc,0);
			}
			free(pDesc);
		}
	}

	void updateReadyCount (U32 dt)
	{
		if (checkReadyState())
		{
			readyCheckTime+=dt;
			U32 oldCountdown = startCountdown;
			startCountdown = readyCheckTime / 1000;
			if (startCountdown != oldCountdown)
				bLocalDataChanged = true;
		}
	}
};
//----------------------------------------------------------------------------------//
//
Menu_mshell::~Menu_mshell (void)
{
	stopPollingThread();
}
//----------------------------------------------------------------------------------//
//
void Menu_mshell::updateChatColors (void)
{
	// update colors for chat box
	S32 i = listChat->GetNumberOfItems();
	while (i-- > 0)
	{	
		U32 dpid = listChat->GetDataValue(i);

		if (dpid)
		{
			S32 row = getSlot(dpid);

			if (row >= 0)
				listChat->SetColorValue(i, COLORTABLE[slot[row].color]);
			else
			{
				listChat->SetColorValue(i, COLORTABLE[0]);
				listChat->SetDataValue(i, 0);
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_mshell::setStateInfo (void)
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
	title->InitStatic(data.title, this);

//	session->InitStatic(data.session, this);
//	sessionName->InitStatic(data.sessionName, this);
	ipaddress->InitStatic(data.ipaddress, this);
	enterChat->InitStatic(data.enterChat, this);

	editChat->InitEdit(data.editChat, this);
	listChat->InitListbox(data.listChat, this);

//	sessionName->SetText(szSessionName); 

	if (PLAYERID!=0)
	{
		title->SetTextID(IDS_MULTI_PLAYER);
	}


	U32 i;
	for (i = 0; i < activeSlots; i++)
	{
		if (slot[i].dpid == PLAYERID)
		{
			localSlot = i;
			break;
		}
	}

	updateChatColors();

	if (childFrame == 0)
	{
		DoMenu_final(this, menu1, *this, !bLAN);
	}

	if (childFrame)
		childFrame->setStateInfo();

	// get a pointer to menu_final, this is your child frame
	// get a pointer to menu_map, this your child frame's child frame
	finalFrame = childFrame;
	mapFrame = childFrame->childFrame;

	// set the group behavior so we do the right tab thing
	setGroupBehavior();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);
	
	GENDATA->CreateInstance(data.ipaddress.staticType, pComp);
	pComp->QueryInterface("IStatic", ipaddress);

	GENDATA->CreateInstance(data.enterChat.staticType, pComp);
	pComp->QueryInterface("IStatic", enterChat);

	GENDATA->CreateInstance(data.editChat.editType, pComp);
	pComp->QueryInterface("IEdit2", editChat);

	GENDATA->CreateInstance(data.listChat.listboxType, pComp);
	pComp->QueryInterface("IListbox", listChat);

	editChat->SetControlID(EDIT_CHAT_ID);
	editChat->SetMaxChars(CHATMSGSIZE);
	editChat->EnableLockedTextBehavior();

	initcqgame();
	setStateInfo();
	startPollingThread();

	NETBUFFER->SetMaxBandwidth((conn) ? conn->guidSP : DPSPGUID_TCPIP, bLAN);

	if (!PLAYERID)
	{
		editChat->SetVisible(false);
		listChat->SetVisible(false);
		enterChat->SetVisible(false);
	}

	if (ZONESCORE)
	{
		ZONESCORE->SendGameState(DPLOBBY, ZSTATE_STARTSTAGING);
	}

	if (bTCP)
	{
		wchar_t bufferw[256];
		wchar_t ipbuffw[128];
		wchar_t ipbuffw2[128];
		ipbuffw2[0] = 0;
		if (GetHostIPAddress(ipbuffw, ipbuffw2, sizeof(ipbuffw)))
		{
			if(ipbuffw2[0])
			{
				swprintf(bufferw, L"%s  %s/%s", _localLoadStringW(IDS_STATIC_IPADDRESS), ipbuffw,ipbuffw2);
			}
			else
			{
				swprintf(bufferw, L"%s  %s", _localLoadStringW(IDS_STATIC_IPADDRESS), ipbuffw);
			}
			ipaddress->SetText(bufferw);
		}
	}

	MUSICMANAGER->PlayMusic("Multiplayer_menu.wav");
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::stopPollingThread (void)
{
	if (hThread)
	{
		bThreadCancel = true;
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread=0;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::startPollingThread (void)
{
	DWORD dwThreadID;

	CQASSERT(hThread==0);

	bThreadCancel = false;

	hThread = CreateThread(0,4096, threadProc, (LPVOID)this, 0, &dwThreadID);
	CQASSERT(hThread);
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::pollingThreadMain (void)
{
	bMessageReceived = true;

	while (bThreadCancel==false)
	{
		if (bUpdateHandled == false)
		{
			if (bMessageReceived)	// only post one message at a time
			{
				// don't post messages during load because of re-entrance issue that
				// can happen if a messagebox pops up during the load, or a minimize
				if (bLoadInProgress==false&&bDownloadEnabled==false)
				{
					bMessageReceived = false;
					::PostMessage(hMainWindow, CQE_DPLAY_MSGWAITING, 100, 0);
				}
			}
		}
		bUpdateHandled = false;
		Sleep(100);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::uploadZoneInfo (void)
{
	if (ZONESCORE)
	{
		char buffer[PLAYERNAMESIZE];
		int i, numPlayers=0;
		DWORD dwSeat = 0;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (slot[i].state == READY)
				numPlayers++;
		}

		ZONESCORE->Initialize(APPGUID_CONQUEST, numPlayers, DEFAULTS->GetDefaults()->bCheatsEnabled, 1, ZONESCORE_WINLOSSTIE);

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (slot[i].state == READY)
			{
				U32 flags = 0;
				if (slot[i].type == COMPUTER)
				{
					strncpy(buffer, _localLoadString(IDS_COMPUTER_NAME), sizeof(buffer));
					flags = ZONESCORE_COMPUTERPLAYER;
				}
				else
					_localWideToAnsi(szPlayerNames[i], buffer, sizeof(buffer));
				slot[i].zoneSeat = dwSeat;
				ZONESCORE->SetPlayer(dwSeat++, buffer, 0, slot[i].team, 0);
			}
		}

		ZONESCORE->SetGameOptions(0, "TODO: Make a Localized game description here!");
		ZONESCORE->SendGameOptions(DPLOBBY);
	}
	MISSION->SetInitialCheatState();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::resetcqgame (U32 mversion)
{
	memset(static_cast<CQGAME *>(this), 0, sizeof(CQGAME));
	version = mversion;
	gameType = KILL_PLATS_FABS;
	gameSpeed = 0;
	money = MEDIUM_MONEY;
	mapSize = MEDIUM_MAP;
	templateType = TEMPLATE_RANDOM;
	terrain = MEDIUM_TERRAIN;
	units = UNITS_MEDIUM;
	visibility = VISIBILITY_NORMAL;

	numSystems = 4;
	regenOn = true;
#ifdef _DEMO_
	activeSlots = MAX_PLAYERS/2;
#else
	activeSlots = (CQFLAGS.bLimitMaxPlayers) ? (MAX_PLAYERS/2) : MAX_PLAYERS;	
#endif

	localSlot = 0;
	slot[0].type = HUMAN;
	slot[0].state = ACTIVE;
	slot[0].race = TERRAN;
	slot[0].color = YELLOW;
	slot[0].team = _1;
	slot[0].dpid = PLAYERID;

	if (PLAYERID==0)			
		SetDifficulty(AVERAGE, false);
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::initcqgame (void)
{
	if (PLAYERID)
	{
		getPlayerName(PLAYERID, szPlayerNames[0]);
	}
	else
	{
		wcsncpy(szPlayerNames[0], szPlayerName, PLAYERNAMESIZE-1);
	}

	const U32 mversion = GetProductVersion();

#ifdef _DEMO_
	resetcqgame(mversion);
#else
	if (DEFAULTS->GetDataFromRegistry("LobbyOptions", static_cast<CQGAME *>(this), sizeof(CQGAME)) != sizeof(CQGAME) ||
		version != GetBuildVersion())
	{
		resetcqgame(mversion);
	}
	else
	{
		int i;

		// can we use the registry information?
		if (slot[0].state == READY && localSlot==0 && ((slot[0].dpid!=0) == (PLAYERID!=0)) && bZone==0)
		{
			slot[0].type = HUMAN;				// use registry, but remove old human players
			slot[0].state = ACTIVE;
			slot[0].dpid = PLAYERID;

			for (i = 1; i < MAX_PLAYERS; i++)
			{
				if (slot[i].state != READY || slot[i].type == HUMAN || slot[i].type == CLOSED)
				{
					slot[i].type = HUMAN;
					slot[i].state = OPEN;
					slot[i].race = TERRAN;
					slot[i].color = UNDEFINEDCOLOR;
					slot[i].team = NOTEAM;
				}
				slot[i].dpid = 0;
			}
		}
		else	// we can't use the registry
		{
			resetcqgame(mversion);
		}
	}
#endif

//demo forces 4 players
#ifndef _DEMO_
	if (CQFLAGS.bLimitMaxPlayers == 0)
		activeSlots = MAX_PLAYERS;	
	else	// are slots 4-8 in use? if not, force active slots back down to 4
	{
#endif
		int i;

		activeSlots = MAX_PLAYERS/2;	// assume we are setting it lower
		for (i = 4; i < MAX_PLAYERS; i++)
		{
			if (slot[i].state == READY || slot[i].state == ACTIVE)
			{
				activeSlots = MAX_PLAYERS;	// need to keep slots open, since they are in use!
				break;
			}
		}
#ifndef _DEMO_
	}
#endif

	version = mversion;
	bHostBusy = (PLAYERID==0);			// host is always busy in a skirmish game
	
	commandLimit = COMMAND_LOW;

	mapType = RANDOM_MAP;

	startCountdown = 0;

	if (HOSTID==PLAYERID)
	{
		gameSpeed = DEFAULTS->GetDefaults()->gameSpeed;
		updateSessDesc();
	}
}
//--------------------------------------------------------------------------//
// can only be set by host
//
void Menu_mshell::SetMapName (const wchar_t * szName)
{
	if (wcscmp(szName, szMapName))
	{
		wcsncpy(szMapName, szName, sizeof(szMapName)/sizeof(wchar_t));
		bMapNameChanged = true;		// resend map name needed
	}
}
//--------------------------------------------------------------------------//
// can only be set by host
//
void Menu_mshell::SetType (U32 row, TYPE type, bool bUpdate)
{
	slot[row].type = type;
	if (type == COMPUTER)
		slot[row].dpid = 0;
	bLocalDataChanged = true;		
	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetCompChalange (U32 row, CQGAMETYPES::COMP_CHALANGE compChalange, bool bUpdate)
{
	slot[row].compChalange = compChalange;
	bLocalDataChanged = true;		
	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetState (U32 row, STATE state, bool bUpdate)
{
	if (bDownloadEnabled==false)
	{
		if (slot[row].state != state)
		{
			slot[row].state = state;
			if (state == CLOSED || state == OPEN)
			{
				slot[row].type = HUMAN;
				slot[row].state = OPEN;
				slot[row].race = TERRAN;
				slot[row].color = UNDEFINEDCOLOR;
				slot[row].team = NOTEAM;
				slot[row].dpid = 0;
			}
			bLocalDataChanged = true;		
			if (HOSTID != PLAYERID)
				sendClientPacket();
			recalculateJoinState();
		}
		if (bUpdate)
			setStateInfo();
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetRace (U32 row, RACE race, bool bUpdate)
{
	if (slot[row].race != race)
	{
		slot[row].race = race;
		bLocalDataChanged = true;
		clearAllReadyFlags();
		if (HOSTID != PLAYERID)
			sendClientPacket();
	}
	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetColor (U32 row, COLOR color, bool bUpdate)
{
	if (slot[row].color != color)
	{
		slot[row].color = color;
		bLocalDataChanged = true;		
		updateChatColors();
		clearAllReadyFlags();
		if (HOSTID != PLAYERID)
			sendClientPacket();
	}
	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetTeam (U32 row, TEAM team, bool bUpdate)
{
	if (slot[row].team != team)
	{
		slot[row].team = team;
		bLocalDataChanged = true;		
		clearAllReadyFlags();
		if (HOSTID != PLAYERID)
			sendClientPacket();
	}
	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetGameType (GAMETYPE type)
{
	if (type != gameType)
	{
		gameType = type;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetGameSpeed (S8 speed)
{
	if (speed != gameSpeed)
	{
		gameSpeed = speed;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetMoney (MONEY _money)
{
	if (_money != money)
	{
		money = _money;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetMapType (MAPTYPE type)
{
	if (type != mapType)
	{
		mapType = type;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetMapTemplateType (CQGAMETYPES::RANDOM_TEMPLATE rndTemp)
{
	if(rndTemp != templateType)
	{
		templateType = rndTemp;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetMapSize (MAPSIZE size)
{
	if (size != mapSize)
	{
		mapSize = size;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetTerrain (TERRAIN _terrain)
{
	if (_terrain != terrain)
	{
		terrain = _terrain;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetUnits (STARTING_UNITS _units)
{
	if (_units != units)
	{
		units = _units;
		bLocalDataChanged = true;		
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetResourceRegen (BOOL32 _regen)
{
	if (U32(_regen) != regenOn)
	{
		regenOn = _regen;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetSpectatorState (BOOL32 _bSpectatorsOn)
{
	if (U32(_bSpectatorsOn) != spectatorsOn)
	{
		spectatorsOn = _bSpectatorsOn;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetLockDiplomacy (BOOL32 _bLockOn)
{
	if (U32(_bLockOn) != lockDiplomacyOn)
	{
		lockDiplomacyOn = _bLockOn;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetVisibility (CQGAMETYPES::VISIBILITYMODE _fog)
{
	if (_fog != visibility)
	{
		visibility = _fog;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetNumSystems (U32 _numSystems)
{
	if (_numSystems != numSystems)
	{
		numSystems = _numSystems;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetHostBusy (BOOL32 _bHostBusy)
{
	if (U32(_bHostBusy) != bHostBusy)
	{
		bHostBusy = _bHostBusy;
		bLocalDataChanged = true;
		clearAllReadyFlags();

		// really clear the ready flags in this case
		U32 i;
		for (i = 0; i < activeSlots; i++)
			if (slot[i].type == HUMAN && slot[i].state == READY)
				slot[i].state = ACTIVE;

		setStateInfo();
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::SetCommandLimit (CQGAMETYPES::COMMANDLIMIT commandSetting)
{
	if(commandSetting != commandLimit)
	{
		commandLimit = commandSetting;
		bLocalDataChanged = true;
	}
}
//--------------------------------------------------------------------------//
// only used in skirmish
//
void Menu_mshell::SetDifficulty (CQGAMETYPES::DIFFICULTY difficulty, bool bUpdate)
{
	CQASSERT(PLAYERID==0);

	switch (difficulty)
	{
	case NODIFFICULTY:
		break;

	case EASY:
		money = HIGH_MONEY;
		mapSize = MEDIUM_MAP;
		terrain = MEDIUM_TERRAIN;
		units = UNITS_LARGE;
		visibility = VISIBILITY_NORMAL;
		numSystems = 3;
		memset(&slot[1], 0, sizeof(slot)-sizeof(slot[0]));		// clear all rows except first row
		slot[0].type = HUMAN;
		slot[0].state = ACTIVE;
		slot[0].team = NOTEAM;
		slot[0].dpid = PLAYERID;

		slot[1].type = COMPUTER;
		slot[1].state = READY;
		slot[1].team = NOTEAM;
		slot[1].dpid = 0;
		switch (slot[0].race)		// assign computer player race+color
		{
		case TERRAN:
			slot[1].race = MANTIS;
			if (slot[0].color != RED)
				slot[1].color = RED;
			else
				slot[1].color = GREEN;
			break;
		case MANTIS:
			slot[1].race = SOLARIAN;
			if (slot[0].color != BLUE)
				slot[1].color = BLUE;
			else
				slot[1].color = PINK;
			break;
		case SOLARIAN:
			slot[1].race = TERRAN;
			if (slot[0].color != YELLOW)
				slot[1].color = YELLOW;
			else
				slot[1].color = ORANGE;
			break;
		}
		break;
	//
	// ------------end case EASY
	//
	case AVERAGE:
		money = MEDIUM_MONEY;
		mapSize = MEDIUM_MAP;
		terrain = MEDIUM_TERRAIN;
		units = UNITS_MEDIUM;
		visibility = VISIBILITY_NORMAL;
		numSystems = 6;
		memset(&slot[1], 0, sizeof(slot)-sizeof(slot[0]));		// clear all rows except first row
		slot[0].type = HUMAN;
		slot[0].state = ACTIVE;
		slot[0].team = NOTEAM;
		slot[0].dpid = PLAYERID;

		slot[1].type = COMPUTER;
		slot[1].state = READY;
		slot[1].team = _1;
		slot[1].dpid = 0;

		slot[2].type = COMPUTER;
		slot[2].state = READY;
		slot[2].team = _1;
		slot[2].dpid = 0;

		switch (slot[0].race)		// assign computer player race+color
		{
		case TERRAN:
			slot[1].race = MANTIS;
			slot[2].race = SOLARIAN;
			if (slot[0].color != RED)
				slot[1].color = RED;
			else
				slot[1].color = GREEN;
			slot[2].color = isAvailable(BLUE, 2) ? BLUE : findUnusedColor();
			break;
		case MANTIS:
			slot[1].race = SOLARIAN;
			slot[2].race = TERRAN;
			if (slot[0].color != BLUE)
				slot[1].color = BLUE;
			else
				slot[1].color = PINK;
			slot[2].color = isAvailable(YELLOW, 2) ? YELLOW : findUnusedColor();
			break;
		case SOLARIAN:
			slot[1].race = TERRAN;
			slot[2].race = MANTIS;
			if (slot[0].color != YELLOW)
				slot[1].color = YELLOW;
			else
				slot[1].color = ORANGE;
			slot[2].color = isAvailable(RED, 2) ? RED : findUnusedColor();
			break;
		}
		break;		  //  end case AVERAGE
	//
	// ------------end case AVERAGE
	//

	case HARD:
		money = LOW_MONEY;
		mapSize = MEDIUM_MAP;
		terrain = LIGHT_TERRAIN;
		units = UNITS_MINIMAL;
		visibility = VISIBILITY_NORMAL;
		numSystems = 8;
		memset(&slot[1], 0, sizeof(slot)-sizeof(slot[0]));		// clear all rows except first row
		slot[0].type = HUMAN;
		slot[0].state = ACTIVE;
		slot[0].team = NOTEAM;
		slot[0].dpid = PLAYERID;

		slot[1].type = COMPUTER;
		slot[1].state = READY;
		slot[1].team = _1;
		slot[1].dpid = 0;

		slot[2].type = COMPUTER;
		slot[2].state = READY;
		slot[2].team = _1;
		slot[2].dpid = 0;

		slot[3].type = COMPUTER;
		slot[3].state = READY;
		slot[3].team = _1;
		slot[3].dpid = 0;

		switch (slot[0].race)		// assign computer player race+color
		{
		case TERRAN:
			slot[1].race = MANTIS;
			slot[2].race = SOLARIAN;
			slot[3].race = TERRAN;
			if (slot[0].color != RED)
				slot[1].color = RED;
			else
				slot[1].color = GREEN;
			slot[2].color = isAvailable(BLUE, 2) ? BLUE : findUnusedColor();
			slot[3].color = isAvailable(YELLOW, 3) ? YELLOW : findUnusedColor();
			break;
		case MANTIS:
			slot[1].race = SOLARIAN;
			slot[2].race = TERRAN;
			slot[3].race = MANTIS;
			if (slot[0].color != BLUE)
				slot[1].color = BLUE;
			else
				slot[1].color = PINK;
			slot[2].color = isAvailable(YELLOW, 2) ? YELLOW : findUnusedColor();
			slot[3].color = isAvailable(RED, 3) ? RED : findUnusedColor();
			break;
		case SOLARIAN:
			slot[1].race = TERRAN;
			slot[2].race = MANTIS;
			slot[3].race = SOLARIAN;
			if (slot[0].color != YELLOW)
				slot[1].color = YELLOW;
			else
				slot[1].color = ORANGE;
			slot[2].color = isAvailable(RED, 2) ? RED : findUnusedColor();
			slot[3].color = isAvailable(BLUE, 3) ? BLUE : findUnusedColor();
			break;
		}
		break;		  //  end case HARD
	}

#ifdef _DEMO_
	slot[0].race = slot[1].race = slot[2].race = slot[3].race = TERRAN;
#endif

	if (bUpdate)
		setStateInfo();
}
//--------------------------------------------------------------------------//
//
DIFFICULTY Menu_mshell::GetDifficulty (void)
{
	DIFFICULTY result = NODIFFICULTY;

	if (money == HIGH_MONEY && 
		mapSize == MEDIUM_MAP &&
		terrain == MEDIUM_TERRAIN && 
		units == UNITS_LARGE && 
		visibility == VISIBILITY_NORMAL &&
		numSystems == 3)
	{
		if (slot[1].state == READY && slot[2].state != READY && slot[3].state != READY)
		{
			if (slot[1].team == NOTEAM)
			{
				if (slot[0].color != slot[1].color)
				{
					result = EASY;
				}
			}
		}
	}
	else
	if (money == MEDIUM_MONEY &&
		mapSize == MEDIUM_MAP &&
		terrain == MEDIUM_TERRAIN &&
		units == UNITS_MEDIUM &&
		visibility == VISIBILITY_NORMAL &&
		numSystems == 6)
	{
		if (slot[1].state == READY && slot[2].state == READY && slot[3].state != READY)
		{
			if (slot[1].team == _1 && slot[2].team == _1)
			{
				if (slot[0].color != slot[1].color && slot[0].color != slot[2].color)
				{
					result = AVERAGE;
				}
			}
		}
	}
	else
	if (money == LOW_MONEY &&
		mapSize == MEDIUM_MAP &&
		terrain == LIGHT_TERRAIN &&
		units == UNITS_MINIMAL &&
		visibility == VISIBILITY_NORMAL &&
		numSystems == 8)
	{
		if (slot[1].state == READY && slot[2].state == READY && slot[3].state == READY)
		{
			if (slot[1].team == _1 && slot[2].team == _1 && slot[3].team == _1 && slot[0].team != slot[1].team)
			{
				if (slot[0].color != slot[1].color && slot[0].color != slot[2].color && slot[0].color != slot[3].color)
				{
					result = HARD;
				}
			}
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::ForceUpdate (void)
{
	setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::clearReady (void)
{
	SetState(localSlot, ACTIVE, true);
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onAddPlayer (DPID id)
{
	S32 row = getSlot(id);

	if (bLoadInProgress)
		killLoadProcess();

	clearAllReadyFlags();

	if (row < 0)
	{
		if ((row = findOpenSlot()) >= 0)
		{
			COLOR color = findUnusedColor();

			slot[row].type = HUMAN;
			slot[row].state = ACTIVE;
			slot[row].race = TERRAN;
			slot[row].color = color;
			slot[row].team = NOTEAM;
			slot[row].dpid = id;
			bLocalDataChanged = true;			// make sure everyone is up to speed
			bMapNameChanged = true;

			getPlayerName(id, szPlayerNames[row]);
			setStateInfo();
			chatAnnouncePlayer(id, true);
		}
		else  // out of slots
		{
			if (PLAYERID == HOSTID)
			{
				NETBUFFER->DestroyPlayer(id);
			}
		}
	}
	else
	{
		getPlayerName(id, szPlayerNames[row]);
		setStateInfo();
		chatAnnouncePlayer(id, true);
	}

	recalculateJoinState();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onDeletePlayer (DPID id)
{
	S32 row = getSlot(id);

	if (row>=0 && bLoadInProgress)
		killLoadProcess();

	if (row >= 0)
	{
		chatAnnouncePlayer(id, false);

		if (slot[row].state != CLOSED)
			slot[row].state = OPEN;
		slot[row].dpid = 0;
	}

	clearAllReadyFlags();
	bLocalDataChanged = true;		
	setStateInfo();
	recalculateJoinState();

	if (id == PLAYERID)
	{
		CQMessageBox(IDS_HELP_BOOTED, IDS_APP_NAMETM, MB_OK);
		endDialog(0);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onNewHost (void)
{
	if (bLoadInProgress)
		killLoadProcess();

	if (HOSTID == PLAYERID)
	{
		bLocalDataChanged = true;		// remember to tell everyone the true state
		bOutgoingData = false;

		bHostBusy = 0;
		mapType = RANDOM_MAP;
		clearAllReadyFlags();
		setStateInfo();
	}
	recalculateJoinState();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onLocalUpdate (void)
{
	if (bDownloadEnabled || (bLocalDataChanged==false && bLoadInProgress==false && HOSTID==PLAYERID && checkReadyState() && (PLAYERID==0||(startCountdown>=10&&readyCheckTime>10500))))
	{
		if (childFrame)
		{
			childFrame->GetBase()->Release();		// get rid of menu
			CQASSERT(childFrame==0);
		}

		SetVisible(false);
		bLoadInProgress=true;
		bDownloadEnabled = false;

		version = GetBuildVersion();
		DEFAULTS->SetDataInRegistry("LobbyOptions", static_cast<CQGAME *>(this), sizeof(CQGAME));

		if (HOSTID == PLAYERID)
		{
			enableJoin(false);

			USER_DEFAULTS * defaults = DEFAULTS->GetDefaults();

			// set the game speed
			defaults->gameSpeed = gameSpeed;
			defaults->bLockDiplomacy = lockDiplomacyOn;
			defaults->bVisibilityRulesOff = 0;
			defaults->bSpectatorModeOn = 0;
			defaults->bSpectatorModeAllowed = spectatorsOn;
			defaults->fogMode = static_cast<FOGOFWARMODE>(visibility);

			DEFAULTS->StoreDefaults();
			MENU->InitPreferences();
		}

		uploadZoneInfo();

		U32 loadingResult;
		if ((loadingResult = DoMenu_nl(this, menu1, *this, remoteCheckSum, remoteRandomSeed))==0)		// do netloading menu
			endDialog(1);
		else
		{
			SetVisible(true);
			version = GetProductVersion();
			bLoadInProgress=bDownloadEnabled=false;
			recalculateJoinState();
			if (HOSTID==PLAYERID)
				clearAllReadyFlags();

			CQASSERT(childFrame == NULL);

			DoMenu_final(this, menu1, *this, !bLAN);
			setStateInfo();

//			enableSubmenu(submenu);
//			CQMessageBox(loadingResult, IDS_APP_NAME, MB_OK);
		}
	}


	if (bLocalDataChanged && HOSTID==PLAYERID)
	{
		if (NETPACKET->TestLowPrioritySend(sizeof(CQGAME_PACKET)) > sizeof(CQGAME_PACKET))
		{
			CQGAME_PACKET packet;

			packet = *this;
			NETPACKET->Send(0, NETF_ALLREMOTE, &packet);
			bLocalDataChanged = false;
			updateSessDesc();
		}
	}

	if (bMapNameChanged && HOSTID==PLAYERID)
	{
		if (NETPACKET->TestLowPrioritySend(sizeof(MAP_PACKET)) > sizeof(MAP_PACKET))
		{
			MAP_PACKET packet;

			memcpy(packet.szMapName, szMapName, sizeof(packet.szMapName));
			NETPACKET->Send(0, NETF_ALLREMOTE, &packet);
			bMapNameChanged = false;
		}
	}

	if (bOutgoingData)
		sendClientPacket();
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onNetPacket (const BASE_PACKET *_packet)
{
	if (_packet->type == PT_GAMEROOM)
	{
		const GR_PACKET * packet = (const GR_PACKET *) _packet;

		switch (packet->subtype)
		{
		case GAMEDESC:
			if (packet->fromID == HOSTID)
			{
				const CQGAME_PACKET * gpack = (const CQGAME_PACKET *) packet;
				*static_cast<CQGAME *>(this) = *static_cast<const CQGAME *>(gpack);
				setNamesForSlots();
				setStateInfo();
			}
			break;
		case MAPNAME:
			if (packet->fromID == HOSTID)
			{
				const MAP_PACKET * mpack = (const MAP_PACKET *) packet;
				memcpy(szMapName, mpack->szMapName, sizeof(szMapName));
				setStateInfo();
			}
			break;
		case CLIENT:
			if (HOSTID == PLAYERID)
			{
				const CLIENTSETTING_PACKET * client = (const CLIENTSETTING_PACKET *) packet;
				S32 row = getSlot(client->fromID);

				if (row >= 0)
				{
					// if they change race or team, make everyone check in again
//					if (slot[row].race != client->race || slot[row].team != client->team || slot[row].color != client->color)
						clearAllReadyFlags();

					if (bHostBusy==false || client->state != READY)
						slot[row].state = client->state;
					slot[row].race = client->race;
					slot[row].color = client->color;
					slot[row].team = client->team;
					if (bLoadInProgress==false)
						setStateInfo();
					else
						killLoadProcess();
					bLocalDataChanged = true;		// remember to tell everyone the true state
				}
			}
			break;
		case STARTDOWNLOAD:
			if (packet->fromID == HOSTID)
			{
				receiveRemoteDefaults(((START_PACKET *)packet)->userDefaults);
				remoteCheckSum = ((START_PACKET *)packet)->checkSum;
				remoteRandomSeed = ((START_PACKET *)packet)->randomSeed;
				*static_cast<_CQGAME *>(this) = ((START_PACKET *)packet)->cqgame;
				setStateInfo();
				bDownloadEnabled = true;
			}
			break;

		case GRCHAT:
			onReceiveChatPacket((const GRCHAT_PACKET *)packet);
			break;
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::sendClientPacket (bool bAlwaysSend)
{
	if (HOSTID && (bAlwaysSend || NETPACKET->TestLowPrioritySend(sizeof(CLIENTSETTING_PACKET)) >= sizeof(CLIENTSETTING_PACKET)))
	{
		CLIENTSETTING_PACKET packet;

		packet.state = slot[localSlot].state;
		packet.race  = slot[localSlot].race;
		packet.color = slot[localSlot].color;
		packet.team  = slot[localSlot].team;

		NETPACKET->Send(HOSTID, 0, &packet);
		bOutgoingData = false;
	}
	else
		bOutgoingData = true;
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::clearAllReadyFlags (void)
{
//	U32 i;

	bLocalDataChanged = true;		

	if (HOSTID==PLAYERID)
	{
		startCountdown = readyCheckTime = 0;
		slot[localSlot].state = ACTIVE;			// switch the host back to active
	}

	// see if we can use countdown instead of resetting all of the flags
	/*
	for (i = 0; i < activeSlots; i++)
		if (slot[i].type == HUMAN && slot[i].state == READY)
			slot[i].state = ACTIVE;
	*/
}
//--------------------------------------------------------------------------//
//
bool Menu_mshell::checkReadyState (void)
{
	U32 i;
	bool result = true;

	for (i = 0; i < activeSlots; i++)
		if (slot[i].state == ACTIVE)
		{
			result = false;
			break;
		}

	if (result==0)
		startCountdown = readyCheckTime = 0;

	return result;
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::killLoadProcess (void)
{
	CQASSERT(bLoadInProgress);
	if (childFrame->bExitCodeSet==0)
	{
		childFrame->endDialog(IDS_HELP_PLAYERDROPPED);
		MUSICMANAGER->PlayMusic("Multiplayer_menu.wav");
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::receiveRemoteDefaults (const USER_DEFAULTS & userDefaults)
{
	USER_DEFAULTS & defaults = *DEFAULTS->GetDefaults();
	
	defaults.bVisibilityRulesOff = userDefaults.bVisibilityRulesOff;
	defaults.gameSpeed = userDefaults.gameSpeed;
	defaults.fogMode = userDefaults.fogMode;
	defaults.bCheatsEnabled = userDefaults.bCheatsEnabled;
	defaults.bLockDiplomacy = userDefaults.bLockDiplomacy;
	defaults.bVisibilityRulesOff = 0;
	defaults.bSpectatorModeAllowed = userDefaults.bSpectatorModeAllowed;
	defaults.bSpectatorModeOn = 0;
	MENU->InitPreferences();
}
//--------------------------------------------------------------------------//
// user has hit return on edit control
//
void Menu_mshell::sendChatMsg (void)
{
	if (HOSTID && NETPACKET->TestLowPrioritySend(sizeof(GRCHAT_PACKET)) >= sizeof(GRCHAT_PACKET))
	{
		GRCHAT_PACKET packet;

		editChat->GetText(packet.szMsg, sizeof(packet.szMsg));
		if (packet.szMsg[0])		// if not empty text
		{
			int len = (wcslen(packet.szMsg)+1) * sizeof(wchar_t); 

//			packet.iComputerPlayer = -1;
			packet.dwSize = sizeof(GR_PACKET) + len;

			NETPACKET->Send(0, NETF_ALLREMOTE|NETF_ECHO, &packet);
		}
		editChat->SetText(L"");
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::onReceiveChatPacket (const GRCHAT_PACKET *packet)
{
	S32 row;

	row = getSlot(packet->fromID);
	if (row >= 0)
	{
		wchar_t buffer[256];
		swprintf(buffer, L"[%s] %s", szPlayerNames[row], packet->szMsg);
		addToChatBox(packet->fromID, buffer);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::chatAnnouncePlayer (U32 dpid, bool bEntering)
{
	S32 row = getSlot(dpid);
	wchar_t buffer[256];

	swprintf(buffer, L"[%s] %s", szPlayerNames[row], _localLoadStringW((bEntering)?IDS_PLAYER_ENTERING:IDS_PLAYER_EXITING));
	addToChatBox(0, buffer);
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::setNamesForSlots (void)
{
	U32 i;

	for (i = 0; i < activeSlots; i++)
	{
		if (slot[i].type != COMPUTER)
		{
			getPlayerName(slot[i].dpid, szPlayerNames[i]);
		}
	}
}
//--------------------------------------------------------------------------//
//
S32 Menu_mshell::findOpenSlot (void)
{
	U32 i;

	for (i = 0; i < activeSlots; i++)
		if (slot[i].state == OPEN)
			return i;

	return -1;  // no open slots
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::recalculateJoinState (void)
{
	if (PLAYERID == HOSTID && bLoadInProgress==false)
	{
		U32 i;
		bool bEnable = false;

		for (i = 0; i < activeSlots; i++)
			if (slot[i].state == OPEN)
			{
				bEnable = true;
				break;
			}

		enableJoin(bEnable);		// enable join if at least one slot is open
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mshell::addToChatBox (U32 dpid, const wchar_t * string)
{
	const S32 row = (dpid) ? getSlot(dpid) : -1;
	const S32 bindex = listChat->GetBreakIndex(string);  // used for multi-line strings

	if (bindex <= 0)
	{
		const S32 index = listChat->AddString(string);
		listChat->EnsureVisible(index ? index-1 : 0);
		listChat->SetDataValue(index, dpid);
		listChat->SetColorValue(index, (row >= 0) ? COLORTABLE[slot[row].color] : COLORTABLE[0]);
	}
	else // more complicated case, break up string into 2 parts
	if (bindex < 256)
	{
		wchar_t buffer[256];
		memcpy(buffer, string, bindex * sizeof(wchar_t));
		buffer[bindex] = 0;

		S32 index = listChat->AddString(buffer);
		listChat->SetDataValue(index, dpid);
		listChat->SetColorValue(index, (row >= 0) ? COLORTABLE[slot[row].color] : COLORTABLE[0]);

		index = listChat->AddString(string+bindex);
		listChat->EnsureVisible(index ? index-1 : 0);
		listChat->SetDataValue(index, dpid);
		listChat->SetColorValue(index, (row >= 0) ? COLORTABLE[slot[row].color] : COLORTABLE[0]);
		listChat->EnsureVisible(index ? index-1 : 0);
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone)
{
	Menu_mshell * dlg = new Menu_mshell(parent, data, conn, szPlayerName, szSessionName, bLAN, bZone);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//--------------------------End Menu_mshell.cpp-----------------------------//
//--------------------------------------------------------------------------//
