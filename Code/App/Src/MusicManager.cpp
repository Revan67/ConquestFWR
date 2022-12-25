//                                                                          //
//                            MusicManager.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/MusicManager.cpp 24    4/18/01 8:58a Tmauer $

   High level music player. Streams .WAV files in the background.
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "MusicManager.h"
#include "EventPriority.h"
#include "TDocClient.h"
#include "UserDefaults.h"
#include "Resource.h"
#include "Menu.h"
#include "Startup.h"
#include <DMusic.h>
#include "BaseHotRect.h"
#include "Mission.h"
#include <DMissionEnum.h>
#include "SoundManager.h"

#include <Streamer.h>
#include <TComponent.h>
#include <IConnection.h>
#include <HeapObj.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Viewer.h>
#include <ViewCnst.h>
#include <Document.h>
#include <IDocClient.h>
#include <EventSys.h>
#include <MemFile.h>

static char szKey[] = "Music";


struct DACOM_NO_VTABLE MManager : IMusicManager, IEventCallback, DocumentClient
{
	BEGIN_DACOM_MAP_INBOUND(MManager)
	DACOM_INTERFACE_ENTRY(IMusicManager)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;

	U32 menuID;
	U32 eventHandle;
	BOOL32 bIgnoreUpdate, bActive;
	SINGLE musicVolume;

	MT_MUSIC_DATA data;

	HSTREAM loadTimeHandle;

	char currentMusic[MAX_PATH];

	//------------------------

	MManager (void);

	~MManager (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IMusicManager methods */

	DEFMETHOD_(void,Enable) (BOOL32 bEnable);

	DEFMETHOD_(void,SetVolume) (SINGLE volume);

	virtual void __stdcall Load (struct IFileSystem * inFile);
	
	virtual void __stdcall LoadFinish ();

	virtual void __stdcall Save (struct IFileSystem * outFile);

	virtual void __stdcall InitPlaylist (enum M_RACE race);

	virtual void __stdcall PlayMusic (const char * fileName, bool bSmoothTransition);

	virtual void __stdcall GetMusicFileName (struct M_STRING & string);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	/* MManager methods */

	BOOL32 CreateViewer (void);

	void handleStreamerNotify (HSTREAM hStream, IStreamer::STATUS state);

	IDAComponent * getBase (void)
	{
		return static_cast<IMusicManager *>(this);
	}


};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
MManager::MManager (void)
{
	musicVolume = 1.0;
	loadTimeHandle = 0;
}
//--------------------------------------------------------------------------//
//
MManager::~MManager (void)
{
	if(loadTimeHandle)
	{
		STREAMER->CloseHandle(loadTimeHandle);
		loadTimeHandle = 0;
	}
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}
}
//--------------------------------------------------------------------------//
//
GENRESULT MManager::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_STREAMER:
		handleStreamerNotify((HSTREAM)msg->lParam, (IStreamer::STATUS)msg->wParam);
		break;

	case WM_CLOSE:
		if (viewer)
			viewer->set_display_state(0);
		break;

#if 0		
	case WM_CHAR:
		if (msg->wParam == 'p')
			STREAMER->Open(data[1].filename, MUSICDIR);
		break;
#endif

	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			if (viewer)
				viewer->set_display_state(1);
		}
		break;
	};

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
void MManager::Enable (BOOL32 bEnable)
{
	if (bEnable == bActive)
		return;				// no change

	if ((bActive = bEnable) == 0)
	{
		// disable all streams
		int i;

		for (i = 0; i < NUM_SONGS; i++)
			if (data[i].handle)
				STREAMER->Stop(data[i].handle);
	}
	else
	if (doc)		// might not be initialized yet
		OnUpdate(doc);
}
//--------------------------------------------------------------------------//
//
void MManager::SetVolume (SINGLE volume)
{
	if (musicVolume != volume)
	{
		int i;

		musicVolume = volume;

		for (i = 0; i < NUM_SONGS; i++)
			if (data[i].handle)
				STREAMER->SetVolume(data[i].handle, ConvertFloatToDSound(data[i].volume * musicVolume));
	}
}
//--------------------------------------------------------------------------//
//
void MManager::Load (struct IFileSystem * inFile)
{
#ifndef _DEMO_
	HANDLE hFile=INVALID_HANDLE_VALUE;
	DAFILEDESC fdesc = "Music";
	DWORD dwRead;
	char buffer[1024];
	int i;

	if (inFile->SetCurrentDirectory("\\MT_MUSIC_DATA") == 0)
		goto Done;

	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	if (inFile->GetFileSize(hFile) == 0)
		goto Done;

//	SOUNDMANAGER->TweekMusicVolume();

	for (i = 0; i < NUM_SONGS; i++)
		if (data[i].handle)
		{
			if(data[i].playing && !loadTimeHandle)
			{
				loadTimeHandle = data[i].handle;
			}
			else
			{
				STREAMER->CloseHandle(data[i].handle);
			}
		}

	inFile->ReadFile(hFile, buffer, sizeof(buffer), &dwRead, 0);
	
	MISSION->CorrelateSymbol("MT_MUSIC_DATA", buffer, &data);

	for (i = 0; i < NUM_SONGS; i++)
		data[i].handle = 0;

Done:
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);

	// null out or current file so that the right music gets a chance to be played
	memset(currentMusic, 0, sizeof(currentMusic));
