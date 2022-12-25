// TestFailureTest.cpp
//
// 
//


#include <windows.h>
#include "DarthTest.h"
#include "DACOM.h"

//

static HANDLE s_Module = 0;

static char *s_Name			= "TestFailureTest";
static char *s_Description	= "This will verify that the Darth Runtime correctly responds\n"
							  "to tests that fail to complete successfully.              \n"
							  "\n"
							  "During normal usage, the runtime will log the test failure\n"
							  "and optionally prompt the user (you) to retry the test.  In\n"
							  "the case of this test, the user can simply ignore the failure.\n";
static char *s_Category		= "Runtime";
static char *s_Contact		= "pbleisch";

//

DARTH_DEFINE_NAME(			s_Name )
DARTH_DEFINE_CONTACT(		s_Contact )
DARTH_DEFINE_CATEGORY(		s_Category )
DARTH_DEFINE_DESCRIPTION(	s_Description )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	return DARTH_TEST_FAIL;
}

//
