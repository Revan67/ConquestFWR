//
// StdWidget.cpp - Standard widgets supplied by UniTool
//

//
// Design Notes:
//        UniTool provides a set of common widgets which allow UniTool to be useful
// immediately, before any plugins are available.
//        The standard widget set includes:
//   TopLevel - A top level window widget. All other widgets must have a TopLevel somewhere in their
// parent chain.
//   Frame - A widget intended to contain other widgets
//   Button - Implements a pushbutton
//   Label - A widget displaying non-editable text
//   Text - A widget into which text can be entered.
//   Image - A widget displaying a bitmapped image
//   Scroll - A widget implementing a scrollbar
//   List - A widget implementing a standard windows list box
//   Combo - A widget implementing a standard windows combo box
//

// *** TODO:
// 1) Figure out a good way of exporting invokable functions that also takes into account inherited
//    methods.
// 2) Split these up into seperate files for each widget, since the code has become a bit long.
// 3) Add more events to each of the widgets.

//
// Include files
//

#include <windows.h>
#include <assert.h>
#include "StdWidget.h"
#include "script.h"
#include "exportdef.h"

//
// Imported variables
//
extern HWND hWndMain;  // in unitool.cpp

//
// Constants
//

const int BUTTON_BORDER = 5;  // amount of button bordered around its text to achieve the perferred dimensions.
const int LABEL_BORDER = 0;   // amount of lable bordered around its text to achieve the preferred dimensions.

//
// Class and structure definitions
//

// =========== Button Widget =========== 
// *** Events ***
const EventId BUTTON_PRESSED = 0;
const EventId BUTTON_EVENT_COUNT = 1;

// *** Methods *** 

ButtonWidget::ButtonWidget()
{
	buttonWnd = NULL;
}

ButtonWidget::~ButtonWidget()
{
	// There is no need to destroy the buttonWnd; it will be destroyed as a child of
	// hBaseWnd.
}
	
// Creation methods
bool ButtonWidget::create (int w, int h, const char *text, HWND parent, ButtonType type)
{
	// Create the base window first, then create buttonWnd as a child of it.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the button window as a child of the base window.
	DWORD style;
	switch (type)
	{
	case BTN_PUSH:
		style = BS_PUSHBUTTON;
		break;

	case BTN_RADIO:
		style = BS_RADIOBUTTON;
		break;

	case BTN_CHECK:
		style = BS_CHECKBOX;
		break;

	default:
		assert (false && "Invalid button type");
		break;
	}

	buttonWnd =
		CreateWindow
		(
			"BUTTON",
			text,
			WS_CHILD | WS_VISIBLE | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!buttonWnd)
	{
		return false;
	}

	// The button has been created, so do any other initialization here and return true.

	return true;
}

