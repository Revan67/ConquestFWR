//
// Script.cpp - Tools for interfacing with the scripting language
//

//
// Design Notes:
//      IWidget, IScriptable, and IEventSource objects are exported as tagged tables.
// Tag methods are used to make sure that table indices are not created with the same name as
// invokable methods on the objects, and events are sent to the lua function with the same name
// as the event.
// 

//
// Include files
//
#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "iwidget.h"
#include "script.h"
#include "lua.h"

//
// Constants
//

static char * WIDGET_IFACE = "widget_interface";
static char * SCRIPTABLE_IFACE = "scriptable_interface";
static char * EVENTSOURCE_IFACE = "eventsource_interface";

//
// Global variables
//

int MENU_TAG;
int MENUITEM_TAG;
int WINDOW_TAG;
int BUTTON_TAG;
int WIDGET_TAG;
int SCRIPTABLE_TAG;
int EVENTSOURCE_TAG;
int INTERFACE_TAG;
int UNITOOL_TAG;  // indicates tables that follow the UNITOOL export conventions for interfaces

//
// Class and structure definitions
//

// *** Perhaps this should move into StdWidget.cpp.
struct WidgetEventSink : public IEventSink
{
	int ref;  // lua reference to the table object.

	WidgetEventSink (lua_Object widgetTable)
	{
		lua_pushobject (widgetTable);
		ref = lua_ref(true);
	}

	~WidgetEventSink ()
	{
		lua_unref (ref);
	}
	
	// IEventSink interface
	virtual void handle_event (EventId id, struct IEventSource *sender, int pCount=0, Variant *params=NULL);
};

//
// Local variables
//

//
// Local routines
//

static void push_variant (Variant &v)
{
	switch (v.spec.type)
	{
	case PS_VOID:
		// Push nil for no return value.
		lua_pushnil();
		break;

	case PS_INT:
		lua_pushnumber(v.iVal);
		break;

	case PS_FLOAT:
		lua_pushnumber(v.fVal);
		break;
		
	case PS_STRING:
		lua_pushstring((char *) v.sVal);
		break;

	case PS_ELLIPSIS:
		assert (false && "PS_ELLIPSIS can't be pushed.");
		break;

	case PS_OBJECT:
		lua_pushusertag(v.oVal, v.spec.tag);
		break;

	case PS_BOOL:
		// Push a non-nil value for true, nil for false.
		if (v.bVal)
		{
			lua_pushnumber(1);
		}
		else
		{
			lua_pushnil();
		}
		break;

	default:
		assert (false && "Invalid variant type");
		break;
	}
}

//
// Methods
//

void WidgetEventSink::handle_event (EventId id, struct IEventSource *sender, int pCount, Variant *params)
{
	// Look up the name of the indexed event among the sender's events.
	// Get the table for the widget via the lua_getref().
	// If the table contains a function entry whose index is the name of the event, 
	// execute that function, otherwise do nothing.

	const char *eventName = sender->event_name (id);
	if (eventName)
	{
		lua_beginblock();
		lua_Object wt = lua_getref(ref);
		if (wt != LUA_NOOBJECT)
		{
			lua_pushobject (wt);
			lua_pushstring ((char *) eventName);
			lua_Object handler = lua_gettable();
			if (lua_isfunction (handler) || lua_iscfunction(handler))
			{
				// Push the standard parameters: the object sending the event and the name of the event
				lua_pushobject (wt);
				lua_pushstring ((char *) eventName);
				// Push any event specific parameters
				if (params != NULL)
				{
					for (int i = 0; i < pCount; ++i)
					{
						push_variant (params[i]);
					}
				}
				// Now call the function.
				lua_callfunction (handler);
				// Don't care about return values for now.
			}
		}
		lua_endblock();
	}
}

//
// Local routines
//

// Function used to invoke methods on IWidgets

