// DirectDraw.cpp
//
//
// 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dacom.h"
#include "FDUMP.h"
#include "TEMPSTR.h"
#include "tsmartpointer.h"

#include "DirectDraw.h"
//#include "RenderDebugger.h"

//

const char *rp_rd_ddmessage( HRESULT error );

//

U32 rp_dd_debug_fill_color = 0;
static U32 rp_dd_lock_flags = 0;
static HMODULE rp_dd_dll_handle = 0;

//

//      Maxi Gamer Phoenix 2            0x5008 (20488) TNT Vanta
//      Maxi Gamer Xentor               0x5810 (22544) TNT2
//      Maxi Gamer Xentor 32            0x5820 (22560) TNT2 Ultra
//      Maxi Gamer Cougar               0x4D20 (19744) TNT2 M64
//      Maxi Gamer Cougar Video Edition 0x5620 (22048) TNT2 M64
//      Maxi Gamer 3D Prophet           0x5020 (20512) GeForce256

//      dwVendorId  0x12D2 (4818)
//          DeviceId 0x0018 (24) and 0x0019 (25): RIVA128
//
//      dwVendorID 0x10DE (4318)
//          DeviceId  0x0020 (32)                 TNT
//          DeviceId  0x0028 (40)  to 0xFF  (255) TNT2
//          DeviceId  0x0100 (256) to 0x103 (259) GeForce

//      Voodoo Graphics   0x121A (4634)   0x0001 (1)
//      Voodoo 2          0x121A (4634)   0x0002 (2)
//      Voodoo Banshee    0x121A (4634)   0x0003 (3)
//      Voodoo 3          0x121A (4634)   0x0005 (5)  (0x0004 is skipped)
//      Voodoo Rush       0x1142 (4418)   0x643D (25661)
//      Voodoo Rush       0x10D9 (4313)   0x8626 (34342)

//   ** FROM 3DLABS (Kevin Strange [Kevin.Strange@3dlabs.com])
//
//      Chip        Vendor ID    Device ID      Revision
//      ----------- ------------ -------------- --------
//      300SX       3D3D (15677) 0x0001 (1)     0x00 (0)
//      300SX-RO2   3D3D (15677) 0x0001 (1)     0x02 (2)
//      DELTA       3D3D (15677) 0x0003 (3)     0x01 (1)
//      500TX       3D3D (15677) 0x0002 (2)     0x01 (1)
//      PERMEDIA    3D3D (15677) 0x0004 (4)     0x01 (1)
//      TI P1       104C (4172)  0x3D04 (15620) 0x01 (1)
//      P2          3D3D (15677) 0x0007 (7)     0x01 (1)
//      TI P2       104C (4172)  0x3D07 (15623) 0x01 (1)
//      MX          3D3D (15677) 0x0006 (6)     0x01 (1)
//      GAMMA       3D3D (15677) 0x0008 (8)     0x01 (1)
//      P2S (ST)    3D3D (15677) 0x0009 (9)     0x01 (1)
//      TI P2A      104C (4172)  0x3D07 (15623) 0x11 (17)
//      P3          3D3D (15677) 0x000A (10)    0x01 (1)
//      R3          3D3D (15677) 0x000B (11)    0x01 (1)
//      P4          3D3D (15677) 0x000C (12)    0x01 (1)
//      R4          3D3D (15677) 0x000D (13)    0x01 (1)
//      G2          3D3D (15677) 0x000E (14)    0x01 (1)
//

//
//      Creative is always 0x1102 (4354).
//
//      Vendor        Vendor ID      Chips           Device ID
//      ------------- -------------- --------------- ---------
//      Cirrus Logic  0x1013 (4115)  CL5446          0x00B8 (184)
//                                   CL5462          0x00D0 (208)
//                                   CL5464          0x00D4 (212)
//                                   CL5465          0x00D6 (214)
//
//      Rendition     0x1163 (4451)  V1000           0x0001 (1)
//
//      3D Labs       0x3D3D (15677) Permedia        0x0004 (4)
//                                   Permedia2v      0x0009 (9)
//
//      TI            0x104C (4172)  Permedia2c      0x3D07 (15623)
//
//      nVidia        0x10DE (4318)  128ZX           0x0018 (24) 
//                                   TNT             0x0020 (32)
//                                   TNT2, TNT2 Pro  0x0028 (40)
//                                   TNT2 Ultra      0x0029 (41)
//                                   TNT2 Vanta      0x002C (44)
//                                   TNT2 M64        0x002D (45)
//                                   NV10            0x0100 (256)
//                                   NV10 DDR        0x0101 (257)
//
//      3Dfx          0x121A (4634)  Voodoo2         0x0002 (2)
//                                   Banshee         0x0003 (3)
//
//      S3            0x5333 (21299) Savage4         0x8A22 (35362)
//

//
//   ** FROM MATROX (Elio Del Giudice [edelgiud@matrox.com])
//
//      Matrox Vendor ID:    0x102B (4139)
//
//      STORM                0x0519 (1305)   Millennium
//      HURRICANE            0x051A (1306)   Mystique
//      MISTRAL_PCI          0x051B (1307)   Millennium II PCI
//      MISTRAL_AGP          0x051F (1311)   Millennium II AGP
//      TWISTER_PCI          0x1000 (4096)   G100 PCI
//      TWISTER_AGP          0x1001 (4097)   G100 AGP
//      ECLIPSE_PCI          0x0520 (1312)   G200 PCI
//      ECLIPSE_AGP          0x0521 (1313)   G200 AGP
//      TOUCAN               0x0525 (1317)   G400


//

struct RPDDDEVICEINFO
{
	U32			 vendor_id_val;
	U32			 device_id_val;
	RPDEVICEID	 device_id;
};


RPDDDEVICEINFO rp_dd_device_ids[] =
{
	{ 0x12D2, 0x0018, RP_D_RIVA128			},
	{ 0x12D2, 0x0019, RP_D_RIVA128			},
	{ 0x10DE, 0x0020, RP_D_RIVATNT			},
	{ 0x10DE, 0x0028, RP_D_RIVATNT2			},
	{ 0x10DE, 0x0029, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002A, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002B, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002C, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002D, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002E, RP_D_RIVATNT2			},
	{ 0x10DE, 0x002F, RP_D_RIVATNT2			},
	{ 0x121A, 0x0001, RP_D_VOODOO_1			},
	{ 0x121A, 0x0002, RP_D_VOODOO_2			},
	{ 0x121A, 0x0003, RP_D_VOODOO_BANSHEE	},
	{ 0x121A, 0x0005, RP_D_VOODOO_3			},
	{ 0x1142, 0x643D, RP_D_VOODOO_RUSH		},
	{ 0x10D9, 0x8626, RP_D_VOODOO_RUSH		},
	{ 0x3d3d, 0x0009, RP_D_PERMEDIA_2		},
	{ 0x1102, 0x5333, RP_D_SAVAGE_4			},
	{ 0x102B, 0x0519, RP_D_MILLENIUM		},
	{ 0x102B, 0x051A, RP_D_MYSTIQUE			},	
	{ 0x102B, 0x051B, RP_D_MILLENIUM_2		},
	{ 0x102B, 0x051F, RP_D_MILLENIUM_2		},

	{      0,      0, RP_D_GENERIC			}
};

//

struct RPDDDEVICEBLENDINFO
{
	RPDEVICEID device_id;
	U32		   blend_matrix[4];
};

