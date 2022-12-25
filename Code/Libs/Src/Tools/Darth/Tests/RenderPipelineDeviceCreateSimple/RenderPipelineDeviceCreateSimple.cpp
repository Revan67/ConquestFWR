// RenderPipelineDeviceCreateSimple.cpp
//
// Attempts to create a renderpipe device and call startup/shutdown.
//


#include <windows.h>
#include "DarthTest.h"		// Include this for IDarthRuntime

#include "DACOM.h"
#include "System.h"
#include "RendPipeline.h"

//

static HANDLE s_Module = 0;	// Handy for loading resources from this DLL.

//
// A Darth Test is a DLL that contains the following functions exported
// as "C" functions:
//
//	HRESULT GetTestName( LPTSTR out_Name, int MaxLen );
//	HRESULT GetTestContact( LPTSTR out_Contact, int MaxLen );
//	HRESULT GetTestDescription( LPTSTR out_Description, int MaxLen );
//	HRESULT GetTestCategory( LPTSTR out_Cat, int MaxLen );
//	HRESULT RunTest( IDarthRuntime *RunTime );
//
// The first four functions define some metadata about the test that
// the runtime uses to display information about the test. Use the 
// DARTH_DEFINE_* macros (defined in DarthTest.h) to easily define 
// GetTestName(), GetTestContact(), GetTestCategory(), and 
// GetTestDescription().
//
// RunTest() is the actual test.  The runtime exports a function to
// the scripting environment that has the symbolname set to the 
// same as the name returned from GetTestName().  This function can
// accept parameters like any normal function.  For example, this
// test is exported as the funtion:
//
//   RenderPipelineDeviceCreateSimple( ... )
//
// Note that in Lua, all functions are intrinsically varargs functions.
// Because of this, RunTest() should get the number of arguments and
// use GetTestArgument() to to type coercion on the parameters.
//
// Note: Try not to use \n's or \r's anywhere in text constants except 
// when necessary as the strings are displayed in variable size text boxes.
//
DARTH_DEFINE_NAME(			"RenderPipelineDeviceCreateSimple" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Device.Create" )
DARTH_DEFINE_DESCRIPTION(	"This tests a single RenderPipeline startup and shutdown. " 
							"There should be no visible result of this test except "
							"on some video cards with 3dfx chipsets such as Voodoo1, "
							"Voodoo2, and Voodoo Rush." );

// DARTH_DEFINE_DLLMAIN is useful if you don't need anything but
// the most basic DLLMain() call.  Note that DLLMain() must exist
// for this dll to load correctly, so if you don't use the
// DARTH_DEFINE_DLLMAIN macro, you must define DLLMain() somewhere.
//
DARTH_DEFINE_DLLMAIN(		s_Module )


// RunTest()
//
// This is the actual test that is executed when the lua code
// has some code like:
//
//     RenderPipelineDeviceCreateSimple();	
//
// Note that the name of the Lua function is the same as the
// Name returned by GetTestName and NOT the name of the DLL.
//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	ICOManager		 *s_DACOM	= NULL;
	ISystemContainer *s_SYSTEM	= NULL;
	IRenderPipeline  *s_PIPE	= NULL;
	TCHAR ini_file[MAX_PATH];
	char device_id[MAX_PATH];
	char *device_id_ptr = NULL;

	// GetTestArgumentCount() returns the number of arguments
	// in the function call that invoked RunTest().
	//
	if( RunTime->GetTestArgumentCount() != 0 ) {


		device_id_ptr = device_id;
		RunTime->GetTestArgument( 0, IDR_AT_STRING, _T(""), device_id, MAX_PATH );
	}

	// Get the name of the DACOM.ini file to use.  Unless you really know
	// what you're doing, you should always use this method to get the ini
	// file name.  The filename defaults to GetTestName() + ".ini", but the
	// script can use this feature to allow multiple configuration testing.
	//
	RunTime->GetIniFilename( ini_file, MAX_PATH );

	// Use the DARTH_REQUIRE_* macros to create DACOM, System, and Engine 
	// components, and to get DACOM necessary interfaces from objects.
	//
	// Note that the last argument to the DARTH_REQUIRE_* macros is the
	// code to execute when something fails in the macro.
	// 
	// Note that you should use DARTH_REQUIRE_INTERFACE instead of pure
	// QueryInterface calls as DARTH_REQUIRE_INTERFACE will log to the
	// runtime on failures.
	//
	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, ini_file, return DARTH_TEST_FAIL );
	DARTH_REQUIRE_SYSTEM( RunTime, s_DACOM, s_SYSTEM, return DARTH_TEST_FAIL );
	DARTH_REQUIRE_INTERFACE( RunTime, s_SYSTEM, IID_IRenderPipeline, s_PIPE, return DARTH_TEST_FAIL );

	// This is the start of this specific test
	//
	if( FAILED( s_PIPE->startup( device_id_ptr ) ) ) {
		 RunTime->Log( IDR_LC_TEST, 0, "s_PIPE->startup() FAILED" );
		return E_FAIL;
	}

	// If you use RenderPipe, you must explicitly call shutdown() as
	// there are some bugs with just releasing the interface pointers.
	//
	if( FAILED( s_PIPE->shutdown() ) ) {
		RunTime->Log( IDR_LC_TEST, 0, "s_PIPE->shutdown() failed" );
		return E_FAIL;
	}

	// Be sure to ALWAYS ALWAYS ALWAYS release interfaces pointers, 
	// call DACOM::ShutDown(), and MATH_ENGINE_Uninitialize() or else 
	// the test harness will get confused.
	//
	DARTH_RELEASE( s_PIPE );
	DARTH_RELEASE( s_SYSTEM );
	
	s_DACOM->ShutDown();

	MATH_ENGINE_Uninitialize();

	return DARTH_TEST_PASS;
}

//
