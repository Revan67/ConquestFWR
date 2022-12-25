//--------------------------------------------------------------------------//
//                                                                          //
//                             WindowManager.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/WindowManager.cpp 9     4/09/01 4:19p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

#include "pch.h"
#include <globals.h>

#include "resource.h"
#include <CQTrace.h>

#include <TComponent.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <fdump.h>
#include <tempstr.h>
#include <da_heap_utility.h>

#include <WindowManager.h>
#include <System.h>

static C8 *interface_name = "IWindowManager";       // Interface name used for registration

static struct WindowManager *GlobalWindowManagerSingleton;


//--------------------------------------------------------------------------//
//
struct WMInner : public DAComponentInner<WindowManager>
{
	WMInner (WindowManager * _owner) : DAComponentInner<WindowManager>(_owner)
	{
	}
	
	DEFMETHOD_(U32,Release) (void);
};
//--------------------------------------------------------------------------//
//
struct WindowManager : public ISystemComponent, 
					   IComponentFactory,
					   ConnectionPointContainer<WindowManager>,
					   IWindowManager
{
	
	BEGIN_DACOM_MAP_INBOUND(WindowManager)
	DACOM_INTERFACE_ENTRY(ISystemComponent)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	DACOM_INTERFACE_ENTRY(IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY2(IID_IDAConnectionPointContainer, IDAConnectionPointContainer)
	DACOM_INTERFACE_ENTRY(IWindowManager)
	DACOM_INTERFACE_ENTRY2(IID_IWindowManager, IWindowManager)
	END_DACOM_MAP()
	
	BEGIN_DACOM_MAP_OUTBOUND(WindowManager)
	DACOM_INTERFACE_ENTRY_AGGREGATE("ISystemEventCallback", point)
	END_DACOM_MAP()

	ConnectionPoint<WindowManager,ISystemEventCallback> point;

	BOOL32 bInitialized;
	WMInner innerComponent;
	IDAComponent *outerComponent;

	CRITICAL_SECTION * criticalSection;
	//--------------------------------------
	// data items
	//--------------------------------------

	U32		   showWindowFlags;	

	U16        wClientHeight, wClientWidth; // size of client window

	S32		   bAppActive;					// TRUE if this app has input focus
	
	WMEXITCB   exitCallback;				// Address of mandatory exit callback
	WNDPROC    windowCallback;				// Address of WNDPROC, if any
	HANDLE     hMutex;						// Semaphore to limit instances
	
	HWND       hWnd;						// Handle to application window
	
	//
	// windowed-mode variables and related data
	//
	
	RECT		clientRect, windowRect;
	
	S32			xPosMouse, yPosMouse;	  // client relative numbers

	//--------------------------------------
	// end data items
	//--------------------------------------
	
	WindowManager (void) : innerComponent(this), point(0)
	{
		outerComponent = &innerComponent;
		bAppActive = 1;
	}

	~WindowManager (void)
	{
		if( hWnd ) {
			DestroyWindow( hWnd );
		}
	}

	GENRESULT init (AGGDESC * info)
	{
		return GR_OK;
	}

	DA_HEAP_DEFINE_NEW_OPERATOR(WindowManager)
	
	/* IDAComponent methods */
	
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance)
	{
		return outerComponent->QueryInterface(interface_name, instance);
	}
	DEFMETHOD_(U32,AddRef)           (void)
	{
		return outerComponent->AddRef();
	}
	DEFMETHOD_(U32,Release)          (void)
	{
		return outerComponent->Release();
	}
	
	/* IComponentFactory methods */
	
	DEFMETHOD(CreateInstance) (DACOMDESC  *descriptor, void **instance);
	
	/* IAggregateComponent methods */
	
    DEFMETHOD(Initialize) (void);
	
	/* ISystemComponent methods */
	
	DEFMETHOD_(void,Update) (void);
	
	/* IWindowManager methods */
	
	DEFMETHOD_(S32,Startup) (HINSTANCE instance, 
							 const C8 *appName, 
							 WMEXITCB exitHandler, 
							 U32 flags,
							 const tagWNDCLASSEXA *wc = 0);

	DEFMETHOD_(BOOL32,SetWindowPos) (U32 width, U32 height, U32 flags);

	DEFMETHOD_(void,SetCallback) (WNDPROC wndProc);

	DEFMETHOD_(HWND,GetWindowHandle) (void) const;

	DEFMETHOD_(void,ServeMessageQueue) (U32 flags=0);

	DEFMETHOD_(void,GetWindowArea) (WM_WINAREA & area);

    DEFMETHOD_(void,GetClientArea) (WM_WINAREA & area);

	DEFMETHOD_(BOOL32,AppIsActive) (void) const;

	DEFMETHOD_(void,SetCursorPos) (S32 x, S32 y);

	DEFMETHOD_(void,GetCursorPos) (S32 & x, S32 & y);

	/* WindowManager methods */
	
	IDAComponent * getBase (void)
	{
		return (IDAComponent *) ((ISystemComponent *) this);
	}
	
	void getWindowRects (void);

	long WndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);
	static LONG CALLBACK _wndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

	BOOL32 resizeTheWindow (S32 width, S32 height, U32 flags);

	void shutdown (void);

	BOOL32 onClose (void);

	static BOOL32 onClose (CONNECTION_NODE<ISystemEventCallback> *node);
};

