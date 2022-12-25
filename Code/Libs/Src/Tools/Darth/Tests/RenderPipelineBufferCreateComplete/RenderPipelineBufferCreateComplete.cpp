// RenderPipelineBufferCreateComplete.cpp
//
// 
//


#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "DarthTest.h"

#include "DACOM.h"
#include "System.h"
#include "RendPipeline.h"


//

static ICOManager		*s_DACOM = NULL;
static ISystemContainer *s_SYSTEM = NULL;
static IRenderPipeline  *s_RENDERPIPE = NULL;
static HANDLE	s_Module = NULL;

//

DARTH_DEFINE_NAME(			"RenderPipelineBufferCreateComplete" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Buffer.Create" )
DARTH_DEFINE_DESCRIPTION(	"Attempts (at different bitdepths if necessary) to create render buffers." )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

HRESULT CreateRenderWindow( IDarthRuntime *RunTime, HWND hWnd, unsigned long width, unsigned long height, unsigned long bpp, unsigned long depth, unsigned long fullscreen )
{
	GENRESULT gr;
	HDC hDC;
	U32 post_width, post_height, post_bpp, post_depth, post_buffers, post_fullscreen;
	TCHAR ModeString[128+1];

	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_COLOR_BPP, bpp );
	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_DEPTH_BPP, depth );
	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_COUNT, 2 );
	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_FULLSCREEN, fullscreen );

	RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers( %08X, %08X, %08X ) %d %d % 2d % 2d"), hWnd, width, height, 2, fullscreen, bpp, depth );

	if( FAILED( gr = s_RENDERPIPE->create_buffers( hWnd, width, height ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers failed (%08X, %08d)"), gr, gr );
		return E_FAIL;
	}
	else {
	
		s_RENDERPIPE->set_pipeline_state( RP_CLEAR_COLOR, 0xFFC0C0C0 );
		s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT, NULL );

		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_WIDTH, &post_width );
		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_HEIGHT, &post_height );
		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_COLOR_BPP, &post_bpp );
		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_DEPTH_BPP, &post_depth );
		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_COUNT, &post_buffers );
		s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_FULLSCREEN, &post_fullscreen );

		_stprintf( ModeString, _T("Mode %dx%d %d bpp (%d depth, %d buffers, %d fullscreen)"), post_width, post_height, post_bpp, post_depth, post_buffers, post_fullscreen );

		
		s_RENDERPIPE->get_dc( &hDC );
		TextOut( hDC, 10, 10, ModeString, _tcslen(ModeString) );
		TextOut( hDC, 10, 30, _T("Hello World"), _tcslen(_T("Hello World")) );
		s_RENDERPIPE->release_dc( hDC );

		s_RENDERPIPE->swap_buffers();

		RunTime->Log( IDR_LC_TEST, 0, ModeString );
		
#define CHECK_POST_VALUE( pre_val, post_val, rp_state ) if( (pre_val) != (post_val) ) {	RunTime->Log( IDR_LC_TEST, 0, _T("Value of %s after create_buffers() does not match value before create_buffers() (%d != %d)"), # rp_state, post_val, pre_val );	}

		
		CHECK_POST_VALUE( width,		post_width,		RP_BUFFERS_WIDTH ) ;
		CHECK_POST_VALUE( height,		post_height,	RP_BUFFERS_HEIGHT ) ;
		CHECK_POST_VALUE( 2,			post_buffers,	RP_BUFFERS_COUNT ) ;
		CHECK_POST_VALUE( bpp,			post_bpp,		RP_BUFFERS_COLOR_BPP ) ;
		CHECK_POST_VALUE( depth,		post_depth,		RP_BUFFERS_DEPTH_BPP) ;
		CHECK_POST_VALUE( fullscreen,	post_fullscreen,RP_BUFFERS_FULLSCREEN ) ;
	}

	return S_OK;
}

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT ReturnValue = DARTH_TEST_FAIL;
	HWND hWnd = 0;
	GENRESULT gr;
	char device_id[128+1];
	char *device_id_ptr = NULL;
	char ini_file[128+1];
	unsigned char state[256];
	int width, height;
	int done = 0;


	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	RunTime->GetTestArgument( 1, IDR_AT_INT,    ((void*)0), &width, 0 );
	RunTime->GetTestArgument( 2, IDR_AT_INT,    ((void*)0), &height, 0 );

	if( _tcslen( device_id ) ) {
		device_id_ptr = device_id;
	}

	RunTime->GetIniFilename( ini_file, MAX_PATH );

	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, ini_file, goto Cleanup );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, goto Cleanup );
	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_RENDERPIPE, goto Cleanup );

	if( FAILED( gr = s_RENDERPIPE->startup( device_id_ptr ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::startup() failed (%08X, %08d)"), gr, gr );
		goto Cleanup;
	}

	RunTime->Log( IDR_LC_TEST, 0, _T("Begin complete test") );

	if( (hWnd = RunTime->CreateTestWindow( -1, -1, width, height, "RenderPipelineBufferCreateComplete" )) == NULL ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("CreateTestWindow failed") );
		goto Cleanup;
	}

	RunTime->Log( IDR_LC_TEST, 0, _T("Trying 32/32/0...") );
	if( FAILED( CreateRenderWindow( RunTime, hWnd, width, height, 32, 32, 0 ) ) ) {

		RunTime->Log( IDR_LC_TEST, 0, _T("Trying 32/16/0...") );
		if( FAILED( CreateRenderWindow( RunTime, hWnd, width, height, 32, 16, 0 ) ) ) {
		
			RunTime->Log( IDR_LC_TEST, 0, _T("Trying 16/16/0...") );
			if( FAILED( CreateRenderWindow( RunTime, hWnd, width, height, 16, 16, 0 ) ) ) {

				RunTime->Log( IDR_LC_TEST, 0, _T("Trying 16/16/1...") );
				if( FAILED( CreateRenderWindow( RunTime, hWnd, width, height, 16, 16, 1 ) ) ) {
					goto Cleanup;
				}
			}
		}
	}

	ReturnValue = DARTH_TEST_PASS;

	s_RENDERPIPE->destroy_buffers();

Cleanup:
	RunTime->DestroyTestWindow( hWnd );
	RunTime->Log( IDR_LC_TEST, 0, _T("End complete test") );

	if( FAILED( gr = s_RENDERPIPE->shutdown() ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::shutdown() failed (%08X, %08d)\n"), gr, gr );
	}

	DARTH_RELEASE( s_RENDERPIPE );
	DARTH_RELEASE( s_SYSTEM );

	if( s_DACOM ) {
		s_DACOM->ShutDown();
		s_DACOM = NULL;
	}

	MATH_ENGINE_Uninitialize();

	return ReturnValue;
}

//
