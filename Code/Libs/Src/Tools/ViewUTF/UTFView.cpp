// UTFView.cpp : implementation of the UTFView class
//

#include "stdafx.h"
#include "UTFApp.h"
#include <afxole.h>

#include "UTFDoc.h"
#include "CntrItem.h"
#include "UTFView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ChildFrm.h"
#include "MainFrm.h"
#include <Viewer.h>
#include "typelist.h"
#include "TGA.h"

//---------------------------------------------------------------------------
// NewFileDlg
//---------------------------------------------------------------------------

const char *NewFile = "*FILE";
const char *NewFolder = "*FOLDER";

static CString DefaultType(NewFile);


class NewFileDlg : public CDialog
{
public:
	NewFileDlg();

	~NewFileDlg (void)
	{
		DefaultType = type;
	}

// Dialog Data
	//{{AFX_DATA(NewFileDlg)
	enum { IDD = IDD_NEW_FILE };
	CString name;
	CString size;
	CString type;
	int ins_before;
	int ins_child;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(NewFileDlg)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(NewFileDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

NewFileDlg::NewFileDlg() : CDialog(NewFileDlg::IDD)
{
	//{{AFX_DATA_INIT(NewFileDlg)
	//}}AFX_DATA_INIT
	type = DefaultType;
	ins_before = 0;
	ins_child = 0;
}

BOOL NewFileDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CWnd *w = GetDlgItem(IDC_NEW_TYPE);
	CComboBox *list = (CComboBox *)w;
	{
		list->AddString(NewFile);
		list->AddString(NewFolder);

		POSITION pos;
		for (pos=TheTypeList.name_list.GetHeadPosition(); pos!=NULL;)
		{
			CString name = TheTypeList.name_list.GetNext(pos);
			list->AddString(name);
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void NewFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(NewFileDlg)
	DDX_Text(pDX,IDC_NEW_NAME,name);
	DDX_Text(pDX,IDC_NEW_SIZE,size);
	DDX_Text(pDX,IDC_NEW_TYPE,type);
	DDX_Check(pDX,IDC_INS_BEFORE,ins_before);
	DDX_Check(pDX,IDC_INS_CHILD,ins_child);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
	}

	CWnd *w = GetDlgItem(IDC_NEW_SIZE);
	if (w->IsKindOf(RUNTIME_CLASS(CEdit)))
	{
		int on = (type == NewFolder) || (type != NewFile);
		((CEdit *)w)->SetReadOnly(on);
	}
}

BEGIN_MESSAGE_MAP(NewFileDlg, CDialog)
	//{{AFX_MSG_MAP(NewFileDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// TypeDlg
//---------------------------------------------------------------------------

class TypeDlg : public CDialog
{
public:
	TypeDlg();

	~TypeDlg (void)
	{
		DefaultType = type;
	}

// Dialog Data
	//{{AFX_DATA(TypeDlg)
	enum { IDD = IDD_TYPE };
	CString type;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TypeDlg)
	protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(TypeDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

TypeDlg::TypeDlg() : CDialog(TypeDlg::IDD)
{
	//{{AFX_DATA_INIT(TypeDlg)
	//}}AFX_DATA_INIT
	type = DefaultType;
}

BOOL TypeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CWnd *w = GetDlgItem(IDC_NEW_TYPE);
	CComboBox *list = (CComboBox *)w;
	{
		list->AddString(NewFile);
		list->AddString(NewFolder);

		POSITION pos;
		for (pos=TheTypeList.name_list.GetHeadPosition(); pos!=NULL;)
		{
			CString name = TheTypeList.name_list.GetNext(pos);
			list->AddString(name);
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void TypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TypeDlg)
	DDX_Text(pDX,IDC_NEW_TYPE,type);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
	}
}

BEGIN_MESSAGE_MAP(TypeDlg, CDialog)
	//{{AFX_MSG_MAP(TypeDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
// UTFView
//---------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(UTFView, CTreeView)

BEGIN_MESSAGE_MAP(UTFView, CTreeView)
	//{{AFX_MSG_MAP(UTFView)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelectChanged)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_COMMAND(ID_REFRESH, OnRefresh)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
	ON_WM_CREATE()
	ON_COMMAND(ID_OLE_INSERT_NEW, OnInsertObject)
	ON_COMMAND(ID_CANCEL_EDIT_CNTR, OnCancelEditCntr)
	ON_COMMAND(ID_CANCEL_EDIT_SRVR, OnCancelEditSrvr)
	ON_COMMAND(ID_TOOLS_EXPORT_RAW, OnToolsExportRaw)
	ON_COMMAND(ID_TOOLS_EXPORT_TGA, OnToolsExportTga)
	ON_COMMAND(ID_TOOLS_IMPORT_RAW, OnToolsImportRaw)
	ON_COMMAND(ID_TOOLS_IMPORT_TGA, OnToolsImportTga)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------

void UTFView::set_status (const char *msg)
{
	CWnd *w = GetParent();
	if	(w->IsKindOf(RUNTIME_CLASS(ChildFrame)))
	{
		CMDIFrameWnd *f = ((ChildFrame *)w)->GetMDIFrame();
		if	(f->IsKindOf(RUNTIME_CLASS(MainFrame)))
		{
			((MainFrame *)f)->set_status(msg);
		}
	}
}

bool IsComplete (CTreeCtrl *ctrl, HTREEITEM parent)
{
	if (ctrl->GetItemData(parent) == 0)
	{
		CString name = ctrl->GetItemText(parent);
		return false;
	}
	HTREEITEM kid = ctrl->GetNextItem(parent,TVGN_CHILD);
	while (kid)
	{
		if (!IsComplete(ctrl,kid))
			return false;
		kid = ctrl->GetNextItem(kid,TVGN_NEXT);
	}
	return true;
}

#include "float.h" // _isnan()
#include "math.h" // fabs()

int IdenitfyFormat (void *data, int bytes)
{
	int quads = bytes/4;
	if (quads*4 == bytes)
	{
		int c = 0;
		for (int i=0; i<quads; i++)
		{
			float f = ((float *)data)[i];
			if (_isnan(f))
				{ c=0; break; }
			if (f > 1e9 && f < -1e9)
				{ c=0; break; }
			if (fabs(f) < 0.0001) // maybe not a good float?
				continue;
			if (fabs(f) > 1e6)
				continue;
			if (int(f) == f)
				c++; // an extra point
			c++;
		}
		if (c >= quads*0.40)
			return 'f';	// FLOATING POINT
	}

	{
		char *ptr = (char *)data;
		int c = 0;
		for (int i=0; i<bytes; i++)
		{
			char ch = ptr[i];
			if (ch==' ' || ch=='\n' || ch=='\r' || ch=='\t')
				c++;
			else if (ch >= 'a' && ch <= 'z')
				c++;
			else if (ch >= 'A' && ch <= 'Z')
				c++;
			else if (ch >= '0' && ch <= '9')
				c++;
		}
		if (c >= bytes*0.75)
			return 'c'; // CHARACTER STRING
	}

	if (quads*4 == bytes)
	{
		bool ok = true;
		for (int i=0; i<quads; i++)
		{
			int n = ((int *)data)[i];
			if (n > 0x7FFFF || n < -0x7FFFF)
				{ ok = false; break; }
		}
		if (ok)
			return 'i';	// FLOATING POINT
	}

	return 0; // unknown format
}

void UTFView::update_status (void)
{
	CString out = "Unknown";

	CTreeCtrl *ctrl = &GetTreeCtrl();
	HTREEITEM s = ctrl->GetSelectedItem();
    if (s)
    {
        Chunk *chunk = get_chunk(s);
		if (chunk)
		{
			int size = chunk->get_size();

			if (chunk->is_root())
			{
				out.Format("ROOT");
			}
			else
			{
				if (chunk->is_folder())
				{
					int count = chunk->get_count();
					const char *reliability = "+?";
					if (IsComplete(ctrl,s))
						reliability = "";

					out.Format("'%s' (files=%d%s, %d bytes) DOC=%lX",chunk->name,count-1,reliability,size,chunk->doc);
				}
				else
				{
					out.Format("'%s' (%d bytes) DOC=%lX",chunk->name,size,chunk->doc);
				}

				CTime t;
				t = chunk->ftLastWriteTime;
				out += t.Format("  Modified=%m/%d/%y %I:%M %p");
			}

			if (!chunk->is_folder())
			{
				unsigned long bytes=0;
				unsigned char data[16];
				chunk->doc->SetFilePointer(0,0);
				chunk->doc->ReadFile(0,data,sizeof(data),&bytes,0);

				char fmt = IdenitfyFormat(data,bytes);
				
				CString work;
				out += "  { ";
				unsigned i;
				switch (fmt)
				{
				default:
					bytes = min(8,bytes);
					for (i=0; i<bytes; i++)
					{
						work.Format("%02X ",data[i]);
						out += work;
					}
					break;

				case 'f':
					bytes = min(12,bytes);
					for (i=0; i<bytes; i+=4)
					{
						work.Format("%f ",*(float *)(data+i));
						out += work;
					}
					break;

				case 'i':
					bytes = min(12,bytes);
					for (i=0; i<bytes; i+=4)
					{
						work.Format("%d ",*(int *)(data+i));
						out += work;
					}
					break;

				case 'c':
					for (i=0; i<bytes; i++)
						if (data[i] == 0)
							out += ' ';
						else
							out += data[i];
					break;
				}
				if (unsigned(chunk->size) > bytes)
					out += "  ...";
				else
					out += " }";
			}
		}

    }

    set_status(out);
}


/////////////////////////////////////////////////////////////////////////////
// UTFView construction/destruction
CLIPFORMAT UTFView::m_cfObjectDescriptor =
    (CLIPFORMAT)::RegisterClipboardFormat(_T("Object Descriptor"));


UTFView::UTFView()
{
	m_prevDropEffect = DROPEFFECT_NONE;


	dragging = FALSE;
	drag_item = 0;
	drop_item = 0;

	HICON drag_icon = LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDI_DRAG));
	my_image_list.Create(16,16,ILC_MASK|ILC_COLOR4,1,1);
	my_image_list.Add(drag_icon);

