// RenderPipelineEnumerate.cpp
//
// 
//


#include <windows.h>
#include <windowsx.h>
#include <vector>

#include "DarthTest.h"

#include "resource.h"

#include "DACOM.h"
#include "TSmartPointer.h"
#include "RPUL.h"


//

static HANDLE s_Module;
static ICOManager *s_DACOM = NULL;
static TCHAR s_DevicePersist[MAX_PATH];

typedef IRenderPipeline* RPDEVICE;
typedef vector<RPDEVICE> RPDEVICELIST;

RPDEVICELIST Devices;
int			 QueryDeviceAbilities = 0;



//

DARTH_DEFINE_NAME(			"RenderPipelineEnumerate" )
DARTH_DEFINE_CONTACT(		"pbleisch" )
DARTH_DEFINE_CATEGORY(		"RenderPipeline.Enumerate" )
DARTH_DEFINE_DESCRIPTION(	"Enumerates all of the RenderPipeline devices available on the system." )
DARTH_DEFINE_DLLMAIN(		s_Module )

//

BOOL EnumerateCB( IDAComponent *device, void *user )
{
	IDarthRuntime *RT = (IDarthRuntime *)user;
	IRenderPipeline* IRP;
	U32 value[4];
	TCHAR *Ret;

	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**)&IRP ) ) ) {

		if( QueryDeviceAbilities ) {

			for( U32 Ability=1; Ability<RP_A_MAX_ABILITY; Ability++ ) {

				if( FAILED( IRP->query_device_ability( (RPDEVICEABILITY)Ability, value,  NULL ) ) ) {
					Ret = _T("failed");
				}
				else {
					Ret = _T("passed");
				}

				RT->Log( IDR_LC_TEST, 0, _T("query_device_ability( %03d, &value, NULL ) %s: value = (%08X, %08d) "), Ability, Ret, value[0], value[0] );
			}
		}

		Devices.push_back( IRP );
	}

	return TRUE;
}

//

static BOOL CALLBACK DeviceListDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static IDarthRuntime *RT = NULL;

	HWND hTypes = GetDlgItem( hDlg, IDC_TYPE );
	HWND hNames = GetDlgItem( hDlg, IDC_NAME );
	HWND hIDs   = GetDlgItem( hDlg, IDC_ID );
	HWND hD3DType = GetDlgItem( hDlg, IDC_D3DTYPE );

	RPDEVICEINFO dinfo;

	switch( uMsg ) {
	
	case WM_INITDIALOG:
		{
			RT = (IDarthRuntime*)lParam;

			U32 num_devices;
			GENRESULT gr;
			if( FAILED( gr = EnumerateRenderPipelineDevices( RPUL_DIRECT3D|RPUL_OPENGL|RPUL_RASTER, EnumerateCB, RT, &num_devices ) ) ) {
				RT->Log( IDR_LC_TEST, 0, _T("EnumerateRenderPipelineDevices failed with %08X"), gr );
				EndDialog( hDlg, 0 );
				return TRUE;
			}

			if( num_devices != Devices.size() ) {
				RT->Log( IDR_LC_TEST, 0, _T("EnumerateRenderPipelineDevices num_devices does not match size: %d, %d"), num_devices, Devices.size() );
			}

			if( QueryDeviceAbilities ) {
				EndDialog( hDlg,  0 );
				return FALSE;
			}

			ComboBox_ResetContent( hTypes );
			ComboBox_SetItemData( hTypes, ComboBox_AddString( hTypes, "Direct3D" ),	RPUL_DIRECT3D );
			ComboBox_SetItemData( hTypes, ComboBox_AddString( hTypes, "OpenGL" ), RPUL_OPENGL );
			ComboBox_SetItemData( hTypes, ComboBox_AddString( hTypes, "Raster" ), RPUL_RASTER );
			ComboBox_SetCurSel( hTypes, 0 );

			ComboBox_ResetContent( hD3DType );
			ComboBox_SetItemData( hD3DType, ComboBox_AddString( hD3DType, "HAL" ),	0 );
			ComboBox_SetItemData( hD3DType, ComboBox_AddString( hD3DType, "REF" ),	1 );
			ComboBox_SetItemData( hD3DType, ComboBox_AddString( hD3DType, "RGB" ),	2 );
			ComboBox_SetItemData( hD3DType, ComboBox_AddString( hD3DType, "MMX" ),	3 );
			ComboBox_SetItemData( hD3DType, ComboBox_AddString( hD3DType, "NUL" ),	4 );
			ComboBox_SetCurSel( hD3DType, 0 );

			ComboBox_ResetContent( hNames );
			for( RPDEVICELIST::iterator i=Devices.begin(); i<Devices.end(); i++ ) {
				(*i)->get_device_info( &dinfo );
				ComboBox_SetItemData( hTypes, ComboBox_AddString( hNames, dinfo.device_description ),	i );
			}
			ComboBox_SetCurSel( hNames, 0 );

			(Devices[0])->get_device_info( &dinfo );
			Edit_SetText( hIDs, dinfo.device_id_persist );

		}
		break;

	case WM_NOTIFY:
		(Devices[ComboBox_GetCurSel(hNames)])->get_device_info( &dinfo );
		Edit_SetText( hIDs, dinfo.device_id_persist );
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) ) {
		case IDOK:		
			(Devices[0])->get_device_info( &dinfo );
			_tcscpy( s_DevicePersist, dinfo.device_id_persist );
			EndDialog( hDlg, 1 );	
			break;

		case IDCANCEL:	
			(Devices[ComboBox_GetCurSel(hNames)])->get_device_info( &dinfo );
			_tcscpy( s_DevicePersist, dinfo.device_id_persist );
			EndDialog( hDlg, 1 );	
			break;
		}
		break;

	}

	return FALSE;
}


//

__declspec( dllexport ) HRESULT RunTest( IDarthRuntime *RunTime )
{
	TCHAR IniFile[MAX_PATH];
	IniFile[0] = 0;
	RPDEVICELIST::iterator i;
	
	
	RunTime->GetTestArgument( 0, IDR_AT_INT, 0, &QueryDeviceAbilities, 0 );

	RunTime->GetIniFilename( IniFile, MAX_PATH );
	DARTH_REQUIRE_DACOM( RunTime, s_DACOM, IniFile, goto TestFailed );

	Devices.clear();

	if( DialogBoxParam( ((HINSTANCE)s_Module), MAKEINTRESOURCE(IDD_DEVICE_DIALOG), NULL, DeviceListDialogProc, (LPARAM)RunTime ) ) {
		RunTime->Log( IDR_LC_TEST, 0, _T("User selected '%s'"), s_DevicePersist );
	}

	RunTime->PushReturnValue( IDR_AT_STRING, s_DevicePersist );

	for( i=Devices.begin(); i<Devices.end(); i++ ) {
		(*i)->shutdown();
		(*i)->Release();
		(*i) = NULL;
	}

	if( s_DACOM ) {
		s_DACOM->ShutDown();
		s_DACOM = NULL;
	}

	MATH_ENGINE_Uninitialize();

	return DARTH_TEST_PASS;

TestFailed:

	if( s_DACOM ) {
		s_DACOM->ShutDown();
		s_DACOM = NULL;
	}

	MATH_ENGINE_Uninitialize();

	return DARTH_TEST_FAIL;
}

//
