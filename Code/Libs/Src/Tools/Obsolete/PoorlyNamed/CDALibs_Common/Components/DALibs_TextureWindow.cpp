// DALibs_TextureWindow.cpp: implementation of the CDALibs_TextureWindow class.
//
//////////////////////////////////////////////////////////////////////

#define STRICT
#include <windows.h>
#include <comdef.h>
#include <stdio.h>

#include "dacom.h"
#include "davariant.h"
#include "TSmartPointer.h"
#include "lightman.h"
#include "ILight.h"
#include "ICamera.h"
#include "RPUL.h"
#include "stddat.h"
#include "IRenderPrimitive.h"
#include "ITextureLibrary.h"
#include "model.h"
#include "vfx.h"

#include "resource.h"
#include "DPF.h"
#include "DACOM_Utility.h"
#include "MessageCrackers.h"
#include "CommonDialog.h"
#include "TCollection.h"
#include "options_inl.cpp"
#include "CommonControls.h"

#include "IWindow.h"
#include "IPersistable.h"

//

#define ID_TEXTUREWINDOW_START			60000
#define ID_TEXTUREWINDOW_VIEW			(ID_TEXTUREWINDOW_START+0)
#define ID_TEXTUREWINDOW_LIST			(ID_TEXTUREWINDOW_START+1)
#define ID_TEXTUREWINDOW_ADD			(ID_TEXTUREWINDOW_START+2)
#define ID_TEXTUREWINDOW_REMOVE			(ID_TEXTUREWINDOW_START+3)
#define ID_TEXTUREWINDOW_ANI_BEGIN		(ID_TEXTUREWINDOW_START+4)
#define ID_TEXTUREWINDOW_ANI_PREV		(ID_TEXTUREWINDOW_START+5)
#define ID_TEXTUREWINDOW_ANI_PAUSE		(ID_TEXTUREWINDOW_START+6)
#define ID_TEXTUREWINDOW_ANI_STOP		(ID_TEXTUREWINDOW_START+7)
#define ID_TEXTUREWINDOW_ANI_PLAY		(ID_TEXTUREWINDOW_START+8)
#define ID_TEXTUREWINDOW_ANI_NEXT		(ID_TEXTUREWINDOW_START+9)
#define ID_TEXTUREWINDOW_ANI_END		(ID_TEXTUREWINDOW_START+10)
#define ID_TEXTUREWINDOW_LOD_MINUS		(ID_TEXTUREWINDOW_START+11)
#define ID_TEXTUREWINDOW_LOD_PLUS		(ID_TEXTUREWINDOW_START+12)
#define ID_TEXTUREWINDOW_MISC			(ID_TEXTUREWINDOW_START+13)
				
//

#define TW_BORDER_WIDTH			(8)

#define TW_BUTTON_HEIGHT		(22)

#define TW_LIST_BUTTON_WIDTH	(70)
#define TW_LIST_BUTTON_SPACE	(6)
#define TW_LIST_BUTTON_CNT		(2)

#define TW_VIEW_BUTTON_WIDTH	(28)
#define TW_VIEW_BUTTON_SPACE	(4)
#define TW_VIEW_BUTTON_CNT		(10)

#define TW_LIST_STYLE	(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_EX_FULLROWSELECT | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_NOSORTHEADER)
#define TW_BUTTON_STYLE (WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_PUSHBUTTON)

//

#define DACOM_COMPONENT_NAME CDALibs_TextureWindow

const char *CLSID_CDALibs_TextureWindow = "CDALibs_TextureWindow";

dacom_component CDALibs_TextureWindow :	dacom_implements IWindow,
//										dacom_implements IDataConsumer,
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
	CDALibs_TextureWindow( CLSID_DACOMDESC &creation_info );
	~CDALibs_TextureWindow();

protected:	// Private Data

	struct TEXTUREINFO
	{
		U32 texture_id;
		U32 width, height, num_lod;
	};

	TEXTUREINFO m_CurrentTexture;

	// cached stuff
	//
	HWND	m_hWnd, m_hList;
	RECT	m_ClientRect;
	RECT	m_TextureView;
	int		m_RefCnt;

	COMPTR<ICOManager>			m_ICOManager;
	COMPTR<ISystemContainer>	m_ISystem;
	COMPTR<IEngine>				m_IEngine;
	COMPTR<IRenderPipeline>		m_IRenderPipe;
	COMPTR<ITextureLibrary>		m_ITextureLibrary;
	
