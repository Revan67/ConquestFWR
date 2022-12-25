//--------------------------------------------------------------------------//
//                                                                          //
//                            Menu_netloading.cpp                           //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_netloading.cpp 54    11/10/00 1:21p Jasony $
*/
//--------------------------------------------------------------------------//
// Static memu while loading net game
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IStatic.h"
#include <DMenu1.h>
#include "CQGame.h"
#include "Mission.h"
#include "GRPackets.h"
#include "UserDefaults.h"
#include "NetBuffer.h"
#include "Menu.h"
#include "NetFileTransfer.h"
#include "GenData.h"
#include "MusicManager.h"
#include "InprogressAnim.h"

#include <dplay.h>
#include <dplobby.h>
#include "ZoneLobby.h"
#include <stdio.h>

using namespace CQGAMETYPES;

void __stdcall EnableLogging (const FULLCQGAME & cqgame, U32 seed);


// turn off "global" optimizations to make the recursive onNetPacket() call work correctly
#ifndef _DEBUG
#pragma optimize( "g", off )
#endif

//--------------------------------------------------------------------------
//
#define BEGIN_MAPPING(parent, name) \
	{								\
		HANDLE hFile, hMapping;		\
		DAFILEDESC fdesc = name;	\
		void *pImage=0;				\
		if ((hFile = parent->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)	\
		{ hMapping = INVALID_HANDLE_VALUE; }	\
		else \
		{ \
		hMapping = parent->CreateFileMapping(hFile);	\
		pImage = parent->MapViewOfFile(hMapping);	\
		}
			

#define END_MAPPING(parent) \
		parent->UnmapViewOfFile(pImage); \
		parent->CloseHandle(hMapping); \
		parent->CloseHandle(hFile); }
//--------------------------------------------------------------------------//
//
struct Menu_nl : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1 & menu1;
	ICQGame & cqgame;
	const U32 remoteCheckSum;
	U32 localCheckSum;
	const U32 remoteRandomSeed;
	U32 localRandomSeed;
	COMPTR<IPANIM> pIPAnim;

	bool bStarted;
	bool bLoadSuccessful;
	int  transfersPending;		// number of FTP completions needed before we move on
	int  initsPending;
	U32  percentDone;
	bool bInitialized;

	struct FTENUMERATOR : IFileTransferEnumerator
	{
		U32 totalBytes;
		U32 totalTransfered;
		U32 numChannels;

		FTENUMERATOR (void)
		{
			totalBytes = totalTransfered = numChannels = 0;
		}

		BOOL32 __stdcall EnumerateFTChannel (const struct FTCHANNELDEF & channel)
		{
			totalBytes += channel.fileSize;
			totalTransfered += channel.bytesTransfered;
			numChannels++;
			return 1;
		}
	};

	//
	// instance methods
	//

	Menu_nl (Frame * _parent, const GT_MENU1 & _data, ICQGame & _cqgame, U32 _checkSum, U32 _seed) : menu1(_data), cqgame(_cqgame), remoteCheckSum(_checkSum), remoteRandomSeed(_seed)
	{
		initializeFrame(_parent);
		init();
	}

	~Menu_nl (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* Menu_nl methods */

	virtual GENRESULT __stdcall Notify (U32 message, void *param)
	{
		switch (message)
		{
		case CQE_DPLAY_MSGWAITING:
			onLocalUpdate();
			break;
		case CQE_NETPACKET:
			onNetPacket((const BASE_PACKET *)param);
			break;
		case WM_CLOSE:
			pIPAnim.free();		// make sure background drawing is shutdown
			break;
		} // end switch (message)


		return Frame::Notify(message, param);
	}

	virtual void setStateInfo (void);

	virtual void onUpdate (U32 dt)
	{
		onLocalUpdate();
	}

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		// don't do anything here?
		return true;
	}


	void init (void);

	void onNetPacket (const BASE_PACKET *packet);
	void onLocalUpdate (void);
	void startLoadProcess (void);
	static bool getcheckSum (IFileSystem * missionDir, const char * filename, U32 & checkSum);
};
//----------------------------------------------------------------------------------//
//
Menu_nl::~Menu_nl (void)
{
	// 
	// make sure progress animation is stopped (before potentially switching back to 2D mode)!
	// 
	MISSION->CancelProgressAnim();
	pIPAnim.free();
	//
	// do all cleanup needed if we failed to start game
	//
	if (bLoadSuccessful == false)
	{
		MISSION->Close();
		MISSION->CancelNetDownload();
		ChangeInterfaceRes(IR_FRONT_END_RESOLUTION);

		if (HOSTID!=PLAYERID)
		{
			GR_PACKET packet(CANCELLOAD);
			NETPACKET->Send(HOSTID, 0, &packet);
			CQTRACE50("Sending cancel packet.");
		}
		else
		{
			GR_PACKET packet(CANCELLOAD);
			NETPACKET->Send(0, NETF_ALLREMOTE, &packet);
			CQTRACE50("Sending cancel packet.");
		}
	}
	else
	{
		if (ZONESCORE)
		{
			ZONESCORE->SendGameState(DPLOBBY, ZSTATE_START);
		}
	}

#ifdef _DEBUG
	NETBUFFER->EnableThroughputLimiting(true);	
#endif

	CURSOR->SetBusy(0);
//	CURSOR->EnableCursor(true);
}
//----------------------------------------------------------------------------------//
//
void Menu_nl::setStateInfo (void)
{
	screenRect = FULLSCREEN->screenRect;

	//
	// initialize in draw-order
	//

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_nl::init (void)
{
	GENDATA->FlushUnusedArchetypes();
	CURSOR->SetBusy(true);
	ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);

	MISSION->StartProgressAnim(pIPAnim);

	MUSICMANAGER->PlayMusic("Battle_loading.wav", false);
	
	setStateInfo();
	bInitialized = true;
}
//--------------------------------------------------------------------------//
//
void Menu_nl::onNetPacket (const BASE_PACKET *_packet)
{
	if (_packet->type == PT_GAMEROOM)
	{
		const GR_PACKET * packet = (const GR_PACKET *) _packet;

		switch (packet->subtype)
		{
		case DOWNLOADCOMPLETE:
			if (HOSTID == PLAYERID)
			{
				transfersPending--;
				CQASSERT(transfersPending>=0);

				if (transfersPending == 0)
				{
					GR_PACKET packet(INITGAME);
					packet.dwSize = sizeof(packet);

					CQTRACE50("Sending INITGAME.");
					NETPACKET->Send(0,NETF_ALLREMOTE,&packet);
					MISSION->InitializeNetGame(cqgame, localRandomSeed);

					if (initsPending==0)
					{
						initsPending = 1;
						GR_PACKET packet(INITCOMPLETE);
						onNetPacket(&packet);	// simulate receiving a packet
					}
				}
			}
			break;

		case INITCOMPLETE:
			if (HOSTID == PLAYERID)
			{
				CQASSERT(transfersPending==0);
				if (--initsPending <= 0)
				{
					if (bExitCodeSet==0)		// we may have failed for some other reason
					{
						bLoadSuccessful = true;
						endDialog(0);		// success

						GR_PACKET packet(GAMEREADY);
						packet.dwSize = sizeof(packet);

						CQTRACE50("Sending GAMEREADY.");
						NETPACKET->Send(0,NETF_ALLREMOTE,&packet);
					}
				}
			}
			break;

		case INITGAME:
			if (packet->fromID == HOSTID)
			{
				MISSION->InitializeNetGame(cqgame, remoteRandomSeed);

				GR_PACKET packet(INITCOMPLETE);
				packet.dwSize = sizeof(packet);

				CQTRACE50("Sending INITCOMPLETE.");
				NETPACKET->Send(HOSTID, 0,&packet);
			}
			break;

		case GAMEREADY:
			if (packet->fromID == HOSTID)
			{
				if (bExitCodeSet==0)	// we may have failed for some other reason
				{
					CQTRACE50("Received GAMEREADY.");
					bLoadSuccessful = true;
					endDialog(0);	// success!
				}
			}
			break;

		case CANCELLOAD:
			endDialog(IDS_HELP_REMOTELOADFAILED);		// remote user wants to stop this whole thing
			break;
		} // end switch (packet->subtype)
	}
}
//--------------------------------------------------------------------------//
//
void Menu_nl::onLocalUpdate (void)
{
	if (bInitialized==false)
		return;
	if (bStarted == false)
		startLoadProcess();

	// we are a client in download state
	if (HOSTID!=PLAYERID && transfersPending > 0)
	{
		FTSTATUS status;
		status = MISSION->GetDownloadStatus();
		switch (status)
		{
		case FTS_SUCCESS:
			{
				// send confirm
				GR_PACKET packet(DOWNLOADCOMPLETE);
				packet.dwSize = sizeof(packet);
				CQTRACE50("Sending DOWNLOADCOMPLETE.");
				NETPACKET->Send(HOSTID,0,&packet);
				transfersPending--;
				CQASSERT(transfersPending==0);
			}
			break;
		case FTS_CLOSED:
		case FTS_INVALIDFILE:
		case FTS_INVALIDCHANNEL:
			endDialog(IDS_HELP_REMOTELOADFAILED);
			break;
		}
	}

	if (transfersPending>0 || percentDone == 0)
	{
		//
		// now find out ftp progress
		//
		FTENUMERATOR ftenum;
		U32 newPercent;

		FILETRANSFER->EnumerateChannels(&ftenum);

		if (ftenum.totalBytes)
			newPercent = (ftenum.totalTransfered * 100) / ftenum.totalBytes;
		else
			newPercent = 0;

		percentDone = __max(percentDone, newPercent);
		percentDone = __min(100, percentDone);

		if (percentDone > 0)
		{
			pIPAnim->UpdateString(IDS_XFERPROGRESS);
			pIPAnim->SetProgress(SINGLE(percentDone) / 100);
		}
	}
}
//--------------------------------------------------------------------------//
//
void Menu_nl::startLoadProcess (void)
{
	bStarted = true;

	char buffer[256];
	COMPTR<IFileSystem> missionDir;
	_localWideToAnsi(cqgame.szMapName, buffer, sizeof(buffer));
	missionDir = (cqgame.mapType==SELECTED_MAP)  ? MPMAPDIR : SAVEDIR;

	if (HOSTID == PLAYERID)
	{
		START_PACKET packet;
		packet.cqgame = cqgame;
		packet.userDefaults = *DEFAULTS->GetDefaults();
		//
		// generate random seed for generator, player assignments
		//
		packet.randomSeed = localRandomSeed = rand() * GetTickCount();

		CQTRACE50("Enabling download...");

		if (cqgame.mapType == RANDOM_MAP)
		{
			NETPACKET->Send(0,NETF_ALLREMOTE,&packet);
			if (CQFLAGS.bHostRecordMode)
				EnableLogging(cqgame, packet.randomSeed);
			MISSION->GenerateMultiplayerMap(cqgame, packet.randomSeed);
		}
		else
		{
			if (getcheckSum(missionDir, buffer, packet.checkSum))
			{
				NETPACKET->Send(0,NETF_ALLREMOTE,&packet);
				if (CQFLAGS.bHostRecordMode)
					EnableLogging(cqgame, packet.randomSeed);
				if (MISSION->Load(buffer,missionDir) == 0)  // if load failed
				{
					endDialog(IDS_HELP_LOADFAILED);
					return;
				}
			}
			else // load failed
			{
				endDialog(IDS_HELP_LOADFAILED);
				return;
			}
		}

		U32 i;
		for (i = transfersPending = 0; i < cqgame.activeSlots; i++)
		{
			if (i != cqgame.localSlot && cqgame.slot[i].type == HUMAN && (cqgame.slot[i].state==ACTIVE || cqgame.slot[i].state==READY))
				transfersPending++;
		}
		if ((initsPending = transfersPending) == 0)
		{
			transfersPending = 1;
			GR_PACKET packet(DOWNLOADCOMPLETE);
			onNetPacket(&packet);	// simulate receiving a packet
		}
	} 
	else
	{
		if (cqgame.mapType == RANDOM_MAP)
		{
			MISSION->GenerateMultiplayerMap(cqgame, remoteRandomSeed);
			// send confirm
			GR_PACKET packet(DOWNLOADCOMPLETE);
			packet.dwSize = sizeof(packet);
			CQTRACE50("Sending DOWNLOADCOMPLETE.");
			NETPACKET->Send(HOSTID,0,&packet);
		}
		else
		if (getcheckSum(missionDir, buffer, localCheckSum) == false || localCheckSum != remoteCheckSum)
		{
			CQTRACE40("Starting file receive...");
			MISSION->StartNetDownload();
			transfersPending = 1;	// start off in download mode (if we are the client)
		}
		else // use local copy
		{
			if (MISSION->Load(buffer, missionDir) == 0)  // if load failed
			{
				endDialog(IDS_HELP_LOADFAILED);
				return;
			}
			else
			{
				// send confirm
				GR_PACKET packet(DOWNLOADCOMPLETE);
				packet.dwSize = sizeof(packet);
				CQTRACE50("Sending DOWNLOADCOMPLETE.");
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
	}

#ifdef _DEBUG
	NETBUFFER->EnableThroughputLimiting(false);		// let download happen fast!
#endif
}
//--------------------------------------------------------------------------//
//
bool Menu_nl::getcheckSum (IFileSystem * missionDir, const char * filename, U32 & checkSum)
{
	bool result = 0;

	if (missionDir)
	{
		BEGIN_MAPPING(missionDir, filename);

		if (pImage)
		{
			U32 size = missionDir->GetFileSize(hFile) >> 2;
			U32 i;
			U32 * ptr = (U32 *) pImage;

			for (i = 0; i < size; i++)
				checkSum += ptr[i];

			checkSum *= size;

			result = true;
		}

		END_MAPPING(missionDir);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_nl (Frame * parent, const GT_MENU1 & data, ICQGame & cqgame, U32 checkSum, U32 randomSeed)
{
	Menu_nl * dlg = new Menu_nl(parent, data, cqgame, checkSum, randomSeed);
	dlg->beginModalFocus();

	U32 result = CQNonRenderingModal(dlg);
	delete dlg;

	return result;
}

//--------------------------------------------------------------------------//
//-----------------------End Menu_netloading.cpp----------------------------//
//--------------------------------------------------------------------------//
