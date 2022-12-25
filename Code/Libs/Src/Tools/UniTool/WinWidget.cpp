//
// WinWidget.cpp - The implementation of the BaseWidget class
//

//
// Design Notes:
//      The BaseWidget class uses the USERDATA field of its window to store the 'this' pointer.
// Therefore, USERDATA is not availble for use in the inherited classes. YOU HAVE BEEN WARNED!

//
// Include files
//

#include <windows.h>
#include "winwidget.h"
#include "unitool.h"
#include "resource.h"
#include "assert.h"

//
// Static variables
//

static char WIDGETCLASS[] = "UniToolWidgetClass";

//
// Static data members
//

bool BaseWidget::classRegistered = false;

//
// Routines
//

// Local routines

void display_last_error ()
{
	LPVOID lpMsgBuf;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Display the string.
	MessageBox( NULL, (LPCTSTR) lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
} 


// Static methods

struct BWCreateData
{
	SHORT       cbExtra;  // to keep NT happy.
	BaseWidget *widget;
};

typedef BWCreateData UNALIGNED *LPBWCreateData;

LRESULT CALLBACK BaseWidget::widgetProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// During the create, the USERDATA field of the window is set to the value passed in lParam, which
	// is the 'this' pointer for the BaseWidget class that created the window.
	// If this is a WM_CREATE message, attach the lParam value and reissue the create call.
	// Otherwise, retrieve the object pointer and issue to message to it.

	if (uMsg == WM_CREATE)
	{
		LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
		LPBWCreateData lpbwcd = (LPBWCreateData) lpcs->lpCreateParams;
		SetWindowLong (hwnd, GWL_USERDATA, (LONG) lpbwcd->widget);
		if (lpbwcd->widget)
		{
			return lpbwcd->widget->handle_message (hwnd, uMsg, wParam, lParam);
		}
	}
	else
	{
		BaseWidget *w = (BaseWidget *) GetWindowLong(hwnd, GWL_USERDATA);
		if (w)
		{
			LRESULT result = w->handle_message (hwnd, uMsg, wParam, lParam);

			// Ensure that the window handle is nulled out when the window is destroyed, regardless
			// of the overloaded handling.
			if (uMsg == WM_DESTROY)
			{
				w->hBaseWnd = NULL;
			}

			return result;
		}
	}

	// If things failed above, perform default processing.

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void BaseWidget::register_wnd_class()
{
	// If the window class has not already been registered, register it.

	if (!classRegistered)
	{
		WNDCLASSEX wc;
		wc.cbSize = sizeof(wc);
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = (WNDPROC) widgetProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hToolInstance;
		wc.hIcon = LoadIcon( hToolInstance, MAKEINTRESOURCE (IDI_UNITOOL));
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = WIDGETCLASS;
		wc.hIconSm = NULL;
		if (RegisterClassEx (&wc))
		{
			classRegistered = true;
		}
	}
}

// Constructors and Destructors
BaseWidget::BaseWidget()
{
	hBaseWnd = NULL;
	textBuffer = NULL;
	eventHandler = NULL;
	esAppData = 0;
}

BaseWidget::~BaseWidget()
{
	// NOTE: Due to the mysteries of windows message passing, you should always destroy the window
	// before deleting it.

	assert (hBaseWnd == NULL);
	if (textBuffer)
	{
		delete textBuffer;
		textBuffer = NULL;
	}
}

// Message Handlers

LRESULT BaseWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// This is the basic message handler. It dispatches certain messages to the virtual message handlers
	// below.
	// Anything not dispatched gets default message handling.

	switch (uMsg)
	{
	case WM_CREATE:
		return on_create ((LPCREATESTRUCT) lParam);
		break;

	case WM_DESTROY:
		if (on_destroy())
		{
			return 0;
		}
		break;

	case WM_COMMAND:
		if (on_command (HIWORD(wParam), LOWORD(wParam), (HWND) lParam))
		{
			return 0;
		}
		break;

	case WM_SIZE:
		if (on_size (wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_MOVE:
		if (on_move ((int) LOWORD(lParam), (int) HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_LBUTTONDOWN:
		if (on_buttondown(MF_LEFT | MF_DOWN, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_RBUTTONDOWN:
		if (on_buttondown(MF_RIGHT | MF_DOWN, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_MBUTTONDOWN:
		if (on_buttondown(MF_MIDDLE | MF_DOWN, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_LBUTTONUP:
		if (on_buttonup(MF_LEFT | MF_UP, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_RBUTTONUP:
		if (on_buttonup(MF_RIGHT | MF_UP, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_MBUTTONUP:
		if (on_buttonup(MF_MIDDLE | MF_UP, wParam, LOWORD(lParam), HIWORD(lParam)))
		{
			return 0;
		}
		break;

	case WM_MOUSEMOVE:
		on_mousemove (wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_NOTIFY:
		return on_notify((int) wParam, (LPNMHDR) lParam);
		break;

	case WM_PAINT:
		if (on_paint ((HDC) wParam))
		{
			return 0;
		}
		break;

	case WM_ERASEBKGND:
		if (on_erasebkgnd ((HDC) wParam))
		{
			return 1;
		}
		break;
	}

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT BaseWidget::on_create (LPCREATESTRUCT lpcs)
{
	// Return 0 to indicate we handled the message.
	// Returning -1 will abort the creation.

	return 0;
}

bool BaseWidget::on_destroy ()
{
	// Return false, indicating that we didn't handle the message, and that default
	// processing should occur.
	return false;
}

bool BaseWidget::on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
	// If the command is from a menu, feed it to on_menu() and return the result.
	// Likewise with on_accel(). Otherwise, return false, indicating we didn't process the
	// command.

	if (wNotifyCode == 0)
	{
		return on_menu(wID);
	}
	else if (wNotifyCode == 1)
	{
		return on_accel(wID);
	}

	return false;
}

bool BaseWidget::on_menu (WORD wID)
{
	// Return false, indicating we didn't handle the message
	return false;
}

bool BaseWidget::on_accel (WORD wID)
{
	// Return false, indicating we didn't handle the message
	return false;
}

bool BaseWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// Return false, indicating we didn't handle the message
	return false;
}

bool BaseWidget::on_move (int xPos, int yPos)
{
	// Return false, indicating we didn't handle the message
	return false;
}

bool BaseWidget::on_buttondown (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Return false, indicating we didn't handle the message
	return false;
}

bool BaseWidget::on_buttonup (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Return false, indicating we didn't handle the message
	return false;
}

void BaseWidget::on_mousemove (WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Do nothing.
	return;
}

LRESULT BaseWidget::on_notify (int idCtrl, LPNMHDR pnmh)
{
	// By default, we return 0, since most notification returns are ignored.
	// *** What is the proper way of doing this?
	return 0;
}

bool BaseWidget::on_paint (HDC hdc)
{
	// Return false, indicating we didn't handle the message.
	return false;
}

bool BaseWidget::on_erasebkgnd (HDC hdc)
{
	// Return false, indicating we didn't handle the message.
	return false;
}

//
// Methods
//

// Creation and destruction methods
bool BaseWidget::create(DWORD exStyle, DWORD style, HWND parent, int w, int h)
{
	assert (hBaseWnd == NULL);

	// Register the window class if not already registered.
	register_wnd_class();
	
	// Create a widget window with the given style, width, and height.

	BWCreateData bwcd;
	bwcd.cbExtra = sizeof(bwcd);
	bwcd.widget = this;

	hBaseWnd = 
		CreateWindowEx
		(
			exStyle,
			WIDGETCLASS,
			"",
			style,
			CW_USEDEFAULT, 0,
			w, h,
			parent,
			NULL, // no menu, for now
			hToolInstance,
			(LPVOID) &bwcd
		);

	if (!hBaseWnd)
	{
		display_last_error();
	}

	return (hBaseWnd != NULL);
}

bool BaseWidget::destroy()
{
	if (hBaseWnd == NULL)
	{
		// This is legal, and concidered success.
		return true;
	}

	// Destroy the base window.

	if (DestroyWindow(hBaseWnd))
	{
		hBaseWnd = NULL;
		return true;
	}

	// We failed, for some reason.
	return false;
}

// Size and position manipulation functions
void BaseWidget::set_position (int x, int y)
{
	if (hBaseWnd)
	{
		SetWindowPos (hBaseWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}
}

POINT BaseWidget::get_position()
{
	POINT p;
	p.x = p.y = 0;
	if (hBaseWnd)
	{
		RECT r;
		GetWindowRect (hBaseWnd, &r);
		p.x = r.left;
		p.y = r.top;

		// URGH! The window rect of a window is in screen coordinates, not in parent client coordinates.
		ScreenToClient (GetParent(hBaseWnd), &p);
	}
	return p;
}

void BaseWidget::set_size (int w, int h)
{
	if (hBaseWnd)
	{
		SetWindowPos (hBaseWnd, NULL, 0, 0, w, h, SWP_NOZORDER | SWP_NOMOVE);
	}
}

SIZE BaseWidget::get_size()
{
	SIZE s;
	s.cx = s.cy = 0;
	if (hBaseWnd)
	{
		RECT r;
		GetWindowRect (hBaseWnd, &r);
		s.cx = r.right - r.left;
		s.cy = r.bottom - r.top;
	}
	return s;
}

// Style manipulation
void BaseWidget::set_style (DWORD newStyle)
{
	if (hBaseWnd)
	{
		SetWindowLong (hBaseWnd, GWL_STYLE, newStyle);
	}
}

void BaseWidget::set_exstyle (DWORD newExStyle)
{
	if (hBaseWnd)
	{
		SetWindowLong (hBaseWnd, GWL_EXSTYLE, newExStyle);
	}
}

DWORD BaseWidget::get_style()
{
	DWORD result = 0;
	if (hBaseWnd)
	{
		result = GetWindowLong (hBaseWnd, GWL_STYLE);
	}
	return result;
}

DWORD BaseWidget::get_exstyle()
{
	DWORD result = 0;
	if (hBaseWnd)
	{
		result = GetWindowLong (hBaseWnd, GWL_EXSTYLE);
	}
	return result;
}

// Text manipulation
const char *BaseWidget::get_text()
{
	// Free the current text buffer, if any.
	if (textBuffer)
	{
		delete textBuffer;
		textBuffer = NULL;
	}

	if (hBaseWnd)
	{
		int len = GetWindowTextLength (hBaseWnd);
		if (len)
		{
			textBuffer = new char[len+1];
			if (textBuffer)
			{
				GetWindowText (hBaseWnd, textBuffer, len+1);
				textBuffer[len] = '\0';
			}
		}
	}
	return textBuffer;
}

void BaseWidget::set_text(const char *newText)
{
	if (hBaseWnd)
	{
		SetWindowText (hBaseWnd, newText);
		get_text();
	}
}

// Visibility methods
void BaseWidget::set_visible(bool makeVisible)
{
	if (hBaseWnd)
	{
		ShowWindow (hBaseWnd, makeVisible ? SW_SHOW : SW_HIDE);
	}
}

bool BaseWidget::is_visible()
{
	bool result = false;
	if (hBaseWnd)
	{
		result = (IsWindowVisible(hBaseWnd) == TRUE);
	}
	return result;
}

// Enabling methods
void BaseWidget::set_enabled(bool makeEnabled)
{
	if (hBaseWnd)
	{
		EnableWindow (hBaseWnd, makeEnabled ? true : false);
	}
}

bool BaseWidget::is_enabled()
{
	bool result = false;
	if (hBaseWnd)
	{
		result = (IsWindowEnabled (hBaseWnd) == TRUE);
	}
	return result;
}

//
// Interface methods
//
// IDontKnow interface

bool BaseWidget::query_interface (const char *name, void **iface)
{
	if
	(
		!strcmp ("IScriptable", name) ||
		!strcmp ("IEventSource", name) ||
		!strcmp ("IWidget", name) ||
		!strcmp ("IDontKnow", name)
	)
	{
		*iface = this;
		return true;
	}

	return false;
}

// IWidget Interface

HWND BaseWidget::get_hwnd()
{
	return hBaseWnd;
}

// NOTE: By default, the window wants to stay its current size, so
// these routines all return the current size of the window.
SIZE BaseWidget::get_minsize()
{
	SIZE s;
	if (hBaseWnd)
	{
		s = get_size();
	}
	else
	{
		s.cx = 0;
		s.cy = 0;
	}
	return s;
}

SIZE BaseWidget::get_maxsize()
{
	SIZE s;
	if (hBaseWnd)
	{
		s = get_size();
	}
	else
	{
		s.cx = 0;
		s.cy = 0;
	}
	return s;
}

SIZE BaseWidget::get_prefsize()
{
	SIZE s;
	if (hBaseWnd)
	{
		s = get_size();
	}
	else
	{
		s.cx = 0;
		s.cy = 0;
	}
	return s;
}

bool BaseWidget::fire_event (EventId event, int pCount, Variant *params)
{
	if (eventHandler)
	{
		eventHandler->handle_event(event, this, pCount, params);
		return true;
	}

	return false;
}

// IScriptable Interface, inherited from IWidget

// Structures and macros that help to export invocation interfaces.
struct MethodDef
{
	const char *      name;
	int               specLen;
	const ParamSpec * spec;
};

#define METHOD_SPEC_BEGIN(name, count, retSpec) static ParamSpec methodSpec_ ## name [(count)+1] = {{retSpec, 0},
#define METHOD_SPEC_ENTRY(type) {type, 0}
#define METHOD_SPEC_END(name, count) }; static MethodDef methodDef_ ## name = {#name, (count)+1, methodSpec_ ## name};

#define METHOD_TABLE_START(widgetName) static MethodDef *methods_ ## widgetName [] = {
#define METHOD_DEF(name) &(methodDef_ ## name)
#define METHOD_TABLE_END() };

#define METHOD_COUNT(widgetName) (sizeof(methods_ ## widgetName)/sizeof(MethodDef *))
#define METHOD_DATA(widgetName, index) (methods_ ## widgetName [index])

METHOD_SPEC_BEGIN(set_size, 2, PS_VOID)
METHOD_SPEC_ENTRY(PS_INT),
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(set_size, 2)

METHOD_SPEC_BEGIN(get_width, 0, PS_INT)
METHOD_SPEC_END(get_width, 0)

METHOD_SPEC_BEGIN(get_height, 0, PS_INT)
METHOD_SPEC_END(get_height, 0)

METHOD_SPEC_BEGIN(get_x, 0, PS_INT)
METHOD_SPEC_END(get_x, 0)

METHOD_SPEC_BEGIN(get_y, 0, PS_INT)
METHOD_SPEC_END(get_y, 0)

METHOD_SPEC_BEGIN(set_position, 2, PS_VOID)
METHOD_SPEC_ENTRY(PS_INT),
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(set_position, 2)

METHOD_SPEC_BEGIN(get_text, 0, PS_STRING)
METHOD_SPEC_END(get_text, 0)

METHOD_SPEC_BEGIN(set_text, 1, PS_VOID)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(set_text, 1)

METHOD_SPEC_BEGIN(get_visible, 0, PS_BOOL)
METHOD_SPEC_END(get_visible, 0)

METHOD_SPEC_BEGIN(set_visible, 1, PS_VOID)
METHOD_SPEC_ENTRY(PS_BOOL)
METHOD_SPEC_END(set_visible, 1)

METHOD_SPEC_BEGIN(get_enabled, 0, PS_BOOL)
METHOD_SPEC_END(get_enabled, 0)

METHOD_SPEC_BEGIN(set_enabled, 1, PS_VOID)
METHOD_SPEC_ENTRY(PS_BOOL)
METHOD_SPEC_END(set_enabled, 1)

METHOD_SPEC_BEGIN(get_prefwidth, 0, PS_INT)
METHOD_SPEC_END(get_prefwidth, 0)

METHOD_SPEC_BEGIN(get_prefheight, 0, PS_INT)
METHOD_SPEC_END(get_prefheight, 0)


METHOD_TABLE_START(BaseWidget)
METHOD_DEF(set_size),
METHOD_DEF(get_width),
METHOD_DEF(get_height),
METHOD_DEF(set_position),
METHOD_DEF(get_x),
METHOD_DEF(get_y),
METHOD_DEF(set_text),
METHOD_DEF(get_text),
METHOD_DEF(set_visible),
METHOD_DEF(get_visible),
METHOD_DEF(set_enabled),
METHOD_DEF(get_enabled),
METHOD_DEF(get_prefwidth),
METHOD_DEF(get_prefheight)
METHOD_TABLE_END()

// The basic window implements no methods, for now. Eventually, it will export basic window
// manipulation methods (size, position, visibility, text, etc.)
int BaseWidget::method_count(void)
{
//	return 0;
	return METHOD_COUNT(BaseWidget);
}

const char *BaseWidget::method_name(int index)
{
	if (index < METHOD_COUNT(BaseWidget))
	{
		return METHOD_DATA(BaseWidget, index)->name;
	}
	return NULL;
}

int BaseWidget::method_speclen(int index)
{
	if (index < METHOD_COUNT(BaseWidget))
	{
		return METHOD_DATA(BaseWidget, index)->specLen;
	}
	return 0;
}

const ParamSpec *BaseWidget::method_spec (int index)
{
	if (index < METHOD_COUNT(BaseWidget))
	{
		return METHOD_DATA(BaseWidget, index)->spec;
	}
	return NULL;
}

bool BaseWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	int i;
	for (i = 0; i < METHOD_COUNT(BaseWidget); ++i)
	{
		if (!strcmp (methodName, METHOD_DATA(BaseWidget, i)->name))
		{
			return invokeByIndex (i, result, paramCount, params);
		}
	}
	return false;
}

bool BaseWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	// This is the only thing we can't do with tables or macros.
	switch (index)
	{
	case 0: // set_size
		if (paramCount >= 2)
		{
			if (params[0].spec.type == PS_INT && params[1].spec.type == PS_INT)
			{
				set_size (params[0].iVal, params[1].iVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 1: // get_width
		{
			SIZE s = get_size();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = s.cx;
			return true;
		}
		break;

	case 2: // get_height
		{
			SIZE s = get_size();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = s.cy;
			return true;
		}
		break;

	case 3: // set_position
		if (paramCount >= 2)
		{
			if (params[0].spec.type == PS_INT && params[1].spec.type == PS_INT)
			{
				set_position (params[0].iVal, params[1].iVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 4: // get_x
		{
			POINT p = get_position();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = p.x;
			return true;
		}
		break;

	case 5: // get_y
		{
			POINT p = get_position();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = p.y;
			return true;
		}
		break;

	case 6: // set_text
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				set_text (params[0].sVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 7: // get_text
		{
			const char *text = get_text();
			result->spec.type = PS_STRING;
			result->spec.tag = 0;
			result->sVal = text;
			return true;
		}
		break;

	case 8: // set_visible
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_BOOL)
			{
				set_visible (params[0].bVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 9: // get_visible
		{
			result->spec.type = PS_BOOL;
			result->spec.tag = 0;
			result->bVal = is_visible();
			return true;
		}
		break;

	case 10: // set_enabled
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_BOOL)
			{
				set_enabled (params[0].bVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 11: // get_enabled
		{
			result->spec.type = PS_BOOL;
			result->spec.tag = 0;
			result->bVal = is_enabled();
			return true;
		}
		break;

	case 12: // get_prefwidth
		{
			SIZE s = get_prefsize();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = s.cx;
			return true;
		}
		break;

	case 13: // get_prefheight
		{
			SIZE s = get_prefsize();
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = s.cy;
			return true;
		}
		break;
	}
	return false;
}

// IEventSource Interface, inherited from IWidget
// For now, there are no events sent by the base class. Eventually, this base class will send size
// and position change events, maybe some mouse events as well.
// The base class DOES implement the storage and setting of the event handler and application data,
// however.

EventId BaseWidget::event_count(void)
{
	return 0;
}

const char *BaseWidget::event_name(EventId index)
{
	return NULL;
}

void BaseWidget::set_app_data (DWORD data)
{
	esAppData = data;
}

DWORD BaseWidget::get_app_data ()
{
	return esAppData;
}

IEventSink *BaseWidget::set_event_handler (IEventSink *sink)
{
	IEventSink *old = eventHandler;
	eventHandler = sink;
	return old;
}
