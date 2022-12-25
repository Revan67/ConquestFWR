//--------------------------------------------------------------------------//
//                                                                          //
//                             SoundManager.cpp                             //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/SoundManager.cpp 62    12/13/00 11:46a Jasony $

   Manager the volume level of various sounds in the game
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "SoundManager.h"
#include "Startup.h"
#include "DBHotkeys.h"
#include "Resource.h"
#include "UserDefaults.h"
#include "SFX.h"
#include "MusicManager.h"
#include "Streamer.h"
#include "GenData.h"
#include "CQTrace.h"
//#include "DSStream.h"
#include "IObject.h"
#include "ObjList.h"
#include "IBriefing.h"
#include "BaseHotRect.h"
#include "ObjWatch.h"

#include "IAnimate.h"
#include "LFParser.h"
#include <DAnimate.h>

#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <HKEvent.h>

#include <commctrl.h>
#include <malloc.h>

#define MIN_VOLUME  0
#define MAX_VOLUME  10
#define DEFAULT_MUSIC_VOLUME 6
#define DEFAULT_COMM_VOLUME  7
#define DEFAULT_SFX_VOLUME	 9
#define DEFAULT_MOVIE_VOLUME 9

static char szRegKey[] = "Volume";


#define VOLUME_RAMP  (0.0005)     // 2 seconds to increase 1 in volume
//--------------------------------------
//
void SOUNDSTATE::init (void)
{
	// default values
	music.volume	= DEFAULT_MUSIC_VOLUME;
	comms.volume	= DEFAULT_COMM_VOLUME;
	effects.volume	= DEFAULT_SFX_VOLUME;
	chat.volume		= DEFAULT_MOVIE_VOLUME;

	music.bMute = 
	effects.bMute = 
	comms.bMute = 
	chat.bMute = false;
};

inline SINGLE scaleVolume (S32 volume)
{
	return SINGLE(volume - MIN_VOLUME) / (MAX_VOLUME-MIN_VOLUME);
}

//--------------------------------------
//
enum STREAM_TYPE
{
	INVALID=0,
	COMM,
	CHAT
};
//--------------------------------------
//
struct STREAM_NODE
{
	struct STREAM_NODE * pNext;
	union {
		HSTREAM hStream;
		//DSStream * pStream;
	};
	U32 soundID;
	STREAM_TYPE type;
	SINGLE volume;
	U32 fromID;
	bool bVideo;
	bool bOwnershipTransfered;
	// animation stuff
	COMPTR<IAnimate> anim;
	COMPTR<ILFParser> pParser;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~STREAM_NODE (void)
	{
		if (bVideo==false && hStream)
			STREAMER->CloseHandle(hStream);
//		else
//			delete pStream;

		if (fromID)
		{
			IBaseObject * obj;
			 if ((obj = OBJLIST->FindObject(fromID)) != NULL)
			 {
				 obj->bSpecialHighlight = false;
			 }
		}
	
		if (anim)
		{
			if (bOwnershipTransfered==0)
			{
				anim->DeferredDestruction();
			}
			else
			{
				anim->PauseAnim(true);
			}
			anim.ptr = 0;
		}
	}
};
//--------------------------------------
//
struct PENDING_NODE
{
	struct PENDING_NODE * pNext;
	U32 soundID;

	COMPTR<ILFParser> pParser;
	COMPTR<IAnimate> anim;

