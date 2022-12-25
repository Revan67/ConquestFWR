// DALibs_GammaTestWindow.cpp: implementation of the 
// CDALibs_GammaTestWindow class.
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
#include "IProfileParser.h"
#include "RPUL.h"
#include "stddat.h"
#include "IRenderPrimitive.h"
#include "IGammaControl.h"

#include "resource.h"

#include "DPF.h"
#include "MessageCrackers.h"
#include "TCollection.h"
#include "SymbolTable.h"
#include "CommonDialog.h"
#include "FrameTracker.h"
#include "IPersistable.h"
#include "IDecorator.h"
#include "ILowLevelCamera.h"
#include "IRenderable.h"
#include "INamedProperty.h"
#include "ISceneCamera.h"
#include "IWindow.h"
#include "IObjectDatabase.h"
#include "DACOM_Utility.h"
#include "ITextureLibrary.h"

//

template <class T>
int t_clamp( T val, T lower, T upper, T &out ) 
{
	int c = 0;
	T o = val;
	if( val < lower ) {
		o = lower;
		c = -1;
	}
	if( val > upper ) {
		o = upper;
		c = 1;
	}
	
	out = o;
	return c;
}


//

#define DACOM_COMPONENT_NAME CDALibs_GammaTestWindow

const char *CLSID_CDALibs_GammaTestWindow = "CDALibs_GammaTestWindow";

dacom_component CDALibs_GammaTestWindow :	dacom_implements IWindow,
											dacom_implements IComponentFactory
{
	BEGIN_STATIC_WP_MAPS(CDALibs_GammaTestWindow)
//		BEGIN_COMMAND_MAP
//		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_PAINT,OnPaint)
			ON_MESSAGE(WM_KEYDOWN,OnKey)
		END_MESSAGE_MAP
	END_STATIC_WP_MAPS

	HRESULT OnPaint( UINT message, WPARAM wParam, LPARAM lParam ) 
	{ 
		PAINTSTRUCT ps;
		BeginPaint( m_hWnd, &ps );
		EndPaint(m_hWnd, &ps );
		Refresh();
		return S_OK; 
	}

	HRESULT OnKey( UINT message, WPARAM wParam, LPARAM lParam ); 

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
	CDALibs_GammaTestWindow( CLSID_DACOMDESC &creation_info );
	~CDALibs_GammaTestWindow();

protected:	// Interface
	void Refresh( void );


protected:	// Private Data
//	COMPTR<ICOManager>				m_ICOManager;
	COMPTR<IEngine>					m_IEngine;
	COMPTR<ISystemContainer>		m_ISystem;
	COMPTR<IRenderPipeline>			m_IRenderPipe;
	COMPTR<IGammaControl>			m_IGammaControl;
	COMPTR<ITextureLibrary>			m_ITextureLibrary;

	U32		m_GammaTexture;

	HWND							m_hWnd;
	RECT							m_ClientRect;

	RPFont							m_Font;

	float	m_Gamma;
	float	m_Slope;
	float	m_Offset;
	float	m_Bias;

	int		m_RefCnt;
};

//