// BaseWidget overloads
LRESULT ButtonWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to buttonWnd!
	switch (uMsg)
	{
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
		// Reflect text messages to the button window, bypassing default behavior
		return SendMessage (buttonWnd, uMsg, wParam, lParam);
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the button window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (buttonWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

bool ButtonWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the button as well.
	if (buttonWnd)
	{
		SetWindowPos (buttonWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

bool ButtonWidget::on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
	// Send an event when this button is pressed.
	if (hwndCtl == buttonWnd && wNotifyCode == BN_CLICKED)
	{
		if (fire_event (BUTTON_PRESSED))
		{
			return true;
		}
	}

	// Perform the inherited behavior
	return BaseWidget::on_command(wNotifyCode, wID, hwndCtl);
}

// *** Supported Interfaces ***

// IWidget Interface
SIZE ButtonWidget::get_prefsize()
{
	if (buttonWnd)
	{
		// The preferred size of a button is slightly larger than its current text.
		SIZE s;

		HDC hDC = GetDC (buttonWnd);
		
		const char *text = get_text();
		BOOL good = GetTextExtentPoint32(hDC, text, strlen(text), &s);
		ReleaseDC (buttonWnd, hDC);

		if (good)
		{
			s.cx += BUTTON_BORDER * 2;
			s.cy += BUTTON_BORDER * 2;
			return s;
		}
	}
	return BaseWidget::get_prefsize();
}

// IEventSource Interface
EventId ButtonWidget::event_count(void)
{
	// We only generate a single event for now, the button pressed event
	return BUTTON_EVENT_COUNT;
}

const char *ButtonWidget::event_name(EventId index)
{
	// We only generate a single event for buttons, the button pressed event
	static char *eventNames[BUTTON_EVENT_COUNT] =
	{
		"OnPress"
	};
	if (index < 0 || index >= BUTTON_EVENT_COUNT)
	{
		return NULL;
	}
	return eventNames[index];
}

// Button creation function, exported to lua

static void newButton (void)
{
	// Syntax: 
	//     NewButton
	//     (
	//         Window <parent>,
	//         string <text>,
	//         number <xpos>, number <ypos>,
	//         [number <width>, number <height>]
	//     )
	// Create a button and returns its HWND as an object.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button
	// If width and height are not specified, the button will be sized to its initial text

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object text = lua_getparam(2);
	lua_Object xpos = lua_getparam(3);
	lua_Object ypos = lua_getparam(4);
	lua_Object width = lua_getparam(5);
	lua_Object height = lua_getparam(6);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(text))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}

	bool defWidth = false;
	bool defHeight = false;

	int w, h;

	// The width and height are optional. For those that are nil, use the preferred dimension.
	if (width == LUA_NOOBJECT || lua_isnil(width))
	{
		defWidth = true;
		w = 1;
	}
	else if (lua_isnumber(width))
	{
		w = (int) lua_getnumber(width);
	}
	else
	{
		// Invalid value for width. Abort.
		return;
	}

	if (height == LUA_NOOBJECT || lua_isnil(height))
	{
		defHeight = true;
		h = 1;
	}
	else if (lua_isnumber(height))
	{
		h = (int) lua_getnumber(height);
	}
	else
	{
		// Invalid value for height. Abort.
		return;
	}

	// The parameters are valid, so go about creating a new button.

	HWND hParent;
	if (lua_isnil(parent))
	{
		hParent = NULL;
	}
	else if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent tag!\n");
		return;
	}

	ButtonWidget *button = new ButtonWidget;
	assert (button != NULL && "Failed to allocate a new button.");

	if (button->create (w, h, lua_getstring(text), hParent))
	{
		// Position the child at the given location.
		button->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Adjust default dimensions according to the preferred dimensions.
		if (defWidth || defHeight)
		{
			SIZE s = button->get_prefsize();
			if (defWidth) { w = s.cx; }
			if (defHeight) { h = s.cy; }
			button->set_size (w, h);
		}

		// Finally, export the widget.
		export_widget (button);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============ Label Widget ==============

// *** Methods ***

LabelWidget::LabelWidget()
{
	labelWnd = NULL;
}

LabelWidget::~LabelWidget()
{
	// There is no need to destroy the labelWnd; it will be destroyed as a child of
	// hBaseWnd.
}

// Creation methods
bool LabelWidget::create (int w, int h, const char *text, HWND parent)
{
	// Create the base window first, then create labelWnd as a child of it.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the label window as a child of the base window.
	DWORD style = 0;

	labelWnd =
		CreateWindow
		(
			"STATIC",
			text,
			WS_CHILD | WS_VISIBLE | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!labelWnd)
	{
		return false;
	}

	// The button has been created, so do any other initialization here and return true.

	return true;
}

// BaseWidget overloads

LRESULT LabelWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to labelWnd!
	switch (uMsg)
	{
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
		// Reflect text messages to the button window, bypassing default behavior
		return SendMessage (labelWnd, uMsg, wParam, lParam);
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the button window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (labelWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

bool LabelWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the label as well.
	if (labelWnd)
	{
		SetWindowPos (labelWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

// *** Supported Interfaces ***

// IWidget Interface
SIZE LabelWidget::get_prefsize()
{
	if (labelWnd)
	{
		// The preferred size of a button is slightly larger than its current text.
		SIZE s;

		HDC hDC = GetDC (labelWnd);
		
		const char *text = get_text();
		BOOL good = GetTextExtentPoint32(hDC, text, strlen(text), &s);
		ReleaseDC (labelWnd, hDC);

		if (good)
		{
			s.cx += LABEL_BORDER * 2;
			s.cy += LABEL_BORDER * 2;
			return s;
		}
	}
	return BaseWidget::get_prefsize();
}

// Label creation function, exported to lua

static void newLabel (void)
{
	// Syntax: 
	//     NewLabel
	//     (
	//         Window <parent>,
	//         string <text>,
	//         number <xpos>, number <ypos>,
	//         [number <width>, number <height>]
	//     )
	// Create a button and returns its HWND as an object.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button
	// If width and height are not specified, the label will default to the size of its initial text.

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object text = lua_getparam(2);
	lua_Object xpos = lua_getparam(3);
	lua_Object ypos = lua_getparam(4);
	lua_Object width = lua_getparam(5);
	lua_Object height = lua_getparam(6);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(text))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}

	bool defWidth = false;
	bool defHeight = false;

	int w, h;

	// The width and height are optional. For those that are nil, use the preferred dimension.
	if (width == LUA_NOOBJECT || lua_isnil(width))
	{
		defWidth = true;
		w = 1;
	}
	else if (lua_isnumber(width))
	{
		w = (int) lua_getnumber(width);
	}
	else
	{
		// Invalid value for width. Abort.
		return;
	}

	if (height == LUA_NOOBJECT || lua_isnil(height))
	{
		defHeight = true;
		h = 1;
	}
	else if (lua_isnumber(height))
	{
		h = (int) lua_getnumber(height);
	}
	else
	{
		// Invalid value for height. Abort.
		return;
	}

	// The parameters are valid, so go about creating a new button.

	HWND hParent;
	if (lua_isnil(parent))
	{
		hParent = NULL;
	}
	else if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent tag!\n");
		return;
	}

	LabelWidget *label = new LabelWidget;
	assert (label != NULL && "Failed to allocate a new label.");

	if (label->create (w, h, lua_getstring(text), hParent))
	{
		// Position the child at the given location.
		label->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Adjust default dimensions according to the preferred dimensions.
		if (defWidth || defHeight)
		{
			SIZE s = label->get_prefsize();
			if (defWidth) { w = s.cx; }
			if (defHeight) { h = s.cy; }
			label->set_size (w, h);
		}

		// Finally, export the widget.
		export_widget (label);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============= Image Widget ==============

// *** Methods ***
ImageWidget::ImageWidget()
{
	imageWnd = NULL;
}

ImageWidget::~ImageWidget()
{
	// There is no need to destroy the imageWnd; it will be destroyed as a child of
	// hBaseWnd.
}

// Creation methods
bool ImageWidget::create (int w, int h, const char *image, HWND parent)
{
	// Create the base window first, then create labelWnd as a child of it.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the label window as a child of the base window.
	DWORD style = SS_BITMAP | SS_CENTERIMAGE;

	imageWnd =
		CreateWindow
		(
			"STATIC",
			NULL,
			WS_CHILD | WS_VISIBLE | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!imageWnd)
	{
		return false;
	}

	// The control has been created. Set the image, if any.
	set_image (image);

	return true;
}

// Image manipulation methods

bool ImageWidget::set_image (const char *filename)
{
	// Attempt to load the given bitmap.
	if (filename)
	{
		HANDLE hNewImage = LoadImage (NULL, filename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
		return set_image (hNewImage);
	}
	return false;
}

bool ImageWidget::set_image (HANDLE hNewImage)
{
	if (imageWnd && hNewImage)
	{
		// Set this image as the current one, then free the old image, if any.
		HANDLE hOldImage = (HANDLE) SendMessage (imageWnd, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hNewImage);

		if (hOldImage)
		{
			// Free the old image.
			DeleteObject ((HGDIOBJ) hOldImage);
		}

		return true;
	}
	return false;
}

HANDLE ImageWidget::get_image ()
{
	if (imageWnd)
	{
		return (HANDLE) SendMessage (imageWnd, STM_GETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) 0);
	}
	return (HANDLE) NULL;
}

// BaseWidget overloads

bool ImageWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the image as well.
	if (imageWnd)
	{
		SetWindowPos (imageWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

// *** Supported Interfaces ***

// IWidget Interface

SIZE ImageWidget::get_prefsize()
{
	if (imageWnd)
	{
		// The preferred size of an image is exactly the size of its image.
		SIZE s;
		HANDLE hImage = get_image();
		if (hImage)
		{
			DIBSECTION ds;
			memset (&ds, 0, sizeof(ds));
			if (GetObject ((HGDIOBJ) hImage, sizeof(DIBSECTION), &ds))
			{
				s.cx = ds.dsBm.bmWidth;
				s.cy = ds.dsBm.bmHeight;
				return s;
			}
		}
	}
	return BaseWidget::get_prefsize();
}

// IScriptable Interface

int ImageWidget::method_count(void)
{
	return BaseWidget::method_count() + 1;
}

const char *ImageWidget::method_name(int index)
{
	if (index >= BaseWidget::method_count())
	{
		return "set_image";
	}
	else
	{
		return BaseWidget::method_name (index);
	}
}

int ImageWidget::method_speclen(int index)
{
	if (index >= BaseWidget::method_count())
	{
		return 2;
	}
	else
	{
		return BaseWidget::method_speclen (index);
	}
}

const ParamSpec *ImageWidget::method_spec (int index)
{
	if (index >= BaseWidget::method_count())
	{
		static ParamSpec spec[] = 
		{
			{PS_VOID, 0},
			{PS_STRING, 0}
		};
		return spec;
	}
	else
	{
		return BaseWidget::method_spec (index);
	}
}

bool ImageWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	if (!strcmp (methodName, "set_image"))
	{
		return invokeByIndex (BaseWidget::method_count(), result, paramCount, params);
	}
	else
	{
		return BaseWidget::invoke (methodName, result, paramCount, params);
	}
}

bool ImageWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	if (index >= BaseWidget::method_count())
	{
		if (paramCount >= 1 && params[0].spec.type == PS_STRING)
		{
			set_image (params[0].sVal);
			result->spec.type = PS_VOID;
			return true;
		}
		return false;
	}
	return BaseWidget::invokeByIndex (index, result, paramCount, params);
}

// Image creation function, exported to lua

static void newImage (void)
{
	// Syntax: 
	//     NewImage
	//     (
	//         Window <parent>,
	//         string <image filename>,
	//         number <xpos>, number <ypos>,
	//         [number <width>, number <height>]
	//     )
	// Create a button and returns its HWND as an object.
	// <parent> is the parent window, nil for none
	// <image filename> is the name of the .BMP file to display
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the image, -1 to use the actual image's value

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object imageName = lua_getparam(2);
	lua_Object xpos = lua_getparam(3);
	lua_Object ypos = lua_getparam(4);
	lua_Object width = lua_getparam(5);
	lua_Object height = lua_getparam(6);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(imageName))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}

	bool defWidth = false;
	bool defHeight = false;

	int w, h;

	// The width and height are optional. For those that are nil, use the preferred dimension.
	if (width == LUA_NOOBJECT || lua_isnil(width))
	{
		defWidth = true;
		w = 1;
	}
	else if (lua_isnumber(width))
	{
		w = (int) lua_getnumber(width);
	}
	else
	{
		// Invalid value for width. Abort.
		return;
	}

	if (height == LUA_NOOBJECT || lua_isnil(height))
	{
		defHeight = true;
		h = 1;
	}
	else if (lua_isnumber(height))
	{
		h = (int) lua_getnumber(height);
	}
	else
	{
		// Invalid value for height. Abort.
		return;
	}

	// The parameters are valid, so go about creating a new button.

	HWND hParent;
	if (lua_isnil(parent))
	{
		hParent = NULL;
	}
	else if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent tag!\n");
		return;
	}

	ImageWidget *image = new ImageWidget;
	assert (image != NULL && "Failed to allocate a new image widget.");

	if (image->create (w, h, lua_getstring(imageName), hParent))
	{
		// Position the child at the given location.
		image->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Adjust default dimensions according to the preferred dimensions.
		if (defWidth || defHeight)
		{
			SIZE s = image->get_prefsize();
			if (defWidth) { w = s.cx; }
			if (defHeight) { h = s.cy; }
			image->set_size (w, h);
		}

		// Finally, export the widget.
		export_widget (image);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============= TopLevel Widget =============
// *** Events ***
// *** TODO: Add more events here
// *** Events: mouse movement [What else?]
const EventId TOPLEVEL_SIZED = 0;
const EventId TOPLEVEL_BUTTONDOWN = 1;
const EventId TOPLEVEL_BUTTONUP = 2;
const EventId TOPLEVEL_MOUSEMOVE = 3;
const EventId TOPLEVEL_EVENT_COUNT = 4;

// *** Methods ***
// Constructors and Destructors
TopLevelWidget::TopLevelWidget ()
{
	// Nothing to do, for now
}

TopLevelWidget::~TopLevelWidget ()
{
	// Nothing to do, for now
}

// Creation methods
bool TopLevelWidget::create (int x, int y, int w, int h, const char *title)
{
	// There is no child window for this widget; it only creates the base window.
	RECT rect;

	rect.top = rect.left = 0;
	rect.right = w;
	rect.bottom = h;

	DWORD style = 
		WS_VISIBLE |
		WS_OVERLAPPEDWINDOW |
		WS_CLIPCHILDREN;

	DWORD exStyle = WS_EX_OVERLAPPEDWINDOW;

	AdjustWindowRectEx 
		(
			&rect,
			style,
			FALSE,
			exStyle
		);

	if (!BaseWidget::create(exStyle, style, NULL, rect.right - rect.left, rect.bottom - rect.top))
	{
		return false;
	}

	// For some reason, the WS_VISIBLE style doesn't work with WS_OVERLAPPEDWINDOW.
	// Show it explicitly here.
	ShowWindow (hBaseWnd, SW_SHOW);

	// The base window is created.
	// Move it to the given location and set its title.
	set_position (x, y);
	set_text (title);

	// All is well. Return success.
	return true;
}

HMENU TopLevelWidget::get_menu ()
{
	if (hBaseWnd)
	{
		return GetMenu(hBaseWnd);
	}
	return NULL;
}

void TopLevelWidget::set_menu (HMENU hNewMenu)
{
	if (hBaseWnd)
	{
		// NOTE: Ensure that the client stays the same size when the menu is added.
		SIZE s = get_size();
		SetMenu(hBaseWnd, hNewMenu);
		set_size (s.cx, s.cy);

		// Tell this window that it needs to update its menu bar. [This is so stupid!]
		DrawMenuBar (hBaseWnd);
	}
}

// BaseWidget overloads
bool TopLevelWidget::on_menu (WORD wID)
{
	if (wID >= MENU_BASE_ID)
	{
		MENUITEMINFO mi;
		memset (&mi, 0, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.fMask = MIIM_DATA;
		if (GetMenuItemInfo (get_menu(), wID, FALSE, &mi))
		{
			lua_beginblock();
			// Get the action object associated with this menu.
			// We check to see if it is a function because it might be the nil
			// object, which indicates that the menu has no function.
			lua_Object action = lua_getref(mi.dwItemData);
			if (lua_isfunction(action))
			{
				lua_callfunction(action);
			}
			lua_endblock();
			return true;
		}
	}

	return false;
}

bool TopLevelWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// Send an event, then pass the message on the default handler.
	fire_event (TOPLEVEL_SIZED);
	return BaseWidget::on_size(fwSizeType, nWidth, nHeight);
}

static void encode_mouse_keys (char *buffer, WPARAM fwKeys)
{
	char *here = buffer;
	*here = ' ';
	if (fwKeys & MK_CONTROL)
	{
		*here = 'c';
	}
	*++here = ' ';
	if (fwKeys & MK_SHIFT)
	{
		*here = 's';
	}
	*++here = ' ';
	if (fwKeys & MK_LBUTTON)
	{
		*here = 'l';
	}
	*++here = ' ';
	if (fwKeys & MK_MBUTTON)
	{
		*here = 'm';
	}
	*++here = ' ';
	if (fwKeys & MK_RBUTTON)
	{
		*here = 'r';
	}
	*++here = '\0';
}

bool TopLevelWidget::on_buttondown (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Fire this event.
	char buffer[8];
	encode_mouse_keys (buffer, fwKeys);
	Variant params[3] = {buffer, xPos, yPos};
	fire_event (TOPLEVEL_BUTTONDOWN, 3, params);

	// Return false to enable the default processing of the message
	return false;
}

bool TopLevelWidget::on_buttonup (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Fire this event.
	char buffer[8];
	encode_mouse_keys (buffer, fwKeys);
	Variant params[3] = {buffer, xPos, yPos};
	fire_event (TOPLEVEL_BUTTONUP, 3, params);

	// Return false to enable the default processing of the message
	return false;
}

void TopLevelWidget::on_mousemove (WPARAM fwKeys, WORD xPos, WORD yPos)
{
	// Fire this event.
	char buffer[8];
	encode_mouse_keys (buffer, fwKeys);
	Variant params[3] = {buffer, xPos, yPos};
	fire_event (TOPLEVEL_MOUSEMOVE, 3, params);
}

void TopLevelWidget::set_size (int w, int h)
{
	// Size refers to the client rect, not the window rect.
	if (hBaseWnd)
	{
		DWORD style, exStyle;
		style = (DWORD) GetWindowLong (hBaseWnd, GWL_STYLE);
		exStyle = (DWORD) GetWindowLong (hBaseWnd, GWL_EXSTYLE);
		BOOL hasMenu = (GetMenu (hBaseWnd) != NULL);

		RECT rect;

		rect.top = rect.left = 0;
		rect.right = w;
		rect.bottom = h;

		AdjustWindowRectEx (&rect, style, hasMenu, exStyle);
		BaseWidget::set_size (rect.right - rect.left, rect.bottom - rect.top);
	}
}

SIZE TopLevelWidget::get_size()
{
	// Get the client rect instead of the window rect.
	SIZE s;
	s.cx = s.cy = 0;
	if (hBaseWnd)
	{
		RECT r;
		GetClientRect (hBaseWnd, &r);
		s.cx = r.right - r.left;
		s.cy = r.bottom - r.top;
	}
	return s;
}

// *** Supported Interfaces ***

// IEventSource Interface, inherited from IWidget

EventId TopLevelWidget::event_count(void)
{
	return TOPLEVEL_EVENT_COUNT;
}

const char *TopLevelWidget::event_name(EventId index)
{
	static char *eventNames[TOPLEVEL_EVENT_COUNT] = 
	{
		"OnSize",
		"OnMouseDown",
		"OnMouseUp",
		"OnMouseMove"
	};
	if (index < 0 || index >= TOPLEVEL_EVENT_COUNT)
	{
		return NULL;
	}
	return eventNames[index];
}

// IScriptable Interface, inherited from IWidget

int TopLevelWidget::method_count(void)
{
	return BaseWidget::method_count() + 2;
}

static const char *tlw_names[] = {"get_menu", "set_menu"};
const char *TopLevelWidget::method_name(int index)
{
	if (index >= BaseWidget::method_count())
	{
		return tlw_names[index-BaseWidget::method_count()];
	}
	else
	{
		return BaseWidget::method_name (index);
	}
}

int TopLevelWidget::method_speclen(int index)
{
	if (index >= BaseWidget::method_count())
	{
		static int counts[] = {1, 2};
		return counts[index-BaseWidget::method_count()];
	}
	else
	{
		return BaseWidget::method_speclen (index);
	}
}

const ParamSpec *TopLevelWidget::method_spec (int index)
{
	if (index >= BaseWidget::method_count())
	{
		switch (index - BaseWidget::method_count())
		{
		case 0:
			{
				static ParamSpec spec[] = 
				{
					{PS_OBJECT, 0}
				};
				spec[0].tag = MENU_TAG;
				return spec;
			}
			break;

		case 1:
			{
				static ParamSpec spec[] = 
				{
					{PS_VOID, 0},
					{PS_OBJECT, 0}
				};
				spec[1].tag = MENU_TAG;
				return spec;
			}
			break;
		}
		return NULL;
	}
	else
	{
		return BaseWidget::method_spec (index);
	}
}

bool TopLevelWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	for (int i = 0; i < sizeof(tlw_names)/sizeof(char *); ++i)
	{
		if (!strcmp (methodName, tlw_names[i]))
		{
			return invokeByIndex (BaseWidget::method_count() + i, result, paramCount, params);
		}
	}
	return BaseWidget::invoke (methodName, result, paramCount, params);
}

bool TopLevelWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	if (index >= BaseWidget::method_count())
	{
		switch (index - BaseWidget::method_count())
		{
		case 0:
			{
				HMENU hMenu = get_menu ();
				result->spec.type = PS_OBJECT;
				result->spec.tag = MENU_TAG;
				result->oVal = (void *) hMenu;
				return true;
			}
			break;

		case 1:
			if (params[0].spec.type == PS_OBJECT && params[0].spec.tag == MENU_TAG)
			{
				HMENU hMenu = (HMENU) params[0].oVal;
				set_menu (hMenu);
				result->spec.type = PS_VOID;
				result->spec.tag = 0;
				return true;
			}
			break;
		}

		return false;
	}
	return BaseWidget::invokeByIndex (index, result, paramCount, params);
}

// TopLevel creation function, exported to lua

static void newTopLevel (void)
{
	// Syntax: 
	//     NewTopLevel
	//     (
	//         string <title>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>
	//     )
	// Create a button and returns its HWND as an object.
	// <image filename> is the name of the .BMP file to display
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the image, -1 to use the actual image's value

	// Get and validate the parameters
	lua_Object title = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(title))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	// The parameters are valid, so go about creating a new button.

	TopLevelWidget *tlw = new TopLevelWidget;
	assert (tlw != NULL && "Failed to allocate a new TopLevel widget.");

	if
	(
		tlw->create
		(
			(int) lua_getnumber(xpos), (int) lua_getnumber(ypos), 
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			lua_getstring(title)
		)
	)
	{
		// Finally, export the widget.
		export_widget (tlw);
	}

	// Return the object on the top of the stack, either a valid widget or nil.
	return;
}

// ============ Text Widget ============
// *** Events ***
const EventId TEXT_CHANGED = 0;
const EventId TEXT_EVENT_COUNT = 1;

// *** Methods ***
TextWidget::TextWidget()
{
	textWnd = NULL;
}

TextWidget::~TextWidget()
{
	// There is no need to destroy the textWnd; it will be destroyed as a child of
	// hBaseWnd.
}

// Creation methods
bool TextWidget::create (int w, int h, const char *text, HWND parent)
{
	// Create the base window first, then create buttonWnd as a child of it.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the text window as a child of the base window.
	DWORD style = WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN;

	textWnd =
		CreateWindow
		(
			"EDIT",
			text,
			WS_CHILD | WS_VISIBLE | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!textWnd)
	{
		return false;
	}

	// The button has been created, so do any other initialization here and return true.

	return true;
}

// BaseWidget overloads
LRESULT TextWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to textWnd!
	switch (uMsg)
	{
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
		// Reflect text messages to the button window, bypassing default behavior
		return SendMessage (textWnd, uMsg, wParam, lParam);
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the button window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (textWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

bool TextWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the button as well.
	if (textWnd)
	{
		SetWindowPos (textWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

bool TextWidget::on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
	// Send an event when this button is pressed.
	if (hwndCtl == textWnd && wNotifyCode == EN_CHANGE)
	{
		if (fire_event (TEXT_CHANGED))
		{
			return true;
		}
	}

	// Perform the inherited behavior
	return BaseWidget::on_command(wNotifyCode, wID, hwndCtl);
}

// === Supported Interfaces === 

EventId TextWidget::event_count(void)
{
	return TEXT_EVENT_COUNT;
}

const char *TextWidget::event_name(EventId index)
{
	static char *eventNames[TEXT_EVENT_COUNT] =
	{
		"OnChange"
	};
	if (index < 0 || index >= TEXT_EVENT_COUNT)
	{
		return NULL;
	}
	return eventNames[index];
}

// Text creation function, exported to lua

static void newText (void)
{
	// Syntax: 
	//     NewText
	//     (
	//         Window <parent>,
	//         string <text>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>
	//     )
	// Create a button and returns its HWND as an object.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object text = lua_getparam(2);
	lua_Object xpos = lua_getparam(3);
	lua_Object ypos = lua_getparam(4);
	lua_Object width = lua_getparam(5);
	lua_Object height = lua_getparam(6);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(text))
	{
		return;
	}
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	// The parameters are valid, so go about creating a new button.

	HWND hParent;
	if (lua_isnil(parent))
	{
		hParent = NULL;
	}
	else if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent tag!\n");
		return;
	}

	TextWidget *tw = new TextWidget;
	assert (tw != NULL && "Failed to allocate a new text widget.");

	if
	(
		tw->create
		(
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			lua_getstring(text),
			hParent
		)
	)
	{
		// Position the child at the given location.
		tw->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Finally, export the widget.
		export_widget (tw);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============== FrameWidget ===============

// *** TODO: Add transparent frame support, i.e. transparent window and no painting. Useful for
// *** grouping and controling visibility of child controls.

FrameWidget::FrameWidget ()
{
	// Nothing to do now
}

FrameWidget::~FrameWidget ()
{
	// Nothing to do now
}

// Creation methods
bool FrameWidget::create (int w, int h, HWND parent)
{
	DWORD style = WS_VISIBLE | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	DWORD exStyle = WS_EX_CLIENTEDGE;
	if (!BaseWidget::create(exStyle, style, parent, w, h))
	{
		return false;
	}

	// The base window is created.
	// All is well. Return success.
	return true;
}

static void newFrame (void)
{
	// Syntax: 
	//     NewFrame
	//     (
	//         Window <parent>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>
	//     )
	// Create a button and returns its HWND as an object.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);

	// Check the types of the input data before proceeding.
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	// The parameters are valid, so go about creating a new button.

	HWND hParent;
	if (lua_isnil(parent))
	{
		hParent = NULL;
	}
	else if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent tag!\n");
		return;
	}

	FrameWidget *fw = new FrameWidget;
	assert (fw != NULL && "Failed to allocate a new FrameWidget.");

	if
	(
		fw->create
		(
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			hParent
		)
	)
	{
		// Position the child at the given location.
		fw->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Finally, export the widget.
		export_widget (fw);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============ Scroll Widget =============
// *** Events ***
const EventId SCROLL_CHANGED = 0;
const EventId SCROLL_EVENT_COUNT = 1;

// *** Methods ***
ScrollWidget::ScrollWidget ()
{
	scrollWnd = NULL;
}

ScrollWidget::~ScrollWidget ()
{
	// Nothing for now
}

bool ScrollWidget::create (int w, int h, HWND parent, ScrollType type)
{
	// Create the base window, then the scrollbar as a child, so we can get notification messages.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the scroll window as a child of the base window.
	DWORD exStyle=0, style=0;
	switch (type)
	{
	case SCR_VERTICAL:
		style = SBS_VERT;
		break;

	case SCR_HORIZONTAL:
		style = SBS_HORZ;
		break;

	default:
		assert (false && "Invalid scroll type");
		break;
	}

	scrollWnd =
		CreateWindowEx
		(
			exStyle,
			"SCROLLBAR",
			(LPSTR) NULL,
			WS_CHILD | WS_VISIBLE | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!scrollWnd)
	{
		return false;
	}

	// The scrollbar has been created, so do any other initialization here and return true.
	set_scroll_range (0, 100);
	set_scroll_pos (0);
	ShowScrollBar (scrollWnd, SB_CTL, TRUE);

	return true;
}

void ScrollWidget::set_scroll_range (int min, int max)
{
	if (scrollWnd)
	{
		// Set the range of this scrollbar and redraw
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.nMin = min;
		si.nMax = max;
		si.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
		SetScrollInfo (scrollWnd, SB_CTL, &si, TRUE);

		// Generate a SCROLL_CHANGED event.
		fire_event (SCROLL_CHANGED);
	}
}

int ScrollWidget::get_scroll_min()
{
	if (scrollWnd)
	{
		// Get the range of this scrollbar and return the min value.
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE;
		if (GetScrollInfo (scrollWnd, SB_CTL, &si))
		{
			return si.nMin;
		}
	}
	return 0;
}

int ScrollWidget::get_scroll_max()
{
	if (scrollWnd)
	{
		// Get the range of this scrollbar and return the max value.
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE;
		if (GetScrollInfo (scrollWnd, SB_CTL, &si))
		{
			return si.nMax;
		}
	}
	return 0;
}

void ScrollWidget::set_scroll_pos (int pos)
{
	if (scrollWnd)
	{
		// Set the position and redraw.
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.nPos = pos;
		si.fMask = SIF_POS | SIF_DISABLENOSCROLL;
		SetScrollInfo (scrollWnd, SB_CTL, &si, TRUE);

		// Generate a SCROLL_CHANGED event.
		fire_event (SCROLL_CHANGED);
	}
}

int ScrollWidget::get_scroll_pos ()
{
	if (scrollWnd)
	{
		// Get the position.
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_POS;
		if (GetScrollInfo (scrollWnd, SB_CTL, &si))
		{
			return si.nPos;
		}
	}
	return 0;
}

void ScrollWidget::set_scroll_page (unsigned int pagelen)
{
	if (scrollWnd)
	{
		// Get 
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE;
		si.nPage = pagelen;
		SendMessage (scrollWnd, SBM_SETSCROLLINFO, (WPARAM) TRUE, (LPARAM) &si);

		// Generate a SCROLL_CHANGED event.
		fire_event (SCROLL_CHANGED);
	}
}

unsigned int ScrollWidget::get_scroll_page()
{
	if (scrollWnd)
	{
		// Set the position and redraw.
		SCROLLINFO si;
		memset (&si, 0, sizeof(si));
		si.cbSize = sizeof(si);
		si.fMask = SIF_PAGE;
		SendMessage (scrollWnd, SBM_GETSCROLLINFO, (WPARAM) 0, (LPARAM) &si);
		return si.nPage;
	}
	return 1;
}

// ***** Message Handlers *****
bool ScrollWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the scrollbar as well.
	if (scrollWnd)
	{
		SetWindowPos (scrollWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

LRESULT ScrollWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to scrollWnd!
	switch (uMsg)
	{
	case WM_HSCROLL:
	case WM_VSCROLL:
		// NOTE: We don't care which version of scroll message we get, because there is only
		// one child scrollbar.
		{
			int nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			short int nPos = (short int) HIWORD(wParam);  // scroll box position 
			HWND hwndScrollBar = (HWND) lParam;      // handle of scroll bar

			// Make sure our assumption is true.
			if (hwndScrollBar != scrollWnd)
			{
				// Other stuff just falls through
				break;
			}

			// Switch on the specific event.
			// NOTE: Events are generated when position, range, and page setting methods
			// are called, since setting the values doesn't cause this message to be sent.

			switch (nScrollCode)
			{
				case SB_BOTTOM:
				case SB_ENDSCROLL:
					break;

				case SB_LINEDOWN:
					set_scroll_pos (get_scroll_pos() + 1);
					break;

				case SB_LINEUP:
					set_scroll_pos (get_scroll_pos() - 1);
					break;

				case SB_PAGEDOWN:
					set_scroll_pos (get_scroll_pos() + get_scroll_page());
					break;

				case SB_PAGEUP:
					set_scroll_pos (get_scroll_pos() - get_scroll_page());
					break;

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					set_scroll_pos(nPos);
					UpdateWindow(scrollWnd);
					break;

				case SB_TOP:
					break;
			}

			// We processed this, so return.
			return 0;
		}
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the scroll window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (scrollWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

// ***** Interfaces *****

METHOD_SPEC_BEGIN(ScrollWidget, set_scroll_range, 2, PS_VOID)
METHOD_SPEC_ENTRY(PS_INT),
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ScrollWidget, set_scroll_range, 2)

METHOD_SPEC_BEGIN(ScrollWidget, get_scroll_min, 0, PS_INT)
METHOD_SPEC_END(ScrollWidget, get_scroll_min, 0)

METHOD_SPEC_BEGIN(ScrollWidget, get_scroll_max, 0, PS_INT)
METHOD_SPEC_END(ScrollWidget, get_scroll_max, 0)

METHOD_SPEC_BEGIN(ScrollWidget, set_scroll_pos, 1, PS_VOID)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ScrollWidget, set_scroll_pos, 1)

METHOD_SPEC_BEGIN(ScrollWidget, get_scroll_pos, 0, PS_INT)
METHOD_SPEC_END(ScrollWidget, get_scroll_pos, 0)

METHOD_SPEC_BEGIN(ScrollWidget, set_scroll_page, 1, PS_VOID)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ScrollWidget, set_scroll_page, 1)

METHOD_SPEC_BEGIN(ScrollWidget, get_scroll_page, 0, PS_INT)
METHOD_SPEC_END(ScrollWidget, get_scroll_page, 0)

METHOD_TABLE_START(ScrollWidget)
METHOD_DEF(ScrollWidget, set_scroll_range),
METHOD_DEF(ScrollWidget, get_scroll_min),
METHOD_DEF(ScrollWidget, get_scroll_max),
METHOD_DEF(ScrollWidget, set_scroll_pos),
METHOD_DEF(ScrollWidget, get_scroll_pos),
METHOD_DEF(ScrollWidget, set_scroll_page),
METHOD_DEF(ScrollWidget, get_scroll_page)
METHOD_TABLE_END()

// IScriptable Interface, inherited from IWidget
int ScrollWidget::method_count(void)
{
	return GET_METHOD_COUNT (ScrollWidget, BaseWidget);
}

const char *ScrollWidget::method_name(int index)
{
	return GET_METHOD_NAME (index, ScrollWidget, BaseWidget);
}

int ScrollWidget::method_speclen(int index)
{
	return GET_METHOD_SPECLEN (index, ScrollWidget, BaseWidget);
}

const ParamSpec *ScrollWidget::method_spec (int index)
{
	return GET_METHOD_SPEC (index, ScrollWidget, BaseWidget);
}

bool ScrollWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	bool retVal = false;
	METHOD_INVOKE (ScrollWidget, BaseWidget, retVal, methodName, result, paramCount, params);
	return retVal;
}

bool ScrollWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	// NOTE: The given index includes the inherited indexing. For indices < our ancestors count, 
	// just invoke the ancestor.

	if (index < BaseWidget::method_count())
	{
		return BaseWidget::invokeByIndex (index, result, paramCount, params);
	}

	// This is one of ours, so invoke it.
	index -= BaseWidget::method_count();

	switch (index)
	{
	case 0:  // set_scroll_range
		if (paramCount >= 2)
		{
			if (params[0].spec.type == PS_INT && params[1].spec.type == PS_INT)
			{
				set_scroll_range (params[0].iVal, params[1].iVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 1:  // get_scroll_min
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_scroll_min();
			return true;
		}
		break;

	case 2:  // get_scroll_max
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_scroll_max();
			return true;
		}
		break;

	case 3:  // set_scroll_pos
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				set_scroll_pos (params[0].iVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 4:  // get_scroll_pos
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_scroll_pos();
			return true;
		}
		break;

	case 5:  // set_scroll_page
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				set_scroll_page (params[0].iVal);
				result->spec.type = PS_VOID;
				return true;
			}
		}
		break;

	case 6:  // get_scroll_page
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_scroll_page();
			return true;
		}
		break;
	}

	return false;
}

// IEventSource Interface, inherited from IWidget
EventId ScrollWidget::event_count(void)
{
	return SCROLL_EVENT_COUNT;
}

const char *ScrollWidget::event_name(EventId index)
{
	static char *eventNames[SCROLL_EVENT_COUNT] =
	{
		"OnChange"
	};
	if (index < 0 || index >= SCROLL_EVENT_COUNT)
	{
		return NULL;
	}
	return eventNames[index];
}

// ***** Scripting Export *****

static void newScroll (void)
{
	// Syntax: 
	//     NewScroll
	//     (
	//         Window <parent>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>,
	//         [string <type>]
	//     )
	// Create a scroll and returns it to the scripting language.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button
	// <type> is either "v[ertical]" or "h[orizontal]". It is optional; default is horizontal.

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);
	lua_Object type = lua_getparam(6);

	// Check the types of the input data before proceeding.
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	ScrollWidget::ScrollType st = ScrollWidget::SCR_HORIZONTAL;
	if (lua_isnil(type))
	{
		// Default value.
		st = ScrollWidget::SCR_HORIZONTAL;
	}
	else if (lua_isstring(type))
	{
		const char *str = lua_getstring(type);
		switch (*str)
		{
		case 'h':
		case 'H':
			st = ScrollWidget::SCR_HORIZONTAL;
			break;

		case 'v':
		case 'V':
			st = ScrollWidget::SCR_VERTICAL;
			break;

		default:
			return;
			break;
		}
	}
	else
	{
		// Invalid type for 'type'.
		return;
	}

	// The parameters are valid, so go about creating a new scroll.

	HWND hParent;
	if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent window for scrollbar!\n");
		return;
	}

	ScrollWidget *sw = new ScrollWidget;
	assert (sw != NULL && "Failed to allocate a new ScrollWidget.");

	if
	(
		sw->create
		(
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			hParent,
			st
		)
	)
	{
		// Position the child at the given location.
		sw->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Finally, export the widget.
		export_widget (sw);
	}

	// We failed to allocate a window, so just return.
	return;
}

