// DALibs_3DWindow.cpp: implementation of the CDALibs_3DWindow class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <stdio.h>

#include "dacom.h"
#include "davariant.h"
#include "TSmartPointer.h"
#include "system.h"
#include "lightman.h"
#include "ILight.h"
#include "ICamera.h"
#include "RPUL.h"
#include "stddat.h"
#include "IRenderPrimitive.h"
#include "model.h"
#include "vfx.h"

#include "resource.h"
#include "DPF.h"
#include "DACOM_Utility.h"
#include "MessageCrackers.h"
#include "CommonDialog.h"
#include "TCollection.h"
#include "options_inl.cpp"

#include "IWindow.h"
#include "ILowLevelCamera.h"
#include "ISceneCamera.h"
#include "IRenderable.h"
#include "IPersistable.h"
#include "IDACOMEngineInstance.h"
#include "IGeoTransformable.h"

/*
#include "IObjectDatabase.h"
*/

//

#define ID_3DWINDOW_START							50000

#define ID_3DWINDOW_WINDOW_START					(ID_3DWINDOW_START+10)
#define ID_3DWINDOW_WINDOW_INSERT					(ID_3DWINDOW_WINDOW_START+1)
#define ID_3DWINDOW_WINDOW_OPTION_WORLD_AXIS		(ID_3DWINDOW_WINDOW_START+3)

#define ID_3DWINDOW_OBJECT_START					(ID_3DWINDOW_START+20)
#define ID_3DWINDOW_OBJECT_CUT						(ID_3DWINDOW_OBJECT_START+1)
#define ID_3DWINDOW_OBJECT_COPY						(ID_3DWINDOW_OBJECT_START+2)
#define ID_3DWINDOW_OBJECT_PASTE					(ID_3DWINDOW_OBJECT_START+3)
#define ID_3DWINDOW_OBJECT_OPTION_AXIS				(ID_3DWINDOW_OBJECT_START+4)
#define ID_3DWINDOW_OBJECT_OPTION_NAMES				(ID_3DWINDOW_OBJECT_START+5)
#define ID_3DWINDOW_OBJECT_OPTION_FACE_NORMALS		(ID_3DWINDOW_OBJECT_START+6)
#define ID_3DWINDOW_OBJECT_OPTION_VERTEX_NORMALS	(ID_3DWINDOW_OBJECT_START+7)
#define ID_3DWINDOW_OBJECT_OPTION_HARDPOINTS		(ID_3DWINDOW_OBJECT_START+8)

#define ID_3DWINDOW_CAMERA_START					(ID_3DWINDOW_START+30)
#define ID_3DWINDOW_CAMERA_SHOT_START				(ID_3DWINDOW_CAMERA_START+0)
#define ID_3DWINDOW_CAMERA_APEX						(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_APEX)
#define ID_3DWINDOW_CAMERA_APEX_CLOSEUP				(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_APEX_CLOSEUP)
#define ID_3DWINDOW_CAMERA_EXTERNAL					(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_EXTERNAL)
#define ID_3DWINDOW_CAMERA_INTERNAL					(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_INTERNAL)
#define ID_3DWINDOW_CAMERA_TRACK					(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_TRACK)
#define ID_3DWINDOW_CAMERA_PAN						(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_PAN)
#define ID_3DWINDOW_CAMERA_FOLLOW					(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_FOLLOW)
#define ID_3DWINDOW_CAMERA_FIXED					(ID_3DWINDOW_CAMERA_SHOT_START+ISC_ST_FIXED)
#define ID_3DWINDOW_CAMERA_ID						(ID_3DWINDOW_CAMERA_START+15)

#define ID_3DWINDOW_LIGHT_START						(ID_3DWINDOW_START+40)
#define ID_3DWINDOW_LIGHT_AMBIENT					(ID_3DWINDOW_LIGHT_START+1)
#define ID_3DWINDOW_LIGHT_ID						(ID_3DWINDOW_LIGHT_START+15)

//

#define _R(clr) (((clr)>>16) & 0xFF)
#define _G(clr) (((clr)>>8)  & 0xFF)
#define _B(clr) (((clr)>>0)  & 0xFF)


//

#define DACOM_COMPONENT_NAME CDALibs_3DWindow

const char *CLSID_CDALibs_3DWindow = "CDALibs_3DWindow";

