// Setup_DeviceWindow.cpp: implementation of the CDALibs_DeviceWindow class.
//
//	Stolen from DALibs_DeviceWindow.cpp. Contains functions added for
//	video and audio device selection and configuration for a specific app as part 
//	of the app's setup program.
//
//////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <comdef.h>
#include <stdio.h>
#include <commctrl.h>

#include <prsht.h>

#include "dacom.h"
#include "davariant.h"
#include "TSmartPointer.h"
#include "ITxmLib.h"
#include "FileSys.h"
#include "RPUL.h"
#include "FDump.h"

#include "IDACOMClient.h"
#include "IDeviceSelector.h"
#include "IDeviceConfigurator.h"
#include "IWindow.h"
#include "MessageCrackers.h"
#include "TCollection.h"
#include "CommonDialog.h"
#include "CommonControls.h"

#include <dsound.h>
#include "deviceselection.h"
#include "resource.h"
#include "hotsetup.h"

#include "widclass.h"

using namespace NGLOBALS;

int __cdecl MY_STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

#define ID_NullGuid "{00000000-0000-0000-0000-000000000000}"
static GUID GUID_Null = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
extern char *ds_rp_abilities[RP_A_MAX_ABILITY]; 

HANDLE		ghBitmap;
DIBSECTION	gDib;
RGBQUAD		gDibRgb[256];

char		m_CurrentAudioDevice[64];
char		m_CurrentAudioCaptureDevice[64];
GUID		m_AudioDeviceGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
GUID		m_AudioCaptureDeviceGUID = { 0, 0, 0, {0,0,0,0,0,0,0,0} };
GUID    	m_audioDeviceGuid[16];
GUID    	m_audioCaptureDeviceGuid[16];

extern "C" 
{
	DXDEC DA_ERROR_HANDLER OLD_FDUMP;
}

struct mouseCapturer
{
private:
	HWND m_hWnd;
public:
	mouseCapturer(HWND hWnd) {SetCapture(hWnd);};
	~mouseCapturer() {ReleaseCapture();};
};

#define DACOM_COMPONENT_NAME CDALibs_DeviceWindow

