// RenderPipelineTextureLock.cpp
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
#include "packed_argb.h"

//

static ICOManager		*s_DACOM = NULL;
static ISystemContainer *s_SYSTEM = NULL;
static IRenderPipeline  *s_RENDERPIPE = NULL;
static HANDLE	s_Module = NULL;

//

DARTH_DEFINE_NAME(			"RenderPipelineTextureLock" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Texture.Lock" )
DARTH_DEFINE_DESCRIPTION(	"Attempts to create a mipmapped texture and lock each level with lock_texture() and unlock_texture()." )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

HRESULT LoadTextureFromFile( IRenderPipeline *RenderPipe, TCHAR *filename, U32 *out_htexture );
HRESULT CreateColoredMipMapChain( IRenderPipeline *RenderPipe, U32 *out_htexture );

//

U32 Colors[9] = {
	ARGB_MAKE( 255,  0,  0, 255 ),
	ARGB_MAKE(   0,255,  0, 255 ),
	ARGB_MAKE(   0,  0,255, 255 ),
	ARGB_MAKE( 255,  0,  0, 255 ),
	ARGB_MAKE(   0,255,  0, 255 ),
	ARGB_MAKE(   0,  0,255, 255 ),
	ARGB_MAKE( 255,  0,  0, 255 ),
	ARGB_MAKE(   0,255,  0, 255 ),
	ARGB_MAKE(   0,  0,255, 255 )
};


//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT ReturnValue = DARTH_TEST_FAIL;
	GENRESULT gr;
	char device_id[MAX_PATH+1];
	char *device_id_ptr = NULL;
	char ini_file[128+1];
	U32 htexture;
	U32 w,h,l, lod;
	char fmt[128+1];
	RPLOCKDATA ld;

	RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );

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

	if( FAILED( gr = s_RENDERPIPE->create_buffers( GetDesktopWindow(), 640, 480 ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_buffers( GetDesktopWindow(), 640, 480 ) failed (%08X, %08d)"), gr, gr );
	}

	// Test lock/unlock
	//

	HRESULT hr;
	if( FAILED( hr = CreateColoredMipMapChain( s_RENDERPIPE, &htexture ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("Unable to create colored texture (%08X,%08d)"), hr, hr );
		goto Cleanup;
	}

	if( FAILED( gr = s_RENDERPIPE->get_texture_dim( htexture, &w, &h, &l ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::get_texture_dims( %08X, ... ) failed (%08X,%08d)"), htexture, gr, gr );
		goto Cleanup;
	}

	if( l == 0 ) {
		l = 1;
	}

	for( lod=0; lod<l; lod++ ) {
		
		if( FAILED( gr = s_RENDERPIPE->lock_texture( htexture, lod, &ld ) ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::lock_texture( %08X, %08X, ... ) failed (%08X,%08d)"), htexture, lod, gr, gr );
			break;
		}
		else {
			
			U32 src_pxl_u32 = (*((U32*)ld.pixels));
			U32 r, g, b;

			r = ((src_pxl_u32 & ld.pf.get_r_mask()) >> ld.pf.rl);
			r = (r << ld.pf.rr) | (r & ((1<<ld.pf.rr)-1));
			g = ((src_pxl_u32 & ld.pf.get_g_mask()) >> ld.pf.gl);
			g = (g << ld.pf.gr) | (g & ((1<<ld.pf.gr)-1));
			b = ((src_pxl_u32 & ld.pf.get_b_mask()) >> ld.pf.bl);
			b = (b << ld.pf.br) | (b & ((1<<ld.pf.br)-1));
		
			U32 bgra = ARGB_MAKE( r,g,b,0 );

			if( !(Colors[lod] & bgra) ) {
				// mismatch
				break;
			}
			
			ld.pf.persist( fmt );
			RunTime->Log( IDR_LC_TEST, 0, _T("lock_texture: Texture:%08X Lod:% 3d Width:% 5d Height:% 5d Format: %s Color: %08X"), htexture, lod, ld.width, ld.height, fmt, bgra );
		}

		if( FAILED( gr = s_RENDERPIPE->unlock_texture( htexture, lod ) ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::unlock_texture( %08X, %08X ) failed (%08X,%08d)"), htexture, lod, gr, gr );
			break;
		}
	}

	// Cleanup
	//
	if( lod == l ) {
		ReturnValue = DARTH_TEST_PASS;
	}
	else {
		ReturnValue = DARTH_TEST_FAIL;
	}

Cleanup:
	if( s_RENDERPIPE ) {
		if( FAILED( gr = s_RENDERPIPE->shutdown() ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::shutdown() failed (%08X, %08d)\n"), gr, gr );
		}
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

HRESULT CreateColoredMipMapChain( IRenderPipeline *RenderPipe, U32 *out_htexture )
{
	U32 ht;

	if( SUCCEEDED( RenderPipe->create_texture( 256, 256, PixelFormat(PF_4CC_DAOT), 9, ht ) ) ){
		for( U32 l=0; l<9; l++ ) {
			if( SUCCEEDED( RenderPipe->set_texture_level_data( ht, l, PixelFormat(0,0,0,0,0), 1, 1, sizeof(U32), PixelFormat(PF_BGRA_EXT), &Colors[l], NULL, NULL ) ) ) {
			}
		}

		*out_htexture = ht;
		return S_OK;
	}

	*out_htexture = 0;
	return E_FAIL;
}

//
