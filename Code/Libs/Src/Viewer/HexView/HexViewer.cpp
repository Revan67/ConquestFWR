//---------------------------------------------------------------------------
//
// HexViewer.CPP
//
//
//---------------------------------------------------------------------------

/*

	Simple viewer of binary data.

*/

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include "HexViewer.h"
#include "Document.h"
#include "IConnection.h"
#include "TSmartPointer.h"

#include <stdlib.h>


#include "resource.h"
#define IDC_EDIT1		0x00000401		// child edit box control ID

//---------------------------------------------------------------------------

HINSTANCE hInstance = 0;		// initialized in some other module

static char InterfaceName[] = "IViewer";
static char ClassName[] = "HexViewer";

#define DISPLAY_BINARY	'b'
#define DISPLAY_DECIMAL	'd'
#define DISPLAY_HEX		'h'
#define DISPLAY_FLOAT	'f'

	enum DisplayFlags
	{
		DISPLAY_ACTIVE=1,
		DISPLAY_ASCII=2,
		DISPLAY_BIG_ENDIAN=4,
		DISPLAY_SIGNED=8,
		DISPLAY_OFFSET=16,
		DISPLAY_FIXED_WIDTH=32,
	};


typedef signed char		int8;
typedef signed short	int16;
typedef signed long		int32;

//---------------------------------------------------------------------------
// print_numbers()
//---------------------------------------------------------------------------

#define WIDTH_DECIMAL1		3			// 255
#define FORMAT_DECIMAL1		"%03u "
#define FORMAT_sDECIMAL1	"%04d "

#define WIDTH_DECIMAL2		5			// 65535
#define FORMAT_DECIMAL2		"%05u "
#define FORMAT_sDECIMAL2	"%06d "

#define WIDTH_DECIMAL4		10			// 4294967295
#define FORMAT_DECIMAL4		"%10u "
#define FORMAT_sDECIMAL4	"%11d "

#define WIDTH_FLOAT4		10			// 1234567.90
#define FORMAT_FLOAT4		"%10f "

#define WIDTH_FLOAT8		16			// 123456789.123456
#define FORMAT_FLOAT8		"%16f "


char HexDigit[16+1] = "0123456789ABCDEF";

char ViewChar[128+1] =
"................"	// 0x00
"................"	// 0x10
" ..............."	// 0x20
"0123456789......"	// 0x30
"@ABCDEFGHIJKLMNO"	// 0x40
"PQRSTUVWXYZ....."	// 0x50
".abcdefghijklmno"	// 0x60
"pqrstuvwxyz....."	// 0x70
;

char *print_numbers (char *dst,
					 void *data, int count,
					 int type, int bytes,
					 int display_flags)
{
	int i;
	switch (type)
	{
//		case DISPLAY_BINARY:
//		break; // BINARY

		default:
		case DISPLAY_HEX:

switch (bytes)
{
	default:
	case 1:
	{
		BYTE *ptr = (BYTE *)data;

		for(i=0; i<count; i++)
		{
			BYTE b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				char s;
				if (b > 0x7F)
				{
					b = -b;
					s = '-';
				}
				else
				{
					s = '+';
				}
				*dst++ = s;
			}
			dst[0] = HexDigit[b>>4];
			dst[1] = HexDigit[b&15];
			dst[2] = ' ';
			dst+=3;
		}
	}
	break;

	case 2:
	{
		WORD *ptr = (WORD *)data;
		for(i=0; i<count; i++)
		{
			WORD b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				char s;
				if (b > 0x7FFF)
				{
					b = -b;
					s = '-';
				}
				else
				{
					s = '+';
				}
				*dst++ = s;
			}
			dst[0] = HexDigit[(b>>12)&0xF];
			dst[1] = HexDigit[(b>> 8)&0xF];
			dst[2] = HexDigit[(b>> 4)&0xF];
			dst[3] = HexDigit[(b>> 0)&0xF];
			dst[4] = ' ';
			dst+=5;
		}
	}
	break;

	case 4:
	{
		DWORD *ptr = (DWORD *)data;
		for(i=0; i<count; i++)
		{
			DWORD b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				char s;
				if (b > 0x7FFFFFFF)
				{
					b = -(signed long)b;
					s = '-';
				}
				else
				{
					s = '+';
				}
				*dst++ = s;
			}
			dst[0] = HexDigit[(b>>28)&0xF];
			dst[1] = HexDigit[(b>>24)&0xF];
			dst[2] = HexDigit[(b>>20)&0xF];
			dst[3] = HexDigit[(b>>16)&0xF];
			dst[4] = HexDigit[(b>>12)&0xF];
			dst[5] = HexDigit[(b>> 8)&0xF];
			dst[6] = HexDigit[(b>> 4)&0xF];
			dst[7] = HexDigit[(b>> 0)&0xF];
			dst[8] = ' ';
			dst+=9;
		}
	}
	break;
} // switch bytes

		break; // HEX


		case DISPLAY_DECIMAL:

