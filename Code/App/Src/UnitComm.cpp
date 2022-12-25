//--------------------------------------------------------------------------//
//                                                                          //
//                               UnitComms.cpp                              //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/UnitComm.cpp 56    6/25/01 4:27p Tmauer $
*/
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "UnitComm.h"
#include "Startup.h"
#include "SoundManager.h"
#include "StringData.h"
#include "CQTrace.h"
#include "MPart.h"
#include <MGlobals.h>
#include <DBaseData.h>
#include "DrawAgent.h"
#include "Hotkeys.h"
#include "ObjList.h"
#include "Sector.h"
#include "Sysmap.h"
#include "Camera.h"
#include "UserDefaults.h"
#include "MScript.h"
#include "GenData.h"
#include "ScrollingText.h"
#include <stdio.h>

#include <TSmartPointer.h>
#include <EventSys.h>
#include <IConnection.h>
#include <TComponent.h>
#include <HeapObj.h>
#include <FileSys.h>
#include <HKEvent.h>

#include <commctrl.h>
#include <malloc.h>

#define cqoffsetofmember(base, member) (size_t)((&(((base *)0)->member))+0)


#define SHORT_DECAY 2000		// 2 seconds
#define ALERT_DECAY 30000		// 30 seconds
#define GOTO_DECAY  10000		// 10 seconds

#define MAX_ALERT_FONTS 10

//--------------------------------------------------------------------------//
//
struct CommNode
{
	CommNode * pNext;
	U32		index;				// which speech to play?		(0 == enemy sighted)
	U32		flags;
	U32		dwMissionID;
	U32		subtitle;
	U32		unitNameID;
	const char *filename;		
	U32		timeStamp;			// time when created
	S32		decayTime;			// time until node expires
	S32		gotoDecay;			// time until "goto" expires (only valid if index==0)
	// physical pos when first created
	U32		systemID;			// can be zero (probe)
	Vector	pos;
	bool	bAlert;				// index==0 || bAdmiralAlert
	
	DWORD	param;				// used to hold the parameter of PlayCommMessage(). Right now just a resource string ID for alerts.


    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
};