dacom_component CDALibs_3DWindow :	dacom_implements IWindow,
//									dacom_implements IDataConsumer,
									dacom_implements IComponentFactory
{
	// IWindow
	DACOM_INTERFACE_METHOD_DECL( Create,		( IWINDOWCREATEDATA *create_data, HWND *out_hChild ));
	DACOM_INTERFACE_METHOD_DECL( Destroy,		( void ));
	DACOM_INTERFACE_METHOD_DECL( GetHandle,		( HWND *out_hChild ));

	// IComponentFactory && IDAComponent
	DEFMETHOD(CreateInstance) (DACOMDESC *descriptor, void **instance);
	DEFMETHOD(QueryInterface) (const C8 *interface_name, void **instance);
	DEFMETHOD_(U32,AddRef)    (void);
	DEFMETHOD_(U32,Release)   (void);
	

public:		// Interface
	CDALibs_3DWindow( CLSID_DACOMDESC &creation_info );
	~CDALibs_3DWindow();

protected:	// Private Data

	// Currently selected camera, light, renderable
	//
	COMPTR<ILight>			m_CurrentLight;
	COMPTR<IRenderable>		m_CurrentSelection;
	COMPTR<ILowLevelCamera> m_CurrentCamera;

	// camera used when the currently active shot is not
	// exactly the same as a canned camera
	//
	COMPTR<ILowLevelCamera> m_InternalCamera;

	// Properties
	//
	float   m_DeltaAngle;
	float	m_DeltaTranslate;
	U32		m_ViewWorldAxis;	
	U32		m_AmbientLight;

	// cached stuff
	//
	RPFont	m_Font;
	HWND	m_hWnd;
	RECT	m_ClientRect;
	int		m_RefCnt;

	COMPTR<ICOManager>				m_ICOManager;
	COMPTR<ISystemContainer>		m_ISystem;
	COMPTR<IEngine>					m_IEngine;
	COMPTR<IRenderPipeline>			m_IRenderPipe;
	COMPTR<IRenderPrimitive>		m_IRenderPrimitive;
	
protected:	// Interface

	// Object Management
	//
	HRESULT InsertNewItem( const char *object_name, const char *content_type );
	HRESULT InsertNewItems( char *string, const char *content_type );

	HRESULT Refresh( void );

	// Interaction (IA) handers
	//
	HRESULT HandleSelectionIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags );
	HRESULT HandleObjectIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags );
	HRESULT HandleLightIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags );
	HRESULT HandleCameraIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags );

protected:
#include "DALibs_3DWindow_db_inl.cpp"
#include "DALibs_3DWindow_ui_inl.cpp"
	
public:

	BEGIN_STATIC_WP_MAPS(CDALibs_3DWindow)
		BEGIN_COMMAND_MAP
			ON_COMMAND(ID_3DWINDOW_WINDOW_INSERT,OnInsert)
			ON_COMMAND(ID_3DWINDOW_WINDOW_OPTION_WORLD_AXIS,OnWindowOptionWorldAxis)
			ON_COMMAND(ID_3DWINDOW_CAMERA_APEX,OnCameraCannedShot)			
			ON_COMMAND(ID_3DWINDOW_CAMERA_APEX_CLOSEUP,OnCameraCannedShot)	
			ON_COMMAND(ID_3DWINDOW_CAMERA_EXTERNAL,OnCameraCannedShot)		
			ON_COMMAND(ID_3DWINDOW_CAMERA_INTERNAL,OnCameraCannedShot)		
			ON_COMMAND(ID_3DWINDOW_CAMERA_TRACK,OnCameraCannedShot)			
			ON_COMMAND(ID_3DWINDOW_CAMERA_PAN,OnCameraCannedShot)			
			ON_COMMAND(ID_3DWINDOW_CAMERA_FOLLOW,OnCameraCannedShot)		
			ON_COMMAND(ID_3DWINDOW_CAMERA_FIXED,OnCameraCannedShot)	
			ON_COMMAND(ID_3DWINDOW_LIGHT_AMBIENT,OnLightAmbient)
			ON_COMMAND_DEFAULT(OnCameraCannedCamera)			
		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_KEYDOWN,HandleIA)
			ON_MESSAGE(WM_KEYUP,HandleIA)
			ON_MESSAGE(WM_MOUSEMOVE,HandleIA)
			ON_MESSAGE(WM_LBUTTONDOWN,HandleIA)
			ON_MESSAGE(WM_LBUTTONUP,HandleIA)
			ON_MESSAGE(WM_PAINT,OnPaint)
			ON_MESSAGE(WM_RBUTTONUP,OnPopupContextMenu)
		END_MESSAGE_MAP
	END_STATIC_WP_MAPS

	DEF_ON_COMMAND(OnInsert)
	DEF_ON_COMMAND(OnCameraCannedShot)
	DEF_ON_COMMAND(OnCameraCannedCamera)
	DEF_ON_COMMAND(OnLightAmbient)
	DEF_ON_COMMAND(OnWindowOptionWorldAxis)
	DEF_ON_MESSAGE(OnPaint)
	DEF_ON_MESSAGE(HandleIA)
	DEF_ON_MESSAGE(OnPopupContextMenu)
};