switch (bytes)
{
	default:
	case 1:
	{
		BYTE *ptr = (BYTE *)data;
		for(i=0; i<count; i++)
		{
			BYTE b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				int32 v = ((int8*)ptr)[i];
				sprintf(dst,FORMAT_sDECIMAL1,v);
				if (v >= 0) dst[0] = '+';
				dst += 1;
			}
			else
				sprintf(dst,FORMAT_DECIMAL1,b);	// max=255
			dst+=WIDTH_DECIMAL1+1;
		}
	}
	break;

	case 2:
	{
		WORD *ptr = (WORD *)data;
		for(i=0; i<count; i++)
		{
			WORD b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				int32 v = ((int16*)ptr)[i];
				sprintf(dst,FORMAT_sDECIMAL2,v);
				if (v >= 0) dst[0] = '+';
				dst += 1;
			}
			else
				sprintf(dst,FORMAT_DECIMAL2,b);		// max=65535
			dst+=WIDTH_DECIMAL2+1;
		}
	}
	break;

	case 4:
	{
		DWORD *ptr = (DWORD *)data;
		for(i=0; i<count; i++)
		{
			DWORD b = ptr[i];
			if (display_flags & DISPLAY_SIGNED)
			{
				int32 v = ((int32*)ptr)[i];
				sprintf(dst,FORMAT_sDECIMAL4,v);
				if (v >= 0) dst[0] = '+';
				dst += 1;
			}
			else
				sprintf(dst,FORMAT_DECIMAL4,b);	// max=4294967295
			dst+=WIDTH_DECIMAL4+1;
		}
	}
	break;
} // switch bytes

		break; // DECIMAL


		case DISPLAY_FLOAT:
switch (bytes)
{
	default:
	case 4:
	{
		float *ptr = (float *)data;
		for(i=0; i<count; i++)
		{
			double b = ptr[i];
			sprintf(dst,FORMAT_FLOAT4,b);
			dst+=WIDTH_FLOAT4+1;
			dst[-1] = ' ';
			dst[ 0] =  0;
		}
	}
	break;

	case 8:
	{
		double *ptr = (double *)data;
		for(i=0; i<count; i++)
		{
			double b = ptr[i];
			sprintf(dst,FORMAT_FLOAT8,b);
			dst+=WIDTH_FLOAT8+1;
			dst[-1] = ' ';
			dst[ 0] =  0;
		}
	}
	break;
}
		break; // FLOAT


	} // switch type

	return (dst);
}

//---------------------------------------------------------------------------
// HexWindow
//---------------------------------------------------------------------------

//also 10,18 or 10,20 FW_BOLD
//#define FONT_INFO 18,10,FW_SEMIBOLD,"Terminal"
#define FONT_INFO 18,9,FW_NORMAL,"Courier"

HexWindow::HexWindow (void)
{
	char_width = 1;
	char_height = 1;

	display_flags = DISPLAY_OFFSET | DISPLAY_ASCII;
	display_bytes = 1;
	display_type = DISPLAY_HEX;

	data_ptr = 0;
	data_size = 0;

	line_width = 0;
}

//---------------------------------------------------------------------------

