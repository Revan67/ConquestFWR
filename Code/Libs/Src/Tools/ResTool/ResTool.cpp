//
// ResTool.cpp - A tool for performing command line manipulation of resources in Win32 .DLL and .EXE files
//

// Stop the annoying "symbol too long" and "unwind semantics without exception handling"
#pragma warning( disable: 4786 4530 4800 )

//
// Include files
//

#include <windows.h>
#include <winnt.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <map>

//

bool can_binary_have_resources( const char *filename );


//
// Class and structure definitions
//

class ResId
{
private:
	std::string storage;
	LPCTSTR     pStr;

public:
	ResId (const std::string &str)
	{
		storage = str;
		pStr = storage.c_str();
	}

	ResId (LPCTSTR str)
	{
		pStr = str;
	}

	ResId () { pStr = NULL; }

	operator LPCTSTR ()
	{
		return pStr;
	}

	operator = (LPCTSTR str)
	{
		if (str > (LPCTSTR) 0xFFFF && str[0] == '#')
		{
			pStr = (LPCTSTR) atoi (str+1);
		}
		else
		{
			pStr = str;
		}
	}

	operator = (const std::string &str)
	{
		storage = str;
		pStr = storage.c_str();
		if (pStr[0] == '#')
		{
			pStr = (LPCTSTR) atoi (pStr+1);
		}
	}
};

class ResSpec
{
private:
	typedef std::map<std::string, ResId> StdMap;

	std::string  resFile;
	ResId        resType;
	ResId        resName;

	StdMap stdRes;

private:
	void init_stdres ()
	{
		stdRes["RT_ACCELERATOR"] = (LPCTSTR)RT_ACCELERATOR;
		stdRes["RT_ANICURSOR"] = (LPCTSTR)RT_ANICURSOR;
		stdRes["RT_ANIICON"] = (LPCTSTR)RT_ANIICON;
		stdRes["RT_BITMAP"] = (LPCTSTR)RT_BITMAP;
		stdRes["RT_CURSOR"] = (LPCTSTR)RT_CURSOR;
		stdRes["RT_DIALOG"] = (LPCTSTR)RT_DIALOG;
		stdRes["RT_FONT"] = (LPCTSTR)RT_FONT;
		stdRes["RT_FONTDIR"] = (LPCTSTR)RT_FONTDIR;
		stdRes["RT_GROUP_CURSOR"] = (LPCTSTR)RT_GROUP_CURSOR;
		stdRes["RT_GROUP_ICON"] = (LPCTSTR)RT_GROUP_ICON;
		stdRes["RT_HTML"] = (LPCTSTR)RT_HTML;
		stdRes["RT_ICON"] = (LPCTSTR)RT_ICON;
		stdRes["RT_MENU"] = (LPCTSTR)RT_MENU;
		stdRes["RT_MESSAGETABLE"] = (LPCTSTR)RT_MESSAGETABLE;
		stdRes["RT_RCDATA"] = (LPCTSTR)RT_RCDATA;
		stdRes["RT_STRING"] = (LPCTSTR)RT_STRING;
		stdRes["RT_VERSION"] = (LPCTSTR)RT_VERSION;
	}

	bool check_stdres (ResId &id)
	{
		StdMap::iterator here = stdRes.find (std::string(id));
		if (here != stdRes.end ())
		{
			id = (*here).second;
			return true;
		}
		return false;
	}

public:
	ResSpec () { init_stdres(); }

	bool parse_string (const char *spec)
	{
		// Break into pieces: filename/resType:resName
		const char *slash = strchr (spec, '/');
		if (!slash)
		{
			return false;
		}
		resFile = std::string (spec, slash - spec);
		const char *colon = strchr (slash, ':');
		if (!colon)
		{
			return false;
		}

		ResId typeStr (std::string(slash+1, colon - slash - 1));
		check_stdres (typeStr);
		resType = typeStr;

		resName = colon+1;

		// If the first character of the type name is a '$', convert it into one of the special type integers
		return true;
	}

	bool set_file (const char *filename)
	{
		resFile = filename;
	}

	bool set_type (const char *type)
	{
		resType = type;
	}

	bool set_type (WORD type)
	{
		resType = MAKEINTRESOURCE (type);
	}

	bool set_name (const char *name)
	{
		resName = name;
	}

	bool set_name (WORD name)
	{
		resName = MAKEINTRESOURCE (name);
	}