#endif  // !_DEMO_
}

//--------------------------------------------------------------------------//
//

void MManager::LoadFinish ()
{
	if(loadTimeHandle)
	{
		STREAMER->CloseHandle(loadTimeHandle);
		loadTimeHandle = 0;
	}
	doc->UpdateAllClients();
}

//--------------------------------------------------------------------------//
//
void MManager::Save (struct IFileSystem * outFile)
{
	HANDLE hFile=INVALID_HANDLE_VALUE;
	DAFILEDESC fdesc = "Music";
	DWORD dwWritten;
	int i;

	fdesc.dwDesiredAccess |= GENERIC_WRITE;
	fdesc.dwShareMode = 0;
	fdesc.dwCreationDistribution = CREATE_ALWAYS;

	outFile->CreateDirectory("\\MT_MUSIC_DATA");

	if (outFile->SetCurrentDirectory("\\MT_MUSIC_DATA") == 0)
		goto Done;

	if ((hFile = outFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;

	for (i = 0; i < NUM_SONGS; i++)
		if (data[i].playing)
		{
			// only save struct if at least one song is playing
			outFile->WriteFile(hFile, &data, sizeof(data), &dwWritten, 0);
			break;
		}

Done:
	if (hFile != INVALID_HANDLE_VALUE)
		outFile->CloseHandle(hFile);
}
//--------------------------------------------------------------------------//
//
void MManager::InitPlaylist (enum M_RACE race)
{
	const char *fileName = 0;
	
	switch (race)
	{
	case M_TERRAN:
		fileName = "terrangame14.wav";
		break;
	case M_MANTIS:
		fileName = "mantisgame14.wav";
		break;
	case M_SOLARIAN:
		fileName = "solarian_Game14.wav";
		break;
	}

	PlayMusic(fileName, true);
}
//--------------------------------------------------------------------------//
//
void MManager::PlayMusic (const char * fileName, bool bSmoothTransition)
{
#ifdef _DEMO_
	if (fileName)
		fileName = "demomusic.wav";		// override filename for demo
#endif
	if (fileName)
	{
		CQTRACE11("Attempting to play music file   %s", fileName);

		// if the filename is the same as what is already playing, then ignore it
		if (strcmp(fileName, currentMusic) == 0)
		{
			return;
		}
		else
		{
			strcpy(currentMusic, fileName);
		}

		int i;
		if (bSmoothTransition)
			SOUNDMANAGER->TweekMusicVolume();
		
		for (i = 0; i < NUM_SONGS; i++)
		{
			if (data[i].handle)
			{
				STREAMER->CloseHandle(data[i].handle);
				data[i].handle = 0;
				data[0].looping = 0;
				data[0].playing = 0;
				data[0].filename[0] = 0;
			}
		}

		strncpy(data[0].filename, fileName, sizeof(data[0].filename));
		data[0].volume = 1;
		data[0].playing = 1;
		data[0].looping = 1;

		doc->UpdateAllClients();
	}
	else	// just shutdown all music
	{
		// zero out the current music file
		memset(currentMusic, 0, sizeof(currentMusic));

		int i;
		if (bSmoothTransition)
			SOUNDMANAGER->TweekMusicVolume();
		
		for (i = 0; i < NUM_SONGS; i++)
		{
			if (data[i].handle)
			{
				STREAMER->CloseHandle(data[i].handle);
				data[i].handle = 0;
				data[0].looping = 0;
				data[0].playing = 0;
				data[0].filename[0] = 0;
			}
		}

		doc->UpdateAllClients();
	}
}
//--------------------------------------------------------------------------//
//
void MManager::GetMusicFileName (struct M_STRING & string)
{
	int i;
	string.string[0] = 0;

	for (i = 0; i < NUM_SONGS; i++)
		if (data[i].playing)
		{
			string = data[i].filename;
			break;
		}

}
//--------------------------------------------------------------------------//
// data has changed. do something about it
//
GENRESULT MManager::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	if (bActive && bIgnoreUpdate == 0)
	{
		int i;

		bIgnoreUpdate++;

		for (i = 0; i < NUM_SONGS; i++)
		{
			if (data[i].playing == 0)
			{
				if (data[i].handle)
				{
					STREAMER->CloseHandle(data[i].handle);
					data[i].handle = 0;
					data[0].looping = 0;
				}
			}
			else // user wants to play this one
			{
				if (data[i].handle==0)
				{
					U32 flags = (data[i].looping) ? (STRMFL_PLAY|STRMFL_LOOPING) : STRMFL_PLAY;
					data[i].handle = STREAMER->Open(data[i].filename, MUSICDIR, flags);
				}
				else	// currently has a valid handle, is it playing?
				{
					switch (STREAMER->GetStatus(data[i].handle))
					{
					case IStreamer::PLAYING:
					case IStreamer::EOFREACHED:
						break;

					case IStreamer::STOPPED:
						STREAMER->Restart(data[i].handle);
						break;
					}
				}

				if (data[i].handle)
					STREAMER->SetVolume(data[i].handle, ConvertFloatToDSound(data[i].volume * musicVolume));
			}
		}

		doc->UpdateAllClients(0);
		bIgnoreUpdate--;
	}
	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL32 MManager::CreateViewer (void)
{
	DOCDESC ddesc = "MusicData";
	COMPTR<IFileSystem> pMemFile;

	// create a memory file

	MEMFILEDESC mdesc = ddesc.lpFileName;
	mdesc.dwDesiredAccess  = GENERIC_WRITE | GENERIC_READ;
	mdesc.lpBuffer = &data;
	mdesc.dwBufferSize = sizeof(data);
	mdesc.dwFlags = CMF_DONT_COPY_MEMORY;
	if (DACOM->CreateInstance(&mdesc, pMemFile) != GR_OK)
		return 0;

	pMemFile->AddRef();
	ddesc.lpParent = pMemFile;
	ddesc.dwDesiredAccess  = GENERIC_WRITE | GENERIC_READ;
	ddesc.dwCreationDistribution = CREATE_ALWAYS;
	ddesc.lpImplementation = "DOS";

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "MT_MUSIC_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **)&hwnd);
			MoveWindow(hwnd, 100, 100, 400, 200, 1);

			viewer->set_instance_name("Music data");

			MakeConnection(doc);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
//
void MManager::handleStreamerNotify (HSTREAM hStream, IStreamer::STATUS state)
{
	int i;
	BOOL32 bDataChanged=0;
	S32 index = -1;
	BOOL32 bPlaying = 0;

	CQASSERT(hStream);
	//
	// close streams that have completed
	//
	for (i = 0; i < NUM_SONGS; i++)
	{
		if (data[i].handle == hStream)
		{
			switch (state)
			{
			case IStreamer::EOFREACHED:
				index = i;
				break;

	 		case IStreamer::INVALID:
				data[i].filename[0] = 0;
				// fall through intentional
	 		case IStreamer::COMPLETED:
				STREAMER->CloseHandle(data[i].handle);
				data[i].playing = data[i].looping = 0;
				data[i].handle = 0;
				bDataChanged = 1;
				break;
			}
		}
		else
		if (data[i].playing)
			bPlaying = 1;
	}

	//
	// if active and not currently playing a tune, start a new one
	//

	if (bActive && bPlaying == 0)
	{
		int j=++index;
		if (j >= NUM_SONGS)
			j = index = 0;
		do
		{
			if (data[j].filename[0])
			{
				if (data[j].playing == 0)
				{
					data[j].playing = 1;
					bDataChanged = 1;
				}
				break;
			}
			j++;
			if (j >= NUM_SONGS)
				j = 0;
		} while (index != j);
	}

	if (bDataChanged)
		OnUpdate(doc);
}
//--------------------------------------------------------------------------//
//
struct _music : GlobalComponent
{
	MManager * manager;

	virtual void Startup (void)
	{
		MUSICMANAGER = manager = new DAComponent<MManager>;
		AddToGlobalCleanupList((IDAComponent **)&MUSICMANAGER);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
		MENUITEMINFO minfo;

		if (manager->CreateViewer() == 0)
			CQBOMB0("Viewer could not be created.");

		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDS_VIEWMUSIC;
		minfo.dwTypeData = "Music";
		minfo.cch = 5;	// length of string "Music"
			
		if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
			manager->menuID = IDS_VIEWMUSIC;

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(manager->getBase(), &manager->eventHandle);
		FULLSCREEN->SetCallbackPriority(manager,EVENT_PRIORITY_MUSIC);
	}
};

static _music music;
//--------------------------------------------------------------------------//
//-------------------------END MusicManager.cpp-----------------------------//
//--------------------------------------------------------------------------//
