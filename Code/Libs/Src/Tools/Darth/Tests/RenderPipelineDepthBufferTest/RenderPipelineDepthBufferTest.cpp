// RenderPipelineDepthBufferTest.cpp
//
// 
//


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "DarthTest.h"
#include "resource.h"

#include "DACOM.h"
#include "System.h"
#include "RendPipeline.h"
#include "RPUL/PrimitiveBuilder.h"

//

static ICOManager		*s_DACOM = NULL;
static ISystemContainer *s_SYSTEM = NULL;
static IRenderPipeline  *s_RENDERPIPE = NULL;

const float FSHACK_WIDTH = 252;
const float FSHACK_HEIGHT = 225;

bool	s_DoSave = false;
TCHAR	s_Filename[MAX_PATH+1];
TCHAR	s_ThisImage[MAX_PATH+1];
TCHAR	s_IniFile[MAX_PATH+1];
HANDLE  s_ThisBitmap = 0;
U32		s_Windowed = 1;

static HANDLE s_Module = 0;

//

DARTH_DEFINE_NAME(			"RenderPipelineDepthBufferTest" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Depth" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_DESCRIPTION(	"Renders screen-aligned quads in different depths" )
DARTH_DEFINE_DLLMAIN(		s_Module )

//
const float quad_width = 32.0f;
const float quad_height = 32.0f;
const float half_width = quad_width/2;
const float half_height = quad_height/2;
const float depth_inc = 0.10f;


void RenderDepthTest( float center_x, float center_y, D3DCMPFUNC func, unsigned long clear, float scale )
{
	s_RENDERPIPE->set_pipeline_state( RP_CLEAR_DEPTH, clear );
	s_RENDERPIPE->clear_buffers( RP_CLEAR_DEPTH_BIT, NULL );
	
	s_RENDERPIPE->set_render_state( D3DRS_ZFUNC, func );


	PrimitiveBuilder pb( s_RENDERPIPE, 24 );

	pb.Begin( PB_QUADS );

	pb.Color3ub( 255, 0, 0 );
	pb.Vertex3f( center_x-quad_width,	center_y-quad_height,	scale * 1.0 * depth_inc );
	pb.Vertex3f( center_x-quad_width,	center_y,				scale * 1.0 * depth_inc );
	pb.Vertex3f( center_x,				center_y,				scale * 1.0 * depth_inc );
	pb.Vertex3f( center_x,				center_y-quad_height,	scale * 1.0 * depth_inc );

	pb.Color3ub( 0, 255, 0 );
	pb.Vertex3f( center_x-half_width,	center_y-half_height,	scale * 3.0 * depth_inc );
	pb.Vertex3f( center_x-half_width,	center_y+half_height,	scale * 3.0 * depth_inc );
	pb.Vertex3f( center_x+half_width,	center_y+half_height,	scale * 3.0 * depth_inc );
	pb.Vertex3f( center_x+half_width,	center_y-half_height,	scale * 3.0 * depth_inc );

	pb.Color3ub( 0, 0, 255 );
	pb.Vertex3f( center_x,				center_y,				scale * 5.0 * depth_inc );
	pb.Vertex3f( center_x,				center_y+quad_height,	scale * 5.0 * depth_inc );
	pb.Vertex3f( center_x+quad_width,	center_y+quad_height,	scale * 5.0 * depth_inc );
	pb.Vertex3f( center_x+quad_width,	center_y,				scale * 5.0 * depth_inc );

	pb.End();
}

//

void RenderScene( HWND hRender, RECT &R )
{
	s_RENDERPIPE->begin_scene();
	s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT, NULL );
	s_RENDERPIPE->set_window( hRender, R.left, R.top, R.right, R.bottom );
	s_RENDERPIPE->set_viewport( R.left, R.top, R.right, R.bottom );
	s_RENDERPIPE->set_modelview( Transform() ); 
	s_RENDERPIPE->set_ortho( 0, R.right, R.bottom, 0, -10, 10 );

	RenderDepthTest( 32+1*48, 32+1*48, D3DCMP_ALWAYS,		0x00000000,  1.0 );
	RenderDepthTest( 32+3*48, 32+1*48, D3DCMP_LESS,			0xFFFFFFFF,  1.0 );
	RenderDepthTest( 32+5*48, 32+1*48, D3DCMP_GREATER,		0x00000000, -1.0 );

	RenderDepthTest( 32+1*48, 32+3*48, D3DCMP_NEVER,		0x00000000, 1.0 );
	RenderDepthTest( 32+3*48, 32+3*48, D3DCMP_NOTEQUAL,		0x00000000, 0.0 );
	RenderDepthTest( 32+5*48, 32+3*48, D3DCMP_EQUAL,		0x00000000, 0.0 );

	RenderDepthTest( 32+1*48, 32+5*48, D3DCMP_LESSEQUAL,	0xFFFFFFFF,  1.0 );
	RenderDepthTest( 32+5*48, 32+5*48, D3DCMP_GREATEREQUAL,	0x00000000, -1.0 );

	s_RENDERPIPE->end_scene();
	s_RENDERPIPE->swap_buffers();
}

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
		GetClientRect( hRender, &R );

		s_ThisBitmap = 0;

		if( !s_Windowed ) {
			RenderScene( hRender, R );
			RenderScene( hRender, R );
			_tcscpy( dummy_file, _T("\\dummy.bmp") );
			RunTime->SaveBufferToFile( s_RENDERPIPE, R.right, R.bottom, dummy_file );
			if( (s_ThisBitmap = LoadImage( NULL, dummy_file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE )) == NULL ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("Unable to find thisimage file: %s"), dummy_file );
			}
			s_RENDERPIPE->destroy_buffers();
		}

		SetTimer( hDlg, 0, 10, NULL );

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
		
		case IDC_SAVE_OUTPUT:
			if( s_Windowed ) {
				GetClientRect( hRender, &R );
				RenderScene( hRender, R );
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

	case WM_TIMER:
	case WM_PAINT:
		{
			GetClientRect( hRender, &R );

			BeginPaint( hDlg, &ps );
				if( !s_Windowed ) {
					HDC hRenderDC = GetDC( hRender );
					HDC hBmpDC = CreateCompatibleDC( hRenderDC );
					HANDLE old = SelectObject( hBmpDC, s_ThisBitmap );
					BitBlt( hRenderDC, 0, 0, R.right, R.bottom, hBmpDC, 0, 0, SRCCOPY );
					SelectObject( hBmpDC, old );
					ReleaseDC( hRender, hRenderDC );
					DeleteDC( hBmpDC );
				}
			EndPaint( hDlg, &ps );
			
			if( s_Windowed ) {
				RenderScene( hRender, R );
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
	HRESULT Return = DARTH_TEST_FAIL;
	char device_id[MAX_PATH];
	char *device_id_ptr = NULL;

	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	RunTime->GetTestArgument( 1, IDR_AT_STRING, _T(""),	s_ThisImage, MAX_PATH );
	RunTime->GetTestArgument( 2, IDR_AT_INT,    ((void*)FALSE),	&s_DoSave,	0 );

	if( _tcslen( device_id ) ) {
		device_id_ptr = device_id;
	}

	RunTime->GetIniFilename( s_IniFile, MAX_PATH );

	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, s_IniFile, goto cleanup );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, goto cleanup  );

	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_RENDERPIPE, goto cleanup );

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

	Return = DialogBoxParam( (HINSTANCE)s_Module, MAKEINTRESOURCE(IDD_COMPARE_VIEW), 0, CompareViewDialogProc, (LPARAM)RunTime );

cleanup:

	DeleteObject( s_ThisBitmap );

	if( s_RENDERPIPE ) {
		s_RENDERPIPE->shutdown();
	}

	DARTH_RELEASE( s_RENDERPIPE );
	DARTH_RELEASE( s_SYSTEM );

	if( s_DACOM ) {
		s_DACOM->ShutDown();
	}

	MATH_ENGINE_Uninitialize();

	return Return;
}

//
