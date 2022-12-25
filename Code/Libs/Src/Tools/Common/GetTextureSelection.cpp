//
//
//
//
//

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <commctrl.h>

//

#include "ITextureLibrary.h"
#include "RendPipeline.h"
#include "RPUL.h"

//

#define IDC_GTS_LIST         1223
#define IDC_GTS_DESCRIPTION  1226
#define IDC_GTS_TEXTURE      1227

//

#define GTS_TEXTURE_BORDER_WIDTH 8
//

BOOL CALLBACK gts_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

//

struct gts_parameters
{
	const char *title;
	ITextureLibrary *texturelibrary;
	IRenderPipeline *renderpipeline;
	char *out_texture;
} ;

//

#pragma warning( push )
#pragma warning( disable:4800 )

bool GetTextureSelection( HWND hParent, LPCTSTR DlgTemplate, const char *Title, ITextureLibrary *texturelibrary, IRenderPipeline *renderpipe, char *out_texture, U32 max_texture_len )
{
	gts_parameters params;

	if( texturelibrary == NULL ) {
		return false;
	}

	if( renderpipe == NULL ) {
		return false;
	}

	texturelibrary->AddRef();
	renderpipe->AddRef();

	params.texturelibrary = texturelibrary;
	params.renderpipeline = renderpipe;
	params.out_texture = out_texture;
	params.title = Title;

	int ret = DialogBoxParam( GetModuleHandle(NULL), DlgTemplate, hParent, gts_window_proc, (LPARAM)&params );

	texturelibrary->Release();
	renderpipe->Release();

	return ret;
}

#pragma warning( pop )

//

//

BOOL CALLBACK gts_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static gts_parameters *params = NULL;
	static HWND hTexture = 0;
	static HWND hList = 0;

	char buffer[64+1];
	U32 tc,tn, fc ;
	float fr;
	U32 rp_texture_id;
	ITL_TEXTURE_ID tid;
	ITL_TEXTUREFRAME_IRP tlframe;
	RECT rect;
	PAINTSTRUCT ps;

	switch( msg ) {
	
	case WM_INITDIALOG:

		params = (gts_parameters *)lParam;
		SetWindowText( hDlg, params->title );

		hTexture = GetDlgItem( hDlg, IDC_GTS_TEXTURE );
		hList = GetDlgItem( hDlg, IDC_GTS_LIST );

		// Add textures
		//
		params->texturelibrary->get_texture_count( &tc );
		
		ComboBox_ResetContent( hList );
		
		for( tn=0; tn<tc; tn++ ) {
			params->texturelibrary->get_texture( tn, &tid );
			params->texturelibrary->get_texture_name( tid, buffer, 64 );
			ComboBox_SetItemData( hList, ComboBox_AddString( hList, buffer ), tid );
		}

		ComboBox_SetCurSel( hList, ComboBox_FindString( hList, 0, params->out_texture ) );

		// Update selected texture data
		//
		tid = ComboBox_GetItemData( hList, ComboBox_GetCurSel( hList ) );
		if( FAILED( params->texturelibrary->get_texture_frame_count( tid, &fc ) ) ) {
			fc = 0;
		}
		if( FAILED( params->texturelibrary->get_texture_frame_rate( tid, &fr ) ) ) {
			fr = 0.0f;
		}
		sprintf( buffer, "FrameCount: %d\nFrameRate: %5.3f\n", fc, fr );
		SetDlgItemText( hDlg, IDC_GTS_DESCRIPTION, buffer );

		break;

	case WM_PAINT:
		BeginPaint( hDlg, &ps );
		EndPaint( hDlg, &ps );

		UpdateWindow( hTexture );

#if 1
		tid = ComboBox_GetItemData( hList, ComboBox_GetCurSel( hList ) );

		if( SUCCEEDED( params->texturelibrary->get_texture_frame( tid, ITL_FRAME_FIRST, &tlframe ) ) ) {
			rp_texture_id = tlframe.rp_texture_id;
		}
		else{
			rp_texture_id = 0;
		}

		{
			GetClientRect( hTexture, &rect );
			
			float x0 = GTS_TEXTURE_BORDER_WIDTH;
			float y0 = GTS_TEXTURE_BORDER_WIDTH;
			float x1 = rect.right - GTS_TEXTURE_BORDER_WIDTH;
			float y1 = rect.bottom - GTS_TEXTURE_BORDER_WIDTH;
			
			params->renderpipeline->set_window( hTexture, 0, 0, rect.right, rect.bottom );
			params->renderpipeline->set_viewport( 0, 0, rect.right, rect.bottom );

			params->renderpipeline->clear_buffers( RP_CLEAR_COLOR_BIT, NULL );

			params->renderpipeline->begin_scene();

			params->renderpipeline->set_ortho( 0, rect.right, rect.bottom, 0 );
			params->renderpipeline->set_modelview( Transform() );

			params->renderpipeline->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE );
			params->renderpipeline->set_render_state( D3DRS_ZENABLE, FALSE );

			params->renderpipeline->set_texture_stage_texture( 0, rp_texture_id );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
			params->renderpipeline->set_texture_stage_state( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
			
			params->renderpipeline->set_texture_stage_texture( 1, 0 );
			params->renderpipeline->set_texture_stage_state( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
			params->renderpipeline->set_texture_stage_state( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

			PrimitiveBuilder pb( params->renderpipeline, 6 );
			pb.Begin( PB_QUADS );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u0, tlframe.v0 );	pb.Vertex3f( x0, y0, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u0, tlframe.v1 );	pb.Vertex3f( x0, y1, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u1, tlframe.v1 );	pb.Vertex3f( x1, y1, 0.0f );
				pb.Color3f( 1, 1, 1 );	pb.TexCoord2f( tlframe.u1, tlframe.v0 );	pb.Vertex3f( x1, y0, 0.0f );
			pb.End();
			
			params->renderpipeline->end_scene();
			params->renderpipeline->swap_buffers();
		}
#endif
		break;

	case WM_DESTROY:
		tc = ComboBox_GetCount( hList );
		for( tn=0; tn<tc; tn++ ) {
			tid = ComboBox_GetItemData( hList, tn );
			params->texturelibrary->release_texture_id( tid );
		}
		break;

	case WM_COMMAND:
		if( LOWORD(wParam) == IDC_GTS_LIST ) {
			if( HIWORD(wParam) == CBN_SELCHANGE ) {
				// Update selected texture data
				//
				tid = ComboBox_GetItemData( hList, ComboBox_GetCurSel( hList ) );
				if( FAILED( params->texturelibrary->get_texture_frame_count( tid, &fc ) ) ) {
					fc = 0;
				}
				if( FAILED( params->texturelibrary->get_texture_frame_rate( tid, &fr ) ) ) {
					fr = 0.0f;
				}
				sprintf( buffer, "FrameCount: %d\nFrameRate: %5.3f\n", fc, fr );
				SetDlgItemText( hDlg, IDC_GTS_DESCRIPTION, buffer );

				InvalidateRect( hDlg, NULL, FALSE );
				UpdateWindow( hDlg );
			}
		}
		else if( LOWORD(wParam) == IDOK ) {

			tid = ComboBox_GetItemData( hList, ComboBox_GetCurSel( hList ) );
			params->texturelibrary->get_texture_name( tid, params->out_texture, 64 );

			EndDialog( hDlg, 1 );
		}
		else if( LOWORD(wParam) == IDCANCEL ) {
			EndDialog( hDlg, 0 );
		}
		return TRUE;

	}
	return FALSE;
}

//