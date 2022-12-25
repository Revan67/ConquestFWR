// ObjectViewer.cpp
//
//
//
//


//#define STRICT
#include <windows.h>
#include <comdef.h>
#include <afxres.h>
#include <Zmouse.h>
#include <stdio.h>

#include "resource.h"

#include "dacom.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "IProfileParser.h"
#include "RPUL.h"

#include "DACOMProvider.h"
#include "IWindow.h"
#include "IDeviceSelector.h"
#include "IDACOMClient.h"

#include "hotsetup.h"
#include "deviceselection.h"

#include "resc1.h"

#pragma warning(disable:4800) // unsigned long: forcing value to bool

// globals
bool ds_app_supported_resolutions[V_RES_LAST_RES] = {	true,
														true,	//640x480
														true,	//800x600
														true,	//1024
														false,	//1152
														false,	//1280
														false	//1600
													};

bool ds_selected_device_abilities[RP_A_MAX_ABILITY] = {0};
int ds_min_bit_depth = 16;

int GetDeviceSelection(HWND hwnd, LPSTR iniFileName, LPSTR szFlags)
{
	char device[255+1], dc[64+1], did[64+1];
	
    U32 flags = RPUL_DIRECT3D | RPUL_RASTER | RPUL_OPENGL;
	char ini_file[255+1];

	strcpy( ini_file, iniFileName );

	char *token;
	token = strtok( szFlags, " \t" );
	while( token ) {
		if( !strncmp( token, "--", 2 ) ) {
			token += 2;
			if( !strcmp( token, "no-direct3d" ) ) {
				flags &= ~(RPUL_DIRECT3D);	
			}
			else if( !strcmp( token, "direct3d" ) ) {
				flags |= RPUL_DIRECT3D;	
			}
		}
		else if( !strncmp( token, "--", 2 ) ) {
			token += 2;
			if( !strcmp( token, "no-raster" ) ) {
				flags &= ~(RPUL_RASTER);	
			}
			else if( !strcmp( token, "raster" ) ) {
				flags |= RPUL_RASTER;	
			}
		}
		else if( !strncmp( token, "--", 2 ) ) {
			token += 2;
			if( !strcmp( token, "no-opengl" ) ) {
				flags &= ~(RPUL_OPENGL);	
			}
			else if( !strcmp( token, "opengl" ) ) {
				flags |= RPUL_OPENGL;	
			}
		}
		else {
			strcpy( ini_file, token );
		}
		token = strtok( NULL, " \t" );
	}

	CDACOMProvider *m_DACOM = new CDACOMProvider();
	if( FAILED ( m_DACOM->Initialize( ini_file ) ) ) {
		return 0;
	}	
	
	extern HRESULT RegisterCDALibs_DeviceWindow( ICOManager *dacom );
	RegisterCDALibs_DeviceWindow( m_DACOM->Manager );

	COMPTR<IDAComponent> Win;
	CLSID_DACOMDESC desc( "CDALibs_DeviceWindow" );
	if( FAILED( m_DACOM->Manager->CreateInstance( &desc, (void**) &Win ) ) ) {
		return 0;
	}

	COMPTR<IDACOMClient> IDC;
	if( SUCCEEDED( Win->QueryInterface( IID_IDACOMClient, (void**) &IDC ) ) ) {
		IDC->SetICOManager( m_DACOM->Manager);
		IDC->SetISystem( m_DACOM->System );
		IDC->SetIEngine( m_DACOM->Engine );
	}

	char title[300+1];
//	sprintf( title, "%s - DeviceSelector", ini_file );
	sprintf( title, "DeviceSelector" );

	// Set current device
	//
	COMPTR<IDeviceSelector> IDS;
	if( FAILED( Win->QueryInterface( IID_IDeviceSelector, (void**) &IDS ) ) ) {
		return 0;
	}	

	IDS->SetAvailableDeviceClasses( flags );

	GetDeviceInfoInFile( ini_file, dc, did );
	CreateDeviceDescription( dc, did, device );
	IDS->SetSelectedDevice( device );

	COMPTR<IModalWindow> IWin;
	if( SUCCEEDED( Win->QueryInterface( IID_IModalWindow, (void**) &IWin ) ) ) {
		IWin->SetTitle( title );
		if( FAILED( IWin->DoModal( hwnd, NULL ) ) ) {
			return 0;
		}
	}
	
	// Get results
	//
	IDS->GetSelectedDevice( device, 255 );
	GetDeviceClassAndId( device, dc, did );
	SetDeviceInfoInFile( ini_file, dc, did );
	return 0;
}

