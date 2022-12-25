// EditField.cpp
//
//
//

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

//

#include "EditField.h"

//

struct ControlSubclass
{
	EditField *field;
	float field_scale;
	WNDPROC prev_wndproc;		
};

//

template <typename T>
inline T NumberEditField_Clamp( EditField *field, T value )
{
	if( (field->min != -1) && (value < (T)field->min) ) {
		return (T)field->min ;
	}
	else if( (field->max != -1) && (value > (T)field->max) ) {
		return (T)field->max ;
	}
	
	return value;
}

//

#pragma warning( push )
#pragma warning( disable :4244 )

inline long CALLBACK NumberEditField_WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	ControlSubclass *csc = NULL;
	float value, new_value;
	float scale;
	static bool control_down = false;
	static bool shift_down = false;
	static long last_x = 0, last_y = 0;
	long this_x, this_y, delta_y, min_y;
	HWND hDlg;

	if( (csc = (ControlSubclass*)GetWindowLong( hWnd, GWL_USERDATA )) == NULL ) {
//		__asm int 3;
		return DefWindowProc( hWnd, message, wParam, lParam );
	}

	hDlg = GetParent(hWnd);

	switch( message ) {

	case WM_KILLFOCUS:
		CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
		
		EditField_UpdateValueFromControl( hDlg, csc->field, EF_T_REAL, &value );
		value = NumberEditField_Clamp( csc->field, value );
		EditField_UpdateControlFromValue( hDlg, csc->field, EF_T_REAL, &value );
		SendMessage( hDlg, WM_EF_FIELD_CHANGED, csc->field->control_id, (LPARAM)hWnd );
		break;

	case WM_LBUTTONDOWN:
		CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
		SetCursor( LoadCursor( NULL, IDC_SIZENS ) );
		SetCapture( hWnd );
		last_x = LOWORD(lParam);
		last_y = HIWORD(lParam);
		break;

	case WM_LBUTTONUP:
		CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
		SetCursor( LoadCursor( NULL, IDC_ARROW ) );
		ReleaseCapture();
		break;

	case WM_KEYUP:
		switch( wParam ) {
		case VK_CONTROL:	control_down = false;	break;
		case VK_SHIFT:		shift_down = false;		break;
		}
		break;

	case WM_KEYDOWN:
		
		EditField_UpdateValueFromControl( hDlg, csc->field, EF_T_REAL, &value );
		EditField_UpdateValueFromControl( hDlg, csc->field, EF_T_REAL, &new_value );

		scale = 1.0f;
		if( control_down ) {
			scale *= 0.1f;
		}
		if( shift_down ) {
			scale *= 0.01f;
		}

		scale *= csc->field_scale;

		switch( wParam ) {

		case VK_CONTROL:	control_down = true;	break;
		case VK_SHIFT:		shift_down = true;		break;

		case VK_UP:			new_value += scale * 1.0f;	break;
		case VK_DOWN:		new_value -= scale * 1.0f;	break;
		case VK_PRIOR:		new_value += scale * 10.0f;	break;
		case VK_NEXT:		new_value -= scale * 10.0f;	break;

		default:
			return CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
		}

		new_value = NumberEditField_Clamp( csc->field, new_value );

		if( new_value != value ) {
			EditField_UpdateControlFromValue( hDlg, csc->field, EF_T_REAL, &new_value );
			SendMessage( hDlg, WM_EF_FIELD_CHANGED, csc->field->control_id, (LPARAM)hWnd );
		}
		break;

	case WM_MOUSEMOVE:
		
		if( GetCapture() == hWnd ) {

			this_x = (short)LOWORD(lParam);
			this_y = (short)HIWORD(lParam);

			delta_y = this_y - last_y;

			min_y = min( abs(this_x-last_x), 10 );

			if( abs(delta_y) < min_y ) {
				return CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
			}
			else {

				EditField_UpdateValueFromControl( hDlg, csc->field, EF_T_REAL, &value );

				scale = 1.0f;
				if( control_down ) {
					scale *= 0.1f;
				}
				if( shift_down ) {
					scale *= 0.01f;
				}

				scale *= csc->field_scale;

				new_value = value - 0.1f * scale * delta_y;
				
				new_value = NumberEditField_Clamp( csc->field, new_value );

				if( new_value != value ) {
					EditField_UpdateControlFromValue( hDlg, csc->field, EF_T_REAL, &new_value );
					SendMessage( hDlg, WM_EF_FIELD_CHANGED, csc->field->control_id, (LPARAM)hWnd );
				}
				
				last_y = this_y;
			}
		}
		break;

	default:
		return CallWindowProc( csc->prev_wndproc, hWnd, message, wParam, lParam );
	}

	return TRUE;
}