//-------------------------------------------------
//
U32 WMInner::Release (void)
{
	U32 result;
	
	if ((result = DAComponentInner<WindowManager>::Release()) == 1)
	{
		if (owner->bInitialized)
		{
			CQTRACE10("Shutting down window manager.");
			
			owner->shutdown();
			owner->outerComponent = &owner->innerComponent;				// remove container reference
		}
		GlobalWindowManagerSingleton = 0;
	}
	return result;
}
//--------------------------------------------------------------------------------------
//
LONG WindowManager::_wndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	if( GlobalWindowManagerSingleton ) 
	{
		if (GlobalWindowManagerSingleton->criticalSection)
			EnterCriticalSection(GlobalWindowManagerSingleton->criticalSection);

		LONG result = GlobalWindowManagerSingleton->WndProc(hWindow, message, wParam, lParam);

		if (GlobalWindowManagerSingleton->criticalSection)
			LeaveCriticalSection(GlobalWindowManagerSingleton->criticalSection);

		return result;
	}

	return DefWindowProc(hWindow, message, wParam, lParam);
}
//--------------------------------------------------------------------------------------
//
long WindowManager::WndProc (HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		if (bAppActive && onClose())		// just quit if we are not active
		{
			return 0;
		}
		break;

	case WM_SIZE:
		wClientWidth = LOWORD(lParam);
		wClientHeight = HIWORD(lParam);
		
		// fall through intentional
	
	case WM_MOVE:
		getWindowRects();
		break;

	case WM_ACTIVATEAPP:
		if (wParam)
		{
            if (!bAppActive)
			{
				CQTRACE10("WindowManager: WndProc: WM_ACTIVATEAPP activated");

				bAppActive = TRUE;
				
				if (showWindowFlags & WMF_FULL_SCREEN)
				{
					SetWindowLong(hWnd, 
						GWL_STYLE, 
						GetWindowLong(hWnd, 
						GWL_STYLE) & ~WS_SYSMENU);
				}
			}
		}
		else
		{
            if (bAppActive)
			{
				CQTRACE10("WindowManager: WndProc: WM_ACTIVATEAPP deactivated");

				bAppActive = FALSE;
				xPosMouse = yPosMouse = 0x80000000;		// invalidate mouse coordinates
				
				if (showWindowFlags & WMF_FULL_SCREEN)
				{
					SetWindowLong(hWnd, 
						GWL_STYLE, 
						GetWindowLong(hWnd, 
						GWL_STYLE) | WS_SYSMENU);
				}
			}
		}
		break;
		
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		xPosMouse = LOWORD(lParam);
		yPosMouse = HIWORD(lParam);
		
		if (showWindowFlags & WMF_FULL_SCREEN)
		{
			POINT pt;
			
			pt.x = xPosMouse;
			pt.y = yPosMouse;
			
			ClientToScreen(hWnd, &pt);
			xPosMouse = pt.x;
			yPosMouse = pt.y;
		}
		
	case WM_SYSKEYUP:
		if (wParam == VK_MENU)
		{
			if (GetMenu(hWindow) == NULL && (showWindowFlags & WMF_PREVENT_ALT_MENU_POPUP))
			{
				//
				// If key pressed was ALT by itself, set wParam to 0 (invalid
				// key code) to prevent system menu from popping up and 
				// interfering with app
				//
				
				wParam = 0;
			}
		}
		
		break;
		
	case WM_DESTROY:
		CQTRACE10("WindowManager: WndProc: WM_DESTROY");
		
		bAppActive     = FALSE;
		PostQuitMessage(0);
		break;
	}  // end switch (message)


	if (windowCallback != NULL)
	{
		return windowCallback(hWindow, message, wParam, lParam);
	}
	else
	{
		return DefWindowProc(hWindow, message, wParam, lParam);
	}
}
//----------------------------------------------------------------------------------------
//
GENRESULT WindowManager::CreateInstance (DACOMDESC *descriptor, void **instance)
{
	ASSERT(instance);
	//
	// Assume failure
	//
	
	*instance = NULL;
	
	//
	// If unsupported interface or capability requested, fail call
	//
	
	AGGDESC *info = (AGGDESC *) descriptor;
	
	if (info == NULL || info->interface_name == NULL)
		return GR_INVALID_PARMS;
	   
	if (info->size != sizeof(*info))
	{
		return GR_INTERFACE_UNSUPPORTED;
	}
	
	if (strcmp(info->interface_name, interface_name))
	   {
		return GR_INTERFACE_UNSUPPORTED;
	   }
	
	if (info->description && strcmp(info->description, "WindowManager"))
		return GR_INTERFACE_UNSUPPORTED;
	
	
	//
	// System interface layer does not yet exist -- create it
	//
	// Begin by initializing WindowManager library -- return failure if 
	// WindowManager initializion fails
	//
	// Fail attempts to launch more than one instance
	//
	if (innerComponent.ref_count == 1)
	{
		if (info->outer)
		{
			outerComponent = info->outer;
			*(info->inner) = &innerComponent;
		}
	}
	else
		return GR_GENERIC;		// can only have instance
	
	innerComponent.ref_count++;
    *instance = (ISystemComponent *) this;
	GlobalWindowManagerSingleton = this;
	
	return GR_OK;
}
//---------------------------------------------------------------------
//
GENRESULT WindowManager::Initialize (void)
{
	return GR_OK;
}
//---------------------------------------------------------------------
//
void WindowManager::Update (void)
{
	ServeMessageQueue();
}
//---------------------------------------------------------------------
//
S32 WindowManager::Startup (HINSTANCE hInstance, const C8 *appName, WMEXITCB exitHandler, U32 flags, const tagWNDCLASSEXA *pwc)
{
	WNDCLASSEX wc;
	S32 result = 0;

	if (bInitialized)
	{
		result = 1;
		goto Done;
	}
	
	//
	// Reject attempt to launch more than one instance of this application
	//
	hMutex = CreateMutex(NULL, 0, appName);
	
	//
	// Check for redundant startup by creating a semaphore and checking to
	// see if it already existed
	//
	
	if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		wchar_t name[64];
		wcsncpy(name, _localLoadStringW(IDS_APP_NAME), sizeof(name)/sizeof(wchar_t));
		MessageBoxW(0, _localLoadStringW(IDS_ALREADY_RUNNING), name, MB_OK | MB_ICONEXCLAMATION);
		CloseHandle(hMutex);
		goto Done;
	}
	
	//
	// Generate startup message
	//
	
	CQTRACE12("WindowManager: Startup: '%s' HINSTANCE %08X", appName, hInstance);
	
	//
	// Set (mandatory) exit callback function
	//
	
	if ((exitCallback = exitHandler) == 0)
		goto Done;
	
	//
	// Set up and register application window class
	//
	if (pwc)
	{
		wc = *pwc;
		windowCallback = wc.lpfnWndProc;
		wc.lpfnWndProc = _wndProc;
	}
	else
	{
		memset(&wc, 0, sizeof(wc));
		wc.cbSize		 = sizeof(wc);
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = _wndProc;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = hInstance;
		wc.hIcon         = 0; // LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor       = 0; // LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = 0; // GetStockObject(BLACK_BRUSH); 
		wc.lpszMenuName  = NULL;
		wc.lpszClassName = appName;
	}
	
	if (RegisterClassEx(&wc) == 0)
		goto Done;
	
	//
	// Create application's main window
	//
	
	hWnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		appName,
		WS_CLIPCHILDREN|WS_CLIPSIBLINGS, 
		0,
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);
	
	if (!hWnd)
	{
		GENERAL_WARNING( "WindowManager: Startup: CreateWindowEx() failed" );
		goto Done;
	}
	
	result = bInitialized = TRUE;

