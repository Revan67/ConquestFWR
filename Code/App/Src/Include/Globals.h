#ifndef GLOBALS_H
#define GLOBALS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Globals.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/Include/Globals.h 375   7/12/02 3:21p Tmauer $
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//----------------------------GLOBAL #defines-------------------------------//
//--------------------------------------------------------------------------//

#include <TManager.h>

#undef CQEXTERN
#ifdef BUILD_GLOBALS
#define CQEXTERN __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#endif

#define SCREEN_HRIDEAL_WIDTH 1024
#define SCREEN_HRIDEAL_HEIGHT 768

enum InterfaceRes
{
	IR_NO_RESOLUTION = 0,
	IR_IN_GAME_RESOLUTION,
	IR_FRONT_END_RESOLUTION,
};

#define MAX_ORTHO_DEPTH 1
#define MAX_PLAYERS     8
#define MAX_SELECTED_UNITS 22		
#define MAX_NUGGETS		3			// max nuggets created after a ship explodes
#define HYPER_SYSTEM_MASK 0x80
#define FAB_MAX_QUEUE_SIZE 15		// also defined in DFabSave.h
#define MAX_SYSTEMS 16				// also defined in Sector.h


#define BASE_MAX_CREW 250
#define BASE_MAX_METAL 220
#define BASE_MAX_GAS 180

#define SYSVOLATILEPTR (MAX_PLAYERS+1)		// for OBJPTR
#define NONSYSVOLATILEPTR 0					// for OBJPTR
#define TOTALLYVOLATILEPTR (0xFFFFFFFF)		// for OBJPTR
#define LAUNCHVOLATILEPTR (0xFFFFFFFE)		// for OBJPTR

// explosion instance limiting
#define LOWER_EXP_BOUND     5			// off screen|same system, or same player
#define UPPER_EXP_BOUND		10			// on screen


#define GAS_MULTIPLIER 25
#define METAL_MULTIPLIER 25
#define CREW_MULTIPLIER 10

// result codes for mission:
#define MISSION_END_RESIGN		1
#define MISSION_END_WON			2
#define MISSION_END_LOST		3
#define MISSION_END_QUIT		4
#define MISSION_END_SPLASH      5

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported by the OS 
#endif

#define DEF_ELAPSED_TIME  (8.0F / 30.0F)		// 26.6 msecs per update
#define DEF_REALTIME_FRAMERATE (30.0F / 8)		// 3.75 world updates per second (independent of actual renders)
#define RENDER_FRAMERATE 30.0F				// 30 fps
#define RENDER_PERIOD (1.0F / 30.0F)
#define RGB_LOCAL  RGB(255,255,255)
#define RGB_SHADOW RGB(111,218,249)
#define RGB_GOLD RGB(216,201,20) 
#define RGB_NTEXT RGB(238,156,0)

#define REAL2IDEALX(x) ((((x) * SCREEN_WIDTH) / (S32)SCREENRESX))
#define REAL2IDEALY(y) ((((y) * SCREEN_HEIGHT) / (S32)SCREENRESY))

#define IDEAL2REALX(x) ((((x) * ((S32)SCREENRESX) / SCREEN_WIDTH)))
#define IDEAL2REALY(y) ((((y) * ((S32)SCREENRESY) / SCREEN_HEIGHT)))

#define HRIDEAL2IDEALX(x) (CQFLAGS.b3DEnabled ? (((x) * ((S32)SCREEN_WIDTH) / SCREEN_HRIDEAL_WIDTH)) : x)
#define HRIDEAL2IDEALY(y) (CQFLAGS.b3DEnabled ? (((y) * ((S32)SCREEN_HEIGHT) / SCREEN_HRIDEAL_HEIGHT)) : y)
#define HRIDEAL2REALX(x) ((((x) * ((S32)SCREENRESX) / SCREEN_HRIDEAL_WIDTH)))
#define HRIDEAL2REALY(y) ((((y) * ((S32)SCREENRESY) / SCREEN_HRIDEAL_HEIGHT)))
#define IDEAL2HRIDEALX(x) ((((x) * SCREEN_HRIDEAL_WIDTH) / (S32)SCREEN_WIDTH))
#define IDEAL2HRIDEALY(y) ((((y) * SCREEN_HRIDEAL_HEIGHT) / (S32)SCREEN_HEIGHT))

#define DIRECTINPUT_VERSION 0x0700

//--------------------------------------------------------------------------//
//-------------------GLOBAL message #defines for Event System---------------//
//--------------------------------------------------------------------------//