	image_list = &my_image_list;
}

UTFView::~UTFView()
{
}

BOOL UTFView::PreCreateWindow(CREATESTRUCT& cs)
{
// set_folder_style = draw lines to children with open/close buttons

	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS;
	cs.style |= TVS_SHOWSELALWAYS;
    cs.style |= TVS_EDITLABELS;

	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// UTFView drawing

void UTFView::OnDraw(CDC* pDC)
{
	UTFDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
	// TODO: also draw all OLE items in the document

	// Draw the selection at an arbitrary position.  This code should be
	//  removed once your real drawing code is implemented.  This position
	//  corresponds exactly to the rectangle returned by CUTFAppCntrItem,
	//  to give the effect of in-place editing.

	// TODO: remove this code when final draw code is complete.

	if (m_pSelection == NULL)
	{
		POSITION pos = pDoc->GetStartPosition();
		m_pSelection = (CUTFAppCntrItem*)pDoc->GetNextClientItem(pos);
	}
	if (m_pSelection != NULL)
		m_pSelection->Draw(pDC, CRect(10, 10, 210, 210));
}

/////////////////////////////////////////////////////////////////////////////
// UTFView printing

BOOL UTFView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void UTFView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void UTFView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void UTFView::OnDestroy()
{
	// Deactivate the item on destruction; this is important
	// when a splitter view is being used.
	COleClientItem	*pActiveItem	=GetDocument()->GetInPlaceActiveItem(this);
	if(pActiveItem)
	{
		if(pActiveItem->GetActiveView() == this)
		{
			pActiveItem->Deactivate();
			ASSERT(!GetDocument()->GetInPlaceActiveItem(this));
		}
	}
	CView::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
// OLE Client support and commands

BOOL UTFView::IsSelected(const CObject* pDocItem) const
{
	// The implementation below is adequate if your selection consists of
	//  only CUTFAppCntrItem objects.  To handle different selection
	//  mechanisms, the implementation here should be replaced.

	// TODO: implement this function that tests for a selected OLE client item

	return pDocItem == m_pSelection;
}

void UTFView::OnInsertObject()
{
	// Invoke the standard Insert Object dialog box to obtain information
	//  for new CUTFAppCntrItem object.
	COleInsertDialog dlg;
	if (dlg.DoModal() != IDOK)
		return;

	BeginWaitCursor();

	CUTFAppCntrItem* pItem = NULL;
	TRY
	{
		// Create new item connected to this document.
		UTFDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		pItem = new CUTFAppCntrItem(pDoc);
		ASSERT_VALID(pItem);

		// Initialize the item from the dialog data.
		if (!dlg.CreateItem(pItem))
			AfxThrowMemoryException();  // any exception will do
		ASSERT_VALID(pItem);
		
        if (dlg.GetSelectionType() == COleInsertDialog::createNewItem)
			pItem->DoVerb(OLEIVERB_SHOW, this);

		ASSERT_VALID(pItem);

		// As an arbitrary user interface design, this sets the selection
		//  to the last item inserted.

		// TODO: reimplement selection as appropriate for your application

		m_pSelection = pItem;   // set selection to last inserted item
		pDoc->UpdateAllViews(NULL);
	}
	CATCH(CException, e)
	{
		if (pItem != NULL)
		{
			ASSERT_VALID(pItem);
			pItem->Delete();
		}
		AfxMessageBox(IDP_FAILED_TO_CREATE);
	}
	END_CATCH

	EndWaitCursor();
}

BOOL UTFView::GetObjectInfo(COleDataObject* pDataObject,
   CSize* pSize, CSize* pOffset)
{
   ASSERT(pSize != NULL);

   // get object descriptor data
   HGLOBAL hObjDesc = 
            pDataObject->GetGlobalData(m_cfObjectDescriptor);
   if (hObjDesc == NULL)
   {
      if (pOffset != NULL)
         *pOffset = CSize(0, 0); // fill in defaults instead
      *pSize = CSize(0, 0);
      return FALSE;
   }
   ASSERT(hObjDesc != NULL);

   // else, got CF_OBJECTDESCRIPTOR. Lock it down and extract size.
   LPOBJECTDESCRIPTOR pObjDesc =
               (LPOBJECTDESCRIPTOR)GlobalLock(hObjDesc);
   ASSERT(pObjDesc != NULL);
   pSize->cx = (int)pObjDesc->sizel.cx;
   pSize->cy = (int)pObjDesc->sizel.cy;
   if (pOffset != NULL)
   {
      pOffset->cx = (int)pObjDesc->pointl.x;
      pOffset->cy = (int)pObjDesc->pointl.y;
   }
   GlobalUnlock(hObjDesc);
   GlobalFree(hObjDesc);

   // successfully retrieved pSize & pOffset info
   return TRUE;
}


// The following command handler provides the standard keyboard
//  user interface to cancel an in-place editing session.  Here,
//  the container (not the server) causes the deactivation.
void UTFView::OnCancelEditCntr()
{
	// Close any in-place active item on this view.
	COleClientItem* pActiveItem = GetDocument()->GetInPlaceActiveItem(this);
	if (pActiveItem != NULL)
	{
		pActiveItem->Close();
	}
	ASSERT(GetDocument()->GetInPlaceActiveItem(this) == NULL);
}
/////////////////////////////////////////////////////////////////////////////
// OLE Server support

// The following command handler provides the standard keyboard
//  user interface to cancel an in-place editing session.  Here,
//  the server (not the container) causes the deactivation.
void UTFView::OnCancelEditSrvr()
{
	GetDocument()->OnDeactivateUI(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// UTFView diagnostics

#ifdef _DEBUG
void UTFView::AssertValid() const
{
	CView::AssertValid();
}

void UTFView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

UTFDoc* UTFView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(UTFDoc)));
	return (UTFDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// UTFView message handlers

//---------------------------------------------------------------------------

void UTFView::expand_all (HTREEITEM i, UINT xcode)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();

	if (i == NULL)
		i = ctrl->GetRootItem();

	if (i)
	{
		ctrl->Expand(i,xcode);

		HTREEITEM kid = ctrl->GetNextItem(i,TVGN_CHILD);
		while (kid)
		{
			expand_all(kid,xcode);
			kid = ctrl->GetNextItem(kid,TVGN_NEXT);
		}
	}
}

//---------------------------------------------------------------------------

CString UTFView::make_name (Chunk *chunk)
{
    CString name;
	if (chunk == 0) // allow temporary children
		name = "*PlaceHolder*";
    else if (chunk->is_folder())
		name.Format("[%s]",chunk->name);
	else // FILE
		name.Format("%s  (%d)" ,chunk->name,chunk->size);
    return name;
}

//---------------------------------------------------------------------------

HTREEITEM UTFView::add_item (Chunk *chunk, HTREEITEM parent, HTREEITEM after)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();

	char tmp[256];	// buffer used to pass node's name
	strcpy(tmp,make_name(chunk));

	TV_INSERTSTRUCT node;
	node.item.mask = TVIF_TEXT|TVIF_PARAM;
	node.hParent = parent;
	node.hInsertAfter = after;
	node.item.pszText = tmp;
	node.item.lParam = (int)chunk; // store pointer

	HTREEITEM item = ctrl->InsertItem(&node);

	if (chunk && chunk->is_folder())
	{
		ctrl->SetItemState(item,TVIS_BOLD,TVIS_BOLD);
	}

	return item;
}

//---------------------------------------------------------------------------

void UTFView::build (Chunk *list, HTREEITEM parent)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();

	UTFDoc *doc = GetDocument();

    for (Chunk *chunk=list; chunk; chunk = chunk->next)
    {
        HTREEITEM item = add_item(chunk,parent);

        if (chunk->child)
        {
            build(chunk->child,item);
        }
		else
		{
			if (chunk->is_folder() && chunk->read_levels == 0)
			{
				TV_INSERTSTRUCT node;
				node.item.mask = TVIF_TEXT|TVIF_PARAM;
				node.hParent = item;
				node.hInsertAfter = 0;
				node.item.pszText = "*PlaceHolder*";
				node.item.lParam = 0; // null pointer

				ctrl->InsertItem(&node);
			}
		}
    }
}