	const char *get_name ()
	{
		return (const char *) resName;
	}

	const char *get_file ()
	{
		return resFile.c_str();
	}

	const char *get_type ()
	{
		return (const char *) resType;
	}
};

//
// Static data members
//

//
// Local routines
//

void usage ()
{
	printf("Usage: restool res_file [-a src_file/res_type:res_name | -d res_type:res_name | -l | -h]\n");
}

void help ()
{
	usage ();
	printf
		(
			"Where: res_file  is the name of the resource file to be modified\n"
			"       src_file  is the name of the resource file to be copied from\n"
			"       res_type  is the type of the resource to delete or copy\n"
			"       res_name  is the name of the resource to delete or copy\n"
			"\n"
			"       -a        will add the specified resource to the destination file\n"
			"       -d        will delete the specified resource from the destination file\n"
			"       -l        will list all of the resources in the destination file\n"
			"       -h        will display this help\n"
			"\n"
			"    You can put more than one command on the command line. They will be\n"
			"executed in the order they are given on the command line. The program will\n"
			"exit on the first error.\n"
			"    The program return code is 0 if there were no errors, non-zero otherwise.\n"
			"    The program will not generate any messages upon success.\n"
			"\n"
			"    The res_type field is the string name of the resource you are interested\n"
			"in.\n"
			"For the standard Win32 resource types, use the name of the RT_* constants from\n"
			"the Win32 docs, e.g. RT_VERSION. To refer to a type by number, put a '#' in\n"
			"front of the digits, e.g. '#1' will refer to resource type 1.\n"
			"    The res_name field is the string name of the resource you are interested\n"
			"in. As with the res_type field, putting a '#' in front of some digits will\n"
			"address by number instead of name.\n"
			"\n"
			"Examples:\n"
			"    'bogus.dll/RT_VERSION:#1' refers to the version resource in bogus.dll\n"
			"    'petal.exe/DACMP:Marker'  refers to the \"DACMP\" resource in petal.exe\n"
			"named \"Marker\"\n"
			"\n"
			"Thank you for using KludgeWare!\n"
		);
}

void report_last_error ()
{
	DWORD error = GetLastError ();
	const int bufferSize = 1024;
	char buffer[bufferSize];
	DWORD result =
		FormatMessageA
		(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			error,
			0,
			buffer,
			bufferSize,
			NULL
		);

	if (result)
	{
		printf ("Win32 Error: %s\n", buffer);
	}
	else
	{
		printf ("Win32 Error: 0x%x\n", error);
	}
}

static BOOL CALLBACK enumResTypeProc
(
  HMODULE hModule,  // module handle
  LPTSTR lpszType,  // resource type
  LONG lParam   // application-defined parameter
)
{
	// Callback for resource type enumeration.

	// Indicate that we found a type and stop the enumeration.
	BOOL *foundOne = (BOOL *) lParam;
	*foundOne = TRUE;
	
	// Return TRUE here so we don't abort the resource enumeration
	// if we return FALSE, EnumerateResourceTypes() will return failure.
	//
	return TRUE;	
}


