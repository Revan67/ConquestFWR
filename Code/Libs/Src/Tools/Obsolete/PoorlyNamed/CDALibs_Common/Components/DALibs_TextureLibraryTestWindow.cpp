// DALibs_TextureLibraryTestWindow.cpp: implementation of the 
// CDALibs_TextureLibraryTestWindow class.
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
#include "ITextureLibrary.h"

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

#define MAX_WIDE 10
#define MAX_HIGH 10

//

#define DACOM_COMPONENT_NAME CDALibs_TextureLibraryTestWindow

const char *CLSID_CDALibs_TextureLibraryTestWindow = "CDALibs_TextureLibraryTestWindow";

dacom_component CDALibs_TextureLibraryTestWindow :	dacom_implements IWindow,
											dacom_implements IComponentFactory
{
	BEGIN_STATIC_WP_MAPS(CDALibs_TextureLibraryTestWindow)
//		BEGIN_COMMAND_MAP
//		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_PAINT,OnPaint)
			ON_MESSAGE(WM_TIMER,OnPaint)
			ON_MESSAGE(WM_KEYDOWN,OnKeyDown)
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

	HRESULT OnKeyDown( UINT message, WPARAM wParam, LPARAM lParam ) 
	{ 
		int refs_per_screen = m_NumTextureRefsWide * m_NumTextureRefsHigh;
		int max_ref;

		t_clamp( m_NumTextureRefs - refs_per_screen, 0, m_NumTextureRefs, max_ref );

		switch( wParam ) {
		case VK_NEXT:	t_clamp( m_StartTextureRef + refs_per_screen, 0, max_ref, m_StartTextureRef );		break;
		case VK_PRIOR:	t_clamp( m_StartTextureRef - refs_per_screen, 0, max_ref, m_StartTextureRef );		break;	

		case VK_HOME:	m_StartTextureRef = 0;			break;
		case VK_END:	m_StartTextureRef = max_ref;	break;

		case VK_ADD:	
			t_clamp( m_NumTextureRefsWide+1, 0, MAX_WIDE, m_NumTextureRefsWide );		
			t_clamp( m_NumTextureRefsHigh+1, 0, MAX_HIGH, m_NumTextureRefsHigh );		

			// re-clamp stuff to new bounds
			refs_per_screen = m_NumTextureRefsWide * m_NumTextureRefsHigh;
			t_clamp( m_NumTextureRefs - refs_per_screen, 0, m_NumTextureRefs, max_ref );
			t_clamp( m_StartTextureRef, 0, max_ref, m_StartTextureRef );
			t_clamp( m_SelectedTextureRefRow, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow );
			t_clamp( m_SelectedTextureRefCol, 0, m_NumTextureRefsWide-1, m_SelectedTextureRefCol );
			break;

		case VK_SUBTRACT:
			t_clamp( m_NumTextureRefsWide-1, 0, MAX_WIDE, m_NumTextureRefsWide );		
			t_clamp( m_NumTextureRefsHigh-1, 0, MAX_HIGH, m_NumTextureRefsHigh );		
	
			// re-clamp stuff to new bounds
			refs_per_screen = m_NumTextureRefsWide * m_NumTextureRefsHigh;
			t_clamp( m_NumTextureRefs - refs_per_screen, 0, m_NumTextureRefs, max_ref );
			t_clamp( m_StartTextureRef, 0, max_ref, m_StartTextureRef );
			t_clamp( m_SelectedTextureRefRow, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow );
			t_clamp( m_SelectedTextureRefCol, 0, m_NumTextureRefsWide-1, m_SelectedTextureRefCol );
			break;

		case VK_LEFT:	t_clamp( m_SelectedTextureRefCol-1, 0, m_NumTextureRefsWide-1, m_SelectedTextureRefCol );	break;
		case VK_RIGHT:	t_clamp( m_SelectedTextureRefCol+1, 0, m_NumTextureRefsWide-1, m_SelectedTextureRefCol );	break;
		
		case VK_UP:		
			if( t_clamp( m_SelectedTextureRefRow-1, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow ) < 0 && 
				m_StartTextureRef != 0 ) {
				t_clamp( m_StartTextureRef - refs_per_screen, 0, max_ref, m_StartTextureRef );	
				t_clamp( m_NumTextureRefsHigh-1, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow );
			}
			break;

		case VK_DOWN:	
			if( t_clamp( m_SelectedTextureRefRow+1, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow ) > 0 &&
				m_StartTextureRef < max_ref ) {
				t_clamp( m_StartTextureRef + refs_per_screen, 0, max_ref, m_StartTextureRef );	
				t_clamp( 0, 0, m_NumTextureRefsHigh-1, m_SelectedTextureRefRow );
			}
			break;

		}
		return S_OK; 
	}

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
	CDALibs_TextureLibraryTestWindow( CLSID_DACOMDESC &creation_info );
	~CDALibs_TextureLibraryTestWindow();

protected:	// Interface
	HRESULT CreateTextures( );
	void Refresh( void );


protected:	// Private Data
//	COMPTR<ICOManager>				m_ICOManager;
	COMPTR<ISystemContainer>		m_ISystem;
	COMPTR<IEngine>					m_IEngine;
	COMPTR<IRenderPipeline>			m_IRenderPipe;
	COMPTR<ITextureLibrary>			m_ITextureLibrary;

	HWND							m_hWnd;
	RECT							m_ClientRect;

	RPFont							m_Font;

	int					m_NumTextureRefs;
	ITL_TEXTURE_REF_ID	*m_TextureRefs;
	int					m_StartTextureRef;
	int					m_NumTextureRefsWide;
	int					m_NumTextureRefsHigh;
	int					m_SelectedTextureRefRow;
	int					m_SelectedTextureRefCol;
	
	int		m_RefCnt;
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

	if( m_ISystem == NULL ) {
		GENERAL_WARNING( "ISystem is null, be sure to initialize _ISystem of the CLSID_DACOMDESC" );
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_IRenderPipeline, (void**) &m_IRenderPipe ) ) ) {
		return E_FAIL;
	}

	if( FAILED( m_ISystem->QueryInterface( IID_ITextureLibrary, (void**) &m_ITextureLibrary ) ) ) {
		return E_FAIL;
	}

	m_NumTextureRefs = 0;
	delete[] m_TextureRefs;
	m_TextureRefs = NULL;

	WNDCLASS wc = {
		CS_HREDRAW|CS_VREDRAW,
		CDALibs_TextureLibraryTestWindow::HandleMessage,
		0,
		0,
		cd->hModuleInstance,
		NULL,
		NULL,
		(HBRUSH)GetStockObject( LTGRAY_BRUSH ),
		NULL,
		CLSID_CDALibs_TextureLibraryTestWindow
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


	m_hWnd = CreateWindowEx( 0, CLSID_CDALibs_TextureLibraryTestWindow, CLSID_CDALibs_TextureLibraryTestWindow, style,
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

	if( FAILED( CreateTextures() ) ) {
		MessageBox( m_hWnd, "Failed to create textures", "ERROR", MB_OK );
	}

	ShowWindow( m_hWnd, SW_NORMAL );
	UpdateWindow( m_hWnd );


	SetTimer( m_hWnd, 1, 8, NULL );

	if( out_hChild ) {
		*out_hChild = m_hWnd;
	}

	return S_OK;
}

//

DACOM_INTERFACE_METHOD_IMPL( Destroy,( void ))
{
	KillTimer( m_hWnd, 1 );

	if( m_ITextureLibrary ) {

		for( U32 r=0; r<m_NumTextureRefs; r++ ) {
			if( m_TextureRefs[r] != ITL_INVALID_REF_ID ) {
				m_ITextureLibrary->release_texture_ref( ITL_INVALID_ID, m_TextureRefs[r] );
			}
		}			

		delete[] m_TextureRefs;
		m_TextureRefs = NULL;
		m_NumTextureRefs = 0;
	}


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

void CDALibs_TextureLibraryTestWindow::Refresh( void )
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
	
	m_IRenderPipe->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
	

	float ntw = m_NumTextureRefsWide;	// num textures across
	float nth = m_NumTextureRefsHigh;	// num textures down
	float npb = (MAX_HIGH-m_NumTextureRefsWide)*3.2 + 1;	// num pixels border (one side)
	float tw = (m_ClientRect.right - ((ntw+1)*npb)) / ntw;
	float th = (m_ClientRect.bottom - ((nth+1)*npb)) / nth;
	float x;
	float y, yt;

	ITL_TEXTUREFRAME frame;
	char tname[255+1];
	U32 cwidth = 10;
	U32 twidth = tw / cwidth - 2;


	y = npb;
	yt = y - 20;

	for( U32 h=0; h<ntw; h++ ) {
		x = npb;
		for( U32 w=0; w<ntw; w++ ) {
			frame.rp_texture_id = 0;
			strcpy( tname, "(empty)" );

			U32 idx = m_StartTextureRef + h*((U32)ntw) + ((U32)w);
			if( idx < m_NumTextureRefs ) {
				ITL_TEXTURE_ID tid;
				m_ITextureLibrary->get_texture_ref_frame( m_TextureRefs[idx], ITL_FRAME_CURRENT, &frame );
				m_ITextureLibrary->get_texture_ref_texture_id( m_TextureRefs[idx], &tid );
				m_ITextureLibrary->get_texture_name( tid, tname, 255 );
			}

			if( strlen(tname) >= twidth ) {
				tname[twidth-3] = '.';
				tname[twidth-2] = '.';
				tname[twidth-1] = '.';
				tname[twidth-0] = 0;
			}

			m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, 0 );
			m_Font.RenderFormattedString( x, yt, "%s", tname );

			if( w == m_SelectedTextureRefCol && h == m_SelectedTextureRefRow ) {
				pb.Color3ub( 255, 0, 0 );
				pb.Begin( GL_QUADS );
					pb.Vertex3f( x-4,    y-4,    -0.25 );
					pb.Vertex3f( x+tw+4, y-4,    -0.25 );
					pb.Vertex3f( x+tw+4, y+th+4, -0.25 );
					pb.Vertex3f( x-4,    y+th+4, -0.25 );
				pb.End();
				pb.Color3ub( 255, 255, 255 );
			}

			m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, frame.rp_texture_id );
			
			pb.Begin( GL_QUADS );
				pb.TexCoord2f( frame.u0, frame.v1 );	pb.Vertex3f( x,    y,    0 );
				pb.TexCoord2f( frame.u1, frame.v1 );	pb.Vertex3f( x+tw, y,    0 );
				pb.TexCoord2f( frame.u1, frame.v0 );	pb.Vertex3f( x+tw, y+th, 0 );
				pb.TexCoord2f( frame.u0, frame.v0 );	pb.Vertex3f( x,    y+th, 0 );
			pb.End();

			x += tw + npb;
		}

		y += th + npb;
		yt += th + npb;
	}

	m_IRenderPipe->end_scene();
	m_IRenderPipe->swap_buffers();
	return;
}