#define CQE_NEW_SELECTION		0x1000			// parm = NULL, or IDAComponent that was pressed
#define CQE_ENDFRAME			0x1001
#define CQE_PANEL_OWNED			0x1002
#define CQE_HOTKEY				0x1003
#define CQE_JOYSTICK			0x1004
#define CQE_KILL_FOCUS          0x1005			// parm = IDAComponent that sent the message
#define CQE_SET_FOCUS			0x1006			// parm = IDAComponent that sent the message
#define CQE_BUTTON				0x1007			// parm = ID of button that was pressed
#define CQE_UNALERT				0x1008			// parm = IDAComponent that sent the message
#define CQE_NETSTARTUP			0x1009			// network session has started
#define CQE_NETSHUTDOWN			0x100A			// network session is about to shutdown
#define CQE_NEWHOST				0x100B			// parm = ID of old host HOSTID is new host
#define CQE_ADDPLAYER			0x100C			// parm = ID of new player
#define CQE_DELETEPLAYER		0x100D			// parm = ID of player removed from session
#define CQE_NETPACKET			0x100E			// parm = BASE_PACKET *. (Check the actual type)
#define CQE_CAMERA_MOVED		0x100F		    // camera position or orientation changed
#define CQE_UPDATE				0x1010			// parm = GR->last_frame_time(). called once an update loop
#define CQE_VIEW_OBJECT			0x1011			// used by ObjectList (internal)
//#define CQE_ENABLE3DMODE		0x1012			// parm = non-zero = game is now in 3D mode
#define CQE_EDITOR_MODE			0x1013			// parm = (BOOL32) new value of bEditorMode
#define CQE_PANEL_REFRESH		0x1014
#define CQE_ENABLE_BUTTON		0x1015
#define CQE_STREAMER			0x1016			// parm = (MSG *) where lParam =  HSTREAM, wParam = IStreamer::state
#define CQE_NETADDPLAYER		0x1017			// parm = ID of new player, (should only be used by NetPacket)
#define CQE_SYSTEM_CHANGED		0x1018			// parm = (U32) new system ID
#define CQE_UNICHAR				0x1019			// parm = (U16) unicode key pressed
#define CQE_KEYBOARD_FOCUS		0x101A			// parm = (IDAComponent *) of control needing keyboard focus
#define CQE_NETPAUSED           0x101B          // parm = non-zero = game paused by remote host, 0==unpaused
#define CQE_LOCALPAUSED         0x101C          // parm = non-zero = game is paused by local host 0 == unpaused
#define CQE_LOAD_INTERFACE		0x101D			// parm = (IShapeLoader *) if loading, NULL if unloading
#define CQE_DELETE_HOTRECT		0x101E			// parm = (BaseHotRect *) of instance needing a kill'n
#define CQE_RUSE_HOTKEY			0x101F
#define CQE_DLG_RESULT			0x1020			// parm = (U32) exit code of a dialog
#define CQE_LIST_SELECTION		0x1021			// parm = (S32) ID of listbox control 
#define CQE_LIST_CARET_MOVED	0x1022			// parm = (S32) ID of listbox control 
#define CQE_DPLAY_MSGWAITING	0x1023			// parm = (MSG *) (wParam=elapsed msec or lParam defined yet)
#define CQE_PART_SELECTION		0x1024			// parm = (U32) user defined data item, part has been selected from menu
#define CQE_MISSION_ENDING		0x1025			// parm = (U32) result code
#define CQE_DEBUG_HOTKEY		0x1026
#define CQE_LHOTBUTTON			0x1027			// parm = (U32) ID of hotButton pressed with left mouse button
#define CQE_RHOTBUTTON			0x1028			// parm = (U32) ID of hotButton pressed with right mouse button
#define CQE_LDBLHOTBUTTON		0x1029			// parm = (U32) ID of hotbutton pressed with left double-click
#define CQE_LOAD_TOOLBAR		0x102A			// parm = (const M_RACE * pRace) if loading, NULL if unloading
#define CQE_GAME_ACTIVE			0x102B			// parm = (BOOL32) new bGameActive value
#define CQE_MOVE_SELECTED_UNITS	0x102C			// parm = (struct NETGRIDVECTOR *) user wants to move the selected items
#define CQE_DATA_CHECKSUM		0x102D			// WPARAM 0 = objlist, gendata, conquest, mission
#define CQE_HOST_MIGRATE		0x102F			// sent when migration complete
#define CQE_GOTO_PART			0x1030			// parm = (U32) dwMissionID of object to move camera to
#define CQE_SELECTION_EVENT		0x1031			// parm = 0, sent when a unit is added to the selected list
#define CQE_EDIT_CHANGED		0x1032			// parm = (U32) id of control sending the message
#define CQE_SET_RALLY_POINT		0x1033			// parm = (struct NETGRIDVECTOR *) location of rally point
#define CQE_DELETE_CHAT_MENU    0x1034			
#define CQE_DELETE_PLAYER_MENU  0x1035			
#define CQE_QMOVE_SELECTED_UNITS 0x1036			// parm = (struct NETGRIDVECTOR *) user wants to move the selected items, queued
#define CQE_SLIDER				0x1037			// parm = ID of slider that was slid
#define CQE_STREAMER2			0x1038			// parm = (MSG *) where lParam = (DSStream *)
#define CQE_IDLE_UNIT			0x1039			// parm = (U32) dwMissionID of idle unit
#define CQE_MISSION_CLOSE		0x103A			// parm = 0, mission is about to close, parm = 1, mission has closed
#define CQE_PAUSE_CHANGE		0x103B			// parm = new pause state,  The pause state has changed.
#define CQE_PAUSE_WARNING		0x103C			// parm = 0, sent when host has warned us that pause time is running out
#define CQE_GET_MULTIPLAYER_VER 0x103D			// parm = U32 *, address of a U32 where result will be stored

#define CQE_MISSION_LOAD_COMPLETE 0x103E
#define CQE_BUILDQUEUE_REMOVE	0x103F			// parm = index of item to remove;		
#define CQE_CHATDLG_SETYPOS		0x1040			// parm = ypos of chat dialog (moves in realtime)
#define CQE_ADMIRALBAR_ACTIVE   0x1041			// parm = (int) y position of bottom edge of admiral bar
#define CQE_NETDELETEPLAYER		0x1042			// parm = ID of deleted player, (should only be used by NetPacket)
#define CQE_RENDER_LAST3D		0x1043			// parm = GR->last_frame_time(). called once an update loop?

#define CQE_FLUSHMESSAGES		0x1044			//

#define CQE_DELETE_DIPLOMACY_MENU	0x1045
#define CQE_DELETE_CHATFILTER_MENU	0x1046
#define CQE_DELETE_OBJECTIVES_MENU  0x1047

#define CQE_OBJECT_DESTROYED	0x1048			// parm = dwMissionID of object being destroyed (object is still alive at this point)
#define CQE_UNIT_UNDER_ATTACK	0x1049			// parm = dwMissionID of object being attacked

#define CQE_UI_RALLYPOINT		0x104A			// parm = 0,1 for new rally state
#define CQE_UI_DEFDELANIM		0x104B			// parm = ANIMQUEUELIST (used by objcomm.cpp)
#define CQE_OBJECT_PRECAPTURE	0x104C			// parm = dwMissionID of object being captured (object is still alive at this point)
#define CQE_OBJECT_POSTCAPTURE	0x104D			// parm = dwMissionID of object that has been captured.
#define CQE_NETPACKET_KEEPALIVE 0x104E			// parm = (MSG *), where wParam = tickCount of when last message was sent

#define CQE_MISSION_PROG_ENDING	0x1050			// parm = (U32) winning team flags, Sent by MScript when mission won/lost
#define CQE_FABRICATOR_FINISHED 0x1051			// parm = missionID of fabricator
#define CQE_MOVIE_MODE			0x1052			// parm = bool - mode on or off

#define CQE_OBJECTIVES_CHANGE   0x1053			// tell the objective list to update itself
#define CQE_OBJECTIVE_ADDED     0x1054			// a new objective has been added to our list of objectives!!
#define CQE_TABCONTROL_CHANGE   0x1055			// parm = (int) index of tab we've switched to

#define CQE_PLAYER_RESIGN		0x1056			// the player wishes to resign
#define CQE_DIPLOMACYBUTTON		0x1057			// parm = ID of button that was pressed

#define CQE_GET_MAX_PLAYERS		0x1058			// ask the map menu for the max players allowed.
#define CQE_PLAYER_QUIT			0x1059			// the player wishes to abdicate

#define CQE_SQMOVE_SELECTED_UNITS 0x105a		// parm = (struct NETGRIDVECTOR *) user wants to move the selected items, queued and slow
#define CQE_SMOVE_SELECTED_UNITS 0x105b			// parm = (struct NETGRIDVECTOR *) user wants to move the selected items, slow

#define CQE_RESET_NETWORK_PERFORMANCE 0x105c	// used by menu_map to rest the network performance in NetBuffer

#define CQE_MISSION_ENDING_SPLASH 0x105d        // parm = (struct SPLASHINFO *) user defined splash menu info

#define CQE_INTERFACE_RES_CHANGE 0x105e			// sent when the interface resolution has changed
#define CQE_START3DMODE			0x105f			//sent when 3d mode is started
#define CQE_END3DMODE			0x1060			//sent when 3d mode is finished
#define CQE_LEAVING_INGAMEMODE	0x1061			//sent when 3d mode is started
#define CQE_ENTERING_INGAMEMODE	0x1062			//sent when 3d mode is started
#define CQE_LEAVING_FRONTENDMODE 0x1063			//sent when 3d mode is finished
#define CQE_ENTERING_FRONTENDMODE 0x1064		//sent when 3d mode is finished


