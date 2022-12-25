//--------------------------------------------------------------------------//
//                                                                          //
//                           PlayerMenu.cpp                                 //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/PlayerMenu.cpp 23    9/25/00 9:05p Jasony $
*/
//--------------------------------------------------------------------------//
 
#include "pch.h"
#include <globals.h>
#include <MGlobals.h>
#include <DPlayerMenu.h>

#include "Frame.h"
#include "IButton2.h"
#include "IStatic.h"
#include "IListbox.h"
#include "DrawAgent.h"
#include "Camera.h"

#include "Mission.h"
#include "Hotkeys.h"
#include "CQGame.h"

#include <stdio.h>

#include <da_vector>

using namespace da_std;
using namespace CQGAMETYPES;

#define UPDATE_SCORE_TIMER 1000
#define BOX_LENGTH  50
//--------------------------------------------------------------------------//
//
struct PlayerMenu : public DAComponent<Frame>
{
	typedef vector<U32> U32VECTOR;

	//
	// data items
	//

	U32 updateTimer;
	U32VECTOR uvector;

	HFONT hFont, hBoldFont;
	U32 fontHeight;
	COMPTR<IFontDrawAgent> font[MAX_PLAYERS];
	COMPTR<IFontDrawAgent> shadowFont[MAX_PLAYERS];
	bool bBold[MAX_PLAYERS];
	S32  scoreDivisor;

	//
	// instance methods
	//

	PlayerMenu (void)
	{
		eventPriority = EVENT_PRIORITY_IG_OPTIONS;
		parent->SetCallbackPriority(this, eventPriority);
		initializeFrame(NULL);
		init();
	}

	~PlayerMenu (void);

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

	
	/* PlayerMenu methods */

	virtual bool onTabPressed (void)
	{
		return true;
	}

	virtual void onUpdate(U32 dt)
	{
		updateTimer += dt;

		if (updateTimer > UPDATE_SCORE_TIMER)
		{
			resetScore();
			updateTimer = 0;
		}
	}

	virtual void onButtonPressed (U32 buttonID)
	{
	}

	virtual void onDraw (void);

	void init (void);

	void resetScore (void);