bool add_resource (const char *filename, const char *srcSpec)
{
	if( !can_binary_have_resources( filename ) ) {
		printf ("Skipping '%s' [console app?]\n", filename );
		return false;
	}


	// Parse the source spec into its component parts
	ResSpec spec;
	if (!spec.parse_string (srcSpec))
	{
		printf ("Invalid resource spec \"%s\".\n", srcSpec);
		return false;
	}

	// Open the source file, get a handle to the resource we want to add, and make a copy.
	HINSTANCE hSrc = LoadLibraryEx (spec.get_file(), NULL, LOAD_LIBRARY_AS_DATAFILE);
	if (!hSrc)
	{
		report_last_error();
		printf ("Failed to load source dll \"%s\".\n", spec.get_file());
		return false;
	}

	HRSRC hResInfo = FindResource (hSrc, spec.get_name(), spec.get_type());
	if (!hResInfo)
	{
		report_last_error ();
		printf
			(
				"Failed to find a resource of type \"%s\" and name \"%s\" in the file \"%s\".\n",
				spec.get_type(), spec.get_name(), spec.get_file()
			);
		FreeLibrary (hSrc);
		return false;
	}

	DWORD resSize = SizeofResource (hSrc, hResInfo);

	HGLOBAL hResData = LoadResource (hSrc, hResInfo);
	if (!hResData)
	{
		report_last_error ();
		printf
			(
				"Failed to load the resource of type \"%s\" and name \"%s\" in the file \"%s\".\n",
				spec.get_type(), spec.get_name(), spec.get_file()
			);
		FreeLibrary (hSrc);
		return false;
	}

	LPVOID pResData = LockResource (hResData);
	if (!pResData)
	{
		report_last_error ();
		printf ("Failed to lock the resource.\n");
		FreeLibrary (hSrc);
		return false;
	}

	char *pResDataCopy = new char[resSize];
	memcpy (pResDataCopy, pResData, resSize);

	// Clean up after the source file.
	// NOTE: There is no need to unlock or free the resource under Win32, and there is no API to free the
	// resource information handle. 
	if (!FreeLibrary (hSrc))
	{
		printf ("Failed to free the loaded source library. Continuing.\n");
	}

	// Ensure that the given file can receive resources.
	// Due to a bug in Windows, calling BeginUpdateResource(filename, FALSE), FALSE meaning don't delete all of
	// the existing resources, on a file that has never has any resources, then proceeding to add some, will
	// appear to work, but won't write anything out to the file.
	// If, however, you perform the begin with TRUE, thereby clearing all of the resources from the file, it will
	// magically work. We can't just pass TRUE all the time, however, because doing so will erase any other resources
	// in the file. <Sigh>.
	// So, we will attempt to enumerate the resources from the destination file. If we fail, then we will begin the
	// update with TRUE, otherwise FALSE.
	BOOL hasNoResources = FALSE;
	{
		HINSTANCE hDest = LoadLibraryEx (filename, NULL, LOAD_LIBRARY_AS_DATAFILE);
		if (!hDest)
		{
			report_last_error();
			printf ("Failed to load destination dll \"%s\".\n", filename);
			delete [] pResDataCopy;
			return false;
		}

		BOOL foundResources = FALSE;
		if (EnumResourceTypes (hDest, (ENUMRESTYPEPROC) enumResTypeProc, (LONG) &foundResources))
		{
			hasNoResources = !foundResources;
		}
		else
		{
			// If there were no resources in the file already, the enumeration will fail.
			// Don't report in this case.
			// report_last_error ();
			hasNoResources = TRUE;
		}

		if (!FreeLibrary (hDest))
		{
			printf ("Failed to free the loaded destination library. Continuing.\n");
		}
	}

	// Open the destination file for updating
	HANDLE hUpdate = BeginUpdateResource (filename, hasNoResources);
	if (!hUpdate)
	{
		report_last_error();
		printf ("Failed to open destination file \"%s\".\n", filename);
		delete [] pResDataCopy;
		return false;
	}

	// Add the resource to the destination file.

	BOOL result = 
		UpdateResource
		(
			hUpdate,
			spec.get_type(), spec.get_name(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			pResDataCopy, resSize
		);

	if (!result)
	{
		report_last_error ();
		printf ("Failed to update the resource in the destination file.\n");
		EndUpdateResource (hUpdate, TRUE);
		delete [] pResDataCopy;
		return false;
	}

	// End the update, writing the changes to the file.
	if (!EndUpdateResource (hUpdate, FALSE))
	{
		report_last_error ();
		printf ("Failed to end the update of the resource in the desination file.\n");
		delete [] pResDataCopy;
		return false;
	}

	// Return success.
	delete [] pResDataCopy;
	return true;
}

bool delete_resource (const char *filename, const char *resSpec)
{
	printf ("This function is not written yet!\n");
	return false;
}

bool list_resources (const char *filename)
{
	printf ("This function is not written yet!\n");
	return false;
}

//
// Main program entry point
//

int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		usage ();
		return 0;
	}

	const char *filename = argv[1];
	if (filename[0] == '-')
	{
		if (filename[1] == 'h')
		{
			help ();
		}
		else
		{
			usage ();
		}
		return 0;
	}

	int i = 2;
	bool bail = false;
	while (i < argc)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
			case 'h':
				help ();
				break;

			case 'a':
				++i;
				if (i < argc)
				{
					if (strchr (filename, '*') || strchr (filename, '?')) 
					{
						WIN32_FIND_DATA ft_find;
						HANDLE hfind;
						char path[MAX_PATH];
						char curdir[MAX_PATH];
						char *filespec;

						strcpy( path, filename );

						if( (filespec = strrchr( path, '\\' )) != NULL ) {
							GetCurrentDirectory( _MAX_PATH, curdir );
							*filespec = 0;
							filespec++;
							SetCurrentDirectory( path );
						}
						else {
							filespec = const_cast<char*>(filename);
							path[0] = 0;
						}

						if( (hfind = FindFirstFile( filespec, &ft_find )) != INVALID_HANDLE_VALUE ) {
							do {
								if( !add_resource( ft_find.cFileName, argv[i] ) ) {
									//...
								}
							} while( FindNextFile( hfind, &ft_find ) );
						}

						if( path[0] ) {
							SetCurrentDirectory( curdir );
						}
					}
					else if (!add_resource (filename, argv[i]))
					{
						return 2;
					}
				}
				else
				{
					printf ("Missing source resource spec. for add.\n");
					return 1;
				}
				break;

			case 'd':
				++i;
				if (i < argc)
				{
					delete_resource (filename, argv[i]);
				}
				else
				{
					printf ("Missing resource spec. for delete.\n");
					return 1;
				}
				break;

			case 'l':
				list_resources (filename);
				break;

			default:
				printf ("Invalid switch '%c'\n", argv[i][1]);
				return 1;
				break;
			}
		}
		else
		{
			// Syntax error. Break;
			printf ("Invalid command line option \"%s\".\n", argv[i]);
			return 1;
		}

		++i;
	}

	// All is well.
	return 0;
}

