//--------------------------------------------------------------------------//
//                                                                          //
//                                Menu_mission.cpp                        //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Menu_mission.cpp 66    5/15/01 1:53p Tmauer $
*/
//--------------------------------------------------------------------------//
// New player dialog
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <wchar.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IListbox.h"
#include "IAnimate.h"
#include "Mission.h"
#include "IGameProgress.h"
#include "SFX.h"
#include "MusicManager.h"

#include <DMenu1.h>

#define MAX_CHARS 256

#define MISSION_BUTTON_BEGIN	231200
#define MOVIE_BUTTON_BEGIN		331200
#define MOVIE_BUTTON_1			MOVIE_BUTTON_BEGIN
#define MOVIE_BUTTON_2			MOVIE_BUTTON_BEGIN+1
#define MOVIE_BUTTON_3			MOVIE_BUTTON_BEGIN+2
#define MOVIE_BUTTON_4			MOVIE_BUTTON_BEGIN+3
#define MOVIE_BUTTON_0			MOVIE_BUTTON_BEGIN+4
#define MOVIE_BUTTON_5			MOVIE_BUTTON_BEGIN+5

#define BUTTON_MISSION_UNLOCK	65900
#define MENUSP_MUSIC			"single_player_menu.wav"

//#define SEAN_BETA
#ifdef	SEAN_BETA
#define NUM_MISSIONS_MAX	3
#define NUM_MOVIES_MAX		NUM_MOVIES
#else
#define NUM_MISSIONS_MAX	NUM_MISSIONS
#define NUM_MOVIES_MAX		NUM_MOVIES
#endif

U32 __stdcall MovieScreen (Frame * parent, const char * filename);
U32 __stdcall DoMenu_Briefing (Frame * parent, const char * szFileName);

struct LineVar
{
	int x0, y0;
	int x1, y1;
};

//--------------------------------------------------------------------------//
//
struct Menu_mission : public DAComponent<Frame>
{
	//
	// data items
	//
	const GT_MENU1::SELECT_MISSION & data;

	COMPTR<IStatic>  background,title;
	COMPTR<IListbox> list;
	COMPTR<IButton2> start, ok, cancel;

	COMPTR<IStatic>  staticMission, staticHolder;
	COMPTR<IButton2> buttonMissions[NUM_MISSIONS_MAX];
	COMPTR<IButton2> buttonMovies[NUM_MOVIES_MAX];
	COMPTR<IButton2> buttonBack;
	COMPTR<IButton2> buttonUnlock;
	COMPTR<IStatic>  staticMovieHolder;
	COMPTR<IStatic>  staticMissionTitle;

	COMPTR<IAnimate> animSystem;

	int nMissionLineFrom[NUM_MISSIONS_MAX];
	POINT ptMissionCenters[NUM_MISSIONS_MAX];
	LineVar missionLines[NUM_MISSIONS_MAX];

	int nMovieBeforeMission[NUM_MOVIES_MAX];
	POINT ptMovieCenters[NUM_MOVIES_MAX];
	LineVar movieLines[NUM_MOVIES_MAX];

	U32 currentMission;
	
	int currMissionSelection;
	int currMovieSelection;
	
	SINGLE rad;
	SINGLE radius;
	U32 timer;
	
	HSOUND hsndSelection, hsndPressed;

	COMPTR<IFileSystem> mapDir;

	bool bSkipMenu;
	U32 uUpdateRecursion;

	//
	// instance methods
	//

	Menu_mission (Frame * _parent, const GT_MENU1 & _data, const bool _bSkipMenu) : data(_data.selectMission), bSkipMenu(_bSkipMenu) 
	{
		radius = 24.0f;
		currMissionSelection = -1;
		currMovieSelection = -1;
		initializeFrame(_parent);
		init();
	}

	~Menu_mission (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param);

	/* Frame methods */

	virtual void setStateInfo (void);

	virtual void onButtonPressed (U32 buttonID);