//--------------------------------------------------------------------------//
//----------------------------GLOBAL data items-----------------------------//
//--------------------------------------------------------------------------//

class Vector;
class Matrix;
class TRANSFORM;
struct ARCHNODE;
struct BaseHotRect;
typedef ARCHNODE *PARCHETYPE;
typedef S32 INSTANCE_INDEX;
struct GENNODE;
typedef GENNODE *PGENTYPE;
struct ILaunchOwner;
//typedef DWORD HIMC;

CQEXTERN HINSTANCE hResource;		// handle to module containing resources
CQEXTERN HWND hMainWindow;
CQEXTERN HDC hMainDC;
CQEXTERN DWORD PLAYERID;
CQEXTERN DWORD HOSTID;
CQEXTERN DWORD TRACELEVEL;
CQEXTERN DWORD SCREENRESX;			// actual values
CQEXTERN DWORD SCREENRESY;			// actual values
CQEXTERN S32 SCREEN_WIDTH;		//interface size
CQEXTERN S32 SCREEN_HEIGHT;		//interface size

CQEXTERN SINGLE LODPERCENT;			// ranges from 0 to 1.0
CQEXTERN U32   BUILDARCHEID;		// if 0, then build mode is off, else it's the archetype id
CQEXTERN HIMC hIMC;
CQEXTERN SINGLE ELAPSED_TIME;
CQEXTERN SINGLE REALTIME_FRAMERATE;
CQEXTERN S32 EXPCOUNT;				// total number of explosion instances right now
CQEXTERN U32 TEXMEMORYUSED;
CQEXTERN U32 VBMEMORYUSED;
CQEXTERN U32 SNDMEMORYUSED;
enum TEX_LOD
{
	TL_ULTRA_LOW=0,
	TL_LOW=1,
	TL_MEDIUM=2,
	TL_HIGH=3
};
CQEXTERN TEX_LOD TEXLOD;

#define DBG_NONE 0
#define	DBG_SOME -1
#define	DBG_MORE -2
#define DBG_ALL  1

struct GlobalFlags
{
	BOOL32 bFullScreen:1;
	BOOL32 bExceptionHappened:1;
	BOOL32 bGameActive:1;
	BOOL32 bDumpWindow:1;
	BOOL32 bTraceMission:1;
	BOOL32 bTraceNetwork:1;
	BOOL32 bNetBlocking:1;
	BOOL32 bFrameLockEnabled:1;
	BOOL32 debugPrint:2;
	BOOL32 bTracePerformance:1;
	BOOL32 bGamePaused:1;
	BOOL32 bWindowModeAllowed:1;
	BOOL32 b3DEnabled:1;
	BOOL32 bDPLobby:1;
	BOOL32 bTextureBias:1;
	BOOL32 bNoGDI:1;				// no GDI support when in full screen 3D mode
	BOOL32 bPrimaryDevice:1;		// primary render device is in use
	BOOL32 bNoExitConfirm:1;		// true if skipping the exit confirmation dialog
	BOOL32 bLoadingObjlist:1;		// inside Objlist::Load()
	BOOL32 bDumpFile:1;
	BOOL32 bTraceRules:1;
	BOOL32 bRuseActive:1;
	BOOL32 bCQBatcher:1;
	BOOL32 bRTGamePaused:1;			// Real-time part of game is paused
	BOOL32 bFullScreenMap:1;
	BOOL32 bInsideCreateInstance:1;	// for test purposes only
	BOOL32 bClientPlayback:1;		// for test, simulate running game on client machine
	BOOL32 bHostRecordMode:1;		// for test, record skirmish / networked games
	BOOL32 bMovieMode:1;			// in movie mode 
	BOOL32 bFPUExceptions:1;		// enable FPU exceptions
	BOOL32 bStoreDPlayInfo:1;		// store DirectPlay information in registry
	BOOL32 bFullScreenMovie:1;		// playing full screen movie
	BOOL32 bDPDelivery:1;			// use DPLAY's guaranteed delivery mechanism
	BOOL32 bLimitDPConnections:1;	// only use tcp, ipx connections
	BOOL32 bLimitResolutions:1;		// only use 640x480 -> 1024x768
	BOOL32 bInProgressAnimActive:1;  
	BOOL32 bAITest:1;				// fill all players with AI 
	BOOL32 bInstantBuilding:1;		// takes no time to build units/buildings
	BOOL32 bEverythingFree:1;		// everything in the game cost nothing
	BOOL32 bLimitGameTypes:1;		// display only one game type
	BOOL32 bLimitMapSettings:1;		
	BOOL32 bSkipMovies:1;			// if true, do not show any movies
	BOOL32 bSkipIntroMovie:1;		// if true, do not show logo movies
	BOOL32 bLimitMaxPlayers:1;		// if true set max players to 4 in lobby
	BOOL32 bForceLateDelivery:1;	// if true, receive all packets late,  (for playback mode)
	BOOL32 bForceEarlyDelivery:1;	// if true, receive all packets early, (for playback mode)
	BOOL32 bNoToolbar:1;			// if true, not using toolbar
	BOOL32 bInsidePlayerResign:1;	// true when we are blowing up units when player resigns
	BOOL32 bInsideOutZoneLaunch:1;	// true if we launched a game 
	BOOL32 bUseBWCursors:1;			// if true, always use monochrome cursors
	BOOL32 bExtCameraZoom:1;		// if true, extend the max camera zoom
	BOOL32 bDShowLog:1;
	BOOL32 bHardwareGeometry:1;		//if true hardware geometry is enabled in the ini for supporting hardware (also see CQRENDERFLAGS.bHardwareGeometry)
	BOOL32 bAltFontName:1;
} CQEXTERN CQFLAGS;

struct GlobalRenderFlags
{
	BOOL32 bNoPerVertexAlpha:1;
	BOOL32 bSoftwareRenderer:1;		// 3D rendering is being done without hardware support
	BOOL32 bMultiTexture:1;
	BOOL32 b32BitTextures:1;
	BOOL32 bHardwareGeometry:1;
	BOOL32 bStallPipeline:1;
	BOOL32 bFSAA:1;
} CQEXTERN CQRENDERFLAGS;

#define NUM_CIRCLE_SEGS 80 //need for CIRCLELINES extern

#ifdef BUILD_GLOBALS

extern "C" CQEXTERN COLORREF DEFCOLORTABLE[9] = // 8 players plus player "none"
{
	RGB(100,100,100),
	RGB(255,255,0),  //terran  (yellow)
	RGB(240,0,0),	 //mantis	(red)
	RGB(56,52,255),	 //solarian	(blue)
	RGB(255,0,255),	 //vyrium	(pink)
	RGB(18,200,0),	 // (green) 
	RGB(255,150,0),  // (orange)
	RGB(128,0,255),  // (purple)
	RGB(85,218,240)  // (light blue)
};