RPDDDEVICEBLENDINFO	rp_dd_blend_info[] = 
{
	{	RP_D_RIVA128,		{ 0x4018FE3D,0x00442100,0x1A010000,0x00000000 } },
	{	RP_D_PERMEDIA_2,	{ 0x00008800,0x00002000,0x00000000,0x00000000 } },
	{	RP_D_MILLENIUM,		{ 0x00000000,0x00000000,0x00000000,0x00000000 } },
	{	RP_D_MILLENIUM_2,	{ 0x00000000,0x00000000,0x00000000,0x00000000 } },
	{	RP_D_MYSTIQUE,		{ 0x00000000,0x00002000,0x00000000,0x00000000 } },	
	
	// Leave these last
	{	RP_D_GENERIC,		{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } },
	{	RP_D_UNKNOWN,		{ 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF } }
};

//
// DirectDraw Related Stuff
//
#pragma warning( push )
#pragma warning( disable : 4229 )

typedef HRESULT (* WINAPI DirectDrawCreateExFunc)( GUID *lpGuid, LPVOID *lplpDD, REFIID iid, IUnknown * );
typedef HRESULT (* WINAPI DirectDrawCreateFunc)( GUID *lpGuid, LPVOID *lplpDD, IUnknown * );


HRESULT rp_dd_is_directx7( void )
{
	HMODULE hDll;
	HRESULT hr = E_FAIL;
	DirectDrawCreateExFunc DDCX;
	const char *dll_name = "DDRAW.DLL";

	if( (hDll = LoadLibrary( dll_name )) != NULL ) {
		if( (DDCX = (DirectDrawCreateExFunc)GetProcAddress( hDll, "DirectDrawCreateEx" )) != NULL ) {
			hr = S_OK;
		}
		FreeLibrary( hDll );
	}

	if( hr == E_FAIL ) {
		GENERAL_ERROR( "It does not appear that DirectX 7 or later is installed\n" );
	}

	return hr;
}


//

struct rp_dd_find_data
{
	char device_id[MAX_PATH];
	char out_device_guid[MAX_PATH];
};

//

static BOOL WINAPI rp_dd_find_device( GUID FAR *guid, LPSTR driver_desc, LPSTR driver_name, LPVOID context, HMONITOR hm )
{
	rp_dd_find_data *data = (rp_dd_find_data *)context;
	char gu[60];
	char desc[MAX_PATH];

	ConvertGUIDToString( guid, gu );

	if( stricmp( gu, data->device_id ) == 0 ) {
		strcpy( data->out_device_guid, gu );
		return FALSE;	// end enumeration
	}
	else {

		strcpy( desc, driver_desc );
		_strlwr( desc );
		
		if( strstr( desc, data->device_id ) != NULL ) {
			ConvertGUIDToString( guid, data->out_device_guid );
			return FALSE;
		}
	}

	return TRUE;
}

static BOOL WINAPI rp_dd_ipeak_fuckin_sucks ( GUID FAR *guid, LPSTR driver_desc, LPSTR driver_name, LPVOID context, HMONITOR hm )
{
	// Don't end the enumeration, but don't do anything.
	return TRUE;
}
//

HRESULT rp_dd_find_device_guid( const char *device_guid, const char *dll_name, char *out_device_guid )
{
	LPDIRECTDRAWENUMERATEEXA DDEnumerateEx;
	HMODULE hDll;
	rp_dd_find_data data;

	strcpy( data.device_id, device_guid );
	_strlwr( data.device_id );

	data.out_device_guid[0] = 0;

	if( (hDll = LoadLibrary( dll_name )) != NULL ) {

		if( (DDEnumerateEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( hDll, "DirectDrawEnumerateExA" )) != NULL ) {
			DDEnumerateEx( rp_dd_find_device, &data, DDENUM_ATTACHEDSECONDARYDEVICES|DDENUM_DETACHEDSECONDARYDEVICES|DDENUM_NONDISPLAYDEVICES );
		}
		else {
			GENERAL_ERROR( TEMPSTR( "rp_dd_find_device_guid: DirectDrawEnumerateExA is not in %s", dll_name ) );
		}

		FreeLibrary( hDll );
	}

	if( data.out_device_guid[0] != 0 ) {
		return S_OK;
	}

	return E_FAIL;
}

//

HRESULT rp_dd_init_directdraw( LPGUID lpGuid, const char *dll_name, LPDIRECTDRAW7 *out_lpDD )
{
	COMPTR<IDirectDraw> lpDD = NULL;
	HRESULT hr;

	//if( rp_dd_dll_handle ) {
		//FreeLibrary( rp_dd_dll_handle );
		//rp_dd_dll_handle = 0;
	//}

	//if( (rp_dd_dll_handle = LoadLibrary( dll_name )) == NULL ) {
		//GENERAL_ERROR( TEMPSTR( "rp_dd_init_directdraw: unable to find library: %s\n", dll_name ) );
		//return E_FAIL;
	//}

	//DirectDrawCreateExFunc DDCX;

	// HACK!!!
	// Enumerate to make IPeak happy. It doesn't load the real DLL until you enumerate, and only when you enumerate.
	/*{
		LPDIRECTDRAWENUMERATEEXA DDEnumerateEx;
		if( (DDEnumerateEx = (LPDIRECTDRAWENUMERATEEXA)GetProcAddress( rp_dd_dll_handle, "DirectDrawEnumerateExA" )) != NULL ) {
			DDEnumerateEx( rp_dd_ipeak_fuckin_sucks, NULL, DDENUM_ATTACHEDSECONDARYDEVICES|DDENUM_DETACHEDSECONDARYDEVICES|DDENUM_NONDISPLAYDEVICES );
		}
		else {
			GENERAL_ERROR( "Failed to find DirectDraw enumeration function. IPeak won't work.\n" );
		}
	}*/
	// END HACK!!!

	/*
	if( (DDCX = (DirectDrawCreateExFunc)GetProcAddress( rp_dd_dll_handle, "DirectDrawCreateEx" )) == NULL ) {
		GENERAL_NOTICE( "rp_dd_init_directdraw: unable to find DirectDrawCreateEx\n" );
		FreeLibrary( rp_dd_dll_handle );
		rp_dd_dll_handle = 0;
		return E_FAIL;
	}*/

	//if( FAILED( hr = DDCX( lpGuid, lpDD, IID_IDirectDraw7, NULL ) ) ) {
	//	*out_lpDD = NULL;
	//	GENERAL_NOTICE( TEMPSTR( "rp_dd_init_directdraw: unable to create DirectDraw: %s\n", rp_rd_ddmessage(hr) ) );
	//	FreeLibrary( rp_dd_dll_handle );
	//	rp_dd_dll_handle = 0;
	//	return hr;
	//}

    /*
	hr = DirectDrawCreate(NULL, lpDD, NULL) ;
    if (FAILED(hr) || NULL == lpDD)
	{
			GENERAL_NOTICE( TEMPSTR( "rp_dd_init_directdraw: unable to create DirectDraw: %s\n", rp_rd_ddmessage(hr) ) );
			return hr;
	}

	if( FAILED( hr = lpDD->QueryInterface( IID_IDirectDraw7, (void **)out_lpDD ) ) ) {
		*out_lpDD = NULL;
		GENERAL_NOTICE( TEMPSTR( "rp_dd_init_directdraw: unable to acquire IID_IDirectDraw7: %s\n", rp_rd_ddmessage(hr) ) );
		FreeLibrary( rp_dd_dll_handle );
		rp_dd_dll_handle = 0;
		return hr;
	}
*/
	return S_OK;
}

