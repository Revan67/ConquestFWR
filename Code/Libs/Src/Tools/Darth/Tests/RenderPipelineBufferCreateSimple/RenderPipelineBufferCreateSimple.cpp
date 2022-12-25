// RenderPipelineBufferCreateSimple.cpp
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

DARTH_DEFINE_NAME(			"RenderPipelineBufferCreateSimple" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Buffer.Create" )
DARTH_DEFINE_DESCRIPTION(	"Attempts to create a fullscreen double buffered device in every mode available." )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT ReturnValue = DARTH_TEST_FAIL;
	HWND hWnd;
	U32 NumModes, M, ClearColor = 0xFFCCCCCC;
	RPDISPLAYMODEINFO Mode;
	HDC hDC;
	GENRESULT gr;
	char device_id[128+1];
	char *device_id_ptr = NULL;
	char ini_file[128+1];
	TCHAR ModeString[64+1];

	U32 width, height, bpp, buffers, depth, fullscreen;
	U32 post_width, post_height, post_bpp, post_depth, post_buffers, post_fullscreen;

	int delay;
	const NUM_DEPTH_MODES = 3;
	unsigned long DepthBpp[NUM_DEPTH_MODES] = { 16, 24, 32 };


	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	RunTime->GetTestArgument( 1, IDR_AT_INT,    ((void*)0xFFFFFFFF), &width, 0 );
	RunTime->GetTestArgument( 2, IDR_AT_INT,    ((void*)0xFFFFFFFF), &height, 0 );
	RunTime->GetTestArgument( 3, IDR_AT_INT,    ((void*)0xFFFFFFFF), &bpp, 0 );
	RunTime->GetTestArgument( 4, IDR_AT_INT,    ((void*)0xFFFFFFFF), &depth, 0 );
	RunTime->GetTestArgument( 5, IDR_AT_INT,    ((void*)TRUE), &fullscreen, 0 );
	RunTime->GetTestArgument( 6, IDR_AT_INT,    ((void*)2), &buffers, 0 );
	RunTime->GetTestArgument( 7, IDR_AT_INT,    ((void*)3000), &delay, 0 );

	if( _tcslen( device_id ) ) {
		device_id_ptr = device_id;
	}

	RunTime->GetIniFilename( ini_file, MAX_PATH );

	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, ini_file, goto Cleanup );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, goto Cleanup );
	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_RENDERPIPE, goto Cleanup );

	if( FAILED( gr = s_RENDERPIPE->startup( device_id_ptr ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::startup() failed (%08X, %08d)\n"), gr, gr );
		goto Cleanup;
	}

	if( FAILED( gr = s_RENDERPIPE->get_num_display_modes( &NumModes ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::get_num_display_modes() failed (%08X, %08d)\n"), gr, gr );
		goto Cleanup;
	}

	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_DEPTH_BPP, depth );
	s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_COUNT, buffers );

	for( M=0; M<NumModes; M++ ) {

		if( FAILED( gr = s_RENDERPIPE->get_display_mode( &Mode, M ) ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::get_display_mode( &Mode, %d ) failed (%08X, %08d)\n"), M, gr, gr );
			goto Cleanup;
		}
	
		for( int dbpp = 0; dbpp<NUM_DEPTH_MODES; dbpp++ ) {

			U32 depthbpp = DepthBpp[dbpp];

			if( ((width == 0xFFFFFFFF) || (width == Mode.width)) &&
				((height == 0xFFFFFFFF) || (height == Mode.height)) &&
				((bpp == 0xFFFFFFFF) || (bpp == Mode.render_pf.num_bits())) &&
				((depth == 0xFFFFFFFF) || (depthbpp == depth)) ) {

				if( !fullscreen ) {
					// Windowed
					//
					s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_FULLSCREEN, FALSE );
					if( (hWnd = RunTime->CreateTestWindow( -1, -1, Mode.width, Mode.height, "CreateBuffer" )) == NULL ) {
						RunTime->Log( IDR_LC_TEST, 0, _T("CreateTestWindow failed\n") );
						goto Cleanup;
					}
				}
				else {
					// Fullscreen
					//
					s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_FULLSCREEN, TRUE );
					hWnd = GetDesktopWindow();
				}

				s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_COUNT, buffers );
				s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_COLOR_BPP, Mode.render_pf.num_bits() );
				s_RENDERPIPE->set_pipeline_state( RP_BUFFERS_DEPTH_BPP, depthbpp );

				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers( %08X, %08X, %08X ) %d %d % 2d % 2d"), hWnd, Mode.width, Mode.height, buffers, fullscreen, Mode.render_pf.num_bits(), depthbpp );

				if( FAILED( gr = s_RENDERPIPE->create_buffers( hWnd, Mode.width, Mode.height ) ) ) {
					RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers failed (%08X, %08d)"), gr, gr );
				}
				else {
					s_RENDERPIPE->set_pipeline_state( RP_CLEAR_COLOR, ClearColor );

					s_RENDERPIPE->clear_buffers( RP_CLEAR_COLOR_BIT, NULL );
					
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_WIDTH, &post_width );
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_HEIGHT, &post_height );
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_COLOR_BPP, &post_bpp );
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_DEPTH_BPP, &post_depth );
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_COUNT, &post_buffers );
					s_RENDERPIPE->get_pipeline_state( RP_BUFFERS_FULLSCREEN, &post_fullscreen );

					_stprintf( ModeString, _T("Mode %dx%d %d bpp (%d depth, %d buffers, %d fullscreen) %s"), post_width, post_height, post_bpp, post_depth, post_buffers, post_fullscreen, (delay<0)? "[Spacebar] to continue" : "" );
					
					s_RENDERPIPE->get_dc( &hDC );
					TextOut( hDC, 10, 10, ModeString, _tcslen(ModeString) );
					s_RENDERPIPE->release_dc( hDC );

					s_RENDERPIPE->swap_buffers();

					RunTime->Log( IDR_LC_TEST, 0, ModeString );
					
	#define CHECK_POST_VALUE( pre_val, post_val, rp_state ) if( (pre_val) != (post_val) ) {	RunTime->Log( IDR_LC_TEST, 0, _T("Value of %s after create_buffers() does not match value before create_buffers() (%d != %d)"), # rp_state, post_val, pre_val );	}

					
					CHECK_POST_VALUE( Mode.width,					post_width,		RP_BUFFERS_WIDTH ) ;
					CHECK_POST_VALUE( Mode.height,					post_height,	RP_BUFFERS_HEIGHT ) ;
					CHECK_POST_VALUE( buffers,						post_buffers,	RP_BUFFERS_COUNT ) ;
					CHECK_POST_VALUE( Mode.render_pf.num_bits(),	post_bpp,		RP_BUFFERS_COLOR_BPP ) ;
					CHECK_POST_VALUE( depthbpp,						post_depth,		RP_BUFFERS_DEPTH_BPP) ;
					CHECK_POST_VALUE( fullscreen,					post_fullscreen,RP_BUFFERS_FULLSCREEN ) ;

					if( delay < 0 ) {
						while( RunTime->QueryUserInput() != IDR_FC_NEXT );
					}
					else {
						Sleep( delay );
					}

					s_RENDERPIPE->destroy_buffers();
				}

				if( !fullscreen ) {
					RunTime->DestroyTestWindow( hWnd );
				}
			}
		}
	}

	ReturnValue = DARTH_TEST_PASS;

Cleanup:
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
