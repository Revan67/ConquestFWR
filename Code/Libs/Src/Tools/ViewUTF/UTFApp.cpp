// UTFApp.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "UTFApp.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "IpFrame.h"
#include "UTFDoc.h"
#include "UTFView.h"
#include "..\..\Common\ProcessHeaders.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DACOM.h"
#include "FileSys.h"
#include "typelist.h"

ICOManager *DACOM = 0;

/////////////////////////////////////////////////////////////////////////////
// UTFApp

BEGIN_MESSAGE_MAP(UTFApp, CWinApp)
	//{{AFX_MSG_MAP(UTFApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_PARSE, OnParseTypes)
	ON_COMMAND(ID_TYPE_DEFINE, OnTypeDefine)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_FILE_PARSEPROCESSEDFILE, OnFileParseprocessedfile)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// UTFApp construction

UTFApp::UTFApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	list_template = NULL;
	data_template = NULL;
}

UTFApp::~UTFApp()
{
	if (DACOM)
	{
		DACOM->ShutDown();
		DACOM->Release();
		DACOM = 0;
	}
}

static char *GetImplementation (char *filename)
{
	char * result;

	if ((result = strrchr(filename, '.')) != 0)
	{
		if (strchr(result, '\\'))
			result = 0;
		else 
		{
			strupr(result++);
		}
	}

	return result;
}

IFileSystem *FileSys_Open (const char *filename, IComponentFactory *parent)
{
	IFileSystem *file = 0;
	DAFILEDESC	desc  = filename;

    if (parent == NULL)
        parent = DACOM;

    char *imp = 0; //GetImplementation((char *)filename); // "UTF";

	if (parent)
	{
		desc.lpImplementation = imp;

	    desc.dwDesiredAccess = GENERIC_READ;
        desc.dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE;
		//
		// the rest of desc is already set to the correct default parameters
		//
		parent->CreateInstance(&desc, (void **) &file);
	}
	return file;
}

IFileSystem *FileSys_OpenWrite (const char *filename, IComponentFactory *parent)
{
	IFileSystem *file = 0;
	DAFILEDESC	desc  = filename;

    if (parent == NULL)
        parent = DACOM;

    char *imp = GetImplementation((char *)filename); // "UTF";

    if (parent)
    {
	    desc.lpImplementation = imp;
	    desc.dwCreationDistribution = OPEN_ALWAYS; //CREATE_ALWAYS;

	    desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
        desc.dwShareMode = FILE_SHARE_READ;
	    //
	    // the rest of desc is already set to the correct default parameters
	    //
	    parent->CreateInstance(&desc, (void **) &file);
    }

	return file;
}

IFileSystem *FileSys_Create (const char *filename, IComponentFactory *parent)
{
	IFileSystem *file = 0;
	DAFILEDESC desc = filename;

    if (parent == NULL)
        parent = DACOM;

    char *imp = GetImplementation((char *)filename); // "UTF";

    if (parent)
    {
	    desc.lpImplementation = imp;
	    desc.dwCreationDistribution = CREATE_ALWAYS;

	    desc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
        desc.dwShareMode = FILE_SHARE_READ;
	    //
	    // the rest of desc is already set to the correct default parameters
	    //
	    parent->CreateInstance(&desc, (void **) &file);
    }

	return file;
}


/////////////////////////////////////////////////////////////////////////////
// The one and only UTFApp object

UTFApp theApp;

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {4814D383-2960-11D3-9B98-0050049E94BC}
static const CLSID clsid =
{ 0x4814d383, 0x2960, 0x11d3, { 0x9b, 0x98, 0x0, 0x50, 0x4, 0x9e, 0x94, 0xbc } };


CString GetModulePath (HMODULE hModule)
{
	char fname[128];
	if (GetModuleFileName(hModule,fname,sizeof(fname)))
	{
		char *p = strrchr(fname,'\\');
		if (p)
			p[1] = 0;
	}
	else
	{
		fname[0] = 0;
	}
	return fname;
}