protected:	// Interface

	// Object Management
	//
	HRESULT InsertNewItem( const char *object_name, const char *content_type );
	HRESULT RefreshList( void );
	HRESULT RefreshView( void );
	HRESULT SetSelectedTexture( U32 id );


public:

	BEGIN_STATIC_WP_MAPS(CDALibs_TextureWindow)
		BEGIN_COMMAND_MAP
			ON_COMMAND(ID_TEXTUREWINDOW_LIST,OnListEvent)			
			ON_COMMAND(ID_TEXTUREWINDOW_ADD,OnInsert)
			ON_COMMAND(ID_TEXTUREWINDOW_REMOVE,OnRemove)
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_BEGIN,OnAniEvent)
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_PREV,OnAniEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_PAUSE,OnAniEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_STOP,OnAniEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_PLAY,OnAniEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_NEXT,OnAniEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_ANI_END,OnAniEvent)			
			ON_COMMAND(ID_TEXTUREWINDOW_LOD_MINUS,OnMipEvent)		
			ON_COMMAND(ID_TEXTUREWINDOW_LOD_PLUS,OnMipEvent)		
			ON_COMMAND_DEFAULT(OnCommand)		
		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_PAINT,OnPaint)
			ON_MESSAGE(WM_SETFOCUS,OnSetFocus)
			ON_MESSAGE(WM_RBUTTONUP,OnPopupContextMenu)
			ON_MESSAGE(WM_NOTIFY,OnNotify)
		END_MESSAGE_MAP
	END_STATIC_WP_MAPS

	DEF_ON_COMMAND(OnListEvent)			
	DEF_ON_COMMAND(OnInsert)
	DEF_ON_COMMAND(OnRemove)
	DEF_ON_COMMAND(OnAniEvent)
	DEF_ON_COMMAND(OnMipEvent)		
	DEF_ON_COMMAND(OnCommand)		
	DEF_ON_MESSAGE(OnPaint)
	DEF_ON_MESSAGE(OnSetFocus)
	DEF_ON_MESSAGE(OnNotify)
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

	if( FAILED( m_ISystem->QueryInterface( IID_ITextureLibrary, (void**) &m_ITextureLibrary ) ) ) {
		return E_FAIL;
	}

	WNDCLASS wc = {
		CS_HREDRAW|CS_VREDRAW,
		CDALibs_TextureWindow::HandleMessage,
		0,
		0,
		cd->hModuleInstance,
		NULL,
		NULL,
		(HBRUSH)GetStockObject( LTGRAY_BRUSH ),
		NULL,
		CLSID_CDALibs_TextureWindow
	};

	U32 val = (U32)RegisterClass( &wc );

	U32 style = cd->uWindowStyle | 
				WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS |
				0;
	
	RECT rect = cd->rChildRect;

	AdjustWindowRect( &rect, style, FALSE );
	if( rect.top < 0 || rect.left < 0 ) {
		rect.right	+= -rect.left;
		rect.left   += -rect.left;
		rect.bottom += -rect.top;
		rect.top	+= -rect.top;
	}

	m_hWnd = CreateWindowEx( 0, CLSID_CDALibs_TextureWindow, CLSID_CDALibs_TextureWindow, style,
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
	
	// Layout sub children
	//

	RECT ucr;
	HWND c;

	// usable client rect
	SetRect( &ucr, m_ClientRect.left+TW_BORDER_WIDTH,
				   m_ClientRect.top+TW_BORDER_WIDTH,
				   m_ClientRect.right-TW_BORDER_WIDTH,
				   m_ClientRect.bottom-TW_BORDER_WIDTH );

	U32 w = ucr.right - ucr.left - TW_BORDER_WIDTH;	// center trough between list and view
	U32 h = ucr.bottom - ucr.top;
	U32 button_h = TW_BUTTON_HEIGHT + TW_BORDER_WIDTH;

	// List
	m_hList = CreateWindowEx( WS_EX_CLIENTEDGE,
						  WC_LISTVIEW, 
						  "", 
						  TW_LIST_STYLE, 
						  ucr.left, 
						  ucr.top, 
						  w/2, 
						  h-button_h, 
						  m_hWnd, 
						  (HMENU)ID_TEXTUREWINDOW_LIST, 
						  cd->hModuleInstance, 
						  this );

	if( m_hList == NULL ) { 
			DestroyWindow( m_hWnd );
			return E_FAIL;
	}

															
	LV_SetColumns( m_hList,	"Texture    Id", LVCFMT_CENTER,
							"Name", LVCFMT_CENTER,
							"Pixel Format", LVCFMT_CENTER,
							"Width", LVCFMT_CENTER,
							"Height", LVCFMT_CENTER,
							"Mipmaps", LVCFMT_CENTER,
							NULL );

	// List buttons
	//
	U32 button_w = TW_LIST_BUTTON_CNT * TW_LIST_BUTTON_WIDTH + (TW_LIST_BUTTON_CNT-1) * TW_LIST_BUTTON_SPACE;
	U32 button_x = ucr.left + w/4 - button_w/2;

	c = CreateWindow( "BUTTON", 
					  "Add...", 
					  TW_BUTTON_STYLE, 
					  button_x, 
					  ucr.bottom-TW_BUTTON_HEIGHT, 
					  TW_LIST_BUTTON_WIDTH, 
					  TW_BUTTON_HEIGHT, 
					  m_hWnd, 
					  (HMENU)ID_TEXTUREWINDOW_ADD, 
					  cd->hModuleInstance, 
					  this );
	if( c == NULL ) { 
			DestroyWindow( m_hWnd );
	}

	button_x += TW_LIST_BUTTON_WIDTH + TW_LIST_BUTTON_SPACE;
	c = CreateWindow( "BUTTON", 
					  "Remove", 
					  TW_BUTTON_STYLE, 
					  button_x, 
					  ucr.bottom-TW_BUTTON_HEIGHT, 
					  TW_LIST_BUTTON_WIDTH, 
					  TW_BUTTON_HEIGHT, 
					  m_hWnd, 
					  (HMENU)ID_TEXTUREWINDOW_REMOVE, 
					  cd->hModuleInstance, 
					  this );
	if( c == NULL ) { 
			DestroyWindow( m_hWnd );
			return E_FAIL;
	}
	
	// View
	SetRect( &m_TextureView, ucr.right- w/2, ucr.top, w/2, (h-button_h) );


	// View Buttons
	char *button_text[10] = { "|<<", "<<", "||", "[]", "|>", ">>", ">>|", "-", "+", "..." };

	button_w = TW_VIEW_BUTTON_CNT * TW_VIEW_BUTTON_WIDTH + (TW_VIEW_BUTTON_CNT-1) * TW_VIEW_BUTTON_SPACE;
	button_x = ucr.right - w/4  - button_w/2;
	for( U32 b=0; b<10; b++ ) {
	
		c = CreateWindow( "BUTTON", 
						  button_text[b], 
						  TW_BUTTON_STYLE, 
						  button_x, 
						  ucr.bottom-TW_BUTTON_HEIGHT, 
						  TW_VIEW_BUTTON_WIDTH, 
						  TW_BUTTON_HEIGHT, 
						  m_hWnd, 
						  (HMENU)(ID_TEXTUREWINDOW_ANI_BEGIN+b), 
						  cd->hModuleInstance, 
						  this );
		if( c == NULL ) { 
				DestroyWindow( m_hWnd );
				return E_FAIL;
		}
		button_x += TW_VIEW_BUTTON_WIDTH + TW_VIEW_BUTTON_SPACE;
	}

	//
	// End layout

	memset( &m_CurrentTexture, 0, sizeof( m_CurrentTexture ) );

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

HRESULT CDALibs_TextureWindow::SetSelectedTexture( U32 id )
{
	m_CurrentTexture.texture_id = id;
	m_IRenderPipe->get_texture_dim( id, &m_CurrentTexture.width, &m_CurrentTexture.height, &m_CurrentTexture.num_lod );
	return S_OK;
}

//

HRESULT CDALibs_TextureWindow::RefreshList( void )
{
	if( m_ITextureLibrary == NULL || m_IRenderPipe == NULL ) {
		return E_FAIL;
	}

	U32 num_textures, texture_id, first_id=0;
	PixelFormat pf;
	U32 w,h, lod, col = 0;
	char name[255+1], s0[64+1], s1[64+1], s2[64+1], s3[64+1], s4[64+1];

	ListView_DeleteAllItems( m_hList );
	
	m_IRenderPipe->get_num_textures( &num_textures );
	for( U32 t=0; t<num_textures; t++ ) {
	
		m_IRenderPipe->get_texture( t, &texture_id );

		if( texture_id == 0 ) {
			continue;
		}

		if( first_id == 0 ) {
			first_id = texture_id;
		}
		
		m_IRenderPipe->get_texture_format( texture_id, &pf );
		m_IRenderPipe->get_texture_dim( texture_id, &w, &h, &lod );
		
		sprintf( name, "(unnamed)" );
//		m_ITXMLib->get_texture_name( texture_id, name );

		LV_InsertRow( m_hList, MakeString( s4, "%04X (%d)", texture_id, texture_id ), texture_id,
					  name,
					  MakeString( s0, "%dbpp %d%d%d%d", pf.ddpf.dwRGBBitCount, pf.rwidth, pf.gwidth, pf.bwidth, pf.awidth ),
					  MakeString( s1, "%d", w ), MakeString( s2, "%d", h ), MakeString( s3, "%d", lod ), NULL );
	}

	if( ListView_GetSelectedCount( m_hList ) == 0 ) {
		SetSelectedTexture( first_id );
	}

	return S_OK;
}

//

HRESULT CDALibs_TextureWindow::RefreshView( void )
{
	RECT win = m_TextureView;
	m_IRenderPipe->set_window( m_hWnd, win.left, win.top, win.right, win.bottom );
	m_IRenderPipe->begin_scene();
	
	m_IRenderPipe->set_viewport( 0, 0, win.right, win.bottom );
	m_IRenderPipe->set_ortho( 0, win.right, win.bottom, 0 );
	m_IRenderPipe->set_modelview( Transform() );
//	m_IRenderPipe->set_pipeline_state( RP_CLEAR_COLOR, 0x00C0C0 );
	m_IRenderPipe->set_pipeline_state( RP_CLEAR_COLOR, 0x202020 );
	m_IRenderPipe->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );

	// Render quad with texture
	float cx = win.right/2;
	float cy = win.bottom/2;
	float w2 = m_CurrentTexture.width / 2;
	float h2 = m_CurrentTexture.height / 2;

	if( m_CurrentTexture.texture_id == 0 ) {
		w2 = 64;
		h2 = 64;
	}

	PrimitiveBuilder pb( m_IRenderPipe, 8 );

	m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, m_CurrentTexture.texture_id );
	m_IRenderPipe->set_render_state( D3DRS_ALPHABLENDENABLE, FALSE );
	m_IRenderPipe->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
