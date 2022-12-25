// UTFDoc.cpp : implementation of the UTFDoc class
//

#include "stdafx.h"
#include "UTFApp.h"

#include "UTFDoc.h"
#include "CntrItem.h"
#include "SrvrItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "xfile.h"
#include <assert.h>


#define SPEEDY 1
// pci - is trying to speed things up

void FS_Flush (const char *name)
// useful for cleaning up sharing flags...
{
	DAFILEDESC desc = name;

	IFileSystem *file = 0;

	desc.dwDesiredAccess |= GENERIC_WRITE;
	desc.dwShareMode = 0;

	DACOM->CreateInstance(&desc, (void **)&file);
	DACOM->Release();

	file->Release();
	file = 0;
}

//---------------------------------------------------------------------------

struct BaseFile
{
	static IFileSystem *create (const char *name, IComponentFactory *parent=0)
	{
		DAFILEDESC desc(name);
		return FS_Create(&desc,parent);
	}

	static IFileSystem *open (const char *name, const char *mode=0, IComponentFactory *parent=0)
	{
		DAFILEDESC desc(name);
		return FS_Open(&desc,mode,parent);
	}
};

//---------------------------------------------------------------------------

struct DocFile
{
	static IDocument *create (const char *name, IComponentFactory *parent=0)
	{
		DOCDESC desc(name);
		return (IDocument *)FS_Create(&desc,parent);
	}

	static IDocument *open (const char *name, const char *mode, IComponentFactory *parent=0)
	{
		DOCDESC desc(name);
		desc.lpImplementation = "UTF";
		return (IDocument *)FS_Open(&desc,mode,parent);
	}

	static IDocument *cheat (IFileSystem *parent)
	{
		return (IDocument *)parent;
	}
};

//---------------------------------------------------------------------------

int FS_Copy (IFileSystem *dst, IFileSystem *src)
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

int WriteChunks (IFileSystem *sys, Chunk *chunk, int fill) 
{
    if (sys && chunk)
    {
        if (chunk->is_folder())
        {
            if (!fill)
            {
               	int ok = sys->CreateDirectory(chunk->name);
                if (!ok)
                {
                    CString msg;
                    msg.Format("Failed to create sub-directory '%s'",chunk->name);
                    MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
                    return FALSE;
                }
            }
            if (chunk->child)
            {
			    if (sys->SetCurrentDirectory(chunk->name))
			    {
					for (Chunk *c=chunk->child; c; c=c->next)
						WriteChunks(sys,c,fill);

        		    sys->SetCurrentDirectory("..");
                }
            }
        }
        else
        {
            IFileSystem *out;
			if (!fill)
			{
				out = BaseFile::create(chunk->name,sys);
			}
			else
			{
				out = BaseFile::open(chunk->name,"rw",sys);
			}
            if (out)
            {
                if (fill)
                {
                    out->SetFileTime(0,&chunk->ftCreationTime,&chunk->ftLastAccessTime,&chunk->ftLastWriteTime);
                    unsigned long bytes = 0;
					FS_Copy(out,chunk->doc);
//		            out->WriteFile(0, chunk->data, chunk->size, &bytes, 0);
                }
                out->Release();
            }
            else
            {
                CString msg;
                msg.Format("Failed to create file '%s'",chunk->name);
                MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
				return FALSE;
            }
        }
    }

	return TRUE;
}

//---------------------------------------------------------------------------

