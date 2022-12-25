// ObjectViewer.cpp
//
//
//
//


#define STRICT
#include <windows.h>
#include <comdef.h>
#include <afxres.h>
#include <Zmouse.h>

#include "resource.h"

#include "dacom.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "IProfileParser.h"
#include "RPUL.h"

#include "DPF.h"
#include "DACOM_Utility.h"
#include "MessageCrackers.h"
#include "DACOMProvider.h"
#include "CommonDialog.h"
#include "Options_inl.cpp"

#include "IWindow.h"
#include "IPersistable.h"
#include "IRenderable.h"





#pragma warning(disable:4800) // unsigned long: forcing value to bool

//

const char *OBJECTVIEWER_INI = "DACOM.ini";

// ObjectViewer App
//
//
//

class App
{
public:
	BEGIN_STATIC_WP_MAPS(App)
		BEGIN_COMMAND_MAP
			// File 
			ON_COMMAND(ID_FILE_NEW_SCENEWINDOW,OnFileNewWindow)
			ON_COMMAND(ID_FILE_NEW_DIRECTSHOWWINDOW,OnFileNewWindow)
			ON_COMMAND(ID_FILE_NEW_TEXTUREVIEWER,OnFileNewWindow)
			ON_COMMAND(ID_FILE_NEW_OTHER,OnFileNewWindow)
			ON_COMMAND(ID_FILE_DEVICE,OnFileDevice)
			ON_COMMAND(ID_APP_EXIT,OnAppExit)

			// System
			ON_COMMAND(ID_SYSTEM_RENDERPIPELINE_PROFILELOGGING,OnSystemRPProfileLogging)

		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_DESTROY,OnDestroy)
			ON_MESSAGE(WM_PAINT,OnPaint)
