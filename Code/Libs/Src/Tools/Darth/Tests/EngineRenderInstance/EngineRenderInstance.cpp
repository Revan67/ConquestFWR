// EngineRenderInstance.cpp
//
// 
//


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "DarthTest.h"
#include "DarthCamera.h"
#include "resource.h"

#include "DACOM.h"
#include "System.h"
#include "Engine.h"
#include "RendPipeline.h"
#include "FileSys.h"
#include "Lightman.h"
#include "RPUL/PrimitiveBuilder.h"
#include "Renderer.h"

//

static ICOManager		*s_DACOM = NULL;
static ISystemContainer *s_SYSTEM = NULL;
static IEngine			*s_ENGINE = NULL;
static IRenderPipeline  *s_RENDERPIPE = NULL;
static ILightManager	*s_LIGHTMAN = NULL;

const float WORLD_ANIMATE_SPEED = 3.14 / 24;
const float FSHACK_WIDTH = 252;
const float FSHACK_HEIGHT = 225;

DarthCamera				s_Camera;
static INSTANCE_INDEX	s_ModelToRender = INVALID_INSTANCE_INDEX;

bool	s_DoSave = false;
TCHAR	s_Filename[MAX_PATH+1];
TCHAR	s_GoodImage[MAX_PATH+1];
TCHAR	s_ThisImage[MAX_PATH+1];
TCHAR	s_IniFile[MAX_PATH+1];
TCHAR	s_TestText[MAX_PATH+1];
HANDLE  s_GoodBitmap = 0;
HANDLE  s_ThisBitmap = 0;
float	s_Ambient[3];
U32		s_Windowed = 0;

float	s_ModelRotX = 0;
float	s_ModelRotY = 0;
float	s_ModelRotZ = 0;

static HANDLE			s_Module = 0;

//