#define ALERT_STRING_LENGTH 100

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE UnitComm : public IUnitComm, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(UnitComm)
	DACOM_INTERFACE_ENTRY(IUnitComm)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	//--------------------------------
	// data items go here
	//--------------------------------
	COMPTR<IFontDrawAgent> pFont[MAX_ALERT_FONTS];

	COLORREF fontColor[NUM_ALERT_STRINGS];
	wchar_t alertStrings[NUM_ALERT_STRINGS][ALERT_STRING_LENGTH];
	HFONT hFont;
	
	U32 eventHandle;		// handle to event callback

	CommNode * historyList, * queuedList;

	U32 currentTime;
	U32		soundID;			// returned by soundManager
	bool   bSoundIsSelection;	// current sound is for selection -- feel free to override it
	bool   bHasFocus;

	//--------------------------------
	// class methods
	//--------------------------------
	
	UnitComm (void);

	~UnitComm (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}
	
	/* IUnitComm methods */

	virtual void PlayCommMessage (IBaseObject * obj, U32 dwMissionID, U32 indexPos, U32 subtitle, U32 shipNameID, U32 flags, DWORD param);

	virtual void PlayCommMessage (IBaseObject * obj, U32 dwMissionID, char * fileName, U32 subtitle, U32 shipNameID, U32 flags=0,DWORD param = 0);

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* UnitComm methods  */

	void init (void);

	void reset (void);

	void update (U32 dt);	// milliseconds
	
	CommNode * findCommFromUnit (U32 dwMissionID, U32 index) const;

	CommNode * findAlertInSystem (U32 systemID) const;

	CommNode * findActiveAlert (void) const;

	CommNode * findAlertFromUnit (U32 dwMissionID) const;

	void drawAlerts();

	void on3DEnable (bool bEnable);

	static const char * getFilename (IBaseObject * obj, U32 index);

	void gotoSeekEvent (void);

	void addMessage (IBaseObject * obj, U32 dwMissionID, U32 indexPos, const char * filename, U32 subtitle, U32 shipNameID, U32 flags, DWORD param);

	void flushQueuedSelection (void);

	IDAComponent * getBase (void)
	{
		return static_cast<IUnitComm *> (this);
	}
};
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
UnitComm::UnitComm (void)
{
	bHasFocus = 1;
}
//--------------------------------------------------------------------------//
//
UnitComm::~UnitComm (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS && GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	reset();

	if (hFont)
	{
		DeleteObject(hFont);
		hFont = 0;
	}
}
//-------------------------------------------------------------------
//
void UnitComm::reset (void)
{
	CommNode * pNode;

	while ((pNode = historyList) != 0)
	{
		historyList = pNode->pNext;
		delete pNode;
	}

	while ((pNode = queuedList) != 0)
	{
		queuedList = pNode->pNext;
		delete pNode;
	}
}
//-------------------------------------------------------------------
//
inline CommNode * UnitComm::findActiveAlert (void) const
{
	CommNode * pNode = historyList;

	while (pNode)
	{
		if (pNode->bAlert && pNode->gotoDecay > 0)
			break;
		pNode = pNode->pNext;
	}
	return pNode;
}
//-------------------------------------------------------------------
//
inline CommNode * UnitComm::findAlertFromUnit (U32 dwMissionID) const
{
	CommNode * pNode = historyList;

	while (pNode)
	{
		if (pNode->dwMissionID == dwMissionID && pNode->bAlert && pNode->gotoDecay > 0)
			goto Done;
		pNode = pNode->pNext;
	}
	pNode = queuedList;
	while (pNode)
	{
		if (pNode->dwMissionID == dwMissionID && pNode->bAlert && pNode->gotoDecay > 0)
			goto Done;
		pNode = pNode->pNext;
	}
Done:
	return pNode;
}
//-------------------------------------------------------------------
//
inline CommNode * UnitComm::findAlertInSystem (U32 systemID) const
{
	CommNode * pNode = historyList;

	while (pNode)
	{
		if (pNode->systemID == systemID && pNode->index==0)
			goto Done;
		pNode = pNode->pNext;
	}
	pNode = queuedList;
	while (pNode)
	{
		if (pNode->systemID == systemID && pNode->index==0)
			goto Done;
		pNode = pNode->pNext;
	}
Done:
	return pNode;
}
//-------------------------------------------------------------------
//
inline CommNode * UnitComm::findCommFromUnit (U32 dwMissionID, U32 index) const
{
	CommNode * pNode = historyList;
	CommNode * result = 0;

	while (pNode)
	{
		if (pNode->index!= 0 && pNode->dwMissionID == dwMissionID)
		{
			if (pNode->index == index)
				goto Done;
			result = pNode;
		}
		pNode = pNode->pNext;
	}
	pNode = queuedList;
	while (pNode)
	{
		if (pNode->index!= 0 && pNode->dwMissionID == dwMissionID)
		{
			if (pNode->index == index)
				goto Done;
			result = pNode;
		}
		pNode = pNode->pNext;
	}
Done:
	return (pNode) ? pNode : result;
}
//-------------------------------------------------------------------
//
#define ALERT_XPOS IDEAL2REALX(50)
#define ALERT_YPOS IDEAL2REALY(70)
#define ALERT_SPACING IDEAL2REALY(20)