//	m_IRenderPipe->set_render_state( D3DRS_TEXTUREMAPBLEND, D3DTBLEND_DECAL );
	pb.Begin( GL_QUADS );
		pb.TexCoord2f( 0.0, 0.0 );	pb.Vertex3f( cx-w2, cy-h2, 0.25 );
		pb.TexCoord2f( 0.0, 1.0 );	pb.Vertex3f( cx-w2, cy+h2, 0.25);
		pb.TexCoord2f( 1.0, 1.0 );	pb.Vertex3f( cx+w2, cy+h2, 0.25 );
		pb.TexCoord2f( 1.0, 0.0 );	pb.Vertex3f( cx+w2, cy-h2, 0.25);
	pb.End();
	
	m_IRenderPipe->end_scene();
	m_IRenderPipe->swap_buffers();

	return S_OK;
}

//

HRESULT CDALibs_TextureWindow::InsertNewItem( const char *name, const char *content_type )
{
	CCommonDialog ofd( m_hWnd, "UTF Files (*.3db;*.cmp;*.utf)\0*.3db;*.cmp;*.utf\0All Files (*.*)\0*.*\0\0" );

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
		content_type = "CDALibs_TextureLibrary";
	}

	if( m_IEngine == NULL || m_ITextureLibrary == NULL ) {
		return E_FAIL;
	}

	COMPTR<IFileSystem> IFS;
	if( FAILED( m_IEngine->create_file_system( filename, (IFileSystem**) &IFS ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ITextureLibrary->load_library( IFS ) ) ) {
		return E_FAIL;
	}
	
	return S_OK;
}