extern "C" CQEXTERN COLORREF SECTORCOLORTABLE[16] = 
{
	RGB(255,255,255),
	RGB(255,0,0),
	RGB(0,255,0),
	RGB(0,0,255),
	RGB(255,255,0),
	RGB(0,255,255),
	RGB(255,0,255),
	RGB(0,128,255),
	RGB(255,0,128),
	RGB(128,255,0),
	RGB(128,0,255),
	RGB(255,128,0),
	RGB(0,255,128),
	RGB(128,128,255),
	RGB(255,128,128),
	RGB(128,255,128)
};	   
	   
#ifdef _DEMO_

	// FINAL DEMO GUID
	extern "C" CQEXTERN GUID APPGUID_CONQUEST =
		{ 0xecf6f900, 0x5c08, 0x11d5, { 0xae, 0x1,  0,  0x60,  0x08,  0xc9,  0xa7,  0x3 } };

#else

	// FINAL FULL GUID
	extern "C" CQEXTERN GUID APPGUID_CONQUEST =
		{ 0x99717680, 0x49a0, 0x11d1, { 0x91, 0x78,  0,  0x60,  0x8c,  0xf1,  0x35,  0x37 } };

#endif

extern "C" CQEXTERN COLORREF COLORTABLE[9] = {0,0,0,0,0,0,0,0,0}; // 8 players plus player "none"

#else

extern "C" CQEXTERN const COLORREF DEFCOLORTABLE[9]; // 8 players plus player "none"
extern "C" CQEXTERN COLORREF SECTORCOLORTABLE[16];
extern "C" CQEXTERN const GUID APPGUID_CONQUEST;
extern "C" CQEXTERN COLORREF COLORTABLE[9]; // 8 players plus player "none"
#endif

CQEXTERN struct IAnim2D * ANIM2D;
CQEXTERN struct IAnimation * ANIM;
CQEXTERN struct IWindowManager * WM;
CQEXTERN struct ICOManager *DACOM;
CQEXTERN struct IRenderPipeline *PIPE;
CQEXTERN struct	IRenderPrimitive *BATCH;
CQEXTERN struct ICQBatch *CQBATCH;
CQEXTERN struct IRenderer * REND;
CQEXTERN struct IEventSystem *EVENTSYS;
CQEXTERN struct IVideoSurface * SURFACE;
CQEXTERN struct IStatusBarResource * STATUS;
CQEXTERN struct IScrollingText * SCROLLTEXT;
CQEXTERN struct ITeletype * TELETYPE;
CQEXTERN struct ISubtitle * SUBTITLE;
CQEXTERN struct ILineManager * LINEMAN;
CQEXTERN struct IInterfaceManager * INTERMAN;
CQEXTERN struct IHintResource * HINTBOX;
CQEXTERN struct IMenuResource * MENU;
CQEXTERN struct ICursorResource * CURSOR;
CQEXTERN struct IEngine * ENGINE;
CQEXTERN struct ILightManager * LIGHT;
CQEXTERN struct ILights * LIGHTS;
CQEXTERN struct IDAComponent * GS;
CQEXTERN struct IBaseCamera * CAMERA;  /* main camera */
CQEXTERN struct ICamera * MAINCAM;     /* main camera */
CQEXTERN struct IMouseScroll * MSCROLL;
CQEXTERN struct ISystemMap * SYSMAP;
CQEXTERN struct ISector * SECTOR;
CQEXTERN struct IObjectList * OBJLIST;
//CQEXTERN struct IObjectMap * OBJMAP;
CQEXTERN struct IMGlobals * MGLOBALS;
CQEXTERN struct IArchetypeList * ARCHLIST;
CQEXTERN struct IUserDefaults * DEFAULTS;
CQEXTERN struct IComponentFactory * PARSER;
CQEXTERN struct IHotkeyEvent * HOTKEY;
CQEXTERN struct IHotkeyEvent * DBHOTKEY;		// hotkeys for debugging
CQEXTERN struct ITextureLibrary * TEXLIB;
CQEXTERN struct IHardpoint * HARDPOINT;
CQEXTERN struct IVertexBufferManager * VB_MANAGER;
CQEXTERN struct IDebugFontDrawAgent * DEBUGFONT;
CQEXTERN struct IStreamer * STREAMER;
CQEXTERN struct IMusicManager * MUSICMANAGER;
CQEXTERN struct ISFX * SFXMANAGER;
CQEXTERN struct IFogOfWar * FOGOFWAR;
CQEXTERN struct IVoxCompression * VOXCOMP;
CQEXTERN struct ISoundManager * SOUNDMANAGER;
CQEXTERN struct IMission * MISSION;
CQEXTERN struct CQLight * MAINLIGHT;
CQEXTERN struct CQLight * CAMERALIGHT;
CQEXTERN struct IFieldManager * FIELDMGR;
CQEXTERN struct IBanker * BANKER;
CQEXTERN struct IMapGen * MAPGEN;
CQEXTERN struct IMovieCameraManager * CAMERAMANAGER;
CQEXTERN struct IUnbornMeshList * UNBORNMANAGER;
CQEXTERN struct IDust * DUSTMANAGER;
CQEXTERN struct INuggetManager * NUGGETMANAGER;
CQEXTERN struct ITManager * TMANAGER;
CQEXTERN struct IObjMap *OBJMAP;
CQEXTERN struct IBackground *BACKGROUND;
CQEXTERN struct ICommTrack *COMMTRACK;
CQEXTERN struct IFleetMenu * FLEET_MENU;
CQEXTERN struct IEventScheduler * SCHEDULER;
CQEXTERN struct IVideoSystem * VIDEOSYS;
CQEXTERN struct IMaterialManager * MATMAN;
CQEXTERN struct IMeshManager * MESHMAN;
struct IMeshArchetype;//predfined here to solve a lot of problems
CQEXTERN struct IParticleManager * PARTMAN;

CQEXTERN struct IDirectSound * DSOUND;
CQEXTERN struct IDirectPlay4 * DPLAY;
CQEXTERN struct IDirectPlayLobby3 * DPLOBBY;
CQEXTERN struct IZoneScore * ZONESCORE;
CQEXTERN struct INetBuffer * NETBUFFER;
CQEXTERN struct IFileTransfer * FILETRANSFER;
CQEXTERN struct INetPacket * NETPACKET;
CQEXTERN struct ICQImage * CQIMAGE;

CQEXTERN struct BaseHotRect * TOOLBAR;
CQEXTERN struct BaseHotRect * FULLSCREEN;
CQEXTERN struct BaseHotRect * ADMIRALBAR;

CQEXTERN struct IFileSystem * OBJECTDIR;
CQEXTERN struct IFileSystem * MUSICDIR;
CQEXTERN struct IFileSystem * INTERFACEDIR;
CQEXTERN struct IFileSystem * SFXDIR;
CQEXTERN struct IFileSystem * SPEECHDIR;
CQEXTERN struct IFileSystem * TEXTURESDIR;
CQEXTERN struct IFileSystem * MATERIALDIR;
CQEXTERN struct IFileSystem * SPMAPDIR;				// single-player maps
CQEXTERN struct IFileSystem * MPMAPDIR;				// multi-player maps
CQEXTERN struct IFileSystem * SAVEDIR;				//  saved games
CQEXTERN struct IFileSystem * MOVIEDIR;
CQEXTERN struct IFileSystem * NETOUTPUT;			// file used for outputing network info, for testing purposes