static void scriptable_invoke(void)
{
	// The first argument is index the method to invoke (an upvalue).
	// The next argument is the scriptable table object.
	// The rest of the arguments are the arguments to the method being invoked.

	lua_Object method = lua_getparam(1);
	lua_Object table = lua_getparam(2);

	// Get the method name and the scriptable interface pointer.

	int methodIndex = (int) lua_getnumber(method);

	IScriptable *w = get_scriptable (table);
	if (!w)
	{
		lua_error ("NULL Sciptable interface or non-scriptable in scriptable_invoke()!\n");
		return;
	}

	// Get the parameters for the invocation.
	int specLen = w->method_speclen (methodIndex);
	const ParamSpec *spec = w->method_spec (methodIndex);

	// Allocate a parameter array for the invocation.
	Variant *params = new Variant[specLen];

	if (params)
	{
		// Copy the parameters, validating as we go.
		// NOTE: The first entry in the list is the return type.

		int paramLen = specLen - 1;
		for (int i = 1; i < specLen; ++i)
		{
			// Get the next parameter
			lua_Object value = lua_getparam(2 + i);

			// Verify the type.
			bool isRef = ((spec[i].type & PS_REF) != 0);
			switch (spec[i].type & (~PS_REF))
			{
			case PS_VOID:
				assert (false && "PS_VOID not allowed as input parameter.");
				break;

			case PS_INT:
				if (lua_isnumber(value))
				{
					params[i-1].spec = spec[i];
					params[i-1].iVal = (int) lua_getnumber(value);
				}
				else
				{
					fprintf (stderr, "Parameter %d to %s() is not a number.\n", i, w->method_name(methodIndex));
					return;
				}

				break;

			case PS_FLOAT:
				if (lua_isnumber(value))
				{
					params[i-1].spec = spec[i];
					params[i-1].fVal = (float) lua_getnumber(value);
				}
				else
				{
					fprintf (stderr, "Parameter %d to %s() is not a number.\n", i, w->method_name(methodIndex));
					return;
				}
				break;
				
			case PS_STRING:
				if (lua_isstring(value))
				{
					params[i-1].spec = spec[i];
					params[i-1].sVal = lua_getstring(value);
				}
				else
				{
					fprintf (stderr, "Parameter %d to %s() is not a string.\n", i, w->method_name(methodIndex));
					return;
				}
				break;

			case PS_ELLIPSIS:
				// This one must be the last.
				// Count the additional parameters.
				// Allocate a new parameter array.
				// Copy the old parameters.
				// Add the new parameters.
				break;

			case PS_BOOL:
				// Interpret LUA boolean: nil == false, non-nil == true
				params[i-1].spec = spec[i];
				if (lua_isnil(value))
				{
					params[i-1].bVal = false;
				}
				else
				{
					params[i-1].bVal = true;
				}
				break;

			case PS_OBJECT:
				// Verify that the tags match. If they do, pass the user data for the object.
				if (lua_isuserdata(value) && lua_tag(value) == spec[i].tag)
				{
					params[i-1].spec = spec[i];
					params[i-1].oVal = (void *) lua_getuserdata (value);
				}
				else
				{
					fprintf (stderr, "Parameter %d to %s() is not a userdata.\n", i, w->method_name(methodIndex));
					return;
				}
				break;

			default:
				assert (false && "Invalid param spec type");
				break;
			}
		}

		// The params array is filled, so invoke the function.

		Variant result;
		if (w->invokeByIndex(methodIndex, &result, paramLen, params))
		{
			// Push the return value.
			switch (result.spec.type)
			{
			case PS_VOID:
				// Push nil for no return value.
				lua_pushnil();
				break;

			case PS_INT:
				lua_pushnumber(result.iVal);
				break;

			case PS_FLOAT:
				lua_pushnumber(result.fVal);
				break;
				
			case PS_STRING:
				lua_pushstring((char *) result.sVal);
				break;

			case PS_ELLIPSIS:
				assert (false && "PS_ELLIPSIS not allowed as return type.");
				break;

			case PS_OBJECT:
				lua_pushusertag(result.oVal, result.spec.tag);
				break;

			case PS_BOOL:
				// Push a non-nil value for true, nil for false.
				if (result.bVal)
				{
					lua_pushnumber(1);
				}
				else
				{
					lua_pushnil();
				}
				break;

			default:
				assert (false && "Invalid return type");
				break;
			}
		}
		else
		{
			// The invocation failed, so return nil.
			fprintf (stderr, "Failed to invoke %s in scriptable_invoke.\n", w->method_name(methodIndex));
			lua_pushnil();
		}

		// Clean up before returning.
		delete [] params;
		params = NULL;
	}
	else
	{
		fprintf (stderr, "Failed to allocate parameter array in scriptable_invoke().\n");
		lua_pushnil();
	}
}

