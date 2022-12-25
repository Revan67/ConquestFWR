// UTFApp.h : main header file for the UTFAPP2 application
//

#if !defined(AFX_UTFAPP_H__4814D386_2960_11D3_9B98_0050049E94BC__INCLUDED_)
#define AFX_UTFAPP_H__4814D386_2960_11D3_9B98_0050049E94BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// UTFApp:
// See UTFApp.cpp for the implementation of this class
//

struct IDAComponent;
struct ICOManager;
typedef unsigned char byte;
#include "filesys.h"
//typedef IFileSystem *FileHandle;

IFileSystem *FileSys_Open (const char *filename, IComponentFactory *parent=0);
IFileSystem *FileSys_OpenWrite (const char *filename, IComponentFactory *parent=0);
IFileSystem *FileSys_Create (const char *filename, IComponentFactory *parent=0);

extern ICOManager *DACOM;

class UTFApp : public CWinApp
{
public:

//	FileHandle open_file (const char *name, IDAComponent *parent=0);

	CMultiDocTemplate* list_template;	// UTFView
	CMultiDocTemplate* data_template;	// HexView

public:

	UTFApp();
	~UTFApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UTFApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	COleTemplateServer m_server;
		// Server object for document creation
    int is_viewing (void *chunk) const;

	CFrameWnd *new_data_view (CDocument *doc)
	{
		CFrameWnd* new_frame = data_template->CreateNewFrame(doc, NULL);
		if (new_frame)
			data_template->InitialUpdateFrame(new_frame, doc);
		return (new_frame);
	}

	//{{AFX_MSG(UTFApp)
	afx_msg void OnAppAbout ();
	afx_msg void OnParseTypes ();
	afx_msg void OnTypeDefine ();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnFileParseprocessedfile();
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UTFAPP_H__4814D386_2960_11D3_9B98_0050049E94BC__INCLUDED_)