//

HRESULT rp_dd_cleanup_directdraw( void )
{
	if( rp_dd_dll_handle ) {
		FreeLibrary( rp_dd_dll_handle );
		rp_dd_dll_handle = 0;
	}

	return S_OK;
}

//

#pragma warning( pop )

//

HRESULT rp_dd_get_display_mode( LPDIRECTDRAW7 lpDD, RPDISPLAYMODEINFO *out_display_mode )
{
	HRESULT hr = E_FAIL;
	
	out_display_mode->mode_num = 0xFFFFFFFF;
	out_display_mode->render_pf.init( 8, 0, 0, 0, 0 );

	if( lpDD ) {
		DDSURFACEDESC2 ddsd2;
		memset( &ddsd2, 0, sizeof(ddsd2) );
		ddsd2.dwSize = sizeof(ddsd2);
		if( SUCCEEDED( hr = lpDD->GetDisplayMode( &ddsd2 ) ) ) {
			out_display_mode->width = ddsd2.dwWidth;
			out_display_mode->height = ddsd2.dwHeight;
			out_display_mode->render_pf.init( ddsd2.ddpfPixelFormat );
		}
	}

	if( FAILED( hr ) ) {
		HDC hDC = GetWindowDC( GetDesktopWindow() );
		U32 color_bpp = GetDeviceCaps( hDC, BITSPIXEL );
		out_display_mode->width = GetDeviceCaps( hDC, HORZRES );
		out_display_mode->height = GetDeviceCaps( hDC, VERTRES );
		ReleaseDC( GetDesktopWindow(), hDC );
		U32 r, g, b, a;
		switch( color_bpp ) {
		case 8:		r = 0;	g = 0;	b = 0;	a=0; break;
		case 15:	r = 5;	g = 5;	b = 5;	a=0; break;
		case 16:	r = 5;	g = 6;	b = 5;	a=0; break;
		case 24:	r = 8;	g = 8;	b = 8;	a=0; break;
		case 32:	r = 8;	g = 8;	b = 8;	a=8; break;
		default:
			GENERAL_TRACE_1( TEMPSTR( "rp_dd_get_display_mode: unable to determine display mode: %d\n", color_bpp ) );
			return E_FAIL;
		}

		out_display_mode->render_pf.init( color_bpp, r, g, b, a );
	}

	return S_OK;
}

//

struct emddata
{
	RPDISPLAYMODEINFO *modes;
	U32 num_modes;
};

static HRESULT WINAPI rp_dd_get_display_modes_cb( LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext )
{
	DDPIXELFORMAT ddpf = lpDDSurfaceDesc->ddpfPixelFormat;
	emddata *dev = (emddata*)lpContext;

	if( ddpf.dwRGBBitCount >= 15 ) {
		dev->modes[dev->num_modes].mode_num	= dev->num_modes;
		dev->modes[dev->num_modes].width	= lpDDSurfaceDesc->dwWidth;
		dev->modes[dev->num_modes].height	= lpDDSurfaceDesc->dwHeight;
		dev->modes[dev->num_modes].render_pf.init(ddpf);
		dev->num_modes++;
	}

	return DDENUMRET_OK;
}

static HRESULT WINAPI rp_dd_get_display_modes_count_cb( LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext )
{
	DDPIXELFORMAT ddpf = lpDDSurfaceDesc->ddpfPixelFormat;
	if( ddpf.dwRGBBitCount >= 15 ) {
		emddata *dev = (emddata*)lpContext;
		dev->num_modes++;
	}
	return DDENUMRET_OK;
}

HRESULT rp_dd_get_display_modes( LPDIRECTDRAW7 lpDD, RPDISPLAYMODEINFO **modes, U32 *num_modes )
{
	ASSERT( lpDD );

	*modes = NULL;
	*num_modes = 0;

	emddata data;
	data.num_modes = 0;
	if( SUCCEEDED( lpDD->EnumDisplayModes( 0, NULL, &data, rp_dd_get_display_modes_count_cb ) ) ) {
		data.modes = new RPDISPLAYMODEINFO[data.num_modes];
		data.num_modes = 0;
		if( SUCCEEDED( lpDD->EnumDisplayModes( 0, NULL, &data, rp_dd_get_display_modes_cb ) ) ) {
			*modes = data.modes;
			*num_modes = data.num_modes;
			return S_OK;
		}
		delete data.modes;
	}
	
	return E_FAIL;
}

//

struct etfdata
{
	PixelFormat *formats;
	U32 num_formats;
};