void HexWindow::update_line_info (void)
// INPUTS
//		display_bytes
//		display_type
//		display_flags
//		data_size
//
// RETURNS
//		offset_chars
//		offset_format
//		unit_chars
//		units_per_line
//		bytes_per_line
//		line_width
{
// LINE = "OFFSET: UNIT UNIT ..ASCII..

	int base_width = 0;

// SET OFFSET INFO

	offset_chars = 0;
	offset_format = NULL;

	if (display_flags & DISPLAY_OFFSET)
	{
		if (data_size <= 0x100)
		{
			offset_chars = 2;
			offset_format = "%02X: ";
			base_width += 2;
		}
		else if (data_size <= 0x10000)
		{
			offset_chars = 4;
			offset_format = "%04X: ";
			base_width += 4;
		}
		else
		{
			offset_chars = 8;
			offset_format = "%08X: ";
			base_width += 8;
		}
		base_width += 2; // ": "
	}
	base_width *= char_width;

// SET UNIT INFO

	unit_chars = 1;									// "# " trailing space
	
	switch (display_type)
	{
		default:
		case DISPLAY_HEX:
			unit_chars += 2*display_bytes;			// "00 "
		break;

		case DISPLAY_DECIMAL:
			switch (display_bytes)
			{
				default:
				case 1: unit_chars += WIDTH_DECIMAL1; break;
				case 2: unit_chars += WIDTH_DECIMAL2; break;
				case 4: unit_chars += WIDTH_DECIMAL4; break;
			}
		break;

		case DISPLAY_FLOAT:
			switch (display_bytes)
			{
				default:
				case 4: unit_chars += WIDTH_FLOAT4; break;
				case 8: unit_chars += WIDTH_FLOAT8; break;
			}
		break;
	}

	if (display_flags & DISPLAY_SIGNED)				// "-00 "
		unit_chars += 1;

// GET BYTES PER LINE

	RECT r;
	GetClientRect(&r);

	int pixels_per_unit = unit_chars * char_width;

	if (display_flags & DISPLAY_ASCII)				// "-00 X"
		pixels_per_unit += char_width*display_bytes;

	units_per_line = (r.right-base_width) / pixels_per_unit;

	if (units_per_line < 1) // always display something
		units_per_line = 1;

	bytes_per_line = units_per_line * display_bytes;

	line_width = base_width + units_per_line*pixels_per_unit;

// FIX SCROLL BAR

	int num_lines = (data_size+bytes_per_line-1) / bytes_per_line;
	int page_size = (r.bottom - r.top) / char_height;

	scroll_bar.SetScrollSizes(num_lines,page_size);
}

/*
void HexWindow::fix_dimensions (int units, int lines)
// Note: make sure "update_line_info()" is current...
{
	RECT r;
	int w,h;
	CWnd *win = GetParent();
	win->GetWindowRect(&r);

	w = r.right-r.left;
	h = r.bottom-r.top;

	if (units != -1)
	{
		w = 0;
		if (offset_digits)
			w += offset_digits + 2;

		w += unit_chars * units;

		if (display_flags & DISPLAY_ASCII)
			w += 1*units;

		w *= char_width;

		#define VSLIDER_WIDTH 0x10	// optional
		w += VSLIDER_WIDTH;
	}

	if (lines != -1)
	{
		h = lines*char_height + 0x00;
	}

	CRect resize;
	resize.left = 0;
	resize.top = 0;
	resize.right = w+4;
	resize.bottom = h+4;
	win->CalcWindowRect(&resize,1);
	win->SetWindowPos(NULL,0,0,resize.Width(),resize.Height(),SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
//	GetClientRect(&r);

// WINDOW SIZE CHANGED

	update_scroll_bar();
}
*/

//---------------------------------------------------------------------------

void HexWindow::Init (HWND h)
{
	xWnd::Init(h);

	scroll_bar.init(h,SB_VERT);
	scroll_bar.SetScrollSizes(16,4,1);

	font.set_font(FONT_INFO);
	font.get_dimensions(&char_width,&char_height);

	update_line_info();
}

//---------------------------------------------------------------------------

void HexWindow::OnScroll (void)
{
	RECT r;
	GetClientRect(&r); 
	InvalidateRect(&r,0);
}

//---------------------------------------------------------------------------

int HexWindow::OnSize (int w, int h)
{
	update_line_info();

	return 1;
}

//---------------------------------------------------------------------------

void HexWindow::OnDraw (CDC *dc)
{
// GET CLIP REGION

	RECT full,clip;

	if (dc->GetClipBox(&clip) == NULLREGION)
	{
		return;
	}

	GetClientRect(&full);

// CLEAR SURFACE

	int rgb = GetSysColor(COLOR_WINDOW);
	dc->FillSolidRect(&clip,rgb);

	if (line_width == 0)
		return;

// DETERMINE DRAW AREA

	int line,offset,stop;

	int top = clip.top;
	int bot = clip.bottom + char_height-1; // round up

	line = top / char_height;

int pos = scroll_bar.GetScrollPos();

	offset = (line+pos) * bytes_per_line;
	int last_line = (bot / char_height) + pos;
	stop = last_line * bytes_per_line;

// DRAW LINES

	font.use(dc);
	dc->SetBkMode(TRANSPARENT);

	char msg[256];

	int x = 0;
	int y = full.top + line*char_height;

	while (offset < stop)
	{
		int i;
		char *m = msg;

		if (offset < data_size)
		{
		// PRINT OFFSET (optional)

			if (offset_format)
			{
				sprintf(m,offset_format,offset);
				m += strlen(m);
			}

		// PRINT DATA UNITS

			int b = units_per_line * display_bytes;
			int u = units_per_line;

			int less = (offset+b) - data_size;

			if (less > 0)
			{
				less = ((less+display_bytes-1)/display_bytes);
				u -= less;
			}
			else
			{
				less = 0;
			}

			m = print_numbers(m,
					data_ptr+offset,u,
					display_type,display_bytes,display_flags);

			if (less > 0)
			{
				less *= unit_chars;
				memset(m,' ',less);
				m += less;
			}

		// PRINT ASCII (optional)

			if (display_flags & DISPLAY_ASCII)
			{
				for(i=0; i<u*display_bytes; i++)
				{
					char ch;
					BYTE b = data_ptr[offset+i];
					if (b < 128)
						ch = ViewChar[b];
					else
						ch = '.';
					*m = ch;
					m+=1;
				}
			}
		}

	// TERMINATE STRING AND DISPLAY

		*m = NULL;

		int size = m - msg;
		dc->TextOut(x,y, msg, size);

	// ADVANCE TO NEXT LINE

		y += char_height;
		offset += bytes_per_line;

	} // while

// UPDATE SURFACE

	ValidateRect(&clip);
}