Chunk *CopyChunks (IFileSystem *dst, IFileSystem *src, Chunk *parent) 
{
	Chunk *root = parent;

if (src && dst)
{
	WIN32_FIND_DATA data;
	HANDLE handle;

	handle = src->FindFirstFile("*.*", &data);

    Chunk *prev = 0;

	if (handle != INVALID_HANDLE_VALUE) // is directory empty?
    do
    {
		const char *name = data.cFileName;

		// make sure this not a silly "." entry
		if (name[0] != '.' || strchr(name, '\\') != 0)
		{
		// the RIGHT way to tell if an entry is a directory 

			int is_folder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		// PREPARE SOURCE

			int ok = FALSE;

		// CREATE DESTINATION

			if (is_folder)
			{
               	ok = dst->CreateDirectory(name);
                if (!ok)
                {
                    CString msg;
                    msg.Format("Failed to create sub-directory '%s'",name);
                    MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
                    return FALSE;
                }
			}
			else
			{
				IFileSystem *out = BaseFile::create(name,dst);
				if (out)
				{
					IFileSystem *in = BaseFile::open(name,"r",src);
//					out->SetFileTime(0,&chunk->ftCreationTime,&chunk->ftLastAccessTime,&chunk->ftLastWriteTime);
					FS_Copy(out,in);
					out->Release();
					if (in) in->Release();
				}
				else
				{
					CString msg;
					msg.Format("Failed to create file '%s'",name);
					MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
					return FALSE;
				}
			}

		// CREATE CHUNK

            Chunk *chunk = new Chunk;
            ASSERT(chunk);
			chunk->init();
            chunk->open(data);

			if (root == 0)
				root = chunk;

            if (parent && parent->child == 0)
                parent->child = chunk;

            if (prev)
                prev->next = chunk;
            prev = chunk;

			IDocument *doc = DocFile::open(name,"r",dst);
			chunk->doc = doc;
			if (doc == 0)
				MessageBeep(0);

			if (is_folder)
			{
                chunk->set_folder();

				IFileSystem *d = BaseFile::open(name,"rw",dst);
				IFileSystem *s = BaseFile::open(name,"r",src);
				CopyChunks(d,s,chunk);
				if (s) s->Release();
				if (d) d->Release();
			}
        }
    }
    while (src->FindNextFile(handle, &data));

	src->FindClose(handle);
}
	return root;
}


//---------------------------------------------------------------------------

void ReadChunks (IFileSystem *sys, Chunk *parent, int levels=0) 
{
	assert(parent);

	if (parent==0 || levels < 1)
		return;

	if (levels > parent->read_levels)
	{
		parent->read_levels = levels;
	}

    if (sys)
    {
	WIN32_FIND_DATA data;
	HANDLE handle;

	handle = sys->FindFirstFile("*.*", &data);

    Chunk *prev = 0;

	if (handle != INVALID_HANDLE_VALUE) // is directory empty?
    do
    {
		// make sure this not a silly "." entry
		if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
		{
            Chunk *chunk = new Chunk;
            ASSERT(chunk);
			chunk->init();
            chunk->open(data);

            if (parent && parent->child == 0)
                parent->child = chunk;

            if (prev)
                prev->next = chunk;
            prev = chunk;

		// the RIGHT way to tell if an entry is a directory 

			int is_folder = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

			IDocument *doc = DocFile::open(data.cFileName,"r",sys);
			if (doc == 0)
			{
				doc = 0; // BREAK POINT?
			}
			chunk->doc = doc;

			if (is_folder)
			{
                chunk->set_folder();
// pci - SPEEDY
				if (levels > 1)
				{
					IFileSystem *f = BaseFile::open(data.cFileName,"r",sys);
					if (f)
					{
						ReadChunks(f,chunk,levels-1);
						f->Release();
					}
				}
			}
			else // data file
			{
#if 0
                File file(sys);
                if (file.open(data.cFileName))
                {
                    file.read(chunk->data,chunk->size);
                    file.close();
                }
#endif
			}
        }
    }
    while (sys->FindNextFile(handle, &data));

	sys->FindClose(handle);
    }
}


/////////////////////////////////////////////////////////////////////////////
// UTFDoc

IMPLEMENT_DYNCREATE(UTFDoc, COleServerDoc)

BEGIN_MESSAGE_MAP(UTFDoc, COleServerDoc)
	//{{AFX_MSG_MAP(UTFDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Enable default OLE container implementation
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, COleServerDoc::OnUpdatePasteMenu)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_LINK, COleServerDoc::OnUpdatePasteLinkMenu)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_CONVERT, COleServerDoc::OnUpdateObjectVerbMenu)
	ON_COMMAND(ID_OLE_EDIT_CONVERT, COleServerDoc::OnEditConvert)
	ON_UPDATE_COMMAND_UI(ID_OLE_EDIT_LINKS, COleServerDoc::OnUpdateEditLinksMenu)
	ON_COMMAND(ID_OLE_EDIT_LINKS, COleServerDoc::OnEditLinks)
	ON_UPDATE_COMMAND_UI_RANGE(ID_OLE_VERB_FIRST, ID_OLE_VERB_LAST, COleServerDoc::OnUpdateObjectVerbMenu)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// UTFDoc construction/destruction

