//--------------------------------------------------------------------------//
//                                                                          //
//                           Menu_netunloading.cpp                          //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_netunloading.cpp 18    10/24/00 8:34p Jasony $
*/
//--------------------------------------------------------------------------//
// Static memu while unloading net game (waiting for packet delivery)
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "OpAgent.h"
#include "Frame.h"
#include <DStatic.h>
#include "IStatic.h"
#include "NetPacket.h"
#include <mglobals.h>
#include <DCQGame.h>

#include <stdio.h>

using namespace CQGAMETYPES;
//--------------------------------------------------------------------------//
//
struct Menu_nul : public DAComponent<Frame>
{
	//
	// data items
	//
	COMPTR<IStatic> background, unloading;

	bool bReceivedAck[MAX_PLAYERS];

	const bool bResign;

	S32 timeout;
	//
	// instance methods
	//

	Menu_nul (bool _bResign) : bResign(_bResign)
	{
		initializeFrame(NULL);
		init();
	}

	~Menu_nul (void);

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

	/* Menu_nul methods */

	virtual void setStateInfo (void);

	virtual void onUpdate (U32 dt);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		// don't do anything here?
		return true;
	}

	void init (void);

	void onReceivePacket (BASE_PACKET *packet);
};
//----------------------------------------------------------------------------------//
//
Menu_nul::~Menu_nul (void)
{
	CQFLAGS.bGamePaused = 0;
	EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
//	EVENTSYS->Send(CQE_LOCALPAUSED, (void*)0);		// remote users will get the idea when we go away
	CURSOR->SetBusy(0);
}
//----------------------------------------------------------------------------------//
//
void Menu_nul::setStateInfo (void)
{
	screenRect.left = 0;
	screenRect.right = SCREEN_WIDTH-1;
	screenRect.top = 0;
	screenRect.bottom = SCREEN_HEIGHT-1;

	//
	// initialize in draw-order
	//

	STATIC_DATA data;

	strcpy(data.staticType,"Static!!LoadingBackground");
	data.staticText = STTXT::NO_STATIC_TEXT;
	data.alignment = STATIC_DATA::TOPLEFT;
	data.xOrigin = data.yOrigin = 0;
	data.width = data.height = 0;

	background->InitStatic(data, this);

	strcpy(data.staticType, "Static!!GrayedRect");
	data.xOrigin = (SCREEN_WIDTH-250)/2;
	data.yOrigin = (SCREEN_HEIGHT-50)/3;
	data.width = 250;
	data.height = 50;

	unloading->InitStatic(data, this);

	unloading->SetTextID(IDS_STATIC_SHUTDOWNWAIT);

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_nul::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance("Static!!LoadingBackground", pComp);
	pComp->QueryInterface("IStatic", background);
	GENDATA->CreateInstance("Static!!GrayedRect", pComp);
	pComp->QueryInterface("IStatic", unloading);

	CQFLAGS.bGamePaused = 1;
	EVENTSYS->Send(CQE_LOCALPAUSED, (void*)1);
	EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));

	CURSOR->SetBusy(1);

	setStateInfo();

	U32 slot = MGlobals::GetSlotIDFromDPID(PLAYERID);
	if (slot < MAX_PLAYERS)
		bReceivedAck[slot] = true;
}
//--------------------------------------------------------------------------//
//
void Menu_nul::onUpdate (U32 dt)
{
	if (bResign)
	{
		if ((timeout += dt) > 1000)
		{
			S32 i;
			bool bPacketSent = false;
			const struct CQGAME & cqgame = MGlobals::GetGameSettings();

			timeout -= 1000;
			//
			// check to see if all ack's have been received, if not send more packets
			//
			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (bReceivedAck[i] == false && cqgame.slot[i].state == READY && cqgame.slot[i].dpid)
				{
					BASE_PACKET packet;
					bPacketSent = true;
			
					packet.dwSize = sizeof(packet);
					packet.type = PT_RESIGN;
					NETPACKET->Send(cqgame.slot[i].dpid, 0, &packet);
				}
			}

			if (bPacketSent==0)
			{
				if (NETPACKET->GetPacketsInTransit(0, NETF_ALLREMOTE) == 0)
					endDialog(0);
			}
		}
	}
	else
	{
		if (NETPACKET->GetPacketsInTransit(0, NETF_ALLREMOTE) == 0)
			endDialog(0);
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_nul::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_NETPACKET:
		onReceivePacket((BASE_PACKET *)param);
		break;
	}
	return Frame::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Menu_nul::onReceivePacket (BASE_PACKET *packet)
{
	if (packet->type == PT_RESIGNACK)
	{
		U32 slot = MGlobals::GetSlotIDFromDPID(packet->fromID);

		if (slot < MAX_PLAYERS)
			bReceivedAck[slot] = true;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_nul (bool bResign)
{
	if (bResign)
	{
		BASE_PACKET packet;
		packet.dwSize = sizeof(packet);
		packet.type = PT_RESIGN;
		NETPACKET->Send(0, NETF_ALLREMOTE, &packet);
	}

	Menu_nul * dlg = new Menu_nul(bResign);
	dlg->beginModalFocus();

	U32 result = CQDoModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------End Menu_netloading.cpp----------------------------//
//--------------------------------------------------------------------------//