//

DACOM_INTERFACE_METHOD_IMPL( Create,( IWINDOWCREATEDATA *cd, HWND *out_hChild ))
{
	if( m_hWnd ) {
		Destroy();
	}

	if( out_hChild ) {
		*out_hChild =  NULL;
	}

	if( m_ISystem == NULL || m_IEngine == NULL || m_ICOManager == NULL ) {
		GENERAL_WARNING( "ISystem or IEngine is null, be sure to initialize _ISystem and _IEngine of the CLSID_DACOMDESC" );
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPipeline, (void**) &m_IRenderPipe ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPrimitive, (void**) &m_IRenderPrimitive ) ) ) {
		return E_FAIL;
	}

	WNDCLASS wc = {
		CS_HREDRAW|CS_VREDRAW,
		CDALibs_3DWindow::HandleMessage,
		0,
		0,
		cd->hModuleInstance,
		NULL,
		NULL,
		(HBRUSH)GetStockObject( BLACK_BRUSH ),
		NULL,
		CLSID_CDALibs_3DWindow
	};

	U32 val = (U32)RegisterClass( &wc );

	U32 style = cd->uWindowStyle | 
				WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS |
				0;
	
	RECT rect = cd->rChildRect;

	AdjustWindowRect( &rect, style, FALSE );
	if( rect.top < 0 || rect.left < 0 ) {
		rect.right	+= -rect.left;
		rect.left   += -rect.left;
		rect.bottom += -rect.top;
		rect.top	+= -rect.top;
	}

	m_hWnd = CreateWindowEx( 0, CLSID_CDALibs_3DWindow, CLSID_CDALibs_3DWindow, style,
							 rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 
							 cd->hParent, 
							 NULL, 
							 cd->hResourceInstance, 
							 NULL );

	if( m_hWnd == NULL ) {
		return E_FAIL;
	}

	GetClientRect( m_hWnd, &m_ClientRect );
	SetWindowLong( m_hWnd, GWL_USERDATA, (long)this );

	// Setup the font
	//
	m_Font.Initialize( "default_font" );
	m_Font.SetRenderPipeline( m_IRenderPipe );

	// Setup properties
	//

	opt_get_u32( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "ViewWorldAxis", 1, &m_ViewWorldAxis );
	opt_get_float( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "DeltaAngle", 10.0, &m_DeltaAngle );
	opt_get_float( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "DeltaTranslate", 1.0, &m_DeltaTranslate );

	Vector rgb;
	U32 r, g, b;
	opt_get_vector( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "AmbientLight", Vector(255,255,255), rgb );
	r = rgb.x;	g = rgb.y;	b = rgb.z;
	m_AmbientLight = (r<<16)|(g<<8)|(b<<0);


	DB_Initialize();

	m_CurrentSelection = NULL;
	m_CurrentCamera = NULL;
	m_CurrentLight = NULL;

	char string[1024+1];
	
	InsertNewItem( "DefaultCamera", "CDALibs_Camera" );
	
	opt_get_string( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "Cameras", "", string, 1024 );
	InsertNewItems( string, "CDALibs_Camera" );

	opt_get_string( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "Camera", "DefaultCamera", string, 1024 );
	if( FAILED( DB_FindCameraByName( string, m_CurrentCamera ) ) ) {
		GENERAL_WARNING( "Unable to set current camera" );
		m_CurrentCamera = NULL;
		return E_FAIL;
	}

	opt_get_string( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "Lights", "", string, 1024 );
	InsertNewItems( string, "CDALibs_Light" );
	opt_get_string( m_ICOManager, NULL, CLSID_CDALibs_3DWindow, "Light", "DefaultLight", string, 1024 );
	if( FAILED( DB_FindLightByName( string, m_CurrentLight ) ) ) {
		GENERAL_WARNING( "Unable to set current light" );
		m_CurrentLight = NULL;
	}

	ShowWindow( m_hWnd, SW_SHOWNORMAL );
	UpdateWindow( m_hWnd );

	if( out_hChild ) {
		*out_hChild = m_hWnd;
	}
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Destroy,( void ))
{
	if( m_hWnd ) {
		SetWindowLong( m_hWnd, GWL_USERDATA, 0 );
		DestroyWindow( m_hWnd );
		m_hWnd = 0;
	}	

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( GetHandle,( HWND *out_hChild ))
{
	*out_hChild = m_hWnd;
	
	if( m_hWnd ) {
		return S_OK;
	}
	
	return E_FAIL;
}

//

HRESULT CDALibs_3DWindow::Refresh( void )
{
	if( !m_IRenderPipe || !m_CurrentCamera ) {
		return E_FAIL;
	}

	m_IRenderPipe->set_window( m_hWnd, 0, 0, m_ClientRect.right, m_ClientRect.bottom );
	m_IRenderPipe->begin_scene();

	COMPTR<ISceneCamera> ISC;
	m_CurrentCamera->QueryInterface( IID_ISceneCamera, ISC );

	COMPTR<ICamera> IC;
	m_CurrentCamera->QueryInterface( IID_ICamera, IC );
	
	if( IC == NULL ) {
		GENERAL_WARNING( "ICamera is NULL\n" );
		return E_FAIL;
	}

	ISC->BeginScene();

	COMPTR<ILightManager> IL;
	if( SUCCEEDED( m_IEngine->QueryInterface( IID_ILightManager, IL ) ) ) {
		ILight *lights[8];
		int num_lights;
		IL->deactivate_all_lights();
		IL->set_ambient_light( _R(m_AmbientLight), _G(m_AmbientLight), _B(m_AmbientLight) );
		num_lights = IL->get_all_lights( lights );
		IL->activate_lights( lights, num_lights );
		IL->update_lighting( IC );
	}

	PrimitiveBuilder pb( m_IRenderPipe );
	if( m_ViewWorldAxis ) {
		m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, 0 );
		m_IRenderPipe->set_render_state( D3DRS_ALPHABLENDENABLE, 0 );
		pb.Begin( GL_LINES );
			pb.Color3f( 1.00, 0.0, 0.0 );	pb.Vertex3f(  20.0,	0, 0 );
			pb.Color3f( 0.25, 0.0, 0.0 );	pb.Vertex3f( -20.0,	0, 0 );
			
			pb.Color3f( 0.0, 1.00, 0.0 );	pb.Vertex3f( 0,  20.0, 0 );
			pb.Color3f( 0.0, 0.25, 0.0 );	pb.Vertex3f( 0, -20.0, 0 );
			 
			pb.Color3f( 0.0, 0.0, 1.00 );	pb.Vertex3f( 0, 0,  20.0 );
			pb.Color3f( 0.0, 0.0, 0.25 );	pb.Vertex3f( 0, 0, -20.0 );
		pb.End();
	}

	COMPTR<IRenderable> Item;
	if( SUCCEEDED( DB_FindRenderables() ) ) {
		while( SUCCEEDED( DB_NextRenderable( Item ) ) ) {
			Item->Render( m_CurrentCamera );
		}
	}

	ISC->EndScene();
	
	m_IRenderPipe->end_scene();
	m_IRenderPipe->swap_buffers();
	return S_OK;
}

