// UTFDoc.h : interface of the UTFDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_UTFDOC_H__4814D38E_2960_11D3_9B98_0050049E94BC__INCLUDED_)
#define AFX_UTFDOC_H__4814D38E_2960_11D3_9B98_0050049E94BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "chunk.h"
struct IDocument;


#ifndef VIEW_EDITABLE
#define VIEW_EDITABLE	0
#endif


extern Chunk *ReadChunks (IFileSystem *sys, Chunk *parent=0);
extern int WriteChunks (IFileSystem *sys, Chunk *chunk, int fill);
extern Chunk *CopyChunks (IFileSystem *dst, IFileSystem *src, Chunk *parent=0);

//---------------------------------------------------------------------------
// UTFDoc
//---------------------------------------------------------------------------

class CUTFAppSrvrItem;

class UTFDoc : public COleServerDoc
{
protected: // create from serialization only
	UTFDoc();
	DECLARE_DYNCREATE(UTFDoc)

// Attributes
public:
	CUTFAppSrvrItem* GetEmbeddedItem()
		{ return (CUTFAppSrvrItem*)COleServerDoc::GetEmbeddedItem(); }

    CString filename;
	CString tempname;

    IFileSystem *file;		// keep file open to prevent sharing!
	IDocument *doc;			// COPY of file used for modifications

    FILETIME modified_time;

    Chunk *list;

    Chunk *view_chunk;  // temp for HexView::OnInitialUpdate()
    int num_views;

	int is_closed;

// Operations
public:

    Chunk *new_chunk (void);

    void free_chunks (Chunk *chunk);

    int request_modify (void);

    int out_of_date (FILETIME *mod);

    int rename (Chunk *chunk, const char *new_name);

    Chunk *get_root (void)
    {
        return list;
    }

    CView *UTFDoc::is_viewing (void *chunk);

	void close (void)	// called by main view
	{
		if (!is_closed)
			OnCloseDocument();
	}

    void open_data_view (Chunk *chunk)
    {
        view_chunk = chunk;
        ((UTFApp *)AfxGetApp())->new_data_view(this);
        num_views++;
    }

    int get_view_index (void) const
    {
        return num_views;
    }
    void close_data_view (Chunk *original, Chunk *edit)
    {
        if (edit)
        {
        }
        num_views--;
    }

    int open_list (int count);
    void close_list (void);

    void write_chunks (IFileSystem *sys, Chunk *chunk, int fill);
    void read_chunks (IFileSystem *sys, Chunk *parent, int levels=1);
    void init_chunks (IFileSystem *sys);

    int find_path (Chunk **path, int &index, Chunk *search, Chunk *ptr);
    void build_path (CString &name, Chunk *chunk);
    Chunk *find_chunk (const char *path);

    void refresh (void);

    int disconnect (Chunk *&list_ptr, Chunk *chunk);

    int remove_chunk (Chunk *chunk);
	Chunk *insert_chunk (const char *name, int size, Chunk *parent=0);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UTFDoc)
	protected:
	virtual COleServerItem* OnGetEmbeddedItem();
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~UTFDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(UTFDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UTFDOC_H__4814D38E_2960_11D3_9B98_0050049E94BC__INCLUDED_)