	char filename[32+4];
	S32 x;
	S32 y;
	OBJPTR<IBaseObject> obj;
	SINGLE volume;
	bool bOwnershipTransfered;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	~PENDING_NODE (void)
	{
		if (anim)
		{
			if (bOwnershipTransfered==0)
			{
				anim->DeferredDestruction();
			}
			else
			{
				anim->PauseAnim(true);
			}
			anim.ptr = 0;
		}
	}
};
//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE SoundMan : public ISoundManager, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(SoundMan)
	DACOM_INTERFACE_ENTRY(ISoundManager)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	//--------------------------------
	// data items go here
	//--------------------------------
	
	U32 eventHandle;		// handle to event callback
	
	SOUNDSTATE & state;

	U32 soundIDCounter;
	STREAM_NODE *streamList;
	PENDING_NODE *pendingList;
	STREAM_TYPE soundPriority;	// highest priority sound currently playing

	SINGLE currentSFXVolume, currentMusicVolume;

	bool bPauseState;
	//--------------------------------
	// class methods
	//--------------------------------
	
	SoundMan (void);

	~SoundMan (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	/* ISoundManager methods */

	virtual U32 PlayCommMessage (const char * filename, struct IFileSystem * parent, IBaseObject* obj = NULL, SINGLE volume = 1.0);

	virtual U32 PlayChatMessage (const char * filename, struct IFileSystem * parent, SINGLE volume = 1.0);

	virtual U32 PlayAudioMessage (const char * filename, IBaseObject * obj, SINGLE volume);

	virtual U32 PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, IBaseObject* obj, SINGLE volume);

	virtual U32 PlayMovie (const char * filename, S32 x, S32 y, SINGLE volume);

	virtual U32 PlayMovie (const char * filename, const RECT &rc, SINGLE volume);

	virtual BOOL32 IsPlaying (U32 soundID) const;

	virtual BOOL32 StopPlayback (U32 soundID);

	virtual void FlushStreams (void);

	virtual bool IsChatEnabled (void);

	virtual void GetMusicVolumeLevel (S32& level, bool& bMuted)		
	{
		bMuted = state.music.bMute;
		level  = state.music.volume;
	}

	virtual void SetMusicVolumeLevel (const S32& level, bool bMuted)
	{
		bool _unMuting = (state.music.bMute && bMuted==0);
		state.music.bMute = bMuted;
		state.music.volume = level;
		if (_unMuting==0)
			updateVolumeState(5000);
		else
		{
			currentMusicVolume = 0;
			MUSICMANAGER->SetVolume(0);
		}
	}

	virtual void GetSfxVolumeLevel (S32& level, bool& bMuted)
	{
		bMuted = state.effects.bMute;
		level  = state.effects.volume;
	}

	virtual void SetSfxVolumeLevel (const S32& level, bool bMuted)
	{
		state.effects.bMute = bMuted;
		state.effects.volume = level;
		updateVolumeState(5000);
	}

	virtual void GetCommVolumeLevel (S32& level, bool& bMuted)
	{
		bMuted = state.comms.bMute;
		level  = state.comms.volume;
	}

	virtual void SetCommVolumeLevel (const S32& level, bool bMuted)
	{
		state.comms.bMute = bMuted;
		state.comms.volume = level;
		updateVolumeState(5000);
	}

	virtual void GetChatVolumeLevel (S32& level, bool& bMuted)
	{
		bMuted = state.chat.bMute;
		level  = state.chat.volume;
	}

	virtual void SetChatVolumeLevel (const S32& level, bool bMuted)
	{
		state.chat.bMute = bMuted;
		state.chat.volume = level;
		updateVolumeState(5000);
	}

	virtual void TweekMusicVolume (void)
	{
		currentMusicVolume = 0;
		MUSICMANAGER->SetVolume(0);
	}

	virtual GENRESULT GetAnimation (U32 soundID, IAnimate ** ppAnim);

	virtual U32 GetLastStreamID (void)
	{
		return soundIDCounter;
	}
	virtual void SetLastStreamID (U32 id)
	{
		soundIDCounter = id;
	}

	virtual int BltMovies (void);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* SoundMan methods  */

	void init (void);

	void doDialog (void);

	void handleStreamerNotify (HSTREAM hStream, IStreamer::STATUS state);

