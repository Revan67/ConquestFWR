//
// ToolLoader.cpp - Source for the tool loading functions
//

//
// Include files
//

#include "ToolLoader.h"

#include <string.h>

//
// Class and structure definitions
//

struct PlugInEntry
{
	PlugInEntry * next;
	char          name[MAX_PLUGIN_NAME];
	ToolCreator * creator;
};

//
// Local variables
//

PlugInEntry plugins = NULL;

//
// Routines
//

bool init_tool_loader ()
{
	// Clear out the plugin list.
	plugins.next = NULL;
	strcpy(plugins.name, "Bogus:ListHead");
	plugins.creator = NULL;
}

bool shutdown_tool_loader ()
{
	// Delete all plugin entries.
	PlugInEntry *here = plugins.next;
	plugins.next = NULL;
	while (here)
	{
		PlugInEntry *next = here->next;
		here->next = NULL;
		delete here;
		here = next;
	}
}

int register_plugin (const char *name, ToolCreator *createFunc)
{
	// Hunt through the plugins, looking for a match. If one is found, return false.

	PlugInEntry *here = plugins.next;
	while (here)
	{
		if (!strcmp (here->name, name))
		{
			return (int) false;
		}
		here = here->next;
	}

	// The name does not match, so allocate a new one and add it to the head of the list.

	PlugInEntry *newOne = new PlugInEntry;
	strncpy(newOne->name, name, MAX_PLUGIN_NAME-1);
	newOne->name[MAX_PLUGIN_NAME-1] = '\0';
	newOne->creator = createFunc;
	newOne->next = plugins.next;
	plugins.next = newOne;
	return (int) true;
}

// Attempts to load the given filename as a plugin.
// In order to be a plugin, it must export a routine with the following prototype:
// int UniToolPlugin (REGFUNCPTR regFuncPtr);
bool load_plugin (const char *filename)
{
	// *** TODO: Perform a LoadLibrary() on the given filenmame. If it succeeds, 
	// *** do GetProcAddress() on "UniToolPlugin". If that passes, call it with
	// *** register_plugin() as the parameter.
	return false;
}

// This function creates a new instance of the named tool. Returns true if it succeeds, false
// otherwise. An IDontKnow interface is returned. If more interfaces are needed, they must be queried.
bool create_instance (const char *name, IDontKnow **iface)
{
	// Run through the list, looking for the named type. If it is found, call the creation function
	// and return the result.

}

// Routines for publishing widgets, scriptables, and event sources
// *** Is is possible under LUA to add to an object's table after it has been created? If so, then
// *** each of the routines below should take an object handle and should append its stuff to that
// *** object's table.
bool export_scriptable (IScriptable *s)
{
}

bool export_eventsource (IEventSource *es)
{
}

bool export_widget (IWidget *w)
{
}

