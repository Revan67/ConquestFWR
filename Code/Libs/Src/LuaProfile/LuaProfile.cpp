//
// LuaProfile.cpp - An IProfileParser object that uses LUA as the file parser, allowing complex configurations to
// be coded and used where IProfileParser objects are used.
//

//
// Design Notes:
//      At some point, this object should also support a scripting engine interface or a LUA specific interface.
// This other interface would allow correct access to the data space as a set of scalars, tables, and functions.
// This could be done by expanding the DAVARIANT class to include a ScriptTable and ScriptFunction type.
//      Each instance of LuaProfile has its own parser space, ensuring that different input files don't pollute each
// other.
//      Sections are mapped to the global tables, where the table's name is the section name and its members are
// the keys. The special section "$" denotes the global variable space, allowing global variables to be retrieved via
// ReadKeyValue.
//      Since IProfileParser only returns strings, numbers and strings are returned as strings, and table and function
// values are ignored by ReadKeyValue. ReadProfileLine will return the Nth member of the section table that is not a
// function or table, in key=value form.
//

//
// Include files
//

#include <windows.h>
#include <IProfileParser.h>
#include <tcomponent.h>
#include <HeapObj.h>
#include <ilua.h>
#include <fdump.h>

extern "C" 
{
#include "lua.h"
#include "luadebug.h"
#include "lualib.h"
}

#ifndef NULL
#define NULL 0L
#endif

//
// Class and structure definitions
//

struct LuaSection
{
	LuaSection *next; // the next section in our list of allocated sections.
	int ref;          // reference to the locked LUA table for this section, -1 means the global space

	LuaSection ()
	{
		next = NULL;
		ref = -1;
	}

	~LuaSection ()
	{
		set_ref (-1);
	}

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "LuaSection");
	}

	void set_ref (int _ref)
	{
		if (ref != -1)
		{
			lua_unref (ref);
		}
		ref = _ref;
	}

	int get_key (const C8 *keyName, C8 *buffer, U32 bufferSize)
	{
		lua_Object value;

		if (ref == -1)
		{
			// Get the named global variable
			value = lua_getglobal ((char *) keyName);
		}
		else
		{
			// Get the named member of the referenced table
			lua_Object table = lua_getref (ref);
			if (table == LUA_NOOBJECT)
			{
				return 0;
			}

			lua_pushobject (table);
			lua_pushstring ((char *) keyName);
			value = lua_gettable();
		}

		char *str = lua_getstring(value);
		if (str == NULL)
		{
			return 0;
		}

		if (buffer != NULL && bufferSize != 0)
		{
			strncpy (buffer, str, bufferSize);
			buffer[bufferSize-1] = '\0';
			return strlen(buffer);
		}

		return strlen(str);
	}

	int get_line (U32 lineNumber, C8 *buffer, U32 bufferSize)
	{
		lua_Object key;
		lua_Object value;
		if (ref == -1)
		{
			// Get the nth string or number variable in the LUA state.

			// Use the pre-defined LUA function __get_profile_line(i) to retrieve the
			// ith line's key and value

			lua_Object func = lua_getglobal("__get_profile_line");
			if (func == LUA_NOOBJECT)
			{
				return 0;
			}

			lua_pushnumber (lineNumber);
			if (lua_callfunction != 0)
			{
				// The function failed for some reason
				return 0;
			}

			key = lua_getresult(1);
			value = lua_getresult(2);
		}
		else
		{
			// Get the nth stirng or number member of the referenced table
			lua_Object table = lua_getref (ref);
			if (table == LUA_NOOBJECT)
			{
				return 0;
			}

			// Use the pre-defined LUA function __get_table_line(table, i) to retrieve the
			// ith line's key and value

			lua_Object func = lua_getglobal("__get_table_line");
			if (func == LUA_NOOBJECT)
			{
				return 0;
			}

			lua_pushobject (table);
			lua_pushnumber (lineNumber);
			if (lua_callfunction(func) != 0)
			{
				// The function failed for some reason
				return 0;
			}

			key = lua_getresult(1);
			value = lua_getresult(2);
		}

		if (key == LUA_NOOBJECT || value == LUA_NOOBJECT || lua_isnil(key))
		{
			return 0;
		}

		char *str = lua_getstring(key);
		if (str == NULL)
		{
			return 0;
		}

		if (buffer == NULL || bufferSize == 0)
		{
			int len = strlen(str);
			str = lua_getstring(value);
			if (str != NULL)
			{
				len += strlen(str);
			}
			++len;  // for '='
			return len;
		}
		else
		{
			U32 keylen = strlen(str);

			if (keylen < bufferSize)
			{
				strncpy (buffer, str, keylen);
				if (keylen + 1 < bufferSize)
				{
					buffer[keylen++] = '=';
				}

				str = lua_getstring(value);
				if (str != NULL)
				{
					strncpy (buffer + keylen, str, bufferSize - keylen);
				}
			}
			else
			{
				strncpy (buffer, str, bufferSize);
			}

			buffer[bufferSize-1] = '\0';

			return strlen(buffer);
		}
	}
};