//

HRESULT CDALibs_TextureLibraryTestWindow::CreateTextures( )
{

	COMPTR<IFileSystem> IFS;
	if( FAILED( m_IEngine->create_file_system( "itexturelibrarytest.utf", IFS ) ) ) {
		MessageBox( m_hWnd, "Failed to create file system for itexturelibrarytest.utf", "Error", MB_OK );
	}

	if( SUCCEEDED( m_ITextureLibrary->load_library( IFS ) ) ) {
		
		U32 tcount;
		m_ITextureLibrary->get_texture_count( &tcount );

		m_TextureRefs = new ITL_TEXTURE_REF_ID[tcount];
		m_NumTextureRefs = tcount;

		U32 r=0;
		for( U32 t=0; t<tcount; t++ ) {
			ITL_TEXTURE_ID tid;
			m_TextureRefs[t] = ITL_INVALID_REF_ID;
			if( FAILED( m_ITextureLibrary->get_texture( t, &tid ) ) ) {
				MessageBox( m_hWnd, "Failed to find texture", "Error", MB_OK );
			}

			U32 count;
			m_ITextureLibrary->get_texture_frame_count( tid, &count );
			if( count > 1 ) {
				if( FAILED( m_ITextureLibrary->add_texture_ref( tid, &m_TextureRefs[r] ) ) ) {
					MessageBox( m_hWnd, "Failed to get texture ref", "Error", MB_OK );
				}
				r++;
			}
		}
		m_ITextureLibrary->free_library( FALSE );
	}

	return S_OK;
}









//

HRESULT RegisterCDALibs_TextureLibraryTestWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_TextureLibraryTestWindow );
	if( T_DACOM_CreateInstance<CDALibs_TextureLibraryTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureLibraryTestWindow, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

GENRESULT CDALibs_TextureLibraryTestWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_TextureLibraryTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureLibraryTestWindow, desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_TextureLibraryTestWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_TextureLibraryTestWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_TextureLibraryTestWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_TextureLibraryTestWindow::CDALibs_TextureLibraryTestWindow( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
	m_IRenderPipe = NULL;
	m_hWnd = NULL;
	m_ISystem = creation_info._ISystem;
	m_IEngine = creation_info._IEngine;
	m_NumTextureRefs = 0;
	m_TextureRefs = NULL;
	m_StartTextureRef = 0;
	m_NumTextureRefsWide = 4;
	m_NumTextureRefsHigh = 4;
	m_SelectedTextureRefRow = 0;
	m_SelectedTextureRefCol = 0;
}

//

CDALibs_TextureLibraryTestWindow::~CDALibs_TextureLibraryTestWindow()
{
	Destroy();
}



