//
//  Setup_deviceSelector.cpp
//		This file contains the methods which read/write the ini file(s) used by the app.
//

//#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <comdef.h>
//#include <afxres.h>
#include <Zmouse.h>
#include <stdio.h>

//#include "resource.h"

#include "dacom.h"
#include "TSmartPointer.h"
#include "system.h"
#include "engine.h"
#include "IProfileParser.h"
#include "RPUL.h"

#include "DACOMProvider.h"
#include "IWindow.h"
#include "IDeviceSelector.h"
#include "IDeviceConfigurator.h"
#include "IDACOMClient.h"

#include "hotsetup.h"
#include "deviceselection.h"
#include "app_configuration.h"

#pragma warning(disable:4800) // unsigned long: forcing value to bool

const bool USE_DEBUG_OPTIONS = true;
bool useDefaults;
 
BOOL CALLBACK Validate(IDAComponent *Win, HWND hCB, UINT resolution, DS_VALIDATE_CALLBACK_TYPES validationType);

BOOL ValidateResolution(IDAComponent *Win, HWND hCB, UINT resolution);
BOOL SetDefaultValues(IDAComponent *Win, HWND hWnd, UINT resolution);

BOOL CALLBACK HandleCommand(HWND hCB, UINT wID);

HRESULT OnInitDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
BOOL CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);
void ProcessSelections(IDAComponent *Win, LPSTR iniFileName, bool useDefaultSettings);
void LoadCurrentSelections(IDAComponent *Win, LPSTR iniFileName);
void SetButtonStates(HWND hWnd, U32 showState);
char ini_file[255+1];

BOOL CALLBACK Validate(IDAComponent *Win, HWND hCB, UINT resolution, DS_VALIDATE_CALLBACK_TYPES validationType)
{
	switch (validationType)
	{
		case DS_VALIDATE_RESOLUTION:
			return ValidateResolution(Win, hCB, resolution);
		break;

		case DS_VALIDATE_SELECTION:
			return SetDefaultValues(Win, hCB, resolution);
		break;
	}
	return FALSE;
}

// Validate
//	Callback from the property sheet before a resolution is added to the combo box.  If this
//	function fails, the resolution is not listed and therefore not available.
//
BOOL ValidateResolution(IDAComponent *Win, HWND hCB, UINT resolution)
{
	COMPTR<IDeviceSelector> IDS;
	if( FAILED( Win->QueryInterface( IID_IDeviceSelector, (void**) &IDS ) ) ) {
		return FALSE;
	}	
	
	char device[255+1], dc[64+1], did[64+1];
	// add available and supported resolutions to combo
	IDS->GetSelectedDevice( device, 255 );
	GetDeviceClassAndId( device, dc, did );
	if( !strncmp( dc, "Raster", 6 ) )
	{	// software only - so only allow 640x480	
		return (V_RES_640 == resolution);
	}
	else
	{
		COMPTR<IDeviceConfigurator> IDConfig;
		if( FAILED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
			return FALSE;
		}	
		U32 memAvailable, memNeeded = ds_resolution_X[resolution] * ds_resolution_Y[resolution] * ds_app_mem_multiplier;
		IDConfig->GetAbility(&memAvailable, RP_A_DEVICE_MEMORY); 
		if ((memAvailable) >= memNeeded) 
			return (ds_app_supported_resolutions[resolution] );
	}
	return FALSE;
}