//			ON_MESSAGE_DEFAULT(OnDefault)
		END_MESSAGE_MAP
	END_STATIC_WP_MAPS

	// Message Handlers
	//
	
	HRESULT OnFileNewWindow( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnFileDevice( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnAppExit( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSystemRPProfileLogging( UINT wID, HWND hControl, UINT NotifyCode );

	// Windows message handlers
	//
	HRESULT OnDestroy( UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT OnPaint( UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT OnDefault( UINT message, WPARAM wParam, LPARAM lParam );

	// Initialization and Cleanup Code
	//
	HRESULT Initialize( HINSTANCE hInst, LPSTR lpCmdLine );
	HRESULT Cleanup();

	HRESULT ParseCommandLine( LPSTR lpCmdLine );
	HRESULT OnIdle();
	HRESULT MessagePump();

	HRESULT CreateAppWindow( void );

	App();
	~App();

protected:
	static LRESULT CALLBACK PopupWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
	HRESULT CreatePopupWindow( const char *clsid, IWindow **out_win = NULL );
	static BOOL CALLBACK OnFileNewWindowDlgProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam );

protected:
	HINSTANCE					m_hInst;
	HWND						m_hWnd;
	CDACOMProvider				*m_DACOM;
	COMPTR<IRenderPipeline>		m_IRenderPipe;

	U32							m_Width;
	U32							m_Height;
	U32							m_MouseWheelMsg;
};


// --------------------------------------------------------------------------------
//
// --------------------------------------------------------------------------------

HRESULT App::OnPaint( UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	BeginPaint( m_hWnd, &ps );
	EndPaint( m_hWnd, &ps );
	return MSG_HANDLED;
}

//

HRESULT App::OnDestroy( UINT message, WPARAM wParam, LPARAM lParam )
{
	Cleanup();
	PostQuitMessage(0);		
	return MSG_HANDLED;
}

//

HRESULT App::OnDefault( UINT message, WPARAM wParam, LPARAM lParam )
{
	if( message == m_MouseWheelMsg ) {
		SendMessage( m_hWnd, WM_MOUSEWHEEL, wParam, lParam );
		return MSG_HANDLED;
	}

	return MSG_NOT_HANDLED;
}

//

BOOL CALLBACK App::OnFileNewWindowDlgProc( HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam )
{
	static char *text = NULL;
	switch( iMsg ) {
	case WM_INITDIALOG:
		text = (char *)lParam;
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD(wParam) ) {
		case IDOK:
			SendMessage( GetDlgItem( hWnd, IDC_WINDOWTYPE ), WM_GETTEXT, 255, (long)text );
			EndDialog( hWnd, 0 );
			return TRUE;

		case IDCANCEL:
			strcpy( text, "" );
			EndDialog( hWnd, 0 );
			return TRUE;
		}
	}

	return FALSE;
}


HRESULT App::OnFileNewWindow( UINT wID, HWND hControl, UINT NotifyCode )
{
	char type[255+1];
	static char last_other[255+1] = "";
	
	switch( wID ) {
	case ID_FILE_NEW_SCENEWINDOW:		strcpy( type, "CDALibs_3DWindow" );				break;
	case ID_FILE_NEW_DIRECTSHOWWINDOW:	strcpy( type, "CDALibs_DirectShowWindow" );		break;
	case ID_FILE_NEW_TEXTUREVIEWER:		strcpy( type, "CDALibs_TextureWindow" );		break;
	case ID_FILE_NEW_OTHER:				
		strcpy( type, last_other );
		DialogBoxParam( m_hInst, MAKEINTRESOURCE(IDD_NEWWINDOWDIALOG), m_hWnd, (DLGPROC)App::OnFileNewWindowDlgProc, (long)type );
		if( strcmp( type, "" ) == 0 ) {
			return MSG_HANDLED;
		}
		strcpy( last_other, type );
		break;
	}

	if( FAILED( CreatePopupWindow( type, NULL ) ) ) {
		GENERAL_ERROR( "Unable to create new window" );
	}

	return MSG_HANDLED;
}

//

HRESULT App::OnFileDevice( UINT wID, HWND hControl, UINT NotifyCode )
{
	MessageBox( m_hWnd, "Unsupported.", "Warning", MB_OK );
	return MSG_HANDLED;
}

//

HRESULT App::OnAppExit( UINT wID, HWND hControl, UINT NotifyCode )
{
	DestroyWindow( m_hWnd );
	return MSG_HANDLED;
}

//

HRESULT App::OnSystemRPProfileLogging( UINT wID, HWND hControl, UINT NotifyCode )
{
	U32 val;
	m_IRenderPipe->get_pipeline_state( RP_DEBUG_PROFILE_LOG, &val );
	m_IRenderPipe->set_pipeline_state( RP_DEBUG_PROFILE_LOG, 1-val );
	CheckMenuItem( GetMenu(m_hWnd), wID, MF_BYCOMMAND| (1-val)?MF_CHECKED:MF_UNCHECKED );
	return MSG_HANDLED;
}

//

HRESULT App::ParseCommandLine( LPSTR lpCmdLine )
{
	char *token;

	token = strtok( lpCmdLine, " \t\n" );
	while( token ) {
		if( !strncmp( token, "--", 2 ) ) {
			token += 2;
		}
		else {
		// OpenNewWindow( token, NULL );
		}
		token = strtok( NULL, " \t\n" );
	}

	return S_OK;
}

//

BOOL PrintDevices( IDAComponent *device, void *user )
{
	return TRUE;
}


// Initialization and Cleanup Code

HRESULT App::Initialize( HINSTANCE hInst, LPSTR lpCmdLine )
{
	m_DACOM = new CDACOMProvider();
	if( FAILED ( m_DACOM->Initialize( OBJECTVIEWER_INI ) ) ) {
		return E_FAIL;
	}	
	
	if( FAILED( m_DACOM->System->QueryInterface( IID_IRenderPipeline, (void**) &m_IRenderPipe ) ) ) {
		GENERAL_WARNING( "Could not query IRenderPipeline" );
	}

	if( FAILED( m_IRenderPipe->startup() ) ) { 
		GENERAL_WARNING( "Could not startup IRenderPipeline" );
		return E_FAIL;
	}

	U32 n;
	EnumerateRenderPipelineDevices( 0xFFFFFFFF, PrintDevices, NULL, &n );

	m_hInst = hInst;
	m_MouseWheelMsg = RegisterWindowMessage(MSH_MOUSEWHEEL);


	opt_get_u32( m_DACOM->Manager, NULL, "Application", "Width", 800, &m_Width );
	opt_get_u32( m_DACOM->Manager, NULL, "Application", "Height", 600, &m_Height);

	extern HRESULT RegisterCDALibs_Camera( ICOManager *dacom );
	RegisterCDALibs_Camera( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_Light( ICOManager *dacom );
	RegisterCDALibs_Light( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_Renderable( ICOManager *dacom );
	RegisterCDALibs_Renderable( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_Decorator( ICOManager *dacom );
	RegisterCDALibs_Decorator( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_3DWindow( ICOManager *dacom );
	RegisterCDALibs_3DWindow( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_BasicSceneDatabase( ICOManager *dacom );
	RegisterCDALibs_BasicSceneDatabase( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_TextureWindow( ICOManager *dacom );
	RegisterCDALibs_TextureWindow( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_TextureTestWindow( ICOManager *dacom );
	RegisterCDALibs_TextureTestWindow( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_TextureLibraryTestWindow( ICOManager *dacom );
	RegisterCDALibs_TextureLibraryTestWindow( m_DACOM->Manager );

	extern HRESULT RegisterCDALibs_GammaTestWindow( ICOManager *dacom );
	RegisterCDALibs_GammaTestWindow( m_DACOM->Manager );

	m_DACOM->Engine->update(0);

	return S_OK;
}

//

HRESULT App::Cleanup( )
{
	if( m_IRenderPipe ) {
		m_IRenderPipe->shutdown();
		m_IRenderPipe = NULL;
	}

	delete m_DACOM;
	m_DACOM = NULL;
	
	return S_OK;
}

//


HRESULT App::CreateAppWindow()
{
	HWND                hwnd;
	WNDCLASS            wc;

	/*
	 * set up and register window class
	 */
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = App::HandleMessage;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInst;
	wc.hIcon = LoadIcon( m_hInst, MAKEINTRESOURCE(IDI_SWICON) );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_APPLICATION_MENU);
	wc.lpszClassName = "TheApp";

	RegisterClass( &wc );

	DWORD dwStyle;
	RECT R;
	dwStyle = WS_OVERLAPPEDWINDOW;
	GetWindowRect( GetDesktopWindow(), &R );
	R.left = R.right/2 - m_Width/2;
	R.top  = R.bottom/2 - m_Height/2;
	R.right = R.left + m_Width;
	R.bottom = R.top + m_Height;
	AdjustWindowRect( &R, dwStyle, TRUE );

	if( !(hwnd = CreateWindowEx(0,"TheApp", "ObjectViewer", dwStyle, R.left,R.top,R.right-R.left,R.bottom-R.top, NULL,NULL,m_hInst,NULL )) ) {
		DPF( "ObjectViewer: Could not createwindowex\n" );
		return 1;
	}
	
	m_hWnd = hwnd;
	SetWindowLong( hwnd, GWL_USERDATA, (LONG)this );
	ShowWindow( hwnd, SW_SHOWNORMAL );
	UpdateWindow( hwnd );

	// Setup default check states
	//
	U32 log;
	m_IRenderPipe->get_pipeline_state( RP_DEBUG_PROFILE_LOG, &log );
	CheckMenuItem( GetMenu(m_hWnd), ID_SYSTEM_RENDERPIPELINE_PROFILELOGGING, MF_BYCOMMAND| (log)?MF_CHECKED:MF_UNCHECKED );

	U32 bw, bh;
	opt_get_u32( m_DACOM->Manager, NULL, "Application", "BuffersWidth", m_Width, &bw );
	opt_get_u32( m_DACOM->Manager, NULL, "Application", "BuffersHeight", m_Height, &bh );
	if( FAILED( m_IRenderPipe->create_buffers( m_hWnd, bw, bh ) ) ) {
		return E_FAIL;
	}

#if 1
	char deftype[255+1];
	opt_get_string( m_DACOM->Manager, NULL, "Application", "DefaultWindowType", "CDALibs_3DWindow", deftype, 255 );
	if( FAILED( CreatePopupWindow( deftype ) ) ) {
		MessageBox( m_hWnd, "Could not create default window", "Error", MB_OK );
		return E_FAIL;
	}
#endif

	return S_OK;
}

//

HRESULT App::OnIdle( )
{
//	static U32 last_time = 0;
//	
//	U32 time = timeGetTime();
//	if( last_time == 0 ) {
//		last_time = time;
//	}
//
//	m_DACOM->Engine->update( ((float)(time-last_time)) );
//	m_DACOM->Engine->update( 0.0001 );
	return MSG_HANDLED;
}

//

HRESULT App::MessagePump( )
{
	MSG msg;

	while(1) {
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
			if( GetMessage( &msg, NULL, 0, 0 ) ) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else {
				break;
			}
		}
		else {
		//	OnIdle();
		}
	}
	return (HRESULT)msg.wParam;
}

//

App::App()
{
	m_DACOM = NULL;
	m_hWnd = NULL;
	m_hInst = NULL;
	m_Width = 0;
	m_Height =0;
}

//

App::~App()
{
	Cleanup();
}

//

LRESULT CALLBACK App::PopupWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	HWND cwnd;
	IWindow *iwin = (IWindow*)GetWindowLong( hwnd, GWL_USERDATA );

	switch( uMsg ) {

	case WM_SETFOCUS:
		if( iwin ) {
			iwin->GetHandle( &cwnd );
			SetFocus( cwnd );
		}
		break;

	case WM_PAINT:
		BeginPaint( hwnd, &ps );
		EndPaint( hwnd, &ps );
		return TRUE;

	case WM_DESTROY:
		RELEASE( iwin );
		SetWindowLong( hwnd, GWL_USERDATA, 0 );
		return TRUE;

	case WM_RBUTTONUP:
		{
			U32 foo = 0;
		}
		break;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

//

HRESULT App::CreatePopupWindow( const char *clsid, IWindow **out_win )
{
	if( out_win ) {
		*out_win = NULL;
	}

	WNDCLASS wc = 
	{
		CS_HREDRAW|CS_VREDRAW,
		App::PopupWindowProc,
		0,
		0,
		m_hInst,
		LoadIcon( m_hInst, MAKEINTRESOURCE(IDI_SWICON) ),
		LoadCursor( NULL, IDC_ARROW ),
		(HBRUSH)GetStockObject( LTGRAY_BRUSH ),
		NULL,
		"AppPopupWindow"
	};

	RegisterClass( &wc );

	HWND wnd;
	U32 style;
	RECT wr,cr,pcr;
	int w, h, x, y;
	
//	style = WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	style = WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_CLIPCHILDREN;
	
	GetClientRect( m_hWnd, &pcr );
	
	w = opt_get_u32( m_DACOM->Manager, NULL, clsid, "Width", 640, NULL );
	h = opt_get_u32( m_DACOM->Manager, NULL, clsid, "Height", 480, NULL );
	x = opt_get_u32( m_DACOM->Manager, NULL, clsid, "X", pcr.right/2-w/2, NULL ); 
	y = opt_get_u32( m_DACOM->Manager, NULL, clsid, "Y", pcr.bottom/2-h/2, NULL ); 
	SetRect( &cr, 0,0,w,h );

	SetRect( &wr, x, y, x+w, y+h );
	AdjustWindowRect( &wr, style, FALSE );
	x = wr.left;
	y = wr.top;
	w = wr.right - wr.left;
	h = wr.bottom - wr.top;

	if( x < 0 ) {
		x += -x;
	}
	if( y < 0 ) {
		y += -y;
	}

	if( style & WS_OVERLAPPEDWINDOW ) {
		POINT pt;
		pt.x = x;
		pt.y = y;
		ClientToScreen( m_hWnd, &pt );
		x = pt.x;
		y = pt.y;
	}



	wnd = CreateWindowEx( 0, "AppPopupWindow", clsid, style, x, y, w, h, m_hWnd, NULL, m_hInst, NULL );
	if( wnd == NULL ) {
		return E_FAIL;
	}

	COMPTR<IWindow> IWin;
#if 1
	HWND cwnd;
	COMPTR<IDAComponent> Win;
	CLSID_DACOMDESC desc( clsid, m_DACOM->System, m_DACOM->Engine );
	if( FAILED( m_DACOM->Manager->CreateInstance( &desc, (void**) &Win ) ) ) {
		return E_FAIL;
	}

	if( SUCCEEDED( Win->QueryInterface( IID_IWindow, (void**) &IWin ) ) ) {
		IWINDOWCREATEDATA cd;
		cd.hParent = wnd;
		cd.rChildRect = cr;
		if( FAILED( IWin->Create( &cd, &cwnd ) ) ) {
			return E_FAIL;
		}
	}

	IWin->AddRef();
#endif
	SetWindowLong( wnd, GWL_USERDATA, (long)IWin.ptr );

	SetFocus( wnd );
	ShowWindow( wnd, SW_SHOWNORMAL );
	UpdateWindow( wnd );

	if( out_win ) {
		*out_win = IWin;
		IWin->AddRef();
	}

	return S_OK;
}

// --------------------------------------------------------------------------------
//
// --------------------------------------------------------------------------------

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hPrevInstance = hPrevInstance;

	App theApp;

	if( FAILED( theApp.Initialize( hInstance, lpCmdLine ) ) ) {
		DPF( "ObjectViewer: Could not initialize system\n" );
		return 0;
	}

	if( FAILED( theApp.CreateAppWindow() ) ) {
		DPF( "ObjectViewer: Could not initialize app window\n" );
		return 0;
	}

	if( FAILED( theApp.ParseCommandLine( lpCmdLine ) ) ) {
		return E_FAIL;
	}

    return (int)theApp.MessagePump();
} /* WinMain */