	void reloadFonts (bool bLoad);
};
//----------------------------------------------------------------------------------//
//
PlayerMenu::~PlayerMenu (void)
{
	if (hFont)
	{
		DeleteObject(hFont);
		hFont = 0;
	}
	if (hBoldFont)
	{
		DeleteObject(hBoldFont);
		hBoldFont = 0;
	}
}
//----------------------------------------------------------------------------------//
//
GENRESULT PlayerMenu::Notify (U32 message, void *param)
{

	if ((message > WM_MOUSEFIRST) && (message <= WM_MOUSELAST))
	{
		return GR_OK;
	}
	
	if (message == CQE_START3DMODE)
		reloadFonts(true);
	if (message == CQE_END3DMODE)
		reloadFonts(false);

	return Frame::Notify(message, param);
}
//--------------------------------------------------------------------------//
//
void PlayerMenu::resetScore()
{
	// fill the listbox with data, order the players on how "well" they are doing and add them to the box
	// with appropriate color and score
	U32 i;
	S32 score1, score2;
	const CQGAME & cqgame = MGlobals::GetGameSettings();

	uvector.clear();

	// order the all the player's slotID's by their score
	for (i = 0; i < cqgame.activeSlots; i++)
	{
		U32 playerID;

		if (cqgame.slot[i].state == READY)
		{
			playerID = MGlobals::GetPlayerIDFromSlot(i);

			// add it to the list
			score1 = MGlobals::GetPlayerScore(playerID);

			bool bAdded = false;
			if (uvector.size() == 0)
			{
				uvector.push_back(i);
			}
			else
			{
				U32VECTOR::iterator it;
				for (it = uvector.begin(); it != uvector.end(); it++)
				{
					U32 id2 = MGlobals::GetPlayerIDFromSlot(*it);
					score2 = MGlobals::GetPlayerScore(id2);
					
					if (score1 > score2)
					{
						uvector.insert(it, i);
						bAdded = true;
						break;
					}
				}
				if (!bAdded)
				{
					uvector.push_back(i);
				}
			}
		}
	}

	if (uvector.size())
	{
		S32 bestScore = MGlobals::GetPlayerScore(MGlobals::GetPlayerIDFromSlot(uvector[0]));
		
		if (bestScore <= 64)
			scoreDivisor = 6;
		else
		if (bestScore <= 128)
			scoreDivisor = 7;
		else
			scoreDivisor = 8;
	}
}
//--------------------------------------------------------------------------//
//
void PlayerMenu::onDraw (void)
{
	U32 i;
	wchar_t playerName[32];
	const CQGAME & cqgame = MGlobals::GetGameSettings();
	const U32 height = fontHeight;
	S32 yoffset = 0;
	S32 tbHeight = SCREENRESY - (CAMERA->GetPane()->y1 + 1);

	// now add the data to the list box in order
	for (i = 0; i < uvector.size(); i++)
	{
		U32 slotID = uvector[i];
		U32 playID = MGlobals::GetPlayerIDFromSlot(slotID);
		COLOR colorID;
		
		// the uvector is a vector containing slot ID's
		playerName[0] = 0;
		MGlobals::GetPlayerNameBySlot(slotID, playerName, sizeof(playerName));
		
		// if there was no name, then the player must be a computer
		if (playerName[0] == 0 && cqgame.slot[slotID].type == COMPUTER)
		{
			wcsncpy(playerName, _localLoadStringW(IDS_COMPUTER_NAME), 32);
		}

		// has the player resigned?
		if (MGlobals::HasPlayerResigned(slotID))
		{
			colorID = UNDEFINEDCOLOR;
		}
		else
		{
			colorID = COLOR(MGlobals::GetColorID(playID));
		}

		CQASSERT(i < MAX_PLAYERS);

		if (MISSION->IsComputerControlled(playID))
		{
			if (bBold[i])		// switch to regular font
			{
				bBold[i] = false;
				CreateFontDrawAgent(hFont, 0, RGB(0,0,0), RGB(0,0,0), font[i]);
				font[i]->CreateDuplicate(shadowFont[i]);
			}
		}
		else
		{
			if (bBold[i]==0)		// switch to bold font
			{
				bBold[i] = true;
				CreateFontDrawAgent(hBoldFont, 0, RGB(0,0,0), RGB(0,0,0), font[i]);
				font[i]->CreateDuplicate(shadowFont[i]);
			}
		}
		
		font[i]->SetFontColor(COLORTABLE[colorID]|0xFF000000, 0);
		shadowFont[i]->SetFontColor(RGB(0,0,0)|0xFF000000, 0);

		shadowFont[i]->StringDraw(0, screenRect.left+IDEAL2REALX(1), screenRect.top+yoffset+IDEAL2REALY(1)-tbHeight, playerName);
		font[i]->StringDraw(0, screenRect.left, screenRect.top+yoffset-tbHeight, playerName);

		DA::RectangleHash(0, screenRect.left-(BOX_LENGTH+4), screenRect.top+yoffset+1-tbHeight, 
							 screenRect.left-4, screenRect.top+yoffset+height-1-tbHeight, RGB(60,60,60)|0xFF000000);

		S32 score = ((MGlobals::GetPlayerScore(playID) * BOX_LENGTH) >> scoreDivisor);

		DA::RectangleFill(0, screenRect.left-(BOX_LENGTH+4), screenRect.top+yoffset+1-tbHeight, 
							 screenRect.left-(BOX_LENGTH+4)+score, screenRect.top+yoffset+height-1-tbHeight, RGB(60,60,60)|0xFF000000);


		yoffset += height;
	}
}
//--------------------------------------------------------------------------//
//
void PlayerMenu::reloadFonts (bool bLoad)
{
	if (bLoad)
	{
		COLORREF pen, background;
		int i;

		pen			= RGB_GOLD		   | 0xFF000000;		// white	(pen color)
		background	= RGB(0,0,0);	   //| 0xFF000000;		// black	(background color)

		CreateFontDrawAgent(hBoldFont, 0, pen, background, font[0]);
		font[0]->CreateDuplicate(shadowFont[0]);
		bBold[0] = true;
		fontHeight = font[0]->GetFontHeight();

		for (i = 1; i < MAX_PLAYERS; i++)
		{
			font[0]->CreateDuplicate(font[i]);
			font[0]->CreateDuplicate(shadowFont[i]);
			bBold[i] = true;
		}
	}
	else
	{
		int i;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			font[i].free();
			shadowFont[i].free();
		}
	}
}
//--------------------------------------------------------------------------//
//
void PlayerMenu::init (void)
{
	hFont = CQCreateFont(IDS_SCOREBOARD_FONT, true);
	hBoldFont = CQCreateFont(IDS_SCOREBOARD_BOLD_FONT, true);

	//
	// create members if not done already
	//
	screenRect.left		= IDEAL2REALX(575);
	screenRect.right	= IDEAL2REALX(640);
	screenRect.top		= IDEAL2REALY(200+150);		// add 150 for toolbar
	screenRect.bottom	= IDEAL2REALY(350);

	reloadFonts(true);
	resetScore();

	// addition amount we can move down accounting for extra screen res
	S32 extra = IDEAL2REALY(fontHeight*8) - fontHeight*8;		
	screenRect.top += extra;

	// addition amount we can move down accounting less than 8 players
	extra = (MAX_PLAYERS - uvector.size()) * fontHeight;
	screenRect.top += extra;

	if (childFrame)
		childFrame->setStateInfo();
}
//--------------------------------------------------------------------------//
//
Frame* __stdcall CreatePlayerMenu (void)
{
	PlayerMenu * menu = new PlayerMenu();

	return menu;
}

//--------------------------------------------------------------------------//
//-----------------------------End PlayerMenu.cpp-----------------------//
//--------------------------------------------------------------------------//
