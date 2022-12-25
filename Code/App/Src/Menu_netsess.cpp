//--------------------------------------------------------------------------//
//                                                                          //
//                            Menu_netsess.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_netsess.cpp 67    10/24/00 10:45p Jasony $
*/
//--------------------------------------------------------------------------//
// network session options, after connection is chosen
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <wchar.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include "IListbox.h"
#include <DMenu1.h>
#include "NetConnectBuffers.h"
#include "Mission.h"
#include "UserDefaults.h"
#include "MusicManager.h"

#include <dplobby.h>
#include "ZoneLobby.h"

#define MAX_PLAYER_CHAR 32

U32 __stdcall DoMenu_sess2 (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, SESSION_BUFFER * & sessions, U32 flags, const wchar_t * szPlayerName);
U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone);

// flags for next menu
#define SESSFLAG_LAN	  0x00000001
#define SESSFLAG_TCP	  0x00000002
#define SESSFLAG_MODEM	  0x00000004
#define SESSFLAG_SERIAL	  0x00000008
#define SESSFLAG_2PLAYER  0x00000010
//--------------------------------------------------------------------------//
//
struct Menu_sess : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1 & menu1;
	const GT_MENU1::IP_ADDRESS & data;
	const SAVED_CONNECTION * conn;
	SESSION_BUFFER *sessions;

	COMPTR<IStatic> background, description;
	COMPTR<IStatic> enterIP, enterName;
	COMPTR<IStatic> staticName, staticCreate, staticJoin;

	COMPTR<ICombobox> comboboxIP;
	COMPTR<IButton2>  checkCreate, checkJoin, next, back;

	wchar_t szPlayerName[MAX_PLAYER_CHAR];

	U32 sessionFlags;
	bool bSess2Started;			// did we start the sess2 menu (for lan games)
	//
	// instance methods
	//

	Menu_sess (Frame * _parent, const GT_MENU1 & _data, const SAVED_CONNECTION * _conn, U32 flags, wchar_t * _szPlayerName) : menu1(_data),
																									data(_data.ipAddress),
																									sessionFlags(flags),
																								    conn(_conn)
	{
		playMenuMusic();
		wcsncpy(szPlayerName, _szPlayerName, sizeof(szPlayerName)/sizeof(wchar_t));
		initializeFrame(_parent);
		init();
	}

	~Menu_sess (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* Menu_sess methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_JOIN:
			checkJoin->SetPushState(true);
			checkCreate->SetPushState(false);
			if ((sessionFlags & (SESSFLAG_TCP|SESSFLAG_LAN)) == SESSFLAG_TCP)
			{
				comboboxIP->SetVisible(true);
				enterIP->SetVisible(true);
				setFocus(comboboxIP);
			}
			else
			{
				next->EnableButton(true);
				setFocus(next);
			}
			break;

		case IDS_CREATE:
			comboboxIP->SetVisible(false);
			enterIP->SetVisible(false);
			checkJoin->SetPushState(false);
			checkCreate->SetPushState(true);
			next->EnableButton(true);
			setFocus(next);
			break;

		case IDS_BACK:
			endDialog(0);
			break;

		case IDS_NEXT:
			if (checkJoin->GetPushState())
				joinGame();
			else
			if (checkCreate->GetPushState())
				createGame();
			break;

		case IDS_JOIN | 0x80000000:		// dblclick 
			onButtonPressed(IDS_NEXT);
			break;

		case IDS_CREATE | 0x80000000:		// dblclick 
			onButtonPressed(IDS_NEXT);
			break;

		case IDS_DEFAULT_IP_ADDRESS:
			if (next->GetEnableState())
				setFocus(next);
			break;

		}
	}

	virtual void onUpdate (U32 dt)   // dt in milliseconds
	{
		bool bEnable = (checkJoin->GetPushState() || checkCreate->GetPushState());

		next->EnableButton(bEnable);

		if (bSess2Started==false && (sessionFlags & SESSFLAG_LAN) != 0)
		{
			bSess2Started = true;
			joinGame();
		}
	}

	virtual void onFocusChanged (void)
	{
		if (focusControl!=0)
		{
			S32 id = focusControl->GetControlID();

			switch (id)
			{
			case IDS_DEFAULT_IP_ADDRESS:
				description->SetText(_localLoadStringW(IDS_HELP_IPADDRESS));
				break;
			case IDS_CREATE:
				description->SetText(_localLoadStringW(IDS_HELP_CHOOSE_CREATE));
				break;
			case IDS_JOIN:
				description->SetText(_localLoadStringW(IDS_HELP_CHOOSE_JOIN));
				break;
			case IDS_NEXT:
				if (checkJoin->GetPushState())
					description->SetText(_localLoadStringW(getJoinHelpTextID()));
				else
					description->SetText(_localLoadStringW(getCreateHelpTextID()));
				break;
			case IDS_BACK:
				description->SetText(_localLoadStringW(IDS_HELP_GOBACK));
				break;
			default:
				description->SetText(NULL);
				break;
			}
		}
		else
			description->SetText(NULL);

		// make sure we're playing the right music
		playMenuMusic();
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonPressed(IDS_BACK);
		return true;
	}

	void init (void);

	void setNamesInRegistry (void);

	void joinGame (void);

	void createGame (void);

	void * createTCPAddress (const wchar_t * szAddress);

	U32 getJoinHelpTextID (void)
	{
		if (sessionFlags & SESSFLAG_LAN)
			return IDS_HELP_JOIN_LAN;
		else
		if (sessionFlags & SESSFLAG_TCP)
			return IDS_HELP_JOIN_INTERNET;
		else
		if (sessionFlags & SESSFLAG_MODEM)
			return IDS_HELP_JOIN_MODEM;
		else
		if (sessionFlags & SESSFLAG_SERIAL)
			return IDS_HELP_JOIN_SERIAL;
		else
			return IDS_HELP_JOIN_DEFAULT;
	}

	U32 getCreateHelpTextID (void)
	{
		if (sessionFlags & SESSFLAG_LAN)
			return IDS_HELP_CREATE_LAN;
		else
		if (sessionFlags & SESSFLAG_TCP)
			return IDS_HELP_CREATE_INTERNET;
		else
		if (sessionFlags & SESSFLAG_MODEM)
			return IDS_HELP_CREATE_MODEM;
		else
		if (sessionFlags & SESSFLAG_SERIAL)
			return IDS_HELP_CREATE_SERIAL;
		else
			return IDS_HELP_CREATE_DEFAULT;
	}

	static void setNameInRegistry (IEdit2 * edit, U32 id);
	
	static void getNameFromRegistry (IEdit2 * edit, U32 id);

	void playMenuMusic (void)
	{
		MUSICMANAGER->PlayMusic("network_game_start.wav");
	}

	void determineSessionName (wchar_t * szBuffer, int bufferSize);
};
//----------------------------------------------------------------------------------//
//
Menu_sess::~Menu_sess (void)
{
	delete sessions;
}
//--------------------------------------------------------------------------//
//
void Menu_sess::determineSessionName (wchar_t * szBuffer, int bufferSize)
{
	// go through all of the sessions listed and see if a name matches our own
	// if so, then put a number after the name in question
	wchar_t szSessionName[256];
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
			wcsncpy(szSessionName, szPlayerName, sizeof(szSessionName)/sizeof(wchar_t));
			swprintf(szAppend, L"(%d)", num+1);
			wcscat(szSessionName, szAppend);
		}
		else
		{
			// just use the player name
			wcsncpy(szSessionName, szPlayerName, sizeof(szSessionName)/sizeof(wchar_t));
		}
	}
	else
	{
		// just use the player name
		wcsncpy(szSessionName, szPlayerName, sizeof(szSessionName)/sizeof(wchar_t));
	}

	bufferSize /= sizeof(wchar_t);
	int len = wcslen(szSessionName);
	len = __min(len+1, bufferSize);

	memcpy(szBuffer, szSessionName, len*sizeof(wchar_t));
}
//--------------------------------------------------------------------------//
//
void Menu_sess::getNameFromRegistry (IEdit2 * edit, U32 id)
{
	char address[256];
	wchar_t addressw[256];
	int i;
	COMPTR<IListbox> list;

	edit->QueryInterface("IListbox", list);

	for (i = 0; i < 4; i++)
	{
		if (DEFAULTS->GetNameInMRU(address,id, i))
		{
			if (address[0] == 0)
			{
				const wchar_t * ptr = _localLoadStringW(id);
				if (i==0)
					edit->SetText(ptr);
				if (list)
				{
					if (list->FindStringExact(ptr) < 0)
						list->AddString(ptr);
				}
			}
			else
			{
				_localAnsiToWide(address, addressw, sizeof(addressw));
				if (i==0)
					edit->SetText(addressw);
				if (list)
				{
					if (list->FindStringExact(addressw) < 0)
						list->AddString(addressw);
				}
			}
		}
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_sess::setStateInfo (void)
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
	enterIP->InitStatic(data.enterIP, this);
	enterName->InitStatic(data.enterName, this);
	staticName->InitStatic(data.staticName, this);
	staticCreate->InitStatic(data.staticCreate, this);
	staticJoin->InitStatic(data.staticJoin, this);

	checkCreate->InitButton(data.checkCreate, this);
	checkJoin->InitButton(data.checkJoin, this);
	comboboxIP->InitCombobox(data.comboboxIP, this);

	staticCreate->SetBuddyControl(checkCreate);
	staticJoin->SetBuddyControl(checkJoin);

	back->InitButton(data.back, this);
	next->InitButton(data.next, this);

	back->SetTransparent(true);
	next->SetTransparent(true);

	if (childFrame)
		childFrame->setStateInfo();
		

	//
	// get default state info from system
	//
	getNameFromRegistry(comboboxIP, IDS_DEFAULT_IP_ADDRESS);
	staticName->SetText(szPlayerName);
}
//--------------------------------------------------------------------------//
//
void Menu_sess::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);
	GENDATA->CreateInstance(data.description.staticType, pComp);
	pComp->QueryInterface("IStatic", description);
	
	GENDATA->CreateInstance(data.enterIP.staticType, pComp);
	pComp->QueryInterface("IStatic", enterIP);
	GENDATA->CreateInstance(data.enterName.staticType, pComp);
	pComp->QueryInterface("IStatic", enterName);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", staticName);
	GENDATA->CreateInstance(data.staticCreate.staticType, pComp);
	pComp->QueryInterface("IStatic", staticCreate);
	GENDATA->CreateInstance(data.staticJoin.staticType, pComp);
	pComp->QueryInterface("IStatic", staticJoin);

	GENDATA->CreateInstance(data.comboboxIP.comboboxType, pComp);
	pComp->QueryInterface("ICombobox", comboboxIP);

	GENDATA->CreateInstance(data.checkJoin.buttonType, pComp);
	pComp->QueryInterface("IButton2", checkJoin);
	GENDATA->CreateInstance(data.checkCreate.buttonType, pComp);
	pComp->QueryInterface("IButton2", checkCreate);

	GENDATA->CreateInstance(data.next.buttonType, pComp);
	pComp->QueryInterface("IButton2", next);
	GENDATA->CreateInstance(data.back.buttonType, pComp);
	pComp->QueryInterface("IButton2", back);

	setStateInfo();

	comboboxIP->SetVisible(false);
	enterIP->SetVisible(false);
	next->EnableButton(false);

	comboboxIP->SetControlID(IDS_DEFAULT_IP_ADDRESS);
	checkJoin->SetControlID(IDS_JOIN);
	checkCreate->SetControlID(IDS_CREATE);

	if ((sessionFlags & (SESSFLAG_TCP|SESSFLAG_LAN)) != SESSFLAG_TCP)
	{
		enterIP->SetVisible(false);
		comboboxIP->SetVisible(false);
	}

	// if we chose join last time, choose it again, for old time's sake
	if (DEFAULTS->GetDefaults()->bChoseJoin)
	{
		onButtonPressed(IDS_JOIN);
		next->EnableButton(true);
		setFocus(next);
	}
	else
	{
		onButtonPressed(IDS_CREATE);
		next->EnableButton(true);
		setFocus(next);
	}

	if (sessionFlags & SESSFLAG_LAN)
		SetVisible(false);		// go directly to sess2 menu
}
//--------------------------------------------------------------------------//
//
void Menu_sess::setNameInRegistry (IEdit2 * edit, U32 id)
{
	char address[256];
	wchar_t addressw[256];
	
	if (edit->GetText(addressw, sizeof(addressw)))
	{
		if (wcscmp(addressw, _localLoadStringW(id))==0)		// if using default IP
			address[0] = 0;
		else
			_localWideToAnsi(addressw, address, sizeof(address));
		DEFAULTS->RemoveNameFromMRU(address,id);
		DEFAULTS->InsertNameIntoMRU(address,id);
	}
	else
	{
		const wchar_t * ptr = _localLoadStringW(id);
		edit->SetText(ptr);
		DEFAULTS->RemoveNameFromMRU("",id);
		DEFAULTS->InsertNameIntoMRU("",id);		// set empty string in registry
	}
}
//--------------------------------------------------------------------------//
//
void Menu_sess::setNamesInRegistry (void)
{
	setNameInRegistry(comboboxIP,		IDS_DEFAULT_IP_ADDRESS);
}
//---------------------------------------------------------------------------
//
void * Menu_sess::createTCPAddress (const wchar_t * szAddress)
{
	void * result = 0;
	DPCOMPOUNDADDRESSELEMENT element[2];
	DWORD dwAddressSize=0;

	element[0].guidDataType = DPAID_ServiceProvider;
	element[0].dwDataSize = sizeof(DPSPGUID_TCPIP);
	element[0].lpData = (void *) &DPSPGUID_TCPIP;

	element[1].guidDataType = DPAID_INetW;
	element[1].dwDataSize = (wcslen(szAddress) + 1) * sizeof(wchar_t);
	element[1].lpData = (void *) szAddress;

	switch (DPLOBBY->CreateCompoundAddress(element, 2, 0, &dwAddressSize))
	{
	case DPERR_BUFFERTOOSMALL:
	case DP_OK:
		break;	// do nothing
	default:
		goto Done;
	}

	result = malloc(dwAddressSize);

	if (DPLOBBY->CreateCompoundAddress(element, 2, result, &dwAddressSize) != DP_OK)
	{
		free(result);
		result = 0;
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void Menu_sess::joinGame (void)
{
	wchar_t bufferw[256];

	bufferw[0] = 0;
	if ((sessionFlags & (SESSFLAG_TCP|SESSFLAG_LAN)) == SESSFLAG_TCP)
	{
		comboboxIP->GetText(bufferw, sizeof(bufferw));
		if (wcscmp(bufferw, _localLoadStringW(IDS_DEFAULT_IP_ADDRESS)) == 0)	// if using default IP address
			bufferw[0] = 0;
	}
	setNamesInRegistry();
	DEFAULTS->GetDefaults()->bChoseJoin = 1;

	delete sessions;
	sessions = 0;

	BOOL32 bLobbied;
	StartNetConnection(bLobbied);

	CQASSERT(bLobbied==0 && "Should not be here if lobbied!");
	CQASSERT(DPLAY!=0);

	{
		void * lpTCPAddress = 0;
		
		// attempt to provide an IP address for TCP connections
		if (sessionFlags & SESSFLAG_TCP)
			lpTCPAddress = createTCPAddress(bufferw);
		
		if (lpTCPAddress == 0 || DPLAY->InitializeConnection(lpTCPAddress, 0) != DP_OK)
			if (DPLAY->InitializeConnection(conn->lpConnection, 0) != DP_OK)
			{
				CURSOR->SetBusy(0);
				CQERROR0("Connection failed");
				return;
			}
	
		free(lpTCPAddress);
		lpTCPAddress = 0;
			
		sessions =  SESSION_BUFFER::StartEnumSessions ();
	}

	if (!sessions)
	{
		endDialog(0);
		return;
	}

	staticName->GetText(bufferw, sizeof(bufferw));

	SetVisible(false);
	if (DoMenu_sess2(this, menu1, conn, sessions, sessionFlags, bufferw))
		endDialog(1);
	else
	if (sessionFlags & SESSFLAG_LAN)
	{
		endDialog(0);
	}
	else
	{
		playMenuMusic();
		delete sessions;
		sessions = 0;
		SetVisible(true);
		setFocus(checkJoin);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_sess::createGame (void)
{
	BOOL32 bLobbied;
	
	CURSOR->SetBusy(1);

	StartNetConnection(bLobbied);

	CQASSERT(bLobbied==0 && "Should not be here if lobbied!");
	CQASSERT(DPLAY!=0);

	if (sessionFlags & (SESSFLAG_LAN|SESSFLAG_TCP))		// check for conflicting names
	{
		void * lpTCPAddress = 0;
		
		// attempt to provide an IP address for TCP connections
		if (sessionFlags & SESSFLAG_TCP)
			lpTCPAddress = createTCPAddress(L"");
		
		if (lpTCPAddress == 0 || DPLAY->InitializeConnection(lpTCPAddress, 0) != DP_OK)
			if (DPLAY->InitializeConnection(conn->lpConnection, 0) != DP_OK)
			{
				CURSOR->SetBusy(0);
				CQERROR0("Connection failed");
				return;
			}
	
		free(lpTCPAddress);
		lpTCPAddress = 0;
			
		sessions =  SESSION_BUFFER::StartEnumSessions ();
	}
	else  // not a LAN game
	if (DPLAY->InitializeConnection(conn->lpConnection, 0) != DP_OK)
	{
		CURSOR->SetBusy(0);
		CQBOMB0("Connection failed");
	}

	setNamesInRegistry();
	DEFAULTS->GetDefaults()->bChoseJoin = 0;

	//
	// create the session
	//

	wchar_t sname[256];
	wchar_t pname[256];
	DPSESSIONDESC2 sdesc;
	DPNAME dpname;
	HRESULT hr;
	
	memset(&sdesc, 0, sizeof(sdesc));
	sdesc.dwSize = sizeof(DPSESSIONDESC2);
	sdesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
	sdesc.guidApplication = APPGUID_CONQUEST;
	sdesc.dwMaxPlayers = MAX_PLAYERS; // (conn && IsEqualGUID(conn->guidSP,DPSPGUID_IPX) != 0) ? MAX_PLAYERS : (MAX_PLAYERS/2);
	sdesc.lpszSessionName = sname;

	determineSessionName(sname, sizeof(sname));
//	staticName->GetText(sname, sizeof(sname));
	
	sdesc.dwUser1 = GetProductVersion();

	CQASSERT(sname[0]);

	if ((hr = DPLAY->Open(&sdesc, DPOPEN_CREATE)) == DP_OK)
	{
		memset(&dpname, 0, sizeof(dpname));
		dpname.dwSize = sizeof(dpname);
		staticName->GetText(pname, sizeof(pname));
		dpname.lpszShortName = pname;
	
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
			if (DoMenu_mshell(this, menu1, conn, pname, sname, (sessionFlags & SESSFLAG_LAN) != 0, false) == 0)
			{
				playMenuMusic();
				StopNetConnection(false);
				SetVisible(true);
				setFocus(checkCreate);
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
U32 __stdcall DoMenu_sess (Frame * parent, const GT_MENU1 & data, const SAVED_CONNECTION * conn, U32 flags, wchar_t * szPlayerName)
{
	Menu_sess * dlg = new Menu_sess(parent, data, conn, flags, szPlayerName);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}
//--------------------------------------------------------------------------//
//-----------------------------End Menu_netconn.cpp-------------------------//
//--------------------------------------------------------------------------//