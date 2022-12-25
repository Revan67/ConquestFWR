// DarthTest.h
//
//
//

#ifndef DARTHTEST_H
#define DARTHTEST_H

//

#include "rendpipeline.h"

#define IDR_LC_TEST		(1<<0)
#define IDR_LC_LIBRARY	(1<<1)
#define IDR_LC_RUNTIME  (1<<2)
#define IDR_LC_SCRIPT	(1<<3)

//

enum IDR_FLOWCONTROL
{
	IDR_FC_CONTINUE,
	IDR_FC_PAUSE,
	IDR_FC_EXIT,
	IDR_FC_NEXT,
	IDR_FC_PREV,

	IDR_FC_MAX
};

//

enum IDR_STANDARDWINDOW 
{
	IDR_SW_SIMPLE,
	IDR_SW_RENDER,
	IDR_SW_RENDER_COMPARE,
	IDR_SW_FROM_RESOURCE
};
	
//

enum IDR_ARGUMENTTYPE
{
	IDR_AT_STRING,
	IDR_AT_FLOAT,
	IDR_AT_INT,
	IDR_AT_VECTOR
};

//

struct IDarthRuntime
{
	virtual void Log( unsigned int Class, int ReservedZero, TCHAR *Fmt, ... ) = 0;
	
	virtual HWND CreateTestWindow( int x, int y, int w, int h, TCHAR *Text ) = 0;
	virtual HRESULT DestroyTestWindow( HWND hWnd ) = 0;
	
	virtual IDR_FLOWCONTROL QueryUserInput( void ) = 0;
	
	virtual int GetTestArgumentCount( void ) = 0;
	virtual HRESULT GetTestArgument( int ArgNum, IDR_ARGUMENTTYPE Type, void *default_value, void *outBuffer, int MaxBufferLen ) = 0;
	
	virtual HRESULT GetIniFilename( TCHAR *Filename, int MaxBufferLen ) = 0;

	virtual HRESULT SaveBufferToFile( IRenderPipeline *RenderPipe, int window_w, int window_h, TCHAR *Filename ) = 0;

	virtual HRESULT PushReturnValue( IDR_ARGUMENTTYPE Type, void *Value ) = 0;
};	

//


// The following functions must be defined by the imported dll that
// implements a Darth test.  All of the GetTest*() functions can be
// easily implemented with the DARTH_DECLARE_* macros below.
// This basically means that one can implement a Darth test with
// the three functions: InitializeTest, RunTest, CleanupTest.
//
extern "C" {
__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime );
__declspec( dllexport ) HRESULT GetTestName( LPTSTR out_Name, int MaxLen );
__declspec( dllexport ) HRESULT GetTestContact( LPTSTR out_Contact, int MaxLen );
__declspec( dllexport ) HRESULT GetTestDescription( LPTSTR out_Description, int MaxLen );
__declspec( dllexport ) HRESULT GetTestCategory( LPTSTR out_Cat, int MaxLen );
};

//

#define DARTH_TEST_PASS		MAKE_HRESULT(SEVERITY_SUCCESS,	0,	0)
#define DARTH_TEST_FAIL		MAKE_HRESULT(SEVERITY_ERROR,	0,	1)

//


#include <tchar.h>

//

inline _ncpy( LPTSTR Dst, int MaxLen, LPCTSTR Src )
{
	int ls = _tcslen( Src );
	int lc = min( ls, MaxLen-1 );
	_tcsncpy( Dst, Src, lc );
	Dst[lc] = 0;
}

//

#define DARTH_DEFINE_NAME( xx )						\
	__declspec( dllexport ) HRESULT GetTestName( LPTSTR out, int MaxLen )	\
	{	_ncpy( out, MaxLen, _T( xx ) );				\
		return S_OK;								\
	}	

//

#define DARTH_DEFINE_CATEGORY( xx )						\
	__declspec( dllexport ) HRESULT GetTestCategory( LPTSTR out, int MaxLen )	\
	{	_ncpy( out, MaxLen, _T( xx ) );					\
		return S_OK;									\
	}	

//

#define DARTH_DEFINE_CONTACT( xx )						\
	__declspec( dllexport ) HRESULT GetTestContact( LPTSTR out, int MaxLen )	\
	{	_ncpy( out, MaxLen, _T( xx ) );					\
		return S_OK;									\
	}	

//

#define DARTH_DEFINE_DESCRIPTION( xx )						\
	__declspec( dllexport ) HRESULT GetTestDescription( LPTSTR out, int MaxLen )	\
	{	_ncpy( out, MaxLen, _T( xx ) );						\
		return S_OK;										\
	}	

//

#define DARTH_REQUIRE_DACOM( drt, out_dacom_ptr, ini_string, failure )	\
		if( (out_dacom_ptr = DACOM_Acquire()) == NULL ) {				\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to acquire DACOM") );	\
			failure;													\
		}																\
		if( _tcslen( ini_string	) ) {									\
			if( FAILED( out_dacom_ptr->SetINIConfig( ini_string, 0 ) ) ) {	\
				drt->Log( IDR_LC_TEST, 0, _T("Unable to set DACOM INI") );	\
				failure;													\
			}															\
		}
		
//

#define DARTH_REQUIRE_SYSTEM( drt, in_dacom_ptr, out_system_ptr, failure )				\
	{																					\
		AGGDESC adesc = "ISystemContainer";												\
		if( FAILED( in_dacom_ptr->CreateInstance( &adesc, (void**)&out_system_ptr) ) ) {\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to create system container") );		\
			failure;																	\
		}																				\
		if( FAILED( out_system_ptr->LoadSystemComponents() ) ) {						\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to load system components") );			\
			failure;																	\
		}																				\
	}

//

#define DARTH_REQUIRE_ENGINE( drt, in_dacom_ptr, in_system_ptr, out_engine_ptr, failure )\
	{																					 \
		DACOMDESC edesc = "IEngine";													 \
		if( FAILED( in_dacom_ptr->CreateInstance( &edesc, (void**)&out_engine_ptr ) ) ) {\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to create engine component") );		\
			failure;																	\
		}																				\
		if( FAILED( out_engine_ptr->load_engine_components( in_system_ptr ) ) ) {		\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to load engine components") );			\
			failure;																	\
		}																				\
	}

//

#define DARTH_REQUIRE_INTERFACE( drt, in_component, iid, out_ptr, failure )			\
		if( FAILED( in_component->QueryInterface( iid, (void**)&out_ptr ) ) ) {		\
			drt->Log( IDR_LC_TEST, 0, _T("Unable to get interface %s"), iid );		\
			failure;																\
		}
//

#define DARTH_RELEASE( iff ) if( iff ) { iff->Release(); iff = NULL; } 

// Useful for tests that don't require hooks into DllMain
//
#define DARTH_DEFINE_DLLMAIN( ModuleHandle )	\
	BOOL APIENTRY DllMain( HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved )	\
	{									\
		ModuleHandle = hModule;	\
		switch( ul_reason_for_call ) {	\
			case DLL_PROCESS_ATTACH:	\
			case DLL_THREAD_ATTACH:		\
			case DLL_THREAD_DETACH:		\
			case DLL_PROCESS_DETACH:	\
				break;					\
		}								\
		return TRUE;					\
	}




//

#endif	// EOF
