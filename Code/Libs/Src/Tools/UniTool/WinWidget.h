#ifndef WINWIDGET_H
#define WINWIDGET_H
//
// WinWiget.h - Defines the base class for Win32 widgets
//

//
// Include files
//

#include "iwidget.h"

//
// Constants
//

const WORD MENU_BASE_ID = 2000;  // base id for Widget menus

//
// Class and structure definitions
//

// Widget is the base class of all Win32 widgets.  It provides a basic implementation of the
// interfaces, defines a set of functions for manipulating the Widget, and implements the
// default widget manipulation code.

typedef unsigned int MouseFlags;
const MouseFlags MF_LEFT   = 1;
const MouseFlags MF_RIGHT  = 2;
const MouseFlags MF_MIDDLE = 4;
const MouseFlags MF_UP     = 8;
const MouseFlags MF_DOWN   = 16;

struct BaseWidget : public IWidget
{
private:
	static bool classRegistered;
	static LRESULT CALLBACK widgetProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); 

protected:
	char *       textBuffer;   // pointer to the cached text of this window.
	IEventSink * eventHandler; // pointer to the event handling interface

public:
	HWND  hBaseWnd;   // this widget's window handle.
	DWORD esAppData;  // IEventSource app specific data

public:
	// Static methods
	static void register_wnd_class();

	// Constructors and Destructors
	// NOTE: The constructors do NOT create the window for the widgets. Use Create() to
	// actually create the window.
	BaseWidget();
	virtual ~BaseWidget();

	// === Message Handlers ===
	// A virtual function which gets called from widgetProc() to handle every message.
	// This allows an inheritor to completely override the default message handling.
	// The base class implementation feeds the handlers below.
	virtual LRESULT handle_message (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// These methods are handlers for common messages, allowing an inheritor to overload only
	// those messages it wants.
	virtual LRESULT on_create (LPCREATESTRUCT lpcs); // WM_CREATE
	virtual bool on_destroy (); // WM_DESTROY
	virtual bool on_command (WORD wNotifyCode, WORD wID, HWND hwndCtl); // WM_COMMAND
	virtual bool on_menu (WORD wID);  // WM_COMMAND, wNotifyCode == 0
	virtual bool on_accel (WORD wID); // WM_COMMAND, wNotifyCode == 1
	virtual bool on_size (WPARAM fwSizeType, WORD nWidth, WORD nHeight); // WM_SIZE
	virtual bool on_buttondown (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos);  // WM_{L,R,M}BUTTONDOWN
	virtual bool on_buttonup (MouseFlags fMouse, WPARAM fwKeys, WORD xPos, WORD yPos);    // WM_{L,R,M}BUTTONUP
	virtual void on_mousemove (WPARAM fwKeys, WORD xPos, WORD yPos);   // WM_MOUSEMOVE
	virtual bool on_move(int xPos, int yPos); // WM_MOVE
	virtual LRESULT on_notify (int idCtrl, LPNMHDR pnmh); // WM_NOTIFY
	virtual bool on_paint (HDC hdc); // WM_PAINT
	virtual bool on_erasebkgnd (HDC hdc); // WM_ERASEBKGND

	// === Methods ===
	// NOTE:These are mostly wrappers around Win32 calls.

	// Creation and destruction methods
	virtual bool create(DWORD exStyle, DWORD style, HWND parent, int w, int h);
	virtual bool destroy();

	// Size and position manipulation functions
	virtual void set_position (int x, int y);
	virtual POINT get_position();
	virtual void set_size (int w, int h);
	virtual SIZE get_size();

	// Style manipulation
	virtual void set_style (DWORD newStyle);
	virtual void set_exstyle (DWORD newExStyle);
	virtual DWORD get_style();
	virtual DWORD get_exstyle();

	// Text manipulation
	virtual const char *get_text();
	virtual void set_text(const char *newText);

	// Visibility methods
	virtual void set_visible(bool makeVisible=true);
	virtual bool is_visible();

	// Enabling methods
	virtual void set_enabled(bool makeEnabled=true);
	virtual bool is_enabled();

	// Event methods
	virtual bool fire_event (EventId event, int pCount=0, Variant *params=NULL);

	// === Supported Interfaces === 
	// IDontKnow Interface
	virtual bool query_interface (const char *name, void **iface);

	// IWidget Interface
	virtual HWND get_hwnd();
	virtual SIZE get_minsize();
	virtual SIZE get_maxsize();
	virtual SIZE get_prefsize();

	// IScriptable Interface, inherited from IWidget
	virtual int method_count(void);
	virtual const char *method_name(int index);
	virtual int method_speclen(int index);
	virtual const ParamSpec *method_spec (int index);
	virtual bool invoke (const char *methodName, Variant *result, int paramCount, Variant *params);
	virtual bool invokeByIndex (int index, Variant *result, int paramCount, Variant *params);

	// IEventSource Interface, inherited from IWidget
	virtual EventId event_count(void);
	virtual const char *event_name(EventId index);
	virtual void set_app_data (DWORD data);
	virtual DWORD get_app_data ();
	virtual IEventSink *set_event_handler (IEventSink *sink);
};

#endif

