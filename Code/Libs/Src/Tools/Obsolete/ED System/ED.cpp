// ED.cpp : Defines the class behaviors for the application.
//
//---------------------------------------------------------------------------
#include "PCH.h"
#include "stdafx.h"
#include "ED.h"
#include "ScenePaletteUI.h"
#include "DADeformableObject.h"
#include "DARenderPipeline.h"
#include "Char.h"
#include "StringUtils.h"
#include "GlSceneViewUI.h"
#include "FDump.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef _DEBUG
const SeverityLevel	kROSESeverityLevel = SEV_WARNING;
#else
const SeverityLevel	kROSESeverityLevel = SEV_FATAL;
#endif
//---------------------------------------------------------------------------
IRenderPipeline*		PIPE = NULL;
static DA_ERROR_HANDLER	gOriginalErrorHandler = NULL;
//---------------------------------------------------------------------------
int __cdecl ROSEErrorHandler(ErrorCode code, const C8 *fmt, ...)
{
	// Report the error
	// WARNING: This uses a fixed size buffer.
	char buffer[4096];
	va_list args;
	va_start(args, fmt);
	wvsprintf(buffer, fmt, args);
	va_end(args);

	// NOTE: Newlines are already added to trace severity.
	if(code.severity <= kROSESeverityLevel)
	{
		strcat(buffer, "\nInvoke debugger?");

		if(IDYES == MessageBox(NULL, buffer, "Assertion Failed!", MB_YESNO))
		{
			DebugBreak();
		}
	}
	else
	{
		OutputDebugString(buffer);
	}

	return 0;
}
/////////////////////////////////////////////////////////////////////////////
// CEDApp

BEGIN_MESSAGE_MAP(CEDApp, CWinApp)
	//{{AFX_MSG_MAP(CEDApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEDApp construction
CEDApp::CEDApp()
:mAccelerator(NULL)
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}
/////////////////////////////////////////////////////////////////////////////
// CEDApp destruction
CEDApp::~CEDApp()
{
#if 0 
	// Moved into TScenePaletteUIForm::~TScenePaletteUIForm()
	AudioObjectSystemShutdown();
#endif
}
/////////////////////////////////////////////////////////////////////////////
// The one and only CEDApp object

CEDApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CEDApp initialization
//---------------------------------------------------------------------------
void MyWinExit()
{
}
//---------------------------------------------------------------------------
BOOL CEDApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	mAccelerator = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR));

	// Setup error handling function
	gOriginalErrorHandler = FDUMP;
	FDUMP = ROSEErrorHandler;

	// Load the .INI file
	const int	size = 255;
	char		moduleFileName[size];
	
	GetModuleFileName(NULL, moduleFileName, size);

    CString		iniName = CString(GetFilePath(moduleFileName).c_str()) + CString("ED.ini");
	const char*	cIniName = iniName;
	
	try
	{
		CharMain(AfxGetInstanceHandle(), MyWinExit, cIniName, 16, 16, &PIPE);
		
		ISystemContainer*	system = CharGetSystemContainer();
		IEngine*			engine = CharGetEngine();

		// Create the main application dialog form.
		TScenePaletteUIForm dlg(NULL, system, engine);

		m_pMainWnd = &dlg;

	#if 0	
		// Moved into TScenePaletteUIForm::OnInitDialog()
		AudioObjectSystemStartup(dlg.m_hWnd);	/*******NOTE: m_hWnd is NULL at this point. How do we pass a valid handle to AudioObjectSystemStartup()? We could move the call into TScenePaletteUIForm. ******/
	#endif

		// Start the main dialog box

		int nResponse = dlg.DoModal();

		if (nResponse == IDOK)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with OK
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: Place code here to handle when the dialog is
			//  dismissed with Cancel
		}
	}
	catch(const std::exception& ex)
	{	
		MessageBox(NULL, ex.what(), "Error", MB_OK);

		return FALSE;
	}
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
//---------------------------------------------------------------------------
int CEDApp::ExitInstance() 
{
	CharAppExit();
	
	return CWinApp::ExitInstance();
}
//---------------------------------------------------------------------------
BOOL CEDApp::ProcessMessageFilter(int code, LPMSG lpMsg) 
{
	if(code < 0)
	{
		CWinApp::ProcessMessageFilter(code, lpMsg);
	}
	
	if(m_pMainWnd->GetSafeHwnd() && mAccelerator)
	{
		if(::TranslateAccelerator(m_pMainWnd->GetSafeHwnd(), mAccelerator, lpMsg))
		{
			return(TRUE);
		}
	}

	return CWinApp::ProcessMessageFilter(code, lpMsg);
}
//---------------------------------------------------------------------------
