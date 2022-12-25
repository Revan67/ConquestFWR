// Editor.cpp : Defines the class behaviors for the application.
//

// TODO: code loading of campaigns
// TODO: define the "Start Mode" better
// TODO: get the sidebars to behave well
// TODO: sidebars should save & load positions
// TODO: show a list of systems in Scenario mode
// TODO: give a toggle to show system names in Sector mode

#include "stdafx.h"
#include "globals.h"

#include "Editor.h"
#include "MainFrm.h"
#include "EditorDoc.h"
#include "EditorView.h"

#include <EventSys.h>

#include "Campaign.h"
#include "Scenario.h"
#include "SystemStructs.h"
#include "tinyxml\tinyxml.h"
#include "SaveLoad.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Editor Globals

namespace Editor
{
	U8 playerID = 1;
	CRect testRect1;
	CRect testRect2;
	float deltaTime;
	SPLASH g_Splash;

	System* GetActiveSystem()
	{
		if( CAMPAIGN && CAMPAIGN->GetCurrentScenario() && CAMPAIGN->GetCurrentScenario()->GetActiveSector() )
		{
			return CAMPAIGN->GetCurrentScenario()->GetActiveSector()->GetActiveSystem();
		}
		return NULL;
	}

	System* GetSystem( U32 _sectorID, U32 _systemID )
	{
		if( CAMPAIGN && CAMPAIGN->GetCurrentScenario() && CAMPAIGN->GetCurrentScenario()->GetActiveSector() )
		{
			return CAMPAIGN->GetCurrentScenario()->GetActiveSector()->FindSystemByIdx( _systemID );
		}
		return NULL;
	}

