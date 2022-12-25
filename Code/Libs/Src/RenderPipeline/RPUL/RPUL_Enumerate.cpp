// RPUL_Enumerate.cpp
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dacom.h"
#include "TSmartPointer.h"
#include "RendPipeline.h"
#include "RPUL/RPUL_misc.h"
#include "FDUMP.h"

#include <stdio.h>

struct ENUMDATA {
	U32 count;
	RPUL_ENUMERATOR_FUNC cb;
	void *user_data;
};

//

static BOOL WINAPI DD_enumerate_ex( GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext, HMONITOR hm )
{
	ENUMDATA *data = (ENUMDATA*)lpContext;

	char gu[60];
	ConvertGUIDToString( lpGUID, gu );

	COMPTR<IDAComponent> device;
	ICOManager *ICOManager = DACOM_Acquire();
	RPUL_DACOMDESC desc( "Direct3D", gu );
	if( FAILED( ICOManager->CreateInstance( &desc, (void**) &device ) ) ) {
		return TRUE;
	}

	COMPTR<IRenderPipeline> IRP;
	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**) &IRP ) ) ) { 
		if( SUCCEEDED( IRP->startup() ) ) {

			char sz[255+1];
			sprintf( sz, "Device: Direct3D: %s %s %s\n", lpDriverName, lpDriverDescription, gu );
			GENERAL_NOTICE( sz );
			data->cb( device, data->user_data );
			data->count++;
		}
	}
	return TRUE;
}

//

static BOOL WINAPI DD_enumerate( GUID FAR *lpGUID, LPSTR lpDriverDescription, LPSTR lpDriverName, LPVOID lpContext )
{
	ENUMDATA *data = (ENUMDATA*)lpContext;

	char gu[60];
	ConvertGUIDToString( lpGUID, gu );

	COMPTR<IDAComponent> device;
	ICOManager *ICOManager = DACOM_Acquire();
	RPUL_DACOMDESC desc( "Direct3D", gu );
	if( FAILED( ICOManager->CreateInstance( &desc, (void**) &device ) ) ) {
		return TRUE;
	}

	COMPTR<IRenderPipeline> IRP;
	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**) &IRP ) ) ) { 
		if( SUCCEEDED( IRP->startup() ) ) {

			char sz[255+1];
			sprintf( sz, "Device: Direct3D: %s %s %s\n", lpDriverName, lpDriverDescription, gu );
			GENERAL_NOTICE( sz );
			data->cb( device, data->user_data );
			data->count++;
		}
	}
	return TRUE;
}

//

typedef HRESULT (WINAPI * LPDIRECTDRAWENUMERATEA)( LPDDENUMCALLBACKA lpCallback, LPVOID lpContt );

static U32 EnumerateDirect3D( RPUL_ENUMERATOR_FUNC cb, void *user_data )
{
	ENUMDATA data = { 0, cb, user_data };
	
	LPDIRECTDRAWENUMERATEEXA	DDEnumerateEx;
	LPDIRECTDRAWENUMERATEA		DDEnumerate;
	
	HMODULE hMod = LoadLibrary( "DDRAW.DLL" );
	if( hMod != 0 ) {
		if( (DDEnumerateEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( hMod, "DirectDrawEnumerateExA" )) != NULL ) {
			DDEnumerateEx( DD_enumerate_ex, &data, DDENUM_ATTACHEDSECONDARYDEVICES|DDENUM_DETACHEDSECONDARYDEVICES|DDENUM_NONDISPLAYDEVICES );
		}
		else if( (DDEnumerate = (LPDIRECTDRAWENUMERATEA)GetProcAddress( hMod, "DirectDrawEnumerateA" )) != NULL ) {
			DDEnumerate( DD_enumerate, &data );
		}
	}
	FreeLibrary( hMod );

	return data.count;
}

//

static U32 EnumerateRaster( RPUL_ENUMERATOR_FUNC cb, void *user_data )
{
	char gu[60];
	ConvertGUIDToString( NULL, gu );

	COMPTR<IDAComponent> device;
	ICOManager *ICOManager = DACOM_Acquire();
	RPUL_DACOMDESC desc( "Raster", gu );
	if( FAILED( ICOManager->CreateInstance( &desc, (void**) &device ) ) ) {
		return TRUE;
	}

	COMPTR<IRenderPipeline> IRP;
	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**) &IRP ) ) ) { 
		if( SUCCEEDED( IRP->startup() ) ) {

			char sz[255+1];
			sprintf( sz, "Device: Raster: %s\n", gu );
			GENERAL_NOTICE( sz );
			cb( device, user_data );
		}
	}
	return 1;
}

//

GENRESULT EnumerateRenderPipelineDevices( U32 classes, RPUL_ENUMERATOR_FUNC cb, void *user_data, U32 *out_num )
{
	U32 num_devices = 0;

	if( classes & RPUL_DIRECT3D ) {
		num_devices += EnumerateDirect3D( cb, user_data );
	}
	if( classes & RPUL_RASTER ) {
		num_devices += EnumerateRaster( cb, user_data );
	}
	/*
	if( classes & RPUL_GLIDE ) {
		num_devices += EnumerateGlide( cb, user_data );
	}
	if( classes & RPUL_OPENGL ) {
		num_devices += EnumerateOpenGL( cb, user_data );
	}
	*/

	if( out_num ) {
		*out_num = num_devices;
	}

	return num_devices?GR_OK:GR_GENERIC;
}
