#ifndef USERDEFAULTS_H
#define USERDEFAULTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                             UserDefaults.H                               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/UserDefaults.h 46    9/13/01 10:01a Tmauer $
*/			    
//--------------------------------------------------------------------------//
/*
        Stores user preferences in the registry
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

#undef UDEXTERN
#ifdef BUILD_TRIM
#define UDEXTERN __declspec(dllexport)
#else
#define UDEXTERN __declspec(dllimport)
#endif

//--------------------------------------------------------------------------//
// On GetDefaults(), if dwSize member is wrong, the values stored may not be valid
//
//--------------------------------------------------------------------------//

enum FOGOFWARMODE
{
	FOGOWAR_NORMAL=-1,
	FOGOWAR_EXPLORED,
	FOGOWAR_NONE
};
struct SOUNDSTATE
{
	struct ITEM
	{
		bool bMute;
		S32  volume;		// 0 to 10
	} music, effects, comms, chat;

	void init (void);
};

struct USER_DEFAULTS
{
	U32  dwSize;
	U32  dwVersion;
	S32  iMainX, iMainY;
	S32  iMainWidth, iMainHeight;
	U32  showState;
	S32  iViewerWidth, iViewerHeight;
	U32  minLatency;	    // in mseconds
	U32  packetLossPercent;	// 0 = no loss, 100 = every packet is lost
	S32  mouseSpeed;		// 0 = normal, -10 = slowest, +10 = fastest
	SINGLE scrollRate;	// default is 1.0 screens per second
	SOUNDSTATE soundState;
	S32  gammaCorrection;   // 0 = normal, -9 = darkest, +20 = brightest
	// (mission.cpp uses &gammaCorrection for anti-cheating)
	S32  gameSpeed;		    // 0 = normal, -10 = slowest, +10 = fastest
	FOGOFWARMODE fogMode:2;	
	U32 bWindowMode:1;      // not using ddraw
	U32 bNoAutoSave:1;		// never ask when closing
	U32 bNoTooltips:1;		// turn off tooltips
	U32 bNoFrameLimit:1;	// turn off frame limiting
	U32 bPosToolWorld:1;	// display world coordinates for cursor position, instead of screen
	U32 bEditorMode:1;
	U32 bDrawHotrects:1;
	U32 bNoStatusBar:1;
	U32 bVisibilityRulesOff:1;
	U32 bChoseMultiplayer:1;	// true if player selected "Multiplayer" from first option menu last time
	U32 bChoseJoin:1;			// true if player chose to join a game last time
	U32 bPartDlgVisible:1;		// true if part dlg is turned on
	U32 bCheatsEnabled:1;		// true if cheats are enabled
	U32 bSectormapRotates:1;	// true if user wants sector map to rotate with main view
	U32 bNoAutoTarget:1;		// true if auto selection of targets is disabled
	U32 bNoWinningConditions:1;	// true if game ignores end-game conditions
	U32 bNoHints:1;				// turn off game hints
	U32 bRightClickOption:1;	// are we using the right click model or the default left click model
	U32 bInfoHighlights:1;		// when highlighting spaceships relevent ranges will be displayed
	U32 bNoSupplies:1;			// don't use supplies (testing)
	U32 bNoDamage:1;			// don't use damage	(testing)
	U32 bShowGrids:1;			// show grid squares
	U32 bCheapMovement:1;		// always use low quality movement code
	U32 bHardwareRender:1;		// use 3D hardware for rendering
	U32 bConstUpdateRate:1;		// call update more frequently if user increases speed
	U32 bHardwareCursor:1;		// use GDI for cursor (when primary device is being used)
	U32 bLockDiplomacy:1;		// true if alliances are not to be changed during a session
	U32 bSpectatorModeAllowed:1;	// true if we will allow players to see all after losing
	U32 bSpectatorModeOn:1;		// true if current player can see all
	U32 bSubtitles:1;			//true turns on subtitles
	U32 bNetworkBandwidth:1;	//true means use high speed bandwidth option
	U32 maxProjectiles;         //the maximum number of projectiles to allow to exist.
};


struct DACOM_NO_VTABLE IUserDefaults : public IDAComponent
{
virtual BOOL32 __stdcall LoadDefaults (void) = 0;

virtual BOOL32 __stdcall StoreDefaults (void) = 0;

virtual BOOL32 __stdcall DeletePlayerDefaults (const char * playerName) = 0;

virtual U32 __stdcall GetDataFromRegistry (const char *szKey, void *buffer, U32 dwBufferSize) = 0;
	
virtual BOOL32 __stdcall SetDataInRegistry (const char *szKey, void *buffer, U32 dwBufferSize) = 0;

virtual BOOL32 __stdcall GetStringFromRegistry (char *szDirName, U32 id) = 0;
	
virtual BOOL32 __stdcall SetStringInRegistry (const char *szDirName, U32 id) = 0;

virtual BOOL32 __stdcall GetInputFilename (char *dst, U32 id) = 0;	// stores resulting directory name in registry

virtual BOOL32 __stdcall GetOutputFilename (char *dst, U32 id) = 0;	// stores resulting directory name in registry

virtual BOOL32 __stdcall SetNameInMRU (const char *szFileName, U32 id, U32 pos=0) = 0;

virtual BOOL32 __stdcall GetNameInMRU (char *szFileName, U32 id, U32 pos=0) = 0;

virtual S32 __stdcall FindNameInMRU (const char *szFileName, U32 id) = 0;

virtual BOOL32 __stdcall RemoveNameFromMRU (const char *szFileName, U32 id) = 0;

virtual BOOL32 __stdcall RemoveNameFromMRU (U32 id, U32 pos) = 0;

virtual BOOL32 __stdcall InsertNameIntoMRU (const char *szFileName, U32 id) = 0;

virtual BOOL32 __stdcall GetUserData (const C8 * typeName, const C8 * instanceName, void * data, U32 dataSize) = 0;

virtual BOOL32 __stdcall GetScriptData (void * symbol, const C8 * instanceName, void * data, U32 dataSize) = 0;

virtual U32 __stdcall GetStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize) = 0;
	
virtual U32 __stdcall SetStringInRegistry (const char *szKey, const char *buffer) = 0;

virtual U32 __stdcall GetInstallStringFromRegistry (const char * szKey, char *buffer, U32 dwBufferSize) = 0;

virtual void __stdcall ResetToDefaultSettings (void) = 0;

virtual BOOL32 __stdcall UbiEula () = 0;

static USER_DEFAULTS * GetDefaults (void)
	{
		return pUserDefaults;
	}

protected:

	static UDEXTERN struct USER_DEFAULTS * pUserDefaults;

};


#endif
