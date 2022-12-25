// RenderPipelineTextureFormatsCheck.cpp
//
// 
//


#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include <vector>

#include "DarthTest.h"

#include "DACOM.h"
#include "System.h"
#include "RendPipeline.h"

//

static ICOManager		*s_DACOM = NULL;
static ISystemContainer *s_SYSTEM = NULL;
static IRenderPipeline  *s_RENDERPIPE = NULL;
static HANDLE			 s_Module = NULL;

//

DARTH_DEFINE_NAME(			"RenderPipelineTextureFormatsCheck" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Texture.Formats" )
DARTH_DEFINE_DESCRIPTION(	"Attempts to verify that texture formats are available at the right times." )
DARTH_DEFINE_DLLMAIN(		s_Module )

typedef vector< PixelFormat > TextureFormatSet;

//

void GetTextureFormats( IDarthRuntime *RunTime, char *label, TextureFormatSet &tfs )
{
	GENRESULT gr;
	U32 num, tf;
	PixelFormat pf;

	if( FAILED( gr = s_RENDERPIPE->get_num_texture_formats( &num ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::get_num_texture_formats() failed (%08X, %08d)"), gr, gr );
	}
	else {
		RunTime->Log( IDR_LC_TEST, 0, _T("%s: %d texture formats"), label, num );
		for( tf=0; tf<num; tf++ ) {
			if( FAILED( gr = s_RENDERPIPE->get_texture_format( &pf, tf ) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("%s: IRenderPipeline::get_texture_format( %d, ... ) failed (%08X, %08d)"), label, tf, gr, gr );
			}
			else {
				RunTime->Log( IDR_LC_TEST, 0, _T("%s: IRenderPipeline::get_texture_format( %d, ... ) returned %d:%d:%d:%d:%d"), label, tf, pf.num_bits(), pf.num_r_bits(), pf.num_g_bits(), pf.num_b_bits(), pf.num_a_bits() );
				tfs.push_back( pf );
			}
		}
	}
}

//

bool pf_equal( const PixelFormat &a, const PixelFormat &b )
{
	if( memcmp( &a, &b, sizeof(PixelFormat) ) == 0 ) {
		return true;
	}
	return false;
}

//

void CompareAndLog( IDarthRuntime *Runtime, char *a_label, char *b_label, TextureFormatSet &a, TextureFormatSet &b )
{
	if( (a.size() != b.size()) || !equal( a.begin(), a.end(), b.begin(), pf_equal ) ) {
		Runtime->Log( IDR_LC_TEST, 0, _T("Texture formats in %s DO NOT match formats in %s"), a_label, b_label );
	}
	else {
		Runtime->Log( IDR_LC_TEST, 0, _T("Texture formats in %s match formats in %s"), a_label, b_label );
	}
}

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT ReturnValue = DARTH_TEST_FAIL;
	GENRESULT gr;
	char device_id[MAX_PATH+1];
	char *device_id_ptr = NULL;
	char ini_file[128+1];
	int TestPostShutdown;

	TextureFormatSet post_startup;
	TextureFormatSet post_create_buffers;
	TextureFormatSet post_destroy_buffers;
	TextureFormatSet post_shutdown;

	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	RunTime->GetTestArgument( 1, IDR_AT_INT, 0, &TestPostShutdown, 0 );

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

	post_startup.clear();
	post_create_buffers.clear();
	post_destroy_buffers.clear();
	post_shutdown.clear();

	// check texture formats before create buffers; this is not really legal
	GetTextureFormats( RunTime, "post_startup", post_startup );

	// create buffers
	if( FAILED( gr = s_RENDERPIPE->create_buffers( GetDesktopWindow(), 640, 480 ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers( GetDesktopWindow(), 640, 480 ) failed (%08X, %08d)"), gr, gr );
	}

	// check texture formats after create buffers;
	GetTextureFormats( RunTime, "post_create_buffers", post_create_buffers );

	// destroy buffers
	if( FAILED( gr = s_RENDERPIPE->destroy_buffers( ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::destroy_buffers( ) failed (%08X, %08d)"), gr, gr );
	}

	// check texture formats after destroy buffers;  this is not really legal
	GetTextureFormats( RunTime, "post_destroy_buffers", post_destroy_buffers );

	if( FAILED( gr = s_RENDERPIPE->shutdown() ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::shutdown() failed (%08X, %08d)\n"), gr, gr );
	}

	// compare them all
	//
	CompareAndLog( RunTime, "post_startup",			"post_create_buffers",	post_startup,			post_create_buffers );
	CompareAndLog( RunTime, "post_create_buffers",	"post_destroy_buffers", post_create_buffers,	post_destroy_buffers );

	if( TestPostShutdown ) {
		// check texture formats after shutdown; this is not really legal
		GetTextureFormats( RunTime, "post_shutdown", post_shutdown );
		CompareAndLog( RunTime, "post_destroy_buffers",	"post_shutdown",		post_destroy_buffers,	post_shutdown );
	}

	// Cleanup
	//
	ReturnValue = DARTH_TEST_PASS;

Cleanup:
	
	if( s_RENDERPIPE ) {
		s_RENDERPIPE->shutdown();
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