CQEXTERN struct IFileSystem * MSPEECHDIR;				// speech used by mission scripts (campaign mode)

CQEXTERN struct IFileSystem * PROFILEDIR;
CQEXTERN struct IFileSystem * SCRIPTSDIR;

CQEXTERN struct IOpAgent * THEMATRIX;
CQEXTERN struct IGeneralData * GENDATA;
CQEXTERN struct IStringData * STRINGDATA;
CQEXTERN struct IUnitComm * UNITCOMM;
CQEXTERN struct IEffectPlayer * EFFECTPLAYER;
CQEXTERN struct IGamePositionCallback * GAMEPOSCALLBACK;

CQEXTERN struct IGameProgress * GAMEPROGRESS;
CQEXTERN struct IBriefing * BRIEFING;

CQEXTERN struct IStringTable * STRINGTABLE;
CQEXTERN struct IScripting * SCRIPTING;

#ifndef FINAL_RELEASE
CQEXTERN class ObjMapIterator * DEBUG_ITERATOR;		// for debugging only
#endif

CQEXTERN LPTOP_LEVEL_EXCEPTION_FILTER prevExceptionHandler;
CQEXTERN LPTOP_LEVEL_EXCEPTION_FILTER cqExceptionHandler;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
enum CQTIMING
{
	// warning! if you change the order or add a new value, you must update the string names in OBJLIST.cpp
	//
	TIMING_FLUSH,
	TIMING_RENDER2D,
	TIMING_RENDERBACKGROUND,
	TIMING_SWAPBUFFERS,
	TIMING_GAMESYSTEM,
	TIMING_UPDATE2D,
	TIMING_CLEARBUFFERS,
	TIMING_PLAYERAI,
	TIMING_OPAGENT,
	//
	TIMING_END		// no other timing values after here!
};


//--------------------------------------------------------------------------//
//--------------------------Interface IDs-----------------------------------//
//--------------------------------------------------------------------------//
// All ID's need to be of the form Interface##ID
//
enum OBJID
{
	IEmptyID = 0,
	IObjectID = 0x10000000,
	IBaseObjectID,
	IGotoPosID,
	ISaveLoadID,
	IWeaponID,
	IWeaponTargetID,
	IAttackID,
	IExplosionID,
	ILauncherID,
	ILaunchOwnerID,
	IExplosionOwnerID,
	IMissionActorID,
	IBlastID,
	IBuildID,
	IHarvestID,
	IBuildShipID,
	IUpgradeID,
	IPlanetID,
	IFabricatorID,
	IFighterID,
	IFighterOwnerID,
	IPlatformID,
	IDebrisID,
	IJumpGateID,
	IMinefieldID,
	IMinelayerID,
	IFireballID,
	IEffectID,
	INuggetID,
	IFieldID,
	ICloakEffectID,
	IAegisShieldID,
	IGroupID,
	IAOEWeaponID,
	IPhysicalObjectID,
	IEngineTrailID,
	ICloakID,
	IAdmiralID,
	ISpenderID,
	IFleetShipID,
	ISupplierID,
	IRepairPlatformID,
	IRepaireeID,
	ITroopshipID,
	IShipDamageID,
	IScrollingTextID,
	IReconProbeID,
	IReconLauncherID,
	IExtentID,
	IMapGenID,
	IBuildEffectID,
	IMovieCameraID,
	IObjectGeneratorID,
	ITriggerID,
	IRecoverShipID,
	IQuickSaveLoadID,
	IBuildQueueID,
	ISpiderDroneID,
	IShuttleID,
	IGameLightID,
	IJumpPlatID,
	IScriptObjectID,
	ITerranDroneID,
	IMimicID,
	IShipMoveID,
	IWormGeneratorID,
	IWormholeSyncID,
	IPlayerBombID,
	IWormholeBlastID,
	ITalorianEffectID,
	IToggleID,
	ISpaceWaveID,
	IHarvestBuilderID,
	ISystemSupplierID,
	IParticleCircleID,
	INovaExplosionID,
	IArtifactID,
	IArtifactHolderID,
	IEffectTargetID,
};
//----------------------------------------------------
// Different packet types (See NetPacket.h)
//  Need 6 bits for this
//
enum PACKET_TYPE
{
	PT_PLAYER_SYNC=-32,
	PT_HOST,
	PT_FILE_TRANSFER,
	PT_GAMEROOM,
	PT_ACK,
	PT_NACK,
	PT_PAUSE,
	PT_TURTLE,
	PT_PARTRENAME,
	PT_ENDGAME,
	PT_NETTEXT,
	PT_RESIGN,
	PT_PLAYERLOST,
	PT_PAUSEWARNING,
	PT_SYNCSTATS,
	PT_RESIGNACK,
	PT_AIRESIGN,
	PT_HOSTPEND,
	PT_HOSTPENDACK,
	// begin game packets
	PT_HOSTUPDATE,
	PT_USRMOVE,
	PT_USRATTACK,
	PT_USRSPATTACK,
	PT_USRSTOP,
	PT_USRAOEATTACK,
	PT_USRWORMATTACK,
	PT_USRBUILD,
	PT_USRFAB,
	PT_USRHARVEST,
	PT_USRRALLY,
	PT_USRESCORT,
	PT_USRDOCKFLAGSHIP,
	PT_USRUNDOCKFLAGSHIP,
	PT_USRRESUPPLY,
	PT_USRSHIPREPAIR,
	PT_USRFABREPAIR,
	PT_USRCAPTURE,
	PT_USRSPABILITY,
	PT_USRFABSALVAGE,
	PT_USRPROBE,
	PT_USRMIMIC,
	PT_USRRECOVER,
	PT_USRDROPOFF,
	PT_USRFABPOS,
	PT_STANCECHANGE,
	PT_PATROL,
	PT_FLEETDEF,
	PT_USRFABJUMP,
	PT_SUPPLYSTANCECHANGE,
	PT_USRKILLUNIT,
	PT_ALLIANCEFLAGS,
	PT_GIFTORE,
	PT_GIFTCREW,
	PT_GIFTGAS,
	PT_USRCREATEWORMHOLE,
	PT_USRCLOAK,
	PT_USR_EJECT_ARTIFACT,
	PT_USRJUMP,
	PT_FIGHTERSTANCECHANGE,
	PT_ADMIRALTACTICCHANGE,
	PT_HARVESTSTANCECHANGE,
	PT_HARVESTERAUTOMODE,
	PT_ADMIRALFORMATIONCHANGE,
	PT_USRFORMATIONMOVE,
	PT_FORMATIONATTACK,
	PT_USEARTIFACTTARGETED,
	// end. no packet types after this point
	PT_LAST
};

/*
namespace DA
{
	enum FILETYPE
	{
		UNKTYPE,
		BMP=1,
		TGA,
		VFX
	};

}
*/

#undef CQEXTERN
#ifdef BUILD_TRIM
#define CQEXTERN __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#endif