/////////////////////////////////////////////////////////////////////////////
// UTFApp initialization

BOOL UTFApp::InitInstance()
{
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

//#ifdef _AFXDLL
//	Enable3dControls();			// Call this when using MFC in a shared DLL
//#else
//	Enable3dControlsStatic();	// Call this when linking to MFC statically
//#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Digital Anvil UTF viewer"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_UTFAPPTYPE,
		RUNTIME_CLASS(UTFDoc),
		RUNTIME_CLASS(ChildFrame), // custom MDI child frame
		RUNTIME_CLASS(UTFView));
	pDocTemplate->SetContainerInfo(IDR_UTFAPPTYPE_CNTR_IP);
	pDocTemplate->SetServerInfo(
		IDR_UTFAPPTYPE_SRVR_EMB, IDR_UTFAPPTYPE_SRVR_IP,
		RUNTIME_CLASS(CInPlaceFrame));
	AddDocTemplate(pDocTemplate);
	list_template = pDocTemplate;

	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, pDocTemplate, FALSE);

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleTemplateServer::RegisterAll();
		// Note: MDI applications register all server objects without regard
		//  to the /Embedding or /Automation on the command line.

	// create main MDI Frame window
	MainFrame* pMainFrame = new MainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

//-----------------
	if ((DACOM = DACOM_Acquire()) == 0)
	{
		TRACE0("DACOM startup failed!\n");
		return -1;
	}

// LOOK FOR INI FILE WHERE EXECUTABLE LIVES

	CString ini_file = "VU.INI";

	if( FAILED( DACOM->SetINIConfig( ini_file ) ) ) {

		ini_file = GetModulePath(m_hInstance) + "VU.INI";

		if( FAILED( DACOM->SetINIConfig( ini_file ) ) ) {
			MessageBox( GetDesktopWindow(), "Unable to find vu.ini", "Error", MB_OK );
			exit( 0 );
		}
	}

	char *BasicTypes = 
	"struct Vector { float x,y,z; };\n"
	"struct Matrix { Vector i,j,k; };\n"
	"typedef unsigned short wchar_t;\n"
	"typedef wchar_t UnicodeString[];\n"
	"typedef char String[];\n"
	"typedef int varInt[];\n"
	"typedef __hexview unsigned long varU32[];\n"
	"typedef float varFloat[];\n"
	;
	ParseMemory(BasicTypes,0);
//-----------------

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
	m_server.UpdateRegistry(OAT_INPLACE_SERVER);

    // pci = do not call NewDocument on startup!
	if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileNew)
	{
		if (!ProcessShellCommand(cmdInfo))
			return FALSE;
	}


	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void UTFApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// UTFApp message handlers

/////////////////////////////////////////////////////////////////////////////
// UTFApp commands

int UTFApp::is_viewing (void *chunk) const
{
//    POSITION pos = data_template->GetFirstDocPosition();
//    while (pos != NULL)
//    {
//		CDocument *doc = data_template->GetNextDoc(pos);
//		if (doc->IsKindOf(RUNTIME_CLASS(UTFDoc)))
//		{
//			if (((UTFDoc*)doc)->is_viewing(chunk))
//                return 1;
//		}
//	}
    return 0;
}

