#ifndef _GLOBALS_H_
#define _GLOBALS_H_
//globals.h

#ifdef SOM_MAIN
#define SOMEXPORT 
#else
#define SOMEXPORT extern
#endif

//tempory define
#define INVALID_HARD_POINT (-1)

SOMEXPORT HINSTANCE hMainInst;

SOMEXPORT HWND mainWindow;
SOMEXPORT HWND eventGraph;
SOMEXPORT HWND infoArea;
SOMEXPORT HWND hMenuBar;
SOMEXPORT HWND previewWin;
SOMEXPORT HWND splitter;

SOMEXPORT U32 SCREENRESX;
SOMEXPORT U32 SCREENRESY;

#define FEET_TO_WORLD 1    // scale conversion for granny objects in the world
#define WORLD_TO_FEET 1    // scale conversion for granny objects in the world

SOMEXPORT struct ICOManager * DACOM;
SOMEXPORT struct IBaseCamera * CAMERA;
SOMEXPORT struct ICamera * MAINCAM;     /* main camera */
SOMEXPORT struct IArchetypeList * ARCHLIST;
SOMEXPORT struct IEngine * ENGINE;
SOMEXPORT struct IRenderPipeline *PIPE;
SOMEXPORT struct IVertexBufferManager * VB_MANAGER;
SOMEXPORT struct ITextureLibrary * TEXLIB;
SOMEXPORT struct PrimitiveBuilder2 PB;
SOMEXPORT struct IParticleManager * PARTMAN;
SOMEXPORT struct IMeshManager * MESHMAN;
SOMEXPORT struct IMaterialManager * MATMAN;
SOMEXPORT struct IAnimation * ANIM;
SOMEXPORT struct IRenderer * REND;
SOMEXPORT struct ITManager * TMANAGER;
SOMEXPORT struct IHardpoint * HARDPOINT;

//SOMEXPORT struct ISoundSystemCallback * SOUNDCALLBACK;//part of camera

//SOMEXPORT struct IAggregateComponent * AGG_SOUNDSYS;
//SOMEXPORT struct ISystemComponent * SYS_SOUNDSYS;
//SOMEXPORT struct ISoundSystem * SOUNDSYS;

//SOMEXPORT struct IAggregateComponent * AGG_STREAMER;
//SOMEXPORT struct IStreamer * STREAMER;


SOMEXPORT struct IFileSystem * TEXTURESDIR;
SOMEXPORT struct IFileSystem * OBJECTS;
SOMEXPORT struct IFileSystem * SOUNDDIR;
SOMEXPORT struct IFileSystem * MATERIALDIR;


extern struct IEffectFile * EFFECTFILE;


//#define SOM_STREAMER 0x1000

//--------------------------
// primitivebuilder.h
//#include <primitivebuilder.h>
//SOMEXPORT PrimitiveBuilder PB;

struct IDAComponent;
template <class T> void AddToGlobalCleanupList (T ** component);
template <> void AddToGlobalCleanupList (IDAComponent ** component);
template <class T>
inline void AddToGlobalCleanupList (T ** component)
{
	AddToGlobalCleanupList((IDAComponent **) component);
}
void CleanupGlobals (void);

//--------------------------
// search.asm
SINGLE __stdcall get_angle (SINGLE x, SINGLE y);
double __fastcall get_angle (double *x, double *y);
void * __fastcall unmemchr (const void * ptr, int c, int size);
extern "C" void rmemcpy (void * dst, const void * src, int size);

#endif