#ifndef TOOLLOADER_H
#define TOOLLOADER_H
//
// ToolLoader.h - Header file for the module that loads and exposes tools
//

//
// Design Note:
//      In order to properly operate with the scripting language, an interface to the scripting
// language must be provided by UniTool to the plugin. This ensures that both the plugin and UniTool are
// talking to the same interpreter, and prevents having to link the interpreter into the plugin.
//      This is the sequence of events that are performed to load a plugin.
// 1) The plugin DLL is loaded. Although the DLLMain function is called for the DLL, no initialization
//    should be performed there.
// 2) GetProcAddress is called for the plugin_init() function.
// 3) plugin_init() is called with a pointer to the scripting language interface.
// 4) Inside plugin_init(), the plugin initializes itself, then registers its functions and creates its
//    standard objects. It should be noted that since the plugin is talking directly to the scripting
//    interface, UniTool doesn't know about what the plugin is regstering. Perhaps the scripting interface
//    should actually keep some information around before passing the info to the scripting language.
//    It would make debugging easier, and perhaps allow the plugin stuff to usable from straight C++ code.
//
//      The method by which new widgets are created should be standardized. It could either be a
// convention (such as the plugin registering newXXX, where XXX is the type of widget the plugin
// provides, for each widget), or a creation function could be registered via the scripting interface
// for a new() function that takes a string descriptor.
//      I think that the best thing is to provide a scripting interface, which is actually a C++ scripting
// manager that exports stuff to the scripting language, but also handles other things. The interface
// would be directly usable by C++ code to access plugin functionality.
//

// ************ WARNING ************
//    THE CODE BELOW IS OBSOLETE
// ************ WARNING ************

//
// Include files
//

#include "PlugIn.h"
#include "IWidget.h"

//
// Constants
//

const int MAX_PLUGIN_NAME = 64;  // maximum length of a plugin's name

//
// Class and structure definitions
//

//
// Function prototypes
//

// The type of the function exported by a plugin to create a given tool.
// It should be noted that since this does not take any input parameters, the tool will be
// created with a default state, and can be manipulated from there.
typedef IDontKnow *ToolCreator (void);

// Prototypes for the function passed to the plugin by load_plugin(). The plugin will use this
// function to register its exported types.
typedef int REGFUNC (const char *name, ToolCreator *createFunc);
typedef REGFUNC *REGFUNCPTR;

//
// Global routines
//

// Initialization and shutdown routines
extern bool init_tool_loader ();
extern bool shutdown_tool_loader ();

// Registers the given tool name by binding the given name to the given function pointer.
// Fails (returns 0) if a tool by that name already exists. Returns non-zero otherwise.
extern int register_plugin (const char *name, ToolCreator *createFunc);

// Attempts to load the given filename as a plugin.
// In order to be a plugin, it must export a routine with the following prototype:
// int UniToolPlugin (REGFUNCPTR regFuncPtr);
extern bool load_plugin (const char *filename);

// This function creates a new instance of the named tool. Returns true if it succeeds, false
// otherwise. An IDontKnow interface is returned. If more interfaces are needed, they must be queried.
extern bool create_instance (const char *name, IDontKnow **iface);

#endif