//---------------------------------------------------------------------------
// HexWindow POP-UP Menu Options
//---------------------------------------------------------------------------

void HexWindow::OnOptOffset (void) 
{
	display_flags ^= DISPLAY_OFFSET;
	redraw_window();
}

void HexWindow::OnUpdateOptOffset (CCmdUI* ui) 
{
	int flag = ((display_flags & DISPLAY_OFFSET) != 0);
	ui->SetCheck(flag);
}

void HexWindow::OnOptAscii() 
{
	display_flags ^= DISPLAY_ASCII;
	redraw_window();
}

void HexWindow::OnUpdateOptAscii(CCmdUI* ui) 
{
	int flag = ((display_flags & DISPLAY_ASCII) != 0);
	ui->SetCheck(flag);
}

//---------------------------------------------------------------------------

void HexWindow::OnOptByte() 
{
	display_bytes = 1;
	if (display_type == DISPLAY_FLOAT)
		display_type = DISPLAY_HEX;
    update_line_info();
	redraw_window();
}

void HexWindow::OnUpdateOptByte(CCmdUI* ui) 
{
	int flag = (display_bytes == 1) && (display_type != DISPLAY_FLOAT);
	ui->SetCheck(flag);
}

void HexWindow::OnOptShort() 
{
	display_bytes = 2;
	if (display_type == DISPLAY_FLOAT)
		display_type = DISPLAY_HEX;
    update_line_info();
	redraw_window();
}

void HexWindow::OnUpdateOptShort(CCmdUI* ui) 
{
	int flag = (display_bytes == 2) && (display_type != DISPLAY_FLOAT);
	ui->SetCheck(flag);
}

void HexWindow::OnOptLong() 
{
	display_bytes = 4;
    update_line_info();
	redraw_window();
}
void HexWindow::OnUpdateOptLong(CCmdUI* ui) 
{
	int flag = (display_bytes == 4);
	ui->SetCheck(flag);
}

void HexWindow::OnOptHuge() 
{
	display_bytes = 8;
    update_line_info();
	redraw_window();
}
void HexWindow::OnUpdateOptHuge(CCmdUI* ui) 
{
	int flag = (display_bytes == 8);
	ui->SetCheck(flag);
}

//---------------------------------------------------------------------------

void HexWindow::OnOptBinary (void) 
{
	display_type = DISPLAY_BINARY;
	redraw_window();
}

void HexWindow::OnUpdateOptBinary (CCmdUI* ui) 
{
	int flag = (display_type == DISPLAY_BINARY);
	ui->SetCheck(FALSE); //flag);
}

void HexWindow::OnOptHex (void) 
{
	display_type = DISPLAY_HEX;
	redraw_window();
}
void HexWindow::OnUpdateOptHex (CCmdUI* ui) 
{
	int flag = (display_type == DISPLAY_HEX);
	ui->SetCheck(flag);
}

void HexWindow::OnOptDecimal (void) 
{
	display_type = DISPLAY_DECIMAL;
	redraw_window();
}
void HexWindow::OnUpdateOptDecimal (CCmdUI* ui) 
{
	int flag = (display_type == DISPLAY_DECIMAL);
	ui->SetCheck(flag);
}