// ============ List Widget =============
// ***** Events *****

const EventId LIST_CHANGED = 0;
const EventId LIST_DBLCLK = 1;
const EventId LIST_EVENT_COUNT = 2;

// ***** Methods *****

ListWidget::ListWidget ()
{
	listWnd = NULL;
	itemTextBuffer = NULL;
	itemIndexBuffer = NULL;
	itemIndexLen = 0;
}

ListWidget::~ListWidget ()
{
	if (itemTextBuffer)
	{
		delete itemTextBuffer;
		itemTextBuffer = NULL;
	}
	if (itemIndexBuffer)
	{
		delete itemIndexBuffer;
		itemIndexBuffer = NULL;
		itemIndexLen = 0;
	}
}

bool ListWidget::create (int w, int h, HWND parent, ListType type, bool sort)
{
	// Create the base window, then the listbox as a child, so we can get notification messages.
	// The base window is a plain child window.
	if (!BaseWidget::create(0, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the listbox window as a child of the base window.
	DWORD exStyle=0;

	DWORD style = LBS_NOTIFY | LBS_HASSTRINGS | LBS_DISABLENOSCROLL | LBS_NOINTEGRALHEIGHT;
	switch (type)
	{
	case LST_NOSEL:
		style = LBS_NOSEL;
		break;

	case LST_SEL:
		// Standard style
		break;

	case LST_MULTISEL:
		style = LBS_EXTENDEDSEL;
		break;

	default:
		assert (false && "Invalid list type");
		break;
	}

	if (sort)
	{
		style |= LBS_SORT;
	}

	listWnd =
		CreateWindowEx
		(
			exStyle,
			"LISTBOX",
			(LPSTR) NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | WS_VSCROLL | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!listWnd)
	{
		return false;
	}

	// The list box has been created, so do any other initialization here and return true.

	return true;
}

int ListWidget::add_string (const char *string)
{
	if (listWnd)
	{
		LRESULT result = SendMessage (listWnd, LB_ADDSTRING, (WPARAM) 0, (LPARAM) string);
		if (result != LB_ERR && result != LB_ERRSPACE)
		{
			return (int) result;
		}
	}
	return -1;
}

bool ListWidget::del_string (int index)
{
	if (listWnd && index >= 0)
	{
		LRESULT result = SendMessage (listWnd, LB_DELETESTRING, (WPARAM) index, (LPARAM) 0);
		if (result != LB_ERR)
		{
			return true;
		}
	}
	return false;
}

bool ListWidget::del_named_string (const char *string)
{
	return del_string (find_string(string));
}

int ListWidget::find_string (const char *string)
{
	if (listWnd)
	{
		LRESULT result = SendMessage (listWnd, LB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) string);
		if (result != LB_ERR)
		{
			return (int) result;
		}
	}
	return -1;
}

int ListWidget::find_partial_string (const char *prefix)
{
	if (listWnd)
	{
		LRESULT result = SendMessage (listWnd, LB_FINDSTRING, (WPARAM) -1, (LPARAM) prefix);
		if (result != LB_ERR)
		{
			return (int) result;
		}
	}
	return -1;
}

const char *ListWidget::get_item_string (int index)
{
	if (listWnd && index >= 0)
	{
		if (itemTextBuffer)
		{
			delete itemTextBuffer;
			itemTextBuffer = NULL;
		}

		LRESULT result = SendMessage (listWnd, LB_GETTEXTLEN, (WPARAM) index, (LPARAM) 0);
		if (result != LB_ERR)
		{
			int len = (int) result;
			itemTextBuffer = new char[len];
			if (itemTextBuffer)
			{
				result = SendMessage (listWnd, LB_GETTEXT, (WPARAM) index, (LPARAM) itemTextBuffer);
				if (result != LB_ERR)
				{
					return itemTextBuffer;
				}
			}
		}
	}
	return NULL;
}

DWORD ListWidget::get_item_data (int index)
{
	if (listWnd && index >= 0)
	{
		return (DWORD) SendMessage (listWnd, LB_GETITEMDATA, (WPARAM) index, (LPARAM) 0);
	}
	return 0;
}

int ListWidget::get_item_count ()
{
	if (listWnd)
	{
		return (int) SendMessage (listWnd, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
	}
	return 0;
}

bool ListWidget::set_single_select (int index, bool set)
{
	return set_select (1, &index, set);
}

bool ListWidget::set_select (int count, const int *indices, bool set)
{
	if (listWnd && indices && count)
	{
		LRESULT result;
		LONG style = GetWindowLong (listWnd, GWL_STYLE);
		if (style & LBS_EXTENDEDSEL)
		{
			// Set or clear multiple selections, according to the 'set' flag.
			// NOTE: A -1 index indicates all
			for (int i = 0; i < count; ++i)
			{
				result = SendMessage (listWnd, LB_SETSEL, (WPARAM) set, (LPARAM) indices[i]);
				if (result == LB_ERR)
				{
					break;
				}
			}

			return (result != LB_ERR);
		}
		else if (!(style & LBS_NOSEL))
		{
			// Set the single selection.
			// NOTE: a -1 selection will clear the selection, as will passing 'false' for 'set'
			result = SendMessage (listWnd, LB_SETCURSEL, (WPARAM) (set ? *indices : -1), (LPARAM) 0);
			return (result != LB_ERR);
		}
	}
	return false;
}

int ListWidget::get_select_count ()
{
	if (listWnd)
	{
		LONG style = GetWindowLong (listWnd, GWL_STYLE);
		if (style & LBS_EXTENDEDSEL)
		{
			return (int) SendMessage (listWnd, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		else
		{
			if (SendMessage (listWnd, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) != LB_ERR)
			{
				return 1;
			}
		}
	}
	return 0;
}

int ListWidget::get_single_select (int index)
{
	// NOTE: This retrieves the list every time.
	const int *selected = get_select();
	if (selected && index >= 0 && index < itemIndexLen)
	{
		return selected[index];
	}
	return -1;
}

const int *ListWidget::get_select ()
{
	if (listWnd)
	{
		LRESULT result;

		LONG style = GetWindowLong (listWnd, GWL_STYLE);
		if (style & LBS_EXTENDEDSEL)
		{
			int count = (int) SendMessage (listWnd, LB_GETSELCOUNT, (WPARAM) 0, (LPARAM) 0);
			if (count > itemIndexLen)
			{
				if (itemTextBuffer)
				{
					delete itemIndexBuffer;
					itemIndexBuffer = NULL;
				}
				itemIndexBuffer = new int[count];
				itemIndexLen = count;
			}
			if (itemIndexBuffer)
			{
				// Get all of the selections.
				result = SendMessage (listWnd, LB_GETSELITEMS, (WPARAM) count, (LPARAM) itemIndexBuffer);
				if (result == LB_ERR)
				{
					return itemIndexBuffer;
				}
			}
		}
		else if (!(style & LBS_NOSEL))
		{
			// Get the single selection.
			if (!itemIndexBuffer)
			{
				itemIndexBuffer = new int[1];
				itemIndexLen = 1;
			}
			if (itemIndexBuffer)
			{
				result = SendMessage (listWnd, LB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
				if (result != LB_ERR)
				{
					*itemIndexBuffer = (int) result;
					return itemIndexBuffer;
				}
			}
		}
	}
	return NULL;
}

void ListWidget::reset_content ()
{
	if (listWnd)
	{
		SendMessage (listWnd, LB_RESETCONTENT, (WPARAM) 0, (LPARAM) 0);
	}
}

// ***** Message Handlers *****
LRESULT ListWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to buttonWnd!
	switch (uMsg)
	{
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
		// Reflect text messages to the button window, bypassing default behavior
		return SendMessage (listWnd, uMsg, wParam, lParam);
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the button window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (listWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

bool ListWidget::on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
	switch (wNotifyCode)
	{
	case LBN_DBLCLK:
		// Fire of an event.
		fire_event (LIST_DBLCLK);
		return true;
		break;

	case LBN_SELCHANGE:
		fire_event (LIST_CHANGED);
		return true;
		break;
	}

	return false;
}

bool ListWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the scrollbar as well.
	if (listWnd)
	{
		SetWindowPos (listWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

bool ListWidget::on_paint (HDC hdc)
{
	// Pretend we painted
	PAINTSTRUCT ps;

	HDC paintDC = BeginPaint(hBaseWnd, &ps);
	EndPaint (hBaseWnd, &ps);
	return true;
}

bool ListWidget::on_erasebkgnd (HDC hdc)
{
	// Pretend we cleared the background
	return true;
}

// ***** Interfaces *****

METHOD_SPEC_BEGIN(ListWidget, add_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ListWidget, add_string, 1)

METHOD_SPEC_BEGIN(ListWidget, del_string, 1, PS_BOOL)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ListWidget, del_string, 1)

METHOD_SPEC_BEGIN(ListWidget, del_named_string, 1, PS_BOOL)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ListWidget, del_named_string, 1)

METHOD_SPEC_BEGIN(ListWidget, find_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ListWidget, find_string, 1)

METHOD_SPEC_BEGIN(ListWidget, find_partial_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ListWidget, find_partial_string, 1)

METHOD_SPEC_BEGIN(ListWidget, get_item_string, 1, PS_STRING)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ListWidget, get_item_string, 1)

METHOD_SPEC_BEGIN(ListWidget, get_item_data, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ListWidget, get_item_data, 1)

METHOD_SPEC_BEGIN(ListWidget, get_item_count, 0, PS_INT)
METHOD_SPEC_END(ListWidget, get_item_count, 0)

METHOD_SPEC_BEGIN(ListWidget, get_select_count, 0, PS_INT)
METHOD_SPEC_END(ListWidget, get_select_count, 0)

METHOD_SPEC_BEGIN(ListWidget, set_select, 2, PS_BOOL)
METHOD_SPEC_ENTRY(PS_INT),
METHOD_SPEC_ENTRY(PS_BOOL)
METHOD_SPEC_END(ListWidget, set_select, 2)

METHOD_SPEC_BEGIN(ListWidget, get_select, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ListWidget, get_select, 1)

METHOD_SPEC_BEGIN(ListWidget, reset, 0, PS_VOID)
METHOD_SPEC_END(ListWidget, reset, 0)

METHOD_TABLE_START(ListWidget)
METHOD_DEF(ListWidget, add_string),
METHOD_DEF(ListWidget, del_string),
METHOD_DEF(ListWidget, del_named_string),
METHOD_DEF(ListWidget, find_string),
METHOD_DEF(ListWidget, find_partial_string),
METHOD_DEF(ListWidget, get_item_string),
METHOD_DEF(ListWidget, get_item_data),
METHOD_DEF(ListWidget, get_item_count),
METHOD_DEF(ListWidget, get_select_count),
METHOD_DEF(ListWidget, set_select),
METHOD_DEF(ListWidget, get_select),
METHOD_DEF(ListWidget, reset)
METHOD_TABLE_END()

// IScriptable Interface, inherited from IWidget
int ListWidget::method_count(void)
{
	return GET_METHOD_COUNT (ListWidget, BaseWidget);
}

const char *ListWidget::method_name(int index)
{
	return GET_METHOD_NAME (index, ListWidget, BaseWidget);
}

int ListWidget::method_speclen(int index)
{
	return GET_METHOD_SPECLEN (index, ListWidget, BaseWidget);
}

const ParamSpec *ListWidget::method_spec (int index)
{
	return GET_METHOD_SPEC (index, ListWidget, BaseWidget);
}

bool ListWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	bool retVal = false;
	METHOD_INVOKE (ListWidget, BaseWidget, retVal, methodName, result, paramCount, params);
	return retVal;
}

bool ListWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	// NOTE: The given index includes the inherited indexing. For indices < our ancestors count, 
	// just invoke the ancestor.

	if (index < BaseWidget::method_count())
	{
		return BaseWidget::invokeByIndex (index, result, paramCount, params);
	}

	// This is one of ours, so invoke it.
	index -= BaseWidget::method_count();

	switch (index)
	{
	case 0:  // add_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = add_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 1:  // del_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = del_string(params[0].iVal);
				return true;
			}
		}
		break;

	case 2:  // del_named_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = del_named_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 3:  // find_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = find_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 4:  // find_partial_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = find_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 5:  // get_item_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_STRING;
				result->spec.tag = 0;
				result->sVal = get_item_string (params[0].iVal);
				return true;
			}
		}
		break;

	case 6:  // get_item_data
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = get_item_data (params[0].iVal);
				return true;
			}
		}
		break;

	case 7:  // get_item_count
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_item_count ();
			return true;
		}
		break;

	case 8:  // get_select_count
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_select_count ();
			return true;
		}
		break;

	case 9:  // set_select
		if (paramCount >= 2)
		{
			if (params[0].spec.type == PS_INT && params[1].spec.type == PS_BOOL)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = set_single_select (params[0].iVal, params[1].bVal);
				return true;
			}
		}
		break;

	case 10:  // get_select
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = get_single_select (params[0].iVal);
				return true;
			}
		}
		break;

	case 11:  // reset
		result->spec.type = PS_VOID;
		result->spec.tag = 0;
		reset_content ();
		return true;
		break;
	}

	return false;
}