DARTH_DEFINE_NAME(			"EngineRenderInstance" )
DARTH_DEFINE_CATEGORY(		"Engine" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_DESCRIPTION(	"Creates an Engine instance and continually renders it with render_instance" )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

static BOOL CALLBACK CompareViewDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static IDarthRuntime *RunTime = NULL;
	PAINTSTRUCT ps;
	RECT R;
	HWND hRender = GetDlgItem( hDlg, IDC_YOU_VIEW );
	HWND hSample = GetDlgItem( hDlg, IDC_SAMPLE_VIEW );
	static TCHAR dummy_file[MAX_PATH];

	switch( uMsg ) {

	case WM_INITDIALOG:
		
		RunTime = (IDarthRuntime *)lParam;

		SetWindowText( GetDlgItem( hDlg, IDC_TEST_DESCRIPTION ), s_TestText );
		
		ScrollBar_SetRange( GetDlgItem( hDlg, IDC_ROTATE_X ), 0, 180, FALSE );
		ScrollBar_SetPos( GetDlgItem( hDlg, IDC_ROTATE_X ), s_ModelRotX + 90, FALSE );

		ScrollBar_SetRange( GetDlgItem( hDlg, IDC_ROTATE_Y ), 0, 180, FALSE );
		ScrollBar_SetPos( GetDlgItem( hDlg, IDC_ROTATE_Y ), s_ModelRotY + 90, FALSE );
		
		ScrollBar_SetRange( GetDlgItem( hDlg, IDC_ROTATE_Z ), 0, 90, FALSE );
		ScrollBar_SetPos( GetDlgItem( hDlg, IDC_ROTATE_Z ), s_ModelRotZ + 45, FALSE );

		GetClientRect( hRender, &R );

		s_ThisBitmap = 0;
		if( !s_Windowed ) {
			// render scene twice to get front/back buffers filled.
			s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
			s_RENDERPIPE->begin_scene();
			s_Camera.SetViewport( 0, 0, R.right, R.bottom );
			s_Camera.Render( NULL, s_RENDERPIPE );
			s_ENGINE->update_instance( s_ModelToRender, 0.0001 );
			s_ENGINE->render_instance( s_Camera.GetICamera(), s_ModelToRender );
			s_RENDERPIPE->end_scene();
			s_RENDERPIPE->swap_buffers();

			s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
			s_RENDERPIPE->begin_scene();
			s_Camera.SetViewport( 0, 0, R.right, R.bottom );
			s_Camera.Render( NULL, s_RENDERPIPE );
			s_ENGINE->update_instance( s_ModelToRender, 0.0001 );
			s_ENGINE->render_instance( s_Camera.GetICamera(), s_ModelToRender );
			s_RENDERPIPE->end_scene();
			s_RENDERPIPE->swap_buffers();
			
			_tcscpy( dummy_file, _T("\\dummy.bmp") );

			RunTime->SaveBufferToFile( s_RENDERPIPE, R.right, R.bottom, dummy_file );
			
			if( (s_ThisBitmap = LoadImage( NULL, dummy_file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE )) == NULL ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("Unable to find thisimage file: %s"), dummy_file );
			}
			
			s_RENDERPIPE->destroy_buffers();

			EnableWindow( GetDlgItem( hDlg, IDC_ROTATE_X ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_ROTATE_Y ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_ROTATE_Z ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_ANIMATE ), FALSE );
			EnableWindow( GetDlgItem( hDlg, IDC_RESET ), FALSE );
		}

		SetTimer( hDlg, 0, 13, NULL );
		break;

	case WM_COMMAND:
		switch( LOWORD(wParam) ) {
		
		case IDOK:	
			RunTime->Log( IDR_LC_TEST, 0, _T("User says it passes") );
			EndDialog( hDlg, DARTH_TEST_PASS );		
			break;
		
		case IDCANCEL:	
			RunTime->Log( IDR_LC_TEST, 0, _T("User says it does not pass") );
			EndDialog( hDlg, DARTH_TEST_FAIL );		
			break;
		
		case IDC_RESET:	
			{
				s_ModelRotX = 0;
				s_ModelRotY = 0;
				s_ModelRotZ = 0;
				Matrix o = Matrix().rotate_x(s_ModelRotX) * Matrix().rotate_y(s_ModelRotY) * Matrix().rotate_z(s_ModelRotZ);
				s_ENGINE->set_orientation( s_ModelToRender, o );
			}
			break;

		case IDC_SAVE_OUTPUT:
			if( s_Windowed ) {
				s_Camera.Render( hRender, s_RENDERPIPE );
				s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
				s_RENDERPIPE->begin_scene();
				s_ENGINE->render_instance( s_Camera.GetICamera(), s_ModelToRender );
				s_RENDERPIPE->end_scene();
				s_RENDERPIPE->swap_buffers();

				GetClientRect( hRender, &R );

				if( s_ThisImage[0] == 0 ) {
					RunTime->SaveBufferToFile( s_RENDERPIPE, R.right, R.bottom, NULL );
				}
				else {
					RunTime->SaveBufferToFile( s_RENDERPIPE, R.right, R.bottom, s_ThisImage );
				}
			}
			else {
				OPENFILENAME ofn;
				TCHAR CurrDir[MAX_PATH];

				if( s_ThisImage[0] == 0 ) {

					memset( &ofn, 0, sizeof(ofn) );

					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFilter = _T("Bitmap Files (*.bmp)\0*.bmp\0\0");
					ofn.lpstrFile = s_ThisImage;
					ofn.nMaxFile = MAX_PATH;

					GetCurrentDirectory( MAX_PATH, CurrDir );
					if( GetSaveFileName( &ofn ) ) {
						CopyFile( dummy_file, s_ThisImage, FALSE );
					}
					SetCurrentDirectory( CurrDir );

					s_ThisImage[0] = 0;
				}
				else {
					CopyFile( dummy_file, s_ThisImage, FALSE );
				}
			}

			break;
		}
		break;

	case WM_HSCROLL:
	case WM_VSCROLL:
		{
			int nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			short int nPos = (short int) HIWORD(wParam);  // scroll box position 
			int Max = 180;
			HWND hThis = (HWND) lParam;      
			HWND hRotX = GetDlgItem( hDlg, IDC_ROTATE_X );
			HWND hRotY = GetDlgItem( hDlg, IDC_ROTATE_Y );
			HWND hRotZ = GetDlgItem( hDlg, IDC_ROTATE_Z );

#define clamp( val, mn, mx )  ( ((val)<(mn))?  (mn) : ( ((val)>(mx))?  (mx) : (val) ) )

			if( hThis == hRotZ ) {
				Max = 90;
			}

			switch( nScrollCode ) {
				case SB_BOTTOM:
				case SB_ENDSCROLL:
					break;

				case SB_LINEDOWN:
					nPos = ScrollBar_GetPos( hThis );
					nPos = clamp( nPos+1, 0, Max );
					ScrollBar_SetPos( hThis, nPos, FALSE );
					break;

				case SB_LINEUP:
					nPos = ScrollBar_GetPos( hThis );
					nPos = clamp( nPos-1, 0, Max );
					ScrollBar_SetPos( hThis, nPos, FALSE );
					break;

				case SB_PAGEDOWN:
					nPos = ScrollBar_GetPos( hThis );
					nPos = clamp( nPos+5, 0, Max );
					ScrollBar_SetPos( hThis, nPos, FALSE );
					break;

				case SB_PAGEUP:
					nPos = ScrollBar_GetPos( hThis );
					nPos = clamp( nPos-5, 0, Max );
					ScrollBar_SetPos( hThis, nPos, FALSE );
					break;

				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					nPos = clamp( nPos, 0, Max );
					ScrollBar_SetPos( hThis, nPos, FALSE );
					break;

				case SB_TOP:
					break;
			}

			InvalidateRect( hThis, NULL, TRUE );

			s_ModelRotX = ((float)ScrollBar_GetPos( hRotX )) - 90.0f;	
			s_ModelRotY = ((float)ScrollBar_GetPos( hRotY )) - 90.0f;	
			s_ModelRotZ = ((float)ScrollBar_GetPos( hRotZ )) - 45.0f;		
			Matrix o = Matrix().rotate_x(s_ModelRotX) * Matrix().rotate_y(s_ModelRotY) * Matrix().rotate_z(s_ModelRotZ);
			s_ENGINE->set_orientation( s_ModelToRender, o );
		}
		break;

	case WM_TIMER:
	case WM_PAINT:
		{
			GetClientRect( hRender, &R );

			BeginPaint( hDlg, &ps );
				HDC hSampleDC = GetDC( hSample );
				HDC hBmpDC = CreateCompatibleDC( hSampleDC );
				HANDLE old = SelectObject( hBmpDC, s_GoodBitmap );
				BitBlt( hSampleDC, 0, 0, R.right, R.bottom, hBmpDC, 0, 0, SRCCOPY );
				SelectObject( hBmpDC, old );
				ReleaseDC( hSample, hSampleDC );
				DeleteDC( hBmpDC );

				if( !s_Windowed ) {
					HDC hRenderDC = GetDC( hRender );
					hBmpDC = CreateCompatibleDC( hRenderDC );
					old = SelectObject( hBmpDC, s_ThisBitmap );
					BitBlt( hRenderDC, 0, 0, R.right, R.bottom, hBmpDC, 0, 0, SRCCOPY );
					SelectObject( hBmpDC, old );
					ReleaseDC( hRender, hRenderDC );
					DeleteDC( hBmpDC );
				}
			EndPaint( hDlg, &ps );
			
			if( s_Windowed ) {
				s_Camera.Render( hRender, s_RENDERPIPE );
				
				s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );

				s_RENDERPIPE->begin_scene();

				if( Button_GetCheck( GetDlgItem( hDlg, IDC_ANIMATE ) ) ) {
					s_ModelRotX += WORLD_ANIMATE_SPEED;
					Matrix o = Matrix().rotate_x(s_ModelRotX) * Matrix().rotate_y(s_ModelRotY) * Matrix().rotate_z(s_ModelRotZ);
					s_ENGINE->set_orientation( s_ModelToRender, o );
				}

				s_ENGINE->update_instance( s_ModelToRender, 0.0001 );
				s_ENGINE->render_instance( s_Camera.GetICamera(), s_ModelToRender );
				
				s_RENDERPIPE->end_scene();

				s_RENDERPIPE->swap_buffers();

				if( s_DoSave ) {
					s_DoSave = false;
					RunTime->SaveBufferToFile( s_RENDERPIPE, R.right, R.bottom, s_ThisImage[0]? s_ThisImage : NULL );
				}
			}
		}
		break;

	case WM_DESTROY:
		KillTimer( hDlg, 0 );
		break;
	}

	return FALSE;
}

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	float default_ambient[3] = { 1.0, 1.0, 1.0 };
	IRenderer *IR = NULL;
	HRESULT Return = DARTH_TEST_FAIL;
	char device_id[MAX_PATH];
	char *device_id_ptr = NULL;
	IFileSystem *IFS;

	if( RunTime->GetTestArgumentCount() < 1 ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("EngineRenderInstance requires at least one argument: object_name") );
		return E_FAIL;
	}

	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	RunTime->GetTestArgument( 1, IDR_AT_STRING, _T(""),	s_Filename, MAX_PATH );
	RunTime->GetTestArgument( 2, IDR_AT_STRING, _T(""), s_GoodImage, MAX_PATH );
	RunTime->GetTestArgument( 3, IDR_AT_STRING, _T(""),	s_ThisImage, MAX_PATH );
	RunTime->GetTestArgument( 4, IDR_AT_INT,    ((void*)FALSE),	&s_DoSave,	0 );
	RunTime->GetTestArgument( 5, IDR_AT_STRING, _T("Compare the two images below."), s_TestText, MAX_PATH );
	RunTime->GetTestArgument( 6, IDR_AT_VECTOR, default_ambient, s_Ambient, 0 );

	if( _tcslen( device_id ) ) {
		device_id_ptr = device_id;
	}

	RunTime->GetIniFilename( s_IniFile, MAX_PATH );

	TCHAR CurrDir[MAX_PATH];
	GetCurrentDirectory( MAX_PATH, CurrDir );

	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, s_IniFile, goto cleanup );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, goto cleanup );
	DARTH_REQUIRE_ENGINE( RunTime, s_DACOM, s_SYSTEM, s_ENGINE, goto cleanup );

	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_RENDERPIPE, goto cleanup );
	DARTH_REQUIRE_INTERFACE( RunTime, s_ENGINE, IID_ILightManager, s_LIGHTMAN, goto cleanup );

	if( FAILED( s_RENDERPIPE->startup( device_id_ptr ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("Unable to start renderpipe") );
		goto cleanup;
	}
	
	s_RENDERPIPE->query_device_ability( RP_A_DEVICE_WINDOWED, &s_Windowed, NULL );

	if( !s_Windowed ) {
		s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_FULLSCREEN, TRUE );
	}

	if( FAILED( s_RENDERPIPE->create_buffers( GetDesktopWindow(), 640, 480 ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("create_buffers( GetDesktopWindow(), 640, 480 ) failed") );
		goto cleanup;
	}

	s_LIGHTMAN->set_ambient_light( s_Ambient[0] * 255, s_Ambient[1] * 255, s_Ambient[2] * 255 );

	s_ENGINE->update( 0.0 );

	s_ModelRotX = 0;
	s_ModelRotY = 0;
	s_ModelRotZ = 0;

	if( FAILED( s_ENGINE->create_file_system( s_Filename, &IFS ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("create_file_system( %s, &IFS ) failed"), s_Filename );
		goto cleanup;
	}

	if( (s_ModelToRender = s_ENGINE->create_instance( s_Filename, IFS )) == INVALID_INSTANCE_INDEX ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("create_instance( %s, IFS ) failed"), s_Filename );
		goto cleanup;
	}

	s_Camera.ResetToDefaults( s_ENGINE, s_ModelToRender );

	if( (s_GoodBitmap = LoadImage( NULL, s_GoodImage, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE )) == NULL ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("Unable to find goodimage file: %s"), s_GoodImage );
	}
	
	Return = DialogBoxParam( (HINSTANCE)s_Module, MAKEINTRESOURCE(IDD_COMPARE_VIEW), 0, CompareViewDialogProc, (LPARAM)RunTime );

cleanup:
	DeleteObject( s_GoodBitmap );
	DeleteObject( s_ThisBitmap );

	s_Camera.ReleaseAll();

	if( s_RENDERPIPE ) {
		s_RENDERPIPE->shutdown();
	}

	DARTH_RELEASE( s_LIGHTMAN );
	DARTH_RELEASE( s_RENDERPIPE );
	DARTH_RELEASE( s_ENGINE );
	DARTH_RELEASE( s_SYSTEM );

	if( s_DACOM ) {
		s_DACOM->ShutDown();
	}

	MATH_ENGINE_Uninitialize();

	return Return;
}

//