HRESULT CDALibs_GammaTestWindow::OnKey( UINT message, WPARAM wParam, LPARAM lParam ) 
{
	float so_delta = 1.0/256.0;

	switch( wParam ) {
	
	case VK_ADD:		t_clamp<float>( m_Gamma+0.05, -5.0, 5.0, m_Gamma );			break;  
	case VK_SUBTRACT:	t_clamp<float>( m_Gamma-0.05, -5.0, 5.0, m_Gamma );			break;  

	case VK_LEFT:		t_clamp<float>( m_Offset-so_delta, -1.0, 1.0, m_Offset );	break;
	case VK_RIGHT:		t_clamp<float>( m_Offset+so_delta, -1.0, 1.0, m_Offset );	break;

	case VK_DOWN:		t_clamp<float>( m_Slope-so_delta, 0.0, 1.0, m_Slope );		break;
	case VK_UP:			t_clamp<float>( m_Slope+so_delta, 0.0, 1.0, m_Slope );		break;

	case VK_NEXT:		t_clamp<float>( m_Bias-so_delta, -1.0, 1.0, m_Bias );		break;
	case VK_PRIOR:		t_clamp<float>( m_Bias+so_delta, -1.0, 1.0, m_Bias );		break;
	}

	m_IGammaControl->set_gamma_function( IGC_ALL, m_Gamma, m_Bias, m_Slope, m_Offset );

	InvalidateRect( m_hWnd, NULL, FALSE );
	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Create,( IWINDOWCREATEDATA *cd, HWND *out_hChild ))
{
	if( m_hWnd ) {
		Destroy();
	}

	if( out_hChild ) {
		*out_hChild =  NULL;
	}

	if( m_ISystem == NULL || m_IEngine == NULL ) {
		GENERAL_WARNING( "ISystem is null, be sure to initialize _ISystem and _IEngine of the CLSID_DACOMDESC" );
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPipeline, (void**) &m_IRenderPipe ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IGammaControl, (void**) &m_IGammaControl ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_ITextureLibrary, (void**) &m_ITextureLibrary ) ) ) {
		return E_FAIL;
	}

	WNDCLASS wc = {
		CS_HREDRAW|CS_VREDRAW,
		CDALibs_GammaTestWindow::HandleMessage,
		0,
		0,
		cd->hModuleInstance,
		NULL,
		NULL,
		(HBRUSH)GetStockObject( LTGRAY_BRUSH ),
		NULL,
		CLSID_CDALibs_GammaTestWindow
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


	m_hWnd = CreateWindowEx( 0, CLSID_CDALibs_GammaTestWindow, CLSID_CDALibs_GammaTestWindow, style,
							 rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 
							 cd->hParent, 
							 NULL, 
							 GetModuleHandle(NULL), 
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

	COMPTR<IFileSystem> IFS;
	if( FAILED( m_IEngine->create_file_system( "gamma2.txm", IFS ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ITextureLibrary->load_texture( IFS, "gammatest-template" ) ) ) {
		return E_FAIL;
	}

	ITL_TEXTURE_ID tid;
	ITL_TEXTURE_REF_ID trid;
	ITL_TEXTUREFRAME frame;
	m_ITextureLibrary->get_texture_id( "gammatest-template", &tid );
	m_ITextureLibrary->add_texture_ref( tid, &trid );
	m_ITextureLibrary->get_texture_ref_frame( trid, ITL_FRAME_CURRENT, &frame );
	m_GammaTexture = frame.rp_texture_id;

	m_Gamma		= 1.0F;
	m_Slope		= 1.0F;
	m_Offset	= 0.0F;
	m_Bias		= 0.0F;

//	if( FAILED( m_IGammaControl->set_gamma_function( IGC_ALL, m_Slope, m_Offset, m_Gamma ) ) ) {
//		MessageBox( m_hWnd, "Failed to set gamma function", "Error", MB_OK );
//	}

	ShowWindow( m_hWnd, SW_NORMAL );
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
		DestroyWindow( m_hWnd );
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

void CDALibs_GammaTestWindow::Refresh( void )
{
	if( !m_IRenderPipe ) {
		return;
	}

	PrimitiveBuilder pb( m_IRenderPipe );

	m_IRenderPipe->set_window( m_hWnd, 0, 0, m_ClientRect.right, m_ClientRect.bottom );
	m_IRenderPipe->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
	m_IRenderPipe->begin_scene();
	m_IRenderPipe->set_ortho( 0, m_ClientRect.right, m_ClientRect.bottom, 0 );
	m_IRenderPipe->set_modelview( Transform() );
	m_IRenderPipe->set_render_state( D3DRS_ZENABLE, 0 );
	
	m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, m_GammaTexture );
	
	pb.Begin( GL_QUADS );
//		pb.Color3ub( 0, 0, 128 );
		pb.TexCoord2f( 0.0, 0.0 );	pb.Vertex3f( 150,						150,						0.0 );
		pb.TexCoord2f( 0.0, 1.0 );	pb.Vertex3f( m_ClientRect.right-150,	150,						0.0 );
		pb.TexCoord2f( 1.0, 1.0 );	pb.Vertex3f( m_ClientRect.right-150,	m_ClientRect.bottom-150,	0.0 );
		pb.TexCoord2f( 1.0, 0.0 );	pb.Vertex3f( 150,						m_ClientRect.bottom-150,	0.0 );
	pb.End();

	m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, 0 );

	float graph_x = 10;
	float graph_y = 460;
	float scale_w = 100;
	float scale_h = 100;
	pb.Begin( GL_LINE_STRIP );
		pb.Color3ub( 255, 255, 255 );
		pb.Vertex3f( graph_x,			graph_y-scale_h,	0.00F );
		pb.Vertex3f( graph_x,			graph_y,			0.00F );
		pb.Vertex3f( graph_x+scale_w,	graph_y,			0.00F );
	pb.End();

	pb.Begin( GL_LINE_STRIP );
		pb.Color3ub( 255, 255, 255 );
		for( U32 sample=0; sample<256; sample++ ) {
			float s = ((float)sample/256);
			float gc;
			t_clamp<float>( ( m_Bias + pow( m_Slope*s+m_Offset, 1.0/m_Gamma ) ), 0, 1, gc );
			float y = scale_h*gc;
			pb.Vertex3f( graph_x+(s*scale_w), graph_y-y, 0.00F );
		}
	pb.End();

	m_Font.RenderFormattedString( 10, 20, "out = %5.3f + pow(%5.3f*in-%5.3f, %5.3f)", m_Bias, m_Slope, m_Offset, m_Gamma );

	m_IRenderPipe->end_scene();
	m_IRenderPipe->swap_buffers();
	return;
}

//



HRESULT RegisterCDALibs_GammaTestWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_GammaTestWindow );
	if( T_DACOM_CreateInstance<CDALibs_GammaTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_GammaTestWindow, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

GENRESULT CDALibs_GammaTestWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_GammaTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_GammaTestWindow, desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_GammaTestWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_GammaTestWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_GammaTestWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_GammaTestWindow::CDALibs_GammaTestWindow( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
	m_IRenderPipe = NULL;
	m_IGammaControl = NULL;
	m_hWnd = NULL;
	m_ISystem = creation_info._ISystem;
	m_IEngine = creation_info._IEngine;
}

//

CDALibs_GammaTestWindow::~CDALibs_GammaTestWindow()
{
	Destroy();
}