dacom_component CDALibs_DeviceWindow :	dacom_implements IDACOMClient,
										dacom_implements IWindow,
										dacom_implements IModalWindow,
										dacom_implements IDeviceSelector,
										dacom_implements IDeviceConfigurator,
										dacom_implements IComponentFactory
{
	// IWindow
	DACOM_INTERFACE_METHOD_DECL( Create,					( HWND hParent, RECT *rect, U32 flags ));
	DACOM_INTERFACE_METHOD_DECL( Destroy,					( void ));
	DACOM_INTERFACE_METHOD_DECL( Show,						( U32 show ));
	DACOM_INTERFACE_METHOD_DECL( Refresh,					( void ));

	// IModalWindow
	DACOM_INTERFACE_METHOD_DECL( DoModal,					( HWND hParent, U32 *out_return ));
	DACOM_INTERFACE_METHOD_DECL( SetTitle,					( const char *szTitle ));

	// IDeviceSelector
	DACOM_INTERFACE_METHOD_DECL( SetSelectedDevice,			( char *device_descriptor ));
	DACOM_INTERFACE_METHOD_DECL( GetSelectedDevice,			( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD_DECL( SetAvailableDeviceClasses,	( U32 classes ));
	DACOM_INTERFACE_METHOD_DECL( GetAvailableDeviceClasses,	( U32 *classes ));

	// IDeviceConfigurator
	DACOM_INTERFACE_METHOD_DECL( SetValidateCB,	( DS_VALIDATIONCALLBACK fn));
	DACOM_INTERFACE_METHOD_DECL( SetHandleControlCommandCB,	( DS_CONTROLCALLBACK fn));
	DACOM_INTERFACE_METHOD_DECL( GetAbility,				( U32 * supported, U32 ability));
	DACOM_INTERFACE_METHOD_DECL( GetInfo,					( RPDEVICEINFO * info));
	DACOM_INTERFACE_METHOD_DECL( GetAudioDevice,			( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD_DECL( SetAudioDevice,			( char *device_descriptor));
	DACOM_INTERFACE_METHOD_DECL( GetAudioCaptureDevice,		( char *device_descriptor, U32 max_buf_size ));
	DACOM_INTERFACE_METHOD_DECL( SetAudioCaptureDevice,		( char *device_descriptor));
	DACOM_INTERFACE_METHOD_DECL( GetSelectedResolution,		( U32 * resolution));
	DACOM_INTERFACE_METHOD_DECL( SetSelectedResolution,		( U32 resolution));

	// IDACOMClient										
	DACOM_INTERFACE_METHOD_DECL( SetIEngine,				( IEngine *iengine));
	DACOM_INTERFACE_METHOD_DECL( SetISystem,				( ISystemContainer *isystem));
	DACOM_INTERFACE_METHOD_DECL( SetICOManager,				( ICOManager *icomanager));
														
	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	
public:		// Interface
	CDALibs_DeviceWindow();
	~CDALibs_DeviceWindow();

	static BOOL CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DSoundEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext );
	static BOOL CALLBACK DSoundCaptureEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext );
	static BOOL EnumDevices( IDAComponent *, void *user );
	static EnumAudioDevices( HWND hWnd );
	static void bltRenderBuffer(RPLOCKDATA &lock_data, S32 width, S32 height);

	HRESULT OnInitDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT InitVideoDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT InitAudioDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
	HRESULT RefreshDeviceInfo( HWND hControl );
	HRESULT OnContextHelp(HWND hParentWnd, LPHELPINFO pHelpInfo );

	HRESULT OnCancel( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnSelectDevice( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnResolutionCombo( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnAudioDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnAudioCaptureDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnControlCommand( UINT wID, HWND hControl, UINT NotifyCode );
	HRESULT OnNotify(HWND hControl, WPARAM wParam, NMHDR * pNMHDR);
	HRESULT DoPropertySheet(HINSTANCE hInstance, HWND hwndOwner, DLGPROC dlgProc, LPARAM lParam);

protected:	// Private Data
	COMPTR<ICOManager>			m_ICOManager;
	COMPTR<ISystemContainer>	m_ISystem;
	COMPTR<IEngine>				m_IEngine;

	struct DSWINDOWRPDEVICE
	{
		RPDEVICEINFO info;
		U32          abilities[RP_A_MAX_ABILITY];
		BOOL		supportedResolutions[V_RES_LAST_RES];
	};

	U32							m_DeviceClasses;
	char						m_CurrentDevice[64];
	DSWINDOWRPDEVICE			m_Devices[16];
	U32							m_NumDevices;
	U32							m_SelectedResolution;
	

	HWND				 m_hWnd;
	int					 m_RefCnt;
	U32					 m_InRelease;
	U32					 m_Flags;
	char				 m_szTitle[255];

	DS_VALIDATIONCALLBACK	 m_fnValidate;
	DS_CONTROLCALLBACK		 m_fnHandleControlCommand;
};

//

HRESULT RegisterCDALibs_DeviceWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( "CDALibs_DeviceWindow" );
	if( T_DACOM_CreateInstance<CDALibs_DeviceWindow,CLSID_DACOMDESC>( "CDALibs_DeviceWindow", &desc, (IDAComponent **) &IDAC ) == S_OK ) {
		COMPTR<IComponentFactory> ICF;
		if( SUCCEEDED( IDAC->QueryInterface( IID_IComponentFactory, (void**) &ICF ) ) ) {
			dacom->RegisterComponent( ICF, IID_IDAComponent );	// NOTE: DACOM requires us to register an 'interface 
																// provider' not a component!  EEG
			return S_OK;
		}
	}
	return E_FAIL;
}


//
DACOM_INTERFACE_METHOD_IMPL( Create,( HWND hParent, RECT *rect, U32 flags ))
{
	return DoModal(hParent, NULL);
}

//
DACOM_INTERFACE_METHOD_IMPL( Destroy,(void))
{
	Show( FALSE );
	if( !m_InRelease ) {
		DestroyWindow( m_hWnd );
	}
	if (ghBitmap)
		DeleteObject(ghBitmap);

	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( Show,( U32 show ))
{
	ShowWindow( m_hWnd, (show?SW_SHOWNORMAL:SW_HIDE) );
	if( show ) {
		Refresh();
	}
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( Refresh,( void ))
{
	if( m_hWnd == NULL ) {
		return E_FAIL;
	}

	mouseCapturer capture(GetParent(m_hWnd));
	//	ShowWindow( GetDlgItem( GetParent(m_hWnd), IDCANCEL), SW_HIDE );
	SetWindowText( GetDlgItem( GetParent(m_hWnd), IDOK), "&OK" );
	SetWindowText( GetDlgItem( GetParent(m_hWnd), IDCANCEL), "&Cancel" );

	HWND hCB = GetDlgItem( m_hWnd, IDC_VIDEO_COMBO );
	m_NumDevices = 0;
	ComboBox_ResetContent( hCB );
	EnumerateRenderPipelineDevices( m_DeviceClasses, CDALibs_DeviceWindow::EnumDevices, this, NULL );
	
	ASSERT( m_NumDevices );
	char dd[255];

	ComboBox_SetCurSel( hCB, 0 );
	for( U32 i=0; i<m_NumDevices; i++ ) {
		CreateDeviceDescription( m_Devices[i].info.device_class, m_Devices[i].info.device_id_persist, dd );
		if( strcmp( m_CurrentDevice, dd ) == 0 ) { 
			ComboBox_SetCurSel( hCB,  i );
			break;
		}
	}
	RefreshDeviceInfo(hCB);

	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( DoModal,( HWND hParent, U32 *out_return ))
{
	AddRef();
	m_Flags |= IWINDOW_MODAL;
	U32 val = DoPropertySheet( GetResourceInst(), hParent, CDALibs_DeviceWindow::HandleMessage, (LPARAM)this );
	if( out_return ) {
		*out_return = val;
	}
	m_Flags &= ~IWINDOW_MODAL;
	Release();
	return (val != 0)?S_OK:E_FAIL;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetTitle,( const char *szTitle ))
{
	strncpy( m_szTitle, szTitle, 254 );
	m_szTitle[254] = 0;
	SetWindowText( m_hWnd, m_szTitle );
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetSelectedDevice,( char *device_descriptor ))
{
	strcpy( m_CurrentDevice, device_descriptor );
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetSelectedDevice,( char *device_descriptor, U32 max_buf_size ))
{
	strncpy( device_descriptor, m_CurrentDevice, max_buf_size );
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetAvailableDeviceClasses,( U32 classes ))
{
	m_DeviceClasses = classes;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetAvailableDeviceClasses,( U32 *classes ))
{
	*classes = m_DeviceClasses;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetValidateCB,( DS_VALIDATIONCALLBACK fn ))
{
	m_fnValidate = fn;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetHandleControlCommandCB,( DS_CONTROLCALLBACK fn ))
{
	m_fnHandleControlCommand = fn;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetAbility,( U32 * supported, U32 ability ))
{
	U32 sel_dev = ComboBox_GetCurSel( GetDlgItem( m_hWnd, IDC_VIDEO_COMBO ) );
	*supported = m_Devices[sel_dev].abilities[ability];
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetInfo,( RPDEVICEINFO * info ))
{
	U32 sel_dev = ComboBox_GetCurSel( GetDlgItem( m_hWnd, IDC_VIDEO_COMBO ) );
	*info = m_Devices[sel_dev].info;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetAudioDevice,( char *device_descriptor, U32 max_buf_size ))
{
	strncpy( device_descriptor, m_CurrentAudioDevice, max_buf_size );
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetAudioDevice,( char *device_descriptor ))
{
	ConvertStringToGUID(device_descriptor, &m_AudioDeviceGUID);
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetAudioCaptureDevice,( char *device_descriptor, U32 max_buf_size ))
{
	strncpy( device_descriptor, m_CurrentAudioCaptureDevice, max_buf_size );
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetAudioCaptureDevice,( char *device_descriptor ))
{
	ConvertStringToGUID(device_descriptor, &m_AudioCaptureDeviceGUID);
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( GetSelectedResolution,( U32 *resolution ))
{
	*resolution = m_SelectedResolution;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetSelectedResolution,( U32 resolution ))
{
	m_SelectedResolution = resolution;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetIEngine,( IEngine *iengine))
{
	m_IEngine = iengine;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetISystem,( ISystemContainer *isystem))
{
	m_ISystem = isystem;
	return S_OK;
}

//
DACOM_INTERFACE_METHOD_IMPL( SetICOManager,( ICOManager *icomanager))
{
	m_ICOManager = icomanager;
	return S_OK;
}

//
CDALibs_DeviceWindow::CDALibs_DeviceWindow( )
{
	m_RefCnt = 0;
	m_hWnd = NULL;
	m_InRelease = 0;
	strcpy( m_szTitle, "RenderPipeline Devices" );
	m_CurrentDevice[0] = 0;
	m_CurrentAudioDevice[0] = 0;
	m_CurrentAudioCaptureDevice[0] = 0;
	strcpy(m_CurrentAudioDevice, ID_NullGuid);
	strcpy(m_CurrentAudioCaptureDevice, ID_NullGuid);
	m_DeviceClasses = RPUL_DIRECT3D;
}

//
CDALibs_DeviceWindow::~CDALibs_DeviceWindow()
{
	Destroy();
}

//
U32 CDALibs_DeviceWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//
U32 CDALibs_DeviceWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		m_InRelease = 1;
		delete this;
	}
	return 0;
}

//
BOOL CALLBACK CDALibs_DeviceWindow::HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	U32 return_value = TRUE;
	CDALibs_DeviceWindow *a = (CDALibs_DeviceWindow*)GetWindowLong( hWnd, DWL_USER );
	PROPSHEETPAGE * dialogPage;
	switch( message ) {
		case WM_INITDIALOG:
			// for property sheets, lparam is a pointer the dialog, not the user value
			dialogPage = (PROPSHEETPAGE *) lParam;
			SetWindowLong( hWnd, DWL_USER, dialogPage->lParam );
			a = (CDALibs_DeviceWindow*)dialogPage->lParam;
			a->OnInitDialog( hWnd, message, wParam, lParam );
		break;
		case WM_NOTIFY:
			if( SUCCEEDED( a->OnNotify((HWND)lParam, wParam, (NMHDR *) lParam) ) ) {
				return return_value;
			}
			
		break;
		case WM_HELP:
			if ( SUCCEEDED( a->OnContextHelp(hWnd, (LPHELPINFO) lParam) ) )	{
				return return_value;
			}
		break;

		BEGIN_COMMAND_MAP
			ON_COMMAND(IDC_VIDEO_COMBO,OnDeviceCombo)
			ON_COMMAND(IDC_RESOLUTION_COMBO,OnResolutionCombo)
			ON_COMMAND(IDC_AUDIO_COMBO,OnAudioDeviceCombo)
			ON_COMMAND(IDC_AUDIO_CAPTURE_COMBO,OnAudioCaptureDeviceCombo)
			default:
				if( SUCCEEDED( a->OnControlCommand( LOWORD(wParam), (HWND)lParam, HIWORD(wParam) ) ) ) {
					return return_value;
				}
		END_COMMAND_MAP
	}
	return FALSE;
}

//
BOOL CALLBACK CDALibs_DeviceWindow::DSoundEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext )
{
    HWND   hCombo = *(HWND *)lpContext; 		
    LPGUID lpTemp = NULL;
    if ( lpGUID != NULL )
    {
        if (( lpTemp = (LPGUID) LocalAlloc( LPTR, sizeof(GUID))) == NULL )
        return( TRUE );
    }
 
    ComboBox_AddString( hCombo, lpszDesc );
	U32 index = ComboBox_FindString( hCombo, 0, lpszDesc );
	if (lpGUID)
	{
		m_audioDeviceGuid[index] = *lpGUID;
		if (m_AudioDeviceGUID == *lpGUID)
			ComboBox_SetCurSel(hCombo, index);
	}
	else
	{
		memset( &m_audioDeviceGuid[index], 0, sizeof(GUID) );
		if (GUID_Null == m_AudioDeviceGUID)
			ComboBox_SetCurSel(hCombo, index);
	}
    return( TRUE );
}

//
BOOL CALLBACK CDALibs_DeviceWindow::DSoundCaptureEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext )
{
    HWND   hCombo = *(HWND *)lpContext; 		
    LPGUID lpTemp = NULL;
    if ( lpGUID != NULL )
    {
        if (( lpTemp = (LPGUID) LocalAlloc( LPTR, sizeof(GUID))) == NULL )
        return( TRUE );
    }
 
    ComboBox_AddString( hCombo, lpszDesc );
	U32 index = ComboBox_FindString( hCombo, 0, lpszDesc );
	if (lpGUID)
	{
		m_audioCaptureDeviceGuid[index] = *lpGUID;
		if (m_AudioCaptureDeviceGUID == *lpGUID)
			ComboBox_SetCurSel(hCombo, index);
	}
	else
	{
		memset( &m_audioCaptureDeviceGuid[index], 0, sizeof(GUID) );
		if (GUID_Null == m_AudioCaptureDeviceGUID)
			ComboBox_SetCurSel(hCombo, index);
	}
    return( TRUE );
}

class FDUMP_REDIRECTOR
{
	public:
		
	FDUMP_REDIRECTOR(DA_ERROR_HANDLER NEW_FDUMP) {OLD_FDUMP = FDUMP; FDUMP = NEW_FDUMP;};
	~FDUMP_REDIRECTOR() {FDUMP = OLD_FDUMP;};
	int __cdecl MY_STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...);

	private:
	DA_ERROR_HANDLER OLD_FDUMP;
};

//

//
BOOL CDALibs_DeviceWindow::EnumDevices( IDAComponent *device, void *user )
{
	//FDUMP_REDIRECTOR fdump(MY_STANDARD_DUMP);

	CDALibs_DeviceWindow *win = (CDALibs_DeviceWindow*)user;
	HWND hCB = GetDlgItem( win->m_hWnd, IDC_VIDEO_COMBO);
	char sz[255+1];
	RPDISPLAYMODEINFO displayInfo;
	U32 numModes;

	RPLOCKDATA 	lock_data;
	memset(&lock_data, 0, sizeof(RPLOCKDATA));

	COMPTR<IRenderPipeline> IRP;
	if( SUCCEEDED( device->QueryInterface( IID_IRenderPipeline, (void**) &IRP ) ) ) {

		IRP->get_device_info( &win->m_Devices[win->m_NumDevices].info );
		
//		if ((win->m_Devices[win->m_NumDevices].info.device_chipset_id != RP_D_RIVA128) && (strncmp( win->m_Devices[win->m_NumDevices].info.device_class, "Raster", 6 )))
		if ((strncmp( win->m_Devices[win->m_NumDevices].info.device_class, "Raster", 6 )))
		{

			IRP->set_pipeline_state( RP_BUFFERS_COLOR_BPP, 16 );
			IRP->set_pipeline_state( RP_BUFFERS_DEPTH_BPP, 0 );
			IRP->set_pipeline_state( RP_BUFFERS_FULLSCREEN, 1 );
			IRP->set_pipeline_state( RP_BUFFERS_COUNT, 2 );
			if( FAILED( IRP->create_buffers( GetWndParent(), 640, 480 ) ) ) 
					return TRUE;

			if ( SUCCEEDED(IRP->lock_buffer(&lock_data)) )
			{
				bltRenderBuffer(lock_data, 640, 480);
				IRP->unlock_buffer();
			}

			IRP->swap_buffers();

			for( U32 A=RP_A_DEVICE_2D_ONLY; A<RP_A_MAX_ABILITY; A++ ) {
				IRP->query_device_ability( (RPDEVICEABILITY)A, &win->m_Devices[win->m_NumDevices].abilities[A], NULL );
			}
			Sleep(4000);
		}

		BOOL	fSupportsThisResolution = FALSE;
		IRP->get_num_display_modes(&numModes);
		for( U32 I=V_RES_FIRST_RES; I<V_RES_LAST_RES; I++ ) {
			for( U32 A=0; A < numModes; A++ ) {
				IRP->get_display_mode( &displayInfo, A );
				if ((15 <= displayInfo.render_pf.ddpf.dwRGBBitCount) && (displayInfo.width == ds_resolution_X[I]) && (displayInfo.height == ds_resolution_Y[I]))
				{
					fSupportsThisResolution = TRUE;
					break;
				}
			}
			win->m_Devices[win->m_NumDevices].supportedResolutions[I] = fSupportsThisResolution;
		}

		if( !strncmp( win->m_Devices[win->m_NumDevices].info.device_class, "Raster", 6 ) )
		{
			sprintf( sz, "%s", 
				 "Digital Anvil Software Renderer");
		}
		else
		{
			sprintf( sz, "%s - %s", 
				 "D3D Hardware Acceleration", 
				 win->m_Devices[win->m_NumDevices].info.device_description);
		}
		U32 item = ComboBox_AddString( hCB, sz );
		win->m_NumDevices++;
	}
	return TRUE;
}

//
BOOL CDALibs_DeviceWindow::EnumAudioDevices( HWND hWnd )
{
	HWND hSoundCombo = GetDlgItem( hWnd, IDC_AUDIO_COMBO );
	ComboBox_SetCurSel( hSoundCombo, 1 );
	if (DirectSoundEnumerate( (LPDSENUMCALLBACK)CDALibs_DeviceWindow::DSoundEnumProc, &hSoundCombo) != DS_OK)
	{
		return( false );
	}
	HWND hCaptureCombo = GetDlgItem( hWnd, IDC_AUDIO_CAPTURE_COMBO );
	ComboBox_SetCurSel( hCaptureCombo, 1 );
	if (DirectSoundCaptureEnumerate( (LPDSENUMCALLBACK)CDALibs_DeviceWindow::DSoundCaptureEnumProc, &hCaptureCombo) != DS_OK)
	{
		return( false );
	}
	return true;
}

//
HRESULT CDALibs_DeviceWindow::OnInitDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if (lParam) 
	{
		// center the property sheet
		RECT windowRect;
		GetWindowRect(GetParent(hWnd), &windowRect);
		int width = GetSystemMetrics(SM_CXSCREEN);
		int height = GetSystemMetrics(SM_CYSCREEN);
		int x = width/2 - ((windowRect.right - windowRect.left)/2);
		int y = height/2 - ((windowRect.bottom - windowRect.top)/2);

		if (x<0)
			x=0;
		if(y<0)
			y=0;

		SetWindowPos(GetParent(hWnd),          
					 HWND_TOP,          
					 x,y,
					 0,0,
					 SWP_NOACTIVATE | SWP_NOSIZE | SWP_FRAMECHANGED);

		PROPSHEETPAGE * dialogPage = (PROPSHEETPAGE *) lParam;
		if (0 == strcmp("Video",dialogPage->pszTitle))
		{
			InitVideoDialog(hWnd, message, wParam, NULL);
		}
		else
		{
			InitAudioDialog(hWnd, message, wParam, NULL);
		}
		return S_OK;
	}
	return E_FAIL;
}

//
HRESULT CDALibs_DeviceWindow::InitVideoDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{

	m_hWnd = hWnd;
	SetWindowText( GetParent(m_hWnd), m_szTitle );
	
	ghBitmap = LoadImage(GetResourceInst(), MAKEINTRESOURCE(IDB_HW), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION );
	GetObject (ghBitmap, sizeof(DIBSECTION), &gDib);
	if( (gDib.dsBmih.biBitCount ) <= 8 )
	{
       HDC           hMemDC;
       HANDLE       hOldBitmap;
       // Create a memory DC and select the DIBSection into it
       hMemDC = CreateCompatibleDC( NULL );
       hOldBitmap = SelectObject( hMemDC, ghBitmap );
       // Get the DIBSection's color table
       GetDIBColorTable( hMemDC, 0, 256, gDibRgb );
       SelectObject( hMemDC, hOldBitmap );      
	   DeleteDC( hMemDC );
	}
	
	Refresh();
	
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::InitAudioDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	EnumAudioDevices(hWnd);
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::RefreshDeviceInfo( HWND hControl )
{
	HWND parentWnd = GetParent(hControl);
	
	U32 sel_dev = ComboBox_GetCurSel( GetDlgItem( parentWnd, IDC_VIDEO_COMBO ) );
	
	char desc[255+1];

	CreateDeviceDescription( m_Devices[sel_dev].info.device_class, m_Devices[sel_dev].info.device_id_persist, desc );
	SetSelectedDevice(desc);
	// add available and supported resolutions to combo
	HWND hCB = GetDlgItem(parentWnd, IDC_RESOLUTION_COMBO);
	ComboBox_ResetContent(hCB);

	for( U32 A=V_RES_FIRST_RES; A<V_RES_LAST_RES; A++ ) {
		if (m_Devices[sel_dev].supportedResolutions[A])
		{
			COMPTR<IDAComponent> idac;
			static_cast<IModalWindow*>(this)->QueryInterface( IID_IDAComponent, (void**) &idac );
			if (m_fnValidate(idac, hCB, A, DS_VALIDATE_RESOLUTION))
			{
				ComboBox_AddString( hCB, ds_video_resolutions[A] );
				ComboBox_SetItemData( hCB, ComboBox_FindString( hCB, 0, ds_video_resolutions[A] ), A );
			}
		}
	}

	COMPTR<IDAComponent> idac;
	static_cast<IModalWindow*>(this)->QueryInterface( IID_IDAComponent, (void**) &idac );
	m_fnValidate(idac, parentWnd, 0, DS_VALIDATE_SELECTION);

	EnableWindow(hCB, (ComboBox_GetCount(hCB) > 1));

	ComboBox_SetCurSel( hCB,ComboBox_FindString( hCB, 0, ds_video_resolutions[m_SelectedResolution] ) );
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnCancel( UINT wID, HWND hControl, UINT NotifyCode )
{
	if( m_Flags & IWINDOW_MODAL ) {
		EndDialog( m_hWnd, -1 );
	}
	else {
		Release();
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnSelectDevice( UINT wID, HWND hControl, UINT NotifyCode )
{
	if( m_Flags & IWINDOW_MODAL ) {
		EndDialog( m_hWnd, -1 );
	}
	else {
		Release();
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode )
{
	if( NotifyCode == CBN_SELCHANGE ) {
		RefreshDeviceInfo(hControl);
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnResolutionCombo( UINT wID, HWND hControl, UINT NotifyCode )
{
	HWND hCB = GetDlgItem( GetParent(hControl), IDC_RESOLUTION_COMBO );
	if( NotifyCode == CBN_SELCHANGE ) {
		COMPTR<IDAComponent> idac;
		static_cast<IModalWindow*>(this)->QueryInterface( IID_IDAComponent, (void**) &idac );
		if (m_fnValidate (idac, hControl, ComboBox_GetItemData(hCB,ComboBox_GetCurSel(hCB)),DS_VALIDATE_SELECTION))
			m_SelectedResolution = ComboBox_GetItemData(hCB,ComboBox_GetCurSel(hCB));
	}
	return S_OK;
}

HRESULT CDALibs_DeviceWindow::OnAudioDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode )
{
 	if( NotifyCode == CBN_SELCHANGE ) {
		U32 index = ComboBox_GetCurSel(hControl);
		ConvertGUIDToString(&m_audioDeviceGuid[index], m_CurrentAudioDevice);
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnAudioCaptureDeviceCombo( UINT wID, HWND hControl, UINT NotifyCode )
{
 	if( NotifyCode == CBN_SELCHANGE ) {
		U32 index = ComboBox_GetCurSel(hControl);
		ConvertGUIDToString(&m_audioCaptureDeviceGuid[index], m_CurrentAudioCaptureDevice);
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnControlCommand( UINT wID, HWND hControl, UINT NotifyCode )
{
	m_fnHandleControlCommand(hControl, wID);
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnContextHelp(HWND hParentWnd, LPHELPINFO pHelpInfo) 
{

	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		DWORD wID = LOWORD(pHelpInfo->dwContextId);
		if (wID)
		{
			WinHelp(hParentWnd,"setup.hlp",HELP_CONTEXTPOPUP, wID + 0x10000);
		}
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::OnNotify(HWND hControl, WPARAM wParam, NMHDR * pNMHDR)
{
	U32 temp = PSN_KILLACTIVE;
	switch (pNMHDR->code)
	{
		case PSN_KILLACTIVE:
			return OnSelectDevice(pNMHDR->idFrom, hControl, pNMHDR->code);
		break;
		case PSN_RESET:
			return OnCancel(pNMHDR->idFrom, hControl, pNMHDR->code);
		break;
	}
	return S_OK;
}

//
HRESULT CDALibs_DeviceWindow::DoPropertySheet(HINSTANCE hInstance, HWND hwndOwner, DLGPROC dlgProc, LPARAM lParam)
{    
	PROPSHEETPAGE psp[2];
	PROPSHEETHEADER psh;
	char title[256];
	EBULoadString(GetResourceInst(),IDS_CONFIG_DIALOG_TITLE,title,256);

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE | PSP_PREMATURE ;
    psp[0].hInstance = hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_VIDEO_DEVICE);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = dlgProc;
    psp[0].pszTitle = "Video";
	psp[0].lParam = lParam;
    psp[0].pfnCallback = NULL;
	
	psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE | PSP_PREMATURE;
    psp[1].hInstance = hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_AUDIO_DEVICE);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = dlgProc;
    psp[1].pszTitle = "Audio";    
	psp[1].lParam = lParam;
    psp[1].pfnCallback = NULL;    
	
	psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USECALLBACK ;
    psh.hwndParent = hwndOwner;    
	psh.hInstance = hInstance;
    psh.pszIcon = NULL;
    psh.pszCaption = title;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);    
	psh.nStartPage = 0;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;    
	psh.pfnCallback = NULL;
    return PropertySheet(&psh);   
}

//
GENRESULT CDALibs_DeviceWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_DeviceWindow,CLSID_DACOMDESC>( "CDALibs_DeviceWindow", desc, (IDAComponent **) instance );
}

//
// 
//
void CDALibs_DeviceWindow::bltRenderBuffer(RPLOCKDATA &lock_data, S32 width, S32 height)
{
	// Get the information about the allocated bitmap.
	void *da_bits;

	da_bits = gDib.dsBm.bmBits;
	// There are three ways to copy the data here:
	// 1) Straight copy. Only valid if the dib's pixel format and the buffer's pixel format are
	//    the same, and the buffer's pitch matches that of the DIB
	// 2) Per-line copy. Only valid if the dib's pixel format and the buffer's pixel format match.
	// 3) Convertion copy. When nothing matches.


	bool colorMatch = false;
	bool pitchMatch = false;

	if
	(
		lock_data.pf.get_r_mask() == gDib.dsBitfields[0] && 
		lock_data.pf.get_g_mask() == gDib.dsBitfields[1] && 
		lock_data.pf.get_b_mask() == gDib.dsBitfields[2]
	)
	{
		colorMatch = true;
	}

	if ((U32) gDib.dsBm.bmWidthBytes == lock_data.pitch)
	{
		pitchMatch = true;
	}

	if (pitchMatch && colorMatch)
	{
		// Perform a single copy. Oh happy day.
		memcpy (lock_data.pixels, da_bits, lock_data.width * lock_data.height * sizeof(short));
	}
	else if (colorMatch)
	{
		// Perform a line-by-line copy.
		unsigned char* dest_row_start = (unsigned char*)lock_data.pixels;
		unsigned char* src_row_start = (unsigned char*)da_bits;
		for (int i = 0; i < height; ++i)
		{
			memcpy (dest_row_start, src_row_start, sizeof(short) * width);
			src_row_start += gDib.dsBm.bmWidthBytes;
			dest_row_start += lock_data.pitch;
		}
	}
	else
	{
		// Copy the scene into the DIB -- painfully slow
		unsigned char* dest_row_start = (unsigned char*)lock_data.pixels;  
		unsigned short* dest_row_start_t = (unsigned short*)dest_row_start; // dest is always 16 bit

		UINT bytesPerPixel = gDib.dsBmih.biBitCount/8;
		switch (gDib.dsBmih.biBitCount)
		{
			case 8:
			{
				unsigned char* src_row_start = (unsigned char*)da_bits;
				unsigned char* src_row_start_t = src_row_start;
				for (int i = 0; i < height; i++) 
				{
					for (int j = 0; j < width; j++) 
					{
						*dest_row_start_t++ = lock_data.pf.compute(gDibRgb[*src_row_start_t].rgbRed, gDibRgb[*src_row_start_t].rgbGreen, gDibRgb[*src_row_start_t].rgbBlue);
						src_row_start_t += bytesPerPixel;
					}
					dest_row_start += lock_data.pitch;
					src_row_start += gDib.dsBm.bmWidthBytes;

					dest_row_start_t = (unsigned short*)dest_row_start;
					src_row_start_t = src_row_start;
				}
			}
			break;
			case 24:
			{
				unsigned char* src_row_start = (unsigned char*)da_bits;
				unsigned char* src_row_start_t = src_row_start;
				for (int i = 0; i < height; i++) 
				{
					for (int j = 0; j < width; j++) 
					{
						*dest_row_start_t++ = lock_data.pf.compute(src_row_start_t[0], src_row_start_t[1], src_row_start_t[2]);
						src_row_start_t += bytesPerPixel;
					}
					dest_row_start += lock_data.pitch;
					src_row_start += gDib.dsBm.bmWidthBytes;

					dest_row_start_t = (unsigned short*)dest_row_start;
					src_row_start_t = src_row_start;
				}
			}
			break;
		}
	}
}

//
// Standard implementation of error dumper.
//

int __cdecl MY_STANDARD_DUMP (ErrorCode code, const C8 *fmt, ...)
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	char buffer[4096];
	va_list args;
	va_start (args, fmt);
	wvsprintf (buffer, fmt, args);
	va_end (args);
	OutputDebugString (buffer);

	// NOTE: Newlines are already added to trace severity.
	if (code.severity < SEV_TRACE_1)
	{	
		OutputDebugString ("\n");
	}

	// Kill the program in all SEV_FATAL or SEV_ERROR dumps.

	if (code.severity == SEV_FATAL)
	{
		switch (MessageBox(0, buffer, "DACOM::STANDARD_DUMP", MB_ABORTRETRYIGNORE|MB_ICONSTOP|MB_TOPMOST))
		{
		case IDABORT:
			abort();
			break;

		case IDRETRY:
			return 1;

		case IDIGNORE:
		default:
			break;
		}

	}
	
	return 0;
}

//
GENRESULT CDALibs_DeviceWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IWindow)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_ENTRY(instance, IModalWindow)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDeviceSelector)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDeviceConfigurator)
	DACOM_QUERYINTERFACE_ENTRY(instance, IDACOMClient)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_END(instance)
}

//
int CALLBACK PSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
	switch (uMsg)
	{
		case PSCB_PRECREATE:
			// remove the help box from the title bar
			((DLGTEMPLATE *) lParam)->style &= ~DS_CONTEXTHELP;
			break;
		default:
			break;
	}
	return 0;
}

//
char *ds_rp_abilities[] = 
{
	"",
	"Is 2D only",
	"Is 3D only",
	"Is DirectDraw Primary",
	"Supports gamma control",
	"Supports windowed rendering",
	"Amount of video memory",
	"Number of display modes",
	"Supports nonlocal video memory",
	"Supports square textures only",
	"Number of texture stages",
	"Maximum texture width",
	"Maximum texture height",
	"Number of texture formats",
	"Has separate texture memory",
	"Supports bilinear filtering",
	"Supports trilinear filtering",
	"Supports iterated alpha",
	"Supports table fog"
};

// EOF
