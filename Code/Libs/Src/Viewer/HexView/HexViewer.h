//---------------------------------------------------------------------------
//
// HexViewer.H
//
//
//---------------------------------------------------------------------------

#ifndef HEXVIEWER_H
#define HEXVIEWER_H

//---------------------------------------------------------------------------

#ifndef VIEWER_H
#include "Viewer.h"
#endif

#ifndef IDOCCLIENT_H
#include "IDocClient.h"
#endif

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef TDISPATCH_H
#include "TDispatch.h"
#endif

#include "xWnd.h"

#include "filesys.h"

//--------------------------------------------------------------------------//
// xWnd
//--------------------------------------------------------------------------//

struct HexWindow : xWnd
{
	xFont font;
	xScrollBar scroll_bar;

	char *data_ptr;
	int data_size;

	int display_flags;		// DisplayFlags bit field
	int display_bytes;		// bytes per display unit (1=byte,2=short,4=long)
	int display_type;		// display unit type (Ex=DISPLAY_HEX)

	char *offset_format;	// sprintf format string

	int offset_chars;		// characters for offset number
							// ex. "0000: 41 X" = 4 digits

	int unit_chars;			// characters per number
							// ex. "0000: +41 +42 XX" = 5 chars

	int bytes_per_line;		// number of bytes on display line
							// ex. "00: 1234 5678 ...." = 4 bytes

	int units_per_line;		// how many numbers on line
							// ex. "00: 01 02 03 XXX" = 3 units

	int line_width;			// pixels for bytes_per_line

	int char_width;			// average font-character size
	int char_height;
	int view_lines;			// how many lines of data displayed

// Methods

	HexWindow (void);

	void update_line_info (void);

	// testing
	void set_data (const void *ptr, int size)
	{
		data_ptr = (char *)malloc(size);
		if (data_ptr)
		{
			memcpy(data_ptr,ptr,size);
			data_size = size;
		}
		update_line_info();
	}

	~HexWindow (void)
	{
		::free(data_ptr);
		data_ptr = 0;
	}

	void redraw_window (void)
	{
		update_line_info();
		InvalidateRect(0);
	}

	afx_msg void OnOptOffset();
	afx_msg void OnUpdateOptOffset(CCmdUI* pCmdUI);
	afx_msg void OnOptAscii();
	afx_msg void OnUpdateOptAscii(CCmdUI* pCmdUI);
	afx_msg void OnOptByte();
	afx_msg void OnUpdateOptByte(CCmdUI* pCmdUI);
	afx_msg void OnOptShort();
	afx_msg void OnUpdateOptShort(CCmdUI* pCmdUI);
	afx_msg void OnOptLong();
	afx_msg void OnUpdateOptLong(CCmdUI* pCmdUI);
	afx_msg void OnOptHuge();
	afx_msg void OnUpdateOptHuge(CCmdUI* pCmdUI);
	afx_msg void OnOptBinary();
	afx_msg void OnUpdateOptBinary(CCmdUI* pCmdUI);
	afx_msg void OnOptHex();
	afx_msg void OnUpdateOptHex(CCmdUI* pCmdUI);
	afx_msg void OnOptDecimal();
	afx_msg void OnUpdateOptDecimal(CCmdUI* pCmdUI);
	afx_msg void OnOptFloat();
	afx_msg void OnUpdateOptFloat(CCmdUI* pCmdUI);
	afx_msg void OnOptSigned();
	afx_msg void OnUpdateOptSigned(CCmdUI* pCmdUI);

// xWindow

	virtual void Init (HWND h);

	virtual int OnSize (int w, int h);

	virtual void OnDraw (CDC *dc);
	virtual void OnScroll (void);

	virtual void CmdUI (CCmdUI *cmd);

	void OnVScroll (UINT wParam, LONG lParam)
	{
		scroll_bar.OnVScroll(wParam,lParam);
		OnScroll();
	}
};

