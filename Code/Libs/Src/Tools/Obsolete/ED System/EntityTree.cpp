// EntityTree.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "EntityTree.h"

#include "char.h"
#include "MotionStateAccessor.h"
#include "CompoundSceneEntity.h"
#include <assert.h>

// DA Library includes
#include <tsmartpointer.h>
#include <engine.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntityTree

CEntityTree::CEntityTree()
{
	m_entity = NULL;
	selectedPart.type = kNoType;
}

CEntityTree::~CEntityTree()
{
}

void CEntityTree::unbind ()
{
	// Delete all of the items in the tree and clear the instance index.
	if (::IsWindow(m_hWnd))
	{
		DeleteAllItems ();
	}
	m_entity = NULL;
}

void CEntityTree::bind_to_entity (ROS::ASceneEntity &entity)
{
	unbind();
	m_entity = &entity;

	// Add an entry for the root. This represents the entity itself and not any of its
	// components.
	if (::IsWindow(m_hWnd))
	{
		// Insert the default entries each entity gets.
		HTREEITEM hRoot = InsertItem ("<Entity>", TVI_ROOT);
		assert (hRoot != NULL);
//		SetItemData (hRoot, (DWORD) m_entity);
		SetItemData (hRoot, 0);

		add_hardpoints (hRoot);
		add_parts (hRoot);

		// Expand and select the first item in the list.
		Expand(hRoot, TVE_EXPAND);
		SelectItem (hRoot);
	}
}

HTREEITEM CEntityTree::add_hardpoints (HTREEITEM hParent)
{
	// Add the tree of hardpoints, if applicable. Only compound scene entities have
	// hardpoints.

	HTREEITEM hHps = NULL;

	ROS::ACompoundSceneEntity*	compSceneEntity;

    if(compSceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(m_entity))
    {
	    std::auto_ptr<ROS::MotionStateAccessor> ma = compSceneEntity->GetMotionStateAccessor();
		int count = ma->GetHardPointCount();

		if (count > 0)
		{
			hHps = InsertItem ("<Hardpoints>", hParent);
			assert (hHps != NULL);
			SetItemData (hHps, 0);

			for (int i = 0; i < count; ++i)
			{
				Part *part = new Part(kHardpoint, ma->GetHardPointName(i).c_str());
				part->hardIndex = i;

				HTREEITEM hItem = InsertItem (ma->GetHardPointName(i).c_str(), hHps);
				
				SetItemData (hItem, (DWORD) part);
			}
		}
    }

	return hHps;
}

HTREEITEM CEntityTree::add_parts (HTREEITEM hParent)
{
	// Add the tree of parts, if applicable.

	COMPTR<IEngine> ENG;
	COMPTR<IModel>  MOD;
	HTREEITEM hParts = NULL;

	// Get an IModel pointer.
	ENG = CharGetEngine();
	if (ENG)
	{
		if (ENG->QueryInterface (IID_IModel, MOD) == GR_OK)
		{
			ROS::ACompoundSceneEntity*	compSceneEntity;

			if(compSceneEntity = dynamic_cast<ROS::ACompoundSceneEntity*>(m_entity))
			{
				HTREEITEM hParts = InsertItem ("<Parts>", hParent);
				assert (hParts != NULL);
				SetItemData (hParts, 0);

				std::auto_ptr<ROS::MotionStateAccessor>	ma = compSceneEntity->GetMotionStateAccessor();
				INSTANCE_INDEX idx = (INSTANCE_INDEX) ma->GetRootEngineIndex();
				if (!add_tree (MOD, idx, hParts))
				{
					DeleteItem (hParts);
					hParts = NULL;
				}
			}
		}
	}

	return hParts;
}

HTREEITEM CEntityTree::add_tree (IModel *model, INSTANCE_INDEX root, HTREEITEM hParent)
{
	// If the root is not invalid, add an item for it, then add each of its child trees
	if (::IsWindow(m_hWnd))
	{
		if (root != INVALID_INSTANCE_INDEX)
		{
			// Add an item for the root, storing its instance index in the item data field.
			assert (model != NULL);
			const C8 *name = model->get_name (root);
			if (name == NULL)
			{
				name = "<unnamed>";
			}

			HTREEITEM hRoot = InsertItem (name, hParent);
			assert (hRoot != NULL);
			Part *part = new Part(kHardpoint, name);
			part->subIndex = root;
			SetItemData (hRoot, (DWORD) part);

			// Add a tree for each of the children of the root.
			INSTANCE_INDEX child = model->get_child (root);
			while (child != INVALID_INSTANCE_INDEX)
			{
				add_tree (model, child, hRoot);
				child = model->get_child (root, child);
			}

			return hRoot;
		}
	}

	return NULL;
}


BEGIN_MESSAGE_MAP(CEntityTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CEntityTree)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnDeleteitem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntityTree message handlers

void CEntityTree::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	HTREEITEM hSel = GetSelectedItem();
	if (hSel == NULL)
	{
		selectedPart.type = kNoType;
	}
	else
	{
		Part *part = (Part *) GetItemData (hSel);
		if (part == NULL)
		{
			selectedPart.type = kNoType;
		}
		else
		{
			selectedPart = *part;
		}
	}
	
	*pResult = 0;
}

void CEntityTree::OnDeleteitem(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	Part *part = (Part *) GetItemData (pNMTreeView->itemOld.hItem);
	if (part != NULL)
	{
		delete part;
	}

	*pResult = 0;
}