struct LuaProfile :  public ILua, public IProfileParser, public IAggregateComponent
{
public:
	BEGIN_DACOM_MAP_INBOUND(LuaProfile)
	DACOM_INTERFACE_ENTRY(ILua)
	DACOM_INTERFACE_ENTRY(IProfileParser)
	DACOM_INTERFACE_ENTRY(IAggregateComponent)
	END_DACOM_MAP()

	// Data members
	lua_State *   myState;
	LuaSection    root;    // dummy first section in the list.

public:
	LuaProfile ();
	~LuaProfile ();

	static void error_handler (void);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "LuaProfile");
	}

	// Methods need by the template code
	GENRESULT init (AGGDESC * desc)
	{
		return GR_OK;
	}

	IDAComponent * getBase (void)
	{
		return static_cast<IProfileParser *>(this);
	}

	// === IAggregateComponent methods ===
	DEFMETHOD(Initialize) (void)
	{
		return GR_OK;
	}
		
	// === ILua methods ===
	DEFMETHOD(CallFunction) (const C8 *funcName, DACOM_VARIANT *result, int paramCount, DACOM_VARIANT *params[]);

	// === IProfileParser methods ===
	DEFMETHOD(Initialize) (const C8 *fileName, ACCESS access = READ_ACCESS );
	DEFMETHOD_(BOOL32,EnumerateSections) (ENUM_PROC proc = 0, void *context=0);
	DEFMETHOD_(HANDLE,CreateSection) (const C8 *sectionName, CREATE_MODE mode = PP_OPENEXISTING);
	DEFMETHOD_(BOOL32,CloseSection) (HANDLE hSection);
	DEFMETHOD_(U32,ReadProfileLine) (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize);
	DEFMETHOD_(U32,ReadKeyValue) (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize);
};

//
// Methods
//

LuaProfile::LuaProfile ()
{
	// Create a new state for this instance.
	lua_State *old = lua_setstate(NULL);
	lua_open ();
//	lua_strlibopen ();
//	lua_mathlibopen ();
//	lua_iolibopen ();

	// Register the hardwired methods we will be using.
	static char *get_profile_line =
		"function __get_profile_line (i)"
		"	local n, v = nextvar(nil)"
		"	while n do"
		"		local t = type(v);"
		"		if (t == \"string\" or t == \"number\") then"
		"			if i == 0 then"
		"				return n, v;"
		"			else"
		"				i = i - 1;"
		"			end"
		"		end"
		"		n, v = nextvar(n);"
		"	end"
		"	return nil;"
		"end";


	static char *get_table_line =
		"function __get_table_line (table, i)"
		"	local n, v = next(table, nil)"
		"	while n do"
		"		local t = type(v);"
		"		if (t == \"string\" or t == \"number\") then"
		"			if i == 0 then"
		"				return n, v;"
		"			else"
		"				i = i - 1;"
		"			end"
		"		end"
		"		n, v = next(table, n);"
		"	end"
		"	return nil;"
		"end";

	lua_dostring (get_profile_line);
	lua_dostring (get_table_line);

	// Send errors to FDUMP
	lua_pushcfunction(error_handler);
	lua_seterrormethod();

	// Restore the old state and preserve ours.
	myState = lua_setstate(old);
}