//

HRESULT CDALibs_TextureWindow::OnPaint( UINT message, WPARAM wParam, LPARAM lParam )
{
	PAINTSTRUCT ps;
	BeginPaint( m_hWnd, &ps );
	EndPaint( m_hWnd, &ps );
	RefreshList();
	RefreshView();
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnNotify( UINT message, WPARAM wParam, LPARAM lParam )
{
	NMHDR *hdr = (NMHDR*)lParam;
	PostMessage( m_hWnd, WM_COMMAND, MAKEWPARAM( hdr->idFrom, hdr->code ), (long)hdr->hwndFrom );

	switch( hdr->idFrom ) {
	case ID_TEXTUREWINDOW_LIST:
		switch( hdr->code )
		{
		case NM_DBLCLK:
			SetSelectedTexture( LV_GetFirstSelectedData( m_hList ) );
			RefreshView();
			break;

		default:
			return MSG_NOT_HANDLED;
		}
		break;

	default:
		return MSG_NOT_HANDLED;
	}

	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnSetFocus( UINT message, WPARAM wParam, LPARAM lParam )
{
	SetFocus( m_hList );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnPopupContextMenu( UINT message, WPARAM wParam, LPARAM lParam )
{
	POINT p;
	p.x = LOWORD(lParam);
	p.y = HIWORD(lParam);
	ClientToScreen( m_hWnd, &p );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnListEvent( UINT wID, HWND hControl, UINT NotifyCode )			
{
	return MSG_NOT_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnInsert( UINT wID, HWND hControl, UINT NotifyCode )
{
	InsertNewItem( NULL,NULL );
	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnRemove( UINT wID, HWND hControl, UINT NotifyCode )
{
	U32 num_items = ListView_GetItemCount( m_hList );
	for( U32 i=0; i<num_items; i++ ) {
		if( ListView_GetItemState( m_hList, i, LVIS_SELECTED ) & LVIS_SELECTED ) {
			LV_ITEM lvi = { LVIF_PARAM, i, 0, 0, 0, NULL, 0, 0, 0 };
			ListView_GetItem( m_hList, &lvi );
			m_IRenderPipe->destroy_texture( (U32)lvi.lParam );
		}
	}

	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnAniEvent( UINT wID, HWND hControl, UINT NotifyCode )
{
	switch( wID ) {
	case  ID_TEXTUREWINDOW_ANI_BEGIN:
		break;

	case  ID_TEXTUREWINDOW_ANI_PREV:	
		break;

	case  ID_TEXTUREWINDOW_ANI_PAUSE:
		break;

	case  ID_TEXTUREWINDOW_ANI_STOP:	
		break;

	case  ID_TEXTUREWINDOW_ANI_PLAY:	
		break;

	case  ID_TEXTUREWINDOW_ANI_NEXT:	
		break;

	case  ID_TEXTUREWINDOW_ANI_END:	
		break;

	default:
		return MSG_NOT_HANDLED;
	}

	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnMipEvent( UINT wID, HWND hControl, UINT NotifyCode )		
{
	switch( wID ) {
	case  ID_TEXTUREWINDOW_LOD_MINUS:
		break;

	case  ID_TEXTUREWINDOW_LOD_PLUS:
		break;

	default:
		return MSG_NOT_HANDLED;
	}
		
	InvalidateRect( m_hWnd, NULL, FALSE );
	return MSG_HANDLED;
}

//

HRESULT CDALibs_TextureWindow::OnCommand( UINT wID, HWND hControl, UINT NotifyCode )		
{
	return MSG_HANDLED;
}







// 

HRESULT RegisterCDALibs_TextureWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_TextureWindow );
	if( T_DACOM_CreateInstance<CDALibs_TextureWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureWindow, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
		COMPTR<IComponentFactory> ICF;
		if( SUCCEEDED( IDAC->QueryInterface( IID_IComponentFactory, (void**) &ICF ) ) ) {
			dacom->RegisterComponent( ICF, IID_IDAComponent );	
																
			return S_OK;
		}
	}
	return E_FAIL;
}

//

GENRESULT CDALibs_TextureWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_TextureWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureWindow, desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_TextureWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_TextureWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_TextureWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_TextureWindow::CDALibs_TextureWindow( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
	m_hWnd = 0;
	m_hList = 0;
	SetRect( &m_ClientRect, 0, 0, 0, 0 );
	SetRect( &m_TextureView, 0, 0, 0, 0 );
	
	m_ICOManager = DACOM_Acquire();
	m_ISystem = creation_info._ISystem;
	m_IEngine = creation_info._IEngine;
}

//

CDALibs_TextureWindow::~CDALibs_TextureWindow()
{
	Destroy();
}



