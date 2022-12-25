// DarthRT.cpp 
//

#include "stdafx.h"

//

#define DARTH_RUNTIME_VERSION_MAJOR 1
#define DARTH_RUNTIME_VERSION_MINOR 7

//

#define DARTH_BUFFER_SIZE	8192

//

#define ALL_TEST_FILES_WILDCARD	"*.dll"

#define DARTH_TEST_PROMPT_MODE		"TestPromptMode"
#define DARTH_TEST_DEFAULT_RETURN	"TestDefaultReturn"
#define DARTH_TEST_FAILURE_MODE		"TestFailureMode"
#define DARTH_TEST_EXCEPTION_MODE	"TestExceptionMode"
#define DARTH_TEST_LIBERROR_MODE	"TestLibraryErrorMode"
#define DARTH_TEST_INI_FILE			"IniFile"
#define DARTH_INI_PATH				"IniFilePath"

//

#define DARTH_INI_PATH_DEFAULT		"Config\\"
#define DARTH_TEST_PATH_DEFAULT		"Tests\\"

//

enum DARTH_TESTMODES
{
	DTM_ABORT	= 0,
	DTM_RETRY	= 1,
	DTM_EXECUTE	= 1,
	DTM_CONTINUE= 2,
	DTM_DONT_EXECUTE = 2,
	DTM_PROMPT	= 3
};

//

#pragma warning( disable : 4800 )

//

typedef DARTHTESTINFO test_info;

//

void LuaRuntimeReadOnlyTagSet( void );
void LuaRuntimeCallTest( void );
void LuaRuntimeErrorHandler( void );
void LuaRuntimeLog( void );
int  LibraryDumpHandler( ErrorCode code, const C8 *Fmt, ... );
LONG __stdcall ExceptionHandler( EXCEPTION_POINTERS *pExPtrs, TCHAR *OutString, int MaxOutLen );
void DoStackTrace ( LPTSTR szString, DWORD dwSize, DWORD dwNumSkip );

//

#ifndef NO_BUGSLAYER
static BOOL  EH_GetFaultReason		= TRUE ;
static BOOL  EH_GetStackTraceString = TRUE ;
static BOOL  EH_ShowRegs			= TRUE ;
static BOOL  EH_LetItCrash			= FALSE ;
static DWORD EH_StackDisplayOptions	= GSTSO_PARAMS | GSTSO_MODULE | GSTSO_SYMBOL | GSTSO_SRCLINE;
#endif
static TCHAR EH_LastOutputString[DARTH_BUFFER_SIZE];

//

//	.................................................................
//
//	DarthRuntime
//  This is passed around to all of the tests.
//

struct DarthRuntime : public IDarthRuntime
{
	// External Public Interface
	//

	IDR_FLOWCONTROL QueryUserInput( void ) 
	{
		if( GetAsyncKeyState( VK_ESCAPE ) ) {
			return IDR_FC_EXIT;
		}
		if( GetAsyncKeyState( VK_SPACE ) ) {
			return IDR_FC_NEXT;
		}
		if( GetAsyncKeyState( VK_BACK ) ) {
			return IDR_FC_PREV;
		}
		if( GetAsyncKeyState( VK_F2 ) ) {
			return IDR_FC_PAUSE;
		}
		if( GetAsyncKeyState( VK_F3 ) ) {
			return IDR_FC_CONTINUE;
		}

		return IDR_FC_CONTINUE;
	}

	//

	void Log( unsigned int _LogClass, int _LogLevel, TCHAR *Fmt, ... ) 
	{
		if( !(_LogClass & LogMask) ) {
			return;
		}

		TCHAR String[DARTH_BUFFER_SIZE];

		va_list argp;
		va_start( argp, Fmt );
		_vstprintf( String, Fmt, argp );
		va_end( argp );

		TCHAR Output[DARTH_BUFFER_SIZE];
		TCHAR Name[MAX_PATH];

		if( CurrentTestTable ) {
			lua_get_string( CurrentTestTable, "Name", Name, MAX_PATH );
		}
		else {
			Name[0] = 0;
		}

		_stprintf( Output, _T("%s:%s: %s\n"), CurrentScript, Name, String );
		OutputDebugString( Output );
	}

	//

	static LRESULT CALLBACK TestWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		DarthRuntime *rt = (DarthRuntime*)GetWindowLong( hwnd, GWL_USERDATA );
		
