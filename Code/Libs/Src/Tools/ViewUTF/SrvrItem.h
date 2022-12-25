// SrvrItem.h : interface of the CUTFAppSrvrItem class
//

#if !defined(AFX_SRVRITEM_H__4814D393_2960_11D3_9B98_0050049E94BC__INCLUDED_)
#define AFX_SRVRITEM_H__4814D393_2960_11D3_9B98_0050049E94BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUTFAppSrvrItem : public COleServerItem
{
	DECLARE_DYNAMIC(CUTFAppSrvrItem)

// Constructors
public:
	CUTFAppSrvrItem(UTFDoc* pContainerDoc);

// Attributes
	UTFDoc* GetDocument() const
		{ return (UTFDoc*)COleServerItem::GetDocument(); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUTFAppSrvrItem)
	public:
	virtual BOOL OnDraw(CDC* pDC, CSize& rSize);
	virtual BOOL OnGetExtent(DVASPECT dwDrawAspect, CSize& rSize);
	//}}AFX_VIRTUAL

// Implementation
public:
	~CUTFAppSrvrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRVRITEM_H__4814D393_2960_11D3_9B98_0050049E94BC__INCLUDED_)