//---------------------------------------------------------------------------

static bool ready = false;

void UTFView::OnInitialUpdate()
{
	UTFDoc *doc = GetDocument();
	CTreeCtrl *ctrl = &GetTreeCtrl();

	CTreeView::OnInitialUpdate();

// setup window frame for view

	CWnd *win = GetParent();
	RECT r;
	win->GetClientRect(&r);
	int w = 0x140;
	int h = r.bottom;
	win->SetWindowPos(NULL,0,0,w,h,SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);

// style appearance

// initialize tree list

ShowWindow(SW_HIDE);
    build(doc->get_root());
	build_time = doc->modified_time;
	HTREEITEM root = ctrl->GetRootItem();
	if (root)
		ctrl->Expand(root,TVE_EXPAND);
ShowWindow(SW_SHOW);
	if (root)
		ctrl->EnsureVisible(root);

	ready = true;
}

//---------------------------------------------------------------------------

int UTFView::get_line (CPoint &point)
{
	TEXTMETRIC tm;
	CDC *pDC = GetDC();
	pDC->GetTextMetrics(&tm);

	int RowHeight = tm.tmHeight;
	int line = point.y / RowHeight;

	return (line);
}

//---------------------------------------------------------------------------

Chunk *UTFView::get_chunk (HTREEITEM s)
{
    Chunk *chunk = NULL;
    if (s)
    {
        chunk = (Chunk *)GetTreeCtrl().GetItemData(s);
    }
    return chunk;
}

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

int UTFView::view (IDocument *doc, char *class_name)
{
	IViewer *viewer = 0;
	VIEWDESC vdesc;

	//
	// now create a viewer for the document.
	// our viewer responds to the className "SampleViewer"
	//

	vdesc.className = class_name;
	vdesc.doc = doc;
	
	if (DACOM->CreateInstance(&vdesc, (void **) &viewer) == GR_OK)
	{

RECT rect;
rect.left = 320;
rect.right = 640;
rect.top = 128;
rect.bottom = 320;
viewer->set_rect(&rect);

char fname[128];
doc->GetFileName(fname,sizeof(fname));
int fsize = doc->GetFileSize(0,0);
char title[256];
sprintf(title,"'%s' (0x%X)  %s",fname,fsize,GetDocument()->filename);
viewer->set_instance_name(title);

		HWND w;
		viewer->get_main_window((void**)&w);
		HWND p = ((MainFrame *)AfxGetApp()->m_pMainWnd)->m_hWndMDIClient;
		if (w)
		{
			::SetParent(w,p);
		}

		viewer->set_display_state(TRUE);

		if (w)
			::SetFocus(w);

		viewer->Release();
	}

	return (viewer != 0);
}

