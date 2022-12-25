//--------------------------------------------------------------------------//
//                                                                          //
//                            Menu_zone.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_zone.cpp 27    6/12/01 12:49p Tmauer $
*/
//--------------------------------------------------------------------------//
// network session options, after connection is chosen
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IEdit2.h"
#include <DMenu1.h>
#include "NetConnectBuffers.h"

#include <dplobby.h>
#include <shellapi.h>

#define ZONE_QUIT 10 //also defined in menu1.cpp

U32 __stdcall DoMenu_mshell (Frame * parent, const GT_MENU1 & data, const struct SAVED_CONNECTION * conn, const wchar_t * szPlayerName, const wchar_t * szSessionName, bool bLAN, bool bZone);

#define IDS_DOINIT  0x12000	// some number over 64k
#define IDZONE_PAGE 10
//--------------------------------------------------------------------------//
//
struct Menu_zone : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1 & menu1;
	const GT_MENU1::GAME_ZONE & data;

	COMPTR<IStatic> description;

	//
	// zone related data
	//
	bool bWaiting;
	bool bInitPending;
	bool bZoneStarted;
	bool bFrameDrawnWhilePending;
	volatile bool bThreadCancel;		// request to kill thread (set by default thread)
	volatile bool bConnectionMade;		// set by worker thread
	HANDLE hThread, hProcess;

	//
	// instance methods
	//

	Menu_zone (Frame * _parent, const GT_MENU1 & _data) : data(_data.gameZone), menu1(_data)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_zone (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	GENRESULT __stdcall Notify (U32 message, void *param);

	/* Menu_zone methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID)
	{
		switch (buttonID)
		{
		case IDS_NEXT:
			if (hThread==0)
				startZone();
			break;

		case IDS_DOINIT:
			if (bInitPending)
				initLobby();
			break;

		case IDS_CANCEL:
			if (hThread!=0)
			{
				stopWaitingThread();
				endDialog(0);
			}
			break;
		}
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		SetVisible(true);
		return true;
	}

	virtual void onDraw (void)
	{
		if (bZoneStarted==0)
		{
			bZoneStarted = true;
			PostMessage(CQE_BUTTON, (void *)IDS_NEXT);
		}
		if (bInitPending)
			PostMessage(CQE_BUTTON, (void *)IDS_DOINIT);
	}

	virtual void onUpdate (U32 dt)   // dt in milliseconds
	{
		if (bConnectionMade)
		{
			stopWaitingThread();
			bConnectionMade = false;
			bInitPending = true;		// wait until one frame has drawn
			description->SetText(_localLoadStringW(IDS_HELP_WARNLOBBYDELAY));
		}
	}

	void init (void);

	void startZone (void);

	void initLobby (void);

	void stopWaitingThread (void);

	void startWaitingThread (void);

	void waitThreadMain (void);

	void getSessionNames (wchar_t szPlayerName[64], wchar_t szSessionName[128]);

	static DWORD WINAPI threadProc (LPVOID lpParameter)
	{
		struct Menu_zone * _this = (Menu_zone *) lpParameter;

		_this->waitThreadMain();
		return 0;
	}
};
//----------------------------------------------------------------------------------//
//
GENRESULT __stdcall Menu_zone::Notify (U32 message, void *param)
{
	MSG * msg = (MSG *)param;

	switch (message)
	{
	case WM_ACTIVATEAPP:
		if (bWaiting && msg->wParam!=0)
			PostMessage(CQE_BUTTON, (void *)IDS_CANCEL);
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
Menu_zone::~Menu_zone (void)
{
	if (hThread)
		stopWaitingThread();
}
//----------------------------------------------------------------------------------//
//
void Menu_zone::setStateInfo (void)
{
	//screenRect = data.screenRect;
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	//
	// initialize in draw-order
	//

	description->InitStatic(data.description, this);

	if (childFrame)
		childFrame->setStateInfo();

}
//--------------------------------------------------------------------------//
//
void Menu_zone::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.description.staticType, pComp);
	pComp->QueryInterface("IStatic", description);
	
	setStateInfo();

	description->SetText(_localLoadStringW(IDS_HELP_ZONEPENDING));

	CQFLAGS.bInsideOutZoneLaunch = 0;
}
//--------------------------------------------------------------------------//
//
void Menu_zone::startZone (void)
{
	wchar_t addressw[256];
	char address[256];

	CQASSERT(DPLOBBY);

	startWaitingThread();

	address[0] = 0;
	DEFAULTS->GetInstallStringFromRegistry("Zone", address, sizeof(address));
	_localAnsiToWide(address, addressw, sizeof(addressw));
	
	ShowWindow(hMainWindow, SW_MINIMIZE);

//	HINSTANCE hInst = ShellExecute( NULL, "open", address, NULL, NULL, SW_SHOWNORMAL );

	SHELLEXECUTEINFO info;
	memset(&info, 0, sizeof(info));

	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = "open";
	info.lpFile = address;
	info.nShow = SW_SHOWNORMAL;

	if (ShellExecuteEx(&info)==0)
//	if ( (int) hInst <= 32 )
	{
		stopWaitingThread();

		CQMessageBox(IDS_HELP_LAUNCHFAILED, IDS_APP_NAMETM, MB_OK);

		endDialog(0);
		return;
	}

	hProcess = info.hProcess;
}
//--------------------------------------------------------------------------//
//
void Menu_zone::stopWaitingThread (void)
{
	if (hThread)
	{
		bThreadCancel = true;
		WaitForSingleObject(hThread, INFINITE);
		::CloseHandle(hThread);
		hThread=0;
	}

	if (hProcess)
		CloseHandle(hProcess);
	hProcess = 0;
}
//--------------------------------------------------------------------------//
//
void Menu_zone::startWaitingThread (void)
{
	DWORD dwThreadID;

	CQASSERT(hThread==0);

	bConnectionMade = bWaiting = bThreadCancel = false;

	hThread = CreateThread(0,4096, threadProc, (LPVOID)this, 0, &dwThreadID);
	CQASSERT(hThread);

	while (bWaiting == false && bConnectionMade == false)
	{
		DWORD dwCode;
		
		if (GetExitCodeThread(hThread, &dwCode)==0 || dwCode != STILL_ACTIVE)
			CQBOMB0("Thread terminated early?");
		Sleep(0);
	}
}
//--------------------------------------------------------------------------//
// background thread
//
void Menu_zone::waitThreadMain (void)
{
	DPLOBBY->WaitForConnectionSettings(0);		// start wait mode
	bWaiting = true;

	while (bThreadCancel==false && bConnectionMade==false)
	{
		HRESULT hr;
		DWORD dwFlags;
		DWORD dwSize;

		dwSize = 0;
		hr = DPLOBBY->ReceiveLobbyMessage(0, 0, &dwFlags, 0, &dwSize);
		CQASSERT(hr == DP_OK || hr == DPERR_NOMESSAGES || hr == DPERR_BUFFERTOOSMALL);

		if ((hr == DP_OK||hr == DPERR_BUFFERTOOSMALL) && dwSize)
		{
			DPLMSG_SYSTEMMESSAGE * pMessage = (DPLMSG_SYSTEMMESSAGE *) GlobalAlloc(GPTR, dwSize);
			hr = DPLOBBY->ReceiveLobbyMessage(0, 0, &dwFlags, pMessage, &dwSize);
			CQASSERT(hr == DP_OK);


			if (pMessage->dwType == DPLSYS_NEWCONNECTIONSETTINGS)
			{
				bConnectionMade = true;
				bWaiting = false;

				ShowWindow(hMainWindow, SW_RESTORE);
				SetForegroundWindow(hMainWindow);
			}

			GlobalFree(pMessage);
		}

		if (hProcess && WaitForSingleObject(hProcess, 0) != WAIT_TIMEOUT)
		{
			ShowWindow(hMainWindow, SW_RESTORE);
			SetForegroundWindow(hMainWindow);
		}
		
		Sleep(250);
	}

	if (bWaiting)
		DPLOBBY->WaitForConnectionSettings(DPLWAIT_CANCEL);		// cancel wait mode
}
//--------------------------------------------------------------------------//
// user connected to lobby through our menu system
//
void Menu_zone::initLobby (void)
{
	S32 lobbied;
	DPCAPS dpcaps;
	
	CURSOR->SetBusy(1);
	CQFLAGS.bDPLobby = 1;				// force it use lobby connection
	StartNetConnection(lobbied);	
	CQFLAGS.bDPLobby = 0;
	CURSOR->SetBusy(0);
	bInitPending = 0;

	if (lobbied==0)   // if lobby connection failed (firewall?)
	{
		CQMessageBox(IDS_HELP_LOBBYFAILED, IDS_APP_NAMETM, MB_OK);
		endDialog(0);
	}
	else
	{
		SetVisible(false);

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
		SetVisible(false);
		wchar_t sname[128];
		wchar_t pname[64];
		getSessionNames(pname, sname);

		if (DoMenu_mshell(this, menu1, NULL, pname, sname, false, true) == 0)
			endDialog(0);
		else
		{
			CQFLAGS.bInsideOutZoneLaunch = 1;
			endDialog(1);
		}
		SetVisible(true);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_zone::getSessionNames (wchar_t szPlayerName[64], wchar_t szSessionName[128])
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
U32 __stdcall DoMenu_zone (Frame * parent, const GT_MENU1 & data)
{
/*	Menu_zone * dlg = new Menu_zone(parent, data);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
*/
	wchar_t addressw[256];
	char address[256];

	address[0] = 0;
	DEFAULTS->GetInstallStringFromRegistry("Zone", address, sizeof(address));
	_localAnsiToWide(address, addressw, sizeof(addressw));
	
	SHELLEXECUTEINFO info;
	memset(&info, 0, sizeof(info));

	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
	info.lpVerb = "open";
	info.lpFile = address;
	info.nShow = SW_SHOWNORMAL;

	ShellExecuteEx(&info);

	return ZONE_QUIT;
}
//---------------------------------------------------------------------------------//
//------------------------------------End Menu_zone.cpp----------------------------//
//---------------------------------------------------------------------------------//