	IObject* GetObjectByHandle( const char* _scriptHandle )
	{
		if( CAMPAIGN )
		{
			IScenario** sList = (IScenario**)_alloca( CAMPAIGN->GetNumScenarios() * sizeof(IScenario*) );
			if( CAMPAIGN->GetScenarioList( sList, CAMPAIGN->GetNumScenarios() ) )
			{
				for( unsigned i = 0; i < CAMPAIGN->GetNumScenarios(); i++ )
				{
					ISector* sector = sList[i]->GetActiveSector();
					if( sector )
					{
						for( int systemID = 0; systemID <= MAX_SYSTEMS; systemID++ )
						{
							System* system = sector->FindSystemByIdx(systemID);
							if( system )
							{
								IObject* obj = system->objectList.FindByHandle( _scriptHandle );
								if( obj )
								{
									return obj;
								}
							}
						}
					}
				}
			}
		}
		return NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CEditorApp

BEGIN_MESSAGE_MAP(CEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditorApp construction

CEditorApp::CEditorApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CEditorApp object

CEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CEditorApp initialization

BOOL CEditorApp::InitInstance()
{
	ZeroMemory( &CQFLAGS, sizeof(CQFLAGS) );

	AfxInitRichEdit();
	AfxEnableControlContainer();

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("Conquest 2 : Editor"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register document templates

	char szpath[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH,szpath);
	path = szpath;

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CEditorDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CEditorView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

//	// verify that the list elements are XML docs
//	int numFiles = m_pRecentFileList->GetSize() - 1;
//	for( int i = 0; i < numFiles; i++ )
//	{
//		TiXmlDocument doc;
//		if( !doc.LoadFile(m_pRecentFileList->m_arrNames[i]) )
//		{
//			m_pRecentFileList->Remove(i);
//			m_pRecentFileList->m_nSize--;
//			numFiles = m_pRecentFileList->GetSize() - 1;
//			i = 0;
//		}
//	}

	return TRUE;
}

int CEditorApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

CDocument* CEditorApp::OpenDocumentFile(LPCTSTR lpszFileName) 
{
	TiXmlDocument doc;
	if( !doc.LoadFile(lpszFileName) )
	{
		return NULL;
	}

	if( doc.FirstChildElement("CAMPAIGN") )
	{
		CString dir = lpszFileName;
		Editor::PushFileDirectory(dir);

		COMPTR<ISaverLoader> loader;
		if( CAMPAIGN->QueryInterface("ISaverLoader", loader) == GR_OK )
		{
			if( loader->Load(doc) )
			{
				return ((CFrameWnd*)m_pMainWnd)->GetActiveDocument();
			}
		}
	}

	if( doc.FirstChildElement("SCENARIO") )
	{
		IScenario* s = Scenario::New();

		COMPTR<ISaverLoader> loader;
		if( s->QueryInterface("ISaverLoader", loader) == GR_OK )
		{
			if( loader->Load(doc) )
			{
				CAMPAIGN->AddScenario(s);
				if( !CAMPAIGN->GetCurrentScenario() )
				{
					CAMPAIGN->SetCurrentScenario(s);
				}
				return ((CFrameWnd*)m_pMainWnd)->GetActiveDocument();
			}
		}
		
		Scenario::Delete(s);
	}

	return NULL;
}

BOOL CEditorApp::OnIdle(LONG lCount) 
{
	// see MainLoop.cpp
	void MainLoop();
	MainLoop();
	
	return CWinApp::OnIdle(lCount);
}

BOOL CEditorApp::ProcessMessageFilter(int code, LPMSG lpMsg) 
{
	if( EVENTSYS )
	{
		EVENTSYS->Send( lpMsg->message, lpMsg );
	}
	return CWinApp::ProcessMessageFilter(code, lpMsg);
}

//-----------------------------------------------------------------------------------------------------

#include "cderr.h" //for definition of FNERR_BUFFERTOOSMALL

namespace Editor
{
	bool GetOpenFileList( CArray<CString,CString>& _array )
	{
		CFileDialog dlg( TRUE, NULL, NULL, OFN_ALLOWMULTISELECT, NULL, NULL );

		DWORD MAXFILE = 2562; //2562 is the max
		dlg.m_ofn.nMaxFile = MAXFILE;
		char* pc = new char[MAXFILE];
		dlg.m_ofn.lpstrFile = pc;
		dlg.m_ofn.lpstrFile[0] = NULL;

		int iReturn = dlg.DoModal();
		if(iReturn ==  IDOK)
		{
			POSITION pos = dlg.GetStartPosition();
			while (pos != NULL)
			{
				_array.Add( dlg.GetNextPathName(pos) );
			}
		}
		else if(iReturn == IDCANCEL)
		{
			AfxMessageBox("Cancel");
			return false;
		}

		if(CommDlgExtendedError() == FNERR_BUFFERTOOSMALL)
		{
			AfxMessageBox("BUFFERTOOSMALL");
		}

		delete []pc;
		return true;
	}

	bool GetSaveFile( CString& _copyFilename )
	{
		CFileDialog dlg( false, NULL, NULL, OFN_EXPLORER, NULL, NULL );

		dlg.m_ofn.nMaxFile = MAX_PATH;
		char* pc = new char[MAX_PATH];
		dlg.m_ofn.lpstrFile = pc;
		dlg.m_ofn.lpstrFile[0] = NULL;

		int iReturn = dlg.DoModal();
		if(iReturn == IDOK)
		{
			POSITION pos = dlg.GetStartPosition();
			if(pos != NULL)
			{
				_copyFilename = dlg.GetNextPathName(pos);
			}
		}
		else if(iReturn == IDCANCEL)
		{
			AfxMessageBox("Cancel");
			return false;
		}

		if(CommDlgExtendedError() == FNERR_BUFFERTOOSMALL)
		{
			AfxMessageBox("BUFFERTOOSMALL");
		}

		delete []pc;
		return true;
	}

	// LoadBMPImage	- Loads a BMP file and creates a bitmap GDI object
	//		  also creates logical palette for it.
	// Returns	- TRUE for success
	// sBMPFile	- Full path of the BMP file
	// bitmap	- The bitmap object to initialize
	// pPal		- Will hold the logical palette. Can be NULL
	BOOL LoadBMPImage( LPCTSTR sBMPFile, CBitmap& bitmap, CPalette *pPal )
	{
		CFile file;
		if( !file.Open( sBMPFile, CFile::modeRead) )
			return FALSE;

		BITMAPFILEHEADER bmfHeader;

		// Read file header
		if (file.Read((LPSTR)&bmfHeader, sizeof(bmfHeader)) != sizeof(bmfHeader))
			return FALSE;

		// File type should be 'BM'
		if (bmfHeader.bfType != ((WORD) ('M' << 8) | 'B'))
			return FALSE;

		// Get length of the remainder of the file and allocate memory
		DWORD nPackedDIBLen = file.GetLength() - sizeof(BITMAPFILEHEADER);
		HGLOBAL hDIB = ::GlobalAlloc(GMEM_FIXED, nPackedDIBLen);
		if (hDIB == 0)
			return FALSE;

		// Read the remainder of the bitmap file.
		if (file.Read((LPSTR)hDIB, nPackedDIBLen) != nPackedDIBLen )
		{
			::GlobalFree(hDIB);
			return FALSE;
		}


		BITMAPINFOHEADER &bmiHeader = *(LPBITMAPINFOHEADER)hDIB ;
		BITMAPINFO &bmInfo = *(LPBITMAPINFO)hDIB ;

		// If bmiHeader.biClrUsed is zero we have to infer the number
		// of colors from the number of bits used to specify it.
		int nColors = bmiHeader.biClrUsed ? bmiHeader.biClrUsed : 
		1 << bmiHeader.biBitCount;

		LPVOID lpDIBBits;
		if( bmInfo.bmiHeader.biBitCount > 8 )
			lpDIBBits = (LPVOID)((LPDWORD)(bmInfo.bmiColors + bmInfo.bmiHeader.biClrUsed) + 
			((bmInfo.bmiHeader.biCompression == BI_BITFIELDS) ? 3 : 0));
		else
			lpDIBBits = (LPVOID)(bmInfo.bmiColors + nColors);

		// Create the logical palette
		if( pPal != NULL )
		{
			// Create the palette
			if( nColors <= 256 )
			{
				UINT nSize = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * nColors);
				LOGPALETTE *pLP = (LOGPALETTE *) new BYTE[nSize];

				pLP->palVersion = 0x300;
				pLP->palNumEntries = nColors;

				for( int i=0; i < nColors; i++)
				{
					pLP->palPalEntry[i].peRed = bmInfo.bmiColors[i].rgbRed;
					pLP->palPalEntry[i].peGreen = bmInfo.bmiColors[i].rgbGreen;
					pLP->palPalEntry[i].peBlue = bmInfo.bmiColors[i].rgbBlue;
					pLP->palPalEntry[i].peFlags = 0;
				}

				pPal->CreatePalette( pLP );

				delete[] pLP;
			}
		}

		CClientDC dc(NULL);
		CPalette* pOldPalette = NULL;
		if( pPal )
		{
			pOldPalette = dc.SelectPalette( pPal, FALSE );
			dc.RealizePalette();
		}

		HBITMAP hBmp = CreateDIBitmap( dc.m_hDC,		// handle to device context 
			&bmiHeader,	// pointer to bitmap size and format data 
			CBM_INIT,	// initialization flag 
			lpDIBBits,	// pointer to initialization data 
			&bmInfo,	// pointer to bitmap color-format data 
			DIB_RGB_COLORS);		// color-data usage 
		bitmap.Attach( hBmp );

		if( pOldPalette )
			dc.SelectPalette( pOldPalette, FALSE );

		::GlobalFree(hDIB);
		return TRUE;
	}

	bool PushFileDirectory( CString& _string )
	{
		if( ::GetFileAttributes(_string) == 0xffffffff )
		{
			if( ::SetCurrentDirectory( _string ) )
			{
				return true;
			}
		}

		int slash = _string.ReverseFind('\\');
		if( slash == -1 )
		{
			return false;
		}
		_string.SetAt(slash,'\0');

		if( ::SetCurrentDirectory( _string ) )
		{
			return true;
		}

		return false;
	}
};

