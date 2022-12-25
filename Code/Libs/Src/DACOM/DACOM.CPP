 //--------------------------------------------------------------------------//
//                                                                          //
//                              DACOM.CPP                                   //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
$Author: Jasony $

  DA COM Manager Implementation
  
	Modified 13-Jan-97 JMiles:
	- Removed arbitrary limitations on # of libraries and components
	- Cleaned up acquire/release process (not thoroughly tested yet)
	- Replaced Windows-specific types with standard DA types
	- Added some comments
	
	  Modified 10-Mar-97 JMiles:
      - Replaced buggy version of LList template with stddat.h
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "libver.h"
#include "dacom.h"
#include "IProfileParser.h"
#include "FDump.h"
#include "TempStr.h"
#include "stddat.h"

//
HINSTANCE hInstance;

extern "C" 
{
	DXDEC DA_ERROR_HANDLER FDUMP;
}

//

int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

//

#define DA_SWITCH_CHAR '\\'

//--------------------------------------------------------------------------//
//------------------------DACOManager class definition----------------------//
//--------------------------------------------------------------------------//

//
// Win32 DLL managed by DACOM
//

struct ADDED_LIBRARY
{
	HINSTANCE instance;                 // Win32 DLL instance
	C8        base_name[12];            // Short library filename prefix
	
	ADDED_LIBRARY *next;                // Used by LList template
	ADDED_LIBRARY *prev;
};

//
// Registered component object
//

struct REGISTERED_OBJECT
{   
	IComponentFactory *component;       // Component interface
	const C8          *interface_name;  // Name of interface
	U32                priority;        // Implementation priority
	ADDED_LIBRARY	  *library;		    // may be null if unknown
	
	REGISTERED_OBJECT *next;            // Used by LList template
	REGISTERED_OBJECT *prev;
};

//
// Component object manager class
//

static C8 *DACOManager_interface_name = "ICOManager";

struct DACOManager : public ICOManager
{
	U32 reference_cnt;         // # of instance pointers acquired+1
	BOOL32 initialized;		   // have the DLL's been loaded?	

	// data used during AddLibrary()
	U32 registration_cnt;            // # of components registered by DLLLoad()
	ADDED_LIBRARY *pCurrentLibrary;	 // temp pointer to DLL being loaded
	
	LList <REGISTERED_OBJECT> object_list;
	LList <ADDED_LIBRARY>     library_list;
	
	IProfileParser2 * parser;
	IDAComponent   * innerParser;
	
	//
	// Constructor initializes reference count to 0
	//
	
	DACOManager(void);
	
	//
	// IDAComponent methods
	//
	
	DEFMETHOD(QueryInterface)  (const C8 *interface_name, void **instance);
	
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	
	
	//
	// IDACOManager methods
	//
	
	DEFMETHOD(RegisterComponent)	  (IComponentFactory *component, 
		const C8     *interface_name, 
		U32           priority = DACOM_NORMAL_PRIORITY);
	
	DEFMETHOD(UnregisterComponent) (IComponentFactory *component, 
		const C8     *interface_name = NULL);
	
	DEFMETHOD(EnumerateComponents) (const C8     *interface_name, 
		DACOMENUMCALLBACK callback, 
		void             *context = NULL);
	
	DEFMETHOD(AddLibrary)          (const C8 *DLL_filename);
	
	DEFMETHOD(RemoveLibrary)       (const C8 *DLL_filename);
	
	DEFMETHOD(ShutDown)            (void);
	
	DEFMETHOD(SetINIConfig)        (const C8 *info, U32 flags=0);
	
	BOOL32 LoadAllFromDirectory (const C8 *pathName, const C8 * searchName);

	void LoadAllFromPath (const C8 *searchName);
	
	BOOL32 initialize (void);
	
	void UnregisterRelatedComponents (ADDED_LIBRARY *library);

	static BOOL32 __stdcall GetAbsolutePath (C8 *lpOutput, const C8 *lpBaseDir, const C8 *lpInput, LONG lSize);
};

