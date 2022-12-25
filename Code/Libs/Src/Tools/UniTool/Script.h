#ifndef SCRIPT_H
#define SCRIPT_H
//
// Script.h - Tools for interfacing UniTool objects to the scripting language.
//

//
// Include files
//
extern "C" 
{
#include "lua.h"
#include "luadebug.h"
#include "lualib.h"
}

//
// Constants
//

// Data tags
extern int MENU_TAG;
extern int MENUITEM_TAG;
extern int WINDOW_TAG;
extern int BUTTON_TAG;
extern int WIDGET_TAG;
extern int SCRIPTABLE_TAG;
extern int EVENTSOURCE_TAG;
extern int INTERFACE_TAG;
extern int UNITOOL_TAG;

//
// Interface definitions
//

// *** TODO: Write the IScripting interface. It will contain methods for exporting objects and
// *** functions to UniTool.

//
// Global functions
//

// Call this to initialize the scripting system
extern bool init_scripting ();

// These handle the exportation of the methods and events on the given objects to
// the scripting language.
extern bool export_scriptable (struct IScriptable *s);
extern bool export_eventsource (struct IEventSource *es);
extern bool export_widget (struct IWidget *w);

// Handy functions for mapping from lua objects to interfaces
extern struct IWidget *get_widget (lua_Object obj);
extern struct IScriptable *get_scriptable (lua_Object obj);
extern struct IEventSource *get_eventsource (lua_Object obj);

#endif