// IEventSource Interface, inherited from IWidget
EventId ListWidget::event_count(void)
{
	return LIST_EVENT_COUNT;
}

const char *ListWidget::event_name(EventId index)
{
	static char *eventNames[LIST_EVENT_COUNT] = {"OnChange", "OnDoubleClick"};
	if (index >= 0 && index < LIST_EVENT_COUNT)
	{
		return eventNames[index];
	}
	return NULL;
}

// ***** Scripting Export *****

static void newListBox (void)
{
	// Syntax: 
	//     NewListBox
	//     (
	//         Window <parent>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>,
	//         [string <type>],
	//         [number <sort>]
	//     )
	// Create a scroll and returns it to the scripting language.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button
	// <type> is either "n[oselect]", "s[elect]", or "m[ultiselect]". It is optional; default is "select".

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);
	lua_Object type = lua_getparam(6);
	lua_Object sort = lua_getparam(7);

	// Check the types of the input data before proceeding.
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	ListWidget::ListType lt = ListWidget::LST_SEL;
	if (lua_isnil(type) || type == LUA_NOOBJECT)
	{
		// Default value.
		lt = ListWidget::LST_SEL;
	}
	else if (lua_isstring(type))
	{
		const char *str = lua_getstring(type);
		switch (*str)
		{
		case 'n':
		case 'N':
			lt = ListWidget::LST_NOSEL;
			break;

		case 's':
		case 'S':
			lt = ListWidget::LST_SEL;
			break;

		case 'm':
		case 'M':
			lt = ListWidget::LST_MULTISEL;
			break;

		default:
			return;
			break;
		}
	}
	else
	{
		// Invalid type for 'type'.
		return;
	}

	bool doSort = true;
	if (sort != LUA_NOOBJECT && lua_isnil(sort))
	{
		doSort = true;
	}

	// The parameters are valid, so go about creating a new scroll.

	HWND hParent;
	if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent window for scrollbar!\n");
		return;
	}

	ListWidget *lw = new ListWidget;
	assert (lw != NULL && "Failed to allocate a new ListWidget.");

	if
	(
		lw->create
		(
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			hParent,
			lt, doSort
		)
	)
	{
		// Position the child at the given location.
		lw->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Finally, export the widget.
		export_widget (lw);
	}

	// We failed to allocate a window, so just return.
	return;
}