/////////////////////////////////////////////////////////////////////////////
// UTFDoc construction/destruction

int UTFDoc::open_list (int count)
{
    list = NULL;
    return TRUE;
}

void UTFDoc::free_chunks (Chunk *chunk)
{
	Chunk *next;
	for (; chunk; chunk=next)
	{
		next = chunk->next;

	// FREE DATA

		chunk->close();

	// FREE CHILDREN

		if (chunk->child)
		{
			free_chunks(chunk->child);
		}

	// ADD TO FREE LIST

		if (list == chunk)
			list = chunk->next;

		delete chunk;
	}
}

void UTFDoc::close_list (void)
{
// FUTURE: some chunks may be allocated SEPARATELY from the main list

    free_chunks(list);

    list = 0;
}

/////////////////////////////////////////////////////////////////////////////

UTFDoc::UTFDoc()
{
    file = 0;
	doc = 0;

    list = 0;

    view_chunk = NULL;
    num_views = 0;

	is_closed = TRUE;
}

UTFDoc::~UTFDoc()
{
    close_list();

	if (doc)
	{
		doc->Release();
		doc = 0;

		FS_Delete(tempname);
	}
    if (file)
    {
        file->Release();
        file = 0;
    }
}


BOOL UTFDoc::OnNewDocument()
{
	if (!COleServerDoc::OnNewDocument())
		return FALSE;

#if VIEW_EDITABLE
	char	PathTemp[_MAX_PATH];

	is_closed = FALSE;

	filename	=GetTitle();
	filename	+=".utf";
	file		=DocFile::open(filename,"rws");

	file->GetFileName(PathTemp, sizeof(PathTemp));
	filename	=PathTemp;

    if(file)
    {
        modified_time.dwLowDateTime = 0;    // make sure REFRESH occurs
        modified_time.dwHighDateTime = 0;

        refresh();
    }

	return TRUE;
#else
	return	FALSE;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// UTFDoc server implementation

COleServerItem* UTFDoc::OnGetEmbeddedItem()
{
	// OnGetEmbeddedItem is called by the framework to get the COleServerItem
	//  that is associated with the document.  It is only called when necessary.

	CUTFAppSrvrItem* pItem = new CUTFAppSrvrItem(this);
	ASSERT_VALID(pItem);
	return pItem;
}



/////////////////////////////////////////////////////////////////////////////
// UTFDoc serialization

void UTFDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}

	// Calling the base class COleServerDoc enables serialization
	//  of the container document's COleClientItem objects.
	COleServerDoc::Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////
// UTFDoc diagnostics

#ifdef _DEBUG
void UTFDoc::AssertValid() const
{
	COleServerDoc::AssertValid();
}

