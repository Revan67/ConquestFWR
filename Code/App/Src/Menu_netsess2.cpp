//--------------------------------------------------------------------------//
//                                                                          //
//                            Menu_netsess2.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_netsess2.cpp 64    10/24/00 5:04p Jasony $
*/
//--------------------------------------------------------------------------//
// network session list
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include <DMenu1.h>
#include "NetConnectBuffers.h"
#include "CQGame.h"
#include "MusicManager.h"

#include <dplobby.h>
#include <stdio.h>

using namespace CQGAMETYPES;

U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone);

// flags for next menu
#define SESSFLAG_LAN	  0x00000001
#define SESSFLAG_TCP	  0x00000002
#define SESSFLAG_MODEM	  0x00000004
#define SESSFLAG_SERIAL	  0x00000008
#define SESSFLAG_2PLAYER  0x00000010

#define MAX_PLAYER_CHAR 32

//--------------------------------------------------------------------------//
//
struct Menu_sess2 : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1 & menu1;
	const GT_MENU1::NET_SESSIONS2 & data;
	const SAVED_CONNECTION * conn;
	const wchar_t * const szPlayerName;

	COMPTR<IStatic> background, description, title, version;
	COMPTR<IListbox> list;
	COMPTR<IButton2> next, back;
	COMPTR<IStatic> staticGame, staticPlayers, staticSpeed, staticMap, staticResources;

	U32 sessionFlags;

	//
	// net connection data
	//
	SESSION_BUFFER * & sessions;
	S32 currentSelection;
	GUID selectedSession;

	S32 timer;
	bool bListUpdated;
	bool bJoining;			// startup in progress
	bool bJoinDraw;			// at least one draw has gone by
	bool bUpdateAttempted;

	U32  iAddedCreate;		// equals 1 when added "Create new game option"
	//
	// instance methods
	//

	Menu_sess2 (Frame * _parent, const GT_MENU1 & _data, const SAVED_CONNECTION * _conn, SESSION_BUFFER * & _sessions, U32 flags, const wchar_t * _szPlayerName) : menu1(_data),
																								  data(_data.netSessions2),
																								  conn(_conn), 
																								  sessions(_sessions),
																								  sessionFlags(flags),
																								  szPlayerName(_szPlayerName)

	{
		initializeFrame(_parent);
		init();
	}

	~Menu_sess2 (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* Menu_sess2 methods */

	virtual void setStateInfo (void);

	virtual void onListSelection (S32 listID)		// user has selected a list item
	{
		S32 sel = list->GetCurrentSelection();
		CQASSERT(bInvisible == false);		// prevent double click syndrome
		if (iAddedCreate && sel==0)
			create();
		else
		if (bJoining==0)
			join(sel);
		else
		if (bJoinDraw)		// special draw message has been displayed
			join2(sel);
		else
			PostMessage(CQE_LIST_SELECTION, 0);		// repost the message for next frame
	}

	virtual void onDraw (void)
	{
		if (bJoining)
			bJoinDraw = true;
	}

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_BACK:
			endDialog(0);
			break;

		case IDS_NEXT:
			onListSelection(0);
			break;
		}
	}

	virtual void onListCaretMove (S32 listID)		// user has moved the caret
	{
		S32 sel = list->GetCurrentSelection();
		if (sel != currentSelection)
		{
			SAVED_SESSION *session = NULL;

			currentSelection = sel;
			memset(&selectedSession, 0, sizeof(selectedSession));

			if (currentSelection>=0)
			{
				next->EnableButton(true);
				onFocusChanged();
				CQASSERT(sessions);
				if ((session = sessions->findSession(list->GetDataValue(currentSelection))) != 0)
				{	
					selectedSession = session->guidInstance;
//					initOptions(*((OPTIONS *)&(session->dwUser1)));
				}
			}
			else
			{
				if (focusControl!=0 && focusControl->GetControlID() == IDS_NEXT)
					setFocus(list);
				next->EnableButton(false);
				onFocusChanged();
//				options->SetText(L"");
			}
		}
	}

	virtual void onFocusChanged (void)
	{
		if (bJoining)
			description->SetText(_localLoadStringW(IDS_HELP_WAITINGTOCONNECT));
		else
		if (focusControl!=0)
		{
			S32 id = focusControl->GetControlID();

			switch (id)
			{
			case IDS_BACK:
				description->SetText(_localLoadStringW(IDS_HELP_GOBACK));
				break;
			default:
				if (list->GetNumberOfItems() > 0)
				{
					if (list->GetCurrentSelection() >= 0)
					{
						if (iAddedCreate && list->GetCurrentSelection() == 0)
							description->SetText(_localLoadStringW(IDS_HELP_CREATENOW));
						else
							description->SetText(_localLoadStringW(IDS_HELP_SELECTGAMENOW));
					}
					else
					{
						if (list->GetNumberOfItems()==iAddedCreate)
							description->SetText(_localLoadStringW(IDS_HELP_NO_LAN_GAMES));
						else
							description->SetText(_localLoadStringW(IDS_HELP_SELECTGAME));
					}
				}
				else
				{
					if (bListUpdated==false)
						description->SetText(_localLoadStringW(IDS_HELP_SEARCHING_FOR_GAMES));
					else
					if (sessionFlags & SESSFLAG_LAN)
						description->SetText(_localLoadStringW(IDS_HELP_NO_LAN_GAMES));
					else
						description->SetText(_localLoadStringW(IDS_HELP_NO_DEFAULT_GAMES));
				}
				break;
			}
		}
		else
			description->SetText(NULL);
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonPressed(IDS_BACK);
		return true;
	}

	virtual void onUpdate (U32 dt)   // dt in milliseconds
	{
		if (bJoining==false && bInvisible==false && (timer -= dt) <= 0)
		{
			timer = 200;	// update 2 times a second
			if (bUpdateAttempted==false)
			{
				updateList();

				if (bListUpdated==false)
				{
					bListUpdated=true;
					onFocusChanged();
				}
			}
		}
	}

	void updateList (void);
	void init (void);
	void join (S32 index);
	void join2 (S32 index);
	void create (void);