// =========== Combo Widget =============
// *** Events ***
const EventId COMBO_CHANGED = 0;  // NOTE: This is fired on both list box changes and edit changes.
const EventId COMBO_DBLCLK  = 1;
const EventId COMBO_EVENT_COUNT = 2;

// *** Methods ***
ComboWidget::ComboWidget ()
{
	comboWnd = NULL;
	itemTextBuffer = NULL;
	itemIndexBuffer = NULL;
	itemIndexLen = 0;
}

ComboWidget::~ComboWidget ()
{
	if (itemTextBuffer)
	{
		delete itemTextBuffer;
		itemTextBuffer = NULL;
	}
	if (itemIndexBuffer)
	{
		delete itemIndexBuffer;
		itemIndexBuffer = NULL;
		itemIndexLen = 0;
	}
}

bool ComboWidget::create (int w, int h, HWND parent, ComboType type, bool sort)
{
	// Create the base window, then the combobox as a child, so we can get notification messages.
	// The base window is a plain child window.
	if (!BaseWidget::create(WS_EX_TRANSPARENT, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE, parent, w, h))
	{
		return false;
	}

	// Now create the listbox window as a child of the base window.
	DWORD exStyle=0;

	DWORD style = CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT;
	switch (type)
	{
	case CMB_SIMPLE:
		style = CBS_SIMPLE;
		break;

	case CMB_DROPBOX:
		style = CBS_DROPDOWN;
		break;

	case CMB_DROPLIST:
		style = CBS_DROPDOWNLIST;
		break;

	default:
		assert (false && "Invalid combobox type");
		break;
	}

	if (sort)
	{
		style |= CBS_SORT;
	}

	comboWnd =
		CreateWindowEx
		(
			exStyle,
			"COMBOBOX",
			(LPSTR) NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_BORDER | WS_VSCROLL | style,
			0, 0,
			w, h,
			hBaseWnd,
			NULL, 
			(HINSTANCE) GetWindowLong(hBaseWnd, GWL_HINSTANCE),
			NULL
		);

	if (!comboWnd)
	{
		return false;
	}

	// The list box has been created, so do any other initialization here and return true.

	return true;
}

