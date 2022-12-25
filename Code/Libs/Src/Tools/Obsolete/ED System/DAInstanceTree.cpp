// DAInstanceTree.cpp : implementation file
//

// MFC Includes
#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "DAInstanceTree.h"

// Standard Includes
#include <assert.h>
// DA Library includes
#include <engine.h>
#include <tsmartpointer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDAInstanceTree

CDAInstanceTree::CDAInstanceTree()
{
	instance = INVALID_INSTANCE_INDEX;
	m_srcImageList = NULL;
	image_list_loaded = false;
}

CDAInstanceTree::~CDAInstanceTree()
{
	if (m_srcImageList != NULL)
	{
		delete m_srcImageList;
	}
}

void CDAInstanceTree::unbind ()
{
	// Delete all of the items in the tree and clear the instance index.
	if (::IsWindow(m_hWnd))
	{
		DeleteAllItems ();
	}
	instance = INVALID_INSTANCE_INDEX;
}

HTREEITEM CDAInstanceTree::add_tree (IModel *model, INSTANCE_INDEX root, HTREEITEM hParent)
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
				name = "<null>";
			}

			HTREEITEM hRoot = InsertItem (name, hParent);
			assert (hRoot != NULL);
			SetItemData (hRoot, root);

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

void CDAInstanceTree::bind_to_instance (IModel *model, INSTANCE_INDEX inst)
{
	load_image_list();

	// Set the instance for this control then force it to refresh.
	COMPTR<IModel> MODEL = model;
	unbind ();
	instance = inst;
	HTREEITEM hRoot = add_tree (MODEL, instance);
	if (hRoot)
	{
		Expand(hRoot, TVE_EXPAND);
	}
}

void CDAInstanceTree::set_item_ik_state (HTREEITEM hItem, IK_CHAIN_STATE state)
{
	load_image_list();
	SetItemState (hItem, INDEXTOSTATEIMAGEMASK(state), TVIS_STATEIMAGEMASK);
}

CDAInstanceTree::IK_CHAIN_STATE CDAInstanceTree::get_item_ik_state (HTREEITEM hItem)
{
	UINT state = (GetItemState (hItem, TVIS_STATEIMAGEMASK) >> 12);
	switch (state)
	{
	case 1:
		return IK_ROOT;
		break;

	case 2:
		return IK_MIDDLE;
		break;

	case 3:
		return IK_END;
		break;
	}
	return IK_NONE;
}

void CDAInstanceTree::clear_tree_ik_state (HTREEITEM hRoot)
{
	// Reset the state image to no image.

	SetItemState (hRoot, 0, TVIS_STATEIMAGEMASK);

	// Reset the tree for each of its children, if any.

	HTREEITEM hChild = GetChildItem (hRoot);
	while (hChild)
	{
		clear_tree_ik_state (hChild);
		hChild = GetNextSiblingItem (hChild);
	}
}

void CDAInstanceTree::load_image_list()
{
	if (!image_list_loaded)
	{
		// Load the image map.
		
		m_srcImageList = new CImageList;
		const int IK_IMAGE_WIDTH = 12;
		if (m_srcImageList->Create (IDB_IK_IMAGELIST, IK_IMAGE_WIDTH, 3, (COLORREF) 0x00FFFFFF))
		{
			image_list_loaded = true;

			// Make this the state image list for this control.
			SetImageList (m_srcImageList, TVSIL_STATE);
		}
	}
}

BEGIN_MESSAGE_MAP(CDAInstanceTree, CTreeCtrl)
	//{{AFX_MSG_MAP(CDAInstanceTree)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDAInstanceTree message handlers

int CDAInstanceTree::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	// Load the image map.

	load_image_list ();
	return 0;
}

