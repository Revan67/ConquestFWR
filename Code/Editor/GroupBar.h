#ifndef GROUP_BAR_H
#define GROUP_BAR_H

#include "Sidebar.h"
#include "EventSys.h"
#include "TComponent.h"
#include "treelist\MltiTree.h"

struct IObject;
struct IObjectFamily;

class CGroupBar : public CSidebar
{
public:
	virtual ~CGroupBar();
	virtual int DoPaint( CPaintDC& );
	virtual bool Update();

	struct GroupInfo
	{
		CString object;
		CString family;
	};

private:
	//CMultiTree m_treeView;
	CTreeCtrl  m_treeView;
	CImageList m_imageList;

protected:

	void startNewGroup();
	void showGroupProps(CString&,CPoint&);
	void showObjectProps(CString&,CPoint&);

	bool updateNewGroups(HTREEITEM);
	bool updateOldGroups(HTREEITEM);
	bool deleteData(HTREEITEM);

	// Generated message map functions
protected:
	//{{AFX_MSG(CGroupBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};

#endif  // GROUP_BAR_H
