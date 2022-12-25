// PropEditDoc.h : interface of the CPropEditDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROPEDITDOC_H__F08D530C_4127_11D3_85B6_0000F4A24553__INCLUDED_)
#define AFX_PROPEDITDOC_H__F08D530C_4127_11D3_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "property.h"

class CPropEditDoc : public CDocument
{
protected: // create from serialization only
	CPropEditDoc();
	DECLARE_DYNCREATE(CPropEditDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropEditDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void DeleteContents();
	//}}AFX_VIRTUAL

// Implementation
public:
	CList<Property, Property &> props;

	void add_property (Property &p);
	void del_property (int index);
	Property *get_property (int index);

	virtual ~CPropEditDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPropEditDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPEDITDOC_H__F08D530C_4127_11D3_85B6_0000F4A24553__INCLUDED_)