//--------------------------------------------------------------------------//
//---------------------------DACOManager Methods----------------------------//
//--------------------------------------------------------------------------//
void RegisterHeap (ICOManager *pManager);		// extern to the heap manager module
IComponentFactory * CreateProfileParserFactory (void);  // extern to profile parser
IComponentFactory * CreateProfileParserFactory2 (void);  // extern to profile parser

//--------------------------------------------------------------------------//
//
DACOManager::DACOManager (void)
{
	reference_cnt = 1;
	initialized = 0;
	pCurrentLibrary = 0;
	innerParser = NULL;

	RegisterHeap(this);

	IComponentFactory * factory = CreateProfileParserFactory();
	RegisterComponent(factory, "IProfileParser");
	factory->Release();

	factory = CreateProfileParserFactory2();
	RegisterComponent(factory, "IProfileParser2");

	PROFPARSEDESC2 pdesc;
	pdesc.inner = &innerParser;
	pdesc.outer = this;
	factory->CreateInstance( &pdesc, (void **) &parser );

	factory->Release();
}


//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::QueryInterface (const C8 *interface_name, void **instance)
{
	ASSERT(instance);

	*instance = 0;
	
	if (strcmp(interface_name, "ICOManager") == 0)
	{
		AddRef();
		*instance = (ICOManager *) this;
		return GR_OK;
	}
	if (innerParser)
		return innerParser->QueryInterface(interface_name, instance);
	
	return GR_INTERFACE_UNSUPPORTED;
}
//--------------------------------------------------------------------------//
//
U32 DACOManager::AddRef (void)
{
	return 1;
}