int ComboWidget::add_string (const char *string)
{
	if (comboWnd)
	{
		LRESULT result = SendMessage (comboWnd, CB_ADDSTRING, (WPARAM) 0, (LPARAM) string);
		if (result != CB_ERR && result != CB_ERRSPACE)
		{
			return (int) result;
		}
	}
	return -1;
}

bool ComboWidget::del_string (int index)
{
	if (comboWnd && index >= 0)
	{
		LRESULT result = SendMessage (comboWnd, CB_DELETESTRING, (WPARAM) index, (LPARAM) 0);
		if (result != LB_ERR)
		{
			return true;
		}
	}
	return false;
}

bool ComboWidget::del_named_string (const char *string)
{
	return del_string (find_string(string));
}

int ComboWidget::find_string (const char *string)
{
	if (comboWnd)
	{
		LRESULT result = SendMessage (comboWnd, CB_FINDSTRINGEXACT, (WPARAM) -1, (LPARAM) string);
		if (result != CB_ERR)
		{
			return (int) result;
		}
	}
	return -1;
}

int ComboWidget::find_partial_string (const char *prefix)
{
	if (comboWnd)
	{
		LRESULT result = SendMessage (comboWnd, CB_FINDSTRING, (WPARAM) -1, (LPARAM) prefix);
		if (result != CB_ERR)
		{
			return (int) result;
		}
	}
	return -1;
}