//

HRESULT CDALibs_3DWindow::InsertNewItem( const char *name, const char *content_type )
{
	CCommonDialog ofd( m_hWnd, "Compound 3D Files (*.cmp)\0*.cmp\0 3DB Files (*.3db)\0*.3db\0Particle System (*.pte)\0*.pte\0All Files (*.*)\0*.*\0\0" );

	char filename[255+1];
	if( name == NULL ) {
		if( FAILED( ofd.DoModal() ) ) {
			return S_OK;
		}
		ofd.GetPathName( filename, 255 );
	}
	else {
		strcpy( filename, name );
	}

	if( content_type == NULL ) {
		content_type = "CDALibs_Renderable";
	}

	CLSID_DACOMDESC obj_desc( content_type, m_ISystem, m_IEngine );
	COMPTR<IDAComponent> obj;
	if( FAILED( m_ICOManager->CreateInstance( &obj_desc, (void**) &obj ) ) ) {
		return E_FAIL;
	}

	COMPTR<IPersistable> IP2;
	if( SUCCEEDED( obj->QueryInterface( IID_IPersistable, (void**) &IP2) ) ) {
		if( FAILED( IP2->LoadFromFileSystem( filename, NULL ) ) ) {
			return E_FAIL;
		}
	}

	char shortname[255+1];
	_splitpath( filename, NULL, NULL, shortname, NULL );
	DB_InsertObject( shortname, obj );

#if 1
	COMPTR<IRenderable> IR;
	COMPTR<ILowLevelCamera> IC;
	if( SUCCEEDED( obj->QueryInterface( IID_IRenderable, IR ) ) ) {
		if( FAILED( obj->QueryInterface( IID_ILowLevelCamera, IC ) ) ) {
//			if( m_CurrentSelection == NULL ) {
				m_CurrentSelection = IR;
//			}
		}
	}
#endif

	return S_OK;
}

