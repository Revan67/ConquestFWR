#ifndef DPROGRESSSTATIC_H
#define DPROGRESSSTATIC_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DProgressStatic.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DProgressStatic.h 4     10/05/00 3:23p Jasony $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

//--------------------------------------------------------------------------//
//
namespace PRG_STTXT
{
enum PROGRESS_STATIC_TEXT
{

  NO_STATIC_TEXT=0,
  TEST = IDS_TEST_BUTTON_TEXT,
  PLAYER_NAME = IDS_ENTER_PLAYER_NAME,
  IP_ADDRESS = IDS_ENTER_IP_ADDRESS,
  SESSION_NAME = IDS_ENTER_SESSION_NAME,
  CONFIRM_QUIT = IDS_CONFIRM_QUIT,
  TITLE_SELECT_GAME = IDS_TITLE_SELECTGAME,
  PLAYER = IDS_STATIC_PLAYER,
  SESSION = IDS_STATIC_SESSION,
  ENTER_TEAM = IDS_ENTER_TEAM,
  ENTER_RACE = IDS_ENTER_RACE,
  ENTER_COLOR = IDS_ENTER_COLOR,
  SELECTED_MAP = IDS_SELECTED_MAP,
  CHATBOX = IDS_STATIC_CHATBOX,
  GAME_PAUSED = IDS_GAMEPAUSED,
  INGAME_TITLE = IDS_IGOPTIONS_TITLE,
  OPTIONS = IDS_STATIC_OPTIONS,
  GAMETYPE = IDS_STATIC_GAMETYPE,
  GAMESPEED = IDS_STATIC_GAMESPEED,
  CURSOR_SENSITIVITY = IDS_CURSOR_SENSITIVITY,
  ARTIFACTS = IDS_STATIC_ARTIFACTS,
  MONEY = IDS_STATIC_MONEY,
  RESEARCH = IDS_STATIC_RESEARCH,
  MAPTYPE = IDS_STATIC_MAPTYPE, 
  MAPFILE = IDS_STATIC_MAPFILE,
  MAPSIZE = IDS_STATIC_MAPSIZE,
  TERRAIN = IDS_STATIC_TERRAIN,
  STATIC_RESOURCES = IDS_STATIC_RESOURCES,
  WARNBOOT = IDS_HELP_WARNBOOT,
  SOLOGAME = IDS_HELP_SOLOGAME,
  LOADGAME = IDS_STATIC_LOADGAME,
  SAVEGAME = IDS_STATIC_SAVEGAME,
  FILENAME = IDS_STATIC_FILENAME,
  FLEET_SIZE = IDS_FLEET_SIZE,
  SKILL = IDS_FLEET_SKILL,
  BONUS = IDS_FLEET_BONUS,
  DUMMY_MONEY = IDS_DUMMY_MONEY,
  DUMMY_NAME = IDS_DUMMY_NAME,
  SHIPCLASS = IDS_IND_SHIPCLASS,
  HULL = IDS_IND_HULL,
  SUPPLIES = IDS_IND_SUPPLIES,
  ARMOR = IDS_IND_ARMOR,
  ENGINES = IDS_IND_ENGINES,
  WEAPONS = IDS_IND_WEAPONS,
  SHIELDS = IDS_IND_SHIELDS,
  KILLS = IDS_IND_KILLS,
  UNITS = IDS_STATIC_UNITS,
  INGAME_CHAT = IDS_REQUEST_CHAT,
  MISSION_OBJECTIVES = IDS_MISSION_OBJECTIVES,
  END_MISSION = IDS_END_MISSION,
  SCOREBOARD = IDS_SCOREBOARD,
  SCORE = IDS_SCORE,
  VOLUME = IDS_VOLUME_CONTROL,
  MUSIC = IDS_MUSIC,
  EFFECTS = IDS_EFFECTS,
  COMMS = IDS_COMMS,
  NETCHAT = IDS_NETCHAT,
  TOOLTIPS = IDS_TOOLTIPS,
  STATUS_TEXT = IDS_STATUSTEXT,
  FOGOFWAR = IDS_FOG,
  MAPROTATE = IDS_MAPROTATE,
  ROLLOVER = IDS_ROLLOVER,
  SLOW = IDS_GAMESPEED_SLOW,
  FAST = IDS_GAMESPEED_FAST,
  GAMMA = IDS_GAMMA,
  WEAPON_TRAILS = IDS_WEAPON_TRAILS,
  EMMISIVE_TEXTURES = IDS_EMMISIVE_TEXTURES,
  LOD = IDS_LOD,
  TEXTURES = IDS_USE_TEXTURES,
  BACKGROUND = IDS_DRAW_BACKGROUND,
  DEVICES = IDS_HARDWARE_DEVICES,
  ENTER_NAME = IDS_ENTER_NAME,
  CAMP_SELECT = IDS_SELECT_CAMPAIGN,
  SINGLE_PLAYER = IDS_SINGLE_PLAYER,
  MULTI_PLAYER = IDS_MULTI_PLAYER,
  FRONT_OPTIONS = IDS_OPTIONS,
  SKIRMISH = IDS_STATIC_SKIRMISH,
  INTRO = IDS_STATIC_INTRO,
  SELECT_MISSION = IDS_SELECT_MISSION,
  RIGHTCLICK = IDS_RIGHTCLICK,
  JOIN = IDS_JOIN,
  CREATE = IDS_CREATE,
  ACCEPT = IDS_ACCEPT,
  SCROLLRATE = IDS_SCROLL_RATE,
  QUIT = IDS_QUIT,
  RESOLUTION = IDS_RESOLUTION,
  ENTER_STATE = IDS_ENTER_STATE,
  PLAYER_LIST = IDS_PLAYER_LIST,
  PING = IDS_PING,
  APP_NAME = IDS_APP_NAME,
  VERSION = IDS_STATIC_VERSION,
  HELP = IDS_STATIC_HELP,
  NET_CONNECT = IDS_NET_CONNECT,
  HARDWARE = IDS_HARDWARE,
  GAME = IDS_STATIC_GAME,
  SPEED = IDS_SPEED,
  MAP = IDS_STATIC_MAP,
  RESOURCES = IDS_RESOURCES,
  TITLE_UNITS = IDS_TITLE_UNITS,
  TITLE_BUILDINGS = IDS_TITLE_BUILDINGS,
  TITLE_RESOURCES = IDS_TITLE_RESOURCES,
  TITLE_TERRITORY = IDS_TITLE_TERRITORY,
  BRIEFING = IDS_BRIEFING,
  VISIBILITY = IDS_STATIC_VISIBILITY,
  DEVICE_CHOICE = IDS_DEVICE_CHOICE,
  SOFTWARE = IDS_SOFTWARE
};

} // end namespace
//--------------------------------------------------------------------------//
//  Definition file for static controls
//--------------------------------------------------------------------------//

struct GT_PROGRESS_STATIC : GENBASE_DATA
{
	char fontName[GT_PATH];
	struct COLOR
	{
		unsigned char red, green, blue;
	} normalText, overText, background, background2;
    char shapeFile[GT_PATH];                        // vfx shape file

	enum drawtype
	{
		NODRAW,
		FILL,
		HASH
	} backgroundDraw;

	bool bBackdraw;
};
//---------------------------------------------------------------------------
//
struct PROGRESS_STATIC_DATA
{
    char staticType[GT_PATH];
	PRG_STTXT::PROGRESS_STATIC_TEXT staticText;         // can also be used for ID
	enum alignmenttype
	{
		LEFT,
		CENTER,
		RIGHT,
		TOPLEFT,
	} alignment;
    S32 xOrigin, yOrigin;
	S32 width, height;				// if non-zero, override shape bounds
};

#endif