// SetDefaultValues
//	Sets the default values of the options and sets the default resolution based on the selected device.  This
//	function is called by the property sheet whenever a device is selected.
//	
BOOL SetDefaultValues(IDAComponent *Win, HWND hWnd, UINT resolution)
{
	COMPTR<IDeviceSelector> IDS;
	if( FAILED( Win->QueryInterface( IID_IDeviceSelector, (void**) &IDS ) ) ) {
		return FALSE;
	}	
	COMPTR<IDeviceConfigurator> IDConfig;
	if( FAILED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
		return FALSE;
	}	
	RPDEVICEINFO deviceInfo;
	IDConfig->GetInfo(&deviceInfo); 

	char device[255+1], dc[64+1], did[64+1];
	// add available and supported resolutions to combo
	IDS->GetSelectedDevice( device, 255 );
	GetDeviceClassAndId( device, dc, did );
	if( !strncmp( dc, "Raster", 6 ) )
	{	// software renderer - so set res to 640x480	
		IDConfig->SetSelectedResolution( V_RES_640 );
		Configurable_Options[OPTION_LOCKING][DEFAULT_STATE] = TRUE;
		Configurable_Options[OPTION_D3DTEXTURE][DEFAULT_STATE] = FALSE;
		Configurable_Options[OPTION_HW_RENDERER][DEFAULT_STATE] = FALSE;
		Configurable_Options[OPTION_GDI][DEFAULT_STATE] = TRUE;
		Configurable_Options[OPTION_HW_CURSOR][DEFAULT_STATE] = TRUE;
		for( U32 A=FIRST_CONFIGURABLE_OPTION; A<LAST_CONFIGURABLE_OPTION; A++ )
		{
			Configurable_Options[A][CURRENT_STATE] = Configurable_Options[A][DEFAULT_STATE];
		}
	}
	else
	{
		IDConfig->SetSelectedResolution( V_RES_640 );
		switch (deviceInfo.device_chipset_id)
		{
			case RP_D_VOODOO_1:
			case RP_D_VOODOO_RUSH:
				Configurable_Options[OPTION_D3DTEXTURE][DEFAULT_STATE] = FALSE;
			break;
			default:
				Configurable_Options[OPTION_D3DTEXTURE][DEFAULT_STATE] = TRUE;
			break;
		}
		Configurable_Options[OPTION_LOCKING][DEFAULT_STATE] = TRUE;
		Configurable_Options[OPTION_GDI][DEFAULT_STATE] = USE_DEBUG_OPTIONS;
		Configurable_Options[OPTION_HW_RENDERER][DEFAULT_STATE] = TRUE;
		Configurable_Options[OPTION_HW_CURSOR][DEFAULT_STATE] = TRUE;
		for( U32 A=FIRST_CONFIGURABLE_OPTION; A<LAST_CONFIGURABLE_OPTION; A++ )
		{
			Configurable_Options[A][CURRENT_STATE] = Configurable_Options[A][DEFAULT_STATE];
		}

		// if the options have already been saved, get the values
		LoadCurrentSelections(Win, ini_file);
	}
	// update the buttons - some may not exist yet if they are advanced options
//	SetButtonStates(hWnd, DEFAULT_STATE);
	return TRUE;
}

// HandleCommand
//	Processes the commands from the property sheet and the "advanced" dialog
//
BOOL CALLBACK HandleCommand(HWND hCB, UINT wID)
{
	U32 A;
	HWND parentWnd = GetParent(hCB);
	switch (wID)
	{
		case IDC_ADVANCED_VIDEO:
			return SUCCEEDED(DialogBox( GetWindowInstance(parentWnd), MAKEINTRESOURCE(IDD_ADVANCED_VIDEO_DIALOG), parentWnd, HandleMessage ));
		break;

		case IDOK:
			// ok was selected so save states of all controls
			for( A=FIRST_CONFIGURABLE_OPTION; A<LAST_CONFIGURABLE_OPTION; A++ )
			{
				HWND hControl = GetDlgItem(parentWnd, Configurable_Options[A][CONTROL_ID]);
				if (hControl)
					Configurable_Options[A][CURRENT_STATE] = Button_GetState(hControl);
			}
			return TRUE;
		break;

		case IDC_RESET:
		{
			SetButtonStates(parentWnd, DEFAULT_STATE);
		}
		break;

		default:
			// code used before advanced button was added - still usable for any controls on the property sheet that need to be handled uniquely for the game
			for( A=FIRST_CONFIGURABLE_OPTION; A<LAST_CONFIGURABLE_OPTION; A++ )
			{
				if (Configurable_Options[A][CONTROL_ID] == wID)
				{
					Configurable_Options[A][CURRENT_STATE] = Button_GetState(hCB);
					return TRUE;
				}
			}
		break;
	}
	return FALSE;
}