//--------------------------------------------------------------------------//
//----------------------------GLOBAL FUNCTIONS------------------------------//
//--------------------------------------------------------------------------//

//--------------------------
// WinVfx16.asm
extern "C"{
S32  WINAPI VFX_shape_scan8 (PANE *pane, U32 transparent_color, S32 hotX, S32 hotY, VFX_SHAPETABLE *shape_table);
void WINAPI VFX_shape_draw (PANE *pane, VFX_SHAPETABLE *shape_table, S32 shape_number, S32 hotX, S32 hotY);
void WINAPI VFX_shape_draw8 (PANE *pane, VFX_SHAPETABLE *shape_table, S32 shape_number, S32 hotX, S32 hotY);
void WINAPI VFX_shape_draw_unclipped8 (PANE *pane, VFX_SHAPETABLE *shape_table, S32 shape_number, S32 hotX, S32 hotY);
void WINAPI VFX_shape_palette (VFX_SHAPETABLE *shape_table, S32 shape_num, VFX_RGB *palette);
S32  WINAPI VFX_shape_colors (VFX_SHAPETABLE *shape_table, S32 shape_num, VFX_CRGB *colors);
S32  WINAPI VFX_shape_bounds (VFX_SHAPETABLE *shape_table, S32 shape_num);
S32  WINAPI VFX_font_height (VFX_FONT *font);
S32  WINAPI VFX_character_width (VFX_FONT *font, S32 character);
S32  WINAPI VFX_character_draw (PANE *pane, S32 x, S32 y, VFX_FONT *font, S32 character, void *color_translate);
void WINAPI VFX_string_draw (PANE *pane, S32 x, S32 y, VFX_FONT *font, const char *string, void *color_translate);
S32  WINAPI VFX_rectangle_hash (const PANE *pane, S32 x0, S32 y0, S32 x1, S32 y1, U32 color);
S32  WINAPI VFX_pane_wipe (const PANE *pane, U32 color);
S32  WINAPI VFX_pane_copy (PANE *source, S32 sx, S32 sy, PANE *target, S32 tx, S32 ty, S32 fill);
void WINAPI VFX_ellipse_fill (PANE *pane, S32 xc, S32 yc, S32 width, S32 height, U32 color);
U32  WINAPI VFX_pixel_write (const PANE *pane, S32 x, S32 y, U32 color);
U32  WINAPI VFX_pixel_read (const PANE *pane, S32 x, S32 y);
S32  WINAPI VFX_line_draw (const PANE *pane, S32 x0, S32 y0, S32 x1, S32 y1, S32 mode, U32 parm);
} // end extern "C"
//
//
//--------------------------
// TerrainMap.cpp
bool __stdcall CreateTerrainMap (struct ITerrainMap ** map);
bool __stdcall CreateDummyTerrainMap (struct ITerrainMap **map);
//--------------------------
// ObjGen.cpp
void __stdcall EditorStartObjectInsertion (PARCHETYPE pArchetype, const struct MISSION_INFO & _info);
void __stdcall EditorStopObjectInsertion (PARCHETYPE pArchetype);
//--------------------------
// BaseObject.cpp
BOOL32 __stdcall RectIntersects (const RECT & rect1, const RECT & rect2);
BOOL32 __stdcall RectIntersects (const RECT & _rect, struct ViewPoint * points, S32 numPoints);
//--------------------------
// Blast.cpp
struct IBaseObject * __stdcall CreateBlast (PARCHETYPE pArchetype, const class TRANSFORM &orientation, U32 systemID, SINGLE animScale=1.0);
//--------------------------
// Debris.cpp
struct IBaseObject * __stdcall CreateDebris (SINGLE lifeTime, BOOL32 bExplodeUponExpiration,INSTANCE_INDEX _index, PARCHETYPE pBlastType, U32 systemID,struct PHYS_CHUNK *chunk);
struct IBaseObject * __stdcall CreateDebris (SINGLE lifeTime, BOOL32 bExplodeUponExpiration,struct MeshChain *_mc, PARCHETYPE _pBlastType, U32 systemID,PHYS_CHUNK *chunk);
//--------------------------
// Trail.cpp
struct IBaseObject * __stdcall CreateTrail (PARCHETYPE pArchetype, IBaseObject *_jumper, Vector _target, U32 _systemID);

