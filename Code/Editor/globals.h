//-----------------------------------------------------------------------------------------------------
// globals.h
//-----------------------------------------------------------------------------------------------------

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#ifdef CQ2_MAIN
	#define CQEXTERN 
#else
	#define CQEXTERN extern
#endif

#define MEXTERN

// global Components

CQEXTERN struct ICOManager * DACOM;
CQEXTERN struct ISystemContainer * SYSTEM;
CQEXTERN struct IEngine * ENGINE;
CQEXTERN struct IRenderPipeline * PIPE;
CQEXTERN struct IRenderPrimitive * BATCH;
CQEXTERN struct IBaseCamera * CAMERA;    // in Camera.cpp for controlling camera
CQEXTERN struct ICamera * CAMERALIB;     // in Camera.cpp for rendering 3DB and CMP files
CQEXTERN struct IRenderer * REND;
CQEXTERN struct ITextureLibrary * TEXLIB;
CQEXTERN struct ILightManager * LIGHT;
CQEXTERN struct IVertexBufferManager * VB_MANAGER;
CQEXTERN struct IAnimation * ANIM;
CQEXTERN struct IHardpoint * HARDPOINT;
CQEXTERN struct IHotkeyEvent * HOTKEY;
CQEXTERN struct IEventSystem * EVENTSYS;
CQEXTERN struct IComponentFactory * PARSER;
CQEXTERN struct IBaseParser * BASEPARSER;
CQEXTERN struct IDataList * GAMETYPES;
CQEXTERN struct IDataList * GENDATA;
CQEXTERN struct IDataList * STRINGPACK;
CQEXTERN struct IFileSystem * OBJECTDIR;
CQEXTERN class  IMode * MODE_CAMPAIGN; // (could be an array of modes...) aaj
CQEXTERN class  IMode * MODE_SCENARIO; // 
CQEXTERN class  IMode * MODE_SECTOR;
CQEXTERN class  IMode * MODE_SYSTEM;
CQEXTERN struct ICampaign * CAMPAIGN;
CQEXTERN struct IStringTable * STRINGTABLE;
CQEXTERN struct IClipboard * CLIPBOARD;
CQEXTERN struct IFieldManager* FIELDMANAGER;
CQEXTERN struct IObjectSelection* OBJECTSELECTION;
CQEXTERN struct IBackground * BACKGROUND; // See SpaceEnv.cpp
CQEXTERN struct IPython * PYTHON; // local scripting API
CQEXTERN struct IDAComponent * GS;
CQEXTERN struct ITManager * TMANAGER;

//CQEXTERN struct IAnim2D * ANIM2D;
//CQEXTERN struct IWindowManager * WM;
//CQEXTERN struct ICOManager *DACOM;
//CQEXTERN struct ICQBatch *CQBATCH;
//CQEXTERN struct IVideoSurface * SURFACE;
//CQEXTERN struct IStatusBarResource * STATUS;
//CQEXTERN struct IScrollingText * SCROLLTEXT;
//CQEXTERN struct ITeletype * TELETYPE;
//CQEXTERN struct ISubtitle * SUBTITLE;
//CQEXTERN struct ILineManager * LINEMAN;
//CQEXTERN struct IHintResource * HINTBOX;
//CQEXTERN struct IMenuResource * MENU;
//CQEXTERN struct ICursorResource * CURSOR;
//CQEXTERN struct ILights * LIGHTS;
//CQEXTERN struct IMouseScroll * MSCROLL;
//CQEXTERN struct ISystemMap * SYSMAP;
//CQEXTERN struct IObjectList * OBJLIST;
//CQEXTERN struct IMGlobals * MGLOBALS;
//CQEXTERN struct IArchetypeList * ARCHLIST;
//CQEXTERN struct IUserDefaults * DEFAULTS;
//CQEXTERN struct IComponentFactory * PARSER;
//CQEXTERN struct IHotkeyEvent * DBHOTKEY;		// hotkeys for debugging
//CQEXTERN struct IDebugFontDrawAgent * DEBUGFONT;
//CQEXTERN struct IStreamer * STREAMER;
//CQEXTERN struct IMusicManager * MUSICMANAGER;
//CQEXTERN struct ISFX * SFXMANAGER;
//CQEXTERN struct IFogOfWar * FOGOFWAR;
//CQEXTERN struct IVoxCompression * VOXCOMP;
//CQEXTERN struct ISoundManager * SOUNDMANAGER;
//CQEXTERN struct IMission * MISSION;
//CQEXTERN struct CQLight * MAINLIGHT;
//CQEXTERN struct CQLight * CAMERALIGHT;
//CQEXTERN struct IFieldManager * FIELDMGR;
//CQEXTERN struct IBanker * BANKER;
//CQEXTERN struct IMapGen * MAPGEN;
//CQEXTERN struct IMovieCameraManager * CAMERAMANAGER;
//CQEXTERN struct IUnbornMeshList * UNBORNMANAGER;
//CQEXTERN struct IDust * DUSTMANAGER;
//CQEXTERN struct INuggetManager * NUGGETMANAGER;
//CQEXTERN struct IObjMap *OBJMAP;
//CQEXTERN struct IBackground *BACKGROUND;
//CQEXTERN struct ICommTrack *COMMTRACK;

CQEXTERN U32 TEXMEMORYUSED;
CQEXTERN U32 VBMEMORYUSED;
CQEXTERN U32 SCREENRESX;			// actual values
CQEXTERN U32 SCREENRESY;			// actual values

// global handles

CQEXTERN HMODULE   hStringTable; // from App\Info.dll
CQEXTERN HDC       hMainDC;
CQEXTERN HINSTANCE hMainInst;
CQEXTERN HWND      hMainWindow;