const char *ComboWidget::get_item_string (int index)
{
	if (comboWnd && index >= 0)
	{
		if (itemTextBuffer)
		{
			delete itemTextBuffer;
			itemTextBuffer = NULL;
		}

		LRESULT result = SendMessage (comboWnd, CB_GETLBTEXTLEN, (WPARAM) index, (LPARAM) 0);
		if (result != CB_ERR)
		{
			int len = (int) result;
			itemTextBuffer = new char[len];
			if (itemTextBuffer)
			{
				result = SendMessage (comboWnd, CB_GETLBTEXT, (WPARAM) index, (LPARAM) itemTextBuffer);
				if (result != CB_ERR)
				{
					return itemTextBuffer;
				}
			}
		}
	}
	return NULL;
}

DWORD ComboWidget::get_item_data (int index)
{
	if (comboWnd && index >= 0)
	{
		return (DWORD) SendMessage (comboWnd, CB_GETITEMDATA, (WPARAM) index, (LPARAM) 0);
	}
	return 0;
}

int ComboWidget::get_item_count ()
{
	if (comboWnd)
	{
		return (int) SendMessage (comboWnd, CB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
	}
	return 0;
}

bool ComboWidget::set_single_select (int index, bool set)
{
	if (comboWnd)
	{
		LRESULT result;
		// Set the single selection.
		result = SendMessage (comboWnd, CB_SETCURSEL, (WPARAM) (set ? index : -1), (LPARAM) 0);
		return true;
	}
	return false;
}

int ComboWidget::get_single_select ()
{
	if (comboWnd)
	{
		LRESULT result = SendMessage (comboWnd, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
		if (result != CB_ERR)
		{
			return (int) result;
		}
	}
	return -1;
}

// *** Message Handlers ***
LRESULT ComboWidget::handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// NOTE: These are messages to hBaseWnd, not to buttonWnd!
	switch (uMsg)
	{
	case WM_GETTEXT:
	case WM_SETTEXT:
	case WM_GETTEXTLENGTH:
		// Reflect text messages to the button window, bypassing default behavior
		return SendMessage (comboWnd, uMsg, wParam, lParam);
		break;

	case WM_ENABLE:
		// Mirror the enabled state of this window in the button window
		{
			BOOL fEnabled = (BOOL) wParam;
			EnableWindow (comboWnd, fEnabled);
		}
		break;
	}

	// Do the inherited behavior for for everything else.
	return BaseWidget::handle_message (hwnd, uMsg, wParam, lParam);
}

bool ComboWidget::on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl)
{
	switch (wNotifyCode)
	{
	case CBN_DBLCLK:
		// Fire of an event.
		fire_event (COMBO_DBLCLK);
		return true;
		break;

//	case CBN_SELCHANGE:
	case CBN_SELENDOK:
		{
			// Post a fake CBN_EDITCHANGE message, so that the an event will be sent after
			// the text has changed.
			DWORD wParam = ((DWORD) CBN_EDITCHANGE << 16) | ((DWORD) wID);
			PostMessage (hBaseWnd, WM_COMMAND, (WPARAM) wParam, (LPARAM) hwndCtl);
		}
		break;

	case CBN_EDITCHANGE:
		fire_event (COMBO_CHANGED);
		return true;
		break;
	}

	return false;
}

bool ComboWidget::on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight)
{
	// The base window has been resized, so resize the combobox as well.
	if (comboWnd)
	{
		SetWindowPos (comboWnd, NULL, 0, 0, nWidth, nHeight, SWP_NOMOVE | SWP_NOZORDER);
	}

	return true;
}

bool ComboWidget::on_paint (HDC hdc)
{
	// Pretend we painted
	PAINTSTRUCT ps;

	HDC paintDC = BeginPaint(hBaseWnd, &ps);
	EndPaint (hBaseWnd, &ps);
	return true;
}

bool ComboWidget::on_erasebkgnd (HDC hdc)
{
	// Pretend we cleared the background
	return true;
}

// ***** Interfaces *****

METHOD_SPEC_BEGIN(ComboWidget, add_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ComboWidget, add_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, del_string, 1, PS_BOOL)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ComboWidget, del_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, del_named_string, 1, PS_BOOL)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ComboWidget, del_named_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, find_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ComboWidget, find_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, find_partial_string, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_STRING)
METHOD_SPEC_END(ComboWidget, find_partial_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, get_item_string, 1, PS_STRING)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ComboWidget, get_item_string, 1)

METHOD_SPEC_BEGIN(ComboWidget, get_item_data, 1, PS_INT)
METHOD_SPEC_ENTRY(PS_INT)
METHOD_SPEC_END(ComboWidget, get_item_data, 1)

METHOD_SPEC_BEGIN(ComboWidget, get_item_count, 0, PS_INT)
METHOD_SPEC_END(ComboWidget, get_item_count, 0)

METHOD_SPEC_BEGIN(ComboWidget, set_select, 2, PS_BOOL)
METHOD_SPEC_ENTRY(PS_INT),
METHOD_SPEC_ENTRY(PS_BOOL)
METHOD_SPEC_END(ComboWidget, set_select, 2)

METHOD_SPEC_BEGIN(ComboWidget, get_select, 0, PS_INT)
METHOD_SPEC_END(ComboWidget, get_select, 0)

METHOD_TABLE_START(ComboWidget)
METHOD_DEF(ComboWidget, add_string),
METHOD_DEF(ComboWidget, del_string),
METHOD_DEF(ComboWidget, del_named_string),
METHOD_DEF(ComboWidget, find_string),
METHOD_DEF(ComboWidget, find_partial_string),
METHOD_DEF(ComboWidget, get_item_string),
METHOD_DEF(ComboWidget, get_item_data),
METHOD_DEF(ComboWidget, get_item_count),
METHOD_DEF(ComboWidget, set_select),
METHOD_DEF(ComboWidget, get_select)
METHOD_TABLE_END()

// IScriptable Interface, inherited from IWidget
int ComboWidget::method_count(void)
{
	return GET_METHOD_COUNT (ComboWidget, BaseWidget);
}

const char *ComboWidget::method_name(int index)
{
	return GET_METHOD_NAME (index, ComboWidget, BaseWidget);
}

int ComboWidget::method_speclen(int index)
{
	return GET_METHOD_SPECLEN (index, ComboWidget, BaseWidget);
}

const ParamSpec *ComboWidget::method_spec (int index)
{
	return GET_METHOD_SPEC (index, ComboWidget, BaseWidget);
}

bool ComboWidget::invoke (const char *methodName, Variant *result, int paramCount, Variant *params)
{
	bool retVal = false;
	METHOD_INVOKE (ComboWidget, BaseWidget, retVal, methodName, result, paramCount, params);
	return retVal;
}

bool ComboWidget::invokeByIndex (int index, Variant *result, int paramCount, Variant *params)
{
	// NOTE: The given index includes the inherited indexing. For indices < our ancestors count, 
	// just invoke the ancestor.

	if (index < BaseWidget::method_count())
	{
		return BaseWidget::invokeByIndex (index, result, paramCount, params);
	}

	// This is one of ours, so invoke it.
	index -= BaseWidget::method_count();

	switch (index)
	{
	case 0:  // add_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = add_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 1:  // del_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = del_string(params[0].iVal);
				return true;
			}
		}
		break;

	case 2:  // del_named_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = del_named_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 3:  // find_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = find_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 4:  // find_partial_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_STRING)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = find_string(params[0].sVal);
				return true;
			}
		}
		break;

	case 5:  // get_item_string
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_STRING;
				result->spec.tag = 0;
				result->sVal = get_item_string (params[0].iVal);
				return true;
			}
		}
		break;

	case 6:  // get_item_data
		if (paramCount >= 1)
		{
			if (params[0].spec.type == PS_INT)
			{
				result->spec.type = PS_INT;
				result->spec.tag = 0;
				result->iVal = get_item_data (params[0].iVal);
				return true;
			}
		}
		break;

	case 7:  // get_item_count
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_item_count ();
			return true;
		}
		break;

	case 8:  // set_select
		if (paramCount >= 2)
		{
			if (params[0].spec.type == PS_INT && params[1].spec.type == PS_BOOL)
			{
				result->spec.type = PS_BOOL;
				result->spec.tag = 0;
				result->bVal = set_single_select (params[0].iVal, params[1].bVal);
				return true;
			}
		}
		break;

	case 9:  // get_select
		{
			result->spec.type = PS_INT;
			result->spec.tag = 0;
			result->iVal = get_single_select ();
			return true;
		}
		break;
	}

	return false;
}

// IEventSource Interface, inherited from IWidget
EventId ComboWidget::event_count(void)
{
	return COMBO_EVENT_COUNT;
}

const char *ComboWidget::event_name(EventId index)
{
	static char *eventNames[COMBO_EVENT_COUNT] = {"OnChange", "OnDoubleClick"};
	if (index >= 0 && index < COMBO_EVENT_COUNT)
	{
		return eventNames[index];
	}
	return NULL;
}

// ***** Scripting Export *****