// Tag methods

static void scriptable_index(void)
{
	// Called on non-existant indices.
	// First arg is table.
	// Second arg is index.
	// NOTE: The top of the stack is 

	// Get the table and index objects.
	lua_Object table = lua_getparam(1);
	char *index = lua_getstring(lua_getparam(2));

	// If the index is not a string, this is an error.
	if (!index)
	{
		lua_error ("Unknown scriptable table index.\n");
		return;
	}

	// Get the scriptable interface pointer.
	IScriptable *w = get_scriptable (table);
	if (!w)
	{
		lua_error ("NULL Scriptable interface or non-scriptable table in scriptable_index()!\n");
		return;
	}

	// See if the index matches any of the invokable methods, push the invocation function as
	// a closure with the given index.
	int count = w->method_count();
	for (int i = 0; i < count; ++i)
	{
		if (!strcmp (index, w->method_name(i)))
		{
			// Push the index onto the stack, then push the invocation function as a closure.
			// This will make the first parameter to the function the index.
			lua_pushnumber (i);
			lua_pushcclosure (scriptable_invoke, 1);
			return;
		}
	}

	// The string doesn't match our methods.
	// NOTE: This is not an error. The proper return value here is nil.
	lua_pushnil();
	return;
}

static void scriptable_settable(void)
{
	// Called on setting of a table index.
	// First arg is table.
	// Second arg is index.
	// Third arg is value.

	// If the index is a string, ensure that it does not match a method for this scriptable.
	// If it does, do nothing.

	// The index doesn't match a method, so just set it.
	// NOTE: Event handlers are just table entries that are functions and whose names match events.
	// The IEventSink for the scriptable just performs a table lookup on the scriptable with the event as
	// the index. If there is a matching entry, and it is a function, it gets called.

	// Get the table and index objects.
	lua_Object table = lua_getparam(1);
	lua_Object index = lua_getparam(2);
	lua_Object value = lua_getparam(3);

	// If the index is a string, check it against the method names for the scriptable.
	if (lua_isstring (index))
	{
		const char *indexStr = lua_getstring(index);

		// Get the scriptable interface pointer.
		IScriptable *w = get_scriptable (table);
		if (!w)
		{
			lua_error ("NULL Scriptable interface or non-scriptable table in scriptable_settable()!\n");
			return;
		}

		// Look for a match among the methods. If one is found, simply return, doing nothing.
		int count = w->method_count();
		for (int i = 0; i < count; ++i)
		{
			if (!strcmp (indexStr, w->method_name(i)))
			{
				// Don't set the value
				return;
			}
		}
	}

	// If here, then the value is ok to set. Set it using lua_rawsettable().
	lua_pushobject (table);
	lua_pushobject (index);
	lua_pushobject (value);
	lua_rawsettable ();
}

