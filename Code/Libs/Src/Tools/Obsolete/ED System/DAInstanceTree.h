#if !defined(AFX_DAINSTANCETREE_H__BFB2D344_F31B_11D2_85B6_0000F4A24553__INCLUDED_)
#define AFX_DAINSTANCETREE_H__BFB2D344_F31B_11D2_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DAInstanceTree.h : header file
//

// DA Library include
#include <model.h>

/////////////////////////////////////////////////////////////////////////////
// CDAInstanceTree window

class CDAInstanceTree : public CTreeCtrl
{
// Construction
public:
	CDAInstanceTree();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDAInstanceTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDAInstanceTree();

	void bind_to_instance (IModel *model, INSTANCE_INDEX inst);

	enum IK_CHAIN_STATE
	{
		IK_NONE = 0,
		IK_ROOT = 1,
		IK_MIDDLE = 2,
		IK_END = 3
	};

	void clear_tree_ik_state (HTREEITEM hRoot);
	void set_item_ik_state (HTREEITEM hItem, IK_CHAIN_STATE state);
	IK_CHAIN_STATE get_item_ik_state (HTREEITEM hItem);

	// Generated message map functions
protected:
	//{{AFX_MSG(CDAInstanceTree)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

public:
	// Variable members
	INSTANCE_INDEX  instance;
	CImageList *    m_srcImageList;

private:
	bool image_list_loaded;

	HTREEITEM add_tree (IModel *model, INSTANCE_INDEX root, HTREEITEM hParent=TVI_ROOT);
	void unbind ();  // clear the instance index and clean out the control
	void load_image_list();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DAINSTANCETREE_H__BFB2D344_F31B_11D2_85B6_0000F4A24553__INCLUDED_)
