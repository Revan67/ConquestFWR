//--------------------------------------------------------------------------//
//                                                                          //
//                             Menu_Briefing.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Briefing.cpp 45    10/31/00 2:59p Jasony $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IAnimate.h"
#include "Mission.h"
#include "MScript.h"
#include "ITeletype.h"
#include "SoundManager.h"
#include "IBriefing.h"
#include "MusicManager.h"

#include <Streamer.h>
#include <DMenuBriefing.h>

#define NUM_CELLS 12
#define NUM_ANIM  4

//--------------------------------------------------------------------------//
//
struct MenuBriefing : public DAComponent<Frame>, IBriefing
{
	//
	// data items
	//
	GT_BRIEFING data;
	COMPTR<IStatic> background, title;
	COMPTR<IButton2> start, replay, cancel;
	COMPTR<IAnimate> animFuzz[NUM_ANIM];

	COMPTR<IAnimate> animUser[NUM_ANIM];

	RECT rcTeletype;
	RECT rcComm[NUM_ANIM];
	U32 streamID[NUM_ANIM];
	bool bKeepCellImage[NUM_ANIM];

	U32 teletypeID;

	const char * szFileName;
	bool bLoaded;
	bool bLowLatencyEnabled;			// true when we have adjusted the streamer for low latency

	U32 animIndexArray[NUM_CELLS];
	char szDefaultAnimationType[GT_PATH];

	//
	// instance methods
	//

	MenuBriefing (Frame * _parent, const char * fileName) : szFileName(fileName)
	{
		eventPriority = EVENT_PRIORITY_IG_OPTIONS;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(_parent);
		init();

		BRIEFING = this;
	}

	~MenuBriefing (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IDAComponent methods */

	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		return DAComponent<Frame>::QueryInterface(interface_name, instance);
	}
	
	DEFMETHOD_(U32,AddRef)           (void)
	{
		return DAComponent<Frame>::AddRef();
	}
	
