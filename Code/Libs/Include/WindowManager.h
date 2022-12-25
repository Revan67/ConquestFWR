#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

//--------------------------------------------------------------------------//
//                                                                          //
//                             WindowManager.h                              //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Author: Rmarr $

	$Header: /Conquest/Libs/Include/WindowManager.h 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
	The WindowManager class provides the simplest, most basic boilerplate windows
	code for creating and managing your game's main window. Please note that the 
	WindowManager does NOT provide access to any drawing surfaces. You should use the
	IDisplay interface for that.

	The WindowManager provides a default window procedure which serves only to update
	the WindowManager's internal state. Most (if not all) applications will need to hook in
	their own windows message callback (See SetCallback(), below.)

	In addition to the IWindowManager interface, the class also supports an outgoing 
	interface, ISystemEventCallback. Your application can implement this interface to control
	the processing of the WM_CLOSE message.

									ISystemEventCallback

	//-----------------------------
	//
    BOOL32 ISystemEventCallback::OnAppClose (void);
		RETURNS:
			FALSE to continue closing the application,
			TRUE to cancel the close.
		OUTPUT:
			This method is called in response to a WM_CLOSE message. If the method returns
			TRUE, the close operation is cancelled. (ie. The default WndProc returns 0, instead 
			of calling the application's window procedure.)
			If the method returns FALSE, the WM_CLOSE message is processed as usual.
		NOTES:
			Typically, this method would bring up a dialog for the user, asking him if he really wants
			to quit. If he decides not to quit after all, return TRUE.


  
      									IWindowManager
	  
		
	//-----------------------------
	//
	S32 IWindowManager::Startup (HINSTANCE instance, 
							 const C8 *appName, 
							 WMEXITCB exitHandler, 
							 U32 flags,
							 const tagWNDCLASSEXA *wc);

		INPUT:
			instance: Handle to the process, received in WinMain.
			appName:  ASCIIZ string to put on the main window's caption.
				This string should remain valid for the life of the program.
			exitHandler: The address of the exit(int) routine supplied by the C runtime library.
				The WindowManager will call this function when it receives a WM_QUIT message.
			flags: 
				WMF_PREVENT_MULTIPLE:	Prevent multiple instance of this program. This method will fail
					with -1 if another instance of the application is already running.
			wc: (Optional) Address of a window class structure.	If this is NULL, the method uses a 
				default window class.
		RETURNS:
			-1 Another instance of the application is running. [FAILURE]
			0 Failed to create the main window. [FAILURE]
			1 Success. Main window created.
		OUTPUT:
			If the function succeeds, it creates a main window for the application. The main window is created
			without the WM_VISIBLE style. To make the window visible, call the ShowWindow() method, below.
		NOTES:
			If you spcified a window class, and the class contains a valid WindowProc, your callback will 
			automatically be registered. (You do not have to call SetCallback().)
		EXAMPLE:
			S32 result;

			result = WM->Startup(hInstance, "MyApp", exit, 0, NULL);


	//-----------------------------
	//
	BOOL32 IWindowManager::SetWindowPos (U32 width, U32 height, U32 flags);
		INPUT:
			width: Desired width (in pixels) for the client area of the window.
			height: Desired height (in pixels) for the client area of the window.
			flags:
				WMF_FULL_SCREEN:	Causes the main window to become a full screen (TOPMOST), pop-up style window, without
									a caption bar or system menu. (ie. The client area fills the desktop.) The
									width and height paramters are ignored. Use this flag in preparation for
									starting full screen mode in Direct Draw.

				WMF_ALLOW_WINDOW_RESIZE:	
									Enables the "Maximize" option on the system menu, and allows the user to dynamically
									resize the main window.

				WMF_PREVENT_ALT_MENU_POPUP:
									Prevents the ALT button from activating the system menu. After setting this flag, 
									ALT-key combinations will be ignored by the system.
				WMF_ALWAYS_ON_TOP:
									The main window is given the TOPMOST style.
				WMF_SHOW:			Call ShowWindow() after resizing, moving window.
				WMF_CENTER:			Center the window on the desktop
		RETURNS:
			TRUE if it successfully resized the window, 
			FALSE on failure. (e.g. Desktop resolution was too small for desired width, height)
		OUTPUT:
			Resizes the main window, and makes it visible.

	//-----------------------------
	//
	void IWindowManager::SetCallback (WNDPROC wndProc);
		INPUT:
			wndProc: Address of user callback procedure.
		OUTPUT:
			Your wndProc will receive messages from the system. Messages that are not handled should be
			sent to the DefaultWindowProc.
		NOTES:
			This method is optional. If you do not need to process any windows messages, you may choose
			not to set a callback.
		Example callback procedure:
			LONG MyWndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
			{
				switch (message)
				{
				   ...
				}

				return DefWindowProc(hWindow, message, wParam, lParam);
			}

	//-----------------------------
	//
	HWND IWindowManager::GetWindowHandle (void);
		RETURNS:
			Handle to the main window.

	//-----------------------------
	//
	void IWindowManager::ServeMessageQueue (U32 flags);
		INPUT:
			flags:
				0					Process messages in the queue until the queue is empty AND the application
									is in the active state.
				WMF_WAIT:			Causes the method to wait until there is at least one message in the queue.
									The method returns when all messages in the queue have been processed.
				WMF_BACKGROUND:		Process all messages in the queue an return immediately. (i.e. Enables application
									to run in the background.)
				WMF_DLGMSG			Send messages through dialog message filter (IsDialogMessage())
		OUTPUT:
			Processes messages in the input queue. If it receives a WM_QUIT message, the method calls the
			exit handler that was set in Startup().

	//-----------------------------
	//
	void IWindowManager::GetWindowArea (WM_WINAREA & area);
		INPUT:
			area: structure that receives the coordinates of the window, and its width and height.
		OUTPUT:
			The returned area encompasses the window's border, and caption if applicable.

	//-----------------------------
	//
    void IWindowManager::GetClientArea (WM_WINAREA & area);
		INPUT:
			area: structure that receives the coordinates of the client rectangle, and its width and height.
		OUTPUT:
			The returned area encompasses just the client area of the main window. Coordinates are in screen space.

	//-----------------------------
	//
	BOOL32 IWindowManager::AppIsActive (void);
		RETURNS:
			TRUE if application is active (in the foreground.)

	//-----------------------------
	//
	void IWindowManager::SetCursorPos (S32 x, S32 y);
		INPUT:
			x, y: Coordinates in client coordinates to set the cursor.
		OUTPUT:
			Translates the coordinates from client coordinates into screen coordinates, then sets the
			new cursor position using the Windows SetCursorPos method.

	//-----------------------------
	//
	void IWindowManager::GetCursorPos (S32 & x, S32 & y);
		INPUT:
			x, y: Variables that receive the coordinates of the cursor, in client coordinates.
		NOTES:
			If the cursor is not within the application's main window, the returned values are undefined.

*/
//---------------------------------------------------------------------------

#ifndef DACOM_H
#include "DACOM.h"
#endif

#ifndef _WINDOWS_
#error Windows.h required for this to compile!
#endif

//-----------------------------
// flags used in Startup() method
//-----------------------------
#define WMF_PREVENT_MULTIPLE		0x00000001			// prevent multiple instance of this program

//-----------------------------
// flags used in SetWindowPos() method
//-----------------------------
#define WMF_FULL_SCREEN				0x00000001
#define WMF_ALLOW_WINDOW_RESIZE		0x00000002
#define WMF_PREVENT_ALT_MENU_POPUP	0x00000004
#define WMF_ALWAYS_ON_TOP			0x00000008
#define WMF_SHOW					0x00000010
#define WMF_CENTER					0x00000020

//-----------------------------
// flags used in ServeMessageQueue() method
//-----------------------------
#define WMF_WAIT					0x00000001			// wait for messages
#define WMF_BACKGROUND				0x00000002			// (ie. run in background same as foreground)
#define WMF_DLGMSG					0x00000004			// dialog message processing

//
// Structures
//

typedef struct tagWM_WINAREA
{
   S32 x;
   S32 y;
   S32 w;
   S32 h;
}
WM_WINAREA;

//
// Function pointer types
//

typedef void (__cdecl * WMEXITCB) (int);
typedef long LONG;
typedef LONG HRESULT; 
typedef LONG LRESULT;
typedef void *HANDLE;
typedef unsigned int UINT;
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LRESULT (__stdcall * WNDPROC)(HWND, UINT, WPARAM, LPARAM);
struct tagWNDCLASSEXA;

//
// ISystemEventCallback is an outgoing interface supported by the WindowManager.
// Implement this interface in your application to control the handling of the 
// WM_CLOSE message.
//
//-----------------------------
//
struct ISystemEventCallback : public IDAComponent
{
	DEFMETHOD_(BOOL32,OnAppClose) (void) = 0;
};

//
// in order to create an instance of the WindowManager, 
// Use the AGGDESC descriptor. 
// Set interface_name = "IWindowManager", and description = NULL or "WindowManager"
// 

//---------------------------------------------------------------------------------
//--------------------------Window Manager Interface-------------------------------
//---------------------------------------------------------------------------------

#define IWINDOWMANAGER_VERSION 2
#define IID_IWindowManager MAKE_IID("IWindowManager",2)


struct DACOM_NO_VTABLE IWindowManager : public IDAComponent
{
	DEFMETHOD_(S32,Startup) (HINSTANCE instance, 
							 const C8 *appName, 
							 WMEXITCB exitHandler, 
							 U32 flags=0,
							 const tagWNDCLASSEXA *wc = 0) = 0;

	DEFMETHOD_(BOOL32,SetWindowPos) (U32 width, U32 height, U32 flags=0) = 0;

	DEFMETHOD_(void,SetCallback) (WNDPROC wndProc) = 0;

	DEFMETHOD_(HWND,GetWindowHandle) (void) const = 0;

	DEFMETHOD_(void,ServeMessageQueue) (U32 flags=0) = 0;

	DEFMETHOD_(void,GetWindowArea) (WM_WINAREA & area) = 0;

    DEFMETHOD_(void,GetClientArea) (WM_WINAREA & area) = 0;

	DEFMETHOD_(BOOL32,AppIsActive) (void) const = 0;

	DEFMETHOD_(void,SetCursorPos) (S32 x, S32 y) = 0;

	DEFMETHOD_(void,GetCursorPos) (S32 & x, S32 & y) = 0;
};

//---------------------------------------------------------------------------------
//------------------------------End WindowManager.h--------------------------------
//---------------------------------------------------------------------------------

#endif
