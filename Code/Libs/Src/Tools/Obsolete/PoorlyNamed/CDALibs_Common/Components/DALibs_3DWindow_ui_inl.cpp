// DALibs_3DWindow_ui_inl.cpp
//
//  This is code that should really go away when the UI hooks are in place.
//



//

struct LOCALMENUITEMINFO : public MENUITEMINFO
{
	LOCALMENUITEMINFO( U32 id = 0, LPVOID data=NULL )
	{
		cbSize = sizeof(*this);
		fMask = MIIM_DATA;
		fType = 0;
		fState = 0;
		wID = 0;
		hSubMenu = 0;
		hbmpChecked = 0;
		hbmpUnchecked = 0;
//		hbmpItem = 0;
		dwItemData = (U32)data;
		dwTypeData = 0;
		cch = 0;
	}
};


//

HRESULT AppendWindowOptionsList( HMENU hMenu, U32 key_flags )
{
	HMENU hSubMenu = CreatePopupMenu();
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_WINDOW_OPTION_WORLD_AXIS, "World A&xis" );
	
	AppendMenu( hMenu, MF_POPUP, (U32)hSubMenu,	"&Options" );
	return S_OK;
}

//

HRESULT AppendCameraList( HMENU hMenu, U32 key_flags )
{

	U32 cam_num = 0;
	COMPTR<ILowLevelCamera> ILLC;

	DB_FindCameras();
	while( SUCCEEDED( DB_NextCamera( ILLC ) ) ) {
		char name[255+1];
		DB_GetCameraName( ILLC, name, 255 );
		
		U32 wId = ID_3DWINDOW_CAMERA_ID+cam_num;
		AppendMenu( hMenu, MF_STRING, wId, name );
		if( ILLC == m_CurrentCamera ) {
			CheckMenuItem( hMenu, wId, MF_BYCOMMAND|MF_CHECKED );
		}

		cam_num++;
	}
	return S_OK;
}

//

HRESULT AppendDefaultCameraContext( HMENU hMenu, U32 key_flags )
{
	HMENU hSubMenu = CreatePopupMenu();
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_APEX,			"Apex" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_APEX_CLOSEUP,	"Apex Closeup" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_EXTERNAL,		"External" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_INTERNAL,		"Internal" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_TRACK,			"Track" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_PAN,			"Pan" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_FOLLOW,			"Follow" );
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_CAMERA_FIXED,			"Fixed..." );
	if( SUCCEEDED( DB_FindCameras() ) ) {
		AppendMenu( hSubMenu, MF_SEPARATOR, 0, NULL );
		AppendCameraList( hSubMenu, key_flags );
	}

	AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
	AppendMenu( hMenu, MF_POPUP, (U32)hSubMenu, "&Cameras" );
	return S_OK;
}

//

HRESULT AppendObjectOptionsList( HMENU hMenu, U32 key_flags )
{	
	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_OPTION_AXIS,			"&Axis" );
	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_OPTION_NAMES,			"&Names" );
	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_OPTION_FACE_NORMALS,	"&Vertex Normals" );
	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_OPTION_VERTEX_NORMALS,	"&Face Normals" );
	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_OPTION_HARDPOINTS,		"&Hardpoints" );
	return S_OK;
}

//

HRESULT AppendLightList( HMENU hMenu, U32 key_flags )
{
	U32 light_num = 0;
	COMPTR<ILight> ILLC;

	DB_FindLights();
	while( SUCCEEDED( DB_NextLight( ILLC ) ) ) {
		char name[255+1];
		DB_GetLightName( ILLC, name, 255 );
		
		U32 wId = ID_3DWINDOW_LIGHT_ID+light_num;
		
		LOCALMENUITEMINFO info( wId, (void*)ILLC.ptr );
		ILLC->AddRef();

		AppendMenu( hMenu, MF_STRING, wId, name );
		SetMenuItemInfo( hMenu, wId, FALSE, &info );
		if( ILLC == m_CurrentLight ) {
			CheckMenuItem( hMenu, wId, MF_BYCOMMAND|MF_CHECKED );
		}
		light_num++;
	}
	return S_OK;
}

//

HRESULT AppendDefaultLightContext( HMENU hMenu, U32 key_flags )
{
	HMENU hSubMenu = CreatePopupMenu();
	AppendMenu( hSubMenu, MF_STRING, ID_3DWINDOW_LIGHT_AMBIENT,	"&Ambient..." );
	if( SUCCEEDED( DB_FindLights() ) ) {
		AppendMenu( hSubMenu, MF_SEPARATOR, 0, NULL );
		AppendLightList( hSubMenu, key_flags );
	}

	AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
	AppendMenu( hMenu, MF_POPUP, (U32)hSubMenu,	"&Lights" );
	return S_OK;
}

//

HRESULT BuildContextMenu( U32 x, U32 y, U32 key_flags )
{
	HMENU hMenu;
	hMenu = CreatePopupMenu();

	AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_WINDOW_INSERT,	"&Insert..." );
	AppendWindowOptionsList( hMenu, key_flags );
	AppendDefaultCameraContext( hMenu, key_flags );
	AppendDefaultLightContext( hMenu, key_flags );

	if( m_CurrentSelection != NULL ) {
		HMENU hSubMenu = CreatePopupMenu();
		AppendObjectOptionsList( hSubMenu, key_flags );

		AppendMenu( hMenu, MF_SEPARATOR, 0, NULL );
		AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_CUT,	"Cu&t" );
		AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_COPY,	"&Copy" );
		AppendMenu( hMenu, MF_STRING, ID_3DWINDOW_OBJECT_PASTE,	"&Paste" );
		AppendMenu( hMenu, MF_POPUP, (U32)hSubMenu,				"&Options" );
	}


//	BuildDefaultLightContext( hMenu, key_flags );
	TrackPopupMenu( hMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RIGHTBUTTON, x, y, 0, m_hWnd, NULL );
	return S_OK;
}


