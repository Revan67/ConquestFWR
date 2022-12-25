// HexView.h : header file
//

#ifndef HEXVIEW_H
#define HEXVIEW_H

#define HEX_DOC UTFDoc
class HEX_DOC;

#include "chunk.h"

/////////////////////////////////////////////////////////////////////////////
// HexView view

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

class HexView : public CScrollView
{
protected:
	HexView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(HexView)

// Attributes
protected:

    int modified;

    Chunk *original;
    Chunk data;
	long file_offset;		// data's file position

	int display_flags;		// DisplayFlags bit field
	int display_bytes;		// bytes per display unit (1=byte,2=short,4=long)
	int display_type;		// display unit type (Ex=DISPLAY_HEX)

	char *offset_format;	// sprintf format string

	int offset_digits;		// characters for offset number
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

// Operations
protected:

	void fix_dimensions (int units=16, int lines=-1);

	void update_line_info (void);

	void update_scroll_bar (void);

public:

    int is_viewing (void *chunk)
    {
        return original == chunk;
    }

	int is_active (void)
	{
		return (display_flags & DISPLAY_ACTIVE);
	}

	void activate (void)
	{
		display_flags |= DISPLAY_ACTIVE;
	}

	void deactivate (void)
	{
		display_flags &= ~DISPLAY_ACTIVE;
	}

	void redraw_window (void)
	{
		Invalidate(TRUE);	// invalidate entire client area
		UpdateWindow();		// repaint all invalidated areas
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(HexView)
	public:
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnInitialUpdate();     // first time after construct
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~HexView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	inline HEX_DOC *GetDocument (void)
	{
		return (HEX_DOC *)m_pDocument;
	}

	// Generated message map functions
	//{{AFX_MSG(HexView)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
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
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

#endif // HEXVIEW_H