#pragma warning( pop )

//

bool EditField_UpdateStructFromControl( HWND hDlg, EditField *field, void *dst_struct )
{
	HWND hwnd;
	char text[MAX_PATH];
	long len;
	float f;
	long l;

	if( field->type == EF_T_BIT ) {

		if( IsDlgButtonChecked( hDlg, field->control_id ) ) {
			*((unsigned long*)(((char*)dst_struct) + field->offset)) |=  field->min;
		}
		else {
			*((unsigned long*)(((char*)dst_struct) + field->offset)) &= ~(field->min);
		}

		return true;
	}
	else {

		hwnd = GetDlgItem( hDlg, field->control_id );
		GetClassName( hwnd, text, sizeof(text) ); 
		
		if( lstrcmpi( text, "COMBOBOX" ) == 0 ) { 
			sprintf( text, "%d", ComboBox_GetItemData( hwnd, ComboBox_GetCurSel(hwnd) ) );
		}
		else if( (len = GetDlgItemText( hDlg, field->control_id, text, MAX_PATH )) == 0 ) {
			return false;
		}

		switch( field->type ) {
	
		case EF_T_REAL:		
			
			f = (float)atof( text );	
			
			if( (field->min != -1) && (f < (float)field->min) ) {
				f = (float)field->min;
			}
			else if( (field->max != -1) && (f > (float)field->max) ) {
				f = (float)field->max;
			}

			*((float *)(((char*)dst_struct) + field->offset)) = f;
			break;

		case EF_T_INT:
			l = atol( text );	

			if( (field->min != -1) && (l < field->min) ) {
				l = field->min;
			}
			else if( (field->max != -1) && (l > field->max) ) {
				l = field->max;
			}

			*((long *)(((char*)dst_struct) + field->offset)) = l;
			break;

		case EF_T_CSTR:
			len++;
			if( (len > field->min && len < field->max) ) {
				strcpy( ((char *)(((char*)dst_struct) + field->offset)), text );
				return true;
			}
			break;

		}
	}

	return true;
}

//

bool EditField_UpdateValueFromControl( HWND hDlg, EditField *field, EditFieldType ef_out_type, void *out_value )
{
	HWND hwnd;
	char text[MAX_PATH];
	long len;
	float f;
	long l;

	if( field->type == EF_T_BIT ) {

		if( ef_out_type != EF_T_BIT ) {
			return false;
		}

		if( IsDlgButtonChecked( hDlg, field->control_id ) ) {
			*((unsigned long*)(out_value)) |=  field->min;
		}
		else {
			*((unsigned long*)(out_value)) &= ~(field->min);
		}

		return true;
	}
	else {

		hwnd = GetDlgItem( hDlg, field->control_id );
		GetClassName( hwnd, text, sizeof(text) ); 
		
		if( lstrcmpi( text, "COMBOBOX" ) == 0 ) { 
			sprintf( text, "%d", ComboBox_GetItemData( hwnd, ComboBox_GetCurSel(hwnd) ) );
		}
		else if( (len = GetDlgItemText( hDlg, field->control_id, text, MAX_PATH )) == 0 ) {
			return false;
		}

		switch( field->type ) {
	
		case EF_T_REAL:		
			
			f = (float)atof( text );	
			
			if( (field->min != -1) && (f < (float)field->min) ) {
				f = (float)field->min;
			}
			else if( (field->max != -1) && (f > (float)field->max) ) {
				f = (float)field->max;
			}

			if( ef_out_type == EF_T_REAL ) {
				*((float *)(out_value)) = (float)f;
			}
			else {
				*((long *)(out_value)) = (long)f;
			}
			break;

		case EF_T_INT:
			l = atol( text );	

			if( (field->min != -1) && (l < field->min) ) {
				l = field->min;
			}
			else if( (field->max != -1) && (l > field->max) ) {
				l = field->max;
			}

			if( ef_out_type == EF_T_REAL ) {
				*((float *)(out_value)) = (float)l;
			}
			else {
				*((long *)(out_value)) = (long)l;
			}
			break;

		case EF_T_CSTR:

			if( ef_out_type != EF_T_CSTR ) {
				return false;
			}

			len++;

			if( (len > field->min && len < field->max) ) {
				strcpy( ((char *)(out_value)), text );
				return true;
			}
			break;

		}
	}

	return true;
}

