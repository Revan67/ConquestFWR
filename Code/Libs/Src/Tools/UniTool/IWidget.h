#ifndef IWIDGET_H
#define IWIDGET_H
//
// IWidget.h - IWidget interface definition
//

//
// Design Notes:
//        This module defines the interface used by UniTool to manipulate visual components, or
// widgets. It is assumed that a widget is implemented as a Win32 window class, and therefore each
// widget can return a window handle when needed.
//        Since Widgets are also windows, it is legal to manipulate them via the standard Win32
// calls. Widget implementations should be aware of this and do the right thing. For this reason,
// the widget interface does not expose any functionality that is available via Win32. IWidget
// provides information needed in order to lay out components in an intelligent manner.
//        UniTool will export a standard set of Win32 window properties for a widget. Any other
// properties for the widget will have to be exported via methods in the IScriptable interface.
// All widgets are scriptable.
//        Note that every Widget is both scriptable and a source of events. The specific events
// and functions 
//

// NOTE: For now, this is not derived from DACOM, but it expected to at some point in the future.

//
// Include files
//

#include "plugin.h"

//
// Interface Definitions
//

struct IWidget : public IScriptable, public IEventSource, public IDontKnow
{
	// Returns the HWND of the widget.
	virtual HWND get_hwnd() PURE_VIRTUAL;

	// Returns information used to layout the component
	// A SIZE of all zeros indicates "I dont care".
	virtual SIZE get_minsize() PURE_VIRTUAL;
	virtual SIZE get_maxsize() PURE_VIRTUAL;
	virtual SIZE get_prefsize() PURE_VIRTUAL;
};

#endif