//

HRESULT CDALibs_3DWindow::InsertNewItems( char *string, const char *content_type )
{
	char *p = string;

	while( p && *p ) {
		
		while( *p && strchr( ",; \t\n", *p ) ) p++;
		if( *p == 0 ) {
			break;
		}
		char *name = p;
		
		while( *p && !strchr( ",; \t\n", *p ) ) p++;
		if( *p != 0 ) {
			*p = 0;
			p++;
		}

		if( strcmp( name, "" ) != 0 ) {
			InsertNewItem( name, content_type );
		}
	} 
	
	return S_OK;
}






//

HRESULT CDALibs_3DWindow::OnInsert( UINT wID, HWND hControl, UINT NotifyCode )
{
	InsertNewItem(NULL,NULL);
	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnWindowOptionWorldAxis( UINT wID, HWND hControl, UINT NotifyCode )
{
	m_ViewWorldAxis = m_ViewWorldAxis?0:1;
	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnLightAmbient( UINT wID, HWND hControl, UINT NotifyCode )
{
	CHOOSECOLOR cc;                 
	static COLORREF acrCustClr[16]; 
	static DWORD rgbCurrent;        

	COMPTR<ILightManager> IL;
	if( FAILED( m_IEngine->QueryInterface( IID_ILightManager, (void **)&IL ) ) ) {
		return MSG_HANDLED;
	}

	int r,g,b;
	IL->get_ambient_light( r, g, b );

	// Initialize CHOOSECOLOR 
	ZeroMemory( &cc, sizeof(CHOOSECOLOR) );
	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = m_hWnd;
	cc.lpCustColors = (LPDWORD) acrCustClr;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT; 
	cc.rgbResult = (b<<16)|(g<<8)|(r<<0);

	if( ChooseColor(&cc) == TRUE ) {
		// m_AmbientLight is a packed 32bit ARGB, rgbResult is 0BGR..
		//
		m_AmbientLight = (((cc.rgbResult>>0) &0xFF)<<16) |
						 (((cc.rgbResult>>8) &0xFF)<<8)  |
						 (((cc.rgbResult>>16)&0xFF)<<0)  |
						 0;
	}
	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnCameraCannedShot( UINT wID, HWND hControl, UINT NotifyCode )
{
	if( m_CurrentSelection ) {
		if( SUCCEEDED( DB_FindCameraByName( "DefaultCamera", m_CurrentCamera ) ) ) {
			COMPTR<ISceneCamera> ISC;
			if( SUCCEEDED( m_CurrentCamera->QueryInterface( IID_ISceneCamera, ISC ) ) ) {
				U32 shot_type = wID - ID_3DWINDOW_CAMERA_SHOT_START;
				ISC->SetShot( ISCSHOTDESC( (ISCSHOTTYPE)shot_type, m_CurrentSelection ) );
				InvalidateRect( m_hWnd, NULL, FALSE );
			}
		}
	}
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnCameraCannedCamera( UINT wID, HWND hControl, UINT NotifyCode )
{
	U32 num_cameras;
//	U32 num_lights;

	DB_GetCameraCount( &num_cameras );
	if( (wID >= ID_3DWINDOW_CAMERA_ID) && (wID < (ID_3DWINDOW_CAMERA_ID+num_cameras)) ) {

		U32 cam_num = wID - ID_3DWINDOW_CAMERA_ID;
		
		COMPTR<ILowLevelCamera> ILLC;
		DB_FindCameras();
		for( U32 c=0; c<=cam_num; c++ ) {
			DB_NextCamera( ILLC );
		}

		m_CurrentCamera = ILLC;
		InvalidateRect( m_hWnd, NULL, FALSE );
	}
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnPaint( UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	BeginPaint( m_hWnd, &ps );
	EndPaint( m_hWnd, &ps );
	Refresh();
	return MSG_HANDLED;
}

//

HRESULT CDALibs_3DWindow::OnPopupContextMenu( UINT message, WPARAM wParam, LPARAM lParam )
{
	POINT p;
	p.x = LOWORD(lParam);
	p.y = HIWORD(lParam);
	ClientToScreen( m_hWnd, &p );
	BuildContextMenu( p.x, p.y, wParam );
	return MSG_HANDLED;
}

//

#define M_PI  3.14159265359

#define OTO_F_OBJECT	(1<<0)
#define OTO_F_WORLD		(1<<1)
#define OTO_F_TETHER	(1<<2)|OTO_F_OBJECT

#define OTO_F_X			(1<<8)
#define OTO_F_Y			(1<<9)
#define OTO_F_Z			(1<<10)

#define OTO_F_NEG		(1<<16)


inline OrientOrTranslateObject( IGeoTransformable *object, U32 oto_f_flags, float delta_o, float delta_t, bool orient )
{
	Vector axis;

	if( oto_f_flags & OTO_F_OBJECT ) {
		if( oto_f_flags & OTO_F_X ) {
			object->GetBasisI( &axis );
		} 
		else if( oto_f_flags & OTO_F_Y ) {
			object->GetBasisJ( &axis );
		}
		else if( oto_f_flags & OTO_F_Z ) {
			object->GetBasisK( &axis );
		}
	}
	else if( oto_f_flags & OTO_F_WORLD ){
		if( oto_f_flags & OTO_F_X ) {
			axis = Vector(1,0,0);
		} 
		else if( oto_f_flags & OTO_F_Y ) {
			axis = Vector(0,1,0);
		}
		else if( oto_f_flags & OTO_F_Z ) {
			axis = Vector(0,0,1);
		}
	}

	if( oto_f_flags & OTO_F_NEG ) {
		axis = -axis;
	}

	if( orient == true ) {
		Quaternion q( axis, (delta_o * M_PI)/180.0 );
		Matrix o;
		object->GetOrientation( &o );
		Matrix new_o = Matrix(q) * o;
		object->SetOrientationFromMatrix( &new_o );
		
		if( oto_f_flags & OTO_F_TETHER ) {
			Vector pos;
			object->GetTranslation( &pos );
			pos = new_o.get_k() * pos.magnitude();
			object->SetTranslation( &pos );
		}
	}
	else {
		Vector pos;
		object->GetTranslation( &pos );
		pos += delta_t * axis;
		object->SetTranslation( &pos );
	}
}

//

#define IA_F_SHIFT	(1<<0)
#define IA_F_CTRL	(1<<1)
#define IA_F_ALT	(1<<2)


HRESULT CDALibs_3DWindow::HandleIA( UINT message, WPARAM wParam, LPARAM lParam )
{
	static U32 key_f = 0;

	switch( message ) {
	case WM_SYSKEYDOWN:		key_f |= IA_F_ALT;		break;
	case WM_KEYDOWN:
		switch( wParam ) {
		case VK_SHIFT:		key_f |= IA_F_SHIFT;	break;
		case VK_CONTROL:	key_f |= IA_F_CTRL;		break;
		}
	
	case WM_SYSKEYUP:		key_f &= ~(IA_F_ALT);	break;
	case WM_KEYUP:
		switch( wParam ) {
		case VK_SHIFT:		key_f &= ~(IA_F_SHIFT);	break;
		case VK_CONTROL:	key_f &= ~(IA_F_CTRL);	break;
		}
	}

	if( HandleSelectionIA( message, wParam, lParam, key_f ) == MSG_HANDLED ) {
		return MSG_HANDLED;
	}

	if( HandleCameraIA( message, wParam, lParam, key_f ) == MSG_HANDLED ) {
		return MSG_HANDLED;
	}

	if( HandleLightIA( message, wParam, lParam, key_f ) == MSG_HANDLED ) {
		return MSG_HANDLED;
	}

	if( HandleObjectIA( message, wParam, lParam, key_f ) == MSG_HANDLED ) {
		return MSG_HANDLED;
	}


	return MSG_NOT_HANDLED;
}

//

HRESULT CDALibs_3DWindow::HandleSelectionIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags )
{
	if( message == WM_KEYDOWN && wParam == VK_TAB ) {
		if( flags & IA_F_CTRL ) {
			COMPTR<ILowLevelCamera> ILLC;
			DB_FindCameras();
			while( SUCCEEDED( DB_NextCamera( ILLC ) ) ) {
				if( ILLC == m_CurrentCamera ) {
					if( FAILED( DB_NextCamera( ILLC ) ) ) {
						DB_FindCameras();
						DB_NextCamera( ILLC );					
					}
					m_CurrentCamera = ILLC;
				}
			}
		}
		else if( flags & IA_F_ALT ) {
			COMPTR<ILight> ILLC;
			DB_FindLights();
			while( SUCCEEDED( DB_NextLight( ILLC ) ) ) {
				if( ILLC == m_CurrentLight ) {
					if( FAILED( DB_NextLight( ILLC ) ) ) {
						DB_FindLights();
						DB_NextLight( ILLC );					
					}
					m_CurrentLight = ILLC;
				}
			}
		}
		else {
			COMPTR<IRenderable> ILLC;
			DB_FindRenderables();
			while( SUCCEEDED( DB_NextRenderable( ILLC ) ) ) {
				if( ILLC == m_CurrentSelection ) {
					if( FAILED( DB_NextRenderable( ILLC ) ) ) {
						DB_FindRenderables();
						DB_NextRenderable( ILLC );					
					}
					m_CurrentSelection = ILLC;
				}
			}
		}
		InvalidateRect( m_hWnd, NULL, FALSE );
		return MSG_HANDLED;
	}
	
	return MSG_NOT_HANDLED;
}

//


HRESULT CDALibs_3DWindow::HandleObjectIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags )
{
	COMPTR<IGeoTransformable> Object;
	if( m_CurrentSelection == NULL || FAILED( m_CurrentSelection->QueryInterface( IID_IGeoTransformable, Object ) ) ) {
		return MSG_NOT_HANDLED;
	}

	if( flags & (IA_F_CTRL|IA_F_ALT) ) {
		return MSG_NOT_HANDLED;
	}

	U32 oto_flags = OTO_F_OBJECT;

	switch( message ) {
	case WM_KEYDOWN:
		switch( wParam ) {
		case VK_RIGHT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_LEFT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_UP:			OrientOrTranslateObject( Object, oto_flags|OTO_F_Y,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_DOWN:		OrientOrTranslateObject( Object, oto_flags|OTO_F_Y|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'A':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'Z':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		default:			return MSG_NOT_HANDLED;
		}
		InvalidateRect( m_hWnd, NULL, FALSE );
		return MSG_HANDLED;

	}

	return MSG_NOT_HANDLED;
}

//

HRESULT CDALibs_3DWindow::HandleCameraIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags )
{
	COMPTR<IGeoTransformable> Object;
	if( m_CurrentCamera == NULL || FAILED( m_CurrentCamera->QueryInterface( IID_IGeoTransformable, Object ) ) ) {
		return MSG_NOT_HANDLED;
	}

	if( !(flags & IA_F_CTRL) ) {
		return MSG_NOT_HANDLED;
	}

	U32 oto_flags = OTO_F_TETHER;
	float f, a;

	switch( message ) {
	
	case WM_KEYDOWN:
		switch( wParam ) {
		case VK_RIGHT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_LEFT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_UP:			OrientOrTranslateObject( Object, oto_flags|OTO_F_Y,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_DOWN:		OrientOrTranslateObject( Object, oto_flags|OTO_F_Y|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'A':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'Z':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;

		case VK_ADD:
			m_CurrentCamera->GetHorizontalFieldOfView( &f );
			m_CurrentCamera->GetAspect( &a );
			m_CurrentCamera->SetHorizontalFieldOfView( f + 10.0 );
			m_CurrentCamera->SetAspect( ILLC_ASPECT_H2V, a );
			break;

		case VK_SUBTRACT:
			m_CurrentCamera->GetHorizontalFieldOfView( &f );
			m_CurrentCamera->GetAspect( &a );
			m_CurrentCamera->SetHorizontalFieldOfView( f - 10.0 );
			m_CurrentCamera->SetAspect( ILLC_ASPECT_H2V, a );
			break;

		case 'F':
			m_CurrentCamera->GetFarClipDistance( &f );
			if( (flags & IA_F_SHIFT) ) {
				f -= 10.0;
			}
			else {
				f += 10.0;
			}
			m_CurrentCamera->SetFarClipDistance( f );
			break;

		case 'N':
			m_CurrentCamera->GetNearClipDistance( &f );
			if( (flags & IA_F_SHIFT) ) {
				f -= 10.0;
			}
			else {
				f += 10.0;
			}
			m_CurrentCamera->SetNearClipDistance( f );
			break;

		default:
			return MSG_NOT_HANDLED;
		}

		InvalidateRect( m_hWnd, NULL, FALSE );
		return MSG_HANDLED;
	}

	return MSG_NOT_HANDLED;
}

//

HRESULT CDALibs_3DWindow::HandleLightIA( UINT message, WPARAM wParam, LPARAM lParam, U32 flags )
{
	COMPTR<IGeoTransformable> Object;
	if( m_CurrentLight == NULL || FAILED( m_CurrentLight->QueryInterface( IID_IGeoTransformable, Object ) ) ) {
		return MSG_NOT_HANDLED;
	}

	if( !(flags & IA_F_ALT) ) {
		return MSG_NOT_HANDLED;
	}

	U32 oto_flags = OTO_F_OBJECT;
	switch( message ) {
	
	case WM_KEYDOWN:
		switch( wParam ) {
		case VK_RIGHT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_LEFT:		OrientOrTranslateObject( Object, oto_flags|OTO_F_X|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_UP:			OrientOrTranslateObject( Object, oto_flags|OTO_F_Y,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case VK_DOWN:		OrientOrTranslateObject( Object, oto_flags|OTO_F_Y|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'A':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z,				m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;
		case 'Z':			OrientOrTranslateObject( Object, oto_flags|OTO_F_Z|OTO_F_NEG,	m_DeltaAngle, m_DeltaTranslate, (flags & IA_F_SHIFT) );	break;

		default:
			return MSG_NOT_HANDLED;
		}

		InvalidateRect( m_hWnd, NULL, FALSE );
		return MSG_HANDLED;
	}

	return MSG_NOT_HANDLED;
}










// 

HRESULT RegisterCDALibs_3DWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_3DWindow );
	if( T_DACOM_CreateInstance<CDALibs_3DWindow,CLSID_DACOMDESC>( CLSID_CDALibs_3DWindow, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
		COMPTR<IComponentFactory> ICF;
		if( SUCCEEDED( IDAC->QueryInterface( IID_IComponentFactory, (void**) &ICF ) ) ) {
			dacom->RegisterComponent( ICF, IID_IDAComponent );	
																
			return S_OK;
		}
	}
	return E_FAIL;
}

//

GENRESULT CDALibs_3DWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_3DWindow,CLSID_DACOMDESC>( CLSID_CDALibs_3DWindow, desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_3DWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_3DWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_3DWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_3DWindow::CDALibs_3DWindow( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
	m_hWnd = 0;
	SetRect( &m_ClientRect, 0, 0, 0, 0 );
	m_ViewWorldAxis = 0;
	
	m_ICOManager = DACOM_Acquire();
	m_ISystem = creation_info._ISystem;
	m_IEngine = creation_info._IEngine;
}

//

CDALibs_3DWindow::~CDALibs_3DWindow()
{
	Destroy();
}



