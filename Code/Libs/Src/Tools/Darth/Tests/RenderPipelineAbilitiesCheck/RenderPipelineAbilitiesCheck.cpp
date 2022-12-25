// s_PIPElineAbilitiesCheck.cpp
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
static IRenderPipeline	*s_PIPE = NULL;
static HANDLE			s_TestModuleHandle = 0;

static char *s_Name			= "RenderPipelineAbilitiesCheck";
static char *s_Description	= "Exercises RenderPipeline::query_device_ability";
static char *s_Category		= "RenderPipeline";
static char *s_Contact		= "pbleisch";

//

DARTH_DEFINE_NAME(			s_Name )
DARTH_DEFINE_CONTACT(		s_Contact )
DARTH_DEFINE_CATEGORY(		s_Category )
DARTH_DEFINE_DESCRIPTION(	s_Description )
DARTH_DEFINE_DLLMAIN(		s_TestModuleHandle )

//

TCHAR *abilities_text[RP_A_MAX_ABILITY] = {
	_T( "A_NON_EXISTENT_ONE" ),		
	_T( "A_DEVICE_2D_ONLY" ),			
	_T( "A_DEVICE_3D_ONLY" ),			
	_T( "A_DEVICE_PRIMARY" ),			
	_T( "A_DEVICE_GAMMA" ),			
	_T( "A_DEVICE_WINDOWED" ),		
	_T( "A_DEVICE_MEMORY" ),			
	_T( "A_BUFFERS_NUM_MODES" ),		
	_T( "A_TEXTURE_NONLOCAL" ),		
	_T( "A_TEXTURE_SQUARE_ONLY" ),	
	_T( "A_TEXTURE_STAGES" ),			
	_T( "A_TEXTURE_MAX_WIDTH" ),		
	_T( "A_TEXTURE_MAX_HEIGHT" ),		
	_T( "A_TEXTURE_NUM_FORMAT" ),		
	_T( "A_TEXTURE_NUMA" ),			
	_T( "A_TEXTURE_BILINEAR" ),		
	_T( "A_TEXTURE_TRILINEAR" ),		
	_T( "A_ALPHA_ITERATED" ),			
	_T( "A_FOG_TABLE" ),				
	_T( "A_DEVICE_SOFTWARE" ),		
	_T( "A_ALPHA_TEST" ),				
	_T( "A_ALPHA_TEST_ALL" ),			
	_T( "A_TEXTURE_LOD" ),			
	_T( "A_BUFFERS_DEPTH_LINEAR" ),	
	_T( "A_TEXTURE_SIMULTANEOUS" ),	
	_T( "A_TEXTURE_COORDINATES" ),	
	_T( "A_BLEND_MUL_SRC" ),			
	_T( "A_BLEND_MUL_DST" ),			
	_T( "A_BLEND_ADD_SRC" ),			
	_T( "A_BLEND_ADD_DST" ),			
	_T( "A_BLEND_TRANSPARENCY_SRC" ),	
	_T( "A_BLEND_TRANSPARENCY_DST" ),	
	_T( "A_BLEND_MATRIX" )
};

//

HRESULT do_loop( IDarthRuntime *RunTime, TCHAR *Label, U32 *Value1, U32 *Value2 ) 
{
	TCHAR *V1 = Value1 ? "Value1" : "(NULL)";
	TCHAR *V2 = Value2 ? "Value2" : "(NULL)";
	TCHAR *Ret = NULL;

	RunTime->Log( IDR_LC_TEST, 0, Label );
	for( U32 Ability=1; Ability<RP_A_MAX_ABILITY; Ability++ ) {
		if( FAILED( s_PIPE->query_device_ability( (RPDEVICEABILITY)Ability, Value1, Value2 ) ) ) {
			Ret = "failed";
		}
		else {
			Ret = "passed";
		}

		RunTime->Log( IDR_LC_TEST, 0, _T("query_device_ability( %s, (%03d), %s, %s ) %s: V1=(%08X, %08d) V2=(%08X,%08d)"), 
										abilities_text[Ability], Ability, 
										V1, V2,
										Ret,
										Value1? Value1[0] : 0xFFFFFFFE, 
										Value1? Value1[0] : 0xFFFFFFFE, 
										Value2? Value2[0] : 0xFFFFFFFE, 
										Value2? Value2[0] : 0xFFFFFFFE );
	}

	return S_OK;
}

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	HRESULT Return = DARTH_TEST_FAIL;
	char device_id[MAX_PATH];
	char *device_id_ptr = NULL;
	char ini_file[MAX_PATH];

	if( RunTime->GetTestArgumentCount() != 0 ) {
		device_id_ptr = device_id;
		RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	}


	RunTime->GetIniFilename( ini_file, MAX_PATH );

	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, ini_file, goto Cleanup );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, goto Cleanup );
	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_PIPE, goto Cleanup );

	GENRESULT gr;
	U32 Value1[4], Value2[4];

	if( FAILED( gr = s_PIPE->startup( device_id_ptr ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("IRenderPipeline::startup() failed (%08X, %08d)"), gr, gr );
		goto Cleanup;
	}

	// First test query_device_ability call before calling create_buffers
	// Everything should be valid.
	//
	do_loop( RunTime, _T("Checking abilities before create_buffers..."), Value1, Value2 );

	if( FAILED( s_PIPE->create_buffers( GetDesktopWindow(), 640, 480 ) ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("create_buffers( GetDesktopWindow(), 640, 480 ) failed") );
		goto Cleanup;
	}

	// Next test query_device_ability call after calling create_buffers
	// Everything should be valid and the same as previous values.
	//
	do_loop( RunTime, _T("Checking abilities after create_buffers..."), Value1, Value2 );

	// Next test query_device_ability call with one NULL parameter
	//
	do_loop( RunTime, _T("Checking abilities with Value1, NULL..."), Value1, NULL );

	// Next test query_device_ability call with difference NULL parameter
	//
	do_loop( RunTime, _T("Checking abilities with NULL, Value2..."), NULL, Value2 );

	// Next test query_device_ability call with two NULL parameters
	//
	do_loop( RunTime, _T("Checking abilities with NULL, NULL..."), NULL, NULL );

	Return = DARTH_TEST_PASS;

Cleanup:
	// Cleanup
	//

	if( s_PIPE ) {
		s_PIPE->shutdown();
	}

	DARTH_RELEASE( s_PIPE );
	DARTH_RELEASE( s_SYSTEM );

	if( s_DACOM ) {
		s_DACOM->ShutDown();
		DARTH_RELEASE( s_DACOM );
	}

	MATH_ENGINE_Uninitialize();

	return Return;
}

//