//

bool EditField_UpdateControlFromStruct( HWND hDlg, EditField *field, void *src_struct )
{
	char text[MAX_PATH];

	if( field->type == EF_T_BIT ) {
		unsigned long check = *((unsigned long*)(((char*)src_struct) + field->offset)) & field->min;
		CheckDlgButton( hDlg, field->control_id, check? BST_CHECKED : BST_UNCHECKED );
	}
	else {
		HWND hwnd = GetDlgItem( hDlg, field->control_id );
		GetClassName( hwnd, text, sizeof(text) ); 
		
		if( lstrcmpi( text, "COMBOBOX" ) == 0 ) {	//setcursel counts from 0
			ComboBox_SetCurSel(hwnd, *((long *)(((char*)src_struct) + field->offset)) - 1);
		}
		else {
			switch( field->type ) {
		
			case EF_T_REAL:		
				sprintf( text, EF_FLOAT_FORMAT, *((float *)(((char*)src_struct) + field->offset)) );
				break;

			case EF_T_INT:
				sprintf( text, EF_LONG_FORMAT, *((long *)(((char*)src_struct) + field->offset)) );
				break;

			case EF_T_CSTR:
				strcpy( text, ((char *)(((char*)src_struct) + field->offset)) );
				break;

			}

			SetDlgItemText( hDlg, field->control_id, text );
		}
	}

	return true;
}

//

bool EditField_UpdateControlFromValue( HWND hDlg, EditField *field, EditFieldType ef_src_type, void *src_value )
{
	char text[MAX_PATH];

	if( field->type == EF_T_BIT ) {
		CheckDlgButton( hDlg, field->control_id, (src_value!=NULL)? BST_CHECKED : BST_UNCHECKED );
	}
	else {
		switch( field->type ) {
	
		case EF_T_REAL:	
			if( ef_src_type == EF_T_REAL ) {
				sprintf( text, EF_FLOAT_FORMAT, *((float *)(src_value)) );
			}
			else {
				sprintf( text, EF_FLOAT_FORMAT, *((long *)(src_value)) );
			}
			break;

		case EF_T_INT:
			if( ef_src_type == EF_T_REAL ) {
				sprintf( text, EF_LONG_FORMAT, (long)(*((float *)(src_value))) );
			}
			else {
				sprintf( text, EF_LONG_FORMAT, *((long *)(src_value)) );
			}
			break;

		case EF_T_CSTR:
			strcpy( text, ((char *)(src_value)) );
			break;

		}

		SetDlgItemText( hDlg, field->control_id, text );
	}

	return true;
}

//

bool EditField_Attach( HWND hDlg, EditField *field, float field_scale )
{
	char szClass[64];
	HWND hChild;
	ControlSubclass *csc;

	if( (hChild = GetDlgItem( hDlg, field->control_id )) == 0 ) {
		return false;
	}
 
    GetClassName( hChild, szClass, sizeof(szClass) ); 
	if( lstrcmpi( szClass, "EDIT" ) != 0 ) { 
		return false;
	} 

	if( (csc = new ControlSubclass()) == NULL ) {
		return false;
	}

	csc->field = field;
	csc->field_scale = field_scale;
	csc->prev_wndproc = (WNDPROC)GetWindowLong( hChild, GWL_WNDPROC ) ;

	SetWindowLong( hChild, GWL_USERDATA, (LONG)csc );
	SetWindowLong( hChild, GWL_WNDPROC, (LONG)NumberEditField_WndProc );

	return true;
}

//

bool EditField_Detach( HWND hDlg, EditField *field )
{
	char szClass[64];
	HWND hChild;
	ControlSubclass *csc;

	if( (hChild = GetDlgItem( hDlg, field->control_id )) == 0 ) {
		return false;
	}
 
    GetClassName( hChild, szClass, sizeof(szClass) ); 
	if( lstrcmpi( szClass, "EDIT" ) != 0 ) { 
		return false;
	} 

	if( (csc = (ControlSubclass*)GetWindowLong( hChild, GWL_USERDATA )) == NULL ) {
		return false;
	}

	SetWindowLong( hChild, GWL_WNDPROC, (LONG)csc->prev_wndproc );

	return true;	
}

//