//---------------------------------------------------------------------------

void UTFView::view_item (HTREEITEM s, const char *type_cast)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();
	if (s==0)
		s = ctrl->GetSelectedItem();
    Chunk *chunk = get_chunk(s);
	if (chunk && !chunk->is_root())
	{
		CString type_name;
		
		if (type_cast)
			type_name = type_cast;
		else
		{
			type_name = chunk->type;
			if (type_name.IsEmpty())
				type_name = GetTypeName(chunk->name);
		}

		if (view(chunk->doc,(char *)(const char *)type_name) == 0)
		{
			if (!chunk->is_folder())
			{
				if (view(chunk->doc,"HexViewer") == 0)
				{
					AfxMessageBox("No VIEWER registered?", MB_ICONSTOP | MB_OK);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------

void UTFView::toggle_item (void)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();
	HTREEITEM s = ctrl->GetSelectedItem();
    if (s)
    {
		ctrl->Expand(s,TVE_TOGGLE);
		ctrl->EnsureVisible(s);
    }
}

//---------------------------------------------------------------------------

void UTFView::OnLButtonDblClk (UINT nFlags, CPoint point) 
{
	CTreeCtrl *ctrl = &GetTreeCtrl();
	HTREEITEM s = ctrl->GetSelectedItem();

    if (s == 0)
        return;

	// Fix: TreeCtrl thinking DblClk is label edit
	TreeView_EndEditLabelNow(GetTreeCtrl().m_hWnd,TRUE);

	Chunk *chunk = get_chunk(s);

	if (0) //chunk == GetDocument()->get_root())
	{
		if (ctrl->GetItemState(s,TVIS_EXPANDED) & TVIS_EXPANDED)
			expand_all(NULL,TVE_COLLAPSE);
		else
			expand_all();

		ctrl->EnsureVisible(s);
	}
	else
	{
		if (s)
		{
		ShowWindow(SW_HIDE);
			if (ctrl->GetItemState(s,TVIS_EXPANDED) & TVIS_EXPANDED)
			{
				expand_all(s,TVE_COLLAPSE);
			}
			else
			{
				expand_all(s,TVE_EXPAND);
			}
			ctrl->EnsureVisible(s);
		ShowWindow(SW_SHOW);
		}
		else
		{
			CTreeView::OnLButtonDblClk(nFlags, point);
		}

        view_item();
	}
}

//---------------------------------------------------------------------------

void UTFView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTreeView::OnLButtonDown(nFlags, point);
}

void UTFView::cast_item (HTREEITEM s)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();
	if (s == 0)
		s = ctrl->GetSelectedItem();

    if (s)
	{
		TypeDlg dlg;
		if (dlg.DoModal() == IDOK)
		{
			Chunk *chunk = get_chunk(s);
			if (chunk)
				chunk->type = dlg.type;
			view_item(s,dlg.type);
		}
	}
}

void UTFView::OnRButtonDown (UINT nFlags, CPoint point) 
{
	CTreeView::OnRButtonDown(nFlags, point);

	CTreeCtrl *ctrl = &GetTreeCtrl();
	HTREEITEM item = ctrl->HitTest(point,0);
	ctrl->SelectItem(item);
	cast_item(item);
}


//---------------------------------------------------------------------------

HTREEITEM UTFView::find_item (DWORD data, HTREEITEM i)
{
    HTREEITEM found = 0;
	CTreeCtrl *ctrl = &GetTreeCtrl();
	if (i == NULL)
		i = ctrl->GetRootItem();
	while (i)
	{
        if (ctrl->GetItemData(i) == data)
        {
            found = i;
            break;
        }

		HTREEITEM kid = ctrl->GetNextItem(i,TVGN_CHILD);
		if (kid)
        {
            found = find_item(data,kid);
            if (found)
                break;
        }
		i = ctrl->GetNextItem(i,TVGN_NEXT);
	}
    return found;
}

//---------------------------------------------------------------------------

void UTFView::refresh (void)
{
    UTFDoc *doc = GetDocument();
	CTreeCtrl *ctrl = &GetTreeCtrl();

    if (doc->out_of_date(&build_time))     // has file changed?
    {
        CString name;
        Chunk *chunk;
        HTREEITEM s = ctrl->GetSelectedItem();
        if (s)
        {
            chunk = get_chunk(s);
            doc->build_path(name,chunk);
        }

        ctrl->DeleteAllItems();
        doc->refresh();
        build(doc->get_root());
		build_time = doc->modified_time;

        if (s)
        {
            chunk = doc->find_chunk(name);
            s = find_item((DWORD)chunk);
            ctrl->EnsureVisible(s);
            ctrl->SelectItem(s);
        }
    }
}

//---------------------------------------------------------------------------

void UTFView::delete_item (void)
{
    UTFDoc *doc = GetDocument();
	CTreeCtrl *ctrl = &GetTreeCtrl();

    refresh();

	HTREEITEM s = ctrl->GetSelectedItem();
    if (s)
	{
		int ok = 0;
		if (doc->request_modify())      // is there WRITE access?
		{
			Chunk *chunk = get_chunk(s);
			if (chunk)
			{
				//FUTURE: delete children
				if (chunk->is_root())	// never delete root!
					return;
				if (doc->remove_chunk(chunk))
				{
					ctrl->DeleteItem(s);
					ok = 1;
				}
				else
				{
					ctrl->DeleteItem(s);	// FIX THIS... but for now delete it anyway!
					refresh();
				}
			}
		}
		if (!ok)
			MessageBeep(0);
	}
}

//---------------------------------------------------------------------------

void UTFView::insert_chunks (HTREEITEM parent, Chunk *chunk)
// add chunk list to TREE, assume list already exist in document
{
	Chunk *c;
	for (c=chunk; c!=0; c=c->next)
	{
		HTREEITEM p = add_item(c,parent,TVI_LAST);
		if (c->child)
			insert_chunks(p,c->child);
	}
}

Chunk *UTFView::insert_chunk (HTREEITEM parent, const char *name, int size, const void *ptr)
// (parent == 0) insert at root level
// (size == -1) insert a folder
// (ptr == 0) insert block of zeroes
{
    UTFDoc *doc = GetDocument();

	Chunk *new_chunk;

	new_chunk = doc->insert_chunk(name,size,get_chunk(parent));

	if (new_chunk)
	{
		HTREEITEM after = TVI_LAST;			// IFileSystem restriction for now?
		add_item(new_chunk,parent,after);
	}
	else
	{
		AfxMessageBox("Failed to INSERT chunk!", MB_ICONSTOP | MB_OK);
	}

	return new_chunk;
}

//---------------------------------------------------------------------------

void UTFView::insert_item (void)
{
    UTFDoc *doc = GetDocument();
	CTreeCtrl *ctrl = &GetTreeCtrl();

	NewFileDlg dlg;

    refresh();

	CString type;

    if (doc->request_modify())      // is there WRITE access?
    {
		HTREEITEM parent = ctrl->GetSelectedItem();
		HTREEITEM after = TVI_LAST;

		int size;
		CString name;

        Chunk *chunk = get_chunk(parent);

		if (chunk && chunk->is_folder())
			dlg.ins_child = 1;

		int done = 0;
		while (!done)
		{
			if (dlg.DoModal() != IDOK)
				break;

			name = dlg.name;
			
			if (dlg.type == NewFolder)
				size = -1;
			else if (dlg.type == NewFile)
				size = atoi(dlg.size);
			else
			{
				size = TheTypeList.get_size(dlg.type);
				if (size < 1)
					break;
				type = dlg.type;
			}

			done = 1; //VERIFY NAME
		}

		if (!done)
			return;

		if (chunk && !chunk->is_folder())
		{
			dlg.ins_child = 0;
		}

		if (parent && chunk)
		{
			if (dlg.ins_child)
			{
				after = TVI_FIRST;
			}
			else
			{
				after = parent;
				parent = ctrl->GetParentItem(after);

				if (dlg.ins_before)
				{
					after = ctrl->GetPrevSiblingItem(after);
					if (after == 0)
						after = TVI_FIRST;
				}
			}
		}
		else // add to root
		{
			parent = TVI_ROOT;
			after = TVI_LAST;
		}

		Chunk *new_chunk = insert_chunk(parent,name,size,0);

		if (new_chunk)
		{
			if (!type.IsEmpty())
				new_chunk->type = type;
		}
    }
}

//---------------------------------------------------------------------------

void UTFView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
    switch (nChar)
    {
        case VK_INSERT:
			if (GetKeyState(VK_CONTROL) < 0)
				copy();
			else if (GetKeyState(VK_SHIFT) < 0)
				paste();
			else
				insert_item();
            return;

        case VK_DELETE:
			if (GetKeyState(VK_SHIFT) < 0)
				cut();
			else
	            delete_item();
            return;

        case VK_SPACE:
            toggle_item();
            return;

        case VK_RETURN:
			cast_item();
			return;

		case 'C':
			if (GetKeyState(VK_CONTROL) < 0)
			{
				copy();
				return;
			}
			break;

		case 'V':
			if (GetKeyState(VK_CONTROL) < 0)
			{
				paste();
				return;
			}
			break;

		case 'X':
			if (GetKeyState(VK_CONTROL) < 0)
			{
				cut();
				return;
			}
			break;
    }
	
	CTreeView::OnKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------

char UTF_CLIP_FORMAT[] = "UTF CHUNKS";

UINT CF_UTF = 0;

void EditInit (void)
{
	CF_UTF = RegisterClipboardFormat(UTF_CLIP_FORMAT); 
}

struct ClipFile
{
	static CString get_name (const char *name)
	{
		char bfr[256];
		int size = GetTempPath(sizeof(bfr),bfr);
		return CString(bfr) + name;
	}

	static bool copy (int format, const void *ptr, int size)
	{
		HWND hwndMain = AfxGetMainWnd()->m_hWnd;

		bool ok = FALSE;

		if (OpenClipboard(hwndMain))
		{
			EmptyClipboard(); 

			HGLOBAL hCopy;
			hCopy = GlobalAlloc(GMEM_DDESHARE, size);
			if (hCopy)
			{
				void *dst = GlobalLock(hCopy);
				if (dst)
				{
					memcpy(dst,ptr,size);
					GlobalUnlock(hCopy);

					SetClipboardData(format, hCopy); 
			        SetClipboardData(CF_TEXT, 0); 

					ok = TRUE;
				}
			}
			CloseClipboard(); 
		}
		return ok;
	}

	static int paste (int format, void *ptr, int max_size)
	{
		HWND hwndMain = AfxGetMainWnd()->m_hWnd;

		int size = 0;

		if (IsClipboardFormatAvailable(format))
		{
	        if (OpenClipboard(hwndMain)) 
			{
				HGLOBAL hCopy;
				hCopy = GetClipboardData(format); 
				if (hCopy)
				{
					size = GlobalSize(hCopy);
					if (size > max_size)
						return 0;
					void *src = GlobalLock(hCopy);
					memcpy(ptr,src,size);
					GlobalUnlock(hCopy);
				}
				CloseClipboard(); 
			}
		}

		return size;
	}
};

//---------------------------------------------------------------------------

struct ChunkCopier : ClipFile
{
	static bool copy (Chunk *chunk)
	{
		bool ok = FALSE;

		if (chunk == 0)
			return FALSE;

		CString name = get_name("~clip.utf");

		DOCDESC desc(name);
		desc.lpImplementation = "UTF";
		IFileSystem *sys = FS_Create(&desc,0);
		if (sys)
		{
			char bfr[256];
			int size = name.GetLength();
			if (size >= sizeof(bfr))
				size = sizeof(bfr)-1;
			memcpy(bfr,name,size);
			bfr[size++] = 0;

			ok = ClipFile::copy(CF_UTF,name,size);

			WriteChunks(sys,chunk,0);
			WriteChunks(sys,chunk,1);

			sys->Release();
		}
		return ok;
	}

	static Chunk *paste (IFileSystem *dst)
	{
		Chunk *chunk = 0;
		char name[256];
		if (ClipFile::paste(CF_UTF,name,sizeof(name)))
		{
			DOCDESC desc(name);
			desc.lpImplementation = "UTF";
			IFileSystem *src = FS_Open(&desc,"r",0);
			if (src)
			{
				chunk = CopyChunks(dst,src);
				src->Release();
			}
		}
		return chunk;
	}
};

//---------------------------------------------------------------------------

bool EditCopy (Chunk *chunk) 
{
	if (CF_UTF == 0)
		EditInit();

	return ChunkCopier::copy(chunk);
}

Chunk *EditPaste (IFileSystem *sys)
{
	return ChunkCopier::paste(sys);
}

bool UTFView::cut (HTREEITEM src)
{
	HTREEITEM s = src;
	if (s == 0)
		s = GetTreeCtrl().GetSelectedItem();
    if (s == 0)
		return FALSE;
    Chunk *chunk = get_chunk(s);
	bool ok = EditCopy(chunk);
	if (ok)
	{
		delete_item();
	}
	else
		MessageBeep(0);
	return ok;
}

bool UTFView::copy (HTREEITEM src)
{
	HTREEITEM s = src;
	if (s == 0)
		s = GetTreeCtrl().GetSelectedItem();
    if (s == 0)
		return FALSE;
    Chunk *chunk = get_chunk(s);
	bool ok = EditCopy(chunk);
	if (!ok)
		MessageBeep(0);
	return ok;
}

bool UTFView::paste (HTREEITEM target)
{
	Chunk *chunk = 0;
	UTFDoc *doc = GetDocument();

	if (doc->request_modify())
	{
		HTREEITEM s = target;
		if (s == 0)
			s = GetTreeCtrl().GetSelectedItem();

		IFileSystem *dst = 0;

		Chunk *parent;
		if (s == 0)
		{
			parent = 0;
			dst = doc->doc;
		}
		else
		{
			parent = get_chunk(s);
			dst = parent->doc;

			if (dst == 0)
				dst = doc->doc;
		}

		chunk = EditPaste(dst);

		if (chunk)
		{
			doc->SetModifiedFlag(TRUE);

			if (parent)
				parent->append_child(chunk);
			else
				doc->list = chunk;

			insert_chunks(s,chunk);
		}
		else
		{
			MessageBeep(0);
		}
	}

	return chunk != 0;
}

//---------------------------------------------------------------------------

void UTFView::OnSelectChanged (NMHDR *pNMHDR, LRESULT *pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	refresh();

	update_status(); // UPDATE SELECTED INFO
	
	*pResult = 0;
}

//---------------------------------------------------------------------------

int IsValidName (LPCSTR name)
{
    return name && name[0];
}

#define BEEP() MessageBeep(0xFFFFFFFF)

//---------------------------------------------------------------------------

static char TmpString[256];

void UTFView::OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* msg = (TV_DISPINFO*)pNMHDR;

	UTFDoc *doc = GetDocument();
	CTreeCtrl *ctrl = &GetTreeCtrl();

	int cancel = TRUE;

    refresh();

	HTREEITEM sel = ctrl->GetSelectedItem();
	if (sel &&
        doc->request_modify())
	{
        Chunk *chunk = get_chunk(sel);
        if (chunk && !chunk->is_root())
        {
            HWND h = TreeView_GetEditControl(ctrl->m_hWnd);
            CWnd *w = CWnd::FromHandle(h);
            if (w)
                w->SetWindowText(chunk->name);

    		cancel = (w == NULL);
        }
	}
	
	*pResult = cancel;
}

