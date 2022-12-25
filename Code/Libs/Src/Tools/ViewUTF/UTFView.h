// UTFView.h : interface of the UTFView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTFVIEW_H__4814D390_2960_11D3_9B98_0050049E94BC__INCLUDED_)
#define AFX_UTFVIEW_H__4814D390_2960_11D3_9B98_0050049E94BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxcview.h>
#include <afxole.h>
typedef unsigned char byte;
#include "filesys.h"

class CUTFAppCntrItem;

class UTFView : public CTreeView
{
protected: // create from serialization only
	UTFView();
	DECLARE_DYNCREATE(UTFView)

// Attributes
public:
	UTFDoc* GetDocument();

	BOOL		dragging;
	HTREEITEM	drag_item;
	HTREEITEM	drop_item;

	COleDropTarget	drop_target;	//for drag & drop

	CImageList	my_image_list;
	CImageList *image_list;

    FILETIME build_time;

	// m_pSelection holds the selection to the current CUTFAppCntrItem.
	// For many applications, such a member variable isn't adequate to
	//  represent a selection, such as a multiple selection or a selection
	//  of objects that are not CUTFAppCntrItem objects.  This selection
	//  mechanism is provided just to help you get started.

	// TODO: replace this selection mechanism with one appropriate to your app.
	CUTFAppCntrItem* m_pSelection;

// Operations
public:
	CPoint m_dragPoint;         // current position
	CSize m_dragSize;         // size of dragged object
	CSize m_dragOffset;         // offset of focus rect
	DROPEFFECT m_prevDropEffect;   
	static CLIPFORMAT m_cfObjectDescriptor;
	BOOL m_bDragDataAcceptable;

	BOOL GetObjectInfo(COleDataObject* pDataObject,
				CSize* pSize, CSize* pOffset);


    CString make_name (Chunk *chunk);

    Chunk *get_chunk (HTREEITEM s);

    int get_line (CPoint &point);

    void set_status (const char *msg);
	void update_status (void);

    HTREEITEM find_item (DWORD data, HTREEITEM i=0);

	void expand_all (HTREEITEM i=NULL, UINT xcode=TVE_EXPAND);

	HTREEITEM add_item (Chunk *chunk, HTREEITEM parent=TVI_ROOT, HTREEITEM after=TVI_LAST);

    void build (Chunk *list, HTREEITEM parent=0);

    void refresh (void);

	BOOL copy_item (HTREEITEM drag_item, HTREEITEM drop_item);

	Chunk	*InsertDroppedSys(IFileSystem	*sys,
							char *fn,
							IFileSystem *dst,
							Chunk	*parent);

	Chunk	*InsertDropped(char *fn,
							IFileSystem *dst,
							Chunk	*parent);


	void insert_chunks (HTREEITEM parent, Chunk *chunk);

	Chunk *insert_chunk (HTREEITEM parent, const char *name, int size, const void *ptr=0);

	bool cut (HTREEITEM src=0);
	bool copy (HTREEITEM src=0);
	bool paste (HTREEITEM target=0);

	void cast_item (HTREEITEM s=0);

    void view_item (HTREEITEM s=0, const char *type_cast=0);
	int view (IDocument *doc, char *class_name="HexViewer");

    void toggle_item (void);
    void delete_item (void);
	void insert_item (void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UTFView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual BOOL OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnDragLeave();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual BOOL IsSelected(const CObject* pDocItem) const;// Container support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~UTFView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(UTFView)
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSelectChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRefresh();
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnInsertObject();
	afx_msg void OnCancelEditCntr();
	afx_msg void OnCancelEditSrvr();
	afx_msg void OnToolsExportRaw();
	afx_msg void OnToolsExportTga();
	afx_msg void OnToolsImportRaw();
	afx_msg void OnToolsImportTga();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in UTFView.cpp
inline UTFDoc* UTFView::GetDocument()
   { return (UTFDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UTFVIEW_H__4814D390_2960_11D3_9B98_0050049E94BC__INCLUDED_)