void UTFDoc::Dump(CDumpContext& dc) const
{
	COleServerDoc::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// UTFDoc commands
/////////////////////////////////////////////////////////////////////////////
// UTFDoc commands

void UTFDoc::write_chunks (IFileSystem *sys, Chunk *chunk, int fill) 
{
	ASSERT(1);
/*
    if (sys)
    while (chunk)
    {
        if (chunk->is_folder())
        {
            if (!fill)
            {
               	int ok = sys->CreateDirectory(chunk->name);
                if (!ok)
                {
                    CString msg;
                    msg.Format("Failed to create sub-directory '%s'",chunk->name);
                    MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
                    return;
                }
            }
            if (chunk->child)
            {
			    if (sys->SetCurrentDirectory(chunk->name))
			    {
                    write_chunks(sys,chunk->child,fill);
        		    sys->SetCurrentDirectory("..");
                }
            }
        }
        else
        {
            IFileSystem *out;
			out = BaseFile::create(chunk->name,sys);
            if (out)
            {
                if (fill)
                {
                    out->SetFileTime(0,&chunk->ftCreationTime,&chunk->ftLastAccessTime,&chunk->ftLastWriteTime);
                    unsigned long bytes = 0;
		            out->WriteFile(0, chunk->data, chunk->size, &bytes, 0);
                }
                out->Release();
            }
            else
            {
                CString msg;
                msg.Format("Failed to create file '%s'",chunk->name);
                MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
                return;
            }
        }

        chunk = chunk->next;
    }
*/
}

//---------------------------------------------------------------------------

int UTFDoc::out_of_date (FILETIME *mod)
{
//	if (mod == 0)
//		mod = &modified_tiem;
//
    int old = FALSE;
    if (file)
    {
        FILETIME latest;
        file->GetFileTime(0,NULL,NULL,&latest);
        // is disk version newer than memory?
        if (CompareFileTime(&latest,mod) > 0)
            old = TRUE;
    }
    return (old);
}

//---------------------------------------------------------------------------

void UTFDoc::refresh (void)
{
	char	PathTemp[_MAX_PATH];
	char	*FileTemp;

    if (file)
    {
        FILETIME latest;
        file->GetFileTime(0,NULL,NULL,&latest);

        close_list();

		//fix new files... I despise cstring
		if(!strrchr((LPCSTR)filename, '\\'))
		{
			GetFullPathName((LPCTSTR)filename, sizeof(PathTemp), PathTemp, &FileTemp);

			filename	=PathTemp;
		}

#if VIEW_EDITABLE
if (FS_Copy(filename,tempname,0))
{
	doc = DocFile::open(tempname,"rws");
}
#else
	doc = DocFile::cheat(file);
#endif

        init_chunks(doc);
        modified_time = latest;

        SetModifiedFlag(FALSE);
    }
}

//---------------------------------------------------------------------------

int UTFDoc::request_modify (void)
{
    int ok = FALSE;

    if (file && (file->GetAccessType() & GENERIC_WRITE))
    {
        ok = TRUE;
    }
    else
    {
#if VIEW_EDITABLE
		IFileSystem *f;
		f = BaseFile::open(filename,"rw");

        if (f && file)
        {
            file->Release();
            file = f;

			if (FS_Copy(filename,tempname,0))
			{
				doc = DocFile::open(tempname,"rws");
			}

            ok = (doc != 0);
        }
#endif
    }

#if VIEW_EDITABLE
    if (!ok)
        MessageBox(0,"Sharing Violation: cannot modify file.\nnote: file is already open or read-only","Warning!",MB_OK|MB_ICONEXCLAMATION);
#endif

    return (ok);
}

//---------------------------------------------------------------------------

BOOL UTFDoc::OnSaveDocument (LPCTSTR full_name) 
{
	IFileSystem *f = 0;

	int ok = 0;

	int same = (filename == full_name);

	if (!doc && same)
	{
		ok = 1;	// no changes!
	}
	else
	{
		if (doc) // is modified?
		{
			if (same)
			{
				file->Release();
				file = 0;
			}
			ok = FS_Copy(tempname,CString(full_name));
			if (ok)
			{
				FS_Flush(full_name);

				close_list();
				doc->Release();
				doc = 0;
				FS_Delete(tempname);
			}
		}
		else
		{
			ok = FS_Copy(filename,CString(full_name));
		}

		f = BaseFile::open(full_name,"r");
	}
    if (f)
    {
		filename = full_name;

		if (f != file)
		{
			if (file)
				file->Release();
			file = f;

			modified_time.dwLowDateTime = 0;    // make sure REFRESH occurs
			modified_time.dwHighDateTime = 0;
			refresh();
		}

        SetModifiedFlag(FALSE);
    }

	if (!ok)
    {
        CString msg;
        msg.Format("Cannot write to file. (sharing?)\n%s",full_name);
        MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
    }

	return	ok;
}

//---------------------------------------------------------------------------

Chunk *UTFDoc::new_chunk (void)
{
    Chunk *chunk = new Chunk;
    if (chunk)
    {
        chunk->init();

        if (list == 0)
            list = chunk;
    }
    return (chunk);
}

void UTFDoc::init_chunks (IFileSystem *fs) 
{
    free_chunks(list);
	assert(list == 0);

	list = new Chunk;
	list->set_root("*ROOT*");	// one wrapper for all sub-chunks
#if VIEW_EDITABLE
	ReadChunks(fs,list,100);
#else
	ReadChunks(fs,list,1);
#endif
}

void UTFDoc::read_chunks (IFileSystem *fs, Chunk *parent, int levels) 
{
	ReadChunks(fs,parent,levels);
}

//---------------------------------------------------------------------------

BOOL UTFDoc::OnOpenDocument(LPCTSTR full_name) 
{
	is_closed = FALSE;

#if 0
	FS_Flush(full_name); // useful for cleaning up sharing flags...
#endif

//	if (!CDocument::OnOpenDocument(full_name))
//		return FALSE;
	
    filename = full_name;

#if VIEW_EDITABLE
	file = BaseFile::open(filename,"r");
#else
	file = DocFile::open(filename,"rs");
#endif

    if (file)
    {
        modified_time.dwLowDateTime = 0;    // make sure REFRESH occurs
        modified_time.dwHighDateTime = 0;

        refresh();
    }
    else
    {
        CString msg;
        msg.Format("Failed to open file. (sharing?)\n%s",filename);
        MessageBox(0,msg,"Warning:",MB_OK|MB_ICONEXCLAMATION);
    }

	return (file != 0);
}

//---------------------------------------------------------------------------

int UTFDoc::rename (Chunk *chunk, const char *new_name)
{
    if (!chunk->is_named(new_name))
    {
        if (request_modify()) // SetModifiedFlag(TRUE);
        {
            if (chunk->rename(new_name))
			{
	            SetModifiedFlag(TRUE);
				return 1;
			}
        }
    }
    return 0;
}

//---------------------------------------------------------------------------

int UTFDoc::disconnect (Chunk *&list_ptr, Chunk *chunk)
{
    if (list_ptr == chunk)
    {
        list_ptr = chunk->next;
        chunk->next = 0;
        return 1;
    }

    Chunk *prev;
    Chunk *i;
    for (prev=0,i=list_ptr; i; prev=i,i=i->next)
    {
        if (i == chunk)
        {
        // DISCONNECT

            if (prev)
                prev->next = i->next;
            else
                list = i->next;
            chunk->next = 0;
            return 1;
        }
        else if (i->child)
        {
            if (disconnect(i->child,chunk))
                return 1;
        }
    }
    return 0;
}

//---------------------------------------------------------------------------

int UTFDoc::remove_chunk (Chunk *chunk)
{
	if (chunk->doc == 0)	// make file errors more obvious?
		return FALSE;
/*
    // CLOSE ANY VIEWS

    CView *view = is_viewing(chunk);
    if (view)
    {
   		CFrameWnd* frame = view->GetParentFrame();
		ASSERT_VALID(frame);
		// and close it
		PreCloseFrame(frame);
		frame->DestroyWindow();
    }
*/
	// REMOVE CHILDREN

    int ok = FALSE;
    if (request_modify())   // SetModifiedFlag(TRUE);
    {
		ok = TRUE;

		if (chunk->is_folder())
		{
		    Chunk *next;
			Chunk *p = chunk->child;
			for (p=chunk->child; p; p=next)
			{
				next = p->next;
				ok = remove_chunk(p);
				if (!ok)
				{
					char msg[128];
					sprintf(msg,"Warning: could not DELETE '%s' chunk'?",p->name);
                    MessageBox(0,msg,"Warning!",MB_OK|MB_ICONEXCLAMATION);
					break;
				}
			}
		}

		if (ok)
		{
			// DELETE FILE

			ok = chunk->delete_doc_file();

			if (ok)
			{
		        SetModifiedFlag(TRUE);

				// FREE DATA & ADD NODES TO FREE LIST

				ok = disconnect(list,chunk);

				free_chunks(chunk);
			}
		}
    }

    return ok;
}

//---------------------------------------------------------------------------

Chunk *UTFDoc::insert_chunk (const char *name, int size, Chunk *parent)
{
	Chunk *chunk = 0;

	int ok = 0;

    if (request_modify())
    {
		IFileSystem *sys = 0;
		if (parent)
			sys = parent->doc;
		if (sys == 0)
			sys = doc;

		if (size == -1) // folder?
		{
			ok = sys->CreateDirectory(name);
		}
		else // file
		{
            IFileSystem *f = BaseFile::create(name,sys);
			if (f == 0)
			{
				int err = sys->GetLastError();
			}
			else
			{
				ok = 1;

				if (size)
				{
					char *bfr = (char *)::malloc(size);
					if (bfr)
					{
						memset(bfr,0,size);
						U32 bytes;
						f->WriteFile(0,bfr,size,&bytes,0);
						::free(bfr);
					}
				}
				f->Release();
			}
		}

		if (ok)
		{
	        SetModifiedFlag(TRUE);

			WIN32_FIND_DATA data;
			HANDLE handle;

			handle = sys->FindFirstFile(name, &data);
			if (handle != INVALID_HANDLE_VALUE) // is directory empty?
			{
				chunk = new_chunk();
				ASSERT(chunk);
				chunk->open(data);
				IDocument *doc = DocFile::open(data.cFileName,"r",sys);
				chunk->doc = doc;

				if (parent)
				{
					Chunk *prev = parent->child;
					if (prev == 0)
					{
						parent->child = chunk;
					}
					else // add last child
					{
						while (prev->next)
						{
							prev = prev->next;
						}
						prev->next = chunk;
					}
				}
				else
				{
					// ROOT
				}

CString out;
out.Format("new_chunk(%s) = %X",data.cFileName,chunk);
OutputDebugString(out);
			}
		}
    }

    return chunk;
}

//---------------------------------------------------------------------------

void UTFDoc::OnCloseDocument() 
{
	is_closed = TRUE;

	close_list();

	if (doc == file)
	{
		doc = 0;
	}
	if (doc)
	{
		doc->Release();
		doc = 0;
		FS_Delete(tempname,0);
	}

	COleServerDoc::OnCloseDocument();
}

//---------------------------------------------------------------------------

//#include "HexView.h"

CView *UTFDoc::is_viewing (void *chunk)
{
/*
	POSITION pos = GetFirstViewPosition();
    while (pos)
    {
	    CView *v = GetNextView(pos);
	    if (v && v->IsKindOf(RUNTIME_CLASS(HexView)) )
	    {
            if (((HexView *)v)->is_viewing(chunk))
                return v;
	    }
    }
*/
    return 0;
}

//---------------------------------------------------------------------------

int UTFDoc::find_path (Chunk **path, int &index, Chunk *search, Chunk *ptr)
{
    if (ptr == 0)
        return FALSE;

    while (ptr)
    {
        path[index++] = ptr;

        if (ptr == search)
            return TRUE;

        if (ptr->child)
        {
            if (find_path(path,index,search,ptr->child))
            {
                return TRUE;
            }
        }

        index--;

        ptr = ptr->next;
    }
    return FALSE;
}

void UTFDoc::build_path (CString &name, Chunk *chunk)
{
    name.Empty();

    Chunk *stack[128];
    int index = 0;

    if (find_path(stack,index,chunk,list))
    {
        for (int i=0; i<index; i++)
        {
            if (i != 0)
                name += '\\';
            name += stack[i]->name;
        }
    }
}

Chunk *UTFDoc::find_chunk (const char *path_name)
{
    Chunk *chunk = list;

    char name[128];

    const char *next = path_name;
    const char *ptr;
    while (chunk && next)
    {
        ptr = next;
        next = strchr(ptr,'\\');
        if (next)
        {
            int size = next-ptr;
            memcpy(name,ptr,size);
            name[size] = 0;
            next++;
        }
        else
            strcpy(name,ptr);

        while (chunk)
        {
            if (chunk->is_named(name))
                break;
            chunk = chunk->next;
        }

        if (next && chunk)
            chunk = chunk->child;
    }
    return chunk;
}

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