		switch( uMsg )
		{

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint( hwnd, &ps );
				RECT r;
				GetClientRect( hwnd, &r );
				FillRect( ps.hdc, &r, (HBRUSH)GetStockObject( GRAY_BRUSH ) );
				DrawEdge( ps.hdc, &r, EDGE_SUNKEN, 0 );
				EndPaint( hwnd, &ps );
			}

		case WM_KEYDOWN:
				break;

			case WM_DESTROY:
			case WM_CLOSE:
				return 0;

			default:
				break;
		}
		return DefWindowProc( hwnd, uMsg, wParam, lParam);
	}


	//

	HWND CreateTestWindow( int x, int y, int w, int h, TCHAR *Title )
	{
		WNDCLASSEX  wc;
		HWND CurrentTestWindowHandle;

		memset( &wc, 0, sizeof(wc) );

		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW|CS_VREDRAW;
		wc.lpfnWndProc = TestWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hIcon = 0; 
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = NULL;//(HBRUSH)GetStockObject( BLACK_BRUSH );
		wc.lpszMenuName =  NULL;
		wc.lpszClassName = "TestWindowClass";

		RegisterClassEx( &wc );

		RECT r;
		GetClientRect( GetDesktopWindow(), &r );
		
		CurrentTestWindowHandle = CreateWindow( "TestWindowClass", 
												Title, 
												WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW, 
												(x==-1)? ((x = (r.right/2-w/2))<0)? 0 : x : x, 
												(y==-1)? ((y = (r.bottom/2-h/2))<0)? 0 : y : y, 
												w, 
												h,
												NULL, 
												NULL, 
												GetModuleHandle(NULL), 
												NULL );

		if( CurrentTestWindowHandle == NULL ) {
			return 0;
		}

		SetWindowLong( CurrentTestWindowHandle, GWL_USERDATA, (long)this );

		ShowWindow( CurrentTestWindowHandle, SW_NORMAL );

		return CurrentTestWindowHandle;
	}

	//

	HRESULT DestroyTestWindow( HWND CurrentTestWindowHandle )
	{
		DestroyWindow( CurrentTestWindowHandle );
		return S_OK;
	}

	//

	int GetTestArgumentCount( void )
	{
		lua_Object arg;
		int count = 0;
		
		do {
			arg = lua_getparam( count+2 );
			count++;
		} 
		while( arg && !lua_isnil( arg ) );

		return count-1;
	}

	//

	HRESULT GetTestArgument( int ArgNum, IDR_ARGUMENTTYPE Type, void *default_Value, void *outBuffer, int MaxBufferLen )
	{
		lua_Object arg = lua_getparam( ArgNum+2 );
		if( !arg || lua_isnil(arg) ) {
			switch( Type ) {
			case IDR_AT_VECTOR:
				((float*)outBuffer)[0] = ((float*)(default_Value))[0];
				((float*)outBuffer)[1] = ((float*)(default_Value))[1];
				((float*)outBuffer)[2] = ((float*)(default_Value))[2];
				break;

			case IDR_AT_STRING:	
				_ncpy( (char*)outBuffer, MaxBufferLen, (char*)default_Value );
				break;

			case IDR_AT_FLOAT:
				*((float*)outBuffer) = *((float*)(&default_Value));
				break;

			case IDR_AT_INT:
				*((int*)outBuffer) = ((int)default_Value);
				break;
				
			}
			return E_FAIL;
		}

		switch( Type ) {

		case IDR_AT_VECTOR:
			if( lua_istable( arg ) ) {
				lua_get_vector_float( arg, NULL, 3, (float*)outBuffer );
			}
			else {
				((float*)outBuffer)[0] = ((float*)(default_Value))[0];
				((float*)outBuffer)[1] = ((float*)(default_Value))[1];
				((float*)outBuffer)[2] = ((float*)(default_Value))[2];
			}
			break;

		case IDR_AT_STRING:
			if( lua_isstring( arg ) ) {
				_ncpy( (char*)outBuffer, MaxBufferLen, lua_getstring(arg) );
			}
			else if( lua_isnumber( arg ) ) {
				_stprintf( (char*)outBuffer, "%f", lua_getnumber(arg) );
			}
			else {
				_ncpy( (char*)outBuffer, MaxBufferLen, (char*)default_Value );
			}
			break;

		case IDR_AT_FLOAT:
			*((float*)outBuffer) = lua_getnumber(arg);
			break;

		case IDR_AT_INT:
			*((int*)outBuffer) = lua_getnumber(arg);
			break;

		}

		return S_OK;
	}


	//

	HRESULT GetIniFilename( TCHAR *Filename, int MaxBufferLen )
	{
		TCHAR Path[MAX_PATH];
		TCHAR Name[MAX_PATH];

		if( CurrentTestTable ) {
			lua_get_string( CurrentTestTable, DARTH_TEST_INI_FILE, Filename, MaxBufferLen );
			if( Filename[0] == 0 ) {
				lua_get_string( 0, DARTH_INI_PATH, Path, MAX_PATH );
				lua_get_string( CurrentTestTable, "Name", Name, MAX_PATH );
				_stprintf( Filename, _T("%s%s.ini"), Path, Name );
			}
			return S_OK;
		}

		Filename[0] = 0;
		return E_FAIL;
	}

	//

	HRESULT SaveBufferToFile( IRenderPipeline *RenderPipe, int window_w, int window_h, TCHAR *Filename )
	{
		RPLOCKDATA ld;
		unsigned char *rgb_data, *in_data, *out_data;
		unsigned int src_pxl_u32;
		unsigned int size=0;

		size = window_h * window_w * 4;

		if( (rgb_data = (unsigned char*)malloc( size )) == NULL ) {
			Log( IDR_LC_RUNTIME, 0, _T("Out of memory in SaveBufferToFile") );
			return E_FAIL;
		}	

		memset( rgb_data, 0x7e, size );
		
		if( FAILED( RenderPipe->lock_buffer( &ld ) ) ) {
			return E_FAIL;
		}

		out_data = rgb_data;
		
		int src_bd = (ld.pf.num_bits() + 7 ) / 8;

		unsigned char r,g,b,a;
		for( int y=window_h-1; y>0; y-- ) {
			
			in_data = ((unsigned char*)ld.pixels) + y*ld.pitch;

			for( int x=0; x<window_w; x++ ) {

				src_pxl_u32	=*((U32*)in_data);

				r = ((src_pxl_u32 & ld.pf.get_r_mask()) >> ld.pf.rl) << ld.pf.rr;
				g = ((src_pxl_u32 & ld.pf.get_g_mask()) >> ld.pf.gl) << ld.pf.gr;
				b = ((src_pxl_u32 & ld.pf.get_b_mask()) >> ld.pf.bl) << ld.pf.br;
				a = 0;

				*out_data = b; out_data++;
				*out_data = g; out_data++;
				*out_data = r; out_data++;
				*out_data = a; out_data++;

				in_data += src_bd;
			}
		}

		RenderPipe->unlock_buffer();

		FILE *file;
		BITMAPFILEHEADER bfh;
		BITMAPINFOHEADER bih;
		short bcookie = 0x4d42;
		OPENFILENAME ofn;
		TCHAR CurrDir[MAX_PATH], FName[MAX_PATH];

		if( (Filename == NULL) || (_tcslen( Filename ) == 0) ) {

			memset( &ofn, 0, sizeof(ofn) );

			FName[0] = 0;

			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFilter = _T("Bitmap Files (*.bmp)\0*.bmp\0\0");
			ofn.lpstrFile = FName;
			ofn.nMaxFile = MAX_PATH;

			GetCurrentDirectory( MAX_PATH, CurrDir );
			if( !GetSaveFileName( &ofn ) ) {
				SetCurrentDirectory( CurrDir );
				return E_FAIL;
			}
			SetCurrentDirectory( CurrDir );

			Filename = FName;
		}

		if( (file = _tfopen( Filename, "w" )) == NULL ) {
			return E_FAIL;
		}

		memset( &bfh, 0, sizeof(bfh) );
		bfh.bfType		= bcookie;
		bfh.bfSize		= sizeof(bfh) + sizeof(bih) + size;
		bfh.bfOffBits	= sizeof(bfh) + sizeof(bih);

		fwrite( &bfh, sizeof(bfh), 1, file );

		memset( &bih, 0, sizeof(bih) );
		bih.biSize		   = sizeof(bih);
		bih.biWidth        = window_w;
		bih.biHeight       = window_h;
		bih.biPlanes       = 1;
		bih.biBitCount     = 32;
		bih.biCompression  = BI_RGB;
		bih.biSizeImage    = size;

		fwrite( &bih, sizeof(bih), 1, file );
		
		fwrite( rgb_data, 1, size, file );
		
		free( rgb_data );

		fclose( file );

		return S_OK;
	}

	//

	HRESULT PushReturnValue( IDR_ARGUMENTTYPE Type, void *Value )
	{
		switch( Type ) {
		case IDR_AT_VECTOR:
			ASSERT(0);
			break;
		
		case IDR_AT_STRING:
			lua_pushstring( ((char*)Value) );
			break;

		case IDR_AT_INT:
			lua_pushnumber( *((int*)&Value) );
			break;

		case IDR_AT_FLOAT:
			lua_pushnumber( *((float*)&Value) );
			break;
		}
		return S_OK;
	}




	// Internal Public Interface
	//

	void EarlyExit( void )
	{
		_exit( 255 );
	}
	
	//
	
	struct GOAHEADDATA
	{
		lua_Object TestTable;
		HRESULT ReturnValue;
		DarthRuntime *Runtime;
	};

	//

	static BOOL CALLBACK TestRetryDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		TCHAR Name[MAX_PATH];
		TCHAR String[MAX_PATH];

		switch( uMsg ) {
		case WM_INITDIALOG:
			lua_get_string( ((lua_Object)lParam), "Name", Name, MAX_PATH );
			_stprintf( String, _T("%s failed to complete successfully!"), Name );
			SetWindowText( hDlg, String );
			break;

		case WM_COMMAND:
			switch( LOWORD(wParam) ) {
			case IDC_RETRY:		EndDialog( hDlg, DTM_RETRY );		break;
			case IDC_CONTINUE:	EndDialog( hDlg, DTM_CONTINUE );	break;
			case IDC_ABORT:		EndDialog( hDlg, DTM_ABORT );		break;
			}
			break;
		}

		return FALSE;
	}

	//

	int GetTestRetryFromUser( lua_Object TestTable ) 
	{
		int Mode = 0;
		
		lua_get_int( 0, DARTH_TEST_FAILURE_MODE, &Mode );

		if( Mode == DTM_PROMPT ) {
			Log( IDR_LC_RUNTIME, 0, _T("Prompting user for TestRetry response...") );
			Mode = DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TEST_FAILURE_PROMPT), NULL, TestRetryDlgProc, (LPARAM)TestTable );
		}

		switch( Mode ) {
		
		case DTM_ABORT:			
			Log( IDR_LC_RUNTIME, 0, _T("GetTestRetryFromUser: aborting...") ); 
			EarlyExit();	// does not return

		case DTM_RETRY:			
			Log( IDR_LC_RUNTIME, 0, _T("GetTestRetryFromUser: retrying...") ); 
			return true;
		
		case DTM_CONTINUE:		
			Log( IDR_LC_RUNTIME, 0, _T("GetTestRetryFromUser: continuing...") ); 
			return false;
		
		default: 
			ASSERT( 0 && "Mode is not one of DTM_ABORT, DTM_RETRY, or DTM_CONTINUE" );
		}

		return false;
	}

	//

	static BOOL CALLBACK TestGoAheadDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		static GOAHEADDATA *Data = NULL;
		static TCHAR String[DARTH_BUFFER_SIZE];
		TCHAR Name[MAX_PATH];
		int i;
		
		HWND hRun = GetDlgItem( hDlg, IDC_RUN_TEST );
		HWND hReturn = GetDlgItem( hDlg, IDC_RUN_RETURN );

		switch( uMsg ) {

		case WM_INITDIALOG:
			Data = (GOAHEADDATA *)lParam;

			lua_get_string( Data->TestTable, "Name", Name, MAX_PATH );
			_stprintf( String, _T("Execute %s?"), Name );
			SetWindowText( hDlg, String );

			lua_get_string( Data->TestTable, "Description", Name, MAX_PATH );
			SetWindowText( GetDlgItem( hDlg, IDC_TEST_DESCRIPTION ), Name );

			ComboBox_ResetContent( hRun );
			ComboBox_SetItemData( hRun, ComboBox_AddString( hRun, _T("Execute Test") ),			true );
			ComboBox_SetItemData( hRun, ComboBox_AddString( hRun, _T("Do Not Execute Test") ),	false );
			ComboBox_SetCurSel( hRun, 0);

			ComboBox_ResetContent( hReturn );
			ComboBox_SetItemData( hReturn, ComboBox_AddString( hReturn, _T("Return Success") ),	DARTH_TEST_PASS );
			ComboBox_SetItemData( hReturn, ComboBox_AddString( hReturn, _T("Return Failure") ),	DARTH_TEST_FAIL );

			lua_get_int( 0, DARTH_TEST_DEFAULT_RETURN, &i );
			ComboBox_SetCurSel( hReturn, (i==DARTH_TEST_PASS)? 0 : 1 );

			EnableWindow( hReturn, FALSE );

			break;
		
		case WM_COMMAND:
			if( (HIWORD(wParam) == CBN_SELCHANGE) && (LOWORD(wParam) == IDC_RUN_TEST) ) { 
				if( ComboBox_GetItemData( hRun, ComboBox_GetCurSel( hRun ) ) ) {
					EnableWindow( hReturn, FALSE );
				}
				else {
					EnableWindow( hReturn, TRUE );
				}
			}
			else if( LOWORD(wParam) == IDC_EXIT ) {
				Data->Runtime->EarlyExit();				
			}
			else if( LOWORD(wParam) == IDOK ) {
				Data->ReturnValue = ((HRESULT) ComboBox_GetItemData( hReturn, ComboBox_GetCurSel( hReturn ) ) );
				EndDialog( hDlg, ((bool) ComboBox_GetItemData( hRun, ComboBox_GetCurSel( hRun ) ) ) );
			}
			break;
		}

		return FALSE;
	}

	//

	int GetTestGoAheadFromUser( lua_Object TestTable, HRESULT *outReturn ) 
	{
		GOAHEADDATA Data;
		int Mode = 0;
		int DefaultReturn=0;
		
		lua_get_int( 0, DARTH_TEST_PROMPT_MODE, &Mode );
		lua_get_int( 0, DARTH_TEST_DEFAULT_RETURN, (int *)outReturn );

		switch( Mode ) {
		
		case DTM_ABORT:			EarlyExit();
		case DTM_EXECUTE:		return true;
		case DTM_DONT_EXECUTE:	return false;
		case DTM_PROMPT:
		default:
			Log( IDR_LC_RUNTIME, 0, _T("Prompting user for TestGoAhead response...") );
			Data.TestTable = TestTable;
			Data.Runtime = this;
			if( !DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXECUTE_TEST_PROMPT), GetDesktopWindow(), TestGoAheadDlgProc, ((long)&Data) ) ) {
				*outReturn = Data.ReturnValue;
				return false;
			}
		}

		return true;
	}

	//

	static BOOL CALLBACK LibraryErrorDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		TCHAR **Strings = (TCHAR **)lParam;

		switch( uMsg ) {

		case WM_INITDIALOG:
			SetWindowText( GetDlgItem( hDlg, IDC_MESSAGE ), Strings[0] );
			SetWindowText( GetDlgItem( hDlg, IDC_TRACE_OUTPUT ), Strings[1] );
			break;

		case WM_COMMAND:
			switch( LOWORD(wParam) ) {
			case IDC_CONTINUE:	EndDialog( hDlg, DTM_CONTINUE );	break;
			case IDC_ABORT:		EndDialog( hDlg, DTM_ABORT );		break;
			case IDC_DEBUG:		DebugBreak();						break;
			}
			break;
		}

		return FALSE;
	}

	//

	int GetLibraryErrorFromUser( TCHAR *ErrorMessage ) 
	{
		int Mode = 0;
		TCHAR STrace[MAX_PATH];
		TCHAR *Strings[2] = { ErrorMessage, STrace };

		lua_get_int( 0, DARTH_TEST_LIBERROR_MODE, &Mode );

        DoStackTrace ( STrace, MAX_PATH, 0 );

		if( Mode == DTM_PROMPT ) {
			Log( IDR_LC_RUNTIME, 0, _T("Prompting user for LibraryErrorRetry response...") );
			Mode = DialogBoxParam( GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LIBRARY_ERROR_PROMPT), NULL, LibraryErrorDlgProc, (LPARAM)Strings );
		}

		switch( Mode ) {
		
		case DTM_ABORT:			
			Log( IDR_LC_RUNTIME, 0, _T("GetLibraryErrorFromUser: aborting...") ); 
			EarlyExit();

		case DTM_RETRY:			
			Log( IDR_LC_RUNTIME, 0, _T("GetLibraryErrorFromUser: retrying...") ); 
			return true;
		
		case DTM_CONTINUE:		
			Log