//--------------------------------------------------------------------------//
//
U32 DACOManager::Release (void)
{
	return 1;
}
//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::CreateInstance (DACOMDESC *descriptor, //)
                                       void      **instance)
{
	GENRESULT result = GR_INTERFACE_UNSUPPORTED;

	ASSERT(instance);
	
	*instance = 0;

	if ((descriptor == NULL)                 || 
		(descriptor->interface_name == NULL) || 
		(!strcmp(descriptor->interface_name, DACOManager_interface_name)))
	{
		//
		// If no descriptor provided, or if component manager instance 
		// explicitly requested, return component manager object pointer
		//
		
		result    = GR_OK;
		*instance = this;
		AddRef();
	}
	else
	{
		//
		// Otherwise, pass client request to all registered interface providers 
		// until creation request is accepted
		//
		
		REGISTERED_OBJECT *obj = NULL;
		
		while ((obj = object_list.next(obj)) != NULL)
		{
			if (!strcmp(obj->interface_name, descriptor->interface_name))
            {
				result = obj->component->CreateInstance(descriptor,
					instance);
				if (result == GR_OK)
					break;
            }
		}
	}
	
	return result;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::RegisterComponent  (IComponentFactory *component, //)
                                           const C8     *interface_name, 
                                           U32           priority)
{
	//
	// Validate registration parameters
	//
	
	if ((component      == NULL) || 
		(interface_name == NULL))
	{
		return GR_INVALID_PARMS;
	}
	
	//
	// Allocate new REGISTERED_OBJECT entry, linking it into the list 
	// just before the first entry with lower or equal priority (if any)
	//
	// Higher-priority implementations appear earlier in the list, 
	// and will be visited first by CreateInstance()
	//
	
	REGISTERED_OBJECT *obj = NULL;
	
	while ((obj = object_list.next(obj)) != NULL)
	{
		if (priority >= obj->priority)
		{
			break;
		}
	}
	
	obj = object_list.alloc(obj);
	
	obj->priority       = priority;
	obj->component      = component;
	obj->interface_name = interface_name;
	obj->library		= pCurrentLibrary;
	
	//
	// Add a reference to account for the stored obj->component pointer 
	//
	
	component->AddRef();
	
	//
	// Increment registration count for this DLL to signal valid component
	// registration, and return success
	//
	
	++registration_cnt;
	
	return GR_OK;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::UnregisterComponent(IComponentFactory *component, //)
                                           const C8     *interface_name)
{
	//
	// Search list backwards to release components in inverse order of
	// registration
	//
	
	REGISTERED_OBJECT *obj = NULL;
	
	while ((obj = object_list.prev(obj)) != NULL)
	{
		//
		// Component instance must match, as well as interface name 
		// (if supplied)
		//
		
		if ((obj->component == component) 
			&&
			((interface_name == NULL) || 
			(!strcmp(interface_name, obj->interface_name))))
		{
			//
			// Release reference before invalidating object pointer
			//
			
			component->Release();
			
			object_list.free(obj);
			return GR_OK;
		}
	}
	
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::EnumerateComponents(const C8          *interface_name, //)
                                           DACOMENUMCALLBACK callback, 
                                           void             *context)
{
	//
	// Enumerate call requires valid callback
	//
	
	if (callback == NULL)
	{
		return GR_INVALID_PARMS;
	}
	
	//
	// Walk object list, calling client-supplied function for all components
	// that are providers of the specified interface
	//
	
	REGISTERED_OBJECT *obj = NULL;
	
	while ((obj = object_list.next(obj)) != NULL)
	{
		//
		// Component instance must match, as well as interface name 
		// (if supplied)
		//
		
		if ((interface_name == NULL) || (!strcmp(interface_name, 
			obj->interface_name)))
		{
			if (!callback(obj->component, 
				obj->interface_name, 
				obj->priority, 
				context))
            {
				// 
				// Stop enumerating components if callback returns FALSE
				// 
				
				break;
            }
		}
	}
	
	return GR_OK;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::AddLibrary(const C8 *DLL_filename)
{
	C8        buffer[MAX_PATH+8];
	C8       *ptr;
	S32       len;
	
	//
	// Reject invalid library name
	//
	
	if (DLL_filename == NULL)
	{
		return GR_INVALID_PARMS;
	}

	//
	// Remove previous instances loaded with same name 
	//

	if (initialized == 0)
		RemoveLibrary(DLL_filename);
	
	//
	// Allocate new entry for library
	// 
	
	ADDED_LIBRARY *library = library_list.alloc();
	
	if (library == NULL)
	{
		return GR_OUT_OF_SPACE;
	}
	
	//
	// Attempt to load library
	//
	// If load fails, or if no components are registered by this library,
	// destroy library entry and return failure
	//
	
	registration_cnt = 0;
	pCurrentLibrary = library;
	
	if( (library->instance = LoadLibrary(DLL_filename)) == NULL ) {
		GENERAL_NOTICE( TEMPSTR( "DACOM: AddLibrary: Unable to locate DLL '%s', ignoring...\n", DLL_filename ) );
	}
	else {
		U32 m, n, b;
		DACOM_GetDllVersion( DLL_filename, &m, &n, &b );
		GENERAL_NOTICE( TEMPSTR( "DACOM: AddLibrary: DLL '%s' [%d.%d.%d]\n", DLL_filename, m, n, b ) );
	}

	pCurrentLibrary = 0;
	
	if ((library->instance == NULL) || 
		(registration_cnt  == 0))
	{
		if (library->instance != NULL)
		{
			FreeLibrary(library->instance);
		}
		
		library_list.free(library);
		return GR_GENERIC;
	}
	
	//
	// Otherwise, determine base filename and copy it to library entry
	//
	
	strncpy_s(buffer, DLL_filename, MAX_PATH);
	buffer[MAX_PATH]=0;
	GetShortPathName(buffer, buffer, MAX_PATH);
	_strupr_s(buffer);
	
	//
	// Truncate extension (".DLL"), and remove path prefix ("C:\FOO\");
	//
	
	if ((ptr = strrchr(buffer, '\\')) == NULL)
	{
		ptr = buffer;
	}
	
	if ((ptr = strrchr(ptr, '.')) != NULL)
	{
		*ptr = 0;
	}
	
	if ((ptr = strrchr(buffer, '\\')) != NULL)
	{
		ptr++;
	}
	else
	{
		ptr = buffer;
	}
	
	//
	// Copy up to 8 characters to DLL descriptor
	// 
	
	len = strlen(ptr);
	
	ptr = ptr + (len - __min(8, len));
	
	memcpy(library->base_name,
		ptr, 
		sizeof(library->base_name));
	
	//
	// Return success
	//
	
	return GR_OK;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::RemoveLibrary (const C8 *DLL_filename)
{
	C8        buffer[MAX_PATH+8];
	C8       *ptr;
	S32       len;
	
	//
	// Reject invalid library name
	//
	
	if (DLL_filename == NULL)
	{
		return GR_INVALID_PARMS;
	}
	
	//
	// Determine base library filename
	//
	
	strncpy_s(buffer, DLL_filename, MAX_PATH);
	buffer[MAX_PATH]=0;
	GetShortPathName(buffer, buffer, MAX_PATH);
	_strupr_s(buffer);
	
	//
	// Truncate extension (".DLL"), and remove path prefix ("C:\FOO\");
	//
	
	if ((ptr = strrchr(buffer, '\\')) == NULL)
	{
		ptr = buffer;
	}
	
	if ((ptr = strrchr(ptr, '.')) != NULL)
	{
		*ptr = 0;
	}
	
	if ((ptr = strrchr(buffer, '\\')) != NULL)
	{
		ptr++;
	}
	else
	{
		ptr = buffer;
	}
	
	//
	// Copy up to 8 characters to DLL descriptor
	// 
	
	len = strlen(ptr);
	
	ptr = ptr + (len - __min(8, len));
	
	//
	// Find library's list entry and destroy it
	//
	
	ADDED_LIBRARY *lib = NULL;
	
	while ((lib = library_list.next(lib)) != NULL)
	{
		if (!strcmp(ptr, lib->base_name))
		{
			//
			// Library entry found; unlink related components, the unload the library module
			//
			UnregisterRelatedComponents(lib);
			
			FreeLibrary(lib->instance);
			library_list.free(lib);
			
			return GR_OK;
		}
	}
	
	//
	// Matching library not found -- return failure
	//
	
	return GR_GENERIC;
}
//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::ShutDown (void)
{
	//
	// Clean up any outstanding
	// object- and library-list entries in their inverse order of
	// creation
	//
	
	REGISTERED_OBJECT *obj;
	
	while ((obj = object_list.last()) != NULL)
	{
		obj->component->Release();
		object_list.free(obj);
	}
	
	//
	// Free all libraries in inverse order of loading
	//
	
	ADDED_LIBRARY *lib;
	
	while ((lib = library_list.last()) != NULL)
	{
		FreeLibrary(lib->instance);
		library_list.free(lib);
	}
	
	initialized = false;

	return GR_OK;
}

//--------------------------------------------------------------------------//
//
GENRESULT DACOManager::SetINIConfig (const C8 *info, U32 flags)
{
	IProfileParser::ACCESS access = IProfileParser::READ_ACCESS;

	U32 maj, min, build;
	DACOM_GetVersion( &maj, &min, &build );

	if ((flags & (DACOM_INI_WRITABLE|DACOM_INI_STRING)) == (DACOM_INI_WRITABLE|DACOM_INI_STRING))
		return GR_INVALID_PARMS;

	if (flags & DACOM_INI_STRING)
		access = IProfileParser::ACCESS (-1);

	if (flags & DACOM_INI_WRITABLE)
		access = IProfileParser::WRITE_ACCESS;

	GENRESULT result = parser->Initialize(info, access);
	
	if (result == GR_OK)
		initialize();
	return result;
}
//--------------------------------------------------------------------------//
//  path is of form "path\\"
//
BOOL32 DACOManager::LoadAllFromDirectory (const C8 *pathName, const C8 *searchName)
{
	HANDLE          search_handle;
	WIN32_FIND_DATA found;
	C8              name_buffer[MAX_PATH+4];
	C8              filePath[MAX_PATH+4];
	BOOL			result = 0;
	const C8 *			tmp;
	
	if ((tmp = strrchr(searchName, '\\')) != 0)
		tmp++;
	else
	if ((tmp = strrchr(searchName, ':')) != 0)
		tmp++;

	if (tmp)
	{
		int len = tmp - searchName;
		memcpy(filePath, searchName, len);
		filePath[len] = 0;

		strcpy_s(name_buffer, searchName);
	}
	else
	{
		strcpy_s(name_buffer,pathName);
		strcat_s(name_buffer,searchName);
		strcpy_s(filePath,pathName);
	}

	search_handle = FindFirstFile(name_buffer, &found);
	
	if (search_handle != INVALID_HANDLE_VALUE)
	{
		result=1;
		do
		{

			strcpy_s(name_buffer, filePath);
			strcat_s(name_buffer, found.cFileName);
			
			AddLibrary(name_buffer);
		} 
		while (FindNextFile(search_handle, &found));
		
		FindClose(search_handle);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
void DACOManager::LoadAllFromPath (const C8 *searchName)
{
	C8 app_path[MAX_PATH+4];
	C8 load_path[MAX_PATH+4];
	C8 search_path[4096];      // jmiles: Env vars can be REAL long in Win95
	C8 *endptr, *startptr;
	
	GetModuleFileName(NULL, app_path, sizeof(app_path));
	if ((endptr = strrchr(app_path,'\\')) != 0)				// just the path, please
		endptr[1] = 0;
	if (GetEnvironmentVariable("DACOM", search_path, sizeof(search_path)) == 0)
		search_path[0] = 0;
	startptr = search_path;
	
	do
	{
		if ((endptr = strchr(startptr,';')) != 0)			// get end of first path
			*endptr++ = 0;
		
		if (GetAbsolutePath(load_path, app_path, startptr, sizeof(load_path)))
			LoadAllFromDirectory(load_path, searchName);
		
	} while ((startptr = endptr) != 0);
	
	LoadAllFromDirectory(app_path, searchName);
}
//--------------------------------------------------------------------------//
//
static char * parseFilename (char * buffer)
{
	char * ptr, * ptr2;

	//
	// if file
	//

	if ((ptr = strchr(buffer, '\"')) != 0)
	{
		if ((ptr2 = strchr(ptr+1, '\"')) != 0)
		{
			*ptr2 = 0;
			return ptr+1;
		}
	}

	if ((ptr = strchr(buffer, ' ')) != 0)
		*ptr = 0;
	if ((ptr = strchr(buffer, '\t')) != 0)
		*ptr = 0;
	if ((ptr = strchr(buffer, '\r')) != 0)
		*ptr = 0;

	return buffer;
}
//--------------------------------------------------------------------------//
//
BOOL32 DACOManager::initialize (void)
{
	//
	// scan the load path for DLLs
	//
	
	if (initialized == 0)
	{
		HANDLE hSection;

		if ((hSection = parser->CreateSection("Libraries")) == 0)
		{
			GENERAL_NOTICE("DACOM: initialize: [Libraries] group not found in INI. Loading *.DLL from DACOM path.\n");
			LoadAllFromPath("*.dll");
		}
		else
		{
			U32 line = 0;
			char buffer[MAX_PATH];

			GENERAL_TRACE_1("DACOM: initialize: Loading DLL's from [Libraries] group.\n");

			while (parser->ReadProfileLine(hSection, line++, buffer, sizeof(buffer)) != 0)
			{
				char * ptr;

				ptr = buffer;
				while (*ptr == ' ')
					ptr++;

				switch (*ptr)
				{
				case '-':
					ptr++;
					while (*ptr == ' ')
						ptr++;
					ptr = parseFilename(ptr);
					GENERAL_TRACE_1( TEMPSTR( "DACOM: initialize: Removing '%s'...\n", ptr ) );
					RemoveLibrary(ptr);
					break;

				case 0:
				case '#':
				case ';':
					break;			// do nothing

				default:
					ptr = parseFilename(ptr);
					GENERAL_TRACE_1( TEMPSTR( "DACOM: initialize: Adding '%s' ...\n", ptr ) );
					if (strchr(ptr, ':') || strchr(ptr, '\\'))	  // path is provided
						LoadAllFromDirectory("", ptr);
					else
						LoadAllFromPath(ptr);
					break;
				} // end switch
			} // end while
		}

		initialized++;
	}

	return initialized;
}
//--------------------------------------------------------------------------//
// 'BaseDir' is of form "path\\"
// 'lpInput' is any valid path. May or may not have ending '\\'
//
//  On Return:
//    if Returns TRUE, 'lpOutput' is of form "path\\"
//
BOOL32 DACOManager::GetAbsolutePath (C8 *lpOutput, const C8 *lpBaseDir, const C8 *lpInput, LONG lSize)
{
	int len;
	C8 *ptr;
	BOOL32 result=1;

	ASSERT(lpInput);
	
	if (--lSize <= 0)
		return 0;
	lpOutput[lSize] = 0;
	
	// is this already an absolute path?
	if (lpInput[0] == DA_SWITCH_CHAR || lpInput[1]==':')
	{
		strncpy(lpOutput, lpInput, lSize);
		lpOutput[lSize-1] = 0;
		len = strlen(lpOutput);
		if (len && len < lSize && lpOutput[len-1] != DA_SWITCH_CHAR)
		{
			lpOutput[len] = DA_SWITCH_CHAR;
			lpOutput[len+1] = 0;
		}
		
		return 1;
	}
	
	strncpy(lpOutput, lpBaseDir, lSize);
	
	// output now of the form "\\Path\\"
	
	if (lpInput[0] == '.' && lpInput[1] == DA_SWITCH_CHAR)
		lpInput+=2;
	
	while (lpInput[0] == '.' && lpInput[1] == '.')
	{
		len = strlen(lpOutput);
		if (len > 2)
		{
			lpOutput[len-1] = 0;		// get rid of trailing '\\'
			
			if ((ptr = strrchr(lpOutput, DA_SWITCH_CHAR)) != 0)
				ptr[1] = 0;
			else
				result = 0;
		}
		else
			result = 0;
		lpInput+=2;
		if (lpInput[0] == DA_SWITCH_CHAR)
			lpInput++;
	}
	
	len = strlen(lpOutput);
	if (lSize - len > 0)
		strncpy(lpOutput+len, lpInput, lSize-len);
	len = strlen(lpOutput);
	if (len && len < lSize && lpOutput[len-1] != DA_SWITCH_CHAR)
	{
		lpOutput[len] = DA_SWITCH_CHAR;
		lpOutput[len+1] = 0;
	}
	
	return result;
}
//--------------------------------------------------------------------------//
// library is going away, unregister any components that exist inside it
//
void DACOManager::UnregisterRelatedComponents (ADDED_LIBRARY *library)
{
	//
	// Search list backwards to release components in inverse order of
	// registration
	//
	
	REGISTERED_OBJECT *obj = NULL;
	
	while ((obj = object_list.prev(obj)) != NULL)
	{
		if (obj->library == library)
		{
			//
			// Release reference before invalidating object pointer
			//
			
			obj->component->Release();
			
			object_list.free(obj);
			obj = NULL;
		}
	}
}	


//

//--------------------------------------------------------------------------//
//----------------------Error Dumping Routines Methods----------------------//
//--------------------------------------------------------------------------//
//
// Standard implementation of error dumper.
//


int __cdecl STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...)
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	static bool bNoMoreWarnings;
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_NOTICE)
	{
		OutputDebugString ("\n");
	}

	// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

	if (code.severity == SEV_FATAL || code.severity == SEV_ERROR || (bNoMoreWarnings==0 && code.severity == SEV_WARNING))
	{
		switch (MessageBox(0, buffer, "DACOM::STANDARD_DUMP", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST))
		{
		case IDABORT:
			PostQuitMessage(-1);
			abort();
			break;

		case IDRETRY:
			if (code.kind == ERR_ASSERT)
				__asm int 3
			return 1;

		case IDIGNORE:
			if (code.severity == SEV_WARNING)
				bNoMoreWarnings = true;
			break;

		default:
			break;
		}

	}
	
	return 0;
}

//--------------------------------------------------------------------------//
//
// extern "C" interface to guarantee static linkage without name-mangling 
//
//--------------------------------------------------------------------------//
extern "C" 
{
	//--------------------------------------------------------------------------//
	//
	DXDEF ICOManager * __cdecl DACOM_Acquire(void)
	{
		//
		// Single, application-global static instance declared here
		//
		
		static DACOManager manager;
		
		//
		// Return pointer to application-global static instance
		//
		
		return &manager;
	}


	//

	//

	GENRESULT DACOM_GetDllVersion( const char *dll_name, U32 *out_major, U32 *out_minor, U32 *out_build )
	{
		VS_FIXEDFILEINFO ffi;
		TCHAR filename[_MAX_PATH];
		DWORD dwDummyHandle, len; // will always be set to zero
		HMODULE hModule = NULL; 
		LPVOID lpvi;
		UINT iLen;
		BYTE *vi;

		memset( &ffi, 0, sizeof(VS_FIXEDFILEINFO) );

		*out_major = 0;
		*out_minor = 0;
		*out_build = 0;

		if( (hModule = ::LoadLibrary( dll_name )) == NULL ) {
			return GR_GENERIC;
		}

		if( GetModuleFileName( hModule, filename, sizeof(filename)/sizeof(filename[0]) ) <= 0 ) {
			return GR_GENERIC; 
		}

		if( (len = GetFileVersionInfoSize( filename, &dwDummyHandle )) <= 0 ) {
			return GR_GENERIC; 
		}

		vi = new BYTE[len]; // allocate version info
		
		if( ::GetFileVersionInfo(filename, 0, len, vi ) == 0 ) {
			delete[] vi;
			return GR_GENERIC; 
		}

		if( VerQueryValue( vi, "\\", &lpvi, &iLen ) == 0 ) {
			return GR_GENERIC; 
		}

		// copy fixed info to myself, which am derived from VS_FIXEDFILEINFO
		*(VS_FIXEDFILEINFO*)&ffi = *(VS_FIXEDFILEINFO*)lpvi;

		FreeLibrary( hModule );

		delete[] vi;

		*out_major = (ffi.dwProductVersionMS & 0xFFFF0000) >> 16;
		*out_minor = (ffi.dwProductVersionMS & 0x0000FFFF);
	//	U32 unused = (ffi.dwProductVersionLS & 0xFFFF0000) >> 16;
		*out_build = (ffi.dwProductVersionLS & 0x0000FFFF);

		return GR_OK;
	}


	//

	GENRESULT DACOM_GetVersion( U32 *out_major, U32 *out_minor, U32 *out_build )
	{
		GENRESULT gr;
		
		if( SUCCEEDED( gr = DACOM_GetDllVersion( "DACOM.DLL", out_major, out_minor, out_build ) ) ) {
			GENERAL_NOTICE( TEMPSTR( "Current DACOM.DLL version: %d.%d.%d\n", *out_major, *out_minor, *out_build ) );
			GENERAL_NOTICE( TEMPSTR( " Compiled against version: %d.%d.%d\n", LIB_MAJOR, LIB_MINOR, LIB_BUILD ) );
		}

		return gr;
	}


}



//--------------------------------------------------------------------------//
//-------------------------------END DACOM.CPP------------------------------//
//--------------------------------------------------------------------------//
