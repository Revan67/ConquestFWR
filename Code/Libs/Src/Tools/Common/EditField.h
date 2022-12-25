// EditField.h
//
//
//
//

#ifndef __EditField_h__
#define __EditField_h__

//

#include <stddef.h>

//

#define EF_FLOAT_FORMAT	"%7.4f"
#define EF_LONG_FORMAT "%d"

//

#define WM_EF_FIELD_CHANGED (WM_USER+100)


//

typedef enum {
	EF_T_NULL,
	EF_T_BIT,
	EF_T_REAL,
	EF_T_CSTR,
	EF_T_INT
} EditFieldType;

//

union EditFieldValue
{
	void *null_value;
	unsigned long bit_field;
	float real_value;
	unsigned long cstr_len;
	long long_value;
};

//

struct EditField
{
	unsigned long control_id;	// windows dialog control id
	EditFieldType type;			// type of data
	unsigned long offset;		// offset of the field from the beginning of a PSP
	long min;					// if min > max, then min is ignored
	long max;					// if max < min, then max is ignored
								// if min == max, then both are ignored
};


//

bool EditField_Attach( HWND hDlg, EditField *field, float field_scale );
bool EditField_Detach( HWND hDlg, EditField *field );

bool EditField_UpdateStructFromControl( HWND hDlg, EditField *field, void *dst_struct );
bool EditField_UpdateControlFromStruct( HWND hDlg, EditField *field, void *src_struct );

bool EditField_UpdateValueFromControl( HWND hDlg, EditField *field, EditFieldType ef_out_type, void *out_value );
bool EditField_UpdateControlFromValue( HWND hDlg, EditField *field, EditFieldType ef_src_type, void *src_value );


//

#endif