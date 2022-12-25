#if !defined(AFX_ENTITYTREE_H__CDC49464_F3DD_11D2_85B6_0000F4A24553__INCLUDED_)
#define AFX_ENTITYTREE_H__CDC49464_F3DD_11D2_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EntityTree.h : header file
//

#include "ASceneEntity.h"
#include <model.h>

/////////////////////////////////////////////////////////////////////////////
// CEntityTree window

class CEntityTree : public CTreeCtrl
{
// Construction
public:
	CEntityTree();

// Attributes
public:
	enum PartType {
		kNoType,
		kHardpoint,
		kSubObject,
	};

	struct Part
	{
		PartType type;
		CString  name;
		union
		{
			INSTANCE_INDEX subIndex;     // sub object engine index
			int            hardIndex;    // hardpoint index
		};

		Part () {}
		Part (PartType _type, const char *_name)
		{
			type = _type;
			name = _name;
			hardIndex = 0;
		}
	};

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityTree)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEntityTree();

	void bind_to_entity (ROS::ASceneEntity &entity);
	void unbind ();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEntityTree)
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

public:
	// Variable members
	ROS::ASceneEntity *m_entity;
	Part selectedPart;

private:
	HTREEITEM add_hardpoints (HTREEITEM hParent);
	HTREEITEM add_parts (HTREEITEM hParent);
	HTREEITEM add_tree (IModel *model, INSTANCE_INDEX root, HTREEITEM hParent);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTITYTREE_H__CDC49464_F3DD_11D2_85B6_0000F4A24553__INCLUDED_)