BOOL32 UTFApp_ParseFileAsMemory(const C8 *fileIn)
{
	DWORD flags = CFile::modeRead;

	CFile file;
	if( file.Open(fileIn,flags) == false )
	{
		return false;
	}

	CTime time = CTime::GetCurrentTime();
	CString fnTemp = time.Format( "tempfile_%A.%B.%d.%Y.%S" );

	flags = CFile::modeWrite | CFile::shareExclusive | CFile::modeCreate;
	CFile temp;
	if( temp.Open(fnTemp,flags) == false )
	{
		file.Close();
		return false;
	}

	CFile fileErr(fnTemp + "_err.txt",flags);

	CString cmdLine = "cl.exe ";
	cmdLine += "/EP ";
	cmdLine += "/nologo ";
	cmdLine += "/IZ:\\CQ2\\Code\\App\\DInclude ";
	cmdLine += "\"";
	cmdLine += fileIn;
	cmdLine += "\"";

	STARTUPINFO info;
	memset(&info, 0, sizeof(info));
	info.cb			= sizeof(info);
	info.dwFlags	= STARTF_USESTDHANDLES;
	info.hStdOutput = temp.m_hFile;
	info.hStdError	= fileErr.m_hFile; // GetStdHandle(STD_ERROR_HANDLE);
	info.hStdInput	= GetStdHandle(STD_INPUT_HANDLE);

	PROCESS_INFORMATION processInfo;
	DWORD dwflags = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS;

	BOOL32 result = CreateProcess(
		NULL,				  // app name
		cmdLine.GetBuffer(0), // cmd line
		NULL,				  // lpProcessAttributes
		NULL,				  // lpThreadAttributes
		true,				  // bInheritHandles
		dwflags,			  // creation flags
		NULL,				  // lpEnvironment
		NULL,				  // lpCurrentDirectory
		&info,
		&processInfo);

	if (result)
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}

	return result;
}

//---------------------------------------------------------------------------

char TypedefFilter[] =
"C Header (*.h,hpp,def)|*.h;*.hpp,def|"
"All Files (*.*)|*.*||";

void UTFApp::OnParseTypes ()
{
	CFileDialog files(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,TypedefFilter,NULL);

	if (files.DoModal() == IDOK)
	{
//		ProcessHeaders::SetPath();
//
//		CString path = files.GetPathName();
//		path.SetAt( path.ReverseFind('\\'), NULL );
//		::SetCurrentDirectory( path );
//		CString name = files.GetPathName();
//		UTFApp_ParseFileAsMemory(name);

		CString name = files.GetPathName();
		ParseFile(name);
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

class DefineDlg : public CDialog
{
public:
	DefineDlg (const char *t);

	CString text;

// Dialog Data
	//{{AFX_DATA(DefineDlg)
	enum { IDD = IDD_TYPE_DEFINE };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(DefineDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CDefineDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

DefineDlg::DefineDlg (const char *t) : CDialog(DefineDlg::IDD)
{
	text = t;
	//{{AFX_DATA_INIT(DefineDlg)
	//}}AFX_DATA_INIT
}

void DefineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DefineDlg)
	DDX_Text(pDX,IDC_TEXT,text);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(DefineDlg, CDialog)
	//{{AFX_MSG_MAP(DefineDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


//---------------------------------------------------------------------------

void UTFApp::OnTypeDefine ()
{
	DefineDlg dlg("");

	if (dlg.DoModal() == IDOK)
	{
		ParseMemory(dlg.text,dlg.text.GetLength());
	}
}

//---------------------------------------------------------------------------

void UTFApp::OnFileParseprocessedfile()
{
	char ProcessedHeaderTypedefFilter[] =
		"Processed Headers (*.i)|*.i|"
		"All Files (*.*)|*.*||";

	CFileDialog files(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,ProcessedHeaderTypedefFilter,NULL);

	if (files.DoModal() == IDOK)
	{
		CString name = files.GetPathName();

		CStdioFile preProcessedFile;
		DWORD dwFlags = CFile::typeText | CFile::modeRead;

		if( preProcessedFile.Open(name,dwFlags) )
		{
			unsigned int fileSize = (unsigned int)preProcessedFile.GetLength();
			char* block = new char[ fileSize ];
			preProcessedFile.ReadString( block, fileSize );
			ParseMemory(block,fileSize);

			ViewMemory("MT_MUSIC_DATA",block);

			delete block;
		}
	}
}
