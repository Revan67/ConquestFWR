//
// Err.cpp
//
//
//

#include <stdio.h>
#include <windows.h>

//

#include "DACOM.h"
#include "FDUMP.h"

//

static char ErrFileName[MAX_PATH] = "ErrView.txt";
static char ErrBuffer[4096];
static bool DisplayWarnings = true;
static bool WriteErrsToFile = false;

//

int __cdecl Err_Handler( ErrorCode code, const C8 *fmt, ... );

//


void Err_Initialize( void )
{
	FDUMP = Err_Handler;
}

//

void Err_SetLogEnable( bool LogToFile )
{
	WriteErrsToFile = LogToFile;
}


//

void Err_SetLogFilename( const char* f )
{
	strncpy( ErrFileName, f, MAX_PATH );
}

//

int __cdecl Err_Handler( ErrorCode code, const C8 *fmt, ... )
{
	va_list args;

	va_start( args, fmt );
	wvsprintf( ErrBuffer, fmt, args );
	va_end( args );

	OutputDebugString( ErrBuffer );

	// NOTE: Newlines are already added to trace severity.
	if( code.severity < SEV_NOTICE ) {
		OutputDebugString( "\n" );
	}

	if( WriteErrsToFile ) {
		FILE* file = fopen( ErrFileName, "at" );
		if( file ) {
			fprintf( file, ErrBuffer );
			fclose( file );
		}
	}

	if( code.severity == SEV_FATAL || code.severity == SEV_ERROR || (DisplayWarnings && code.severity == SEV_WARNING) ) {

		DWORD reply = MessageBox( 0, ErrBuffer, "Objview Message", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST );
		
		switch( reply ) {

		case IDABORT:
#ifdef _DEBUG
			__asm int 3
#endif
			PostQuitMessage(-1);
			abort();
			break;

		case IDRETRY:
			if( code.kind == ERR_ASSERT ) {
#ifdef _DEBUG
				__asm int 3
#else
				abort();
#endif
			}
			return 1;

		case IDIGNORE:
			if( code.severity == SEV_WARNING ) {
				DisplayWarnings = false;
			}
			break;

		default:
			break;
		}

	}
	
	return 0;
}

//