//

#define XFER_BUFFER_SIZE 2048

bool can_binary_have_resources( const char *filename )
{
    HANDLE hImage;

    DWORD  bytes;
    DWORD  SectionOffset;
    DWORD  CoffHeaderOffset;
    DWORD  MoreDosHeader[16];

    ULONG  ntSignature;

    IMAGE_DOS_HEADER      image_dos_header;
    IMAGE_FILE_HEADER     image_file_header;
    IMAGE_OPTIONAL_HEADER image_optional_header;
//    IMAGE_SECTION_HEADER  image_section_header;

    /*
     *  Open the reference file.
     */ 
    hImage = CreateFile( filename,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if( INVALID_HANDLE_VALUE == hImage ) {
		return false;
    }

    /*
     *  Read the MS-DOS image header.
     */ 
    if( !ReadFile( hImage, &image_dos_header, sizeof(IMAGE_DOS_HEADER), &bytes, NULL) ) {
		CloseHandle( hImage );
		return false;
    }

    if( IMAGE_DOS_SIGNATURE != image_dos_header.e_magic ) {
		CloseHandle( hImage );
		return false;
    }

    if( !ReadFile( hImage, MoreDosHeader, sizeof(MoreDosHeader), &bytes, NULL) ) {
		CloseHandle( hImage );
		return false;
    }

    /*
     *  Get actual COFF header.
     */ 

    CoffHeaderOffset = SetFilePointer( hImage, image_dos_header.e_lfanew, NULL, FILE_BEGIN );
	if( CoffHeaderOffset == 0xFFFFFFFF ) {
		CloseHandle( hImage );
		return false;
	}
	CoffHeaderOffset += sizeof(ULONG);

    if( !ReadFile( hImage, &ntSignature, sizeof(ULONG), &bytes, NULL) ) {
		CloseHandle( hImage );
		return false;
    }

    if( IMAGE_NT_SIGNATURE != ntSignature ) {
		CloseHandle( hImage );
		return false;
    }

    SectionOffset = CoffHeaderOffset + IMAGE_SIZEOF_FILE_HEADER +
                    IMAGE_SIZEOF_NT_OPTIONAL_HEADER;

    if( !ReadFile( hImage, &image_file_header, IMAGE_SIZEOF_FILE_HEADER, &bytes, NULL) ) {
		CloseHandle( hImage );
		return false;
    }

    if( !ReadFile( hImage, &image_optional_header, IMAGE_SIZEOF_NT_OPTIONAL_HEADER, &bytes, NULL) ) {
		CloseHandle( hImage );
		return false;
    }

    switch( image_optional_header.Subsystem ) {
    
    case IMAGE_SUBSYSTEM_UNKNOWN:
    case IMAGE_SUBSYSTEM_WINDOWS_CUI:
    case IMAGE_SUBSYSTEM_OS2_CUI:
    case IMAGE_SUBSYSTEM_POSIX_CUI:
		CloseHandle( hImage );
		return false;

    }

	CloseHandle( hImage );
	return true;
}

//