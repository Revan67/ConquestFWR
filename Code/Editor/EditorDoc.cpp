// EditorDoc.cpp : implementation of the CEditorDoc class
//

#include "stdafx.h"
#include "globals.h"

#include "EditorDoc.h"
#include "Editor.h"

#include "Campaign.h"
#include "tinyxml\tinyxml.h"
#include "SaveLoad.h"
#include <TSmartPointer.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditorDoc

IMPLEMENT_DYNCREATE(CEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CEditorDoc, CDocument)
	//{{AFX_MSG_MAP(CEditorDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditorDoc construction/destruction

CEditorDoc::CEditorDoc()
{
}

CEditorDoc::~CEditorDoc()
{
}

BOOL CEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CEditorDoc serialization

void CEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// save this campaign
		if(CAMPAIGN)
		{
			COMPTR<ISaverLoader> saver;
			CAMPAIGN->QueryInterface( "ISaverLoader", (void**)saver );
			if( saver )
			{
				CString fn = ar.GetFile()->GetFileName();

				while( fn.FindOneOf(".") != -1 )
				{
					int i = fn.FindOneOf(".");
					fn.SetAt( i, 0 );
				}
				fn += ".campaign";

				TiXmlDocument doc( fn );
				doc.InsertEndChild( TiXmlDeclaration("1.0","","yes") );
				doc.SetUserData( &ar );
				saver->Save(doc);
				doc.SaveFile();

				extern CEditorApp theApp;
				theApp.AddToRecentFileList(fn);
			}
		}
	}
	else
	{
		int t = 5;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CEditorDoc diagnostics

#ifdef _DEBUG
void CEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditorDoc commands