void UnitComm::drawAlerts()
{
	CommNode * node = findActiveAlert();
	if (node)
	{
		U32 fontUsed = 0;

		pFont[fontUsed]->SetFontColor(fontColor[node->param],0);
		pFont[fontUsed]->StringDraw(0,ALERT_XPOS,ALERT_YPOS,alertStrings[node->param]);
		++fontUsed;
		node = node->pNext;
		while(node && fontUsed < MAX_ALERT_FONTS)
		{
			if (node->bAlert && node->gotoDecay > 0)
			{
				pFont[fontUsed]->SetFontColor(fontColor[node->param],0);
				pFont[fontUsed]->StringDraw(0,ALERT_XPOS,ALERT_YPOS+(ALERT_SPACING*fontUsed),alertStrings[node->param]);
				++fontUsed;				
			}
			node = node->pNext;
		}
	}
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT UnitComm::Notify (U32 message, void *param)
{
//	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_MISSION_CLOSE:
		reset();
		break;
	case CQE_UPDATE:
		if (CQFLAGS.bGameActive==0)
			reset();
		else
		if (CQFLAGS.bGamePaused==0 && DEFAULTS->GetDefaults()->bEditorMode==0)
			update(U32(param) >> 10);
		break;
	case CQE_START3DMODE:
		on3DEnable(true);
		break;
	case CQE_END3DMODE:
		on3DEnable(false);
		break;
	case CQE_ENDFRAME:
		drawAlerts();
		break;
	case CQE_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch (U32(param))
			{
			case IDH_SEEK_EVENT:
				gotoSeekEvent();
				break;
			}
		}
		break;

	case CQE_KILL_FOCUS:
		bHasFocus = false;
		break;
	case CQE_SET_FOCUS:
		bHasFocus = true;
		break;
	}

	return GR_OK;
}
//-------------------------------------------------------------------
//
const char * UnitComm::getFilename (IBaseObject * obj, U32 index)
{
	const char * result = 0;

	if (obj && obj->pArchetype)
	{
		const char * pArchName = ARCHLIST->GetArchName(obj->pArchetype);
		
		if (pArchName)
		{
			U32 dataSize;
			const UNITSOUNDS::SPEECH * pSpeech = (const UNITSOUNDS::SPEECH *) STRINGDATA->GetArchetypeData(pArchName, dataSize);

			if (pSpeech)
			{
				CQASSERT(dataSize == sizeof(UNITSOUNDS::SPEECH) && "StringPack out of sync");

				const M_STRING * stringName = (const M_STRING *) pSpeech;
				const M_STRING * stringPack = (const M_STRING * )  STRINGDATA->GetArchetypeData(stringName[index], dataSize);

				if (stringPack && dataSize)
				{
					CQASSERT(dataSize % sizeof(M_STRING) == 0);
					U32 numStrings = dataSize / sizeof(M_STRING);
					U32 which = rand() % numStrings;
					stringName = stringPack + which;
				}
				else
					stringName += index;

				if (stringName[0][0])
					result = stringName[0];
			}
		}
	}

	return result;
}
//-------------------------------------------------------------------
//
void UnitComm::addMessage (IBaseObject * obj, U32 dwMissionID, U32 indexPos, const char * filename, U32 subtitle, U32 shipNameID, U32 flags, DWORD param)
{
	CommNode * node=queuedList, *pPrev=0;
	IBaseObject * obj2 = OBJLIST->FindObject(dwMissionID);
	if (obj2)
		obj = obj2;			// position comes from "dwMissionID" object, speech comes from "obj"

	while (node)
	{
		pPrev = node;
		node = node->pNext;
	}

	node = new CommNode;
	node->index = indexPos;
	node->flags = flags;
	node->dwMissionID = dwMissionID;
	node->subtitle = subtitle;
	node->unitNameID = shipNameID;
	node->filename = filename;
	node->timeStamp = currentTime;
	node->bAlert = (indexPos==0 || (flags & COMMF_ALERT)!=0);
	node->param = param;
	node->decayTime = (node->bAlert) ? ((flags & COMMF_ALERT) ? GOTO_DECAY : ALERT_DECAY) : SHORT_DECAY;
	node->gotoDecay = GOTO_DECAY;
	node->systemID = obj->GetSystemID() & ~HYPER_SYSTEM_MASK;
	node->pos = obj->GetPosition();
	// append it to the end of the list
	if (pPrev)
		pPrev->pNext = node;
	else
		queuedList = node;
}
//-------------------------------------------------------------------
//
void UnitComm::flushQueuedSelection (void)
{
	CommNode * node=queuedList, *pPrev=0;

	while (node)
	{
		if (node->flags & COMMF_SELECT)
		{
			if (pPrev)
				pPrev->pNext = node->pNext;
			else
				queuedList = node->pNext;
			delete node;
			break;
		}
		else
		{
			pPrev = node;
			node = node->pNext;
		}
	}
}
//-------------------------------------------------------------------
//
void UnitComm::PlayCommMessage (IBaseObject * obj, U32 dwMissionID, U32 indexPos, U32 subtitle, U32 shipNameID, U32 flags, DWORD param)
{
	CQASSERT((subtitle == 0)  || (subtitle >= SUB_FIRST && subtitle <= SUB_LAST));//happens to be ok ok check because strings are sequential in table

	const char * filename;

	if (CQFLAGS.bGamePaused || ((flags & COMMF_ENEMY)==0 && obj->GetPlayerID() != MGlobals::GetThisPlayer()))
		goto Done;

	// selected messages always cancel previous selected
	//    if unit was already playing "selected", then randomly choose aggrevated
	//    else if unit is currently speaking, do not play selected message.
	// non-selected messages always flush QUEUED selected (for all units)

	flushQueuedSelection();	// flush all queued selection messages

	//
	// check for repeat "enemy sighted" warning
	//
	if (indexPos == 0)	// enemy sighted
	{
		CommNode * node = findAlertInSystem(obj->GetSystemID() & ~HYPER_SYSTEM_MASK);

		SECTOR->AddPlayerAttack(obj->GetSystemID());	// notify sector that an attack is underway

		if (node)
		{
			node->decayTime = ALERT_DECAY;
			goto Done;
		}
	}
	else // is this a selection message?
	if (flags & COMMF_SELECT)
	{
		CommNode * node  = findCommFromUnit(dwMissionID, indexPos);

		if (node)	// if already talking
		{
			if (node->index == indexPos)	// already playing selection
			{
				if (obj->objClass == OC_SPACESHIP && (rand() & 7) == 0)		// become aggravated
					indexPos = cqoffsetofmember(UNITSOUNDS::SPEECH::SPACESHIP, aggravated) / sizeof(M_STRING);
				else
				{
					node->decayTime = __max(node->decayTime, SHORT_DECAY/2);
					goto Done;		// throw away redundant
				}
			}
			else
				goto Done;	// don't play selection
		}
	}
	else
	{
		CommNode * node  = findCommFromUnit(dwMissionID, indexPos);

		if (node)
		{
			if (node->index == indexPos)
			{
				node->decayTime = __max(node->decayTime, SHORT_DECAY/2);
				goto Done;		// throw away redundant message
			}
		}
	}

	//
	// if this admiral is chatty, reuse his first alert for GOTO purposes
	//

	if (flags & COMMF_ALERT)
	{
		CommNode * node = findAlertFromUnit(dwMissionID);
		if (node)
		{
			node->decayTime = __max(node->decayTime, GOTO_DECAY);
			node->gotoDecay = GOTO_DECAY;
			flags &= ~COMMF_ALERT;
		}
	}

	if ((flags & (COMMF_ALERT|COMMF_DEATH)) == 0 && queuedList != 0)
		goto Done;		// never queue a low priority message

	if (flags & COMMF_DEATH)
	{
		//
		// remove all pending alerts from this unit
		//
		CommNode * node=queuedList, *pPrev=0;

		while (node)
		{
			if (node->dwMissionID == dwMissionID)
			{
				CommNode * tmp = node;
				if (pPrev)
					node = pPrev->pNext = tmp->pNext;
				else
					node = queuedList = tmp->pNext;
				delete tmp;
			}
			else
			{
				pPrev = node;
				node = node->pNext;
			}
		}

		if ((filename = getFilename(obj, indexPos)) != 0)		// does unit have something to say?
		{
			addMessage(obj, dwMissionID, indexPos, filename, subtitle, shipNameID, flags, param);
			//
			// stop any currently playing messages
			//
			if (soundID)
			{
				SOUNDMANAGER->StopPlayback(soundID);
				soundID = 0;
			}
		}
	}
	else
	{
		//
		// else add it to the queue
		//
		if ((filename = getFilename(obj, indexPos)) != 0)
		{
			addMessage(obj, dwMissionID, indexPos, filename, subtitle, shipNameID, flags, param);
		}
		else
		{
			addMessage(obj, dwMissionID, indexPos, 0, subtitle, shipNameID, flags, param);
		}
		// If it is an enemy sighted or under attack message
		// Let the mission system know about it...

		if (param == ALERT_UNDER_ATTACK)
		{
			EVENTSYS->Send(CQE_UNIT_UNDER_ATTACK, (void *) dwMissionID);
			MScript::RunProgramsWithEvent(CQPROGFLAG_UNDERATTACK, dwMissionID);
		}
		else if (param == ALERT_ENEMY_SIGHTED)
		{
			//Enemy sighted alert...
			MScript::RunProgramsWithEvent(CQPROGFLAG_ENEMYSIGHTED, dwMissionID);
		}
	}
	
Done:
	return;
}
//-------------------------------------------------------------------
//
void UnitComm::PlayCommMessage (IBaseObject * obj, U32 dwMissionID, char * fileName, U32 subtitle, U32 shipNameID, U32 flags,DWORD param)
{
	CQASSERT((subtitle == 0)  || (subtitle >= SUB_FIRST && subtitle <= SUB_LAST));//happens to be ok ok check because strings are sequential in table
	if (CQFLAGS.bGamePaused || ((flags & COMMF_ENEMY)==0 && obj->GetPlayerID() != MGlobals::GetThisPlayer()))
		goto Done;

	// selected messages always cancel previous selected
	//    if unit was already playing "selected", then randomly choose aggrevated
	//    else if unit is currently speaking, do not play selected message.
	// non-selected messages always flush QUEUED selected (for all units)

	flushQueuedSelection();	// flush all queued selection messages

	//
	// check for repeat "enemy sighted" warning
	//

	//
	// if this admiral is chatty, reuse his first alert for GOTO purposes
	//

	if (flags & COMMF_ALERT)
	{
		CommNode * node = findAlertFromUnit(dwMissionID);
		if (node)
		{
			node->decayTime = __max(node->decayTime, GOTO_DECAY);
			node->gotoDecay = GOTO_DECAY;
			flags &= ~COMMF_ALERT;
		}
	}

	if ((flags & (COMMF_ALERT|COMMF_DEATH)) == 0 && queuedList != 0)
		goto Done;		// never queue a low priority message

	if (flags & COMMF_DEATH)
	{
		//
		// remove all pending alerts from this unit
		//
		CommNode * node=queuedList, *pPrev=0;

		while (node)
		{
			if (node->dwMissionID == dwMissionID)
			{
				CommNode * tmp = node;
				if (pPrev)
					node = pPrev->pNext = tmp->pNext;
				else
					node = queuedList = tmp->pNext;
				delete tmp;
			}
			else
			{
				pPrev = node;
				node = node->pNext;
			}
		}

		addMessage(obj, dwMissionID, 0, fileName, subtitle, shipNameID, flags, param);
		//
		// stop any currently playing messages
		//
		if (soundID)
		{
			SOUNDMANAGER->StopPlayback(soundID);
			soundID = 0;
		}
	}
	else
	{
		//
		// else add it to the queue
		//
		addMessage(obj, dwMissionID, 0, fileName, subtitle, shipNameID, flags, param);
		// If it is an enemy sighted or under attack message
		// Let the mission system know about it...

		if (param == ALERT_UNDER_ATTACK)
		{
			EVENTSYS->Send(CQE_UNIT_UNDER_ATTACK, (void *) dwMissionID);
			MScript::RunProgramsWithEvent(CQPROGFLAG_UNDERATTACK, dwMissionID);
		}
		else if (param == ALERT_ENEMY_SIGHTED)
		{
			//Enemy sighted alert...
			MScript::RunProgramsWithEvent(CQPROGFLAG_ENEMYSIGHTED, dwMissionID);
		}
	}
	
Done:
	return;
}
//-------------------------------------------------------------------
//
void UnitComm::update (U32 dt)	// milliseconds
{
	CommNode * node=historyList, *pPrev=0;
	currentTime += dt;

	//
	// remove historic nodes that have expired
	//
	while (node)
	{
		if (S32(node->gotoDecay -= dt) < 0)
			node->gotoDecay = 0;
		node->decayTime -= dt;
		if (node->decayTime <= 0)		// remove the item
		{
			if (pPrev)
			{
				pPrev->pNext = node->pNext;
				delete node;
				node = pPrev->pNext;
			}
			else
			{
				historyList = node->pNext;
				delete node;
				node = historyList;
			}
		}
		else
		{
			pPrev = node;
			node = node->pNext;
		}
	}
	//
	// play a queued node
	//
	if (soundID==0 || (bSoundIsSelection && queuedList!=0) || SOUNDMANAGER->IsPlaying(soundID)==0)
	{
		if (bSoundIsSelection && soundID)
			SOUNDMANAGER->StopPlayback(soundID);
		bSoundIsSelection = false;
		soundID = 0;

		if ((node = queuedList) != 0)
		{
			if(node->filename)
			{
				if (node->flags & COMMF_NOHILITE)
				{
					soundID = SOUNDMANAGER->PlayCommMessage(node->filename, NULL, NULL);
				}
				else
				{
					soundID = SOUNDMANAGER->PlayCommMessage(node->filename, NULL, OBJLIST->FindObject(node->dwMissionID));
				}

				if (node->flags & COMMF_SELECT)
					bSoundIsSelection = true;
			}

			if (node->bAlert && node->systemID && node->systemID<=MAX_SYSTEMS)
			{
				if (node->systemID != SECTOR->GetCurrentSystem())
					SECTOR->ZoneOn(node->systemID);
				else
					SYSMAP->ZoneOn(node->pos);
			}

			if(DEFAULTS->GetDefaults()->bSubtitles && node->subtitle && node->unitNameID)
			{
				CQASSERT(node->subtitle >= SUB_FIRST && node->subtitle <= SUB_LAST);//happens to be ok ok check because strings are sequential in table
				wchar_t name[128];
				wchar_t * ptr;

				wcscpy(name, _localLoadStringW(node->unitNameID));
				if ((ptr = wcschr(name, '#')) != 0)
					*ptr = 0;
				if ((ptr = wcschr(name, '(')) != 0)
					*ptr = 0;
				if ((ptr = wcsrchr(name, '\'')) != 0)
				{
					ptr++;
					if (ptr[0] == ' ')
						ptr++;
				}
				else
					ptr = name;
		
				wchar_t buffer[256];
				swprintf(buffer,_localLoadStringW(node->subtitle),ptr);
				SCROLLTEXT->SetTextString(buffer);
			}

			queuedList = node->pNext;
			node->pNext = 0;
			// add node to historyList
			if (pPrev)
				pPrev->pNext = node;
			else
				historyList = node;
		}
	}
}
//----------------------------------------------------------------------------------
//
void UnitComm::on3DEnable (bool bEnable)
{
	if (bEnable)
	{
		COLORREF pen, background;
		pen			= RGB(255,255,255) | 0xFF000000;		
		background	= RGB(0,0,0);
		hFont = CQCreateFont(IDS_CONTEXTWINDOW);

		for(U32 index = 0; index < MAX_ALERT_FONTS;++index)
		{
			CreateFontDrawAgent(hFont, 0, pen, background, pFont[index]);
		}

		wcsncpy(alertStrings[ALERT_UNKNOWN], _localLoadStringW(IDS_ALERT_UNKNOWN),ALERT_STRING_LENGTH);
		fontColor[ALERT_UNKNOWN] = RGB(255,255,255)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_UNDER_ATTACK], _localLoadStringW(IDS_ALERT_UNDER_ATTACK),ALERT_STRING_LENGTH);
		fontColor[ALERT_UNDER_ATTACK] = RGB(255,0,0)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_ENEMY_SIGHTED], _localLoadStringW(IDS_ALERT_ENEMY_SIGHTED),ALERT_STRING_LENGTH);
		fontColor[ALERT_ENEMY_SIGHTED] = RGB(255,255,0)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_OUT_SUPPLY], _localLoadStringW(IDS_ALERT_OUT_SUPPLY),ALERT_STRING_LENGTH);
		fontColor[ALERT_OUT_SUPPLY] = RGB(255,128,0)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_UNIT_BUILT], _localLoadStringW(IDS_ALERT_UNIT_BUILT),ALERT_STRING_LENGTH);
		fontColor[ALERT_UNIT_BUILT] = RGB(0,0,255)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_PLATFORM_BUILD], _localLoadStringW(IDS_ALERT_PLATFORM_BUILD),ALERT_STRING_LENGTH);
		fontColor[ALERT_PLATFORM_BUILD] = RGB(0,0,255)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_RESOURCE_EXAHAUSTED], _localLoadStringW(IDS_ALERT_RESOURCE_EXAHAUSTED),ALERT_STRING_LENGTH);
		fontColor[ALERT_RESOURCE_EXAHAUSTED] = RGB(128,128,128)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_RESEARCH_COMPLETE], _localLoadStringW(IDS_ALERT_RESEARCH_COMPLETE),ALERT_STRING_LENGTH);
		fontColor[ALERT_RESEARCH_COMPLETE] = RGB(128,128,255)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_MUTATION_COMPLETE], _localLoadStringW(IDS_ALERT_MUTATION_COMPLETE),ALERT_STRING_LENGTH);
		fontColor[ALERT_MUTATION_COMPLETE] = RGB(128,128,255)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_ADMIRAL_LOST], _localLoadStringW(IDS_ALERT_ADMIRAL_LOST),ALERT_STRING_LENGTH);
		fontColor[ALERT_ADMIRAL_LOST] = RGB(255,0,0)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_ADMIRAL_MESSAGE], _localLoadStringW(IDS_ALERT_ADMIRAL_MESSAGE),ALERT_STRING_LENGTH);
		fontColor[ALERT_ADMIRAL_MESSAGE] = RGB(128,255,128)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_ALLIED_ATTACK], _localLoadStringW(IDS_ALERT_ALLIED_ATTACK),ALERT_STRING_LENGTH);
		fontColor[ALERT_ALLIED_ATTACK] = RGB(255,0,0)| 0xFF000000;

		wcsncpy(alertStrings[ALERT_MISSION], _localLoadStringW(IDS_ALERT_MISSION),ALERT_STRING_LENGTH);
		fontColor[ALERT_MISSION] = RGB(0,255,255)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_30], _localLoadStringW(IDS_NOVA_WARNING_30),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_30] = RGB(255,64,64)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_25], _localLoadStringW(IDS_NOVA_WARNING_25),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_25] = RGB(255,64,64)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_20], _localLoadStringW(IDS_NOVA_WARNING_20),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_20] = RGB(255,64,64)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_15], _localLoadStringW(IDS_NOVA_WARNING_15),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_15] = RGB(255,64,64)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_10], _localLoadStringW(IDS_NOVA_WARNING_10),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_10] = RGB(255,64,64)| 0xFF000000;

		wcsncpy(alertStrings[NOVA_WARNING_5], _localLoadStringW(IDS_NOVA_WARNING_5),ALERT_STRING_LENGTH);
		fontColor[NOVA_WARNING_5] = RGB(255,64,64)| 0xFF000000;
	}
	else
	{
		for(U32 index = 0; index < MAX_ALERT_FONTS;++index)
		{
			pFont[index].free();
		}

		if (hFont)
		{
			DeleteObject(hFont);
			hFont = 0;
		}
	}
}
//--------------------------------------------------------------------------//
//
void UnitComm::gotoSeekEvent (void)
{
	CommNode * pNode = findActiveAlert();

	if (pNode)
	{
		IBaseObject * obj = OBJLIST->FindObject(pNode->dwMissionID);
		const U32 systemID = (obj) ? obj->GetSystemID() : pNode->systemID;

		if (systemID && systemID <= MAX_SYSTEMS)
		{
			if(SECTOR->GetAlertState(systemID,MGlobals::GetThisPlayer())&S_VISIBLE)
			{
				if (obj==0)
				{
					SECTOR->SetCurrentSystem(systemID);
					CAMERA->SetLookAtPosition(pNode->pos);
				}
				else
				{
					SECTOR->SetCurrentSystem(systemID);
					CAMERA->SetLookAtPosition(obj->GetPosition());
				}
			}
		}
		// mark it invalid
		pNode->gotoDecay = 0;
	}
}
//--------------------------------------------------------------------------//
//
void UnitComm::init (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (GS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Advise(getBase(), &eventHandle);
}
//--------------------------------------------------------------------------//
//
struct _unitcomm : GlobalComponent
{
	UnitComm * comm;

	virtual void Startup (void)
	{
		UNITCOMM = comm = new DAComponent<UnitComm>;
		AddToGlobalCleanupList((IDAComponent **) &UNITCOMM);
	}

	virtual void Initialize (void)
	{
		comm->init();
	}
};

static _unitcomm UnitComm;

//------------------------------------------------------------------------------------//
//---------------------------------End UnitComm.cpp-----------------------------------//
//------------------------------------------------------------------------------------//