//---------------------------------------------------------------------------

void UTFView::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO *msg = (TV_DISPINFO*)pNMHDR;

	UTFDoc *doc = GetDocument();
	CTreeCtrl &tree = GetTreeCtrl();

	int ok = FALSE;

	HTREEITEM cur = msg->item.hItem;
	if (cur)
	{
		if (msg->item.mask & TVIF_TEXT)
		{
			CString old_name = tree.GetItemText(cur);
			CString new_name = msg->item.pszText;

			CString err;

			ok = IsValidName(new_name);
			if (ok)	// NAME MUST BE UNIQUE FOR LIST
			{
				HTREEITEM i = tree.GetNextItem(cur,TVGN_PARENT);
				i = tree.GetNextItem(i,TVGN_CHILD);
				for (; i; i = tree.GetNextItem(i,TVGN_NEXT))
				{
                    Chunk *ch = get_chunk(i);
					if (i != cur &&  new_name == ch->name)
					{
						err.Format("The label '%s' is already in use.\nPlease specify a unique name.",new_name);
						break;
					}
				}
			}
			else
			{
				err.Format("The label '%s' is not valid here.\nPlease specify a valid name.",new_name);
			}

			if (!err.IsEmpty())
			{
				ok = FALSE;
				BEEP();
				AfxMessageBox(err, MB_ICONSTOP | MB_OK);
			}
			else
			{
                Chunk *chunk = get_chunk(cur);
                ok = doc->rename(chunk,new_name);
                if (ok)
                {
                    strcpy(TmpString,make_name(chunk));
                    msg->item.pszText = TmpString;
                }
			}
		}
	}
	
	*pResult = ok;
}