	virtual void onListSelection (S32 listID);

	virtual bool onEscPressed (void)	//  return true if message was handled
	{
		endDialog(0);
		return true;
	}

	virtual void onUpdate (U32 dt);

	virtual bool onNextPressed (void)
	{
		// don't do anything
		return true;
	}

	virtual bool onPrevPressed (void)
	{
		// don't do anything
		return true;
	}

	virtual bool onTabPressed (void)
	{
		return true;
	}

	virtual bool onShiftTabPressed (void)
	{
		return true;
	}


	/* Menu_mission methods */

	void init (void);

	void initList (void);

	void onSelection (const wchar_t *buffer);

	void addFile (const WIN32_FIND_DATA & data);

	void addFile (HANDLE handle);

	void onButtonNew (void);

	void drawLines (void);

	void unlockMissions (void);

	void onMouseMove (S16 x, S16 y);
};
//----------------------------------------------------------------------------------//
//
Menu_mission::~Menu_mission (void)
{
	if (hsndSelection)
	{
		SFXMANAGER->CloseHandle(hsndSelection);
	}
	if (hsndPressed)
	{
		SFXMANAGER->CloseHandle(hsndPressed);
	}
}
//----------------------------------------------------------------------------------//
//
void Menu_mission::onMouseMove (S16 x, S16 y)
{
	// first take away the old text
	staticMission->SetText(L"");
	staticMissionTitle->SetText(L"");

	for (int i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		if (buttonMissions[i] != NULL && buttonMissions[i]->IsMouseOver(x, y))
		{
			if (i != currMissionSelection)
			{
				// play the sound
				SFXMANAGER->Play(hsndSelection);
			}

			currMissionSelection = i;

			staticMissionTitle->SetTextID(IDS_MISSION_TITLE_BEGIN + i + 1);
			staticMission->SetTextID(IDS_MISSION_DESC_BEGIN+i+1);
			setFocus(buttonMissions[i]);

			staticMovieHolder->SetVisible(false);
			staticHolder->SetVisible(true);
			currMovieSelection = -1;
			return;
		}
	}

	for (int i = 0; i < NUM_MOVIES_MAX; i++)
	{
		if (buttonMovies[i] != NULL && buttonMovies[i]->IsMouseOver(x, y))
		{
			if (i != currMovieSelection)
			{
				// play the sound
				SFXMANAGER->Play(hsndSelection);
			}

			currMovieSelection = i;

			staticMissionTitle->SetTextID(IDS_MOVIE_TITLE_BEGIN + i + 1);
			staticMission->SetTextID(IDS_MOVIE_DESC_BEGIN + i + 1);
			setFocus(buttonMovies[i]);

			staticMovieHolder->SetVisible(true);
			staticHolder->SetVisible(false);
			currMissionSelection = -1;
			return;
		}
	}

	staticMovieHolder->SetVisible(false);
	staticHolder->SetVisible(false);
	currMovieSelection = -1;
	currMissionSelection = -1;
}
//----------------------------------------------------------------------------------//
//
GENRESULT Menu_mission::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case WM_MOUSEMOVE:
		if (!bInvisible && bHasFocus)
		{
			onMouseMove(static_cast<S16>(LOWORD(msg->lParam)), static_cast<S16>(HIWORD(msg->lParam)));
		}
		break;

	case CQE_SET_FOCUS:
		if (CQFLAGS.b3DEnabled == 0)
		{
			MUSICMANAGER->PlayMusic(MENUSP_MUSIC);
		}
		break;

	case CQE_ENDFRAME:
		{
			GENRESULT result = Frame::Notify(message, param);
			drawLines();
			return result;
		}
	}

	return Frame::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void Menu_mission::drawLines (void)
{
	// rgb = 3,114,224
	if (bHasFocus && !bInvisible)
	{
		for (U32 i = 1; i < NUM_MISSIONS_MAX; i++)
		{
			if (buttonMissions[i]->GetVisible())
			{
				DA::LineDraw(0, missionLines[i].x0, missionLines[i].y0, missionLines[i].x1, missionLines[i].y1, RGB(10, 182, 255), true);
			}
		}

		for (U32 i = 0; i < NUM_MOVIES_MAX; i++)
		{
			if (buttonMovies[i]->GetVisible())
			{
				DA::LineDraw(0, movieLines[i].x0, movieLines[i].y0, movieLines[i].x1, movieLines[i].y1, RGB(255, 255, 44), true);
			}
		}

		// draw some funky stuff around the current mission
/*		if (currMissionSelection != -1)
		{
			int i = currMissionSelection;
			const COLORREF color = RGB(240, 190, 69);

			POINT pts[4];

			pts[0].x = ptMissionCenters[i].x + radius*cos(rad);
			pts[0].y = ptMissionCenters[i].y + radius*sin(rad);

			pts[1].x = ptMissionCenters[i].x + radius*cos(rad + 3.14f/2.0f);
			pts[1].y = ptMissionCenters[i].y + radius*sin(rad + 3.14f/2.0f);

			pts[2].x = ptMissionCenters[i].x + radius*cos(rad + 3.14f);
			pts[2].y = ptMissionCenters[i].y + radius*sin(rad + 3.14f);

			pts[3].x = ptMissionCenters[i].x + radius*cos(rad - 3.14f/2.0f);
			pts[3].y = ptMissionCenters[i].y + radius*sin(rad - 3.14f/2.0f);

			DA::LineDraw(0, pts[0].x, pts[0].y, pts[1].x, pts[1].y, color, true);
			DA::LineDraw(0, pts[1].x, pts[1].y, pts[2].x, pts[2].y, color, true);
			DA::LineDraw(0, pts[2].x, pts[2].y, pts[3].x, pts[3].y, color, true);
			DA::LineDraw(0, pts[3].x, pts[3].y, pts[0].x, pts[0].y, color, true);
		}
*/
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mission::onSelection (const wchar_t *buffer)
{
	char abuffer[MAX_PATH];
	
	_localWideToAnsi(buffer, abuffer, sizeof(abuffer));

	SetVisible(false);
	
	if (DoMenu_Briefing(this, abuffer))
	{
		endDialog(1);
	}
	else
	{
		SetVisible(true);
	}
}
//----------------------------------------------------------------------------------------//
//
void Menu_mission::onButtonNew (void)
{
	if (CQFLAGS.b3DEnabled == 0)
	{
		SetVisible(false);
		ChangeInterfaceRes(IR_IN_GAME_RESOLUTION);
	}

	MISSION->New();
	endDialog(1);
}
//----------------------------------------------------------------------------------------//
//
void Menu_mission::onUpdate (U32 dt)
{
	static S32 direction = 1;

	// do we need to skip out of this hell hole menu?
	if (bSkipMenu && uUpdateRecursion == 0)
	{
		uUpdateRecursion++;
		onButtonPressed(MISSION_BUTTON_BEGIN);
		bSkipMenu = false;
	}
	else
	{
		timer += dt;

		if (timer > 100)
		{
			timer = 0;
			buttonUnlock->EnableButton(DEFAULTS->GetDefaults()->bCheatsEnabled);
		}

		// we'll do this every update
		rad += 0.14f;
		radius += direction * 0.5f;
		
		if (radius > 28.0f || radius < 24.0f)
		{
			direction = -direction;
		}
	}
}
//----------------------------------------------------------------------------------------//
//
void Menu_mission::onListSelection (S32 listID)
{
	S32 sel = list->GetCurrentSelection();
	wchar_t buffer[MAX_PATH];

	list->GetString(sel, buffer, sizeof(buffer));	// returns length of string (in characters)
	onSelection(buffer);
}
//--------------------------------------------------------------------------//
//
void Menu_mission::addFile (const WIN32_FIND_DATA & data)
{
	if (data.cFileName[0] != TCHAR('.'))
	{
		wchar_t buffer[256];

		_localAnsiToWide(data.cFileName, buffer, sizeof(buffer));
		list->AddString(buffer);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mission::addFile (HANDLE handle)
{
	WIN32_FIND_DATA data;

	if (mapDir->FindNextFile(handle, &data))
	{
		addFile(handle);		// recursion
		addFile(data);
	}
}
//----------------------------------------------------------------------------------------//
//
void Menu_mission::initList (void)
{
	HANDLE handle;
	WIN32_FIND_DATA data;

	list->ResetContent();

	if (mapDir == NULL || (handle = mapDir->FindFirstFile("*.*", &data)) == INVALID_HANDLE_VALUE)
		return;

	addFile(handle);		// recursion
	addFile(data);
	
	CQASSERT(list->GetNumberOfItems());
	list->SetCurrentSelection(0);

	mapDir->FindClose(handle);
}
//----------------------------------------------------------------------------------//
//
void Menu_mission::setStateInfo (void)
{
	screenRect.left = IDEAL2REALX(data.screenRect.left);
	screenRect.right = IDEAL2REALX(data.screenRect.right);
	screenRect.top = IDEAL2REALY(data.screenRect.top);
	screenRect.bottom = IDEAL2REALY(data.screenRect.bottom);

	// 
	// initialize in draw-order
	//
	background->InitStatic(data.background, this);
	title->InitStatic(data.title, this);
	list->InitListbox(data.list, this);
	ok->InitButton(data.ok, this);
	cancel->InitButton(data.cancel, this);
	buttonBack->InitButton(data.buttonBack, this);

	animSystem->InitAnimate(data.animSystem, this, NULL, 0);

	ok->SetVisible(false);
	cancel->SetVisible(false);
	list->SetVisible(false);
	
	//remove for final release!!!!!
	start->InitButton(data.start, this);
	start->SetTextString(L"[DEBUG BUTTON] Start Clean Mission");
	start->SetControlID(IDS_NEW_CAMPAIGN);
	start->SetVisible(DEFAULTS->GetDefaults()->bCheatsEnabled);
	start->EnableButton(DEFAULTS->GetDefaults()->bCheatsEnabled);
	
	// take this out later
	buttonUnlock->InitButton(data.buttonUnlock, this);
	buttonUnlock->SetControlID(BUTTON_MISSION_UNLOCK);
	buttonUnlock->SetTextString(L"[DEBUG] Unlock Missions");
	buttonUnlock->SetVisible(DEFAULTS->GetDefaults()->bCheatsEnabled);
	buttonUnlock->EnableButton(DEFAULTS->GetDefaults()->bCheatsEnabled);

	staticMovieHolder->InitStatic(data.staticMovie, this);
	staticHolder->InitStatic(data.staticHolder, this);
	staticMission->InitStatic(data.staticMission, this);
	staticMissionTitle->InitStatic(data.staticMissionTitle, this);

	U32 i;
	U32 xpos, ypos, width, height;
	for (i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		buttonMissions[i]->InitButton(data.buttonMissions[i], this);
		buttonMissions[i]->SetControlID(MISSION_BUTTON_BEGIN + i);

		nMissionLineFrom[i] = data.nLineFrom[i];

		buttonMissions[i]->GetDimensions(width, height);
		buttonMissions[i]->GetPosition(xpos, ypos);

		ptMissionCenters[i].x = xpos + (width/2);
		ptMissionCenters[i].y = ypos + (height/2);
	}

	// set up the line structures
	SINGLE dx, dy;
	SINGLE size;
	SINGLE dist = IDEAL2REALX(20);
	int id;

	for (i = 1; i < NUM_MISSIONS_MAX; i++)
	{
		id = nMissionLineFrom[i];
		dx = ptMissionCenters[i].x - ptMissionCenters[id].x;
		dy = ptMissionCenters[i].y - ptMissionCenters[id].y;

		size = sqrt((dx*dx) + (dy*dy));

		if (size < 1.0)
		{
			CQBOMB0("Bad Data in Menu1//MisisonSelect for button placement!!!"); 
		} 

		dx /= size;
		dy /= size;

		missionLines[i].x0 = ptMissionCenters[i].x - (dx*dist);
		missionLines[i].y0 = ptMissionCenters[i].y - (dy*dist);

		missionLines[i].x1 = ptMissionCenters[id].x + (dx*dist);
		missionLines[i].y1 = ptMissionCenters[id].y + (dy*dist);
	}

#ifdef SEAN_BETA
	nMovieBeforeMission[0] = -2;
	nMovieBeforeMission[1] = 0;
	nMovieBeforeMission[2] = 0;
	nMovieBeforeMission[3] = 0;
	nMovieBeforeMission[4] = 0;
#endif

	// the movie buttons
	dist = IDEAL2REALX(24);
	for (i = 0; i < NUM_MOVIES_MAX; i++)
	{
		buttonMovies[i]->InitButton(data.buttonMovies[i], this);
		buttonMovies[i]->SetControlID(MOVIE_BUTTON_BEGIN + i);

#ifndef SEAN_BETA
		nMovieBeforeMission[i] = data.nMoiveBeforeMission[i];
#endif

		// the movie is to happen between two missions
		buttonMovies[i]->GetDimensions(width, height);
		buttonMovies[i]->GetPosition(xpos, ypos);

		ptMovieCenters[i].x = xpos + (width/2);
		ptMovieCenters[i].y = ypos + (height/2);
		
		if (nMovieBeforeMission[i] > 0)
		{
			// find the midpoint between the missions where the movie line starts
			POINT pt1;
			POINT pt2;
			int i1 = nMovieBeforeMission[i];
			int i2 = nMissionLineFrom[i1];

			pt1.x = (ptMissionCenters[i1].x + ptMissionCenters[i2].x)/2;
			pt1.y = (ptMissionCenters[i1].y + ptMissionCenters[i2].y)/2;

			pt2.x = ptMovieCenters[i].x;
			pt2.y = ptMovieCenters[i].y;

			dx = pt1.x - pt2.x;
			dy = pt1.y - pt2.y;

			size = sqrt((dx*dx) + (dy*dy));

			if (size < 1.0)
			{
				CQBOMB0("Bad Data in Menu1//MisisonSelect for button placement!!!"); 
			}

			dx /= size;
			dy /= size;

			pt2.x += (dx*dist);
			pt2.y += (dy*dist);

			movieLines[i].x0 = pt1.x;
			movieLines[i].y0 = pt1.y;
			movieLines[i].x1 = pt2.x;
			movieLines[i].y1 = pt2.y;
		}
		else 
		{
			// the movie is supposed to come out of a mission
			int mi = -(nMovieBeforeMission[i]);

			// find the first point - on the boundary of the movie button
			POINT pt1 = ptMovieCenters[i];

			dx = pt1.x - ptMissionCenters[mi].x;
			dy = pt1.y - ptMissionCenters[mi].y;

			size = sqrt(dx*dx + dy*dy);
			if (size < 1.0)
			{
				CQBOMB0("Bad Data in Menu1//MisisonSelect for button placement!!!"); 
			}

			dx /= size;
			dy /= size;

			pt1.x -= (dx*dist);
			pt1.y -= (dy*dist);

			// fint the second point - on the boundary of the mission button
			POINT pt2 = ptMissionCenters[mi];

			dx = pt2.x - ptMovieCenters[i].x;
			dy = pt2.y - ptMovieCenters[i].y;

			size = sqrt((dx*dx) + (dy*dy));

			dx /= size;
			dy /= size;

			pt2.x -= (dx*dist);
			pt2.y -= (dy*dist);

			movieLines[i].x0 = pt1.x;
			movieLines[i].y0 = pt1.y;
			movieLines[i].x1 = pt2.x;
			movieLines[i].y1 = pt2.y;
		}
	}

	// enable - disable the buttons
	U32 missionBits = GAMEPROGRESS->GetMissionsCompleted();
	U32 movieBits = GAMEPROGRESS->GetMoviesSeen();
	currentMission = 0;
	for (i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		if ((1 << i) & missionBits)
		{
			currentMission = i;
			currentMission++;
		}
	}

	// just in case of rollover
	if (currentMission > NUM_MISSIONS_MAX)
	{
		currentMission = NUM_MISSIONS_MAX;
	}

	for (i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		buttonMissions[i]->SetVisible(i <= currentMission);
		
		if (i <= currentMission)
		{
			currMissionSelection = i;
		}
	}
	
	for (i = 0; i < NUM_MOVIES_MAX-1; i++)
	{
		buttonMovies[i]->SetVisible(abs(nMovieBeforeMission[i]) <= (int)currentMission);
	}

	// only set the last movie to visible if we've seen it already...
	bool bLastMovies = false;
	if ((nMovieBeforeMission[5] <= (int)currentMission) && (missionBits & (1 << 15)))
	{
		buttonMovies[5]->SetVisible(true);
		bLastMovies = true;
	}
	else
	{
		buttonMovies[5]->SetVisible(false);
	}

	// only have the final losing movie available if we've seen it legit or we've seen the winning movie
	bLastMovies = bLastMovies || (movieBits & (1 << 6));
	buttonMovies[3]->SetVisible(bLastMovies);


#ifdef SEAN_BETA
	buttonMovies[1]->SetVisible(false);
	buttonMovies[2]->SetVisible(false);
	buttonMovies[3]->SetVisible(false);
#endif


	// fill the listbox with the mission names
	mapDir = SPMAPDIR;
	initList();

	if (list->GetNumberOfItems() == 0)
	{
		ok->EnableButton(false);
	} 

	if (currMissionSelection >= 0)
	{
		// fill the mission description string
		staticMission->SetTextID(IDS_MISSION_DESC_BEGIN + currMissionSelection + 1);
		setFocus(buttonMissions[currMissionSelection]);
	}
}
//--------------------------------------------------------------------------//
//
void Menu_mission::init (void)
{
	//
	// create members
	//
	COMPTR<IDAComponent> pComp;
	GENDATA->CreateInstance(data.background.staticType, pComp);
	pComp->QueryInterface("IStatic", background);

	GENDATA->CreateInstance(data.title.staticType, pComp);
	pComp->QueryInterface("IStatic", title);

	GENDATA->CreateInstance(data.animSystem.animateType, pComp);
	pComp->QueryInterface("IAnimate", animSystem);

	GENDATA->CreateInstance(data.list.listboxType, pComp);
	pComp->QueryInterface("IListbox", list);
//remove for final release!!!!
	GENDATA->CreateInstance(data.start.buttonType, pComp);
	pComp->QueryInterface("IButton2", start);

	GENDATA->CreateInstance(data.buttonUnlock.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonUnlock);
//
	GENDATA->CreateInstance(data.ok.buttonType, pComp);
	pComp->QueryInterface("IButton2", ok);

	GENDATA->CreateInstance(data.cancel.buttonType, pComp);
	pComp->QueryInterface("IButton2", cancel);

		GENDATA->CreateInstance(data.buttonBack.buttonType, pComp);
	pComp->QueryInterface("IButton2", buttonBack);

	GENDATA->CreateInstance(data.staticMission.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMission);

	GENDATA->CreateInstance(data.staticHolder.staticType, pComp);
	pComp->QueryInterface("IStatic", staticHolder);

	GENDATA->CreateInstance(data.staticMovie.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMovieHolder);

	GENDATA->CreateInstance(data.staticMissionTitle.staticType, pComp);
	pComp->QueryInterface("IStatic", staticMissionTitle);

	U32 i;
	for (i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		GENDATA->CreateInstance(data.buttonMissions[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", buttonMissions[i]);
	}
	// the movie buttons
	for (i = 0; i < NUM_MOVIES_MAX; i++)
	{
		GENDATA->CreateInstance(data.buttonMovies[i].buttonType, pComp);
		pComp->QueryInterface("IButton2", buttonMovies[i]);
	}

	hsndSelection = SFXMANAGER->Open(SFX::BUTTONPRESS2);
	hsndPressed   = SFXMANAGER->Open(SFX::BUTTONPRESS1);

	setStateInfo();
}
//--------------------------------------------------------------------------//
//
void Menu_mission::unlockMissions (void)
{
	U32 i;
	for (i = 0; i < NUM_MISSIONS_MAX; i++)
	{
		buttonMissions[i]->SetVisible(true);
	}
	for (i = 0; i < NUM_MOVIES_MAX; i++)
	{
		buttonMovies[i]->SetVisible(true);
	}

#ifdef SEAN_BETA
	buttonMovies[1]->SetVisible(false);
	buttonMovies[2]->SetVisible(false);
	buttonMovies[3]->SetVisible(false);
#endif
}
//--------------------------------------------------------------------------//
//
void Menu_mission::onButtonPressed (U32 buttonID)
{
	U32 testID = buttonID - MISSION_BUTTON_BEGIN;
	if (testID >= 0 && testID < NUM_MISSIONS_MAX)
	{
		if (bSkipMenu == false)
		{
			SFXMANAGER->Play(hsndPressed);
		}

		// load the proper mission
		wchar_t szMission[64];
	
		// first look for *.dmission then *.qmission
		swprintf(szMission, L"T_Mission%d.dmission", testID+1);
		GAMEPROGRESS->SetMissionsSeen(testID);
		if (list->FindStringExact(szMission) > -1)
		{
			onSelection(szMission);
		}
		else
		{
			swprintf(szMission, L"T_Mission%d.qmission", testID+1);
			onSelection(szMission);
		}
		return;
	}

	switch (buttonID)
	{
	case IDS_BACK:
		endDialog(0);
		break;

	case IDS_NEW_CAMPAIGN:
		onButtonNew();
		endDialog(1);
		break;

	case MOVIE_BUTTON_1:
		// play the movie file between missions 3 and 4
		SetVisible(false);
//		MovieScreen(this, "cq_1.mpg", 640, 272);
		SetVisible(true);
		break;

	case MOVIE_BUTTON_2:
		SetVisible(false);
//		MovieScreen(this, "cq_2.mpg", 640, 272);
		SetVisible(true);
		break;

	case MOVIE_BUTTON_3:
		SetVisible(false);
//		MovieScreen(this, "cq_3.mpg", 640, 272);
		SetVisible(true);
		break;

	case MOVIE_BUTTON_4:
		SetVisible(false);
//		MovieScreen(this, "cq_4.mpg", 640, 272);
		SetVisible(true);
		break;

	case MOVIE_BUTTON_0:
		SetVisible(false);
//		MovieScreen(this, "cq_intro.mpg", 640, 272);
		SetVisible(true);
		break;

	case MOVIE_BUTTON_5:
		SetVisible(false);
//		MovieScreen(this, "cq_4b.mpg", 640, 272);
		SetVisible(true);
		break;

	case BUTTON_MISSION_UNLOCK:
		unlockMissions();
		break;

	case IDS_OK:
		onListSelection(list->GetControlID());
		endDialog(1);
		break;

	case IDS_CANCEL:
		endDialog(0);
		break;
	}
}
//--------------------------------------------------------------------------//
//
U32 __stdcall DoMenu_mission (Frame * parent, const GT_MENU1 & data, bool bSkipMenu)
{
	Menu_mission * dlg = new Menu_mission(parent, data, bSkipMenu);
	dlg->beginModalFocus();
	U32 result = CQDoModal(dlg);
	delete dlg;
	return result;
}

//--------------------------------------------------------------------------//
//-----------------------------End Menu_mission.cpp-----------------------//
//--------------------------------------------------------------------------//