// Object creation methods
static void push_widget (IWidget *w)
{
	// Create a table for this widget.
	// The interface is stored as a userdata with INTERFACE_TAG, indexed as WIDGET_IFACE
	// NOTE: This is also a scriptable, so also set the SCRIPTABLE_IFACE table member
	lua_Object wt = lua_createtable();
	lua_pushobject (wt);
	lua_pushstring (WIDGET_IFACE);
	lua_pushusertag (w, INTERFACE_TAG);
	lua_rawsettable ();
	lua_pushobject (wt);
	lua_pushstring (SCRIPTABLE_IFACE);
	lua_pushusertag (w, INTERFACE_TAG);
	lua_rawsettable ();

	// Create a new event handler, just for this widget.
	// *** TODO: Figure out how to delete this when the widget gets deleted.
	w->set_event_handler (new WidgetEventSink (wt));

	// Set the tag to UNITOOL_TAG.
	lua_pushobject (wt);
	lua_settag (UNITOOL_TAG);

	// Push the widget onto the stack for the final time, ready for a return value.
	lua_pushobject (wt);
}

static void push_scriptable (IScriptable *w)
{
	// Create a table for this widget.
	// The interface is stored as a userdata with INTERFACE_TAG, indexed as SCRIPTABLE_IFACE.
	lua_Object wt = lua_createtable();
	lua_pushobject (wt);
	lua_pushstring (SCRIPTABLE_IFACE);
	lua_pushusertag (w, INTERFACE_TAG);
	lua_rawsettable ();

	// Set the tag to UNITOOL_TAG.
	lua_pushobject (wt);
	lua_settag (UNITOOL_TAG);

	// Push the widget onto the stack for the final time, ready for a return value.
	lua_pushobject (wt);
}

//
// Routines
//

bool init_scripting ()
{
	// Initialize LUA and the standard LUA libraries
	lua_open ();
	lua_strlibopen ();
	lua_mathlibopen ();
	lua_iolibopen ();

	// Set the values of all of the standard tags.
	MENU_TAG = lua_newtag();
	MENUITEM_TAG = lua_newtag();
	WINDOW_TAG = lua_newtag();
	BUTTON_TAG = lua_newtag();
	WIDGET_TAG = lua_newtag();
	SCRIPTABLE_TAG = lua_newtag();
	EVENTSOURCE_TAG = lua_newtag();
	INTERFACE_TAG = lua_newtag();
	UNITOOL_TAG = lua_newtag();

	// Install the tag methods for widgets
	lua_pushcfunction (scriptable_index);
	lua_settagmethod (UNITOOL_TAG, "index");
	lua_pushcfunction (scriptable_settable);
	lua_settagmethod (UNITOOL_TAG, "settable");

	return true;
}

bool export_scriptable (IScriptable *s)
{
	push_scriptable (s);
	return true;
}

bool export_eventsource (IEventSource *es)
{
	return false;
}

bool export_widget (IWidget *w)
{
	push_widget (w);
	return true;
}

IWidget *get_widget (lua_Object obj)
{
	if (!lua_istable (obj) || lua_tag(obj) != UNITOOL_TAG)
	{
		return NULL;
	}
	lua_pushobject (obj);
	lua_pushstring (WIDGET_IFACE);
	IWidget *w = (IWidget *) lua_getuserdata(lua_rawgettable ());
	return w;
}

IScriptable *get_scriptable (lua_Object obj)
{
	if (!lua_istable (obj) || lua_tag(obj) != UNITOOL_TAG)
	{
		return NULL;
	}
	lua_pushobject (obj);
	lua_pushstring (SCRIPTABLE_IFACE);
	IScriptable *s = (IScriptable *) lua_getuserdata(lua_rawgettable ());
	return s;
}

IEventSource *get_eventsource (lua_Object obj)
{
	if (!lua_istable (obj) || lua_tag(obj) != UNITOOL_TAG)
	{
		return NULL;
	}
	lua_pushobject (obj);
	lua_pushstring (EVENTSOURCE_IFACE);
	IEventSource *es = (IEventSource *) lua_getuserdata(lua_rawgettable ());
	return es;
}

