// GetProfileInfo.cpp
//
//
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

//

#include "typedefs.h"

//

HRESULT GetProfileInfo( char *out_app_name, U32 max_app_name_len, char *out_ini_pathname, U32 max_ini_pathname_len )
{
	char cmd_line[MAX_PATH];
	char ini_file[MAX_PATH];
	char app_name[MAX_PATH];
	char app_drive[_MAX_DRIVE], app_dir[_MAX_DIR];
	char *p, *d;

	if( max_app_name_len < _MAX_FNAME ) {
		return E_FAIL;
	}

	// get appname
	//
	GetModuleFileName( GetModuleHandle(NULL), app_name, MAX_PATH );
	_splitpath( app_name, app_drive, app_dir, out_app_name, NULL );
	if( !_stricmp(out_app_name, "OBJVIEW") ) // this is needed so that ObjView is found in the .ini
	{
		strcpy(out_app_name, "ObjView");
	}
	
	// get ini file
	strcpy( cmd_line, GetCommandLine() );
	
	// See if they specified an INI file on the command line
	//
	if( (p = strstr( cmd_line, "-i" )) || (p = strstr( cmd_line, "-I")) ) {
		
		p += 2;															// remove -I (or -i)
		for( ; *p && (*p == ' ' || *p == '\t'); p++ );					// remove leading whitespace
		for( d=ini_file; *p && *p != ' ' && *p != '\t'; p++, d++ ) {	// copy out filename
			*d = *p;
		}
		*d = '\0';

		if( _access( ini_file, 0x04 ) == 0 ) {
			strcpy( out_ini_pathname, ini_file );
			return S_OK;
		}

		char buffer[MAX_PATH];
		sprintf( buffer, "Unable to find profile file '%s'.\n"
				         "Click OK to use the default profile file.\n"
						 "Click Cancel to exit this application.", ini_file );

		if( MessageBox( GetDesktopWindow(), buffer, "Error", MB_OKCANCEL ) == IDCANCEL ) {
			return E_FAIL;
		}
	}
	
	// try "current working directory/app_name.ini"
	_makepath( ini_file, NULL, NULL, out_app_name, ".ini" );
	if( _access( ini_file, 0x04 ) == 0 ) {
		strcpy( out_ini_pathname, ini_file );
		return S_OK;
	}

	// try "application module directory/app_name.ini"
	_makepath( ini_file, app_drive, app_dir, out_app_name, ".ini" );
	if( _access( ini_file, 0x04 ) == 0 ) {
		strcpy( out_ini_pathname, ini_file );
		return S_OK;
	}

	// try "current working directory/dacom.ini"
	_makepath( ini_file, NULL, NULL, "dacom", ".ini" );
	if( _access( ini_file, 0x04 ) == 0 ) {
		strcpy( out_ini_pathname, ini_file );
		return S_OK;
	}

	// try "application module directory/dacom.ini"
	_makepath( ini_file, app_drive, app_dir, "dacom", ".ini" );
	if( _access( ini_file, 0x04 ) == 0 ) {
		strcpy( out_ini_pathname, ini_file );
		return S_OK;
	}

	return E_FAIL;
}