//--------------------------------------------------------------------------//
// HexViewer
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE HexViewer : public Dispatch<HexViewer, IViewer>, IDocumentClient
{
public:

	//
	// create a table of interfaces that are supported. 
	// QueryInterface() uses this table.
	//

	BEGIN_DACOM_MAP_INBOUND(HexViewer)
	DACOM_INTERFACE_ENTRY(IViewer)
	DACOM_INTERFACE_ENTRY(IDADispatch)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY2(IID_IViewer,IViewer)
	DACOM_INTERFACE_ENTRY2(IID_IDADispatch,IDADispatch)
	DACOM_INTERFACE_ENTRY2(IID_IDocumentClient,IDocumentClient)
	END_DACOM_MAP()

	BEGIN_DACOM_DISPATCH_MAP(HexViewer)
	DACOM_DISPATCH_METHOD(set_display_state,  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_display_state,  DAVT_BOOL32|DAVT_BYREF)
	DACOM_DISPATCH_METHOD(get_class_name,     DAVT_STRING)
	DACOM_DISPATCH_METHOD(set_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_instance_name,  DAVT_STRING)
	DACOM_DISPATCH_METHOD(get_main_window,	  DAVT_PVOID|DAVT_BYREF)
	DACOM_DISPATCH_METHOD(set_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(get_rect,			  DAVT_PRECT)
	DACOM_DISPATCH_METHOD(set_read_only,	  DAVT_BOOL32)
	DACOM_DISPATCH_METHOD(get_read_only,	  DAVT_PBOOL32)
	END_DACOM_DISPATCH_MAP()

	char szDisplayName[128];
	char szClassName[32];

	HexWindow window;

	void set_data (char *ptr, int size)
	{
		window.set_data(ptr,size);
	}

	HWND hParentWindow;
	HWND hMainWindow;
	RECT rect;

	BOOL32 bVisible, bReadOnly, bAutoClose, bWinDataChanged;
	HWND hEdit;
	WNDPROC lpfnOldEditProcedure;

	struct IDocument *doc;
	U32 connHandle;

	//----------------------------------------------------------
	//----------------------------------------------------------

	HexViewer (void);

	void * operator new (size_t size);		// calls calloc()

	~HexViewer (void);

	GENRESULT init (VIEWDESC * info);

	/* IViewer members */
	
	DEFMETHOD(set_display_state) (BOOL32 state);

	DEFMETHOD(get_display_state) (BOOL32 *state);

	DEFMETHOD(get_class_name) (C8 *name);

	DEFMETHOD(set_instance_name) (const C8 *name);

	DEFMETHOD(get_instance_name) (C8 *name);

	DEFMETHOD(get_main_window) (void **hwnd);

	DEFMETHOD(set_rect) (const struct tagRECT *rect);

	DEFMETHOD(get_rect) (struct tagRECT *rect);

	DEFMETHOD(set_read_only) (BOOL32 value);

	DEFMETHOD(get_read_only) (BOOL32 *value);

	DEFMETHOD(set_viewer_path) (const char *path);

	/* IDocumentClient members */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message=0, void *parm=0);

	DEFMETHOD(OnClose) (struct IDocument *doc);

	/* HexViewer members */

	BOOL32 init (void);

	BOOL32 WriteNewData (void);

	LRESULT MainWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);
   
	static LRESULT CALLBACK StaticWndProc (HWND hwnd, UINT message, UINT wParam, LONG lParam);

	static LONG CALLBACK EditControlProcedure(HWND hwnd, UINT message, UINT wParam, LONG lParam);

	IDAComponent * BaseComponent (void)
	{
//		return (IDAComponent *) (daoffsetofclass(IDADispatch, HexViewer) + ((U32) this));
		return (IDAComponent *) (daoffsetofclass(IViewer, HexViewer) + ((U32) this));
	}

};


//--------------------------------------------------------------------------//
//----------------------------End HexViewer.h--------------------------------//
//--------------------------------------------------------------------------//

#endif