static HRESULT WINAPI rp_d3d_get_texture_formats_cb( LPDDPIXELFORMAT lpDDPF, LPVOID lpContext )
{
	etfdata *dev = (etfdata*)lpContext;
 
	if(true || !(lpDDPF->dwFlags & DDPF_LUMINANCE) ) {
		dev->formats[dev->num_formats].init(*lpDDPF);
		dev->num_formats++;
	}

	return DDENUMRET_OK;
}
/*
static HRESULT WINAPI rp_d3d_get_texture_formats_count_cb( LPDDPIXELFORMAT lpDDPF, LPVOID lpContext )
{
	etfdata *dev = (etfdata*)lpContext;
	if(true || !(lpDDPF->dwFlags & DDPF_LUMINANCE) ) {
		dev->num_formats++;
	}
	return DDENUMRET_OK;
}


HRESULT rp_d3d_get_texture_formats( LPDIRECT3DDEVICE9 lpD3D, PixelFormat **formats, U32 *num_formats )
{
	ASSERT( lpD3D );

	*formats = NULL;
	*num_formats = 0;

	etfdata data;
	data.num_formats = 0;
	if( SUCCEEDED( lpD3D->EnumTextureFormats( rp_d3d_get_texture_formats_count_cb, &data ) ) ) {
		data.formats = new PixelFormat[data.num_formats];
		data.num_formats = 0;
		if( SUCCEEDED( lpD3D->EnumTextureFormats( rp_d3d_get_texture_formats_cb, &data ) ) ) {
			*formats = data.formats;
			*num_formats = data.num_formats;
			return S_OK;
		}
		delete data.formats;
	}
	
	return E_FAIL;
}
*/
//
/*
HRESULT rp_d3d_get_blend_matrix( LPDIRECTDRAW7 lpDD, LPD3DDEVICEDESC7 lpD3DCaps, U32 *out_modes )
{
	RPDEVICEID device_id;

	rp_dd_get_device_id( lpDD, device_id );

	for( U32 cnt=0; rp_dd_blend_info[cnt].device_id!=RP_D_UNKNOWN; cnt++ ) {

		if( rp_dd_blend_info[cnt].device_id == device_id ) {
			out_modes[0] = rp_dd_blend_info[cnt].blend_matrix[0];
			out_modes[1] = rp_dd_blend_info[cnt].blend_matrix[1];
			out_modes[2] = rp_dd_blend_info[cnt].blend_matrix[2];
			out_modes[3] = rp_dd_blend_info[cnt].blend_matrix[3];

			return S_OK;
		}
		else if( rp_dd_blend_info[cnt].device_id == RP_D_GENERIC ) {
			
			// Try to guess what the supported blendmodes are
			// from the caps bits.  This gets us closer, but there
			// are usually combinations that are not supported that
			// show up as supported here because blend(A,B) != blend(B,A)
			//
			for( U32 src=0; src<10; src++ ) {
				for( U32 dst=0; dst<10; dst++ ) {
					U32 pos = src * 10 + dst;
					U32 idx = pos / 32;
					U32 bit = pos % 32;
					
					if( (lpD3DCaps->dpcTriCaps.dwSrcBlendCaps & (1<<src)) && 
						(lpD3DCaps->dpcTriCaps.dwDestBlendCaps & (1<<dst)) ) {
						
						rp_dd_blend_info[cnt].blend_matrix[idx] |= (1<<bit);
					}
					else {
						rp_dd_blend_info[cnt].blend_matrix[idx] &= ~(1<<bit);
					}
				}
			}
	
			out_modes[0] = rp_dd_blend_info[cnt].blend_matrix[0];
			out_modes[1] = rp_dd_blend_info[cnt].blend_matrix[1];
			out_modes[2] = rp_dd_blend_info[cnt].blend_matrix[2];
			out_modes[3] = rp_dd_blend_info[cnt].blend_matrix[3];

			return S_OK;
		}

	}

	return S_OK;
}
*/
//
/*
struct rp_d3d_enum_devices_data
{
	LPGUID device_type_guid;
	bool is_supported;
};

//

HRESULT CALLBACK rp_d3d_enum_devices_cb( LPSTR lpDeviceDescription, LPSTR lpDeviceName, LPD3DDEVICEDESC7 lpD3DDeviceDesc, LPVOID lpContext )
{
	rp_d3d_enum_devices_data *data = (rp_d3d_enum_devices_data *)lpContext;

	if( memcmp( &lpD3DDeviceDesc->deviceGUID, data->device_type_guid, sizeof(lpD3DDeviceDesc->deviceGUID) ) != 0 ) {
		return D3DENUMRET_OK;
	}

	data->is_supported = true;
	return D3DENUMRET_CANCEL;
}

//

HRESULT rp_d3d_is_device_type_supported( LPDIRECTDRAW7 lpDD, LPGUID device_type_guid )
{
	COMPTR<IDirect3D7> D3D;
	rp_d3d_enum_devices_data data;

	if( FAILED( lpDD->QueryInterface( IID_IDirect3D7, D3D ) ) ) {
		return E_FAIL;
	}

	data.device_type_guid = device_type_guid;
	data.is_supported = false;

	if( FAILED( D3D->EnumDevices( rp_d3d_enum_devices_cb, (void*)&data ) ) ) {
		return E_FAIL;
	}

	if( !data.is_supported ) {
		return E_FAIL;
	}	

	return S_OK;
}

//
*/
HRESULT rp_dd_get_driver_id( LPDIRECTDRAW7 lpDD, LPGUID out_lpGuid )
{
	ASSERT( lpDD );
	ASSERT( out_lpGuid );

	DDDEVICEIDENTIFIER2 dvid;
	if( FAILED( lpDD->GetDeviceIdentifier( &dvid, 0 ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_get_driver_id: unable to determine vendor id\n" ) );
		return E_FAIL;
	}

	memcpy( out_lpGuid, &dvid.guidDeviceIdentifier, sizeof(dvid.guidDeviceIdentifier) );

	return S_OK;
}

//


HRESULT rp_dd_get_device_id( LPDIRECTDRAW7 lpDD, RPDEVICEID &id )
{
	ASSERT( lpDD );

	DDDEVICEIDENTIFIER2 dvid;
	if( FAILED( lpDD->GetDeviceIdentifier( &dvid, 0 ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_get_device_id: unable to determine vendor id\n" ) );
		return E_FAIL;
	}

	id = RP_D_GENERIC;
	for( U32 d=0; rp_dd_device_ids[d].device_id_val; d++ ) {
		if( dvid.dwVendorId == rp_dd_device_ids[d].vendor_id_val &&
			dvid.dwDeviceId == rp_dd_device_ids[d].device_id_val ) {
			id = rp_dd_device_ids[d].device_id;
			break;
		}
	}
	return S_OK;
}

//


HRESULT rp_dd_is_nvidia( LPDIRECTDRAW7 lpDD )
{
	ASSERT( lpDD );

	DDDEVICEIDENTIFIER2 dvid;

	if( FAILED( lpDD->GetDeviceIdentifier( &dvid, 0 ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_is_nvidia: unable to determine vendor id\n" ) );
		return E_FAIL;
	}

	if( dvid.dwVendorId == 0x12D2 || dvid.dwVendorId == 0x10DE ) {
		return S_OK;
	}
	else {
		return E_FAIL;
	}
}

//
HRESULT rp_dd_get_driver_info( LPDIRECTDRAW7 lpDD, char *out_device_name, char *out_device_desc )
{
	ASSERT( lpDD );
	ASSERT( out_device_name );
	ASSERT( out_device_desc );

	DDDEVICEIDENTIFIER2 dvid;
	if( FAILED( lpDD->GetDeviceIdentifier( &dvid, 0 ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_get_driver_info: unable to determine vendor info\n" ) );
		return E_FAIL;
	}

	GENERAL_NOTICE( TEMPSTR( "rp_dd_get_driver_info: %s\n", dvid.szDriver ) );
	GENERAL_NOTICE( TEMPSTR( "rp_dd_get_driver_info: %s\n", dvid.szDescription ) );

	strcpy( out_device_name, dvid.szDriver );
	strcpy( out_device_desc, dvid.szDescription );

	return S_OK;
}

//

HRESULT rp_dd_get_memory( LPDIRECTDRAW7 lpDD, U32 caps, U32 *out_total_bytes, U32 *out_free_bytes )
{
	ASSERT( lpDD );

	HRESULT hr;
	U32 t, f;

	if( out_total_bytes == NULL ) {
		out_total_bytes = &t;
	}
	if( out_free_bytes == NULL ) {
		out_free_bytes = &f;
	}

	DDSCAPS2 ddscaps2;
	memset( &ddscaps2, 0, sizeof(ddscaps2) );
	ddscaps2.dwCaps = caps;
	if( FAILED( hr = lpDD->GetAvailableVidMem( &ddscaps2, out_total_bytes, out_free_bytes ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_get_memory: unable to get memory counts\n" ) );
		return E_FAIL;
	}

	COMPTR<IDirectDrawSurface7> IGDIS = NULL;
	if( SUCCEEDED( lpDD->GetGDISurface( IGDIS ) ) ) {
		DDSURFACEDESC2 ddsd2;
		memset( &ddsd2, 0, sizeof(ddsd2) );
		ddsd2.dwSize = sizeof(ddsd2);
		if( SUCCEEDED( IGDIS->GetSurfaceDesc( &ddsd2 ) ) ) {
			*out_total_bytes += ddsd2.dwHeight * ddsd2.lPitch;
		}
	}

	return S_OK;
}

//

HRESULT rp_dd_get_syslock( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 surface )
{
	// Verify that the driver allows NOSYSLOCK.
	// NOSYSLOCK allows drivers to not grab the win16 mutex if at all possible.
	//
	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);

	HRESULT hr;
	if( SUCCEEDED( hr = surface->Lock( NULL, &ddsd2, DDLOCK_NOSYSLOCK|DDLOCK_WAIT, NULL ) ) ) {
		surface->Unlock( NULL );
		return S_OK;
	} 

	return E_FAIL;
}

//

HRESULT rp_dd_get_2d_only( LPDIRECTDRAW7 lpDD )
{
	// Check to see if this device is 2D only
	// returns S_OK if so, E_FAIL otherwise

	DDCAPS ddHW, ddHEL;
	
	memset( &ddHW, 0, sizeof(ddHW) );
	ddHW.dwSize = sizeof(DDCAPS);
	memset( &ddHEL, 0, sizeof(ddHEL) );
	ddHEL.dwSize = sizeof(DDCAPS);

	if( SUCCEEDED( lpDD->GetCaps( &ddHW, &ddHEL ) ) ) {
		if( !(ddHW.dwCaps & DDCAPS_3D) ) {
			GENERAL_TRACE_1( "rp_dd_get_2d_only: DirectDraw device is 2D only.\n" );
			return S_OK;
		}
	}

	GENERAL_TRACE_1( "rp_dd_get_2d_only: DirectDraw device is not 2D only.\n" );
	return E_FAIL;
}

//

HRESULT rp_dd_get_fullscreen_override( LPDIRECTDRAW7 lpDD )
{
	// Check to see if we need to override the fullscreen status.
	// We do this for all 3D only cards (ala 3DFX Voodoo 1, 2).

	// returns S_OK if need to override, E_FAIL otherwise

	DDCAPS ddHW, ddHEL;
	
	memset( &ddHW, 0, sizeof(ddHW) );
	ddHW.dwSize = sizeof(DDCAPS);
	memset( &ddHEL, 0, sizeof(ddHEL) );
	ddHEL.dwSize = sizeof(DDCAPS);

	if( SUCCEEDED( lpDD->GetCaps( &ddHW, &ddHEL ) ) ) {
		if( !(ddHW.dwCaps2 & DDCAPS2_CANRENDERWINDOWED ) ) {
			GENERAL_TRACE_1( "rp_dd_get_fullscreen_override: Direct3D device does not support windowed rendering.  Ignoring non-fullscreen flags.\n" );
			return S_OK;
		}
	}

	GENERAL_TRACE_1( "rp_dd_get_fullscreen_override: Direct3D device supports windowed rendering.\n" );
	return E_FAIL;
}

//

HRESULT rp_dd_swap_buffers( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 *color_buffers, U32 num_color_buffers, RECT *src, RECT *dst, U32 rprd_flags )
{
	ASSERT( lpDD );

	if( num_color_buffers < 2 ) {
		return S_OK;
	}

	if( color_buffers[0] == NULL ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_swap_buffers: color_buffers[0] is NULL!\n" ) );	
		return E_FAIL;
	}

	HRESULT hr;
	
	if( (rprd_flags & RPRD_F_FLIP) && ((rprd_flags & (RPRD_F_FULLSCREEN_NOW|RPRD_F_FULLSCREEN_ALWAYS)) || (rprd_flags & RPRD_F_HWFLIP)) ) {
		U32 flip_fl = DDFLIP_WAIT;
		if( !(rprd_flags & RPRD_F_HWVSYNC) ) {
			flip_fl |= DDFLIP_NOVSYNC;
		}

		if( FAILED( hr = color_buffers[0]->Flip( NULL, flip_fl ) ) ) {
			GENERAL_NOTICE( TEMPSTR( "rp_dd_swap_buffers: flip failed: %s\n", rp_rd_ddmessage(hr) ) );
			return hr;
		}

		if( rprd_flags & RPRD_F_FLIP_STALLS ) {
			while( FAILED( color_buffers[1]->GetFlipStatus( DDGFS_ISFLIPDONE ) ) );
		}
	}
	else {

		if( rprd_flags & RPRD_F_FULLSCREEN )	{
			// In fullscreen mode, use Blt() with the full surface to full surface.
			// This will possibly stretch or shrink.
			//
			if( FAILED( hr = color_buffers[0]->Blt( NULL, color_buffers[1], NULL, DDBLT_WAIT, NULL ) ) ) {
				GENERAL_NOTICE( TEMPSTR( "rp_dd_swap_buffers: fullscreen blt failed: %s\n", rp_rd_ddmessage(hr) ) );
				return hr;
			}
		}
		else {
			// In windowed mode, use the Blt() function for proper
			// clipping (e.g., if the window is moved partially offscreen)
			// 
			if( FAILED( hr = color_buffers[0]->Blt( dst, color_buffers[1], src, DDBLT_WAIT, NULL ) ) ) {
				GENERAL_NOTICE( TEMPSTR( "rp_dd_swap_buffers: windowed blt failed: %s\n", rp_rd_ddmessage(hr) ) );
				return hr;
			}
		}

		if( rprd_flags & RPRD_F_FLIP_STALLS ) {
			while( FAILED( color_buffers[1]->GetBltStatus( DDGBS_ISBLTDONE ) ) );
		}
	}

	return S_OK;
}

//

HRESULT rp_dd_set_fullscreen( LPDIRECTDRAW7 lpDD, HWND hwnd, U32 width, U32 height, U32 bpp, U32 rpdd_flags )
{
	HRESULT hr;
	U32 fpu, threaded;

	// setup fullscreen
	//
	threaded = (rpdd_flags & RPRD_F_HWTHREADED) ? DDSCL_MULTITHREADED : 0;
	fpu  = (rpdd_flags & RPRD_F_FPUPRESERVE) ? DDSCL_FPUPRESERVE : 0;
	fpu |= (rpdd_flags & RPRD_F_FPUPRESETUP) ? DDSCL_FPUSETUP : 0;
	
	if( FAILED( hr = lpDD->SetCooperativeLevel( hwnd, threaded | fpu | DDSCL_NOWINDOWCHANGES | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_set_fullscreen: could not set exclusive mode: %s\n", rp_rd_ddmessage(hr) ) );
		return hr;
	}

	if( FAILED( hr = lpDD->SetDisplayMode( width, height, bpp, 0, 0 ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_set_fullscreen: could not set display mode: %s\n", rp_rd_ddmessage(hr) ) );
		return hr;
	}

	return S_OK;
}

//

HRESULT rp_dd_set_windowed( LPDIRECTDRAW7 lpDD, HWND hwnd, U32 rpdd_flags )
{
	HRESULT hr;
	U32 fpu, threaded;

	// setup windowed rendering
	//
	threaded = (rpdd_flags & RPRD_F_HWTHREADED) ? DDSCL_MULTITHREADED : 0;
	fpu  = (rpdd_flags & RPRD_F_FPUPRESERVE) ? DDSCL_FPUPRESERVE : 0;
	fpu |= (rpdd_flags & RPRD_F_FPUPRESETUP) ? DDSCL_FPUSETUP : 0;

	if( FAILED( hr = lpDD->RestoreDisplayMode() ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_set_windowed: could not restore display mode: %s\n", rp_rd_ddmessage(hr) ) );
		return hr;
	}

	if( FAILED( hr = lpDD->SetCooperativeLevel( NULL, threaded|fpu|DDSCL_NORMAL  ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_set_windowed: could not restore normal mode: %s\n", rp_rd_ddmessage(hr) ) );
		return hr;
	}

	return S_OK;
}

//

#if 1

HRESULT rp_dd_create_basic_surfaces( LPDIRECTDRAW7 lpDD, RPDDCREATEBUFFERSDATA *init_data, LPDIRECTDRAWSURFACE7 *out_color_buffers )
{
	ASSERT( lpDD );
	ASSERT( init_data );

	HRESULT hr;
	PixelFormat surface_pf;
	
	switch( init_data->buf_color_bpp )
	{
	case 15:	
	case 16:	surface_pf.init( 16, 5,6,5,0 );	break;
	case 24:	surface_pf.init( 24, 8,8,8,0 );	break;
	case 32:	surface_pf.init( 32, 8,8,8,8 );	break;
	default:	return E_FAIL;
	}


	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);

	if( SUCCEEDED( hr = lpDD->GetDisplayMode( &ddsd2 ) ) ) {

		PixelFormat display_pf( ddsd2.ddpfPixelFormat );

		if( init_data->buf_color_bpp != display_pf.num_bits() && !(init_data->rprd_flags & (RPRD_F_FULLSCREEN_NOW)) ) {
			GENERAL_NOTICE( "rp_dd_create_basic_surfaces: not fullscreen and display_bpp != surface_bpp\n" );
			return E_FAIL;
		}

		surface_pf.init( display_pf );
	}

	// Attempt to create primary/backbuffer directdraw surfaces
	//
	U32 buffers_created = 0;
	U32 ddscaps = 0;
	U32 w=0, h=0;
	U32 complex  = (init_data->buf_color_buffers > 1);
	U32 flipping = (init_data->rprd_flags & RPRD_F_FULLSCREEN)	&&
				   (init_data->rprd_flags & RPRD_F_HWVIDMEM)	&&
				   ( (init_data->rprd_flags & RPRD_F_HWFLIP)	||
				     (!(init_data->rprd_flags & RPRD_F_HWFLIP)	&& 
					   (init_data->rprd_flags & RPRD_F_FULLSCREEN_ALWAYS)) );
					

	if( init_data->rprd_flags & RPRD_F_OFFSCREEN ) {
		ddscaps	= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		w = init_data->buf_width;
		h = init_data->buf_height;
	}
	else {
		ddscaps	= DDSCAPS_PRIMARYSURFACE;
	}

	if( init_data->rprd_flags & RPRD_F_HWVIDMEM ) {
		ddscaps |= DDSCAPS_VIDEOMEMORY;
	}
	else {
		ddscaps |= DDSCAPS_SYSTEMMEMORY;
	}

	U32 csf_flags = 0;

	if( init_data->rprd_flags & RPRD_F_ANTIALIAS_HINT ) {
		csf_flags |= RPDD_CSF_ANTIALIAS_HINT;
	}

	out_color_buffers[0] = NULL;
	if( complex && flipping ) {
		
		ddscaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_3DDEVICE;
		if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, ddscaps, w, h, init_data->buf_color_buffers-1, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create complex flipping, trying complex non-flipping\n" );
			ddscaps &= ~(DDSCAPS_FLIP);
			ddscaps &= ~(DDSCAPS_COMPLEX);
			flipping = 0;	// PLB: this was complex = 0;
		}
		else {

			buffers_created = init_data->buf_color_buffers;

			// Get backbuffer
			DDSCAPS2 ddscaps;
			memset( &ddscaps, 0, sizeof(ddscaps) );
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			if( FAILED( hr = out_color_buffers[0]->GetAttachedSurface( &ddscaps, &out_color_buffers[1] ) ) ) {
				GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to get attached backbuffer\n" );
				RELEASE( out_color_buffers[0] );
				return hr;
			}
		}
	}
	
	if( complex && !flipping ) {
		if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, ddscaps, w, h, 0, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create complex non-flipping\n" );
			return hr;
		}
		else {

			buffers_created = 1;

			// Create backbuffer
			U32 bb_ddscaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;

			if( init_data->rprd_flags & RPRD_F_HWVIDMEM ) {
				bb_ddscaps |= DDSCAPS_VIDEOMEMORY;
			}
			else {
				bb_ddscaps |= DDSCAPS_SYSTEMMEMORY;
			}

			if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, bb_ddscaps, init_data->buf_width, init_data->buf_height, 0, NULL, &out_color_buffers[1] ) ) ) {
				GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create backbuffer\n" );
				RELEASE( out_color_buffers[0] );
				return hr;
			}
			else {
				buffers_created++;
			}
		}
	}
	else if( !complex && !out_color_buffers[0] ) {

		if( FAILED( hr = rp_dd_create_surface( lpDD, 0, ddscaps | DDSCAPS_3DDEVICE, w, h, 0, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create non-complex\n" );
			return hr;
		}
		else {
			buffers_created = 1;
			
			out_color_buffers[1] = out_color_buffers[0];
			out_color_buffers[1]->AddRef();
		}
	}

	rp_dd_lock_flags = 0;
	if( SUCCEEDED( rp_dd_get_syslock( lpDD, out_color_buffers[0] ) ) ) {
		rp_dd_lock_flags = DDLOCK_NOSYSLOCK;
	}

	if( buffers_created != init_data->buf_color_buffers ) {
		DACOM_RELEASE( out_color_buffers[1] ) ;
		DACOM_RELEASE( out_color_buffers[0] ) ;
		return E_FAIL;
	}
	else {
		return S_OK;
	}
}
	
#else

HRESULT rp_dd_create_basic_surfaces( LPDIRECTDRAW7 lpDD, RPDDCREATEBUFFERSDATA *init_data, LPDIRECTDRAWSURFACE7 *out_color_buffers )
{
	ASSERT( lpDD );
	ASSERT( init_data );

	HRESULT hr;


	PixelFormat surface_pf;
	
	switch( init_data->buf_color_bpp )
	{
	case 15:	
	case 16:	surface_pf.init( 16, 5,6,5,0 );	break;
	case 24:	surface_pf.init( 24, 8,8,8,0 );	break;
	case 32:	surface_pf.init( 32, 8,8,8,8 );	break;
	default:	return E_FAIL;
	}

	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);
	if( SUCCEEDED( hr = lpDD->GetDisplayMode( &ddsd2 ) ) ) {
		PixelFormat display_pf( ddsd2.ddpfPixelFormat );

		if( init_data->buf_color_bpp != display_pf.num_bits() && !(init_data->rprd_flags & (RPRD_F_FULLSCREEN_NOW)) ) {
			GENERAL_WARNING( "rp_dd_create_basic_surfaces: not fullscreen and display_bpp != surface_bpp\n" );
			return E_FAIL;
		}

		surface_pf.init( display_pf );
	}

	// Attempt to create primary/backbuffer directdraw surfaces
	//
	U32 ddscaps = 0;
	U32 w=0, h=0;
	U32 complex  = (init_data->buf_color_buffers > 1);
	U32 flipping = (init_data->rprd_flags & RPRD_F_FULLSCREEN)	&&
				   (init_data->rprd_flags & RPRD_F_HWVIDMEM)	&&
				   ( (init_data->rprd_flags & RPRD_F_HWFLIP)	||
				    !(init_data->rprd_flags & RPRD_F_HWFLIP)	&& (init_data->rprd_flags & RPRD_F_FULLSCREEN_ALWAYS) );
					

	if( init_data->rprd_flags & RPRD_F_OFFSCREEN ) {
		ddscaps	= DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
		w = init_data->buf_width;
		h = init_data->buf_height;
	}
	else {
		ddscaps	= DDSCAPS_PRIMARYSURFACE;
	}

	if( init_data->rprd_flags & RPRD_F_HWVIDMEM ) {
		ddscaps |= DDSCAPS_VIDEOMEMORY;
	}
	else {
		ddscaps |= DDSCAPS_SYSTEMMEMORY;
	}

	U32 csf_flags = 0;

	if( init_data->rprd_flags & RPRD_F_ANTIALIAS_HINT ) {
		csf_flags |= RPDD_CSF_ANTIALIAS_HINT;
	}

	out_color_buffers[0] = NULL;
	if( complex && flipping ) {
		
		ddscaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_3DDEVICE;
		if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, ddscaps, w, h, init_data->buf_color_buffers-1, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create complex flipping, trying complex non-flipping\n" );
			ddscaps &= ~(DDSCAPS_FLIP);
			ddscaps &= ~(DDSCAPS_COMPLEX);
			complex = 0;
		}
		else {
			// Get backbuffer
			DDSCAPS2 ddscaps;
			memset( &ddscaps, 0, sizeof(ddscaps) );
			ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
			if( FAILED( hr = out_color_buffers[0]->GetAttachedSurface( &ddscaps, &out_color_buffers[1] ) ) ) {
				GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to get attached backbuffer\n" );
				RELEASE( out_color_buffers[0] );
				return hr;
			}
		}
	}
	
	if( complex && !flipping ) {
		if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, ddscaps, w, h, 0, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create complex non-flipping\n" );
			return hr;
		}
		else {
			// Create backbuffer
			U32 bb_ddscaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;

			if( init_data->rprd_flags & RPRD_F_HWVIDMEM ) {
				bb_ddscaps |= DDSCAPS_VIDEOMEMORY;
			}
			else {
				bb_ddscaps |= DDSCAPS_SYSTEMMEMORY;
			}

			if( FAILED( hr = rp_dd_create_surface( lpDD, csf_flags, bb_ddscaps, init_data->buf_width, init_data->buf_height, 0, NULL, &out_color_buffers[1] ) ) ) {
				GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create backbuffer\n" );
				RELEASE( out_color_buffers[0] );
				return hr;
			}
		}
	}
	else if( out_color_buffers[0] == NULL ) {
		if( FAILED( hr = rp_dd_create_surface( lpDD, 0, ddscaps | DDSCAPS_3DDEVICE, w, h, 0, NULL, &out_color_buffers[0] ) ) ) {
			GENERAL_TRACE_1( "rp_dd_create_basic_surfaces: failed to create non-complex\n" );
			return hr;
		}
		else {
			out_color_buffers[1] = out_color_buffers[0];
			out_color_buffers[1]->AddRef();
		}
	}

	rp_dd_lock_flags = 0;
	if( SUCCEEDED( rp_dd_get_syslock( lpDD, out_color_buffers[0] ) ) ) {
		rp_dd_lock_flags = DDLOCK_NOSYSLOCK;
	}

	return S_OK;
}

#endif

//

HRESULT rp_dd_create_surface( LPDIRECTDRAW7 lpDD, U32 rpdd_csf_flags, U32 caps, U32 w, U32 h, U32 lod, PixelFormat *pf, LPDIRECTDRAWSURFACE7 *out_surface )
{
	ASSERT( lpDD );
	ASSERT( out_surface );

	HRESULT hr;
	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );

	ddsd2.dwSize = sizeof(ddsd2);
	ddsd2.dwFlags = DDSD_CAPS;
	ddsd2.ddsCaps.dwCaps = caps;

	if( pf ) {
		ddsd2.dwFlags |= DDSD_PIXELFORMAT;
		ddsd2.ddpfPixelFormat = pf->ddpf;
	}
	
	if( w ) {
		ddsd2.dwFlags |= DDSD_WIDTH;
		ddsd2.dwWidth = w;
	}

	if( h) {
		ddsd2.dwFlags |= DDSD_HEIGHT;
		ddsd2.dwHeight = h;
	}

	if( lod ) {
		if( caps & DDSCAPS_TEXTURE ) {
			if( lod > 1 ) {
				ddsd2.dwFlags |= DDSD_MIPMAPCOUNT;
				ddsd2.ddsCaps.dwCaps |= DDSCAPS_COMPLEX|DDSCAPS_MIPMAP;
				ddsd2.dwMipMapCount = lod;
			}
		}
		else if ( caps & (DDSCAPS_FLIP) ) {
			ddsd2.dwFlags |= DDSD_BACKBUFFERCOUNT;
			ddsd2.ddsCaps.dwCaps |= DDSCAPS_COMPLEX;
			ddsd2.dwBackBufferCount = lod;
		}
	}

	if( caps & DDSCAPS_TEXTURE ) {
		if( rpdd_csf_flags & RPDD_CSF_FOR_SOFTWARE ) {
			ddsd2.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		}
		else if( !(ddsd2.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) ) {
			ddsd2.ddsCaps.dwCaps2 |= DDSCAPS2_TEXTUREMANAGE;
		}
		if( rpdd_csf_flags & RPDD_CSF_DYNAMIC ) {
			ddsd2.ddsCaps.dwCaps2 |=  DDSCAPS2_HINTDYNAMIC;
		}
		else {
			ddsd2.ddsCaps.dwCaps2 |=  DDSCAPS2_HINTSTATIC;
		}
	}

	if( rpdd_csf_flags & RPDD_CSF_ANTIALIAS_HINT ) {
		ddsd2.ddsCaps.dwCaps2 |= DDSCAPS2_HINTANTIALIASING;
	}

	if( rpdd_csf_flags & RPDD_CSF_CM_ALL ) {
		
		ddsd2.ddsCaps.dwCaps  |= DDSCAPS_COMPLEX;
		ddsd2.ddsCaps.dwCaps2 |= DDSCAPS2_CUBEMAP;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_POS_X)? DDSCAPS2_CUBEMAP_POSITIVEX : 0;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_NEG_X)? DDSCAPS2_CUBEMAP_NEGATIVEX : 0;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_POS_Y)? DDSCAPS2_CUBEMAP_POSITIVEY : 0;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_NEG_Y)? DDSCAPS2_CUBEMAP_NEGATIVEY : 0;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_POS_Z)? DDSCAPS2_CUBEMAP_POSITIVEZ : 0;
		ddsd2.ddsCaps.dwCaps2 |= (rpdd_csf_flags & RPDD_CSF_CM_NEG_Z)? DDSCAPS2_CUBEMAP_NEGATIVEZ : 0;
	}

	if( FAILED( hr = lpDD->CreateSurface( &ddsd2, out_surface, NULL ) ) ) {
		U32 tot, fre;
		rp_dd_get_memory( lpDD, caps, &tot, &fre );
		if( pf ) {
			GENERAL_TRACE_1( TEMPSTR( "rp_dd_create_surface: unable to create surface: %dx%d lod:%d caps:%08X bpp:%d (%d)\n", w, h, lod, caps, pf->ddpf.dwRGBBitCount, fre ) );
		}
		else {
			GENERAL_TRACE_1( TEMPSTR( "rp_dd_create_surface: unable to create surface: %dx%d lod:%d caps:%08X (%d)\n", w, h, lod, caps, fre ) );
		}
		GENERAL_TRACE_1( TEMPSTR(  "rp_dd_create_surface: %s\n", rp_rd_ddmessage(hr) ) );
		return hr;
	}

	if( pf && pf->is_indexed() ) {
		rp_dd_surface_set_palette( lpDD, *out_surface, 0, 0, NULL );
	}

	if( !(caps & DDSCAPS_PRIMARYSURFACE) && !(caps & DDSCAPS_TEXTURE) && !(caps & DDSCAPS_ZBUFFER) ) {
		rp_dd_surface_fill( *out_surface, pf, rp_dd_debug_fill_color, DDBLT_COLORFILL, NULL );
	}

	return S_OK;
}

//

HRESULT rp_dd_surface_verify( LPDIRECTDRAWSURFACE7 surface )
{
	if( surface == NULL ) {
		return E_FAIL;
	}

	HRESULT hr;
	if( FAILED( hr = surface->IsLost() ) ) {
		if( FAILED( rp_dd_surface_restore( surface ) ) ) {
			GENERAL_TRACE_1( "rp_dd_surface_verify: could not restore surface\n" );
		}
		return hr;
	}

	return S_OK;
}

//

HRESULT rp_dd_surface_restore( LPDIRECTDRAWSURFACE7 surface )
{
	ASSERT( surface );

	HRESULT hr;
	if( FAILED( hr = surface->Restore() ) ) {
		if( hr == DDERR_WRONGMODE ) {
			GENERAL_TRACE_1( "rp_dd_surface_restore: mode change detected\n" );
		}
		return hr;
	}

	return S_OK;
}

//

HRESULT rp_dd_surface_set_palette( LPDIRECTDRAW7 lpDD, LPDIRECTDRAWSURFACE7 surface, U32 start, U32 num_colors, RGB *in_colors)
{
	ASSERT( surface );
	ASSERT( ( !num_colors || in_colors ) );

	HRESULT hr = E_FAIL;
	IDirectDrawPalette* lpPal = NULL;
	PALETTEENTRY colors[256];
	U32 c;

	for( c=0; c<num_colors; c++ ) {
		colors[c].peRed		= in_colors[c].r;
		colors[c].peGreen	= in_colors[c].g;
		colors[c].peBlue	= in_colors[c].b;
		colors[c].peFlags	= 0;
	}
	for( ; c<256; c++ ) {
#ifdef _DEBUG
#define RPDD_PAL_FILL_COLOR_R 0xFF
#define RPDD_PAL_FILL_COLOR_G 0xFF
#define RPDD_PAL_FILL_COLOR_B 0x00
#else
#define RPDD_PAL_FILL_COLOR_R 0x00
#define RPDD_PAL_FILL_COLOR_G 0x00
#define RPDD_PAL_FILL_COLOR_B 0x00
#endif
		colors[c].peRed		= RPDD_PAL_FILL_COLOR_R;
		colors[c].peGreen	= RPDD_PAL_FILL_COLOR_G;
		colors[c].peBlue	= RPDD_PAL_FILL_COLOR_B;
		colors[c].peFlags	= 0;
	}

	if( FAILED( hr = surface->GetPalette( &lpPal ) ) ) {
		if( lpDD && SUCCEEDED( hr = lpDD->CreatePalette( DDPCAPS_8BIT | DDPCAPS_ALLOW256, colors, &lpPal, NULL ) ) ) {
			if( SUCCEEDED( hr = surface->SetPalette( lpPal ) ) ) {
				hr = S_OK;
			}
		}
	}
	else {
		if( SUCCEEDED( hr = lpPal->SetEntries( 0, start, num_colors, colors ) ) ) {
			hr = S_OK;
		}
	}	
	RELEASE( lpPal );
	return hr;
}

//

HRESULT rp_dd_surface_get_palette( LPDIRECTDRAWSURFACE7 surface, U32 start, U32 max_num_colors, RGB *out_colors)
{
	ASSERT( surface );
	ASSERT( out_colors );

	LPDIRECTDRAWPALETTE lpPal;
	PALETTEENTRY colors[256];

	if( SUCCEEDED( surface->GetPalette( &lpPal ) ) ) {
		if( SUCCEEDED( lpPal->GetEntries( 0, start, max_num_colors, colors ) ) ) {
			for( U32 c=0; c<max_num_colors; c++ ) {
				out_colors[c].r = colors[c].peRed;
				out_colors[c].g = colors[c].peGreen;
				out_colors[c].b = colors[c].peBlue;
			}
			return S_OK;
		}
	}
	return E_FAIL;
}

//

HRESULT rp_dd_surface_get_mipmaps( LPDIRECTDRAWSURFACE7 surface, U32 *out_mipcount, LPDIRECTDRAWSURFACE7 *out_mipmaps ) 
{
	ASSERT( surface );
	ASSERT( out_mipmaps );
	ASSERT( out_mipcount );

	LPDIRECTDRAWSURFACE7 dds = surface, ddsnext=NULL;
	DDSCAPS2 ddsCaps;
	ddsCaps.dwCaps = DDSCAPS_TEXTURE|DDSCAPS_MIPMAP;

	*out_mipcount = 0;
	U32 level = 0;
	out_mipmaps[level] = dds;
	do {
		level++;
		if( FAILED( dds->GetAttachedSurface( &ddsCaps, &ddsnext ) ) ) {
			break;
		}
		out_mipmaps[level] = ddsnext;
		dds = ddsnext;
	} while( dds );

	*out_mipcount = level;
	return S_OK;
}

//

HRESULT rp_dd_surface_get_dims( LPDIRECTDRAWSURFACE7 *surface, U32 *out_width, U32 *out_height, U32 *out_mipcount )
{
	ASSERT( surface );

	return S_OK;
}

//

HRESULT rp_dd_surface_bitblt( LPDIRECTDRAWSURFACE7 surface, 
							  const void *src_bits, 
							  int src_width, 
							  int src_height, 
							  int src_stride, 
							  const PixelFormat &src_format_in,
							  const RGB *src_palette, 
							  const U8 *src_alpha )
{
	ASSERT( src_bits );
	ASSERT( surface );

	HRESULT hr;
	DDSURFACEDESC2 ddsd2;
	memset( &ddsd2, 0, sizeof(ddsd2) );
	ddsd2.dwSize = sizeof(ddsd2);

	if( FAILED( hr = surface->Lock( NULL, &ddsd2, rp_dd_lock_flags|DDLOCK_WAIT, 0 ) ) ) {
		GENERAL_TRACE_1( "rp_dd_surface_bitblt: failed to lock destination surface\n" );
		return E_FAIL;
	}
	
	U8			*dst_bits = (U8*)ddsd2.lpSurface;
	U32			 dst_bd = ((ddsd2.ddpfPixelFormat.dwRGBBitCount+7)>>3);	// dst byte depth
	U32			 dst_stride = ddsd2.lPitch; // - ddsd2.dwWidth * dst_bd;	
	PixelFormat  dst_format( ddsd2.ddpfPixelFormat );
	U32			 dst_width = ddsd2.dwWidth;
	U32			 dst_height = ddsd2.dwHeight;


	mem_bitblt( dst_bits, dst_width, dst_height, dst_stride, dst_format,
				src_bits, src_width, src_height, src_stride, src_format_in,
				src_palette, src_alpha);

	surface->Unlock( NULL );

	return S_OK;

}

//

HRESULT rp_dd_surface_fill( LPDIRECTDRAWSURFACE7 surface, PixelFormat *pf, U32 value, U32 flags, RECT *scissor )
{
	if( surface == NULL ) {
		return E_FAIL;
	}

	HRESULT hr;
	DDBLTFX fx;

	fx.dwSize = sizeof(fx);

	if( pf ) {
		fx.dwFillColor = pf->compute( _R(value), _G(value), _B(value), _A(value) );
	}
	else {
		fx.dwFillColor = value;
	}

	if( FAILED( hr = surface->Blt( scissor, NULL, NULL, flags | DDBLT_WAIT, &fx ) ) ) {
		GENERAL_TRACE_1( TEMPSTR( "rp_dd_surface_fill: %08X: %s\n", surface, rp_rd_ddmessage(hr) ) );
		if( scissor ) {
			GENERAL_TRACE_1( TEMPSTR( "rp_dd_surface_fill: %d,%d %d %d\n", scissor->left, scissor->top, scissor->right, scissor->bottom ) );
		}
		else {
			GENERAL_TRACE_1( "rp_dd_surface_fill: scissor == NULL\n" );
		}
		return E_FAIL;
	}

	return S_OK;
}

//