static void newComboBox (void)
{
	// Syntax: 
	//     NewComboBox
	//     (
	//         Window <parent>,
	//         number <xpos>, number <ypos>,
	//         number <width>, number <height>,
	//         [string <type>],
	//         [number <sort>]
	//     )
	// Create a scroll and returns it to the scripting language.
	// <parent> is the parent window, nil for none
	// <text> is the button's text
	// <xpos>,<ypos> are the location of the button in parent coordinates
	// <width>,<height> are the width and height of the button
	// <type> is either "s[imple]", "d[ropdown]", or "[drop]l[ist]". It is optional; default is "simple".
	// <sort> is nil for non-sorted, non-nil for sorted.

	// Get and validate the parameters
	lua_Object parent = lua_getparam(1);
	lua_Object xpos = lua_getparam(2);
	lua_Object ypos = lua_getparam(3);
	lua_Object width = lua_getparam(4);
	lua_Object height = lua_getparam(5);
	lua_Object type = lua_getparam(6);
	lua_Object sort = lua_getparam(7);

	// Check the types of the input data before proceeding.
	if (!lua_isnumber(xpos) || !lua_isnumber(ypos))
	{
		return;
	}
	if (!lua_isnumber(width) || !lua_isnumber(height))
	{
		return;
	}

	ComboWidget::ComboType ct = ComboWidget::CMB_SIMPLE;
	if (lua_isnil(type) || type == LUA_NOOBJECT)
	{
		// Defauct value.
		ct = ComboWidget::CMB_SIMPLE;
	}
	else if (lua_isstring(type))
	{
		const char *str = lua_getstring(type);
		switch (*str)
		{
		case 'd':
		case 'D':
			ct = ComboWidget::CMB_DROPBOX;
			break;

		case 's':
		case 'S':
			ct = ComboWidget::CMB_SIMPLE;
			break;

		case 'l':
		case 'L':
			ct = ComboWidget::CMB_DROPLIST;
			break;

		default:
			return;
			break;
		}
	}
	else
	{
		// Invalid type for 'type'.
		return;
	}

	bool doSort = true;
	if (sort != LUA_NOOBJECT && lua_isnil(sort))
	{
		doSort = true;
	}

	// The parameters are valid, so go about creating a new scroll.

	HWND hParent;
	if (lua_tag(parent) == WINDOW_TAG)
	{
		hParent = (HWND) lua_getuserdata(parent);
	}
	else if (lua_tag(parent) == UNITOOL_TAG)
	{
		// Get the widget pointer 
		IWidget *w = get_widget (parent);
		if (!w)
		{
			// This is not a widget. Punt.
			return;
		}

		hParent = w->get_hwnd();
	}
	else
	{
		lua_error ("Invalid parent window for scrollbar!\n");
		return;
	}

	ComboWidget *cw = new ComboWidget;
	assert (cw != NULL && "Failed to allocate a new ComboWidget.");

	if
	(
		cw->create
		(
			(int) lua_getnumber(width), (int) lua_getnumber(height),
			hParent,
			ct, doSort
		)
	)
	{
		// Position the child at the given location.
		cw->set_position ((int) lua_getnumber(xpos), (int) lua_getnumber(ypos));

		// Finally, export the widget.
		export_widget (cw);
	}

	// We failed to allocate a window, so just return.
	return;
}


// ================ Menu Handling Routines =================
static WORD nextMenuId = MENU_BASE_ID;

void addMenuItem (void)
{
	// Syntax: AddMenuItem(Window <win>, string <text>, Menu <owner>, number <position>, function <action>);
	// Adds a new menu item to the given menu.
	// <win> is the window that owns <owner>
	// <text> is the text for the item
	// <owner> is the menu into which the item is placed.
	// <position> is the item before which the item should be placed. -1 will go to the end
	// <action> is the function to call when the menu is selected

	// Get and validate the parameters
	lua_Object win = lua_getparam(1);
	lua_Object name = lua_getparam(2);
	lua_Object owner = lua_getparam(3);
	lua_Object position = lua_getparam(4);
	lua_Object action = lua_getparam(5);

	// Check the types of the input data before proceeding.
	if (!lua_isstring(name))
	{
		return;
	}
	if (!lua_isfunction(action) && !lua_isnil(action))
	{
		return;
	}
	if (!lua_isuserdata(owner) || lua_tag(owner) != MENU_TAG)
	{
		return;
	}
	if (!lua_isnumber(position))
	{
		return;
	}

	HWND hRootWnd = hWndMain;
	if (!lua_isnil(win))
	{
		if (lua_tag(win) == WINDOW_TAG)
		{
			hRootWnd = (HWND) lua_getuserdata(win);
		}
		else if (lua_tag(win) == UNITOOL_TAG)
		{
			IWidget *w = get_widget (win);
			if (w)
			{
				hRootWnd = w->get_hwnd();
			}
		}
	}

	// Retrieve the menu handle from the Menu object.
	HMENU hOwner = (HMENU) lua_getuserdata(owner);

	// Create a reference to the action object, locking it to prevent garbage collection.
	// NOTE: The reference to the nil object is -1.
	// *** TODO: Release the lock at the end of the program.

	lua_pushobject(action);
	int actionRef = lua_ref(TRUE);

	// Insert the item into the owner menu, then redraw the menu.
	MENUITEMINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	mi.fType = MFT_STRING;
	mi.wID = nextMenuId;
	mi.dwTypeData = lua_getstring(name);
	mi.dwItemData = actionRef;
	InsertMenuItem (hOwner, (int) lua_getnumber(position), TRUE, &mi);

	DrawMenuBar (hRootWnd);

	// Return a MENUITEM_TAG object, which is simply the ID of the item, cast to void *
	lua_pushusertag ((void *) nextMenuId, MENUITEM_TAG);
	
	// Make sure the next menu item gets a new id.
	++nextMenuId;
	return;
}

void addMenu (void)
{
	// Syntax: AddMenu(Window <win>, MenuItem <parent>)
	// Adds sub menu to the given item.
	// <win> is the window whose menu owns the menuitem
	// <parent> is the MenuItem which is to be the parent of the new menu.
	// NOTE: The menu is initially unfilled.

	// Get and validate the parameters
	lua_Object win = lua_getparam(1);
	lua_Object parent = lua_getparam(2);

	// Check the types of the input data before proceeding.
	if (!lua_isuserdata(parent) || lua_tag(parent) != MENUITEM_TAG)
	{
		return;
	}

	HWND hRootWnd = hWndMain;
	if (!lua_isnil(win))
	{
		if (lua_tag(win) == WINDOW_TAG)
		{
			hRootWnd = (HWND) lua_getuserdata(win);
		}
		else if (lua_tag(win) == UNITOOL_TAG)
		{
			IWidget *w = get_widget (win);
			if (w)
			{
				hRootWnd = w->get_hwnd();
			}
		}
	}

	HMENU hRootMenu = GetMenu(hRootWnd);
	if (!hRootMenu)
	{
		return;
	}

	// The parameters are valid, so go about creating a new menu.

	// Get the parent menu item ID.
	WORD wID = (WORD) lua_getuserdata(parent);

	// Create a new menu.
	HMENU hMenu = CreateMenu();

	// Set the submenu field for the named menu item.
	// NOTE: Since the item must have already been created, and therefore attached to some
	// submenu of the main menu, we will use the main menu in SetItemData call.

	MENUITEMINFO mi;
	memset(&mi, 0, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_SUBMENU;
	mi.hSubMenu = hMenu;
	SetMenuItemInfo (hRootMenu, wID, FALSE, &mi);

	DrawMenuBar (hRootWnd);

	// Return a MENU_TAG object, which is simply the ID of the item, cast to void *
	lua_pushusertag((void *) hMenu, MENU_TAG);
	return;
}

void newMenu (void)
{
	// Syntax: NewMenu()
	// Creates an empty menu and returns it to lua.

	HMENU hMenu = CreateMenu();
	lua_pushusertag((void *) hMenu, MENU_TAG);
	return;
}

// =========== Misc. Routines ============
static void quitUnitool (void)
{
	lua_Object retVal = lua_getparam(1);
	int result;
	if (lua_isnil(retVal))
	{
		result = 0;
	}
	else
	{
		result = (int) lua_getnumber(retVal);
	}
	PostQuitMessage (result);
}

static void fileLoadDialog (void)
{
	// Syntax: GetOpenFilename(string <title>, string <filter>, string <defaultName>);
	lua_Object title = lua_getparam(1);
	lua_Object filter = lua_getparam(2);
	lua_Object defaultName = lua_getparam(3);

	char buffer[MAX_PATH];
	char *titleStr = NULL;
	char *filterStr = NULL;

	if (defaultName == LUA_NOOBJECT || lua_isnil(defaultName))
	{
		buffer[0] = '\0';
	}
	else
	{
		strcpy (buffer, lua_getstring(defaultName));
	}

	if (title != LUA_NOOBJECT)
	{
		if (!lua_isnil(title))
		{
			titleStr = lua_getstring (title);
		}
	}

	if (filter != LUA_NOOBJECT)
	{
		if (!lua_isnil(title))
		{
			filterStr = lua_getstring (filter);
		}
	}

	OPENFILENAME opfn;
	memset (&opfn, 0, sizeof(opfn));
	opfn.lStructSize = sizeof(opfn);
	opfn.hwndOwner = NULL;
	opfn.lpstrFilter = filterStr;
	opfn.lpstrFile = buffer;
	opfn.nMaxFile = MAX_PATH-1;
	opfn.lpstrTitle = titleStr;
	opfn.Flags =
		OFN_FILEMUSTEXIST |
		OFN_PATHMUSTEXIST;

	if (GetOpenFileName (&opfn))
	{
		lua_pushstring (buffer);
	}

	return;
}

//
// Global routines
//

void init_standard_widgets()
{
	// Register the creation functions for all of the standard widgets.
	lua_register ("NewButton", newButton);
	lua_register ("NewLabel", newLabel);
	lua_register ("NewImage", newImage);
	lua_register ("NewTopLevel", newTopLevel);
	lua_register ("NewText", newText);
	lua_register ("NewFrame", newFrame);
	lua_register ("NewScroll", newScroll);
	lua_register ("NewListBox", newListBox);
	lua_register ("NewComboBox", newComboBox);
	lua_register ("AddMenu", addMenu);
	lua_register ("AddMenuItem", addMenuItem);
	lua_register ("NewMenu", newMenu);
	lua_register ("QuitUnitool", quitUnitool);
	lua_register ("FileLoadDialog", fileLoadDialog);

	// *** Should we export the standard tags here? Doing so would allow LUA scripts to
	// *** do run-time type checking, and perhaps to overload certain functionalities from the scripts.
}
