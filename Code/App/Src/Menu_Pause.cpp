//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Pause.cpp                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Pause.cpp 23    5/04/01 2:59p Tmauer $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include <DPause.h>
#include "Hotkeys.h"

#include "NetPacket.h"
#include "DrawAgent.h"

#include <Dplay.h>

#include <stdio.h>

//--------------------------------------------------------------------------//
//
struct MenuPause : public DAComponent<Frame>, IPlayerStateCallback
{
	//
	// data items
	//
	GT_PAUSE data;

	COMPTR<IStatic> staticDescription[MAX_PLAYERS], staticTitle;

	bool bLocalPause;

	//
	// instance methods
	//

	MenuPause (void)
	{
		eventPriority = EVENT_PRIORITY_PAUSEMENU;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~MenuPause (void);

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

	/* IPlayerStateCallback */
	virtual bool EnumeratePlayer (struct NP_PLAYERINFO & info, void * context);

	/* MenuPause methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	static U32 getPlayerName (DPID id, wchar_t szName[64])
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
				wcsncpy(szName, pName->lpszShortName, 64-1);
			::free(pName);
		}
		return wcslen(szName);
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		// we can only unpause the game if we were the one that paused it
		if (bLocalPause && EVENTSYS)
		{
			EVENTSYS->Post(CQE_HOTKEY, (void*)IDH_PAUSE_GAME);
		}
		else
		if (CQFLAGS.bGameActive)
		{
			U32 igResult = CreateMenu_igoptions();
			if(igResult)// player wants to resign
			{
				if (EVENTSYS)
				{
					if(igResult == 1)
					{
						EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_RESIGN);			// go back to shell
						EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_RESIGN);		// send result code
					}
					else
					{
						EVENTSYS->Send(CQE_DLG_RESULT, (void *)MISSION_END_QUIT);			// go back to shell
						EVENTSYS->Send(CQE_MISSION_ENDING, (void *)MISSION_END_QUIT);		// send result code
					}
				}
			}
		}
		return true;
	}

	void init (void);

	GENRESULT __stdcall Notify (U32 message, void *param);
};
//----------------------------------------------------------------------------------//
//
MenuPause::~MenuPause (void)
{
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuPause::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
bool MenuPause::EnumeratePlayer (struct NP_PLAYERINFO & info, void * context)
{
	int * i = (int *) context;

	if (info.dwFlags & (NPPI_PAUSED|NPPI_TURTLED))
	{
		wchar_t buffer[256];
		wchar_t format[64];
		wchar_t reason[64];
		wchar_t time[64];

		if (info.dwFlags & NPPI_HOST)
		{
			time[0] = 0;
			wcsncpy(format, _localLoadStringW(IDS_FORMAT_HOSTPAUSED), sizeof(format)/sizeof(wchar_t));
		}
		else
		{
			S32 timeLeft = info.bootTime / 1000;
			U32 minutes = (timeLeft/60);
			U32 seconds = (timeLeft%60);

			swprintf(time, L"%d:%02d", minutes, seconds);
			wcsncpy(format, _localLoadStringW(IDS_FORMAT_TIMETOBOOT), sizeof(format)/sizeof(wchar_t));
		}

		if (info.dwFlags & NPPI_PAUSED)
			wcsncpy(reason, _localLoadStringW(IDS_PLAYER_PAUSED), sizeof(reason)/sizeof(wchar_t));
		else
			wcsncpy(reason, _localLoadStringW(IDS_PLAYER_CONGESTED), sizeof(reason)/sizeof(wchar_t));

		swprintf(buffer, format, info.szPlayerName, reason, time);

		staticDescription[*i]->SetTextColor(MISSION->GetPlayerColorForDPID(info.dpid));
		staticDescription[*i]->SetText(buffer);

		i[0]++;
	}

	return *i < MAX_PLAYERS;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuPause::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_ENDFRAME:
		if (PLAYERID != 0)
		{
			int i = 0;
			NETPACKET->EnumeratePlayers(this, &i);

			while (i < MAX_PLAYERS)
			{
				staticDescription[i]->SetText(NULL);
				i++;
			}
		}
		/*
		if (PLAYERID != 0)
		{
			wchar_t name[64];
			U32 dpid;
			U32 timeLeft;
			bool bTestPause;

			if ((timeLeft = NETPACKET->GetTimeUntilBooting(&dpid, &bTestPause)) != 0)
			{
				if (bLocalPause)
				{
					bTestPause = true;
					timeLeft = NETPACKET->GetPauseTimeForPlayer(PLAYERID);
					dpid = PLAYERID;
				}
				
				if (bTestPause)
				{
					staticTitle->SetText(_localLoadStringW(IDS_GAMEPAUSED));
				}
				else  // game is implicitly paused by net congestion
				{
					staticTitle->SetText(_localLoadStringW(IDS_NETCONGESTION));
				}

				if (dpid == HOSTID)
				{
					if (bTestPause)
						staticDescription->SetTextID(IDS_STATIC_HOSTPAUSED);
					else	// the host is causing the net congestion, there is no countdown for him
						staticDescription->SetTextID(IDS_STATIC_HOSTCONGESTED);
				}
				else if (getPlayerName(dpid, name))
				{
					wchar_t buffer[256];

					timeLeft /= 1000;
					U32 minutes = (timeLeft/60);
					U32 seconds = (timeLeft%60);

					swprintf(buffer, _localLoadStringW(IDS_FORMAT_TIMETOBOOT), minutes, seconds, name);
					staticDescription->SetText(buffer);
				}
			}

		}
		*/
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void MenuPause::setStateInfo (void)
{
	int i;
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	staticTitle->InitStatic(data.staticTitle, this);
	for (i = 0; i < MAX_PLAYERS; i++)
		staticDescription[i]->InitStatic(data.staticDescription[i], this);

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void MenuPause::init (void)
{
	int i;
	data = 	*((GT_PAUSE *) GENDATA->GetArchetypeData("MenuPause"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.staticTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticTitle);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		GENDATA->CreateInstance(data.staticDescription[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticDescription[i]);
	}
}
//--------------------------------------------------------------------------//
//
Frame * __stdcall CreateMenuPause (int pauseState)
{
	MenuPause * menu = new MenuPause;

	switch (pauseState)
	{
	case 0:		// local pause
		menu->bLocalPause = true;
		break;
	
	case 1:		// net congestion
		break;

	case 2:
		break;
	}

	menu->createViewer("\\GT_PAUSE\\MenuPause", "GT_PAUSE", IDS_VIEWPAUSE);

	return menu;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_Pause.cpp---------------------------//
//--------------------------------------------------------------------------//