Done:
	return result;
}
//---------------------------------------------------------------------
//
BOOL32 WindowManager::SetWindowPos (U32 width, U32 height, U32 flags)
{
	BOOL32 result=0;

	if (flags & WMF_FULL_SCREEN)
	{
		//
		// Disable caption menu
		//
		
		SetWindowLong(hWnd, 
			GWL_STYLE, 
			GetWindowLong(hWnd, GWL_STYLE) | WS_POPUP);
		
		SetWindowLong(hWnd, 
			GWL_STYLE, 
			GetWindowLong(hWnd, GWL_STYLE) & ~(WS_OVERLAPPED  | 
			WS_CAPTION     | 
			WS_SYSMENU     | 
			WS_MINIMIZEBOX | 
			WS_MAXIMIZEBOX | 
			WS_THICKFRAME));
	
		//
		// Set window boundaries to cover entire desktop, and show it
		//
			
		U32 screenWidth  = GetSystemMetrics(SM_CXSCREEN);
		U32 screenHeight = GetSystemMetrics(SM_CYSCREEN);
			
		if (screenWidth <= width || screenHeight <= height)
		{
			::SetWindowPos(hWnd, 
				HWND_TOP, 
				0,
				0,
				width,
				height,
				SWP_NOCOPYBITS | SWP_NOZORDER);
		}
		else
		{
			::SetWindowPos(hWnd, 
				HWND_TOP, 
				0,
				0,
				screenWidth,
				screenHeight,
				SWP_NOCOPYBITS | SWP_NOZORDER);
		}

		result = 1;
	}
	else
		result = resizeTheWindow(width, height, flags);


	if (result)
	{
		showWindowFlags = flags;
		if (flags & WMF_SHOW)
			::ShowWindow(hWnd, SW_SHOWNORMAL);
	}

	//
	// NOTE: If the program is switching to windowed mode from DDraw full screen,
	// you should invalidate the desktop
	// 
	//if (current_window_mode==0)
	//	InvalidateRect(0, 0, 1);	
	
	return result;
}
//---------------------------------------------------------------------
//
void WindowManager::SetCallback (WNDPROC wndProc)
{
	windowCallback = wndProc;
}
//---------------------------------------------------------------------
//
HWND WindowManager::GetWindowHandle (void) const
{
	return hWnd;
}
//---------------------------------------------------------------------
//
void WindowManager::ServeMessageQueue (U32 flags)
{
	//	serve_queue();
	MSG msg;
	BOOL32 bWait;
	
	if (criticalSection)
		EnterCriticalSection(criticalSection);
	if ((flags & WMF_WAIT) != 0)
		bWait = 1;
	else
	if ((flags & WMF_BACKGROUND) == 0)
		bWait = (bAppActive == 0);
	else
		bWait = 0;
	
	//
	// Serve message queue
	//
	
	while (bWait || PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0 ))
		{
			if (criticalSection)
				LeaveCriticalSection(criticalSection);

			exitCallback(msg.wParam);	// pass exit code back to exit chain
			
			//
			// Return statement should not be reached
			//
			
			return;
		}
		
		if ((flags & WMF_DLGMSG)==0 || IsDialogMessage(msg.hwnd, &msg)==0)
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}

		if (flags & WMF_BACKGROUND)
			bWait = 0;
		else
			bWait = (bAppActive == 0);
	}

	if (criticalSection)
		LeaveCriticalSection(criticalSection);
}
//---------------------------------------------------------------------
//
void WindowManager::GetWindowArea (WM_WINAREA & area)
{
	if (showWindowFlags & WMF_FULL_SCREEN)
	{
		area.x = area.y = 0;
		area.w = GetSystemMetrics(SM_CXSCREEN);
		area.h = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		area.x = windowRect.left;
		area.y = windowRect.top;
		area.w = windowRect.right  - windowRect.left + 1;
		area.h = windowRect.bottom - windowRect.top  + 1;
	}
}
//---------------------------------------------------------------------
//
void WindowManager::GetClientArea (WM_WINAREA & area)
{
	if (showWindowFlags & WMF_FULL_SCREEN)
	{
		area.x = area.y = 0;
		area.w = GetSystemMetrics(SM_CXSCREEN);
		area.h = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		area.x = clientRect.left;
		area.y = clientRect.top;
		area.w = clientRect.right - clientRect.left + 1;
		area.h = clientRect.bottom - clientRect.top + 1;
	}
}
//---------------------------------------------------------------------
//
BOOL32 WindowManager::AppIsActive (void) const
{
	return bAppActive;
}
//---------------------------------------------------------------------
//
void WindowManager::SetCursorPos (S32 x, S32 y)
{
	POINT point;
	
	xPosMouse = point.x = x;
	yPosMouse = point.y = y;
	
	if ((showWindowFlags & WMF_FULL_SCREEN) == 0)
		ClientToScreen(hWnd, &point);
	
	::SetCursorPos(point.x, point.y);
}
//---------------------------------------------------------------------
//
void WindowManager::GetCursorPos (S32 & x, S32 & y)
{
 	x = xPosMouse;
 	y = yPosMouse;
}
//---------------------------------------------------------------------
//
void WindowManager::getWindowRects (void)
{
	GetClientRect(hWnd, &clientRect);
	
	ClientToScreen(hWnd, (POINT *) &clientRect.left); 
	ClientToScreen(hWnd, (POINT *) &clientRect.right); 

	clientRect.right--;		
	clientRect.bottom--;	

	GetWindowRect(hWnd, &windowRect);
}
//------------------------------------------------------------------------------
//
BOOL32 WindowManager::resizeTheWindow (S32 display_size_X, S32 display_size_Y, U32 flags)
{
	WM_WINAREA area;                     // Location/size of window client area
	S32      retry;

	//
	// Enable caption menu and user preferences
	//
	
	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP);
	
	SetWindowLong(hWnd, 
		GWL_STYLE, 
		GetWindowLong(hWnd, GWL_STYLE) | (WS_OVERLAPPED  | 
		WS_CAPTION     | 
		WS_SYSMENU     | 
		WS_MINIMIZEBOX));
	
	if (flags & WMF_ALLOW_WINDOW_RESIZE)
	{
		SetWindowLong(hWnd, 
			GWL_STYLE, 
			GetWindowLong(hWnd, GWL_STYLE) | WS_THICKFRAME |
			WS_MAXIMIZEBOX);
	}
	
	if (flags & WMF_ALWAYS_ON_TOP)
	{
		SetWindowLong(hWnd, 
			GWL_EXSTYLE, 
			GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST);
	}
	else
	{
		SetWindowLong(hWnd, 
			GWL_EXSTYLE, 
			GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_TOPMOST);
	}

	//
	// If area not already established, center window's client area on
	// desktop, and size it to correspond to the display size for optimum 
	// performance (no stretching needed)
	//

	//
	// Get desktop size
	//
	
	S32 desktop_w = GetSystemMetrics(SM_CXSCREEN);
	S32 desktop_h = GetSystemMetrics(SM_CYSCREEN);

	GetClientArea(area);
	
	area.w = display_size_X;
	area.h = display_size_Y;
		
	area.x = ((desktop_w - area.w ) / 2);
	area.y = ((desktop_h - area.h) / 2);
	
	//
	// Calculate adjusted position of window
	//
	// Do not allow overall window size to exceed desktop size; keep 
	// dividing height and width by 2 until entire window fits
	//
	// If window is offscreen (or almost entirely offscreen), center it
	//
	
