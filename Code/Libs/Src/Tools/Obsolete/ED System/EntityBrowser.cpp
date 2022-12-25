// EntityBrowser.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "EntityBrowser.h"

#include "MotionStateAccessor.h"
#include "DeformableSceneEntity.h"
#include "Char.h"

// DA Library includes
#include <engine.h>
#include <tsmartpointer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntityBrowser dialog


CEntityBrowser::CEntityBrowser(View *view, CWnd* pParent /*=NULL*/)
	: CDialog(CEntityBrowser::IDD, pParent), m_target(NULL), m_generations(0)
{
	m_view = view;
	m_rootItem = NULL;
	m_endItem = NULL;

	//{{AFX_DATA_INIT(CEntityBrowser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CEntityBrowser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEntityBrowser)
	DDX_Control(pDX, IDC_IK_TARGET_TREE, m_targetTree);
	DDX_Control(pDX, IDC_IK_TARGET, m_targetList);
	DDX_Control(pDX, IDC_BROWSE_TREE, m_tree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEntityBrowser, CDialog)
	//{{AFX_MSG_MAP(CEntityBrowser)
	ON_NOTIFY(NM_RCLICK, IDC_BROWSE_TREE, OnRclickBrowseTree)
	ON_WM_DESTROY()
	ON_COMMAND(ID_IK_SET_EFFECTOR, OnIkSetEffector)
	ON_COMMAND(ID_IK_SET_ROOT, OnIkSetRoot)
	ON_CBN_SELCHANGE(IDC_IK_TARGET, OnSelchangeIkTarget)
	ON_NOTIFY(TVN_SELCHANGING, IDC_BROWSE_TREE, OnSelchangingBrowseTree)
	ON_NOTIFY(NM_SETFOCUS, IDC_BROWSE_TREE, OnSetfocusBrowseTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntityBrowser message handlers

BOOL CEntityBrowser::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	// Load the menu.
	m_menu.LoadMenu (IDR_INSTANCETREE_MENU);

	// Only do this stuff if we have a valid view.
	if (m_view)
	{
		COMPTR<IEngine> ENG;
		COMPTR<IModel>  MOD;

		// Get an IModel pointer.
		ENG = CharGetEngine();
		if (!ENG)
		{
			return TRUE;
		}

		if (ENG->QueryInterface (IID_IModel, MOD) != GR_OK)
		{
			return TRUE;
		}

		// Bind the tree control to the selected entity, if any.
		ROS::DeformableSceneEntity*	defSceneEntity;
		defSceneEntity = dynamic_cast<ROS::DeformableSceneEntity *>(m_view->GetSelectedSceneEntity());
		if(defSceneEntity)
		{
			std::auto_ptr<ROS::MotionStateAccessor> accessor = defSceneEntity->GetMotionStateAccessor();

			INSTANCE_INDEX idx = (INSTANCE_INDEX) accessor->GetRootEngineIndex();
			m_tree.bind_to_instance (MOD, idx);
		}

		// Put the name of the selected entity into the text field.
		SetDlgItemText (IDC_IK_SOURCE_NAME, defSceneEntity->GetConstSceneEntityStateAccessor()->GetName().c_str());

		// Add all but the selected item to the combo box, storing the entity pointer as the data
		// item for each entry.
		ROS::SceneEntityCollection	sceneEntityColl;

		m_view->GetSceneEntities(sceneEntityColl);

		ROS::SceneEntityCollection::const_iterator   begin = sceneEntityColl.begin();
		const ROS::SceneEntityCollection::const_iterator kEnd = sceneEntityColl.end();

		int count = 0;
		while(begin != kEnd)
		{
			if (*begin != defSceneEntity)
			{
				ROS::AStaticSceneEntity*	entity = dynamic_cast<ROS::AStaticSceneEntity*>(*begin);

				if(entity)
				{
					int	itemIdx = m_targetList.AddString((*begin)->GetConstSceneEntityStateAccessor()->GetName().c_str());
					if(itemIdx != LB_ERR && itemIdx != LB_ERRSPACE)
					{
						m_targetList.SetItemDataPtr(itemIdx, entity);
						++count;
					}
					else
					{
						return TRUE;
					}
				}
			}
			++begin;
		}

		if (count > 0)
		{
			// Select the first item
			select_indexed_target (0);
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEntityBrowser::select_indexed_target (int idx)
{
	// Attempt to select the indexed target entity.
	if (m_targetList.SetCurSel (idx) != CB_ERR)
	{
		ROS::ASceneEntity *target = (ROS::ASceneEntity *) m_targetList.GetItemDataPtr(idx);
		if (target)
		{
			m_targetTree.bind_to_entity (*target);
		}
	}
}


void CEntityBrowser::OnSelchangeIkTarget() 
{
	// TODO: Add your control notification handler code here
	int idx = m_targetList.GetCurSel ();
	ROS::ASceneEntity *target = (ROS::ASceneEntity *) m_targetList.GetItemDataPtr(idx);
	if (target)
	{
		m_targetTree.bind_to_entity (*target);
	}
}

void CEntityBrowser::OnRclickBrowseTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	// Pop up a context menu here that will ask if you want to make the current selection the
	// root or the end effector.
	const int IK_SOURCE_CONTEXT = 0;
	CMenu *popupMenu = m_menu.GetSubMenu (IK_SOURCE_CONTEXT);

	if(popupMenu)
	{
		POINT pos;
		GetCursorPos (&pos);
		
		// Select the right clicked item before proceeding.
		{
			POINT clientPos = pos;
			UINT flags;
			m_tree.ScreenToClient (&clientPos);
			HTREEITEM hClicked = m_tree.HitTest (clientPos, &flags);
			if (hClicked)
			{
				m_tree.SelectItem (hClicked);
			}
		}

		// Display the menu
		popupMenu->TrackPopupMenu(TPM_LEFTALIGN, pos.x, pos.y, this, NULL);
	}
	
	*pResult = 0;
}

void CEntityBrowser::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	// Destroy the menu
	m_menu.DestroyMenu();
}

void CEntityBrowser::OnIkSetEffector() 
{
	// TODO: Add your command handler code here
	// Set the end item and update the states.
	m_endItem = m_tree.GetSelectedItem();
	update_source_tree (KEEP_END);
}

void CEntityBrowser::OnIkSetRoot() 
{
	// TODO: Add your command handler code here
	// Set the root item and update the states.	
	m_rootItem = m_tree.GetSelectedItem();
	update_source_tree (KEEP_ROOT);
}

void CEntityBrowser::update_source_tree (UPDATE_HINT hint)
{
	// Clear the state values for all of the items in the tree.

	m_tree.clear_tree_ik_state (m_tree.GetRootItem());

	// Starting at the root, try to find the end among the descendants of the
	// root. If the root end is not found, keep the hinted value and reset the other.

	if (m_rootItem == NULL)
	{
		// Invalid chain. Set the end state properly
		if (hint == KEEP_ROOT)
		{
			m_endItem = NULL;
		}
		else
		{
			m_tree.set_item_ik_state (m_endItem, CDAInstanceTree::IK_END);
		}
		return;
	}

	if (m_endItem == NULL)
	{
		// Invalid chain. Set the root state properly
		if (hint == KEEP_END)
		{
			m_rootItem = NULL;
		}
		else
		{
			m_tree.set_item_ik_state (m_rootItem, CDAInstanceTree::IK_ROOT);
		}
		return;
	}


	// The chain is not patently invalid, so validate it.

	m_tree.set_item_ik_state (m_endItem, CDAInstanceTree::IK_END);

	HTREEITEM hItem = m_tree.GetParentItem (m_endItem);

	while (hItem)
	{
		if (hItem == m_rootItem)
		{
			// The chain is good. Mark this as the end and return.
			m_tree.set_item_ik_state (hItem, CDAInstanceTree::IK_ROOT);
			return;
		}
		else
		{
			// Mark this item as part of the chain.
			m_tree.set_item_ik_state (hItem, CDAInstanceTree::IK_MIDDLE);
		}

		hItem = m_tree.GetParentItem (hItem);
	}

	// The chain is invalid, so clear the any stuff added above and keep the hinted
	// value.

	m_tree.clear_tree_ik_state (m_tree.GetRootItem());
	if (hint == KEEP_ROOT)
	{
		m_tree.set_item_ik_state (m_rootItem, CDAInstanceTree::IK_ROOT);
		m_endItem = NULL;
	}
	else if (hint == KEEP_END)
	{
		m_tree.set_item_ik_state (m_endItem, CDAInstanceTree::IK_END);
		m_rootItem = NULL;
	}
}

const char* CEntityBrowser::GetEndEffectorName() const
{
	return m_end_name;
}

unsigned int CEntityBrowser::GetRootEffectorGenerations() const
{
	return m_generations;
}

ROS::AStaticSceneEntity* CEntityBrowser::GetTargetEntity() const
{
	return m_target;
}

void CEntityBrowser::OnOK() 
{
	m_generations = 0;

	if(m_endItem)
	{
		m_end_name = m_tree.GetItemText(m_endItem);

		if(m_rootItem)
		{
			while(m_endItem != m_rootItem)
			{
				++m_generations;

				m_endItem = m_tree.GetParentItem(m_endItem);
			}
		}
	}

	m_target = NULL;

	const int idx = m_targetList.GetCurSel();

	if(idx != LB_ERR)
	{
		m_target = reinterpret_cast<ROS::AStaticSceneEntity*>(m_targetList.GetItemDataPtr(idx));
	}

	CDialog::OnOK();
}

void CEntityBrowser::OnCancel() 
{
	m_generations = 0;
	m_target = NULL;
	
	CDialog::OnCancel();
}

void CEntityBrowser::OnSelchangingBrowseTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CEntityBrowser::OnSetfocusBrowseTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}
