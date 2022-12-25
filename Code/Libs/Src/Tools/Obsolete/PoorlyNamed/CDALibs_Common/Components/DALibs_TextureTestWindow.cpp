// DALibs_TextureTestWindow.cpp: implementation of the 
// CDALibs_TextureTestWindow class.
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



#define DACOM_COMPONENT_NAME CDALibs_TextureTestWindow

const char *CLSID_CDALibs_TextureTestWindow = "CDALibs_TextureTestWindow";

dacom_component CDALibs_TextureTestWindow :	dacom_implements IWindow,
											dacom_implements IComponentFactory
{
	BEGIN_STATIC_WP_MAPS(CDALibs_TextureTestWindow)
//		BEGIN_COMMAND_MAP
//		END_COMMAND_MAP
		BEGIN_MESSAGE_MAP
			ON_MESSAGE(WM_PAINT,OnPaint)
			ON_MESSAGE(WM_LBUTTONUP,OnPaint)
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
	CDALibs_TextureTestWindow( CLSID_DACOMDESC &creation_info );
	~CDALibs_TextureTestWindow();

protected:	// Interface
	HRESULT CreateTextures( );
	HRESULT RenderTextures();
	void Refresh( void );


protected:	// Private Data
//	COMPTR<ICOManager>				m_ICOManager;
	COMPTR<ISystemContainer>		m_ISystem;
	COMPTR<IRenderPipeline>			m_IRenderPipe;

	HWND							m_hWnd;
	RECT							m_ClientRect;

	RPFont							m_Font;

#define NUM_TEXTURES 256
	U32 textures[NUM_TEXTURES];

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

	WNDCLASS wc = {
		CS_HREDRAW|CS_VREDRAW,
		CDALibs_TextureTestWindow::HandleMessage,
		0,
		0,
		cd->hModuleInstance,
		NULL,
		NULL,
		(HBRUSH)GetStockObject( LTGRAY_BRUSH ),
		NULL,
		CLSID_CDALibs_TextureTestWindow
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


	m_hWnd = CreateWindowEx( 0, CLSID_CDALibs_TextureTestWindow, CLSID_CDALibs_TextureTestWindow, style,
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

void CDALibs_TextureTestWindow::Refresh( void )
{
	if( !m_IRenderPipe ) {
		return;
	}

	PrimitiveBuilder pb( m_IRenderPipe );

	m_IRenderPipe->set_window( m_hWnd, 0, 0, m_ClientRect.right, m_ClientRect.bottom );
	m_IRenderPipe->clear_buffers( RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT, NULL );
	m_IRenderPipe->begin_scene();
	RenderTextures( );
	m_IRenderPipe->end_scene();
	m_IRenderPipe->swap_buffers();
	return;
}

//

HRESULT CDALibs_TextureTestWindow::CreateTextures( )
{
	U32 dims[5] = { 128, 128, 128, 64, 64 };
	U32 lods[5] = {  5,  5,  5,  5,  5 };
	U32 d, h;

	for( U32 t=0; t<NUM_TEXTURES; t++ ) {
		d = rand() % 5;

		if( FAILED( m_IRenderPipe->create_texture( dims[d], dims[d], PixelFormat(16,5,6,5,0), lods[d], h ) ) ) {
			return E_FAIL;
		}

		PixelFormat pf;
		
		m_IRenderPipe->get_texture_format( h, &pf );

		char *bmp_bits;
		U32 bmp_w=0, bmp_h=0, bmp_p=0;
		char txt[20];

		bmp_w = dims[d];
		bmp_h = dims[d];
		for( U32 l=0; l<dims[d]; l++ ) {

			// Create bitmap with 'h' on it
			HDC hDC = GetDC( m_hWnd );
			HDC hMemDC = CreateCompatibleDC( hDC );
			HBITMAP hBmp = CreateCompatibleBitmap( hDC, bmp_w, bmp_h );
			ReleaseDC( m_hWnd, hDC );
			SetMapMode( hMemDC, MM_TEXT );
			SetTextAlign( hMemDC, TA_CENTER );
			HGDIOBJ hold = SelectObject( hMemDC, hBmp );
			PatBlt( hMemDC, 0, 0, bmp_w, bmp_h, WHITENESS );
			sprintf( txt, "%d", h );
			TextOut( hMemDC, bmp_w/2, bmp_h/2, txt, strlen(txt) );
			SelectObject( hMemDC, hold );
				
			bmp_bits = new char[bmp_w*bmp_h*4];
			
			BITMAPINFO bmpi;
			memset( &bmpi, 0, sizeof(bmpi) );
			bmpi.bmiHeader.biSize = sizeof(bmpi.bmiHeader);
			bmpi.bmiHeader.biWidth = bmp_w,
			bmpi.bmiHeader.biHeight = bmp_h,
			bmpi.bmiHeader.biPlanes = 1,
			bmpi.bmiHeader.biBitCount = 24;
			bmpi.bmiHeader.biCompression = BI_RGB;

			if( GetDIBits( hMemDC, hBmp, 0, bmp_h, bmp_bits, &bmpi, DIB_RGB_COLORS ) == 0 ) {
				U32 foo = GetLastError();
			}

			// transfer it to the texture
			if( FAILED( m_IRenderPipe->set_texture_level_data( h, l, pf, bmp_w, bmp_h, bmp_w*3, PixelFormat(24,8,8,8,0), bmp_bits, NULL, NULL ) ) ) {
				return E_FAIL;
			}

			// release bitmap
			delete bmp_bits;
			DeleteDC( hMemDC );
			DeleteObject( hBmp );

			bmp_w >>= 1;
			bmp_h >>= 1;
			if( bmp_w == 1 || bmp_h == 1 ) {
				break;
			}

		} // for each l in lod
		textures[t] = h;
	}

	return S_OK;
}

//

HRESULT CDALibs_TextureTestWindow::RenderTextures()
{
	PrimitiveBuilder pb(m_IRenderPipe);
	m_IRenderPipe->set_ortho( 0, m_ClientRect.right, m_ClientRect.bottom, 0 );
	m_IRenderPipe->set_modelview( Transform() );
	
	float ntw = 8;	// num textures across
	float nth = 8;	// num textures down
	float npb = 20;	// num pixels border (one side)
	float tw = (m_ClientRect.right - ((ntw+1)*npb)) / ntw;
	float th = (m_ClientRect.bottom - ((nth+1)*npb)) / nth;
	float x;
	float y, yt;

	m_IRenderPipe->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
	
	y = npb;
	yt = 2;
	for( U32 h=0; h<nth; h++ ) {
		x = npb;
		for( U32 w=0; w<ntw; w++ ) {
			U32 txm = textures[rand() % NUM_TEXTURES];
			m_IRenderPipe->set_render_state( D3DRS_TEXTUREHANDLE, txm );
			pb.Begin( GL_QUADS );
				pb.TexCoord2f( 0, 1 );	pb.Vertex3f( x,    y,    0 );
				pb.TexCoord2f( 1, 1 );	pb.Vertex3f( x+tw, y,    0 );
				pb.TexCoord2f( 1, 0 );	pb.Vertex3f( x+tw, y+th, 0 );
				pb.TexCoord2f( 0, 0 );	pb.Vertex3f( x,    y+th, 0 );
			pb.End();

			m_Font.RenderFormattedString( x, yt, "%d", txm );
			x += tw + npb;
		}

		y += th + npb;
		yt += th + npb;
	}

	return S_OK;
}

//

HRESULT RegisterCDALibs_TextureTestWindow( ICOManager *dacom )
{
	COMPTR<IDAComponent> IDAC;
	CLSID_DACOMDESC desc( CLSID_CDALibs_TextureTestWindow );
	if( T_DACOM_CreateInstance<CDALibs_TextureTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureTestWindow, &desc, (IDAComponent **) &IDAC ) == S_OK ) {
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

GENRESULT CDALibs_TextureTestWindow::CreateInstance( DACOMDESC *desc, void **instance )
{
	return T_DACOM_CreateInstance<CDALibs_TextureTestWindow,CLSID_DACOMDESC>( CLSID_CDALibs_TextureTestWindow, desc, (IDAComponent **) instance );
}

//

GENRESULT CDALibs_TextureTestWindow::QueryInterface( const C8 *interface_name, void **instance)
{
	DACOM_QUERYINTERFACE_BEGIN(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IComponentFactory)
	DACOM_QUERYINTERFACE_ENTRY(instance, IWindow)
	DACOM_QUERYINTERFACE_END(instance)
}

//

U32 CDALibs_TextureTestWindow::AddRef(void)
{
	m_RefCnt++;
	return 0;
}

//

U32 CDALibs_TextureTestWindow::Release(void)
{
	m_RefCnt--;
	if( m_RefCnt <= 0 ) {
		delete this;
	}
	return 0;
}

//

CDALibs_TextureTestWindow::CDALibs_TextureTestWindow( CLSID_DACOMDESC &creation_info )
{
	m_RefCnt = 0;
	m_IRenderPipe = NULL;
	m_hWnd = NULL;
	m_ISystem = creation_info._ISystem;
}

//

CDALibs_TextureTestWindow::~CDALibs_TextureTestWindow()
{
	Destroy();
}