//--------------------------
// search.asm
SINGLE __stdcall get_angle (SINGLE x, SINGLE y);
double __fastcall get_angle (double *x, double *y);
void * __fastcall unmemchr (const void * ptr, int c, int size);
extern "C" void rmemcpy (void * dst, const void * src, int size);
//
// global functions defined outside of Conquest.exe
//
//--------------------------
// NetConnection.cpp
CQEXTERN BOOL32 StartNetConnection (BOOL32 & bConnected);
CQEXTERN BOOL32 __stdcall StopNetConnection (bool bStopAll=true);
U32 __stdcall GetHostIPAddress (wchar_t * buffer, wchar_t * buffer2, U32 bufferSize);
CQEXTERN bool __stdcall SendZoneHostChange (void);
//--------------------------
// Interface.cpp
CQEXTERN void CreateSelectPanel(struct IDAComponent * parent, struct ISelectPanel **);
CQEXTERN void CreateEditBox (BaseHotRect *pparent,S32 x,S32 y,S32 x1 ,S32 y1, struct IEditBox ** editBox);
CQEXTERN void CreateSlider (BaseHotRect *pparent, struct ISlider ** slider);
CQEXTERN void CreateDropDown (BaseHotRect *pparent,S32 x,S32 y, struct IDrawAgent *bmp1,struct IDrawAgent *bmp_menu,IDrawAgent *bmp_lit,struct IFontDrawAgent *font, struct IDropDown ** dropDown,enum DROPTYPE _dropType);
CQEXTERN void CreateStaticText(BaseHotRect *pparent,S32 x0,S32 y0,struct IFontDrawAgent *inFont,U32 stringID,struct IStaticText **staticText);
CQEXTERN void CreateTeletype (BaseHotRect *pparent,S32 x,S32 y,U32 dwWidth,struct IFontDrawAgent *font1,struct ITeletype ** teletype);
CQEXTERN void CreateScrollBar (BaseHotRect *pparent,S32 x,S32 y,U32 pnheight,U32 height,struct IDrawAgent *shape,struct IGfxScrollBar ** gfxScrollBar);
CQEXTERN void CreateTextWindow (BaseHotRect *pparent,S32 x,S32 y, U32 _boxWidth, U32 _boxHeight,IFontDrawAgent *font1, struct TextWindow ** _textWindow,S32 _lines);
CQEXTERN void CreateSelectWindow (BaseHotRect *pparent,S32 x,S32 y, U32 _boxWidth, U32 _boxHeight,IFontDrawAgent *font1, struct SelectWindow ** _selectWindow,S32 _lines);
//--------------------------
// Lines.cpp
CQEXTERN void AALine (const PANE *pane,int x0,int y0, int x1, int y1, COLORREF colorref);
//--------------------------
// Rotate.cpp
CQEXTERN BOOL32 CreateRotateRects (BaseHotRect * _parent, struct BaseHotRect **ppRotateLeft, struct BaseHotRect **ppRotateRight, struct BaseHotRect **ppThumb, VFX_SHAPETABLE * table);
//--------------------------
// button.cpp
CQEXTERN BOOL32 CreateButtonRect (BaseHotRect *,PANE *inRect,struct ButtonRect ** buttonRect, struct IDrawAgent *bmp1 = 0, struct IDrawAgent *bmp2 = 0, struct IDrawAgent *bmp3 = 0);
CQEXTERN BOOL32 CreateButtonRect (BaseHotRect *,S32 x0,S32 y0,struct ButtonRect ** buttonRect, struct IDrawAgent *bmp1 = 0, struct IDrawAgent *bmp2 = 0, struct IDrawAgent *bmp3 = 0);
CQEXTERN BOOL32 CreateButtonRect (BaseHotRect *,S32 x0,S32 y0,S32 x1,S32 y1,S32 buttonID,ButtonRect ** buttonRect);
//--------------------------
// UserDefaults.cpp
CQEXTERN BOOL32 CreateUserDefaults (void);
//--------------------------
// VideoSurface.cpp
CQEXTERN COLORREF __fastcall PixelToColorRef (U32 pixel);
CQEXTERN U32 __fastcall ColorRefToPixel (COLORREF color);
//--------------------------
// Camera.cpp
CQEXTERN void OrthoView (const PANE *pane=0);
//--------------------------
// StaticInit.cpp
CQEXTERN extern class ISQRT SQRT;
//--------------------------
// System.cpp
CQEXTERN void __stdcall ParseVideoINI (void);
CQEXTERN extern struct PrimitiveBuilder2 PB;
CQEXTERN void SetupWindowCallback (void);
template <class T> void AddToGlobalCleanupList (T ** component);
template <> CQEXTERN void AddToGlobalCleanupList (IDAComponent ** component);
template <class T>
inline void AddToGlobalCleanupList (T ** component)
{
	AddToGlobalCleanupList((struct IDAComponent **) component);
}
CQEXTERN void CleanupGlobals (void);
CQEXTERN BOOL32 __stdcall CheckDEBUGHotkeyPressed (U32 hotkeyID);
CQEXTERN BOOL32 __stdcall CheckHotkeyPressed (U32 hotkeyID);
CQEXTERN void __stdcall AddToGlobalStartupList (struct GlobalComponent & component);
CQEXTERN void __stdcall CreateGlobalComponents (void);
CQEXTERN SINGLE __fastcall ConvertDSoundToFloat (S32 volume);
CQEXTERN S32 __fastcall ConvertFloatToDSound (SINGLE volume);
CQEXTERN S32 __stdcall AllocateCollisionRegion (void);
CQEXTERN SYSTEMTIME __stdcall GarblyWarbly (const char * _date);
CQEXTERN HFONT __stdcall CQCreateFont (U32 fontDescID, bool bNoScaling=0);		// string resource ID of form "pt, faceName"
CQEXTERN bool  __stdcall CQCreateLogFont (U32 fontDescID, LOGFONT & logFont, bool bNoScaling);	// returns TRUE on success
CQEXTERN void __stdcall ReadRenderOptions (void);
//CQEXTERN void __stdcall Enable3DMode (bool bEnable);
CQEXTERN void __stdcall Start3DMode();
CQEXTERN void __stdcall Shutdown3DMode();
CQEXTERN enum InterfaceRes __stdcall GetCurrentInterfaceRes();
CQEXTERN void __stdcall ChangeInterfaceRes(enum InterfaceRes res);
CQEXTERN void __stdcall Set3DVarialbes (const U32 width, const U32 height, const U32 depth);
CQEXTERN BOOL32 __stdcall FindHardpoint (const char * pathname, INSTANCE_INDEX & index, struct HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
CQEXTERN BOOL32 __stdcall FindHardpointSilent (const char * pathname, INSTANCE_INDEX & index, struct HardpointInfo & hardpointinfo, INSTANCE_INDEX parent);
CQEXTERN S32 __stdcall FindChild (const char * pathname, INSTANCE_INDEX parent);
CQEXTERN void __stdcall EnumerateHardpoints (INSTANCE_INDEX parent, const char * pathName, struct IHPEnumerator * callback);

U32 __stdcall GetMultiplayerVersion (char * buffer, U32 bufferSize);
U32 __stdcall GetBuildVersion (char * buffer, U32 bufferSize);
CQEXTERN U32 __stdcall GetBuildVersion (void);
U32 __stdcall GetProductVersion (char * buffer, U32 bufferSize);
U32 __stdcall GetProductVersion (void);
U32 __stdcall GetMultiplayerNumberFromString (const char * buffer);
U32 __stdcall GetMultiplayerStringFromNumber (U32 number, wchar_t * buffer, U32 bufferSize);
U32 __stdcall GetMultiplayerVersion (void);
CQEXTERN GENRESULT __stdcall CreateProfileParser (const char * filename, struct IProfileParser2 **ppParser, struct IFileSystem * parent=0);	// reads from "Profiles" dir by default
CQEXTERN void __stdcall SetupDiffuseBlend( U32 irp_texture_id , bool bClamp);
CQEXTERN void __stdcall DisableTextures();
CQEXTERN BOOL32 __stdcall RecursiveDelete (IFileSystem * pFileSystem);	// deletes *.* from current directory
CQEXTERN U32 __stdcall CreateZBufferAnalog (const char * filename);
void __stdcall FlipToGDI (void);
//--------------------------
// PrintHeap.cpp
CQEXTERN int MarkAllocatedBlocks (struct IHeap *pHeap);
CQEXTERN int PrintHeap (struct IHeap *pHeap);
CQEXTERN void __cdecl _localprintf (const char *fmt, ...);
CQEXTERN const char * __cdecl _localLoadString (U32 dwID);
CQEXTERN LANGID __cdecl findUserLang();
CQEXTERN void SetLangID(LANGID id);
CQEXTERN const wchar_t * __cdecl _localLoadStringW (U32 dwID);
CQEXTERN int __stdcall _localAnsiToWide (const char * input, wchar_t * output, U32 bufferSize);
CQEXTERN int __stdcall _localWideToAnsi (const wchar_t * input, char * output, U32 bufferSize);
CQEXTERN int __stdcall _localMessageBox (HWND hWnd, U32 textID, U32 titleID, UINT uType);
//----------------------------
// TGARead.cpp
CQEXTERN void __stdcall CreateTGAReader (struct IImageReader ** reader);
//----------------------------
// VfxRead.cpp
CQEXTERN void __stdcall CreateVFXReader (struct IImageReader ** reader);
//----------------------------
// BmpRead.cpp
CQEXTERN void __stdcall CreateBMPReader (struct IImageReader ** reader);
//----------------------------
// DrawAgent.cpp
CQEXTERN void __stdcall CreateDrawAgent (const VFX_SHAPETABLE * vfxShape, U32 subImage, struct IDrawAgent ** drawAgent, BOOL32 bHiRes = false, RECT * pRect=0);
CQEXTERN void __stdcall CreateDrawAgent (struct IImageReader * reader, struct IDrawAgent ** _drawAgent,BOOL32 bHiRes = false, RECT * pRect=0);
CQEXTERN void __stdcall CreateDrawAgent (const char * filename, IComponentFactory *parentFile, DA::FILETYPE type, U32 subImage, struct IDrawAgent ** drawAgent,BOOL32 bHiRes = false);
CQEXTERN void __stdcall DEBUGCreateFontDrawAgent (const VFX_FONT * font, U32 fontImageSize, struct IDebugFontDrawAgent ** _fontDrawAgent, const char *txm_name);
namespace DA { CQEXTERN void LineDraw (const PANE * pane, S32 x0, S32 y0, S32 x1, S32 y1, COLORREF color, BOOL32 bSmooth=0); }
namespace DA { CQEXTERN void PointDraw (const PANE * pane, S32 x0, S32 y0, COLORREF color); }
namespace DA { CQEXTERN void RectangleHash (const PANE *pane, S32 x0, S32 y0, S32 x1, S32 y1, COLORREF color); }
namespace DA { CQEXTERN void RectangleFill (const PANE *pane, S32 x0, S32 y0, S32 x1, S32 y1, COLORREF color); }
namespace DA { CQEXTERN void DrawTexture (const PANE *pane, S32 x0, S32 y0, S32 x1, S32 y1, U32 textureHandle, SINGLE u0, SINGLE v0, SINGLE u1, SINGLE v1); }

//----------------------------
// DrawAgent16.cpp
CQEXTERN void __stdcall CreateFontDrawAgent (HFONT hFont, BOOL32 bOwnFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent);
CQEXTERN void __stdcall CreateNumericFontDrawAgent (HFONT hFont, BOOL32 bOwnFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent);

//----------------------------
// MultiLineFont.cpp
CQEXTERN void __stdcall CreateMultilineFontDrawAgent (PGENTYPE pArchetype, HFONT hFont, COLORREF pen, COLORREF background, struct IFontDrawAgent ** _fontDrawAgent);

//-----------------------------
// ObjWatch.cpp
template <class Type> class OBJPTR;
template <> class OBJPTR<IBaseObject>;
CQEXTERN void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject);
CQEXTERN void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject);
CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID);
CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask);

