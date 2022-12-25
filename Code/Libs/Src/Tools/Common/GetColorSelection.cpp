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

#define IDC_GCS_SLIDER_R                1212
#define IDC_GCS_SLIDER_G                1213
#define IDC_GCS_SLIDER_B				1214
#define IDC_GCS_SLIDER_A				1215
#define IDC_GCS_R						1216
#define IDC_GCS_G						1217
#define IDC_GCS_B						1218
#define IDC_GCS_A						1219
#define IDC_GCS_COLOR                   1220

//

#define GCS_MIN	0
#define GCS_MAX 255

//

BOOL CALLBACK gcs_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

//

struct gcs_parameters
{
	const char *title;
	float *out_color;
} ;

//

#pragma warning( push )
#pragma warning( disable:4800 )
bool GetColorSelection( HWND hParent, LPCTSTR DlgTemplate, const char *Title, float *out_color )
{
	gcs_parameters params;

	params.out_color = out_color;
	params.title = Title;
	return DialogBoxParam( GetModuleHandle(NULL), DlgTemplate, hParent, gcs_window_proc, (LPARAM)&params );
}
#pragma warning( pop )

//

BOOL CALLBACK gcs_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	static gcs_parameters *params = NULL;
	static float color[4];
	static HBRUSH hbr = 0;
	static HWND hColor = 0;

	char buffer[64+1];

	switch( msg ) {
	
	case WM_INITDIALOG:

		params = (gcs_parameters *)lParam;

		hColor = GetDlgItem( hDlg, IDC_GCS_COLOR );

		SetWindowText( hDlg, params->title );

		color[0] = params->out_color[0];
		color[1] = params->out_color[1];
		color[2] = params->out_color[2];
		color[3] = params->out_color[3];

		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_SETRANGEMIN, 0,GCS_MIN );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_SETRANGEMAX, 0, GCS_MAX );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_SETPOS, 0, ((int)(color[0] * GCS_MAX)) );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_SETTICFREQ, 16, 0);

		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_SETRANGEMIN, 0,GCS_MIN );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_SETRANGEMAX, 0, GCS_MAX );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_SETPOS, 0, ((int)(color[1] * GCS_MAX)) );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_SETTICFREQ, 16, 0);

		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_SETRANGEMIN, 0,GCS_MIN );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_SETRANGEMAX, 0, GCS_MAX );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_SETPOS, 0, ((int)(color[2] * GCS_MAX)) );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_SETTICFREQ, 16, 0);

		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_SETRANGEMIN, 0,GCS_MIN );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_SETRANGEMAX, 0, GCS_MAX );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_SETPOS, 0, ((int)(color[3] * GCS_MAX)) );
		SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_SETTICFREQ, 16, 0);

		// NOTE lack of break

	case WM_HSCROLL:
		color[0] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_GETPOS, 0, 0 )) / GCS_MAX;
		color[1] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_GETPOS, 0, 0 )) / GCS_MAX;
		color[2] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_GETPOS, 0, 0 )) / GCS_MAX;
		color[3] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_GETPOS, 0, 0 )) / GCS_MAX;

		sprintf( buffer, "%5.3f (%d)", color[0], (int)(color[0] * GCS_MAX) );
		SetDlgItemText( hDlg, IDC_GCS_R, buffer );

		sprintf( buffer, "%5.3f (%d)", color[1], (int)(color[1] * GCS_MAX) );
		SetDlgItemText( hDlg, IDC_GCS_G, buffer );

		sprintf( buffer, "%5.3f (%d)", color[2], (int)(color[2] * GCS_MAX) );
		SetDlgItemText( hDlg, IDC_GCS_B, buffer );

		sprintf( buffer, "%5.3f (%d)", color[3], (int)(color[3] * GCS_MAX) );
		SetDlgItemText( hDlg, IDC_GCS_A, buffer );

		InvalidateRect( hColor, NULL, FALSE );
		UpdateWindow( hColor );
		break;

	case WM_CTLCOLORSTATIC:
		if( ((HWND)lParam) == hColor ) {
			
			COLORREF current_color = ((int(color[2] * 255.0))&0xFF)<<16	| 
									 ((int(color[1] * 255.0))&0xFF)<<8	| 
									 ((int(color[0] * 255.0))&0xFF)<<0	;
			if( hbr != 0 ) {
				DeleteObject( hbr );
				hbr = 0;
			}
			
			return (BOOL)(hbr = CreateSolidBrush( current_color ));
		}
		break;

	case WM_COMMAND:
		if( LOWORD(wParam) == IDOK ) {
			
			params->out_color[0] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_R, TBM_GETPOS, 0, 0 )) / GCS_MAX;
			params->out_color[1] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_G, TBM_GETPOS, 0, 0 )) / GCS_MAX;
			params->out_color[2] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_B, TBM_GETPOS, 0, 0 )) / GCS_MAX;
			params->out_color[3] = ((float)SendDlgItemMessage( hDlg, IDC_GCS_SLIDER_A, TBM_GETPOS, 0, 0 )) / GCS_MAX;

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