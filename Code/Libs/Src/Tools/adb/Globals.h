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
*/			    
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
//----------------------------GLOBAL #defines-------------------------------//
//--------------------------------------------------------------------------//

#define CLR_GRAY 0x40004210


#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480


/*#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)  // message that will be supported
                                        // by the OS 
#endif*/

//--------------------------------------------------------------------------//
//-------------------GLOBAL message #defines for Event System---------------//
//--------------------------------------------------------------------------//

#define HOTRECT_PRESSED		0x1000			// parm = NULL, or IDAComponent that was pressed
#define ENDFRAME_EVENT		0x1001
#define PANEL_OWNED			0x1002
#define HOTKEY_EVENT		0x1003
#define JOYSTICK_EVENT		0x1004



//--------------------------------------------------------------------------//
//----------------------------GLOBAL data items-----------------------------//
//--------------------------------------------------------------------------//

extern HINSTANCE hInstance;
extern HWND hMainWindow;
extern BOOL32 NOGUI;

extern struct IViewConstructor2 *PARSER;
extern struct ICOManager *DACOM;
extern struct IUserDefaults *DEFAULTS;
extern char *pPreprocessBlock;

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
	IObjConnectionPointContainerID,
	IObjConnectionPointID,
	IGotoPosID
};

//--------------------------------------------------------------------------//
//----------------------------GLOBAL FUNCTIONS------------------------------//
//--------------------------------------------------------------------------//

//--------------------------
// UserDefaults.cpp
BOOL32 CreateUserDefaults (void);
//--------------------------
// PrintHeap.cpp
int MarkAllocatedBlocks (struct IHeap *pHeap);
int PrintHeap (struct IHeap *pHeap);
void __cdecl _localprintf (const char *fmt, ...);
const char * __cdecl _localLoadString (U32 dwID);

#endif