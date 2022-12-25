// dbgtee.cpp : Captures Win32 Debug events and dumps them to stdout.
//

#include "stdafx.h"


//

bool run_command( char *command )
{
	STARTUPINFO           StartupInfo;
	PROCESS_INFORMATION   ProcessInfo;

	memset( &StartupInfo, 0, sizeof(StartupInfo) );
	StartupInfo.cb = sizeof( STARTUPINFO );

	memset( &ProcessInfo, 0, sizeof(ProcessInfo) );

	unsigned int Flags = DEBUG_PROCESS | NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;

	if( !CreateProcess(	NULL, command, NULL, NULL, TRUE, Flags, NULL, NULL, &StartupInfo, &ProcessInfo ) ) {
		switch( GetLastError() ) {
		case ERROR_FILE_NOT_FOUND:		fprintf( stdout, "File not found" );	break;
		case ERROR_ACCESS_DENIED:		fprintf( stdout, "Access denied" );		break;
		case ERROR_FILE_INVALID:		fprintf( stdout, "Invalid file" );		break;
		case ERROR_FILE_CORRUPT:		fprintf( stdout, "Corrupt file" );		break;
		case ERROR_BAD_EXE_FORMAT:		fprintf( stdout, "Bad exec format" );	break;
		default:						fprintf( stdout, "Unknown CreateProcess error" );		break;
		}
		return( false );
	}
	else {
		CloseHandle( ProcessInfo.hProcess );
		CloseHandle( ProcessInfo.hThread );
	}

	return true;
}

//

bool handle_debug_events( )
{
	bool continue_debugging = true;
	DEBUG_EVENT db;
	OUTPUT_DEBUG_STRING_INFO *info = NULL;

	if( WaitForDebugEvent( &db, 0 ) ) {

		switch( db.dwDebugEventCode ) {

		case OUTPUT_DEBUG_STRING_EVENT:
			info = &db.u.DebugString;
			if( !info->fUnicode ) {
				DWORD readCount;
				HANDLE hProc;

				char *buffer = new char[info->nDebugStringLength+1];

				if( (hProc = OpenProcess( PROCESS_VM_READ, FALSE, db.dwProcessId )) ) {
					if( ReadProcessMemory( hProc, info->lpDebugStringData, buffer, info->nDebugStringLength, &readCount ) ) {
						buffer[info->nDebugStringLength] = '\0';
						fprintf( stdout, buffer );
					}
					CloseHandle (hProc);
				}

				delete[] buffer;
			}

			ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_CONTINUE);
			break;

		case EXCEPTION_DEBUG_EVENT:
			if( db.u.Exception.dwFirstChance && (db.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT ||
												 db.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP) ) {
				ContinueDebugEvent (db.dwProcessId, db.dwThreadId, DBG_CONTINUE);
			}
			else {
				ContinueDebugEvent( db.dwProcessId, db.dwThreadId, DBG_EXCEPTION_NOT_HANDLED );
			}
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
			fprintf( stdout, "process exited with code %08X (%d)\r\n", db.u.ExitProcess.dwExitCode, db.u.ExitProcess.dwExitCode );
			continue_debugging = false;
			ContinueDebugEvent( db.dwProcessId, db.dwThreadId, DBG_EXCEPTION_NOT_HANDLED );
			break;
			
		default:
			ContinueDebugEvent( db.dwProcessId, db.dwThreadId, DBG_EXCEPTION_NOT_HANDLED );
		}
	}

	return continue_debugging;
}

//

int main( int argc, char* argv[] )
{
	char command[_MAX_PATH+1];
	char tbuf[_MAX_PATH+1];
	int argc_start = 1;

	if( argc <= 1 ) {
		fprintf( stderr, "dbgtee <command with params>\r\n" );
		exit( 255 );
	}

	command[0] = 0;
	_snprintf( command, _MAX_PATH, "%s", argv[argc_start] );
	argc_start++;

	for( int arg=argc_start; arg<argc; arg++ ) {
		_snprintf( tbuf, _MAX_PATH, "%s %s", command, argv[arg] );
		_snprintf( command, _MAX_PATH, "%s", tbuf );
	}

	OutputDebugString( "Kick it\n" );

	if( run_command( command ) ) {
		while( handle_debug_events() );	
	}

	return 0;
}


//