//	void handleStreamerNotify (DSStream *pStream);

	void handlePauseChange (bool bPause);

	void handleClose (void);

	void handleCommVolumeMuting (void);

	void initDialog (HWND hwnd);

	void initSlider (HWND hwnd, UINT sliderID, S32 value);

	void updateVolumeState (S32 dt=0);

	void updatePendingList (void);

	BOOL volumeDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static BOOL staticVolumeDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	SINGLE getCommVolume (SINGLE volume);

	SINGLE getChatVolume (SINGLE volume);

	SINGLE getMusicVolume (void);

	SINGLE getEffectsVolume (void);

	int compare (const SINGLE & a, const SINGLE & b)
	{
		return ((S32 *)(&a))[0] - ((S32 *)(&b))[0];
	}

	IDAComponent * getBase (void)
	{
		return static_cast<ISoundManager *> (this);
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
SoundMan::SoundMan (void) : state(DEFAULTS->GetDefaults()->soundState)
{
	currentSFXVolume = currentMusicVolume = -0.00001F;
}
//--------------------------------------------------------------------------//
//
SoundMan::~SoundMan (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	//
	// flush the stream list
	//
	if (STREAMER)
	{
		STREAM_NODE *node;
		while (streamList)
		{
			node = streamList->pNext;
			delete streamList;
			streamList = node;
		}
	}

	while (pendingList)
	{
		PENDING_NODE * tmp = pendingList->pNext;
		delete pendingList;
		pendingList = tmp;
	}
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayCommMessage (const char * filename, struct IFileSystem * parent, IBaseObject* obj, SINGLE volume)
{
	U32 result = 0;
	HSTREAM hStream;
	STREAM_NODE *node;

	if (state.comms.bMute)
		goto Done;
	if (parent==0)
		parent = SPEECHDIR;
	if ((hStream = STREAMER->Open(filename, parent, 0)) == 0)
		goto Done;

	node = new STREAM_NODE;
	node->pNext = streamList;
	node->hStream = hStream;
	node->soundID = result = ++soundIDCounter;
	node->type = COMM;
	node->volume = volume;
	streamList = node;

	if (obj)
	{
		obj->bSpecialHighlight = true;
		node->fromID = obj->GetPartID();
	}

	if (soundPriority == INVALID)
	{
		soundPriority = COMM;
		updateVolumeState();
	}

	STREAMER->SetVolume(hStream, ConvertFloatToDSound(getCommVolume(volume)));
	STREAMER->Restart(hStream);

Done:	
	return result;
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayChatMessage (const char * filename, struct IFileSystem * parent, SINGLE volume)
{
	U32 result = 0;
	HSTREAM hStream;
	STREAM_NODE *node;

	if (state.chat.bMute)
		goto Done;
	if ((hStream = STREAMER->Open(filename, parent, 0)) == 0)
		goto Done;

	node = new STREAM_NODE;
	node->pNext = streamList;
	node->hStream = hStream;
	node->soundID = result = ++soundIDCounter;
	node->type = CHAT;
	node->volume = volume;
	streamList = node;
	
	if (soundPriority != CHAT)
	{
		soundPriority = CHAT;
		updateVolumeState();
	}

	handleCommVolumeMuting();
	STREAMER->SetVolume(hStream, ConvertFloatToDSound(getChatVolume(volume)));
	STREAMER->Restart(hStream);

Done:	
	return result;
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayAudioMessage (const char * filename, IBaseObject * obj, SINGLE volume)
{
	U32 result = 0;
	HSTREAM hStream;
	STREAM_NODE *node;

	if ((hStream = STREAMER->Open(filename, MSPEECHDIR, 0)) == 0)
		goto Done;

	node = new STREAM_NODE;
	node->pNext = streamList;
	node->hStream = hStream;
	node->soundID = result = ++soundIDCounter;
	node->type = CHAT;
	node->volume = volume;
	streamList = node;
	
	if (soundPriority != CHAT)
	{
		soundPriority = CHAT;
		updateVolumeState();
	}

	if (obj)
	{
		obj->bSpecialHighlight = true;
		node->fromID = obj->GetPartID();
	}

	handleCommVolumeMuting();
	STREAMER->SetVolume(hStream, ConvertFloatToDSound(getChatVolume(volume)));
	STREAMER->Restart(hStream);

Done:	
	return result;
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, IBaseObject* obj, SINGLE volume)
{
	U32 result = 0;

	{
		COMPTR<ILFParser> pParser;
		COMPTR<IDAComponent> pComp;
		COMPTR<IAnimate> anim;
		char buffer[36];
		char * ptr;

		if (GENDATA->CreateInstance(animType, pComp) != GR_OK)
			goto Done;
		if (pComp->QueryInterface("IAnimate", anim) != GR_OK)
			goto Done;

		strncpy(buffer, filename, 32);
		buffer[32] = 0;
		if ((ptr = strrchr(buffer, '.')) != 0)
		{
			strcpy(ptr+1, "txt");
			if (CreateLFParser(pParser) == GR_OK)
			{
				if (pParser->Initialize(buffer))
				{
					PENDING_NODE * node = new PENDING_NODE;

					node->soundID = result = ++soundIDCounter;
					node->pParser = pParser;
					node->anim = anim;
					strncpy(node->filename, filename, 32);
					node->x = x;
					node->y = y;
					if (obj)
						obj->QueryInterface(IBaseObjectID, node->obj, NONSYSVOLATILEPTR);
					node->volume = volume;
					node->pNext = pendingList;
					pendingList = node;
				}
			}
		}
	}

	if (BRIEFING!=0)
	{
		// if doing the briefing, wait until stream has been started before returning
		while (pendingList)
		{
			Sleep(0);
			updatePendingList();
		}
		if (IsPlaying(result) == 0)
			result = 0;
	}

Done:
	return result;
}
//--------------------------------------------------------------------------//
//
void SoundMan::updatePendingList (void)
{
	static bool bRecurse;		// prevent recursion
	if (bRecurse)
		return;

	PENDING_NODE * node = pendingList, *prev=0;
	bRecurse = true;

	while (node)
	{
		U32 *pFrameArray=0;
		S32 numAnimFrames = node->pParser->ParseFile(&pFrameArray);

		if (numAnimFrames > 0)	 // parse success
		{
			HSTREAM hStream = STREAMER->Open(node->filename, MSPEECHDIR, 0);

			if (hStream)
			{
				BaseHotRect * parentRect=FULLSCREEN;
				
				ANIMATE_DATA adata;
				if (BRIEFING!=0)		// make the animation be a child of the briefing screen
				{
					BRIEFING->QueryInterface("BaseHotRect", (void **) &parentRect);
					CQASSERT(parentRect);
					parentRect->Release();		// release the extra reference
					adata.bFuzzEffect = false;
				}
				else
					adata.bFuzzEffect = true;

				adata.animateType[0] = 0;
				adata.dwTimer = 0;
				adata.xOrigin = node->x;
				adata.yOrigin = node->y;
				node->anim->InitAnimate(adata, parentRect, pFrameArray, numAnimFrames); 
				node->anim->PauseAnim(true);

				STREAM_NODE *snode = new STREAM_NODE;
				snode->pNext = streamList;
				snode->hStream = hStream;
				snode->soundID = node->soundID;
				snode->type = CHAT;
				snode->volume = node->volume;
				snode->anim.ptr = node->anim.ptr;
				snode->pParser = node->pParser;
				snode->bOwnershipTransfered = node->bOwnershipTransfered;
				node->anim.ptr = 0;
				streamList = snode;

				if (node->obj)
				{
					node->obj->bSpecialHighlight = true;
					snode->fromID = node->obj->GetPartID();
				}
				
				if (soundPriority != CHAT)
				{
					soundPriority = CHAT;
					updateVolumeState();
				}

				handleCommVolumeMuting();
				STREAMER->SetVolume(hStream, ConvertFloatToDSound(getChatVolume(node->volume)));
			}
		}
		
		if (numAnimFrames != 0)
		{
			// delete this node
			if (prev)
			{
				prev->pNext = node->pNext;
				delete node;
				node = prev->pNext;
			}
			else
			{
				pendingList = node->pNext;
				delete node;
				node = pendingList;
			}
		}
		else
		{
			prev = node;
			node = node->pNext;
		}
	}

	bRecurse = false;
}
//-------------------------------------------------------------------
//
GENRESULT SoundMan::GetAnimation (U32 soundID, IAnimate ** ppAnim)
{
	STREAM_NODE *node = streamList;
	GENRESULT result = GR_GENERIC;

	*ppAnim = 0;

	while (node)
	{
		if (node->soundID == soundID)
		{
			if (node->anim)
			{
				CQASSERT(node->bOwnershipTransfered==0);
				*ppAnim = node->anim;
				node->bOwnershipTransfered = true;
				result = GR_OK;
			}
			break;
		}
		node = node->pNext;
	}

	if (result == GR_GENERIC)
	{
		PENDING_NODE * node = pendingList;
		while (node)
		{
			if (node->soundID == soundID)
			{
				if (node->anim)
				{
					CQASSERT(node->bOwnershipTransfered==0);
					*ppAnim = node->anim;
					node->bOwnershipTransfered = true;
					result = GR_OK;
				}
				break;
			}
			node = node->pNext;
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayMovie (const char * filename, S32 x, S32 y, SINGLE volume)
{
/*	U32 result = 0;
	DSStream * pStream;
	STREAM_NODE *node;

	if (strlen(filename) > 8+4)
		CQERROR1("Movie file '%s' does not conform to 8.3 format.", filename);

	pStream = new DSStream;
	if (pStream->Open(MOVIEDIR, filename) == 0)
	{
		delete pStream;
		goto Done;
	}

	node = new STREAM_NODE;
	node->pNext = streamList;
	node->pStream = pStream;
	node->bVideo = true;
	node->soundID = result = ++soundIDCounter;
	node->type = CHAT;
	node->volume = volume;
	streamList = node;

	if (soundPriority != CHAT)
	{
		soundPriority = CHAT;
		updateVolumeState();
	}

	handleCommVolumeMuting();
	pStream->SetVolume(ConvertFloatToDSound(getChatVolume(volume)));
	pStream->SetPosition(x,y);
	pStream->Run();

Done:	
	return result;*/
	return 0;
}
//-------------------------------------------------------------------
//
U32 SoundMan::PlayMovie (const char * filename, const RECT &rc, SINGLE volume)
{
	/*
	U32 result = 0;
	DSStream * pStream;
	STREAM_NODE *node;

	if (strlen(filename) > 8+4)
		CQERROR1("Movie file '%s' does not conform to 8.3 format.", filename);

	pStream = new DSStream;
	if (pStream->Open(MOVIEDIR, filename) == 0)
	{
		delete pStream;
		goto Done;
	}

	node = new STREAM_NODE;
	node->pNext = streamList;
	node->pStream = pStream;
	node->bVideo = true;
	node->soundID = result = ++soundIDCounter;
	node->type = CHAT;
	node->volume = volume;
	streamList = node;

	if (soundPriority != CHAT)
	{
		soundPriority = CHAT;
		updateVolumeState();
	}

	handleCommVolumeMuting();
	pStream->SetVolume(ConvertFloatToDSound(getChatVolume(volume)));
	pStream->SetPosition(rc);
	pStream->Run();

Done:	
	return result;*/
	return 0;
}
//-------------------------------------------------------------------
//
BOOL32 SoundMan::IsPlaying (U32 soundID) const
{
	STREAM_NODE *node = streamList;
	BOOL32 result = 0;

	while (node)
	{
		if (node->soundID == soundID)
		{
			result = 1;
			break;
		}
		node = node->pNext;
	}

	if (result == 0)
	{
		PENDING_NODE * node = pendingList;
		while (node)
		{
			if (node->soundID == soundID)
			{
				result = 1;
				break;
			}
			node = node->pNext;
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
BOOL32 SoundMan::StopPlayback (U32 soundID)
{
	STREAM_NODE *node = streamList;
	BOOL32 result = 0;

	while (node)
	{
		if (node->soundID == soundID)
		{
			result = 1;
			break;
		}
		node = node->pNext;
	}

	if (node)
	{
		// fake completion of stream
		if (node->bVideo==false)
			handleStreamerNotify(node->hStream, IStreamer::COMPLETED);
/*		else
		{
			node->pStream->Stop();
			handleStreamerNotify(node->pStream);
		}
*/	}
	else  // check the pending list
	{
		PENDING_NODE * node = pendingList, *prev=0;
		while (node)
		{
			if (node->soundID == soundID)
			{
				result = 1;
				break;
			}
			prev = node;
			node = node->pNext;
		}
		if (node)
		{
			if (prev)
				prev->pNext = node->pNext;
			else
				pendingList = node->pNext;
			delete node;
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
int SoundMan::BltMovies (void)
{
/*	STREAM_NODE *node = streamList;
	int result = 0;

	while (node)
	{
		if (node->bVideo)
		{
			node->pStream->Blit();
			result++;
		}
		node = node->pNext;
	}

	return result;*/
	return 0;
}
//-------------------------------------------------------------------
//
void SoundMan::FlushStreams (void)
{
	STREAM_NODE *node = streamList, *next;
	// Make sure we have a valid stream...
	while (node)
	{
		next = node->pNext;
/*		if(node->bVideo)
		{
			node->pStream->Stop();
			handleStreamerNotify(node->pStream);	
		}
		else
*/			handleStreamerNotify(node->hStream, IStreamer::COMPLETED);

		// 'node' might be trashed at this point!
		node = next;
	}

	// flush the pending list
	while (pendingList)
	{
		PENDING_NODE * tmp = pendingList->pNext;
		delete pendingList;
		pendingList = tmp;
	}
}
//-------------------------------------------------------------------
//
bool SoundMan::IsChatEnabled (void)
{
	return !state.comms.bMute;
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT SoundMan::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_UPDATE:
		updateVolumeState(U32(param)>>10);
		updatePendingList();
		break;
	case WM_COMMAND:
		if (LOWORD(msg->wParam) == IDM_VOLUME_ADJUST && CQFLAGS.bNoGDI==0)
			doDialog();
		break;
	case CQE_DEBUG_HOTKEY:
		if (U32(param) == IDH_VOLUME_ADJUST && CQFLAGS.bNoGDI==0)
			doDialog();
		break;

	case CQE_STREAMER2:
//		handleStreamerNotify((DSStream *)msg->lParam);
		break;
	
	case CQE_STREAMER:
		handleStreamerNotify((HSTREAM)msg->lParam, (IStreamer::STATUS)msg->wParam);
		break;
	case CQE_PAUSE_CHANGE:
		handlePauseChange(U32(param) != 0);
		break;
	case CQE_MISSION_CLOSE:
		handleClose();
		break;
	case WM_ACTIVATEAPP:
		if(msg->wParam && !CQFLAGS.bGamePaused)
			handlePauseChange(false);
		else if((!(msg->wParam)) && !CQFLAGS.bGamePaused)
		{
			handlePauseChange(true);
		}
		break;
	}

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
BOOL SoundMan::staticVolumeDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;
	SoundMan * man = (SoundMan *) GetWindowLong(hwnd, DWL_USER);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLong(hwnd, DWL_USER, lParam);
			man = (SoundMan *) GetWindowLong(hwnd, DWL_USER);

			man->initDialog(hwnd);
			result = 1;
		}
		break;

	default:
		if (man)
			result = man->volumeDlgProc(hwnd, message, wParam, lParam);
	}

	return result;
}
//--------------------------------------------------------------------------//
//
BOOL SoundMan::volumeDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_VSCROLL:
		switch (GetDlgCtrlID((HWND)lParam))
		{
		case IDC_SLIDER1:
			state.music.volume = -SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			break;
		case IDC_SLIDER2:
			state.effects.volume = -SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			break;
		case IDC_SLIDER3:
			state.comms.volume = -SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			break;
		case IDC_SLIDER4:
			state.chat.volume = -SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			break;
		}
		updateVolumeState(5000);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK1:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				state.music.bMute = !state.music.bMute;
				updateVolumeState();
			}
			break;
		case IDC_CHECK2:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				state.effects.bMute = !state.effects.bMute;
				updateVolumeState();
			}
			break;
		case IDC_CHECK3:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				state.comms.bMute = !state.comms.bMute;
				updateVolumeState();
			}
			break;
		case IDC_CHECK4:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				state.chat.bMute = !state.chat.bMute;
				updateVolumeState();
			}
			break;

		case IDCANCEL:
		case IDOK:
			EndDialog(hwnd, 0);
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void SoundMan::doDialog (void)
{
	DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG7), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) staticVolumeDlgProc, (LPARAM) this);
}
//--------------------------------------------------------------------------//
//
void SoundMan::handleStreamerNotify (HSTREAM hStream, IStreamer::STATUS state)
{
	//
	// remove streams that have stopped or are invalid
	//
	STREAM_NODE *node = streamList, *prev=0;
	bool bCommPlaying, bChatPlaying;
	bCommPlaying = bChatPlaying = false;

	while (node)
	{
		if (node->hStream == hStream)
		{
			if (state == IStreamer::COMPLETED||state==IStreamer::INVALID)
			{
				// 
				// remove this node from the list, delete it
				//
				if (prev)
				{
					prev->pNext = node->pNext;
					delete node;
					node = prev->pNext;
				}
				else // first element of list
				{
					streamList = node->pNext;
					delete node;
					node = streamList;
				}
				continue;
			} 
			else
			if (state == IStreamer::INITSUCCESS)
			{
				if (bPauseState==false)
				{
					if (node->anim)
					{
						CQASSERT(node->bVideo==false);	// assume regular stream
						node->anim->PauseAnim(false);
						STREAMER->Restart(node->hStream);
					}
				}
			}
		}

		switch (node->type)
		{
		case COMM:
			bCommPlaying = true;
			break;
		case CHAT:
			bChatPlaying = true;
			break;
		}

		prev = node;
		node = node->pNext;
	}
	//
	// update sound priority
	// 
	switch (soundPriority)
	{
	case CHAT:
		if (bChatPlaying == false)
		{
			if (bCommPlaying)
				soundPriority = COMM;
			else
				soundPriority = INVALID;

			updateVolumeState();
		}
		break;

	case COMM:
		if (bCommPlaying == false)
		{
			soundPriority = INVALID;
			updateVolumeState();
		}
		break;
	}
}
//--------------------------------------------------------------------------//
//
/*void SoundMan::handleStreamerNotify (DSStream *pStream)
{
	//
	// remove streams that have stopped or are invalid
	//
	STREAM_NODE *node = streamList, *prev=0;
	bool bCommPlaying, bChatPlaying;
	bCommPlaying = bChatPlaying = false;

	while (node)
	{
		if (pStream == node->pStream)
		{
			pStream->GetEvent();

			if (pStream->GetState() == DSStream::STOPPED || pStream->GetState() == DSStream::ABORTED)
			{
				// 
				// remove this node from the list, delete it
				//
				if (prev)
				{
					prev->pNext = node->pNext;
					delete node;
					node = prev->pNext;
				}
				else // first element of list
				{
					streamList = node->pNext;
					delete node;
					node = streamList;
				}
				continue;
			}
		}

		switch (node->type)
		{
		case COMM:
			bCommPlaying = true;
			break;
		case CHAT:
			bChatPlaying = true;
			break;
		}

		prev = node;
		node = node->pNext;
	}
	//
	// update sound priority
	// 
	switch (soundPriority)
	{
	case CHAT:
		if (bChatPlaying == false)
		{
			if (bCommPlaying)
				soundPriority = COMM;
			else
				soundPriority = INVALID;

			updateVolumeState();
		}
		break;

	case COMM:
		if (bCommPlaying == false)
		{
			soundPriority = INVALID;
			updateVolumeState();
		}
		break;
	}
}*/
//--------------------------------------------------------------------------//
// 
void SoundMan::handlePauseChange (bool bPause)
{
	STREAM_NODE *node = streamList;
	bPauseState = bPause;
	if(bPause)
	{
		while(node)
		{
/*			if(node->bVideo)
				node->pStream->Pause();
			else 
*/			if(node->hStream)
			{
				STREAMER->Stop(node->hStream);
				if (node->anim)
					node->anim->PauseAnim(true);
			}
			node = node->pNext;
		}
	}
	else
	{
		while(node)
		{
/*			if(node->bVideo)
				node->pStream->Run();
			else
*/			if(node->hStream)
			{
				STREAMER->Restart(node->hStream);
				if (node->anim)
					node->anim->PauseAnim(false);
			}
			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
// called when a CHAT message is started
//
void SoundMan::handleCommVolumeMuting (void)
{
	STREAM_NODE *node = streamList;

	while(node)
	{
		if(node->type == COMM)
		{
/*			if(node->bVideo)
				node->pStream->SetVolume(ConvertFloatToDSound(getCommVolume(node->volume)));
			else*/ if(node->hStream)
				STREAMER->SetVolume(node->hStream, ConvertFloatToDSound(getCommVolume(node->volume)));
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
// 
void SoundMan::handleClose (void)
{
	STREAM_NODE *node = streamList;
	while(node)
	{
		STREAM_NODE *next = node->pNext;
/*		if(node->bVideo)
		{
			node->pStream->Stop();
			handleStreamerNotify(node->pStream);
		}
		else*/
		{
			handleStreamerNotify(node->hStream, IStreamer::COMPLETED);
		}
		node = next;
	}

	while (pendingList)
	{
		PENDING_NODE * tmp = pendingList->pNext;
		delete pendingList;
		pendingList = tmp;
	}
}
//--------------------------------------------------------------------------//
// setup the state of the dialog controls
//
void SoundMan::initDialog (HWND hwnd)
{
	initSlider(hwnd, IDC_SLIDER1, state.music.volume);
	initSlider(hwnd, IDC_SLIDER2, state.effects.volume);
	initSlider(hwnd, IDC_SLIDER3, state.comms.volume);
	initSlider(hwnd, IDC_SLIDER4, state.chat.volume);
	
	if (state.music.bMute)
		CheckDlgButton(hwnd, IDC_CHECK1, BST_CHECKED);
	if (state.effects.bMute)
		CheckDlgButton(hwnd, IDC_CHECK2, BST_CHECKED);
	if (state.comms.bMute)
		CheckDlgButton(hwnd, IDC_CHECK3, BST_CHECKED);
	if (state.chat.bMute)
		CheckDlgButton(hwnd, IDC_CHECK4, BST_CHECKED);
}
//--------------------------------------------------------------------------//
//
void SoundMan::initSlider (HWND hwnd, UINT sliderID, S32 value)
{
	HWND hItem;

	hItem = GetDlgItem(hwnd, sliderID);
	SendMessage(hItem, TBM_SETRANGE, 0, MAKELONG(-MAX_VOLUME, MIN_VOLUME));			// from 0 to 10
	// win32 bug?
	if (value == 0)
		SendMessage(hItem, TBM_SETPOS, 1, 1);
	SendMessage(hItem, TBM_SETPOS, 1, -value);
}
//--------------------------------------------------------------------------//
//
void SoundMan::updateVolumeState (S32 dt)	// milliseconds
{
	SFXMANAGER->Enable(state.effects.bMute==0 && DEFAULTS->GetDefaults()->bEditorMode==0 && CQFLAGS.bGamePaused==0);
	MUSICMANAGER->Enable(state.music.bMute==0);

	SINGLE sfx, music;
	sfx = getEffectsVolume();
	music = getMusicVolume();

	if (compare(sfx, currentSFXVolume) != 0 || compare(music, currentMusicVolume) != 0)
	{
		SINGLE ramp = dt * VOLUME_RAMP;
		if (sfx > currentSFXVolume)		// need to increase volume
			if ((currentSFXVolume += ramp) >= sfx)
				currentSFXVolume = sfx;
		if (sfx < currentSFXVolume)		// need to decrease volume
		//	if ((currentSFXVolume -= ramp) <= sfx)
				currentSFXVolume = sfx;

		if (music > currentMusicVolume)		// need to increase volume
			if ((currentMusicVolume += ramp) >= music)
				currentMusicVolume = music;
		if (music < currentMusicVolume)		// need to decrease volume
		//	if ((currentMusicVolume -= ramp) <= music)
				currentMusicVolume = music;
		if (currentSFXVolume < 0)
			currentSFXVolume = 0;
		if (currentMusicVolume < 0)
			currentMusicVolume = 0;

		SFXMANAGER->SetSFXVolume(currentSFXVolume);
		MUSICMANAGER->SetVolume(currentMusicVolume);
	}
}
//--------------------------------------------------------------------------//
// calculate the volume for a Comm stream
SINGLE SoundMan::getCommVolume (SINGLE volume)
{
	if (soundPriority == CHAT)
		return ((volume * scaleVolume(state.comms.volume)) - 0.75F);
	else
		return volume * scaleVolume(state.comms.volume);
}
//--------------------------------------------------------------------------//
// calculate the volume for a Chat stream
SINGLE SoundMan::getChatVolume (SINGLE volume)
{
	return volume * scaleVolume(state.chat.volume);
}
//--------------------------------------------------------------------------//
// calculate the volume for Music
SINGLE SoundMan::getMusicVolume (void)
{
	if (soundPriority == CHAT)
	{
		return __min(scaleVolume(state.music.volume), 0.25F);
	}
	else
	{
		if (soundPriority != INVALID)
			return (scaleVolume(state.music.volume) - 0.25F);
		else
			return scaleVolume(state.music.volume);
	}
}
//--------------------------------------------------------------------------//
// calculate the volume for SFX
SINGLE SoundMan::getEffectsVolume (void)
{
	if (soundPriority == CHAT)
	{
		return __min(scaleVolume(state.effects.volume), 0.25F);
	}
	else
	{
		if (soundPriority != INVALID)
			return (scaleVolume(state.effects.volume) - 0.25F);
		else
			return scaleVolume(state.effects.volume);
	}
}
//--------------------------------------------------------------------------//
//
void SoundMan::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);

	updateVolumeState();
}
//--------------------------------------------------------------------------//
//
struct _soundman : GlobalComponent
{
	SoundMan * soundMan;

	virtual void Startup (void)
	{
		SOUNDMANAGER = soundMan = new DAComponent<SoundMan>;
		AddToGlobalCleanupList((IDAComponent **) &SOUNDMANAGER);
	}

	virtual void Initialize (void)
	{
		soundMan->init();
	}
};

static _soundman soundman;

//------------------------------------------------------------------------------------//
//-----------------------------End SoundManager.cpp-----------------------------------//
//------------------------------------------------------------------------------------//
