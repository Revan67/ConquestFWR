// DACOMProvider.cpp: implementation of the CDACOMProvider class.
//
//////////////////////////////////////////////////////////////////////


#include "DACOMProvider.h"

#define RELEASE(iff) if( iff ) { iff->Release(); iff = NULL; }

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#include <comdef.h>

CDACOMProvider::CDACOMProvider()
{
	System = NULL;
	Engine = NULL;
	Manager = NULL;
}

CDACOMProvider::~CDACOMProvider()
{
	Cleanup();
}

HRESULT CDACOMProvider::Initialize(const char *IniFile)
{
	if( (Manager = DACOM_Acquire()) == NULL ) {
		return E_FAIL;
	}

	Manager->SetINIConfig( IniFile );
//	Manager->SetINIFile( IniFile );

	AGGDESC adesc = "ISystemContainer";
	if( FAILED( Manager->CreateInstance(&adesc, (void**)&System) ) ) {
		return E_FAIL;
	}

	System->LoadSystemComponents();

	DACOMDESC desc = "IEngine";
	if( FAILED( Manager->CreateInstance( &desc, (void**)&Engine ) ) ) {
		return E_FAIL;
	}

	Engine->load_engine_components( System );

	Engine->update (0);

	return S_OK;
}

HRESULT CDACOMProvider::Cleanup()
{
	RELEASE( Engine );
	RELEASE( System );
	RELEASE( Manager );
	return S_OK;
}

HRESULT CDACOMProvider::QueryInterface(const char *Interface, void **IFF)
{
	if( FAILED( System->QueryInterface( Interface, (void**)IFF) ) ) {
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CDACOMProvider::ReleaseInterface(IDAComponent * IFF)
{
	RELEASE(IFF);
	return S_OK;
}