LuaProfile::~LuaProfile ()
{
	lua_State *old = lua_setstate(myState);

	// Destroy all of the allocated sections
	LuaSection *here = root.next;
	while (here)
	{
		LuaSection *next = here->next;
		delete here;
		here = next;
	}

	// Clean up lua
	lua_close ();
	if (old != myState)
	{
		lua_setstate(old);
	}
}

void LuaProfile::error_handler (void)
{
	// Retrieve the error from the current lua stack.
	lua_Object err = lua_getparam(1);
	if (lua_isstring(err))
	{
		GENERAL_ERROR (lua_getstring(err));
	}
}

GENRESULT COMAPI LuaProfile::Initialize (const C8 *fileName, ACCESS access)
{
	if (access != READ_ACCESS)
	{
		return GR_GENERIC;
	}

	// "Do" the filename, LUA style.
	lua_State *old = lua_setstate(myState);
	lua_dofile ((char *) fileName);
	lua_setstate(old);
	return GR_OK;
}

BOOL32 COMAPI LuaProfile::EnumerateSections (ENUM_PROC proc, void *context)
{
	// We must have a valid proc to continue.
	if (proc == NULL)
	{
		return FALSE;
	}

	// Enumerate all of the tables in the global LUA state.
	lua_State *old = lua_setstate(myState);

	// Get the name of the first variable. The value of the variable is on the lua output stack.
	char *name = lua_nextvar (NULL);
	while (name != NULL)
	{
		lua_Object val = lua_getresult(1);
		lua_Object v;
#if 0
		// This code doesn't work for some reason. The lua_nextvar() function is not documented, so I am
		// probably using it wrong. -TNB
		v = val;
#else
		v = lua_getglobal (name);
#endif
		if (lua_istable (v))
		{
			// This is a global table, which we map to sections. Call the enum proc.
			if (!(*proc) (this, name, context))
			{
				return FALSE;
			}
		}

		name = lua_nextvar (name);
	}
	lua_setstate(old);
	return TRUE;
}

HANDLE COMAPI LuaProfile::CreateSection (const C8 *sectionName, CREATE_MODE mode)
{
	HANDLE result = NULL;
	if (mode == PP_OPENEXISTING)
	{
		lua_State *old = lua_setstate(myState);
		
		// If the section name is the special "$" section, simply create a -1 ref section

		LuaSection *section = NULL;
		if (sectionName[0] == '$' && sectionName[1] == '\0')
		{
			section = new LuaSection();
		}
		else
		{
			// Get the LUA object with the same name as the section.
			lua_Object table = lua_getglobal ((char *) sectionName);
			if (table != LUA_NOOBJECT && lua_istable(table))
			{
				// Get a reference to the object, then store it into a new LuaSection object
				lua_pushobject (table);
				int ref = lua_ref (false);
				section = new LuaSection();
				section->set_ref (ref);
			}
		}
		
		if (section)
		{
			// Add the section to the section list and return it.
			section->next = root.next;
			root.next = section;
			result = (HANDLE) section;
		}
		lua_setstate(old);
	}
	return result;
}

