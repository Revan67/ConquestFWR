// UIEditDoc.h : interface of the CUIEditDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIEDITDOC_H__9FFDA60A_4836_11D2_89DA_00400521015D__INCLUDED_)
#define AFX_UIEDITDOC_H__9FFDA60A_4836_11D2_89DA_00400521015D__INCLUDED_

#include "UIData.h"	// Added by ClassView
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Data structures for the edited data.
#include "UIEditTypes.h"

// The document being edited.
class CUIEditDoc : public CDocument
{
protected: // create from serialization only
	CUIEditDoc();
	DECLARE_DYNCREATE(CUIEditDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUIEditDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	UIData     m_Data;
	UIHandle   m_CurrentArt;
	UIHandle   m_CurrentRect;
	UIEditMode m_EditMode;

	virtual ~CUIEditDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	SIZE getSize() { return size; }

protected:
	SIZE size;  // the size of the user interface canvas.

// Generated message map functions
protected:
	//{{AFX_MSG(CUIEditDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UIEDITDOC_H__9FFDA60A_4836_11D2_89DA_00400521015D__INCLUDED_)