CQEXTERN void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID,IBaseObject * targetObj, U32 offset);
CQEXTERN void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance);			// remove watcher from list, playerID must be set correctly
//-----------------------------
// Menu1.cpp
CQEXTERN U32 __stdcall CreateMenu1 (bool bSkipMenus);
//-----------------------------
// Menu_SlideShow.cpp
CQEXTERN U32 __stdcall DoMenu_SlideShow (const char * vfxName,SINGLE speed,bool bAllowExit);
CQEXTERN U32 __stdcall DoMenu_Splash(struct SPLASHINFO&);

//-----------------------------
// Modal.cpp
CQEXTERN U32 __stdcall CQDoModal (BaseHotRect * pDialog);
U32 __stdcall CQNonRenderingModal (BaseHotRect * pDialog);
//-----------------------------
// AssertDlg.cpp
CQEXTERN U32 __stdcall AssertBox (const wchar_t *szText);
//-----------------------------
// Menu_confirm.cpp
CQEXTERN U32 __stdcall CreateMenuConfirm (void);
CQEXTERN U32 __stdcall CreateMenuConfirm (U32 textID);
CQEXTERN U32 __stdcall CQMessageBox (wchar_t * message, wchar_t * title, U32 uType, BaseHotRect * parent = NULL);
CQEXTERN U32 __stdcall CQMessageBox (U32 messageID, U32 titleID, U32 uType, BaseHotRect * parent = NULL);
CQEXTERN U32 __stdcall CQMessageBox (U32 messageID, wchar_t * title, U32 uType, BaseHotRect * parent = NULL);
CQEXTERN U32 __stdcall CQMessageBox (wchar_t * message, U32 titleID, U32 uType, BaseHotRect * parent = NULL);
//-----------------------------
// TestScript.cpp
CQEXTERN void __stdcall RunTestScript (const char *cmd_line_script);

//-----------------------------
// Menu_igoptions.cpp
CQEXTERN U32 __stdcall CreateMenu_igoptions (void);
//-----------------------------
// MovieScreen.cpp
struct Frame;
CQEXTERN U32 __stdcall MovieScreen (Frame * parent, const char * filename);
//-----------------------------
// Menu_LoadSave.cpp
U32 __stdcall CreateMenuLoadSave (bool bLoad, bool bSinglePlayerMaps);
//-----------------------------
// Menu_SysKitSaveLoad.cpp
CQEXTERN U32 __stdcall CreateMenuSystemKitSaveLoad (bool bLoad);
//-----------------------------
// LFParser.cpp
CQEXTERN GENRESULT __stdcall CreateLFParser (struct ILFParser ** result);
//-----------------------------
// INIConfig.cpp
CQEXTERN void __stdcall InitRendSections (const char * filename);
typedef HRESULT (FAR PASCAL * LPDDENUMMODESCALLBACK2)(struct _DDSURFACEDESC2 * , LPVOID);
CQEXTERN bool __stdcall TestDeviceFor3D (const char * pGUID);
//CQEXTERN void __stdcall EnumVideoModes (const char * pGUID, LPDDENUMMODESCALLBACK2 callback, LPVOID lpContext);
//HRESULT __stdcall GetDisplayDeviceIDFromGUID (const char * pGUID, DDDEVICEIDENTIFIER2 * pIdent2);
//-----------------------------
// DumpView.cpp
CQEXTERN void __stdcall TrimResetHeap (IHeap * heap);
//-----------------------------
// SysContainer.cpp
CQEXTERN void __stdcall RegisterContainerFactory (void);
//-----------------------------
// WindowManager.cpp
CQEXTERN void __stdcall RegisterWindowManager (void);
//-----------------------------
// InProgressAnim.cpp
CQEXTERN void __stdcall CreateInProgressAnim (struct IPANIM ** ppAnim);
//--------------------------
// VertexBuffer.cpp
CQEXTERN void __stdcall RestoreAllSurfaces();
CQEXTERN extern struct ICQ_VB_Manager *vb_mgr;

#undef CQEXTERN
#ifdef BUILD_MISSION
#define CQEXTERN __declspec(dllexport)
#define MEXTERN  __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#define MEXTERN  __declspec(dllimport)
#endif

//-----------------------------
// Menu_EndGame.cpp
CQEXTERN U32 __stdcall CreateMenuEndGame (bool bWonGame);
//--------------------------
// Ruse.cpp
void __stdcall ActivateRUSE (struct IFileSystem * outFile);
BOOL32 __stdcall DeactivateRUSE (void);
//-----------------------------
// Mission.cpp
CQEXTERN void __stdcall MissionResetHeap (IHeap * heap);
//-----------------------------
// Menu_netunloading.cpp
CQEXTERN U32 __stdcall DoMenu_nul (bool bResign);


#undef CQEXTERN

#endif