void HexWindow::OnOptFloat (void) 
{
	if (display_type != DISPLAY_FLOAT)
	{
		if (display_bytes < 4)
			display_bytes = 4;
		display_type = DISPLAY_FLOAT;
		redraw_window();
	}
}
void HexWindow::OnUpdateOptFloat (CCmdUI* ui) 
{
	int flag = (display_type == DISPLAY_FLOAT);
	ui->SetCheck(flag);
}

//---------------------------------------------------------------------------

void HexWindow::OnOptSigned (void) 
{
	display_flags ^= DISPLAY_SIGNED;
	redraw_window();
}

void HexWindow::OnUpdateOptSigned (CCmdUI* ui) 
{
	int flag = (display_flags & DISPLAY_SIGNED);
	ui->SetCheck(flag);
}

//---------------------------------------------------------------------------

typedef void ( HexWindow::*HEX_MSG1)(void);
typedef void ( HexWindow::*HEX_MSG2)(CCmdUI*);

void HexWindow::CmdUI (CCmdUI *cmd)
{
	#define xON_COMMAND(id,func) if(cmd->m_nID==id&&cmd->m_nIndexMax==0)func(); else
	#define xON_UPDATE_COMMAND_UI(id,func) if(cmd->m_nID==id&&cmd->m_nIndexMax!=0)func(cmd); else

	xON_COMMAND(ID_OPT_OFFSET, OnOptOffset)
	xON_UPDATE_COMMAND_UI(ID_OPT_OFFSET, OnUpdateOptOffset)
	xON_COMMAND(ID_OPT_ASCII, OnOptAscii)
	xON_UPDATE_COMMAND_UI(ID_OPT_ASCII, OnUpdateOptAscii)

	xON_COMMAND(ID_OPT_BYTE, OnOptByte)
	xON_UPDATE_COMMAND_UI(ID_OPT_BYTE, OnUpdateOptByte)
	xON_COMMAND(ID_OPT_SHORT, OnOptShort)
	xON_UPDATE_COMMAND_UI(ID_OPT_SHORT, OnUpdateOptShort)
	xON_COMMAND(ID_OPT_LONG, OnOptLong)
	xON_UPDATE_COMMAND_UI(ID_OPT_LONG, OnUpdateOptLong)
	xON_COMMAND(ID_OPT_HUGE, OnOptHuge)
	xON_UPDATE_COMMAND_UI(ID_OPT_HUGE, OnUpdateOptHuge)

	xON_COMMAND(ID_OPT_BINARY, OnOptBinary)
	xON_UPDATE_COMMAND_UI(ID_OPT_BINARY, OnUpdateOptBinary)
	xON_COMMAND(ID_OPT_HEX, OnOptHex)
	xON_UPDATE_COMMAND_UI(ID_OPT_HEX, OnUpdateOptHex)
	xON_COMMAND(ID_OPT_DECIMAL, OnOptDecimal)
	xON_UPDATE_COMMAND_UI(ID_OPT_DECIMAL, OnUpdateOptDecimal)
	xON_COMMAND(ID_OPT_FLOAT, OnOptFloat)
	xON_UPDATE_COMMAND_UI(ID_OPT_FLOAT, OnUpdateOptFloat)

	xON_COMMAND(ID_OPT_SIGNED, OnOptSigned)
	xON_UPDATE_COMMAND_UI(ID_OPT_SIGNED, OnUpdateOptSigned)
	;
}

//---------------------------------------------------------------------------
// HexViewer
//---------------------------------------------------------------------------

HexViewer::HexViewer (void)
{
	// NOTE: relies on "new" operator to clear instance memory.
	// Instances on the stack will not be initialized correctly!!!

					rect.top = 128;
	rect.left = 320;				rect.right = 640;
					rect.bottom = 320;
}

//---------------------------------------------------------------------------

HexViewer::~HexViewer (void)
{
	if (hMainWindow)
		DestroyWindow(hMainWindow);
	hMainWindow=0;
	if (doc)
		OnClose(doc);
}

//--------------------------------------------------------------------------

