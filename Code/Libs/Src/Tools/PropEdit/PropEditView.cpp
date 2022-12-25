// PropEditView.cpp : implementation of the CPropEditView class
//

#include "stdafx.h"
#include "PropEdit.h"

#include "PropEditDoc.h"
#include "PropEditView.h"
#include "SimplePropEdit.h"
#include "VectorPropEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropEditView

IMPLEMENT_DYNCREATE(CPropEditView, CListView)

BEGIN_MESSAGE_MAP(CPropEditView, CListView)
	//{{AFX_MSG_MAP(CPropEditView)
	ON_COMMAND(ID_EDIT_INSERTPROPERTY, OnEditInsertProperty)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_VIEWCONTEXT_NEW_LONG, OnViewcontextNewLong)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_DOUBLE, OnViewcontextNewDouble)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_MATRIX, OnViewcontextNewMatrix)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_SINGLE, OnViewcontextNewSingle)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_STRING, OnViewcontextNewString)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_TRANSFORM, OnViewcontextNewTransform)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_ULONG, OnViewcontextNewUlong)
	ON_COMMAND(ID_VIEWCONTEXT_NEW_VECTOR, OnViewcontextNewVector)
	ON_COMMAND(ID_ITEMCONTEXT_EDIT, OnItemcontextEdit)
	ON_COMMAND(ID_ITEMCONTEXT_DELETE, OnItemcontextDelete)
	ON_NOTIFY_REFLECT(HDN_ITEMDBLCLICK, OnItemdblclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropEditView construction/destruction

CPropEditView::CPropEditView()
{
	// TODO: add construction code here
	// Load the context menu and sub-menus.

	mViewContextPos = -1;
	mItemContextPos = -1;
	if (mContextMenu.LoadMenu (IDR_VIEWCONTEXT))
	{
		UINT count = mContextMenu.GetMenuItemCount();
		for (UINT i = 0; i < count; ++i)
		{
			CString name;
			mContextMenu.GetMenuString (i, name, MF_BYPOSITION);
			if (name == "ViewContext")
			{
				mViewContextPos = (int) i;
			}
			else if (name == "ItemContext")
			{
				mItemContextPos = (int) i;
			}
		}
	}
}

CPropEditView::~CPropEditView()
{
	// Release the context menu.
	mContextMenu.DestroyMenu();
}

BOOL CPropEditView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditView drawing

void CPropEditView::OnDraw(CDC* pDC)
{
	CPropEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

void CPropEditView::OnInitialUpdate()
{
	// TODO: You may populate your ListView with items by directly accessing
	//  its list control through a call to GetListCtrl().

	CListCtrl& listCtl= GetListCtrl();

	listCtl.InsertColumn (0, "Name", LVCFMT_LEFT, 100);
	listCtl.InsertColumn (1, "Type", LVCFMT_LEFT, 100);
	listCtl.InsertColumn (2, "Value", LVCFMT_LEFT, 100);

	listCtl.ModifyStyle (LVS_ICON|LVS_SMALLICON|LVS_LIST, LVS_REPORT);

	CListView::OnInitialUpdate();

#if 0
	// Add the items to the view, one for each property.

	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		int count = pDoc->props.GetCount();
		POSITION pos = pDoc->props.GetHeadPosition();
		for (int i = 0; i < count; ++i)
		{
			Property &p = pDoc->props.GetNext(pos);
			int index = listCtl.InsertItem (i, p.name);
			set_property (index, p);
		}
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditView diagnostics

#ifdef _DEBUG
void CPropEditView::AssertValid() const
{
	CListView::AssertValid();
}

void CPropEditView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CPropEditDoc* CPropEditView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CPropEditDoc)));
	return (CPropEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPropEditView misc. methods

void CPropEditView::set_property (int index, Property &p)
{
	CListCtrl& listCtl= GetListCtrl();
	listCtl.SetItemText (index, 0, p.name);
	listCtl.SetItemText (index, 1, p.get_type_name());
	listCtl.SetItemText (index, 2, p.get_data_string());
}

void CPropEditView::add_property(Property &p)
{
	CPropEditDoc* pDoc = GetDocument();
	if (pDoc)
	{
		pDoc->add_property (p);
		CListCtrl& listCtl= GetListCtrl();
		int i = pDoc->props.GetCount() - 1;
		int index = listCtl.InsertItem (i, p.name);
		set_property (index, p);
		pDoc->UpdateAllViews (this);
	}
}

void CPropEditView::del_property (int which)
{
	CPropEditDoc* pDoc = GetDocument();
	if (pDoc)
	{
		pDoc->del_property (which);
		CListCtrl& listCtl= GetListCtrl();
		listCtl.DeleteItem (which);
		pDoc->UpdateAllViews (NULL);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPropEditView message handlers

void CPropEditView::OnEditInsertProperty() 
{
	// TODO: Add your command handler code here
	Property p("New property");
	p.set_long (0);
	add_property (p);
}

void CPropEditView::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	// Pop up the item context menu if over an item, otherwise pop up the view context menu.

	CPoint clientPoint = point;
	ScreenToClient (&clientPoint);

	CListCtrl& listCtl= GetListCtrl();
	UINT flags=0;
	int item = listCtl.HitTest (clientPoint, &flags);
	CMenu *menu = NULL;

	if (item != -1)
	{
		if (mItemContextPos != -1)
		{
			menu = mContextMenu.GetSubMenu (mItemContextPos);
		}
	}
	else
	{
		if (mViewContextPos != -1)
		{
			menu = mContextMenu.GetSubMenu (mViewContextPos);
		}
	}

	if (menu)
	{
		menu->TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	}
}

void CPropEditView::OnViewcontextNewLong() 
{
	// TODO: Add your command handler code here
	Property p("New Long");
	p.set_long (0);
	CSimplePropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::OnViewcontextNewDouble() 
{
	// TODO: Add your command handler code here
	Property p("New Double");
	p.set_double (0.0);
	CSimplePropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::OnViewcontextNewMatrix() 
{
	// TODO: Add your command handler code here
	Property p("New Matrix");
	PersistMatrix m(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
	p.set_matrix (&m);
	add_property (p);
}

void CPropEditView::OnViewcontextNewSingle() 
{
	// TODO: Add your command handler code here
	Property p("New Single");
	p.set_single (0.0f);
	CSimplePropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::OnViewcontextNewString() 
{
	// TODO: Add your command handler code here
	Property p("New String");
	p.set_string ("");
	CSimplePropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::OnViewcontextNewTransform() 
{
	// TODO: Add your command handler code here
	Property p("New Transform");
	PersistMatrix m(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
	PersistVector v(0.0, 0.0, 0.0);
	PersistTransform t(m,v);
	p.set_transform (&t);
	add_property (p);
}

void CPropEditView::OnViewcontextNewUlong() 
{
	// TODO: Add your command handler code here
	Property p("New ULong");
	p.set_ulong (0);
	CSimplePropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::OnViewcontextNewVector() 
{
	// TODO: Add your command handler code here
	Property p("New Vector");
	PersistVector v(0.0, 0.0, 0.0);
	p.set_vector (&v);
	CVectorPropEdit pEdit(p, this);
	if (pEdit.DoModal () == IDOK)
	{
		p = pEdit.p;
		add_property (p);
	}
}

void CPropEditView::edit_item (int nItem)
{
	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		Property *p = pDoc->get_property (nItem);
		if (p)
		{
			switch (p->type)
			{
			case PT_LONG:
			case PT_ULONG:
			case PT_SINGLE:
			case PT_DOUBLE:
			case PT_STRING:
				{
					CSimplePropEdit pEdit(*p, this);
					if (pEdit.DoModal () == IDOK)
					{
						*p = pEdit.p;
						set_property (nItem, *p);
						pDoc->SetModifiedFlag();
					}
				}
				break;

			case PT_VECTOR:
				{
					CVectorPropEdit pEdit(*p, this);
					if (pEdit.DoModal () == IDOK)
					{
						*p = pEdit.p;
						set_property (nItem, *p);
						pDoc->SetModifiedFlag();
					}
				}
				break;

			case PT_MATRIX:
			case PT_TRANSFORM:
				// *** TODO: Write edit dialogs for each of these types.
				break;
			}
		}
	}
}

void CPropEditView::OnItemcontextEdit() 
{
	// TODO: Add your command handler code here
	// The item context will ALWAYS be called for the selected item, so retrieve and edit it.
	
	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		CListCtrl& listCtl= GetListCtrl();

		POSITION pos = listCtl.GetFirstSelectedItemPosition();
		if (pos != NULL)
		{
			// Edit the items, in sequence.
			while (pos)
			{
				int nItem = listCtl.GetNextSelectedItem(pos);
				edit_item (nItem);
			}
		}
	}
}

void CPropEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class

	// Sync the list view to the contents of the document.
	// We will not use hints for now; we'll just duplicate the items.
	// Add the items to the view, one for each property.

	CListCtrl& listCtl= GetListCtrl();
	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		int propCount = pDoc->props.GetCount();
		int itemCount = listCtl.GetItemCount ();
		int count = max(propCount, itemCount);
		POSITION pos = pDoc->props.GetHeadPosition();
		int i;
		for (i = 0; i < count; ++i)
		{
			Property &p = pDoc->props.GetNext(pos);
			if (i >= itemCount)
			{
				int index = listCtl.InsertItem (i, p.name);
				if (index != -1)
				{
					ASSERT (index == i);
				}
				set_property (i, p);
			}
			else if (i >= propCount)
			{
				listCtl.DeleteItem (i);
			}
			else
			{
				set_property (i, p);
			}
		}
	}
}

void CPropEditView::OnItemcontextDelete() 
{
	// TODO: Add your command handler code here

	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		CListCtrl& listCtl= GetListCtrl();

		POSITION pos = listCtl.GetFirstSelectedItemPosition();
		while (pos != NULL)
		{
			// Edit the items, in sequence.
			int nItem = listCtl.GetNextSelectedItem(pos);
			del_property (nItem);
			pos = listCtl.GetFirstSelectedItemPosition();
		}
	}
}

void CPropEditView::OnItemdblclick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	// TODO: Add your control notification handler code here

	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		edit_item (phdn->iItem);
	}

	*pResult = 0;
}

void CPropEditView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here

	// I copied this from the code above. It seems that the code above doesn't actually work, i.e.
	// it doesn't get called on a double click. This one does, however.

	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	
	CPropEditDoc *pDoc = GetDocument();
	if (pDoc)
	{
		edit_item (phdn->iItem);
	}

	*pResult = 0;
}
