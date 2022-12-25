#if !defined(AFX_ENTITYBROWSER_H__BFB2D34A_F31B_11D2_85B6_0000F4A24553__INCLUDED_)
#define AFX_ENTITYBROWSER_H__BFB2D34A_F31B_11D2_85B6_0000F4A24553__INCLUDED_

#include "DAInstanceTree.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EntityBrowser.h : header file
//

#include "DAInstanceTree.h"
#include "EntityTree.h"
#include <model.h>
#include "view.h"

namespace ROS
{
class AStaticSceneEntity;
}
/////////////////////////////////////////////////////////////////////////////
// CEntityBrowser dialog

class CEntityBrowser : public CDialog
{
// Construction
public:
	CEntityBrowser(View *view, CWnd* pParent = NULL);   // standard constructor

	const char* GetEndEffectorName() const;
	unsigned int GetRootEffectorGenerations() const;
	ROS::AStaticSceneEntity* GetTargetEntity() const;

// Dialog Data
	//{{AFX_DATA(CEntityBrowser)
	enum { IDD = IDD_ENTITY_BROWSER };
	CEntityTree	m_targetTree;
	CComboBox	m_targetList;
	CDAInstanceTree	m_tree;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityBrowser)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	View *                    m_view;  // used to access the list of scene entities and the selected entity
	CMenu                     m_menu;  // the collection of menus for this browser.

	HTREEITEM                 m_rootItem;
	HTREEITEM                 m_endItem;

	CString                   m_end_name;
	ROS::AStaticSceneEntity * m_target;
	unsigned int              m_generations;

	void select_indexed_target (int idx);

	enum UPDATE_HINT
	{
		KEEP_ROOT,
		KEEP_END
	};

	void update_source_tree (UPDATE_HINT hint);

	// Generated message map functions
	//{{AFX_MSG(CEntityBrowser)
	virtual BOOL OnInitDialog();
	afx_msg void OnRclickBrowseTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnIkSetEffector();
	afx_msg void OnIkSetRoot();
	afx_msg void OnSelchangeIkTarget();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnSelchangingBrowseTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocusBrowseTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTITYBROWSER_H__BFB2D34A_F31B_11D2_85B6_0000F4A24553__INCLUDED_)
