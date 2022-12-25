// RenderPipelineTextureAbilitiesCheck.cpp
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

DARTH_DEFINE_NAME(			"RenderPipelineTextureAbilitiesCheck" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Texture.Abilities" )
DARTH_DEFINE_DESCRIPTION(	"Attempts to verify that the texture-related RenderPipeline abilities reported are correct." )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

HRESULT LoadTextureFromFile( IRenderPipeline *RenderPipe, TCHAR *filename, U32 *out_htexture );

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT ReturnValue = DARTH_TEST_FAIL;
	GENRESULT gr;
	char fmt[128+1], device_id[MAX_PATH+1], ini_file[MAX_PATH+1], *device_id_ptr = NULL;
	U32 htexture, w,h,l,lv,yesno, failures=0;
	RPLOCKDATA ld;
	bool mipsSupported = false;

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

	// Non-power of two
	//
	if( FAILED( gr = s_RENDERPIPE->create_texture( 63, 63, PixelFormat(PF_4CC_DAOP), 0, htexture ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 63, 63, ... ) failed (%08X, %08d)"), gr, gr );
		// NOT A FAILURE CASE, we just want to know the answer to this
	}

	// RP_A_TEXTURE_SQUARE_ONLY	,		// device supports only square textures
	//
	if( FAILED( gr = s_RENDERPIPE->query_device_ability( RP_A_TEXTURE_SQUARE_ONLY, &yesno, NULL ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::query_device_ability( RP_A_TEXTURE_SQUARE_ONLY, ... ) failed (%08X, %08d)"), gr, gr );
		yesno = 1;
		failures++;
	}

	gr = s_RENDERPIPE->create_texture( 64, 32, PixelFormat(PF_4CC_DAOP), 0, htexture );

	if( yesno && SUCCEEDED(gr) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 64, 32, ... ) succeeded when qda says TEXTURE_SQUARE_ONLY") );
		failures++;
	}
	else if( !yesno && FAILED(gr) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 64, 32, ... ) failed (%08X,%08d) when qda says not TEXTURE_SQUARE_ONLY") );
		failures++;
	}

	// RP_A_TEXTURE_MAX_WIDTH, _HEIGHT
	//
	if( FAILED( gr = s_RENDERPIPE->query_device_ability( RP_A_TEXTURE_MAX_WIDTH, &w, NULL ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::query_device_ability( RP_A_TEXTURE_MAX_WIDTH, ... ) failed (%08X, %08d)"), gr, gr );
		w = 256; 
		failures++;
	}
	if( FAILED( gr = s_RENDERPIPE->query_device_ability( RP_A_TEXTURE_MAX_HEIGHT, &h, NULL ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::query_device_ability( RP_A_TEXTURE_MAX_HEIGHT, ... ) failed (%08X, %08d)"), gr, gr );
		h = 256;
		failures++;
	}
	if( FAILED( gr = s_RENDERPIPE->create_texture( w, h, PixelFormat(PF_4CC_DAOP), 0, htexture ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( %d, %d, ... ) failed (%08X,%08d) using MAX_WIDTH, MAX_HEIGHT"), w, h, gr, gr );
		failures++;
	}

	// Test destroy texture. We do it here since the texture created above might be very large.
	if (SUCCEEDED(gr))
	{
		if( FAILED( gr = s_RENDERPIPE->destroy_texture( htexture ) ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::destroy_texture(...) failed (%08X, %08d)"), gr, gr );
			failures++;
		}
	}

	// RP_A_TEXTURE_LOD			,		// device supports mipmaps.
	//
	if( FAILED( gr = s_RENDERPIPE->query_device_ability( RP_A_TEXTURE_LOD, &yesno, NULL ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::query_device_ability( RP_A_TEXTURE_LOD, ... ) failed (%08X, %08d)"), gr, gr );
		yesno = 1;
		failures++;
	}
	else
	{
		mipsSupported = true;
	}

	if( SUCCEEDED( gr = s_RENDERPIPE->create_texture( 256, 256, PixelFormat(PF_4CC_DAOP), 9, htexture ) ) ) {
		if( !yesno ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 256, 256, ..., 9, ... ) succeeded when qda says not TEXTURE_LOD") );
			mipsSupported = true;
			failures++;
		}

		if( FAILED( gr = s_RENDERPIPE->get_texture_dim( htexture, &w, &h, &l ) ) ) {
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::get_texture_dims( %08X, ... ) failed (%08X,%08d)"), htexture, gr, gr );
			failures++;
			l = 1;
		}


		for( lv=0; lv<l; lv++ ) {
			
			if( FAILED( gr = s_RENDERPIPE->lock_texture( htexture, lv, &ld ) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::lock_texture( %08X, %08X, ... ) failed (%08X,%08d)"), htexture, lv, gr, gr );
				break;
			}
			else {
				ld.pf.persist( fmt );
				RunTime->Log( IDR_LC_TEST, 0, _T("lock_texture: Texture:%08X Lod:% 3d Width:% 5d Height:% 5d Format: %s"), htexture, lv, ld.width, ld.height, fmt );
			}

			if( FAILED( gr = s_RENDERPIPE->unlock_texture( htexture, lv ) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::unlock_texture( %08X, %08X ) failed (%08X,%08d)"), htexture, lv, gr, gr );
				break;
			}
		}
	}
	else {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 256, 256, ..., 9, ... ) failed when qda says TEXTURE_LOD") );
		mipsSupported = false;
		failures++;
	}

	// Auto-mipmapping test
	// Creation with 0 LOD means it is ok to automip, while creation with 1 indicates exactly one level.
	// This test just attempts to create both a 0 and a 1 with otherwise identical parameters. If one succeeds and one
	// fails, there is a problem.
	if( mipsSupported ) {
		bool autoCreated = false;
		bool singleCreated = false;

		// Attempt auto mip create
		if (SUCCEEDED( gr = s_RENDERPIPE->create_texture( 64, 64, PixelFormat(PF_4CC_DAOP), 0, htexture ) ) ) {
			autoCreated = true;
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PF_4CC_DAOP,0,...) worked."));
			if (FAILED( gr = s_RENDERPIPE->destroy_texture (htexture) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::destroy_texture () failed (%08X, %08d)"), gr, gr );
				// Not a failure for now.
				//failures++;
			}
		}
		else
		{
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PF_4CC_DAOP,0,...) failed (%08X, %08d)"), gr, gr );
		}

		// Attempt single mip create
		if (SUCCEEDED( gr = s_RENDERPIPE->create_texture( 64, 64, PixelFormat(PF_4CC_DAOP), 1, htexture ) ) ) {
			singleCreated = true;
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PF_4CC_DAOP,1,...) worked."));
			if (FAILED( gr = s_RENDERPIPE->destroy_texture (htexture) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::destroy_texture () failed (%08X, %08d)"), gr, gr );
				// Not a failure for now.
				//failures++;
			}
		}
		else
		{
			RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PF_4CC_DAOP,1,...) failed (%08X, %08d)"), gr, gr );
		}

		if (autoCreated != singleCreated)
		{
			RunTime->Log( IDR_LC_TEST, 0, _T("Auto-mip test failed"));
			failures++;
		}
		else
		{
			RunTime->Log( IDR_LC_TEST, 0, _T("Auto-mip test passed"));
		}
	}

	// Texture format Pixel format
	// Test the creation of a texture using the 4CC code. We simply create the textures and log the resulting format of
	// the texture received.
	// NOTE: There is some mapping going on here. What we are testing is the mapping of the 4CC code onto the available
	// texture formats. There are default mapping lists, so we are testing that mapping list as well.
	// NOTE: It is important to have a texture format enumeration test available, so that the formats logged here can
	// be checked against what was available.

	{
		const PFenum FTABLE_END = PF_COLOR_INDEX;
		PFenum formatTable[] =
		{
			PF_4CC_DAOP,
			PF_4CC_DAOT,
//			PF_4CC_DAAA,  // currently unsupported
//			PF_4CC_DAAL,  // currently unsupported
			PF_4CC_DAA1,
			PF_4CC_DAA4,
			PF_4CC_DAA8,
			FTABLE_END
		};

		bool badCode = false;
		int i = 0;
		while (formatTable[i] != FTABLE_END)
		{
			if (SUCCEEDED( gr = s_RENDERPIPE->create_texture( 64, 64, PixelFormat(formatTable[i]), 1, htexture ) ) ) {
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PixelFormat(%4.4s),0,...) worked."), &formatTable[i]);
				if (FAILED( gr = s_RENDERPIPE->destroy_texture (htexture) ) ) {
					RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::destroy_texture () failed (%08X, %08d)"), gr, gr );
					// Not a failure, for now.
					//failures++;
				}
			}
			else
			{
				// This 4CC code is not supported.
				RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture (64,64,PixelFormat(%4.4s),0,...) failed (%08X, %08d)."), &formatTable[i], gr, gr);
				badCode++;
				failures++;
			}
			++i;
		}

		if (!badCode)
		{
			RunTime->Log( IDR_LC_TEST, 0, _T("All supported 4CC texture codes passed."));
		}
	}


	// RGBA Pixel format
	if( FAILED( s_RENDERPIPE->create_texture( 64, 64, PixelFormat(PF_RGBA), 0, htexture ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::create_texture( 64, 64, PixelFormat(PF_RGBA),... ) failed (%08X, %08d)"), gr, gr );
		// NOT A FAILURE CASE, we just want to know the answer to this
	}
	else
	{
		RunTime->Log( IDR_LC_TEST, 0, _T("PixelFormat RGBA creation succeeded."));
	}

	//	RP_A_TEXTURE_SIMULTANEOUS	,		// Number of simultaneous textures supported
	//	RP_A_TEXTURE_COORDINATES	,		// Number of simultaneous texture coordinates supported
	//


	// Cleanup
	//
	if( !failures ) {
		ReturnValue = DARTH_TEST_PASS;
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

HRESULT LoadTextureFromFile( IRenderPipeline *RenderPipe, TCHAR *filename, U32 *out_htexture )
{
	HBITMAP hImage;
	BITMAP  bmp;
	HRESULT hr = E_FAIL;
	U32 ht;

	*out_htexture = 0;

	if( (hImage = (HBITMAP)LoadImage( GetModuleHandle(NULL), 
									  filename, 
									  IMAGE_BITMAP, 
									  LR_DEFAULTSIZE, LR_DEFAULTSIZE, 
									  LR_LOADFROMFILE|LR_CREATEDIBSECTION )) == 0 ) {
		return hr;
	}


	GetObject( hImage, sizeof(bmp), &bmp );

	if( SUCCEEDED( RenderPipe->create_texture( bmp.bmWidth, bmp.bmHeight, PixelFormat(PF_4CC_DAOP), 0, ht ) ) ){
		if( SUCCEEDED( RenderPipe->set_texture_level_data( ht, 0, PixelFormat(0,0,0,0,0), bmp.bmWidth, bmp.bmHeight, bmp.bmWidth*3, PixelFormat(PF_BGR_EXT), bmp.bmBits, NULL, NULL ) ) ) {
			*out_htexture = ht;
			hr = S_OK;
		}
	}

	DeleteObject( hImage );

	return hr;
}	

//