//---------------------------------------------------------------------------

void UTFView::OnRefresh() 
{
    refresh();
}

static	int	FS_CopySys(IFileSystem *dst, IFileSystem *src)
{
	DWORD size = src->GetFileSize();
	void *ptr = ::malloc(size+1024);
	if (ptr)
	{
		src->SetFilePointer(0,0);
		U32 bytes;
		src->ReadFile(0,ptr,size,&bytes,0);
		if (bytes == size)
		{
			dst->WriteFile(0,ptr,size,&bytes,0);
		}
		else
		{
			size = 0;	// ERORR: could not read
		}
		::free(ptr);
	}
	else
	{
		size = 0;	// ERORR: could not allocate
	}
	return (size);
}


Chunk	*UTFView::InsertDropped(char		*fn,		//full filename
								IFileSystem	*dst,		//dest fs
								Chunk		*parent)	//parent chunk
{
	WIN32_FIND_DATA		data;
	HANDLE				handle;
	IFileSystem			*sys;

	sys		=NULL;
	handle	=FindFirstFile(fn, &data);

	if(handle != INVALID_HANDLE_VALUE)
	{
		Chunk	*chunk	=new Chunk;

		ASSERT(chunk);
		FindClose(handle);

		chunk->init();
		chunk->open(data);

		DOCDESC desc(fn);
		desc.lpImplementation = "UTF";
		chunk->doc	=(IDocument *)FS_Open(&desc, "r", sys);

		if(!chunk->doc)	//this happens sometimes
		{
			AfxMessageBox("Warning: Failed doc creation");
			return	NULL;
		}

		//look for binary files containing fs info
		if(!chunk->is_folder())
		{
			handle	=chunk->doc->FindFirstFile("*.*", &data);
			if(handle != INVALID_HANDLE_VALUE)
			{
				//recurse into the file system
				sys	=chunk->doc;
				chunk->set_folder();
			}
			chunk->doc->FindClose(handle);
		}

		chunk->read_levels	=parent->read_levels - 1;

		parent->append_child(chunk);

		//drop doc into parent filesys
		if(chunk->is_folder())
		{
			dst->CreateDirectory(chunk->name);
			if(!dst->SetCurrentDirectory(chunk->name))
			{
				return	chunk;	//fix this
			}
		}
		else
		{
			//copy the doc into the parent's sys
			DOCDESC	desc(chunk->name);

			IFileSystem	*newfs	=FS_Create(&desc, dst);
			FS_CopySys(newfs, chunk->doc);

			chunk->doc->Release();
			chunk->doc	=(IDocument *)newfs;
		}

		if(chunk->is_folder())
		{
			char	temp[_MAX_PATH];
			char	temp2[_MAX_PATH];

			memset(&data, 0, sizeof(data));

			if(sys)
			{
				handle	=sys->FindFirstFile("*.*", &data);
			}
			else
			{
				sprintf(temp, "%s\\*.*", fn);

				handle	=FindFirstFile(temp, &data);
			}

			while(handle != INVALID_HANDLE_VALUE)
			{
				if(data.cFileName[0] != '.')
				{
					if(sys)
					{
						InsertDroppedSys(sys, data.cFileName, dst, chunk);
					}
					else
					{
						strncpy(temp2, temp, strlen(temp) - 3);
						temp2[strlen(temp) - 3]	=0;	//terminate
						strcat(temp2, data.cFileName);

						InsertDropped(temp2, dst, chunk);
					}
				}
				if(sys)
				{
					if(!sys->FindNextFile(handle, &data))
					{
						break;
					}
				}
				else
				{
					if(!FindNextFile(handle, &data))
					{
						break;
					}
				}
			}
			if(sys)
			{
				sys->FindClose(handle);
			}
			else
			{
				FindClose(handle);
			}
			dst->SetCurrentDirectory("..");
		}
		return	chunk;
	}

	FindClose(handle);
	return	NULL;
}


