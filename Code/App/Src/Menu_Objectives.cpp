//--------------------------------------------------------------------------//
//                                                                          //
//                           Menu_Objectives.cpp                            //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_Objectives.cpp 25    10/06/00 5:43p Sbarton $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>

#include <wchar.h>

#include <DMenuObjectives.h>
#include <MGlobals.h>
#include <MScript.h>

#include "Frame.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IButton2.h"
#include "ITeletype.h"

//--------------------------------------------------------------------------//
// we are going to use a linked list of objective strings that are waiting to be teletyped
//
struct ObjectiveList
{
	U32 stringID;
	struct ObjectiveList * pNext;
};
//--------------------------------------------------------------------------//
//
void add_objective_stringID (ObjectiveList *& list, U32 stringID)
{
	// the new entry, it always gets added to the end of the list
	ObjectiveList * pEntry = new ObjectiveList;
	pEntry->stringID = stringID;
	pEntry->pNext = NULL;

	// if list is NULL, then add the stringID to the begining of the list
	if (list == NULL)
	{
		list = pEntry;
	}
	else
	{
		// add the object to the end of the list
		ObjectiveList * p = list;
		
		while (p)
		{
			if (p->pNext == NULL)
			{
				p->pNext = pEntry;
				break;
			}
			p = p->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
void delete_first_objective (ObjectiveList *& list)
{
	// if the list is already empty, then our work is done
	if (list)
	{
		ObjectiveList * pDelete = list;
		list = list->pNext;
		delete pDelete;
	}
}

//--------------------------------------------------------------------------//
//
struct dummy_menuobjectives : public Frame
{
	BEGIN_DACOM_MAP_INBOUND(dummy_menuobjectives)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	
	// the following are for BaseHotRect
	DACOM_INTERFACE_ENTRY(IResourceClient)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	END_DACOM_MAP()
};
//--------------------------------------------------------------------------//
//
struct Menu_Objectives : public DAComponent<dummy_menuobjectives>
{
	//
	// data items
	//
	GT_MENUOBJECTIVES data;

	COMPTR<IStatic>  background, staticObjectives, staticName;
	COMPTR<IButton2> buttonOk;
	COMPTR<IButton2> checkObjectives[MAX_MISSION_OBJECTIVES_SHOWN];
	COMPTR<IStatic>  staticObjectiveArray[MAX_MISSION_OBJECTIVES_SHOWN];

	RECT rcTeletype;

	// pause the game if in single player mode
	BOOL32 bOldPause;

	ObjectiveList * pObjectives;
	U32 teletypeID;
	bool bIgnoreInput;

	U32 nObjectives;

	//
	// instance methods
	//

	Menu_Objectives (void)
	{
		eventPriority = EVENT_PRIORITY_OBJECTIVES;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();

		EVENTSYS->Send(CQE_KILL_FOCUS, (void *)this);

		// pause the game if in single player mode
		bOldPause = CQFLAGS.bGamePaused;
		if (PLAYERID == 0)
		{
			CQFLAGS.bGamePaused = TRUE;
			EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
		}
	}

	~Menu_Objectives (void);

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

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);


	/* Menu_Objectives methods */

	virtual void setStateInfo (void);

	virtual bool onTabPressed (void)
	{
		if (childFrame!=0)
			return false;
		return Frame::onTabPressed();
	}

	virtual void onUpdate (U32 dt);   // dt in milliseconds

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		if (!bIgnoreInput)
		{
			EVENTSYS->Send(CQE_DELETE_OBJECTIVES_MENU, 0);
		}
		return true;
	}

	virtual void onButtonPressed (U32 buttonID)
	{
		if (buttonID == IDS_OK)
		{
			EVENTSYS->Send(CQE_DELETE_OBJECTIVES_MENU, 0);
		}
	}

	void init (void);
	
	void updateObjectiveInfo (void);

	void addObjective (void);

	void addObjectiveToList (U32 stringID);
};
//----------------------------------------------------------------------------------//
//
Menu_Objectives::~Menu_Objectives (void)
{
	EVENTSYS->Send(CQE_SET_FOCUS, 0);

	// unpause the game, if we must
	if (PLAYERID == 0)
	{
		CQFLAGS.bGamePaused = bOldPause;
		EVENTSYS->Send(CQE_PAUSE_CHANGE, (void *)(CQFLAGS.bGamePaused));
	}

	// delete everything in the temp objectives list too
	ObjectiveList * p = pObjectives;
	ObjectiveList * pNext = NULL;
	while (p)
	{
		pNext = p->pNext;
		delete p;
		p = pNext;
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_Objectives::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	doc->SetFilePointer(0,0);
	doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);
	setStateInfo();

	return GR_OK;
}
//----------------------------------------------------------------------------------//
//
void Menu_Objectives::onUpdate (U32 dt)
{
	if (teletypeID)
	{
		// there is a teletype playing so check to see if has ended so that we can play the next teletype next time around
		// also want to add the objective to the objective list box
		if (TELETYPE->IsPlaying(teletypeID) == 0)
		{
			teletypeID = 0;

			addObjectiveToList(pObjectives->stringID);

			// take the objective off the beginning of the objective list
			delete_first_objective(pObjectives);
		}
	}
	else
	{
		// there is no teletype playing, see if there is one that is waiting to play...
		if (pObjectives && pObjectives->stringID)
		{
			// get the string and make a teletype of it
			wchar_t * pString = MScript::GetCountedString(pObjectives->stringID);
			teletypeID = TELETYPE->CreateTeletypeDisplayEx(pString, rcTeletype, IDS_TELETYPE_FONT , RGB(200,210,100), 3000, 2200, false, true);
		}
	}

	// should we be allowed to close the menu?
	bIgnoreInput = (teletypeID != 0);
	buttonOk->EnableButton(bIgnoreInput == false);
}
//----------------------------------------------------------------------------------//
//
void Menu_Objectives::addObjective (void)
{
	// a new objective has been added, it is always the last on in the list
	U32 index = (MGlobals::GetNumberObjectives() - 1);
	U32 stringID = MGlobals::GetObjectiveStringID(index);

	if (stringID)
	{
		// add this to the list of everything else that we want teletyped
		add_objective_stringID(pObjectives, stringID);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_Objectives::addObjectiveToList (U32 stringID)
{
	CQASSERT(stringID);
    if (nObjectives >= MAX_MISSION_OBJECTIVES_SHOWN)
    {
        return;
    }

	wchar_t * pString;
	wchar_t buffer[128];
	memset(buffer, 0, sizeof(buffer));
	
	pString = MScript::GetCountedString(stringID);
	CQASSERT(pString);

	wcsncpy(buffer, &pString[1], U16(pString[0]));

	// if the objective has failed - show in red
	// if the objective is completed - show in green
	// if the objective is still active - show in yellow
	COLORREF color;
	bool bObjectiveVisible = true;
	bool bObjectiveFailed = false;
	
	if (MGlobals::IsObjectiveFailed(stringID))
	{
		bObjectiveFailed = true;
		color = RGB(255,50,50);
	}
	else if (MGlobals::IsObjectiveCompleted(stringID))
	{
		bObjectiveFailed = false;
		color = RGB(50,255,50);
	}
	else
	{
		bObjectiveVisible = false;
		color = RGB(255,255,50);
	}

	// if the objective is secondary, then indent it a couple of spaces
	if (MGlobals::IsObjectiveSecondary(stringID))
	{
		wchar_t szIndented[128];
		memset(szIndented, 0, sizeof(szIndented));
		wcsncpy(szIndented, L"    ", 4);
		wcscat(szIndented, buffer);
		staticObjectiveArray[nObjectives]->SetText(szIndented);
	}
	else
	{
		staticObjectiveArray[nObjectives]->SetText(buffer);
	}
	staticObjectiveArray[nObjectives]->SetTextColor(color);
	staticObjectiveArray[nObjectives]->SetVisible(true);
	
	checkObjectives[nObjectives]->SetVisible(bObjectiveVisible);
	checkObjectives[nObjectives]->SetPushState(bObjectiveFailed);
	checkObjectives[nObjectives]->EnableButton(false);

	nObjectives++;
}
//----------------------------------------------------------------------------------//
//
void Menu_Objectives::updateObjectiveInfo (void)
{
	// get the name of the mission
	U32 stringID = MGlobals::GetMissionName();
	wchar_t * pString;

	if (stringID)
	{
		wchar_t buffer[128];
		pString = MScript::GetCountedString(stringID);
		if (pString)
		{
			memcpy(buffer, pString+1, pString[0]*sizeof(wchar_t));
			buffer[pString[0]] = 0;
			staticName->SetText(buffer);
		}
		else
			CQERROR1("Invalid mission stringID %d", stringID);
	}

	// fill the objectives list
	U32 i;
	nObjectives = 0;

	for (i = 0; i < MAX_MISSION_OBJECTIVES_SHOWN; i++)
	{
		checkObjectives[i]->SetVisible(false);
		staticObjectiveArray[i]->SetVisible(false);
	}

	U32 numObjectives = MGlobals::GetNumberObjectives();
	
	for (i = 0; i < numObjectives; i++)
	{
		stringID = MGlobals::GetObjectiveStringID(i);

		// add the objective to our array of stuff
		if (stringID)
		{
			addObjectiveToList(stringID);
		}
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_Objectives::Notify (U32 message, void *param)
{
	switch (message)
	{
	case CQE_OBJECTIVES_CHANGE:
		updateObjectiveInfo();
		break;

	case CQE_OBJECTIVE_ADDED:
		addObjective();
		break;

	case CQE_ENDFRAME:
		{
			// draw the stuff like you normally would
			GENRESULT result = Frame::Notify(message, param);

			// draw our teletype rectangle
			if (DEFAULTS->GetDefaults()->bDrawHotrects)
			{
				DA::RectangleHash(NULL, IDEAL2REALX(rcTeletype.left), IDEAL2REALY(rcTeletype.top), 
					  IDEAL2REALX(rcTeletype.right), IDEAL2REALY(rcTeletype.bottom), RGB(0,255,0));
			}
			return result;
		}
		break;
	}

	return Frame::Notify(message, param);
}
//----------------------------------------------------------------------------------//
//
void Menu_Objectives::setStateInfo (void)
{
	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(data.screenRect.left);
	screenRect.right	= IDEAL2REALX(data.screenRect.right);
	screenRect.top		= IDEAL2REALY(data.screenRect.top);
	screenRect.bottom	= IDEAL2REALY(data.screenRect.bottom);

	// no Ideal2Real for teletypes
	rcTeletype = data.rcTeletype;

	background->InitStatic(data.background, this);
	staticName->InitStatic(data.staticName, this);
	staticObjectives->InitStatic(data.staticObjectives, this);

	// fixit - take this out eventually
	staticName->SetVisible(false);

	buttonOk->InitButton(data.buttonOk, this);
	buttonOk->SetTransparent(true);

	for (int i = 0; i < MAX_MISSION_OBJECTIVES_SHOWN; i++)
	{
		checkObjectives[i]->InitButton(data.checkObjectives[i], this);
		checkObjectives[i]->EnableButton(false);
		checkObjectives[i]->SetVisible(false);

		staticObjectiveArray[i]->InitStatic(data.staticObjectiveArray[i], this);
//		staticObjectiveArray[i]->SetText(L"testing purposes only, real stuff goes here later.Seeing how many lines can fit.");
		staticObjectiveArray[i]->SetVisible(false);
	}

	updateObjectiveInfo();

	if (childFrame)
	{
		childFrame->setStateInfo();
	}
	else
	{
		setFocus(buttonOk);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_Objectives::init (void)
{
	data = 	*((GT_MENUOBJECTIVES *) GENDATA->GetArchetypeData("MenuObjectives"));

	COMPTR<IDAComponent> pComp;

	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.staticName.staticType, pComp);
	pComp->QueryInterface("IStatic", staticName);

	GENDATA->CreateInstance(data.staticObjectives.staticType, pComp);
	pComp->QueryInterface("IStatic", staticObjectives);

	GENDATA->CreateInstance(data.buttonOk.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonOk);

	for (int i = 0; i < MAX_MISSION_OBJECTIVES_SHOWN; i++)
	{
		GENDATA->CreateInstance(data.checkObjectives[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", checkObjectives[i]);		

		GENDATA->CreateInstance(data.staticObjectiveArray[i].staticType, pComp);
		pComp->QueryInterface("IStatic", staticObjectiveArray[i]);
	}
}
//--------------------------------------------------------------------------//
//
Frame * __stdcall CreateMenuObjectives (void)
{
	Menu_Objectives * menu = new Menu_Objectives();
	menu->createViewer("\\GT_MENUOBJECTIVES\\MenuObjectives", "GT_MENUOBJECTIVES", IDS_VIEWOBJECTIVEMENU);
	menu->beginModalFocus();
	return menu;
}