//	do
	{
		retry = 0;
		
		windowRect.left   = area.x;
		windowRect.right  = area.x + area.w - 1;
		windowRect.top    = area.y;
		windowRect.bottom = area.y + area.h - 1;
		
		AdjustWindowRectEx(&windowRect,
			GetWindowLong(hWnd, GWL_STYLE),
			(GetMenu(hWnd) != NULL),
			GetWindowLong(hWnd, GWL_EXSTYLE));
		
		if ((windowRect.right - windowRect.left + 1) > desktop_w)
		{
			area.w >>= 1;
			area.x = ((desktop_w - area.w ) / 2);
			retry = 1;
		}
		
		if ((windowRect.bottom - windowRect.top + 1) > desktop_h)
		{
			area.h >>= 1;
			area.y = ((desktop_h - area.h) / 2);
			retry = 1;
		}
		
		if ((windowRect.left   >= (desktop_w-16)) ||
			(windowRect.top    >= (desktop_h-16)) ||
			(windowRect.right  <= 16)          ||
			(windowRect.bottom <= 16))
		{
			area.x = ((desktop_w - area.w ) / 2);
			area.y = ((desktop_h - area.h) / 2);
			retry = 1;
		}
	}
//	while (retry);
  
	if (retry)
		return 0;
	//
	// Set window size and position
	//
	
	UINT posflags = SWP_NOCOPYBITS | SWP_NOZORDER;
	if ((flags & WMF_CENTER) == 0)
		posflags |= SWP_NOMOVE;

	::SetWindowPos(hWnd, 
		HWND_TOP, 
		windowRect.left,
		windowRect.top,
		windowRect.right  - windowRect.left + 1,
		windowRect.bottom - windowRect.top  + 1,
		posflags);
	
	CQTRACE14("Window at (%d,%d), client size = (%d,%d)\n", windowRect.left, windowRect.top, clientRect.right+1-clientRect.left, clientRect.bottom+1-clientRect.top);

	return 1;
}
//---------------------------------------------------------------------
//
void WindowManager::shutdown (void)
{
	bInitialized = 0;
}
//--------------------------------------------------------------------------
// call clients in reverse order, stop when one returns non-zero.  
//
BOOL32 WindowManager::onClose (CONNECTION_NODE<ISystemEventCallback> *node)
{
	BOOL32 result = 0;

	if (node)
	{
		if ((result = onClose(node->pNext)) == 0)
			result = node->client->OnAppClose();
	}

	return result;
}
//--------------------------------------------------------------------------
//  
BOOL32 WindowManager::onClose (void)
{
	return onClose(point.pClientList);
}
//---------------------------------------------------------------------
//
void __stdcall SetWindowManagerCriticalSection (CRITICAL_SECTION * criticalSection)
{
	if (GlobalWindowManagerSingleton)
		GlobalWindowManagerSingleton->criticalSection = criticalSection;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//
void __stdcall RegisterWindowManager (void)
{
	WindowManager * factory = new WindowManager;
	
	if (factory)
	{
		DACOM->RegisterComponent(factory, interface_name, 0xC0000000);
		factory->Release();
	}
}
//-------------------------------------------------------------------------
//-------------------------End WindowManager.cpp---------------------------
//-------------------------------------------------------------------------