//sys to recurse with a fs
Chunk	*UTFView::InsertDroppedSys(IFileSystem	*sys,		//fs dropping from
								   char		*fn,		//full filename
								   IFileSystem	*dst,		//dest fs
								   Chunk		*parent)	//parent chunk
{
	WIN32_FIND_DATA		data;
	HANDLE				handle;
	IFileSystem			*sys2	=NULL;

	handle	=sys->FindFirstFile(fn, &data);

	if(handle != INVALID_HANDLE_VALUE)
	{
		Chunk	*chunk	=new Chunk;

		ASSERT(chunk);
		sys->FindClose(handle);

		chunk->init();
		chunk->open(data);

		DOCDESC desc(fn);
		desc.lpImplementation = "UTF";
		chunk->doc	=(IDocument *)FS_Open(&desc, "r", sys);

		if(!chunk->doc)	//this happens sometimes
		{
			AfxMessageBox("Warning: Failed doc creation");
			return	NULL;
		}

		//look for binary files containing fs info
		if(!chunk->is_folder())
		{
			handle	=chunk->doc->FindFirstFile("*.*", &data);
			if(handle != INVALID_HANDLE_VALUE)
			{
				//recurse into the file system
				sys2	=chunk->doc;
				chunk->set_folder();
			}
			chunk->doc->FindClose(handle);
		}

		chunk->read_levels	=parent->read_levels - 1;

		parent->append_child(chunk);

		//drop doc into parent filesys
		if(chunk->is_folder())
		{
			dst->CreateDirectory(chunk->name);
			if(!dst->SetCurrentDirectory(chunk->name))
			{
				return	chunk;	//fix this
			}
		}
		else
		{
			//copy the doc into the parent's sys
			DOCDESC	desc(chunk->name);

			IFileSystem	*newfs	=FS_Create(&desc, dst);
			FS_CopySys(newfs, chunk->doc);

			chunk->doc->Release();
			chunk->doc	=(IDocument *)newfs;
		}

		if(chunk->is_folder())
		{
			char	temp[_MAX_PATH];
			char	temp2[_MAX_PATH];

			memset(&data, 0, sizeof(data));

			if(sys2)
			{
				handle	=sys2->FindFirstFile("*.*", &data);
			}
			else
			{
				sprintf(temp, "%s\\*.*", fn);

				handle	=sys->FindFirstFile(temp, &data);
			}

			while(handle != INVALID_HANDLE_VALUE)
			{
				if(data.cFileName[0] != '.')
				{
					if(sys2)
					{
						InsertDroppedSys(sys2, data.cFileName, dst, chunk);
					}
					else
					{
						strncpy(temp2, temp, strlen(temp) - 3);
						temp2[strlen(temp) - 3]	=0;	//terminate
						strcat(temp2, data.cFileName);

						InsertDroppedSys(sys, temp2, dst, chunk);
					}
				}
				if(sys2)
				{
					if(!sys2->FindNextFile(handle, &data))
					{
						break;
					}
				}
				else
				{
					if(!sys->FindNextFile(handle, &data))
					{
						break;
					}
				}
			}
			if(sys2)
			{
				sys2->FindClose(handle);
			}
			else
			{
				sys->FindClose(handle);
			}
			dst->SetCurrentDirectory("..");
		}
		return	chunk;
	}

	sys->FindClose(handle);
	return	NULL;
}

//---------------------------------------------------------------------------

BOOL UTFView::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) 
{
	STGMEDIUM	StgMedium;
	TCHAR		szBuffer[_MAX_PATH];
	HTREEITEM	ItemOver;
	int			NumFiles, i;
	UTFDoc		*doc;
	Chunk		*chunk;

	ASSERT_VALID(this);

	ItemOver	=GetTreeCtrl().GetDropHilightItem();
	doc			=GetDocument();
	chunk		=0;

	// clean up focus rect
	OnDragLeave();

	if(ItemOver)
	{
		if(doc->request_modify())
		{
			HTREEITEM	s		=ItemOver;
			IFileSystem	*dst	=NULL;
			Chunk		*parent, *NewChunk;

			parent	=get_chunk(s);
			dst		=parent->doc;

			if(!dst)
			{
				dst	=doc->doc;
			}			

			if(pDataObject->GetData(CF_HDROP, &StgMedium, NULL) && StgMedium.tymed == TYMED_HGLOBAL)
			{
				//This stuff wasn't anywhere in the msdn
				NumFiles	=DragQueryFile((HDROP)StgMedium.hGlobal, -1, NULL, 0);

				if(NumFiles)
				{
					for(i=0;i < NumFiles;i++)
					{
						DragQueryFile((HDROP)StgMedium.hGlobal, i, szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
						NewChunk	=InsertDropped(szBuffer, dst, parent);

						//insert into UI
						doc->SetModifiedFlag(TRUE);
						insert_chunks(s, NewChunk);
					}
				}
			}
		}
	}
	doc->UpdateAllViews(NULL, 0, NULL);

	return	TRUE;
}

DROPEFFECT UTFView::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	ASSERT(m_prevDropEffect == DROPEFFECT_NONE);

	m_bDragDataAcceptable	=FALSE;

	if(!COleClientItem::CanCreateFromData(pDataObject))
	{
		return	DROPEFFECT_NONE;
	}
	GetObjectInfo(pDataObject, &m_dragSize, &m_dragOffset);

	CClientDC dc(NULL);

	dc.HIMETRICtoDP(&m_dragSize);
	dc.HIMETRICtoDP(&m_dragOffset);

	return	DROPEFFECT_COPY;
}

void	UTFView::OnDragLeave(void)
{
	if(m_prevDropEffect != DROPEFFECT_NONE)
	{
		CTreeView::GetTreeCtrl().SelectDropTarget(NULL);
		m_prevDropEffect = DROPEFFECT_NONE;
	}
}