HRESULT OnInitDialog( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	SetButtonStates(hWnd, CURRENT_STATE);
	ShowWindow(hWnd, TRUE);
	return S_OK;
}

BOOL CALLBACK HandleMessage(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			OnInitDialog(hWnd,message,wParam,lParam);
		case WM_COMMAND:
			switch( LOWORD(wParam) ) 
			{
				case IDCANCEL:
					return SUCCEEDED(EndDialog( hWnd, -1 ));
				break;
				case IDOK:
					HandleCommand( (HWND)lParam, LOWORD(wParam) );
					return SUCCEEDED(EndDialog( hWnd, -1 ));
				break;
				case IDC_RESET:
					return HandleCommand( (HWND)lParam, LOWORD(wParam) );
				break;
			}
		break;
		case WM_HELP:
		{
			LPHELPINFO pHelpInfo = (LPHELPINFO) lParam;
			if (pHelpInfo->iContextType == HELPINFO_WINDOW)
			{
				DWORD wID = LOWORD(pHelpInfo->dwContextId);
				if (wID)
				{
					WinHelp(hWnd,"setup.hlp",HELP_CONTEXTPOPUP, wID + 0x10000 );
					return TRUE;
				}
			}
		}
		break;
	}
	return FALSE;
}

// ProcessSelections
//	Writes the options to the ini file in the appropriate format that the app expects
//
void ProcessSelections(IDAComponent *Win, LPSTR iniFileName, bool useDefaultSettings)
{
	COMPTR<IDeviceConfigurator> IDConfig;
	if( FAILED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
		return;
	}	

	COMPTR<IDeviceSelector> IDS;
	if( FAILED( Win->QueryInterface( IID_IDeviceSelector, (void**) &IDS ) ) ) {
		return;
	}	

	// write IRenderPipeline device
	char deviceString[128+1], device[256+1], device_class[64+1], device_id[64+1];
	IDS->GetSelectedDevice( device, 255 );
	GetDeviceClassAndId( device, device_class, device_id );
	sprintf(deviceString,"{%s,%s}",device_class, device_id );
	WritePrivateProfileString("System", "IRenderPipeline", deviceString, iniFileName);

	// write resolution
	U32 selectedResolution;
	IDConfig->GetSelectedResolution(&selectedResolution);
	WritePrivateProfileString("Video", "Resolution", ds_video_resolutions[selectedResolution], iniFileName);

	U32 setting = useDefaultSettings ? DEFAULT_STATE : CURRENT_STATE;
	// write other options
	WritePrivateProfileString("Video", "Locking", (Configurable_Options[OPTION_LOCKING][setting]?"on":"off"), iniFileName);
	WritePrivateProfileString("Video", "D3DTexture", (Configurable_Options[OPTION_D3DTEXTURE][setting]?"on":"off"), iniFileName);
	WritePrivateProfileString("RenderOptions", "HardwareRender", (Configurable_Options[OPTION_HW_RENDERER][setting]?"on":"off"), iniFileName);
	WritePrivateProfileString("Video", "GDI", (Configurable_Options[OPTION_GDI][setting]?"on":"off"), iniFileName);
	WritePrivateProfileString("Video", "HardwareCursor", (Configurable_Options[OPTION_HW_CURSOR][setting]?"on":"off"), iniFileName);

	// write the audio section
	IDConfig->GetAudioDevice( device_id, 65 );
	WritePrivateProfileString(ini_sound_section, ini_playback_name, device_id, iniFileName);
	IDConfig->GetAudioCaptureDevice( device_id, 65 );
	WritePrivateProfileString(ini_sound_section, ini_capture_name, device_id, iniFileName);
	WritePrivateProfileString(ini_sound_section, "Voxware", "35243410-F7340C0668-CD78867B74DAD857-AC71429AD8CAFCB5-E4E1A99E7FFD-371", iniFileName);

	// flush the buffer
	WritePrivateProfileString(NULL, NULL, NULL, iniFileName);
}

