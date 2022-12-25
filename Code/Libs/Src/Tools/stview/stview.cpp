//-----------------------------------------------------------------------------
// File: stview.cpp
//
// Desc: Simple D3D viewer for .st (IRP Scene Trace) files.
//
// BUG: currently we don't handle textures changing values within a frame
//
//-----------------------------------------------------------------------------

#define STRICT
#include <stdio.h>
#include <math.h>

#include "D3DX.h"
#include "D3DApp.h"
#include "D3DTextr.h"
#include "D3DUtil.h"
#include "D3DMath.h"

#include "resource.h"
#include "stfunctions.h"


//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class provides just about all the
//       functionality we want, so we're just supplying stubs to interface with
//       the non-C++ functions of the app.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();

public:
    CMyD3DApplication();

    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
{
    CMyD3DApplication d3dApp;

    if( FAILED( d3dApp.Create( hInst, strCmdLine ) ) )
        return 0;

    d3dApp.Run();

	st_cleanup();

	return 0;
}


//-----------------------------------------------------------------------------
// 
//
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{

	switch( uMsg ) {

	case WM_CREATE:
		DragAcceptFiles( hWnd, TRUE );
		break;

	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			UINT num_files;
			char filename[MAX_PATH], title[MAX_PATH+32];

			num_files = DragQueryFile( hDrop, 0xFFFFFFFF, NULL, 0 );

			for( UINT f=0; f<num_files; f++ ) {
				DragQueryFile( hDrop, f, filename, MAX_PATH );
				sprintf( title, "%s - stview", filename );
				SetWindowText( hWnd, title );
				st_load_file( filename );
			}

			st_init_scene( m_hWnd, m_pd3dDevice );

			DragFinish( hDrop );
		}
		break;
		
	case WM_COMMAND: 
		if( LOWORD(wParam) == ID_FILE_OPEN ) {
			
			char filename[MAX_PATH];
			OPENFILENAME ofn;

			filename[0] = 0;

			memset( &ofn, 0, sizeof(ofn) );
			ofn.lStructSize = sizeof(ofn);

			ofn.hwndOwner = hWnd;
			ofn.hInstance = GetModuleHandle(NULL);
			ofn.lpstrFilter = "Scene Trace Files (*.st;*.stc)\0*.st;*.stc\0All Files (*.*)\0*.*\0\0";
			ofn.lpstrFile = filename;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = "Open File To View";
			ofn.Flags = OFN_FILEMUSTEXIST | 
						OFN_LONGNAMES | 
						0;

			if( GetOpenFileName( &ofn ) ) {
				char title[MAX_PATH+32];
				sprintf( title, "%s - stview", filename );
				SetWindowText( hWnd, title );
				st_load_file( filename );
				st_init_scene( m_hWnd, m_pd3dDevice );
			}
		}
		break;

	}

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication() : CD3DApplication()
{
    m_strWindowTitle	= TEXT( "stviewer" );
    m_bAppUseZBuffer	= TRUE;
    m_bAppUseStereo		= TRUE;
    m_bShowStats		= TRUE;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: App_Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
	m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0 );

	// Restore view matrix from stats render
	//
	D3DMATRIX M;
	ZeroMemory( &M, sizeof(M) );
	M._11 = M._22 = M._33 = M._44 = 1.0f;
	m_pd3dDevice->SetTransform( D3DTRANSFORMSTATE_VIEW, &M );

	st_render_scene( m_pd3dDevice );

	/*
	const float s = 100.0f;

	D3DLVERTEX axis[6] = {
		D3DLVERTEX( D3DVECTOR(0, 0, 0), 0xffff0000, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(s, 0, 0), 0xffff0000, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(0, 0, 0), 0xff00ff00, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(0, s, 0), 0xff00ff00, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(0, 0, 0), 0xff0000ff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(0, 0, s), 0xff0000ff, 0, 0.0f, 1.0f )
	};

	const float q = 10.0f;

	D3DLVERTEX quad[6] = {
		D3DLVERTEX( D3DVECTOR(-q,  q, 0), 0xffffffff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(-q, -q, 0), 0xffffffff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR( q, -q, 0), 0xffffffff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR( q, -q, 0), 0xffffffff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR( q,  q, 0), 0xffffffff, 0, 0.0f, 1.0f ),
		D3DLVERTEX( D3DVECTOR(-q,  q, 0), 0xffffffff, 0, 0.0f, 1.0f )
	};

	m_pd3dDevice->BeginScene();
	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	m_pd3dDevice->DrawPrimitive( D3DPT_LINELIST, D3DFVF_LVERTEX, axis, 6, 0 ) ;
	m_pd3dDevice->EndScene();
	*/

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
	D3DXInitialize();

	st_init( );

	st_init_scene( m_hWnd, m_pd3dDevice );

    D3DTextr_RestoreAllTextures( m_pd3dDevice );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exitting, or the device is being changed,
//       this function deletes any device dependant objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
	D3DXUninitialize();

    D3DTextr_InvalidateAllTextures();

	st_cleanup();

	return S_OK;
}