DROPEFFECT	UTFView::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) 
{
	UINT			Flags;
	HTREEITEM		ItemOver;

	if(CTreeView::GetTreeCtrl())
	{
		ItemOver	=CTreeView::GetTreeCtrl().HitTest(point, &Flags);
		if(ItemOver)
		{
			Chunk	*chunk	=get_chunk(ItemOver);
			if(chunk)
			{
				while(!chunk->is_folder())
				{
					ItemOver	=CTreeView::GetTreeCtrl().GetParentItem(ItemOver);
					if(!ItemOver)
					{
						chunk	=NULL;
						break;
					}
					chunk	=get_chunk(ItemOver);
				}					
			}
			if(ItemOver)
			{
				CTreeView::GetTreeCtrl().SelectDropTarget(ItemOver);
			}
		}
	}

	m_prevDropEffect	=DROPEFFECT_COPY;
	return				DROPEFFECT_COPY;
}

//---------------------------------------------------------------------------
// DRAG N DROP
//---------------------------------------------------------------------------

BOOL IsChildNodeOf (CTreeCtrl *ctrl, HTREEITEM child, HTREEITEM suspected_parent)
{
	while (child)
	{
		if (child == suspected_parent)
			break;
		child = ctrl->GetParentItem(child);
	}
	return child == suspected_parent;
}

//---------------------------------------------------------------------------

BOOL UTFView::copy_item (HTREEITEM hitemDrag, HTREEITEM hitemDrop)
{
	CTreeCtrl *ctrl = &GetTreeCtrl();

    Chunk *chunk = get_chunk(hitemDrag);
	bool ok = EditCopy(chunk);
	if (ok)
	{
		ok = paste(hitemDrop);
	}
	return TRUE;
}

//---------------------------------------------------------------------------

void UTFView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CTreeView::OnLButtonUp(nFlags, point);

	CTreeCtrl *ctrl = &GetTreeCtrl();

	if (dragging)
	{
		image_list->DragLeave(this);
		image_list->EndDrag();

		if (drag_item != drop_item &&
			!IsChildNodeOf(ctrl, drop_item, drag_item) && 
			ctrl->GetParentItem(drag_item) != drop_item)
		{
			copy_item(drag_item, drop_item);
			ctrl->DeleteItem(drag_item);

			ctrl->SelectItem(drop_item);
		}
		else
		{
			ctrl->SelectItem(drag_item);
			MessageBeep(0);
		}

		ReleaseCapture();
		dragging = FALSE;
		ctrl->SelectDropTarget(NULL);
	}
}

//---------------------------------------------------------------------------

void UTFView::OnMouseMove(UINT nFlags, CPoint point) 
{
	HTREEITEM			hitem;
	UINT				flags;

	CTreeCtrl *ctrl = &GetTreeCtrl();

	if (dragging)
	{
		image_list->DragMove(point);
		if ((hitem = ctrl->HitTest(point, &flags)) != NULL)
		{
			if (hitem != drop_item)
			{
				image_list->DragLeave(this);
				drop_item = hitem;
				ctrl->SelectItem(drag_item);
				ctrl->SelectDropTarget(drop_item);
				image_list->DragEnter(this, point);
			}
		}
	}
	
	CTreeView::OnMouseMove(nFlags, point);
}

//---------------------------------------------------------------------------

void UTFView::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pHdr = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	ASSERT(!dragging);
#if VIEW_EDITABLE
	dragging = TRUE;
	drag_item = pHdr->itemNew.hItem;
	drop_item = NULL;

	SetCapture();
	image_list->BeginDrag(0,CPoint(-12,-4));
	image_list->DragEnter(this,pHdr->ptDrag);
#endif

	*pResult = 0;
}

//---------------------------------------------------------------------------

BOOL UTFView::PreTranslateMessage (MSG* msg) 
{
	if (msg->message == WM_KEYDOWN)
	{
		CWnd *focus = GetFocus();
		if (focus == this)
		{
			CWnd *parent = GetParent();

			switch (msg->wParam)
			{
				case 'C':
					if (GetKeyState(VK_CONTROL) < 0)
					{
						copy();
						return TRUE;
					}
				break;

				case 'V':
					if (GetKeyState(VK_CONTROL) < 0)
					{
						paste();
						return TRUE;
					}
				break;

				case 'X':
					if (GetKeyState(VK_CONTROL) < 0)
					{
						cut();
						return TRUE;
					}
				break;
			}
		}
	}
	
	return CTreeView::PreTranslateMessage(msg);
}


void UTFView::OnItemExpanding (NMHDR *pNMHDR, LRESULT *pResult) 
// pci - SPEEDY
{
	NM_TREEVIEW *msg = (NM_TREEVIEW *)pNMHDR;

	if (ready)
	if (msg->action == TVE_EXPAND)
	{
		UTFDoc *doc = GetDocument();

		CTreeCtrl *ctrl = &GetTreeCtrl();
		HTREEITEM s = ctrl->GetSelectedItem();
		s = msg->itemNew.hItem;
		if (s)
		{
			Chunk *chunk = get_chunk(s);
			if (chunk)
			{
				if (chunk->is_folder())
				{
					if (chunk->read_levels < 1) // not loaded?
					{
						doc->read_chunks(chunk->doc,chunk,1);

						// Note: strip PlaceHolder children

						HTREEITEM kid,next = ctrl->GetNextItem(s,TVGN_CHILD);
						while ((kid=next) != 0)
						{
							next = ctrl->GetNextItem(kid,TVGN_NEXT);
							Chunk *ch = get_chunk(kid);
							ASSERT(ch==0); // ie. *PlaceHolder*
							ctrl->DeleteItem(kid);
						}

						if (chunk->child)
						{
						ShowWindow(SW_HIDE);
							build(chunk->child,s);
						ShowWindow(SW_SHOW);
						}

						update_status(); // UPDATE SELECTED INFO
					}
				}
			}
		}
	}

	*pResult = 0;
}

int UTFView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//register drop target
	if(drop_target.Register(this))
	{
		return	0;
	}
	else
	{
		return	-1;
	}
}

//-----------------------------------------------------------------------------------------------------

void UTFView::OnToolsExportRaw() 
{
	CTreeCtrl *ctrl = &GetTreeCtrl();
	HTREEITEM s = ctrl->GetSelectedItem();
    if (s)
    {
		CString name = ctrl->GetItemText(s);
        Chunk *chunk = get_chunk(s);
		if(chunk)
		{
			if( chunk->is_folder() )
			{
				::CreateDirectory(name,0);
			}

			DOCDESC desc(name);
			desc.lpImplementation = "UTF";
			IFileSystem* file = FS_Create(&desc,0);

			if( file )
				chunk->export(*file);
		}
	}
}

void UTFView::OnToolsExportTga() 
{
}

void UTFView::OnToolsImportRaw() 
{
}

void UTFView::OnToolsImportTga() 
{
	CFileDialog dlg( true, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.tga", NULL );

	if( dlg.DoModal() == IDOK )
	{
		CString fn = dlg.GetPathName();

		TGA tga;
		if( tga.Load(fn) )
		{
			fn += ".tga";
			tga.Save(fn);
		}
	}
}