//	void initOptions (const OPTIONS & cqgameOptions);

	void fillDescriptionString (const SAVED_SESSION * pSession, wchar_t * szString);

	static bool testForName (const DPSESSIONDESC2 & sdesc, const wchar_t * const szPlayerName, int & numPlayers);	// true if found name

	void determineSessionName (wchar_t * szBuffer, int bufferSize);
};
//----------------------------------------------------------------------------------//
//
Menu_sess2::~Menu_sess2 (void)
{
}
//----------------------------------------------------------------------------------//
//
void Menu_sess2::setStateInfo (void)
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
	description->InitStatic(data.description, this);

	title->InitStatic(data.title, this);
	version->InitStatic(data.version, this);
	
	list->InitListbox(data.list, this);
	
	back->InitButton(data.back, this);
	next->InitButton(data.next, this);

	back->SetTransparent(true);
	next->SetTransparent(true);

	staticGame->InitStatic(data.staticGame, this);
	staticPlayers->InitStatic(data.staticPlayers, this);
	staticSpeed->InitStatic(data.staticSpeed, this);
	staticMap->InitStatic(data.staticMap, this);
	staticResources->InitStatic(data.staticResources, this);

	if (childFrame)
		childFrame->setStateInfo();
	else
		setFocus(list);
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.description.staticType, pComp);
	pComp->QueryInterface("IStatic", description);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);
	
	GENDATA->CreateInstance(data.version.staticType, pComp);
	pComp->QueryInterface("IStatic", version);

	GENDATA->CreateInstance(data.list.listboxType, pComp);
	pComp->QueryInterface("IListbox", list);
	
	GENDATA->CreateInstance(data.next.buttonType, pComp);
	pComp->QueryInterface("IButton2", next);	
	
	GENDATA->CreateInstance(data.back.buttonType, pComp);
	pComp->QueryInterface("IButton2", back);

	GENDATA->CreateInstance(data.staticGame.staticType, pComp);
	pComp->QueryInterface("IStatic", staticGame);

	GENDATA->CreateInstance(data.staticPlayers.staticType, pComp);
	pComp->QueryInterface("IStatic", staticPlayers);

	GENDATA->CreateInstance(data.staticSpeed.staticType, pComp);
	pComp->QueryInterface("IStatic", staticSpeed);

	GENDATA->CreateInstance(data.staticMap.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMap);

	GENDATA->CreateInstance(data.staticResources.staticType, pComp);
	pComp->QueryInterface("IStatic", staticResources);

	currentSelection = -1;
	timer = (sessionFlags & SESSFLAG_LAN) ? 1000 : 5000;		
	description->SetText(_localLoadStringW(IDS_HELP_SELECTGAME));
	next->EnableButton(false);

	{
		char    prodversiona[64];
		wchar_t versionw[64];


		GetProductVersion(prodversiona, sizeof(prodversiona));
		swprintf(versionw, L"%s  %S", _localLoadStringW(IDS_STATIC_VERSION), prodversiona);
		version->SetText(versionw);
	}

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::determineSessionName (wchar_t * szBuffer, int bufferSize)
{
	// go through all of the sessions listed and see if a name matches our own
	// if so, then put a number after the name in question
	wchar_t szSessionName[MAX_PLAYER_CHAR];
	memset(szSessionName, 0, sizeof(szSessionName));

	if (sessionFlags & (SESSFLAG_LAN|SESSFLAG_TCP))		// check for conflicting names
	{
		if (sessions)
		{
			sessions->ContinueEnumSessions();
		}
		else
		{
			sessions = SESSION_BUFFER::StartEnumSessions();
		}
	}

	if (sessions)
	{
		// go through all the session and find out how many sessions share the same name (at least partially, anyway)
		int len = wcslen(szPlayerName);
		int num = 0;

		SAVED_SESSION *session = NULL;
		
		while ((session = sessions->getNext(session)) != 0)
		{	
			if (!(session->dwFlags & DPSESSION_JOINDISABLED))
			{
				if (wcsncmp(szPlayerName, session->sessionName, len) == 0)
				{
					num++;
				}
			}
		}

		// if the number is greater than zero, than append the string
		if (num)
		{
			wchar_t szAppend[12];
			wcsncpy(szSessionName, szPlayerName, wcslen(szPlayerName));
			swprintf(szAppend, L"(%d)", num+1);
			wcscat(szSessionName, szAppend);
		}
		else
		{
			// just use the player name
			wcsncpy(szSessionName, szPlayerName, wcslen(szPlayerName));
		}
	}
	else
	{
		// just use the player name
		wcsncpy(szSessionName, szPlayerName, wcslen(szPlayerName));
	}

	bufferSize /= sizeof(wchar_t);
	int len = wcslen(szSessionName);
	len = __min(len+1, bufferSize);

	memcpy(szBuffer, szSessionName, len*sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::updateList (void)
{
/*	static bool bDone;

	if (bDone == false)
	{
		bDone = true;
		list->ResetContent();
		
		const wchar_t * const fmt = L"%-20.18s%-9d%-12s%-20.16s%-8s";

		wchar_t buffer[256];
		for (U32 i = 0; i < 20; i++)
		{
			if (i%2)
			{
				swprintf(buffer, fmt, L"The Crazy Ass Game We all play", rand()%8, L"Medium", L"Random", L"Lite");
			}
			else
			{
				swprintf(buffer, fmt, L"My Game", rand()%8, L"Fast", L"Little Shop of Horrors", L"Heavy");
			}
			list->AddString(buffer);
		}
	}
*/
	HRESULT hr;
	SAVED_SESSION *session = NULL;
	S32 oldSelection = currentSelection;
	S32 index = 0;

	bUpdateAttempted = true;		// prevent recursion

	currentSelection = -1;
	list->ResetContent();

	if (sessionFlags & SESSFLAG_LAN)
	{
		S32 i = list->AddString(_localLoadStringW(IDS_CREATE_LAN_GAME));
		list->SetDataValue(i, 0xFFFFFFFF);
		iAddedCreate = 1;
	}

	hr = sessions->ContinueEnumSessions();

	if (SUCCEEDED(hr))
	{
		while ((session = sessions->getNext(session)) != 0)
		{	
			if (!(session->dwFlags & DPSESSION_JOINDISABLED))
			{
				wchar_t buffer[MAX_PATH];

				fillDescriptionString(session, buffer);
				S32 i = list->AddString(buffer);
				
				list->SetDataValue(i, index);

				if (oldSelection != 0 || iAddedCreate == 0)	// if old selection was "Create New Game", don't bother comparing
				{
					if (oldSelection>=0 && IsEqualGUID(session->guidInstance, selectedSession))
					{
						currentSelection = list->SetCurrentSelection(i);
					}
				}
			}

			index++;
		}

		if (list->GetCurrentSelection() != -1)
		{
			list->EnsureVisible(list->GetCurrentSelection());
		}
		else
		if (oldSelection == 0 && iAddedCreate != 0)
		{
			list->SetCurrentSelection(0);
			currentSelection = 0;
		}

		onFocusChanged();
		bUpdateAttempted = false;		// prevent recursion
	}
	else
	{
		CQMessageBox(IDS_HELP_CONNFAILED, IDS_APP_NAMETM, MB_OK);
		endDialog(0);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::join (S32 listIndex)
{
	DPSESSIONDESC2 sdesc;
	SAVED_SESSION *saved=0;

	memset(&sdesc, 0, sizeof(sdesc));
	sdesc.dwSize = sizeof(DPSESSIONDESC2);
	if (listIndex >= 0)
		saved = sessions->findSession(list->GetDataValue(listIndex));
	if (saved == 0)
		return;

	sdesc.guidInstance = saved->guidInstance;

	description->SetText(_localLoadStringW(IDS_HELP_WAITINGTOCONNECT));

	bJoining = true;
	bJoinDraw = false;
	CURSOR->SetBusy(1);

	PostMessage(CQE_LIST_SELECTION, 0);
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::join2 (S32 listIndex)
{
	DPSESSIONDESC2 sdesc;
	SAVED_SESSION *saved=0;
	HRESULT hr;
	DPNAME dpname;
	int numPlayers=0;

	memset(&sdesc, 0, sizeof(sdesc));
	sdesc.dwSize = sizeof(DPSESSIONDESC2);
	if (listIndex>=0)
		saved = sessions->findSession(list->GetDataValue(listIndex));
	if (saved == 0)
	{
		CURSOR->SetBusy(0);
		goto Done;
	}

	sdesc.guidInstance = saved->guidInstance;

	if (testForName(sdesc, szPlayerName, numPlayers))
	{
		CURSOR->SetBusy(0);
		if (CQMessageBox(IDS_HELP_DUPLICATENAME, IDS_APP_NAMETM, MB_OKCANCEL) == 0)
		{
			goto Done;
		}
		CURSOR->SetBusy(1);
	}

	if (saved->dwUser1!=0 && saved->dwUser1 != GetProductVersion())
	{
		CQMessageBox(IDS_HELP_BADVERSION, IDS_APP_NAMETM, MB_OK);
		CURSOR->SetBusy(0);
		goto Done;
	}

	if (numPlayers!=0 && (hr = DPLAY->Open(&sdesc, DPOPEN_JOIN)) == DP_OK)
	{
		memset(&dpname, 0, sizeof(dpname));
		dpname.dwSize = sizeof(dpname);
		dpname.lpszShortName = const_cast<wchar_t *>(szPlayerName);

		if ((hr = DPLAY->CreatePlayer(&PLAYERID, &dpname, 0, 0, 0, 0)) == DP_OK)
		{
			DPCAPS dpcaps;
			
			dpcaps.dwSize = sizeof(dpcaps);
			DPLAY->GetPlayerCaps(PLAYERID, &dpcaps, 0);
			
			if (dpcaps.dwFlags & DPCAPS_ISHOST)
			{
				HOSTID = PLAYERID;
				EVENTSYS->Send(CQE_NEWHOST, 0);		// notify locals that host has changed
			}
			
			EVENTSYS->Send(CQE_NETSTARTUP, 0);		// annouce to everyone that we are starting a net session
			//
			// create the gameroom menu
			//
			SetVisible(false);
			CURSOR->SetBusy(0);

			if (DoMenu_mshell(this, menu1, conn, szPlayerName, saved->sessionName, (sessionFlags & SESSFLAG_LAN) != 0, false) == 0)
			{
				StopNetConnection(false);
				setFocus(list);
				MUSICMANAGER->PlayMusic("network_game_start.wav");
			}
			else
				endDialog(1);
			SetVisible(true);
			goto Done;
		}
		else
		{
			CURSOR->SetBusy(0);
			CQBOMB0("Failed to create a player!?");
			goto Done;
		}
	}
	else
	{
		CURSOR->SetBusy(0);
		CQMessageBox(IDS_HELP_JOINFAILED, IDS_APP_NAMETM, MB_OK);
		goto Done;
	}

Done:
	bJoining = false;
	setFocus(list);
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::create (void)
{
	CURSOR->SetBusy(1);

	//
	// create the session
	//

	DPSESSIONDESC2 sdesc;
	DPNAME dpname;
	HRESULT hr;
	
	memset(&sdesc, 0, sizeof(sdesc));
	sdesc.dwSize = sizeof(DPSESSIONDESC2);
	sdesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
	sdesc.guidApplication = APPGUID_CONQUEST;
	sdesc.dwMaxPlayers = MAX_PLAYERS; // (conn && IsEqualGUID(conn->guidSP,DPSPGUID_IPX) != 0) ? MAX_PLAYERS : (MAX_PLAYERS/2);
	sdesc.dwUser1 = GetProductVersion();

	wchar_t szSessionName[MAX_PLAYER_CHAR];
	determineSessionName(szSessionName, sizeof(szSessionName));
	sdesc.lpszSessionName = szSessionName;

	if ((hr = DPLAY->Open(&sdesc, DPOPEN_CREATE)) == DP_OK)
	{
		memset(&dpname, 0, sizeof(dpname));
		dpname.dwSize = sizeof(dpname);
		dpname.lpszShortName = const_cast<wchar_t *>(szPlayerName);
	
		CURSOR->SetBusy(0);
		
		if ((hr = DPLAY->CreatePlayer(&PLAYERID, &dpname, 0, 0, 0, 0)) == DP_OK)
		{
			DPCAPS dpcaps;
			
			dpcaps.dwSize = sizeof(dpcaps);
			DPLAY->GetPlayerCaps(PLAYERID, &dpcaps, 0);
			
			if (dpcaps.dwFlags & DPCAPS_ISHOST)
			{
				HOSTID = PLAYERID;
				EVENTSYS->Send(CQE_NEWHOST, 0);		// notify locals that host has changed
			}
			
			EVENTSYS->Send(CQE_NETSTARTUP, 0);		// announce to everyone that we are starting a net session

			//
			// create the gameroom menu
			//
			SetVisible(false);
			if (DoMenu_mshell(this, menu1, conn, szPlayerName, szPlayerName, (sessionFlags & SESSFLAG_LAN) != 0, false) == 0)
			{
				StopNetConnection(false);
				SetVisible(true);
				setFocus(list);
				MUSICMANAGER->PlayMusic("network_game_start.wav");
			}
			else
				endDialog(1);
		}
		else
			CQBOMB0("DPLAY::CreatePlayer failed?");
	}
	else
	{
		CURSOR->SetBusy(0);
		CQMessageBox(IDS_HELP_CREATEFAILED, IDS_APP_NAMETM, MB_OK, this);
	}
}
//--------------------------------------------------------------------------//
//
bool Menu_sess2::testForName (const DPSESSIONDESC2 & sdesc, const wchar_t * const szPlayerName, int & numPlayers)
{
	PLAYER_BUFFER * playerBuffer = EnumPlayers(sdesc.guidInstance);
	SAVED_PLAYER * node = playerBuffer->getNext(NULL);
	bool result = true;
	numPlayers = 0;

	while (node)
	{
		numPlayers++;
		if (wcscmp(node->name, szPlayerName) == 0)
			goto Done;
		node = playerBuffer->getNext(node);
	}

	result = false;

Done:
	delete playerBuffer;
	return result;
}
//--------------------------------------------------------------------------//
//
void Menu_sess2::fillDescriptionString (const SAVED_SESSION * pSession, wchar_t * szString)
{
//	const wchar_t * const fmt = L"%-28.26s%-10d%-8s%-16.14s%-8s";
	const wchar_t * const fmt = L"%-20.18s%-9d%-12s%-20.16s%-8s";

	const OPTIONS & cqgame = *((OPTIONS *)&(pSession->dwUser1));

	wchar_t szSpeed[64];
	wchar_t szMap[64];
	wchar_t szResources[64];

	swprintf(szSpeed, L"%d", cqgame.gameSpeed);

	switch (cqgame.mapType)
	{
	case RANDOM_MAP:
		wcscpy(szMap, _localLoadStringW(IDS_MAPTYPE_RANDOM));
		break;
	case SELECTED_MAP:
		wcscpy(szMap, _localLoadStringW(IDS_MAPTYPE_FILE));
		break;
	case USER_MAP:
		wcscpy(szMap, _localLoadStringW(IDS_MAPTYPE_USERDEFINED));
		break;
	}

	switch (cqgame.terrain)
	{
	case LIGHT_TERRAIN:
		wcscpy(szResources, _localLoadStringW(IDS_TERRAIN_LIGHT));
		break;
	case MEDIUM_TERRAIN:
		wcscpy(szResources, _localLoadStringW(IDS_TERRAIN_MEDIUM));
		break;
	case HEAVY_TERRAIN:
		wcscpy(szResources, _localLoadStringW(IDS_TERRAIN_HEAVY));
		break;
	}

	swprintf(szString, fmt, pSession->sessionName, pSession->dwCurrentPlayers, szSpeed, szMap, szResources);
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_sess2 (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, SESSION_BUFFER * & sessions, U32 flags, const wchar_t * szPlayerName)
{
	Menu_sess2 * dlg = new Menu_sess2(parent, data, conn, sessions, flags, szPlayerName);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_netsess2.cpp------------------------//
//--------------------------------------------------------------------------//