BOOL32 COMAPI LuaProfile::CloseSection (HANDLE hSection)
{
	if (hSection != NULL)
	{
		// Ensure that this is one of our sections before deleting it.
		LuaSection *here = root.next;
		LuaSection *prev = &root;

		while (here)
		{
			if (here == (LuaSection *) hSection)
			{
				prev->next  = here->next;
				
				// WARNING: Must have the LUA state set properly before deleting a section
				lua_State *old = lua_setstate(myState);
				delete here;
				lua_setstate(old);
				return TRUE;
			}
			prev = here;
			here = here->next;
		}
	}
	return FALSE;
}

U32 COMAPI LuaProfile::ReadProfileLine (HANDLE hSection, U32 lineNumber, C8 * buffer, U32 bufferSize)
{
	U32 count = 0;
	if (hSection != NULL)
	{
		LuaSection *section = (LuaSection *) hSection;
		lua_State *old = lua_setstate(myState);
		count = section->get_line (lineNumber, buffer, bufferSize);
		lua_setstate(old);
	}
	return count;
}

U32 COMAPI LuaProfile::ReadKeyValue (HANDLE hSection, const C8 * keyName, C8 * buffer, U32 bufferSize)
{
	U32 count = 0;
	if (hSection != NULL)
	{
		LuaSection *section = (LuaSection *) hSection;
		lua_State *old = lua_setstate(myState);
		count = section->get_key (keyName, buffer, bufferSize);
		lua_setstate(old);
	}
	return count;
}

GENRESULT COMAPI LuaProfile::CallFunction (const C8 *funcName, DACOM_VARIANT *result, int paramCount, DACOM_VARIANT *param[])
{
	lua_State *old = lua_setstate(myState);

	lua_Object f = lua_getglobal ((char *) funcName);
	if (f == LUA_NOOBJECT || !lua_isfunction(f))
	{
		return GR_INVALID_PARMS;
	}

	// Push the parameters onto the lua stack
	for (int i = 0; i < paramCount; ++i)
	{
		// NOTE: Only strings and numbers are allowed
		if (param[i]->variantType == DAVT_STRING)
		{
			lua_pushstring ((char *) ((const C8 *) *param[i]));
		}
		else
		{
			lua_pushnumber (*param[i]);
		}
	}

	lua_callfunction(f);

	if (result != NULL)
	{
		lua_Object ret = lua_getresult(1);
		if (!lua_isnil(ret) && result != NULL)
		{
			if (lua_isstring(ret))
			{
				*result = DACOM_VARIANT (strdup(lua_getstring(ret)));
			}
			else
			{
				*result = DACOM_VARIANT (lua_getnumber(ret));
			}
		}
	}

	lua_setstate(old);

	return GR_OK;
}

//=============================================================================
//
void SetDllHeapMsg (HINSTANCE hInstance)
{
   DWORD dwLen;
   char buffer[260];
   
   dwLen = GetModuleFileName(hInstance, buffer, sizeof(buffer));
 
   while (dwLen > 0)
   {
      if (buffer[dwLen] == '\\')
      {
         dwLen++;
         break;
      }
      dwLen--;
   }

   SetDefaultHeapMsg(buffer+dwLen);
}

//****************************************************************************
//*                                                                          *
//*  DLLMain() called on startup/shutdown                                    *
//*                                                                          *
//****************************************************************************
//
BOOL COMAPI DllMain(HINSTANCE hinstDLL,  //)
                    DWORD     fdwReason,
                    LPVOID    lpvReserved)
{
   IComponentFactory *server;
   static char interface_name[] = "ILua";

   switch (fdwReason)
      {
      //
      // DLL_PROCESS_ATTACH: Create object server component and register it 
      // with DACOM manager
      //

      case DLL_PROCESS_ATTACH:

			HEAP = HEAP_Acquire();
			SetDllHeapMsg(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<LuaProfile>, AGGDESC> (interface_name);
			DACOM_Acquire()->RegisterComponent(server, interface_name, DACOM_LOW_PRIORITY);
			server->Release();

			break;

      case DLL_PROCESS_DETACH:
         break;
      }

   return TRUE;
}

