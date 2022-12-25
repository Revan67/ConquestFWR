// CntrItem.h : interface of the CUTFAppCntrItem class
//

#if !defined(AFX_CNTRITEM_H__4814D397_2960_11D3_9B98_0050049E94BC__INCLUDED_)
#define AFX_CNTRITEM_H__4814D397_2960_11D3_9B98_0050049E94BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class UTFDoc;
class UTFView;

class CUTFAppCntrItem : public COleClientItem
{
	DECLARE_SERIAL(CUTFAppCntrItem)

// Constructors
public:
	CUTFAppCntrItem(UTFDoc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	UTFDoc* GetDocument()
		{ return (UTFDoc*)COleClientItem::GetDocument(); }
	UTFView* GetActiveView()
		{ return (UTFView*)COleClientItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUTFAppCntrItem)
	public:
	virtual void OnChange(OLE_NOTIFICATION wNotification, DWORD dwParam);
	virtual void OnActivate();
	protected:
	virtual void OnGetItemPosition(CRect& rPosition);
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual BOOL OnChangeItemPosition(const CRect& rectPos);
	virtual BOOL CanActivate();
	//}}AFX_VIRTUAL

// Implementation
public:
	~CUTFAppCntrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void Serialize(CArchive& ar);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNTRITEM_H__4814D397_2960_11D3_9B98_0050049E94BC__INCLUDED_)
