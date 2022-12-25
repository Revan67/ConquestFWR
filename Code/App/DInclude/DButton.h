#ifndef DBUTTON_H
#define DBUTTON_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               DButton.h       							//
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/DInclude/DButton.h 56    9/13/01 10:02a Tmauer $
*/			    
//--------------------------------------------------------------------------//

#ifndef DBASEDATA_H
#include "DBaseData.h"
#endif

#ifndef GT_PATH
#define GT_PATH 32
#endif

#undef IGNORE 
#undef YES
#undef NO

namespace BTNTXT
{
enum BUTTON_TEXT
{
	NOTEXT				= 0,
	TEST				= IDS_TEST_BUTTON_TEXT,
	SINGLE_PLAYER		= IDS_SINGLE_PLAYER,
	MULTI_PLAYER		= IDS_MULTI_PLAYER,
	OPTIONS				= IDS_OPTIONS,
	BACK				= IDS_BACK,
	JOIN				= IDS_JOIN,
	CREATE				= IDS_CREATE,
	NEXT				= IDS_NEXT,
	QUIT				= IDS_QUIT,
	CANCEL				= IDS_CANCEL,
	YES					= IDS_YES,
	NO					= IDS_NO,
	FINAL_MENU			= IDS_MPLAYER_FINAL,
	ACCEPT				= IDS_ACCEPT,
	START				= IDS_START,
	RESUME				= IDS_RESUME,
	OK					= IDS_OK,
	SETTINGS			= IDS_MPLAYER_SETTINGS,
	OPEN				= IDS_OPEN,
	SAVE				= IDS_SAVE,
	CONTINUE			= IDS_CONTINUE,
	RANDOM_MAP			= IDS_MAPTYPE_RANDOM,
	LOAD				= IDS_LOAD,
	GAME_OPTIONS		= IDS_OPTIONS,
	MISSION_OBJECTIVES	= IDS_MISSION_OBJECTIVES,
	END_MISSION			= IDS_END_MISSION,
	RESTART				= IDS_RESTART,
	RESIGN				= IDS_RESIGN,
	ABDICATE			= IDS_ABDICATE,
	RETURN_TO_GAME		= IDS_RETURN_TO_GAME,
	GAME				= IDS_GAME,
	SOUND				= IDS_SOUND,
	GRAPHICS			= IDS_GRAPHICS,
	NEW_PLAYER			= IDS_NEW_PLAYER,
	DELETE_PLAYER		= IDS_DELETE_PLAYER,
	LOAD_SAVED			= IDS_LOAD_SAVED_GAME,
	LOAD_CUSTOM			= IDS_LOAD_CUSTOM,
	TERRAN				= IDS_BUTTON_TERRAN,
	MANTIS				= IDS_BUTTON_MANTIS,
	NEW_CAMPAIGN		= IDS_NEW_CAMPAIGN,
	REPLAY				= IDS_REPLAY,
	INTRODUCTION		= IDS_INTRODUCTION,
	SKIRMISH			= IDS_SKIRMISH,
	REPAIR_FLEET		= IDS_REPAIR_FLEET,
	RESUPPLY_FLEET		= IDS_RESUPPLY_FLEET,
	DISBAND_FLEET		= IDS_DISBAND_FLEET,
	CREATE_FLEET		= IDS_CREATE_FLEET,
	TRANSFER_FLAGSHIP	= IDS_TRANSFER_FLAGSHIP,
	RESET				= IDS_RESET,
	CLOSE				= IDS_CLOSE,
	ALLIES				= IDS_ALLIES,
	ENEMIES				= IDS_ENEMIES,
	EVERYONE			= IDS_EVERYONE,
	CHANGE_NAME			= IDS_CHANGE_NAME,
	CAMPAIGN			= IDS_SELECT_CAMPAIGN,
	TERRAN_CAMP			= IDS_TERRAN_CAMPAIGN,
	MANTIS_TRAINING		= IDS_MANTIS_TRAINING,
	SOLARIAN_TRAINING	= IDS_SOLARIAN_TRAINING,
	ZONEGAME			= IDS_ZONEGAME,
	WEBPAGE				= IDS_WEB_BUTTON,
	HELP				= IDS_STATIC_HELP,
	MAP_TYPE			= IDS_BUTTON_MAPTYPE,
	APPLY				= IDS_APPLY,
	LOAD_QUICKBATTLE	= IDS_LOAD_QUICKBATTLE,
	CREDITS				= IDS_CREDITS,
	DELETE_FILE			= IDS_DELETE
};
} // end namespace
//---------------------------------------------------------------------------
// order of the images in the button shape file
//---------------------------------------------------------------------------
#define GTBSHP_DISABLED			0
#define GTBSHP_NORMAL			1
#define GTBSHP_MOUSE_FOCUS		2			
#define GTBSHP_DEPRESSED		3
#define GTBSHP_KEYB_FOCUS		4			// overlay

#define GTBSHP_MAX_SHAPES		5
//---------------------------------------------------------------------------
// text color states
//---------------------------------------------------------------------------
#define GTBTXT_DISABLED			0
#define GTBTXT_NORMAL			1
#define GTBTXT_HIGHLIGHT		2
#define GTBTXT_KEYFOCUS			3

#define GTBTXT_MAX_STATES		4

//---------------------------------------------------------------------------
//
struct GT_BUTTON : GENBASE_DATA
{
	enum BUTTON_TYPE
	{
		DEFAULT,
		PUSHPIN,			// button stays in "pressed" state until pressed again
		CHECKBOX,			// same as PUSHPIN, except "pressed" state is not 3D (text does not move)
		REPEATER,			// while button is down will keep repeating the "pressed" state
	} buttonType:8;

	S32 leftMargin:8;		// skip pixels on the edge (for dropdown arrows)
	S32 rightMargin:8;

	char fontName[GT_PATH];
	struct COLOR
	{
		unsigned char red, green, blue;
	} disabledText, normalText, highlightText;

	char shapeFile[GT_PATH];			// vfx shape file
};
//---------------------------------------------------------------------------
//
struct BUTTON_DATA
{
	char buttonType[GT_PATH];
	BTNTXT::BUTTON_TEXT buttonText;		// can also be used for ID
	S32 xOrigin, yOrigin;
	RECT buttonArea;					// for when we don't have a shape file
};


#endif