	DEFMETHOD_(U32,Release)          (void)
	{
		return DAComponent<Frame>::Release();
	}

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message, void *parm)
	{
		DWORD dwRead;

		doc->SetFilePointer(0,0);
		doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
		setStateInfo();

		return GR_OK;
	}


	/* Frame methods */
	
	virtual void onFocusChanged (void)
	{
		Frame::onFocusChanged();
	}

	/* IBriefing methods */

	virtual U32 PlayAnimatedMessage (const CQBRIEFINGITEM & item);  

	virtual U32 PlayTeletype (const CQBRIEFINGTELETYPE & item);

	virtual U32 PlayAnimation (const CQBRIEFINGITEM & item);

	virtual void FreeSlot (const U32 slotID);

	virtual const bool IsAnimationPlaying (const U32 slotID);


	/* MenuBriefing methods */


	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	virtual void onButtonPressed (U32 buttonID);

	void init (void);
	
	void initLobby (void);

	void draw (void);

	virtual void onUpdate (U32 dt);   // dt in milliseconds

	void checkFreeSlot (U32 slotID);

	GENRESULT __stdcall Notify (U32 message, void *param);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		onButtonPressed(IDS_CANCEL);
		return true;
	}

	void flush (void)
	{
		SOUNDMANAGER->FlushStreams();
		TELETYPE->Flush();
	}

	void enableLowLatencyStreaming (bool bEnable);
};
//----------------------------------------------------------------------------------//
//
MenuBriefing::~MenuBriefing (void)
{
	enableLowLatencyStreaming(false);
	BRIEFING = NULL;
}
//----------------------------------------------------------------------------------//
//
void MenuBriefing::FreeSlot (const U32 slotID)
{
	CQASSERT(slotID < NUM_CELLS);
	bKeepCellImage[slotID] = false;
}
//----------------------------------------------------------------------------------//
//
const bool MenuBriefing::IsAnimationPlaying (const U32 slotID)
{
	// if there is a user animation, *and* it has gone through all of it's animation, then return false
	if (animUser[slotID])
	{
		return animUser[slotID]->HasAnimationCompleted();
	}

	return false;
}
//----------------------------------------------------------------------------------//
//
U32 MenuBriefing::PlayAnimatedMessage (const CQBRIEFINGITEM & item)
{
	// HACK alert!
	if (stricmp(item.szFileName, "empty.wav") == 0)
	{
		// what they really want is a animation.
		return PlayAnimation(item);
	}

	CQASSERT(item.slotID < NUM_CELLS);
	checkFreeSlot(item.slotID);
	bKeepCellImage[item.slotID] = item.bContinueAnimating;

    streamID[item.slotID] = SOUNDMANAGER->PlayAnimatedMessage(item.szFileName, item.szTypeName,
															  REAL2IDEALX(rcComm[item.slotID].left), REAL2IDEALY(rcComm[item.slotID].top));

	// get the pointer to the animation in the message
	if (bKeepCellImage[item.slotID] && streamID[item.slotID])
	{
		SOUNDMANAGER->GetAnimation(streamID[item.slotID], animUser[item.slotID]);
		animUser[item.slotID]->SetLoopingAnimation(!item.bContinueAnimating);
	}

	return streamID[item.slotID];
}
//----------------------------------------------------------------------------------//
//
U32 MenuBriefing::PlayTeletype (const CQBRIEFINGTELETYPE & teletype)
{
	// just in case we get in here with a teletype already playing, do a flush
	TELETYPE->Flush();
	return teletypeID = TELETYPE->CreateTeletypeDisplay(teletype.pString, rcTeletype, IDS_TELETYPE_FONT,
				                           teletype.color, teletype.lifeTime, teletype.textTime, teletype.bMuted);
}
//----------------------------------------------------------------------------------//
//
U32 MenuBriefing::PlayAnimation (const CQBRIEFINGITEM & item)
{
	COMPTR<IDAComponent> pComp;
	COMPTR<IAnimate> anim;
	ANIMATE_DATA adata;

	if (GENDATA->CreateInstance(item.szTypeName, pComp) != GR_OK)
		goto Done;
	if (pComp->QueryInterface("IAnimate", anim) != GR_OK)
		goto Done;

	checkFreeSlot(item.slotID);
	bKeepCellImage[item.slotID] = true;

	strncpy(adata.animateType, item.szTypeName, sizeof(adata.animateType));
	adata.xOrigin = REAL2IDEALX(rcComm[item.slotID].left);
	adata.yOrigin = REAL2IDEALY(rcComm[item.slotID].top);
	adata.bFuzzEffect = false;

	anim->InitAnimate(adata, this, NULL, 0); 
	anim->SetLoopingAnimation(item.bLoopAnimation);
	anim->SetAnimationFrameRate(item.dwTimer);
	
	animUser[item.slotID] = anim;

Done:
	return 0;
}
//----------------------------------------------------------------------------------//
//
GENRESULT MenuBriefing::Notify (U32 message, void *param) 
{
//	MSG *msg = (MSG *) param;
	GENRESULT result = GR_OK;

	switch (message)
	{
	case CQE_ENDFRAME:
		// draw the borders around the rects for our teletype and communications
		result = Frame::Notify(message, param);
		draw();
		return result;

	case CQE_KILL_FOCUS:
		return GR_OK;

	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void MenuBriefing::checkFreeSlot (U32 slotID)
{
	CQASSERT(slotID < NUM_ANIM);
	
	// if something is already playing in the slot, then stop that stream
	if (streamID[slotID])
	{
		if (SOUNDMANAGER->IsPlaying(streamID[slotID]))
		{
			SOUNDMANAGER->StopPlayback(streamID[slotID]);
		}
	}

	if (animUser[slotID])
	{
		animUser[slotID]->DeferredDestruction();
		animUser[slotID].ptr = 0;
	}
}
//----------------------------------------------------------------------------------//
//
void MenuBriefing::onUpdate (U32 dt)
{
	ELAPSED_TIME = SINGLE(dt) / 1000;
	REALTIME_FRAMERATE = (ELAPSED_TIME > 0) ? (1 / ELAPSED_TIME) : 4;
	MISSION->Update();


	static int timer = 0;//what is this and why is it here -tom
	timer += dt;

	if (timer >= 250)
	{
		timer = 0;
		// see if any of our streams is open or not and set our default animations to invisible if need be
		for (int i = 0; i < NUM_ANIM; i++)
		{
			if (streamID[i])
			{
				if (SOUNDMANAGER->IsPlaying(streamID[i]) == false && bKeepCellImage[i] == false)
				{
					streamID[i] = 0;

					if (animUser[i])
					{
						animUser[i]->DeferredDestruction();
						animUser[i].ptr = 0;
					}
				}
			}
			else if (animUser[i] != 0 && bKeepCellImage[i] == false)
			{
				// end the animation
				animUser[i]->DeferredDestruction();
				animUser[i].ptr = 0;
			}

			animFuzz[i]->SetVisible(streamID[i] == 0 && animUser[i]==0);
		}
	}
}
//----------------------------------------------------------------------------------//
//
void MenuBriefing::draw (void)
{
	// draw the borders for the rectangles I guess
	if (DEFAULTS->GetDefaults()->bDrawHotrects)
	{
		// the teletype drawn in green
		DA::RectangleHash(NULL, IDEAL2REALX(rcTeletype.left), IDEAL2REALY(rcTeletype.top), 
			              IDEAL2REALX(rcTeletype.right), IDEAL2REALY(rcTeletype.bottom), RGB(0,255,0));

		// the comms stuff drawn in yellow 
		for (int i = 0; i < NUM_ANIM; i++)
		{
			DA::RectangleHash(NULL, rcComm[i].left, rcComm[i].top, rcComm[i].right, rcComm[i].bottom, RGB(255,255,0));
		}
	}
}
//----------------------------------------------------------------------------------//
//
void MenuBriefing::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);  
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	// no ideal to real for teletypes, moron!
	rcTeletype.left		= data.rcTeletype.left;
	rcTeletype.right	= data.rcTeletype.right;
	rcTeletype.top		= data.rcTeletype.top;
	rcTeletype.bottom	= data.rcTeletype.bottom;

	for (int i = 0; i < NUM_ANIM; i++)
	{
		rcComm[i].left		= IDEAL2REALX(data.rcComm[i].left);
		rcComm[i].top		= IDEAL2REALY(data.rcComm[i].top);
		rcComm[i].right		= rcComm[i].left + IDEAL2REALX(128);
		rcComm[i].bottom	= rcComm[i].top + IDEAL2REALY(96);
	}

	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);

	start->InitButton(data.start, this);
	replay->InitButton(data.replay, this);
	cancel->InitButton(data.cancel, this);

	start->SetTransparent(true);
	replay->SetTransparent(true);
	cancel->SetTransparent(true);

	for (int i = 0; i < NUM_ANIM; i++)
	{
		animFuzz[i]->InitAnimate(data.animFuzz[i], this, NULL, 0);
	}

	strcpy(szDefaultAnimationType, data.animFuzz[0].animateType);


	// tell the mission to load itself
	if (bLoaded == false)
	{
		bLoaded = true;
		MISSION->LoadBriefing(szFileName);
	}

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		setFocus(start);
	}
}
//--------------------------------------------------------------------------//
//
void MenuBriefing::enableLowLatencyStreaming (bool bEnable)
{
	if (bLowLatencyEnabled != bEnable)
	{
		if (bEnable)
		{
			STREAMERDESC desc;

			desc.hMainWindow = hMainWindow;
			desc.lpDSound = DSOUND;
			desc.uMsg = CQE_STREAMER;
			desc.readBufferTime =  4;		// in seconds, 0 = default setting (4.0 is a reasonable value)
			desc.soundBufferTime = 0.25;	// in seconds, 0 = default setting (0.25 is a reasonable value)

			STREAMER->Init(&desc);
		}
		else
		{
			STREAMERDESC desc;

			desc.hMainWindow = hMainWindow;
			desc.lpDSound = DSOUND;
			desc.uMsg = CQE_STREAMER;
			desc.readBufferTime =  4;		// in seconds, 0 = default setting (4.0 is a reasonable value)
			desc.soundBufferTime = 1.0;		// in seconds, 0 = default setting (0.25 is a reasonable value)

			STREAMER->Init(&desc);
		}

		bLowLatencyEnabled = bEnable;
	}
}
//--------------------------------------------------------------------------//
//
void MenuBriefing::init (void)
{
	data = 	*((GT_BRIEFING *) GENDATA->GetArchetypeData("MenuBriefing"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	GENDATA->CreateInstance(data.start.buttonType, pComp);
	pComp->QueryInterface("IButton2", start);

	GENDATA->CreateInstance(data.replay.buttonType, pComp);
	pComp->QueryInterface("IButton2", replay);

	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);

	for (int i = 0; i < 4; i++)
	{
		GENDATA->CreateInstance(data.animFuzz[i].animateType, pComp);
		pComp->QueryInterface("IAnimate", animFuzz[i]);
	}

	resPriority = RES_PRIORITY_HIGH;
	cursorID = IDC_CURSOR_ARROW;

	// fill the anim index array
	for (int i = 0; i < NUM_CELLS; i++)
	{
		animIndexArray[0] = 0;
	}

	enableLowLatencyStreaming(true);
}
//--------------------------------------------------------------------------//
//
void MenuBriefing::onButtonPressed (U32 buttonID)
{
	switch (buttonID)
	{
		case IDS_START:
			// start the mission
			MUSICMANAGER->PlayMusic("Battle_loading.wav", false);
			flush();	// flush needed before switching to 3D mode.  Jason was here, he knows this now...
			enableLowLatencyStreaming(false);
			SetVisible(false);
			ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);
			MISSION->Reload();
			endDialog(1);
			break;

		case IDS_REPLAY:
			// replay the breifing
			// ie. reload the current mission
			FreeSlot(0);
			FreeSlot(1);
			FreeSlot(2);
			FreeSlot(3);
			MISSION->LoadBriefing(szFileName);
			break;

		case IDS_CANCEL:
			// cancel the mission, go back to campaign screen
			enableLowLatencyStreaming(false);
			MISSION->Close();
			endDialog(0);
			break;
	}
}

//--------------------------------------------------------------------------//
//
/*
Frame * __stdcall CreateMenuBriefing ()
{
	MenuBriefing * menu = new MenuBriefing(NULL);
	menu->createViewer("\\GT_BRIEFING\\MenuBriefing", "GT_BRIEFING", IDS_VIEWBRIEFING);

	return menu;
}
*/
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_Briefing (Frame * parent, const char * szFileName)
{
	MUSICMANAGER->PlayMusic(NULL);
	MenuBriefing * menu = new MenuBriefing(parent, szFileName);

	menu->createViewer("\\GT_BRIEFING\\MenuBriefing", "GT_BRIEFING", IDS_VIEWBRIEFING);
	menu->beginModalFocus();

	U32 result = CQDoModal(menu);
	delete menu;
	return result;
}


//--------------------------------------------------------------------------//
//-----------------------------End Menu_Chat.cpp---------------------------//
//--------------------------------------------------------------------------//