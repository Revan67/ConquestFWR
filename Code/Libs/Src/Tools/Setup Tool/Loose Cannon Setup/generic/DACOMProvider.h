// DACOMProvider.h: interface for the CDACOMProvider class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DACOMPROVIDER_H__3F8D81BB_4C87_11D2_AE71_0000F4A24D28__INCLUDED_)
#define AFX_DACOMPROVIDER_H__3F8D81BB_4C87_11D2_AE71_0000F4A24D28__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include <comdef.h>

#include "dacom.h"
#include "system.h"
#include "engine.h"

class CDACOMProvider  
{
public:
	HRESULT ReleaseInterface( IDAComponent *IFF );
	HRESULT QueryInterface( const char *Interface, void **IFF );
	HRESULT Cleanup();
	HRESULT Initialize( const char *IniFile );

	ISystemContainer *System;
	IEngine *Engine;
	ICOManager *Manager;

	CDACOMProvider();
	virtual ~CDACOMProvider();
};

#endif // !defined(AFX_DACOMPROVIDER_H__3F8D81BB_4C87_11D2_AE71_0000F4A24D28__INCLUDED_)