struct GlobalFlags
{
	bool b3DEnabled;
	bool bPrimaryDevice;
	bool bFPUExceptions;
	bool bFullScreen;
	bool bNoGDI;
	bool bFrameLockEnabled;
	bool bWindowModeAllowed;
	bool bHardwareGeometry;
	bool bExtCameraZoom;
	bool bGameActive;
	bool bSectormapRotates;
	bool bFullScreenMap;
}
CQEXTERN CQFLAGS;

struct GlobalRenderFlags
{
	bool bNoPerVertexAlpha;
	bool bSoftwareRenderer;		// 3D rendering is being done without hardware support
	bool bMultiTexture;
	bool b32BitTextures;
	bool bHardwareGeometry;
	bool bStallPipeline;
	bool bFSAA;
	bool bBackground;
}
CQEXTERN CQRENDERFLAGS;

struct GlobalVariables
{
	SINGLE scrollRate;
}
CQEXTERN CQVARS;

enum TEX_LOD
{
	TL_ULTRA_LOW=0,
	TL_LOW=1,
	TL_MEDIUM=2,
	TL_HIGH=3
};
CQEXTERN TEX_LOD TEXLOD;

enum EditorModes
{
	EM_CAMPAIGN,
	EM_SCENARIO,
	EM_SECTOR,
	EM_SYSTEM,
};
CQEXTERN EditorModes CQEDITORMODE;

struct WindowInput
{
	DWORD  flags;
	CPoint point;
	float  delta;
};

struct AssetData
{
	bool isParent;
	U32  archID;
	AssetData() : isParent(false), archID(0) {}
};

struct ContextData
{
	CPoint point;
	HWND   hwnd;
};

enum CQE_EVENTS
{
	CQE_UPDATE = WM_USER,
	CQE_CAMERA_MOVED,
	CQE_HOTKEY,
	CQE_KILL_FOCUS,
	CQE_SET_FOCUS,
	CQE_WINDOW_RESIZE,
	CQE_MOUSE_MOVE,
	CQE_UNDO,
	CQE_REDO,
	CQE_COPY,
	CQE_CUT,
	CQE_PASTE,
	CQE_NEW_CAMPAIGN,
	CQE_ASSET_CHANGE,
	CQE_ENTITY_SELECT,
	CEQ_CONTEXT_EVENT,
	CEQ_START_CONTEXT,
};

enum CQ_CONTEXTEVENT
{
	CQE_CE_DELETE,
	CQE_CE_RENAME,
	CQE_CE_CLONE,
	CQE_CE_PROPERTIES,
	CQE_CE_WARP,
	CQE_CE_GOTO,
	CQE_CE_ADDTOGROUP,
	CQE_CE_SELECT
};

enum HOTKEYS_ID
{
	IDH_NONE,
	IDH_FIRST_HOTKEY = IDH_NONE,

	IDH_ROTATE_WORLD_LEFT,
	IDH_ROTATE_WORLD_RIGHT,
	IDH_ROTATE_WORLD_UP,
	IDH_ROTATE_WORLD_DOWN,
	IDH_TOGGLE_ZOOM,
	IDH_ROTATE_0_WORLD,
	IDH_ROTATE_90_WORLD_LEFT,
	IDH_ROTATE_90_WORLD_RIGHT,
	IDH_ZOOM_IN,
	IDH_ZOOM_OUT,
	IDH_SCROLL_DOWN,
	IDH_SCROLL_DOWNLEFT,
	IDH_SCROLL_DOWNRIGHT,
	IDH_SCROLL_LEFT,
	IDH_SCROLL_RIGHT,
	IDH_SCROLL_UP,
	IDH_SCROLL_UPLEFT,
	IDH_SCROLL_UPRIGHT,

	IDH_HOTKEYS_COUNT,
};

//--------------------------
// typedefs and defines

typedef unsigned int UniqueID;

struct ARCHNODE;
typedef ARCHNODE * PARCHETYPE;

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

// MAX_SYS_SIZE already defined
#define MIN_SYS_SIZE (10000)

// needed to figure out the size of entire sectors
#define MAX_SECTOR_SIZE (MAX_SYSTEMS*3)

//--------------------------
// primitivebuilder.h
#include <rpul\primitivebuilder.h>
CQEXTERN PrimitiveBuilder PB;

//--------------------------
// clean up
template <class T> void AddToGlobalCleanupList (T ** component);
template <> void AddToGlobalCleanupList (struct IDAComponent ** component);
template <class T>
inline void AddToGlobalCleanupList (T ** component)
{
	AddToGlobalCleanupList((struct IDAComponent **) component);
}
void CleanupGlobals (void);
void SetCleanUpAtExit(void);

//--------------------------
// start up
CQEXTERN void __stdcall AddToGlobalStartupList (struct GlobalComponent & component);

//--------------------------
// free floating globals
namespace Editor
{
	const int MAX_PLAYERS = 16;

	extern U8 playerID;

	const AssetData* GetSelectedAsset();
}

//===========================================================================
#define CQ2ED_PI    ((FLOAT)  3.141592654f)
#define CQ2ED_1BYPI ((FLOAT)  0.318309886f)
#define CQ2EDToRadian( degree ) ((degree) * (CQ2ED_PI / 180.0f))
#define CQ2EDToDegree( radian ) ((radian) * (180.0f / CQ2ED_PI))
//===========================================================================

//--------------------------
// search.asm
SINGLE __stdcall get_angle (SINGLE x, SINGLE y);
double __fastcall get_angle (double *x, double *y);
void * __fastcall unmemchr (const void * ptr, int c, int size);
extern "C" void rmemcpy (void * dst, const void * src, int size);

#endif