bool validateIniFile(LPSTR iniFileName)
{
	const int stringSize = 32;
	char iniString[stringSize] = "does not exist";
	bool validIniFile = true;

	GetPrivateProfileString("Video", "Resolution", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") == 0)
		validIniFile = false;

	// get other options from ini file and if they were previously saved, set the state of the option
	GetPrivateProfileString("Video", "Locking", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") == 0)
		validIniFile = false;

	GetPrivateProfileString("Video", "D3DTexture", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") == 0)
		validIniFile = false;

	GetPrivateProfileString("Video", "GDI", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") == 0)
		validIniFile = false;

	GetPrivateProfileString("Video", "HardwareCursor", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") == 0)
		validIniFile = false;

	return validIniFile;
}

// LoadCurrentSelections
//	Gets the options from the ini file in the appropriate format that the app expects and sets the state of the option
//
void LoadCurrentSelections(IDAComponent *Win, LPSTR iniFileName)
{
	const int stringSize = 65;
	char iniString[stringSize] = "does not exist";

	useDefaults = false; // if this is true, something is wrong with ini file, so when we write the file, we'll use default settings if the user cancels

	// get resolution
	GetPrivateProfileString("Video", "Resolution", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
		for (U32 resolution = V_RES_FIRST_RES; resolution<V_RES_LAST_RES; resolution++)
		{
			if ( (!strcmp(iniString, ds_video_resolutions[resolution])) && (ValidateResolution(Win, 0, resolution)) )
			{	// found a saved valid resolution, so set the resolution to the saved one
				COMPTR<IDeviceConfigurator> IDConfig;
				if( FAILED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
					return;
				}	
				IDConfig->SetSelectedResolution(resolution);
				break;
			}
		}
	else
		useDefaults = true;

	// get other options from ini file and if they were previously saved, set the state of the option
	GetPrivateProfileString("Video", "Locking", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
		Configurable_Options[OPTION_LOCKING][CURRENT_STATE] = (strcmp(iniString,"on") == 0);
	else
		useDefaults = true;

	GetPrivateProfileString("Video", "D3DTexture", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
		Configurable_Options[OPTION_D3DTEXTURE][CURRENT_STATE] = (strcmp(iniString,"on") == 0);
	else
		useDefaults = true;

	GetPrivateProfileString("Video", "GDI", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
		Configurable_Options[OPTION_GDI][CURRENT_STATE] = (strcmp(iniString,"on") == 0);
	else
		useDefaults = true;

	GetPrivateProfileString("Video", "HardwareCursor", "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
		Configurable_Options[OPTION_HW_CURSOR][CURRENT_STATE] = (strcmp(iniString,"on") == 0);
	else
		useDefaults = true;

	GetPrivateProfileString(ini_sound_section, ini_playback_name, "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
	{
		COMPTR<IDeviceConfigurator> IDConfig;
		if( SUCCEEDED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
			IDConfig->SetAudioDevice(iniString);
		}	
	}

	GetPrivateProfileString(ini_sound_section, ini_capture_name, "does not exist", iniString, stringSize, iniFileName);
	if (strcmp(iniString,"does not exist") != 0)
	{
		COMPTR<IDeviceConfigurator> IDConfig;
		if( SUCCEEDED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
			IDConfig->SetAudioCaptureDevice(iniString);
		}	
	}
}

// SetButtonStates
//	Sets the state and enables/disables buttons based on the current options
//
void SetButtonStates(HWND hWnd, U32 showState)
{
	CheckDlgButton(hWnd,IDC_LOCKING,Configurable_Options[OPTION_LOCKING][showState]);
	EnableWindow(GetDlgItem( hWnd, IDC_LOCKING ), Configurable_Options[OPTION_HW_RENDERER][showState]);

	CheckDlgButton(hWnd,IDC_D3DTEXTURE,Configurable_Options[OPTION_D3DTEXTURE][showState]);
	EnableWindow(GetDlgItem( hWnd, IDC_D3DTEXTURE ), Configurable_Options[OPTION_HW_RENDERER][showState]);

// only show debugging buttons in debug build
	if (USE_DEBUG_OPTIONS)
	{
		CheckDlgButton(hWnd,IDC_GDI,Configurable_Options[OPTION_GDI][showState]);
		EnableWindow(GetDlgItem( hWnd, IDC_GDI ), Configurable_Options[OPTION_HW_RENDERER][showState]);
		CheckDlgButton(hWnd,IDC_HW_CURSOR,Configurable_Options[OPTION_HW_CURSOR][showState]);
		EnableWindow(GetDlgItem( hWnd, IDC_HW_CURSOR ), Configurable_Options[OPTION_HW_RENDERER][showState]);
	}
	else
	{
		EnableWindow(GetDlgItem( hWnd, IDC_GDI ), FALSE);
		ShowWindow(GetDlgItem( hWnd, IDC_GDI ), FALSE);
		EnableWindow(GetDlgItem( hWnd, IDC_HW_CURSOR ), FALSE);
		ShowWindow(GetDlgItem( hWnd, IDC_HW_CURSOR ), FALSE);
	}
}

bool GetDeviceSelection(HWND hwnd, HINSTANCE hinst, LPSTR iniFileName, LPSTR szFlags, bool forceConfigureWindow)
{
	char device[255+1], dc[64+1], did[64+1];
	bool canceledDialog = false;
	char title[256];
	using namespace NGLOBALS;
	EBULoadString(hinst,IDS_CONFIG_DIALOG_TITLE,title,256);

    U32 flags = supported_renderers; // can still be modified by flags passed in szFlags

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

	if ( !forceConfigureWindow && (validateIniFile(ini_file)) )
	{ // don's show configure window, just run the app
		return TRUE;	
	}


	CDACOMProvider *m_DACOM = new CDACOMProvider();
	if( FAILED ( m_DACOM->Initialize( "DASetup.ini" ) ) ) {
		return FALSE;
	}	
	
	extern HRESULT RegisterCDALibs_DeviceWindow( ICOManager *dacom );
	RegisterCDALibs_DeviceWindow( m_DACOM->Manager );

	COMPTR<IDAComponent> Win;
	CLSID_DACOMDESC desc( "CDALibs_DeviceWindow" );
	if( FAILED( m_DACOM->Manager->CreateInstance( &desc, (void**) &Win ) ) ) {
		return FALSE;
	}

	COMPTR<IDACOMClient> IDC;
	if( SUCCEEDED( Win->QueryInterface( IID_IDACOMClient, (void**) &IDC ) ) ) {
		IDC->SetICOManager( m_DACOM->Manager);
		IDC->SetISystem( m_DACOM->System );
		IDC->SetIEngine( m_DACOM->Engine );
	}

	COMPTR<IDeviceSelector> IDS;
	if( FAILED( Win->QueryInterface( IID_IDeviceSelector, (void**) &IDS ) ) ) {
		return FALSE;
	}	

	IDS->SetAvailableDeviceClasses( flags );

	GetDeviceInfoInFile( ini_file, dc, did );
	CreateDeviceDescription( dc, did, device );
	IDS->SetSelectedDevice( device );

	COMPTR<IDeviceConfigurator> IDConfig;
	if( FAILED( Win->QueryInterface( IID_IDeviceConfigurator, (void**) &IDConfig ) ) ) {
		return FALSE;
	}	

	IDConfig->SetValidateCB( Validate );
	IDConfig->SetHandleControlCommandCB( HandleCommand );
	
	COMPTR<IModalWindow> IWin;
	if( SUCCEEDED( Win->QueryInterface( IID_IModalWindow, (void**) &IWin ) ) ) {
		IWin->SetTitle( title );
		if( FAILED( IWin->DoModal( hwnd, NULL ) ) ) {
			if (useDefaults)
				canceledDialog = true; // user hit cancel before ever saving, so go ahead and save default settings
			else
				return FALSE;  // have saved settings and user canceled, so just exit
		}
	}
	
	// Get results and write device to ini file
	ProcessSelections(Win, iniFileName, (canceledDialog && useDefaults));
	return !forceConfigureWindow; // if we didn't force configuration, go ahead and launch the exe (return true)
}