void * HexViewer::operator new (size_t size)
{
	return calloc(size, 1);
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::set_display_state (BOOL32 state)
{
 	bVisible = (state != 0);
	if (hMainWindow)
	{
		ShowWindow(hMainWindow, (bVisible) ? SW_SHOW:SW_HIDE);
		if (bVisible)
		{	
			if (hEdit)
				SetFocus(hEdit);
		}
		return GR_OK;
	}
	return GR_GENERIC;
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::get_display_state (BOOL32 *state)
{
	if (state)
	{
		*state = bVisible;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::get_class_name (C8 *name)
{
	if (name)
	{
		strcpy(name, ClassName);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::set_instance_name (const C8 *name)
{
	if (name)
	{
		SetWindowText(hMainWindow, name);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::get_instance_name (C8 *name)
{
	if (name)
	{
		GetWindowText(hMainWindow, name, 80);
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------

GENRESULT HexViewer::get_main_window (void **hwnd)
{
	if (hwnd)
	{
		*hwnd = hMainWindow;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT HexViewer::set_rect (const struct tagRECT *pRect)
{
	if (pRect)
	{
		rect = *pRect;
		if (hMainWindow)
			MoveWindow(hMainWindow, rect.left, rect.top, rect.right - rect.left, rect.bottom-rect.top, TRUE);
		else
			return GR_GENERIC;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT HexViewer::get_rect (struct tagRECT *pRect)
{
	if (pRect)
	{
		*pRect = rect;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}
//--------------------------------------------------------------------------
//
GENRESULT HexViewer::set_read_only (BOOL32 value)
{
	bReadOnly = (value != 0);
	SendMessage(hEdit, EM_SETREADONLY, bReadOnly, 0);
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT HexViewer::get_read_only (BOOL32 *value)
{
	if (value)
	{
		*value = bReadOnly;
		return GR_OK;
	}
	return GR_INVALID_PARMS;
}

//--------------------------------------------------------------------------
//
GENRESULT HexViewer::set_viewer_path (const char *path)
{
	return GR_OK;
}
//--------------------------------------------------------------------------
// Called when a document client has modified the document.
// '_doc' may be our document, or a child document. (sub document)
//  This viewer ignores the 'message' and 'parm' parameters, and always
//  updates all of its data.
//
GENRESULT HexViewer::OnUpdate (struct IDocument *_doc, const C8 *message, void *parm)
{
	if (doc != _doc)			// is this our document? (ignore updates from child docs)
		return GR_GENERIC;
/*
	DWORD dwRead;
	char *pData;

	dwRead = doc->GetFileSize();
	if ((pData = (char *) malloc(dwRead+1)) == 0)
		return GR_OUT_OF_MEMORY;

	pData[dwRead] = 0;		// set terminating character

	doc->SetFilePointer(0,0);
	if (doc->ReadFile(0, pData, dwRead, &dwRead, 0) == 0)
	{
		return GR_FILE_ERROR;
	}

	// do something with data

	set_display_value(pData);

	free(pData);
*/
	return GR_OK;
}
//--------------------------------------------------------------------------
//
GENRESULT HexViewer::OnClose (struct IDocument *document)
{
	if (document == doc && doc)	//is this the right instance?
	{
		COMPTR<IDAConnectionPoint> connection;

		if (doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
			return GR_GENERIC;

		if (hMainWindow)
			DestroyWindow(hMainWindow);
		hMainWindow = 0;

		connection->Unadvise(connHandle);
		doc = 0;
		BaseComponent()->Release();		// release the extra reference we added
	}

	return GR_OK;
}
//--------------------------------------------------------------------------
// This viewer's data is the display value. Write the data to the document
// and notify everyone of the modification.
//
BOOL32 HexViewer::WriteNewData (void)
{
	BOOL32 result = 0;
	DWORD dwWrite;

	if (doc)
	{
		doc->SetFilePointer(0,0);
		if (doc->WriteFile(0, szDisplayName, sizeof(szDisplayName), &dwWrite, 0))
			result = 1;
		doc->UpdateAllClients();
	}	

	return result;
}
//----------------------------------------------------------------------------
// Catch the ESC and RETURN button presses. Turn these into IDOK and IDCANCEL messages.
//
LONG CALLBACK HexViewer::EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	HexViewer *pViewer = (HexViewer *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);

	if (pViewer)
	switch (message)
	{
		case WM_CHAR:
			switch (LOWORD(wParam))
			{
			case 13:
				PostMessage(pViewer->hMainWindow, WM_COMMAND, IDOK, (LONG)hwnd);	// send message to parent
				return 0;
			case 27:
				PostMessage(pViewer->hMainWindow, WM_COMMAND, IDCANCEL, (LONG)hwnd);	// send message to parent
				return 0;
			}
			break;
	}

	return CallWindowProc((WNDPROC)pViewer->lpfnOldEditProcedure, hwnd, message, wParam, lParam);
}

//---------------------------------------------------------------------------

LRESULT HexViewer::MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	LRESULT result = 0;

	switch (message)
	{
	case WM_CREATE:
		hMainWindow = hwnd;
		result=1;
		break;	// end WM_CREATE case

	case WM_MOVE:
		GetWindowRect(hwnd, &rect);
		break;

	case WM_SIZE:
		{
			WORD wWidth, wHeight;

			wWidth = LOWORD(lParam);
			wHeight = HIWORD(lParam);

			result = window.OnSize(wWidth,wHeight);

			if (wParam == SIZE_MINIMIZED)
			{
				result = 0;
			}
		}
		break;

	case WM_INITMENUPOPUP:
	{
		HMENU h = (HMENU)LOWORD(wParam);

		int count = GetMenuItemCount(h);

		CCmdUI cmd;
		cmd.m_nIndexMax = count;
		cmd.m_pMenu = CMenu::FromHandle(h);

		for (int i=0; i<count; i++)
		{
			cmd.m_nIndex = i;
			cmd.m_nID = GetMenuItemID(h,i);
			window.CmdUI(&cmd);
		}
	}
	break;

	case WM_COMMAND:
		if (HIWORD(wParam) == 0) // MENU?
		{
			CCmdUI cmd;
			cmd.m_nID = LOWORD(wParam);
			cmd.m_nIndexMax = 0;
			window.CmdUI(&cmd);
		}
		else
		switch (LOWORD(wParam))
		{
			case ID_OPT_OFFSET:
			case ID_OPT_HEX:
				result = 0;
//				window.OnUpdateOptOffset();
			break;

			case CN_UPDATE_COMMAND_UI:
				if (HIWORD(lParam) == ID_OPT_OFFSET)
				{
	//				window.OnUpdateOptOffset();
				}
			break;

		case IDOK:
			if (bReadOnly==false && bWinDataChanged)
			{
				GetWindowText((HWND)lParam, szDisplayName, sizeof(szDisplayName)-1);
				bWinDataChanged=0;
				WriteNewData();
			}

			// fall through intentional

		case IDCANCEL:
			SetWindowText(hEdit, szDisplayName);
			SendMessage(hEdit, EM_SETSEL, 0, -1);
			bWinDataChanged=0;
			if (bAutoClose)
				PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case IDC_EDIT1:
			switch (HIWORD(wParam))
			{
			case EN_CHANGE:
				bWinDataChanged=1;
				break;
			case EN_SETFOCUS:
				if (bReadOnly)
					SendMessage(hEdit, EM_SETSEL, -1, -1);
				break;
			case EN_KILLFOCUS:
				PostMessage(hwnd, WM_COMMAND, IDOK, lParam);	// send message to parent
			}
			break;

		}
		break;

	case WM_CLOSE:
		set_display_state(0);
		result = 1;
		break;

	case WM_DESTROY:
		if (hMainWindow==hwnd)
		{
			hEdit = 0;
			bVisible=0;
			hMainWindow=0;
			bWinDataChanged=0;
		}
		break;

	case WM_PAINT:
		if (hMainWindow==hwnd)
		{
			window.OnPaint();
			result = 1;
		}
	break;

	case WM_ERASEBKGND:
		result = 0;
	break;

	case WM_RBUTTONDOWN:
		result = 0;
	break;
	case WM_RBUTTONUP:
		if (hwnd == hMainWindow)
		{
//			GetParentFrame()->ActivateFrame();

			CPoint point;
			RECT r;
			GetClientRect(hwnd,&r);
			point.x = r.left;
			point.y = r.top;
			ClientToScreen(hwnd,&point);
			point.x += LOWORD(lParam);  // horizontal position of cursor 
			point.y += HIWORD(lParam);  // vertical position of cursor 

			HMENU menu = LoadMenu(hInstance,MAKEINTRESOURCE(IDR_DATA_OPTIONS));

			if (menu)
			{
				HMENU pPopup = GetSubMenu(menu,0);
				ASSERT(pPopup != NULL);

				TrackPopupMenu(pPopup,
								TPM_LEFTALIGN | TPM_RIGHTBUTTON,
								point.x, point.y,
								0,
								hwnd,
								NULL);
			}
		}
	break;

	case WM_VSCROLL:
		if (hMainWindow==hwnd)
		{
			window.OnVScroll(wParam,lParam);
		}
	break;

	}

	return result;
}

//---------------------------------------------------------------------------

BOOL32 HexViewer::init (void)
{
	WNDCLASS wndclass;
	BOOL32 result=0;

	result = GetClassInfo(hInstance,ClassName,&wndclass);

	if (result == 0)
	{
		memset(&wndclass, 0, sizeof(wndclass));
		wndclass.style = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc = StaticWndProc;
		wndclass.hInstance = hInstance;
		wndclass.hCursor = LoadCursor(hInstance, MAKEINTRESOURCE(IDC_ARROW));
		wndclass.lpszClassName = ClassName;
		wndclass.hIcon = LoadIcon(hInstance,MAKEINTRESOURCE(IDR_HEXTYPE));

		result = RegisterClass(&wndclass);
	}

	if (result != 0)
	{
		hMainWindow = CreateWindow(ClassName, "No Name",
			WS_OVERLAPPEDWINDOW|WS_VSCROLL,
//			WS_CAPTION|WS_POPUP|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU|WS_VSCROLL, 
			rect.left, rect.top,
			rect.right - rect.left, rect.bottom - rect.top,
			hParentWindow,
			0,
			hInstance,
			this);

		if (hMainWindow)
		{
			U32 bytes = 0;
			int size = doc->GetFileSize();
			void *buffer = malloc(size);
			doc->SetFilePointer(0,0);
			if (doc->ReadFile(0, buffer, size, &bytes, 0) == 0)
			{
				bytes = 0;
			}
			window.set_data(buffer,bytes);

			window.Init(hMainWindow);
		}

		result=1;
	}

	return result;
}
//--------------------------------------------------------------------------
//
LRESULT CALLBACK HexViewer::StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	if (message == WM_CREATE)
		SetWindowLong(hwnd, GWL_USERDATA, 
			(long)((CREATESTRUCT *)lParam)->lpCreateParams);	// store "this" pointer in dialog data

	HexViewer *viewer = (HexViewer *) GetWindowLong(hwnd, GWL_USERDATA);

	if (viewer)
		viewer->MainWndProc(hwnd, message, wParam, lParam);

	return DefWindowProc(hwnd, message, wParam, lParam);
}
//---------------------------------------------------------------------------
//
GENRESULT HexViewer::init (VIEWDESC *lpDesc)
{
	GENRESULT result = GR_OK;
	COMPTR<IDAConnectionPoint> connection;

	if (strcmp(lpDesc->className, ClassName) != 0)
	{
		result = GR_INTERFACE_UNSUPPORTED;
		goto Done;
	}

	if (lpDesc->doc == 0)
	{
		result = GR_INVALID_PARMS;
		goto Done;
	}

	if (lpDesc->doc->QueryOutgoingInterface("IDocumentClient", connection) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	// NOTE: could also check the data to make sure it is of the right form.
	// This sample viewer does not care about the form of the data.

	doc = lpDesc->doc;
	BaseComponent()->AddRef();		// add reference so we live even if user releases us
									// we close only when document closes

	strcpy(szClassName, lpDesc->className);
/*
{
	static int test = 0;
	void *ptr;
	int size;
	if (test == 0)
	{
		ptr = "the rain in Spain falls mainly on the plain.\n";
		size = strlen((char *)ptr);
		test = 1;
	}
	else if (test == 1)
	{
		static float list1[] =
		{
			5.0F,
		   3.14F,
		123456.0F,
		1234.99F,
		629.1968F,
		111222333.444F,
		-1234.5678F,
		};
		ptr = list1;
		size = sizeof(list1);
		test = 2;
	}
	else
	{
		test = 0;
		static double list2[] =
		{
			5.0,
		   3.14,
		123456.0,
		1234.99,
		629.1968,
		111222333444.555,
		-1234.5678,
		1.0/3
		};
		ptr = list2;
		size = sizeof(list2);
		test = 0;
	}
	pNewInstance->set_data((char *)ptr,size);
}
*/

	if (init() == 0 ||
		connection->Advise(BaseComponent(), &connHandle) != GR_OK)
	{
		result = GR_GENERIC;
		goto Done;
	}

	OnUpdate(lpDesc->doc);

Done:
	return result;
}

//---------------------------------------------------------------------------
// Register Viewer
//---------------------------------------------------------------------------

// Called by the DLL startup code, registers the HexViewerFactory with the DACO manager.

void RegisterHexViewer (ICOManager * DACOM)
{
	IComponentFactory *sample;

	if ((sample = new DAComponentFactory<DAComponent<HexViewer>, VIEWDESC>("IViewer")) != 0)
	{
		DACOM->RegisterComponent(sample, InterfaceName);
		sample->Release();
	}
}

//---------------------------------------------------------------------------
// DllMain
//---------------------------------------------------------------------------

BOOL WINAPI DllMain (HINSTANCE hinstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			hInstance = hinstance;
//			HEAP = HEAP_Acquire();
//			SetDllHeapMsg();

			ICOManager * DACOM;

			if ((DACOM = DACOM_Acquire()) != 0)
			{
				RegisterHexViewer(DACOM);
			}
		}
	}

	return 1;
}

//---------------------------------------------------------------------------
