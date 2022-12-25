/***************************************************************************
	FILE: appspecific.H
		Copyright (C) 1996, Microsoft Corp.

	PURPOSE: Defines values specific to the layout and functionality of the main
		window for each launch application

	COMMENTS: This file contains all the app specific constant declarations for
		the setup application.

***************************************************************************/

#ifndef APPSPECIFIC_H
#define APPSPECIFIC_H

const char * const clsname = "DASetup";

const char * INI_FILE_NAME = "conquest.ini";

// these are the requirements for launching the app
char * REQ_DIRECT_X_VERSION = "4.06.00.0000";
char * DIRECT_X_DLL_NAME = "dsetup";
const int REQ_COLORS = 65535;
const int REQ_BIT_DEPTH = 16;
const int REQ_HOR_RES = 800;
const int REQ_VERT_RES = 600;

// if this is true, setup is not considered complete until the app has been launched at least once
const bool APP_MUST_LAUNCH = false;

const bool APP_REQUIRES_BROWSER = false;

const bool APP_IS_DEMO_VERSION = true;

const int SETUP_WINDOW_WIDTH = 640;
const int SETUP_WINDOW_HEIGHT = 480;
int SETUP_WINDOW_STYLE = WS_POPUP;
const int SETUP_INITIAL_BITMAP_ID = BG_BMP;	// id of the bitmap resource to be displayed at start up (not cycled with billboards)
const char * const SETUP_SOUND_FILE = "setup.wav";
const int SETUP_COPYRIGHT_TEXT_COLOR = COLOR_3DSHADOW;
const bool SETUP_HIDE_BUTTONS_DURING_INSTALL = true;
const char * const SETUP_README_FILENAME = "readme.txt";
bool SETUP_BLACKOUT_SCREEN = true;

// sounds to play when a button is selected
char * const DEFAULT_SOUND_FILE = "IDCLICK";
char * const INSTALL_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const UNINSTALL_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const REINSTALL_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const EXIT_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const PLAY_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const WEBLINK_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const CONFIGURE_SOUND_FILE = DEFAULT_SOUND_FILE;
char * const README_SOUND_FILE = DEFAULT_SOUND_FILE;

const int BUTTON_WIDTH = 120;
const int BUTTON_HEIGHT = 30;
// button locations, sizes, and types
BUTTONRECT ButtonList[] =	{ 
							{10,295,BUTTON_WIDTH,BUTTON_HEIGHT,INSTALL},
							{10,295,BUTTON_WIDTH,BUTTON_HEIGHT,PLAY},
							{10,330,BUTTON_WIDTH,BUTTON_HEIGHT,UNINSTALL},
							{10,365,BUTTON_WIDTH,BUTTON_HEIGHT,README},
							{10,400,BUTTON_WIDTH,BUTTON_HEIGHT,CONFIGURE},
							{10,435,BUTTON_WIDTH,BUTTON_HEIGHT,EXIT}
							};

const int SETUP_NUM_BUTTONS = sizeof(ButtonList) / sizeof(BUTTONRECT);
	
// if this is true, the billboards in the list will be displayed for the corresponding duration
const bool USE_BILLBOARDS = true;
// if this is true, the billboards in the list will continuously cycle, otherwise the last bilboard in the list will remain until setup completes
const bool CYCLE_BILLBOARDS = false;

const int BILLBOARD_LIST[] = {IDB_BITMAP1,IDB_BITMAP2,IDB_BITMAP3,IDB_BITMAP1};	// list of id of bitmaps to display
const int NUM_BILLBOARDS = sizeof(BILLBOARD_LIST) / sizeof(int);

const int BILLBOARD_DURATION = 20000; // msecs per billboard

#endif // APPSPECIFIC_H