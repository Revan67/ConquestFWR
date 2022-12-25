//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemote.cpp
//
//  Purpose: Main application class for the DbgRemote program
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

#include "stdafx.h"
#include "DbgRemote.h"
#include "DbgRemoteDoc.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "AboutDlg.h"
#include "FileFindDlg.h"
#include "DbgRemoteView.h"
#include "ConnectSettingsDlg.h"
#include "RunOptionsDlg.h"
#include "DbgNetLib/HeDbgMsg.h"
#include "OptionsDlg.h"
#include "ProjectSettingsDlg.h"
#include "SourceControlInterface.h"
#include "DocSelectDlg.h"
#include "ToolConfigDlg.h"
#include "FileModifiedDlg.h"
#include "HapRedirect.h"


#include <process.h>

#include <algorithm>


#include "tags.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const char* cszDebugClientRel    = "HapClient.exe";
const char* cszDebugClientDbg    = "HapClient_d.exe";

const char* cszPythonRel = "Python.exe";
const char* cszPythonDbg = "Python_d.exe";

const int DEBUG_EVENT_WAITMS = 50;


/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteApp

BEGIN_MESSAGE_MAP(CDbgRemoteApp, CWinApp)
	//{{AFX_MSG_MAP(CDbgRemoteApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_DEBUG_CONNECT, OnDebugConnect)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_CONNECT, OnUpdateDebugConnect)
	ON_COMMAND(ID_DEBUG_GO, OnDebugGo)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_GO, OnUpdateDebugGo)
	ON_COMMAND(ID_DEBUG_BREAKNOW, OnDebugBreaknow)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_BREAKNOW, OnUpdateDebugBreaknow)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STOPDEBUGGING, OnUpdateDebugStopdebugging)
	ON_COMMAND(ID_DEBUG_STOPDEBUGGING, OnDebugStopdebugging)
	ON_COMMAND(ID_DEBUG_RUNDEBUGGER, OnDebugRundebugger)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_RUNDEBUGGER, OnUpdateDebugRundebugger)
	ON_COMMAND(ID_DEBUG_STEPIN, OnDebugStepin)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPIN, OnUpdateDebugStepin)
	ON_COMMAND(ID_DEBUG_STEPOUT, OnDebugStepout)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPOUT, OnUpdateDebugStepout)
	ON_COMMAND(ID_DEBUG_STEPOVER, OnDebugStepover)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_STEPOVER, OnUpdateDebugStepover)
	ON_COMMAND(ID_DEBUG_BREAKPOINTS_CLEARALL, OnDebugBreakpointsClearall)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_BREAKPOINTS_CLEARALL, OnUpdateDebugBreakpointsClearall)
	ON_COMMAND(ID_EDIT_FINDINFILES, OnEditFindinfiles)
	ON_COMMAND(ID_DEBUG_RUNOPTIONS, OnDebugRunOptions)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_RUNOPTIONS, OnUpdateDebugRunOptions)
	ON_COMMAND(ID_HELP_PYTHONHELP, OnHelpPythonHelp)
	ON_COMMAND(ID_FILE_SAVEALL, OnFileSaveall)
	ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
	ON_COMMAND(ID_FILE_NEWPROJECT, OnFileNewProject)
	ON_COMMAND(ID_FILE_OPENPROJECT, OnFileOpenProject)
	ON_COMMAND(ID_PROJECT_ADDTOPROJECT_FILES, OnAddFilesToProject)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_ADDTOPROJECT_FILES, OnUpdateAddFilesToProject)
	ON_COMMAND(ID_PROJECT_SETTINGS, OnProjectSettings)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_SETTINGS, OnUpdateProjectSettings)
	ON_COMMAND(ID_PROJECT_ADDTOPROJECT_DOCUMENTS, OnAddDocumentsToProject)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_ADDTOPROJECT_DOCUMENTS, OnUpdateAddDocumentsToProject)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT1, OnUpdateFileMruProject1)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT2, OnUpdateFileMruProject2)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT3, OnUpdateFileMruProject3)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT4, OnUpdateFileMruProject4)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT5, OnUpdateFileMruProject5)
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_PROJECT6, OnUpdateFileMruProject6)
	ON_COMMAND(ID_DEBUG_EXECUTE, OnDebugExecute)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_EXECUTE, OnUpdateDebugExecute)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_SOURCECONTROL_CONFIGURE, OnUpdateConfigureSourceControl)
	ON_COMMAND(ID_PROJECT_SOURCECONTROL_CONFIGURE, OnConfigureSourceControl)
	ON_COMMAND(ID_PROJECT_SOURCECONTROL_REFRESHSTATUS, OnSourceControlRefresh)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_SOURCECONTROL_REFRESHSTATUS, OnUpdateSourceControlRefresh)
	ON_COMMAND(ID_WINDOW_CLOSEALL, OnWindowCloseAll)
	ON_COMMAND(ID_TOOLS_PYTHONINTERACTIVE, OnToolsPythonInteractive)
	ON_COMMAND(ID_DEBUG_PYVER_DEBUG, OnDebugPyverDebug)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_PYVER_DEBUG, OnUpdateDebugPyverDebug)
	ON_COMMAND(ID_DEBUG_PYVER_REL, OnDebugPyverRel)
	ON_UPDATE_COMMAND_UI(ID_DEBUG_PYVER_REL, OnUpdateDebugPyverRel)
	ON_COMMAND(ID_TOOLS_TOOLS_CONFIGURE, OnToolsConfigure)
	ON_COMMAND(ID_FILE_CLOSEPROJECT, OnFileCloseProject)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPROJECT, OnUpdateFileCloseProject)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TOOLS_CONFIGURE, OnUpdateConfigureToolsMenu)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)

	//recent projects list
	ON_COMMAND_RANGE(ID_FILE_MRU_PROJECT1, ID_FILE_MRU_PROJECT9, OnFileMruProject)

	//user customizable tool handlers
	ON_COMMAND_RANGE(ID_TOOLS_TOOLS_TOOLSMENU1, ID_TOOLS_TOOLS_TOOLSMENU6, OnToolMenuTool)
	ON_UPDATE_COMMAND_UI_RANGE(ID_TOOLS_TOOLS_TOOLSMENU1, ID_TOOLS_TOOLS_TOOLSMENU6, UpdateToolsMenu )

	ON_COMMAND(ID_PROJECT_BUILDTAGS, OnProjectBuildtags)
	ON_UPDATE_COMMAND_UI(ID_PROJECT_BUILDTAGS, OnUpdateProjectBuildtags)
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteApp construction

CDbgRemoteApp::CDbgRemoteApp()
{
	m_CurState = eDBGSTATE_INVALID;
	m_pSourceCodeDocTemplate = NULL;
	m_pLastCurLineVw = NULL;
	m_uiSockCheckTimer = 0;
	m_uiMemCheckTimer = 0;
	m_pFindInFilesDlg = NULL;
	m_pSourceControl = new CSourceControlInterface();
	m_hDebuggerThread = 0;
	m_bIgnoreFileChanges = false;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDbgRemoteApp object

CDbgRemoteApp theApp;

DWORD dwAppMainThreadId = NULL;

HMODULE m_hScintillaModule = NULL;

/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteApp initialization

BOOL CDbgRemoteApp::InitInstance()
{
	AfxEnableControlContainer();

	if (AfxSocketInit() == FALSE)
	{
		AfxMessageBox("Unable to initialize Sockets", MB_ICONSTOP|MB_OK);
		return FALSE;
	}
#if _MSC_VER < 1300 		
	#ifdef _AFXDLL
		Enable3dControls();			// Call this when using MFC in a shared DLL
	#else
		Enable3dControlsStatic();	// Call this when linking to MFC statically
	#endif
#endif

	SetRegistryKey(_T("HE Tools"));

	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)
	m_UserCfg.LoadUserCfg();

	//load the scintilla dll
	if (m_hScintillaModule == NULL)
	{
#ifdef _DEBUG
		const char* scintillaName = "Scintilla_d.dll";
#else
		const char* scintillaName = "Scintilla.dll";
#endif
		m_hScintillaModule = LoadLibrary(scintillaName);
		if (m_hScintillaModule == NULL)
		{
			// I replaced the use of MessageBox(NULL,...) with AfxMessageBox
			// because of the dangers of passing a NULL window handle to MessageBox.
			// While the problems with MessageBox(NULL,...) may not be present in
			// this case it is a good policy to avoid it, unless there is a specific
			// reason it is required. Bruce Dawson, March 20, 2002
			string message = stringprintf("The Scintilla DLL could not be loaded.\n"
							"Check that '%s' is in your program directory.", scintillaName);
			AfxMessageBox(message.c_str(), MB_OK | MB_ICONERROR);
			return FALSE;
		}
	}


	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	m_pSourceCodeDocTemplate = new CMultiDocTemplate(
		IDR_DBGREMTYPE,
		RUNTIME_CLASS(CDbgRemoteDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CDbgRemoteView));
	AddDocTemplate(m_pSourceCodeDocTemplate);

	EnableShellOpen();
	//Don't register the shell file types, we'll do that in a seperate registration script that does it
	//correctly....
	//RegisterShellFileTypes();

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileNew)
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

	// Dispatch commands specified on the command line
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;
	ProcessShellCommand(cmdInfo);

	// The main window has been initialized, so show and update it.
	UINT iShowCmd = GetProfileInt(_T("Settings"),_T("ShowCmd"),SW_SHOW);
	m_pMainWnd->ShowWindow(iShowCmd);
	//pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();
	pMainFrame->DragAcceptFiles();

	//set the initial debugger state to edit mode
	SetDebuggerState(eDBGSTATE_EDIT);

	dwAppMainThreadId = ::GetCurrentThreadId();
	m_DebugSocket.SetEventCallback(CDbgRemoteApp::SocketCallbackFn);
	
	m_vecMessageBuffer.resize(512);

	//initialize the source control interface dll
	const char* SccDll = m_UserCfg.GetSccDll();
	if (strlen(SccDll))
	{
		m_pSourceControl->Open(SccDll, m_pMainWnd->m_hWnd);
	}

	//optionally open the last project
	if (m_UserCfg.GetOpenLastProj())
	{
		OnFileMruProject(ID_FILE_MRU_PROJECT1);
	}

	//set the current state to edit
	m_CurState = eDBGSTATE_EDIT;

	// Get the application directory. First get the application path.
	char	appDir[MAX_PATH] = "";
	(void)GetModuleFileName(0, appDir, NUMELEMENTS(appDir));
	// Then remove the filename.
	char* lastslash = strrchr(appDir, '\\');
	if (lastslash)
		lastslash[1] = 0;
	string debugName = string(appDir) + cszDebugClientDbg;
	//check for the presence of the HapClient app
	HANDLE hDbgClient = CreateFile(debugName.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
		                           0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	m_bDebugClientAvailable = (hDbgClient != INVALID_HANDLE_VALUE);
	if (m_bDebugClientAvailable)
	{
		//store the HapClient filename
		m_strDebugClientFile = debugName;
		CloseHandle(hDbgClient);
	}

	string releaseName = string(appDir) + cszDebugClientRel;
	HANDLE hRelClient = CreateFile(releaseName.c_str(), 0, FILE_SHARE_READ|FILE_SHARE_WRITE,
		                           0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	m_bReleaseClientAvailable = (hRelClient != INVALID_HANDLE_VALUE);
	if (m_bReleaseClientAvailable)
	{
		//store the HapClient filename
		m_strReleaseClientFile = releaseName;
		CloseHandle(hRelClient);
	}
	
	//set the version mode - according to what is available
	if ((m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_RELEASE) && (m_bReleaseClientAvailable == false))
		m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_DEBUG);

	if ((m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_DEBUG) && (m_bDebugClientAvailable == false))
		m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_RELEASE);
	
	if ((m_bDebugClientAvailable == false) && (m_bReleaseClientAvailable == false))
	{
		AfxMessageBox("HapClient not found - local debugging will be disabled.");
		// need to report it.
		m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_INVALID);
	}
	else if (m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_INVALID)
	{
		if (m_bReleaseClientAvailable)
			m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_RELEASE);
		else
			m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_DEBUG);
	}

	return TRUE;
}


int CDbgRemoteApp::ExitInstance() 
{
	if (m_pFindInFilesDlg)
	{
		m_pFindInFilesDlg->DestroyWindow();
		delete m_pFindInFilesDlg;
		m_pFindInFilesDlg = NULL;
	}
	

	if (m_uiSockCheckTimer)
	{
		::KillTimer(NULL, m_uiSockCheckTimer);
		m_uiSockCheckTimer = 0;
	}

	if (m_uiMemCheckTimer)
	{
		::KillTimer(NULL, m_uiMemCheckTimer);
		m_uiMemCheckTimer = 0;
	}

	m_DebugSocket.Close();


	if (m_hScintillaModule)
		FreeLibrary(m_hScintillaModule);
	m_hScintillaModule = NULL;

	m_UserCfg.SaveUserCfg();

	
	if (m_pSourceControl)
		delete m_pSourceControl;
	m_pSourceControl = 0;

	return CWinApp::ExitInstance();
}




/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteApp message handlers

void CDbgRemoteApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CDbgRemoteApp::OnDebugConnect() 
{
	if (m_UserCfg.GetAutosaveOnDebug())
		OnFileSaveall();

	UpdateDefaultRunOptions();
	if (m_RunScript.GetLength() == 0)
		OnDebugRunOptions();

	if (m_RunScript.GetLength() == 0)
		return;
	
	CConnectSettingsDlg dlg;
	dlg.m_DebugScript = m_RunScript;
	dlg.m_pConnectNames = m_UserCfg.GetLastConnectNames();
	dlg.m_nConnectType = m_UserCfg.nGetLastConnectType();

	if (dlg.DoModal() == IDOK)
	{
		m_UserCfg.SetLastConnectType(dlg.m_nConnectType);
		
		

		if (dlg.m_nConnectType == CONNECT_LOCAL)
		{
			ConnectLocal();
		}
		else
		{
			BOOL bConnectOk = ConnectRemote(dlg.m_RemoteName);
			
			if (bConnectOk)
				m_UserCfg.AddLastConnectName(dlg.m_RemoteName);
		}
	}
}

BOOL CDbgRemoteApp::ConnectLocal()
{
	char NameBuff[1024];
	int HostOk = gethostname (NameBuff, 1024);
	if (HostOk != 0)
	{
		AfxMessageBox("Unable to get host name");
		dprintf("Unable to get host name: %d\n", WSAGetLastError());
		return FALSE;
	}

	HOSTENT* pH = gethostbyname(NameBuff);
	if (pH == NULL)
	{
		AfxMessageBox("Unable to get local host");
		dprintf("Unable to get local host: %d\n", WSAGetLastError());
		return FALSE;
	}

	dprintf("Local Machine Name: ");
	dprintf(NameBuff);
	dprintf("\n");

	unsigned long ConnectAddr = 0;
	
	//ip address of this machine is:
	memcpy(&ConnectAddr, pH->h_addr, sizeof(ConnectAddr));

	if (ConnectAddr != INADDR_NONE)
	{
		int ConnectOk = m_DebugSocket.Connect(ConnectAddr, m_UserCfg.GetLastPort());
		if (ConnectOk == CHeDbgSocket::eSOCK_NOERROR)
		{
			SetDebuggerState(eDBGSTATE_CONNECTING);
		
			assert(m_pMainWnd);
			CString Msg = "Connecting to Local Machine\r\n";
			Msg += NameBuff;
			Msg += " at ";
			Msg += inet_ntoa (*((in_addr*)&ConnectAddr));

			assert( ((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar() );

			((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar()->SetStatus(eDBGSTATE_CONNECTING, Msg);
			//((CMainFrame*)m_pMainWnd)->StartConnectCheckTimer();
			return TRUE;
		}
		else
			AfxMessageBox("Connect Failed");
	}
	else
		AfxMessageBox("Unable to get local machine address");
	
	return FALSE;

} // ConnectLocal

BOOL CDbgRemoteApp::ConnectRemote(const char* szRemoteMachine)
{
	assert(szRemoteMachine);
	CString Msg = "Connecting to Remote Machine\r\n";
		
	unsigned long ConnectAddr = inet_addr(szRemoteMachine);
	if (ConnectAddr == INADDR_NONE)
	{
		HOSTENT* pH = gethostbyname(szRemoteMachine);
		if (pH != NULL)
		{
			Msg += szRemoteMachine;
			Msg += " ";
			memcpy(&ConnectAddr, pH->h_addr, 4);
		}
	}
	
	Msg += "at ";
	Msg += inet_ntoa (*((in_addr*)&ConnectAddr));

	if (ConnectAddr != INADDR_NONE)
	{
		if (m_DebugSocket.Connect(ConnectAddr, m_UserCfg.GetLastPort()) != CHeDbgSocket::eSOCK_NOERROR)
		{
			AfxMessageBox("Connect Failed");
			return FALSE;
		}

		SetDebuggerState(eDBGSTATE_CONNECTING);
	
		assert(m_pMainWnd);
		((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar()->SetStatus(eDBGSTATE_CONNECTING, Msg);
		return TRUE;
	}
	else
		AfxMessageBox("Unable to resolve address");
	
	return FALSE;
}

void CDbgRemoteApp::OnUpdateDebugConnect(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_EDIT);
}


LRESULT CDbgRemoteApp::OnAppTimer(WPARAM wParam, LPARAM lParam)
{
	if (wParam == m_uiSockCheckTimer)
	{
		CheckRead();
		
		m_DebugSocket.SendMessage(eMSG_PING, "");

		if (m_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
		{
			OnDbgMsgDone();
		}
	}
	else if (wParam == m_uiMemCheckTimer)
	{
#ifdef _PLATFORM_SDK
		PROCESS_MEMORY_COUNTERS pmc = {0};
		pmc.cb = sizeof(pmc);
		if (GetProcessMemoryInfo(m_DbgProcessInfo.hProcess, &pmc, sizeof(pmc)) )
		{
			//do some really cool memory usage monitoring				
		}
		else
		{
			int nErr = GetLastError();
			dprintf("GetProcessMemoryInfo failed %d\n", nErr);

			::KillTimer(0, m_uiMemCheckTimer);
			m_uiMemCheckTimer = 0;
		}
#endif		
	}
	return 0;
}
	

void CDbgRemoteApp::CheckConnect()
{
	
	{
		CHeDbgSocket::ESocketState SocketState = m_DebugSocket.eGetState();
		if (SocketState != CHeDbgSocket::eSOCKSTATE_CONNECTING)
		{
			//if (m_pMainWnd)
			//	((CMainFrame*)m_pMainWnd)->EndConnectCheckTimer();


			if (SocketState == CHeDbgSocket::eSOCKSTATE_CONNECTED)
			{
				SetDebuggerState(eDBGSTATE_INIT);
				
				assert(m_pMainWnd);

				unsigned long ConnectAddr = m_DebugSocket.GetAddress();
				CString Msg = "Connected to:\r\n";
				Msg += inet_ntoa (*((in_addr*)&ConnectAddr));

				((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar()->SetStatus(eDBGSTATE_INIT, Msg);

				
				
			}
		}
	}
}

void CDbgRemoteApp::CheckRead()
{
	if (m_DebugSocket.eGetState() == CHeDbgSocket::eSOCKSTATE_CONNECTED)
	{			
		//dprintf("Checking for msg:");
		SMsgHeader MsgHdr;
		EMessageType MsgType = m_DebugSocket.ReadMessageHdr(&MsgHdr);
		while (MsgType > eMSG_INVALID)
		{
			int nMsgBytes = MsgHdr.GetMsgSize();	//strtol(MsgHdr.MsgSize, NULL, 10);
			
			//dprintf(" rcvd - %d, %d bytes\n", MsgType, nMsgBytes);
			if (nMsgBytes > 0)
			{
				m_vecMessageBuffer.resize(nMsgBytes);

				int nMsgBytesRead = m_DebugSocket.ReadMessageBody(&(m_vecMessageBuffer[0]), &MsgHdr);
				if (nMsgBytesRead != nMsgBytes)
					dprintf("Message %d size - read: %d, expected %d\n", MsgType, nMsgBytesRead, nMsgBytes);
			}
			else
			{
				m_vecMessageBuffer[0] = 0;
				m_vecMessageBuffer[1] = 0;
				
			}

			if ((MsgType == eMSG_PING) || (MsgType == eMSG_PONG))
			{
				if (MsgType == eMSG_PING)
					m_DebugSocket.SendMessage(eMSG_PONG, "");
				//else ignore
			}
			else
			{
				switch (m_CurState)
				{
				case eDBGSTATE_EDIT:
				case eDBGSTATE_CONNECTING:
					//ignore
					assert(0);
					break;

				case eDBGSTATE_INIT:
					HandleDbgMsgInit(MsgType, &(m_vecMessageBuffer[0]));
					break;

				case eDBGSTATE_RUNNING:
					HandleDbgMsgRun(MsgType,  &(m_vecMessageBuffer[0]));
					break;

				case eDBGSTATE_BREAK:
					HandleDbgMsgBreak(MsgType, &(m_vecMessageBuffer[0]));
					break;

				case eDBGSTATE_EXCEPTION:
					HandleDbgMsgExcept(MsgType, &(m_vecMessageBuffer[0]));
					break;

				}
			}

			if (m_DebugSocket.eGetState() != CHeDbgSocket::eSOCKSTATE_CONNECTED)
			{
				OnDbgMsgDone();
				break;
			}

			MsgType = m_DebugSocket.ReadMessageHdr(&MsgHdr);
			
		}//while
	
	}//if connected
} // CheckRead




void CDbgRemoteApp::ShowCallStack(char_vector& vecCallStack)
{
	if (m_pMainWnd)
	{
		CStackViewBar* pStackVw = ((CMainFrame*)m_pMainWnd)->pGetStackViewBar();
		assert(pStackVw);
		pStackVw->SetCallStack(vecCallStack);
	}
}

void CDbgRemoteApp::ShowSourceLine(const char* SourceInfo)
{	
	//filename, line#
	char* pFileEnd = strchr(SourceInfo, ',');
	
	char SrcFile[MAX_PATH];
	memset(SrcFile, 0, MAX_PATH);
	int nLineNumber = -1;
	if (pFileEnd)
	{
		strncpy(SrcFile, SourceInfo, pFileEnd-SourceInfo);
		
		nLineNumber = strtoul(pFileEnd+1, NULL, 10) ;
	}

	assert(m_pSourceCodeDocTemplate);
	
	//clear the current indicator
	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDocument* pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		while (pCurDoc)
		{
			POSITION VwPos = pCurDoc->GetFirstViewPosition();
			assert(VwPos);
			CView* pVw = pCurDoc->GetNextView(VwPos);
			assert(pVw);

			((CDbgRemoteView*)pVw)->SetCurrentLine(-1);

			if (DocPos)
				pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}
	}

	//if we have a -1 line number we can just return now that we've cleared
	//the current line indicator
	if (nLineNumber < 0)
		return;

	CDocument* pDoc = 0;

	//get the full path/file for the file
	CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();


	
	CString SrcPathFile = pProjWnd->GetFilePathName(SrcFile);
	if (SrcPathFile.GetLength() > 0)
		pDoc = OpenDocumentFile(SrcPathFile);

	if (pDoc == NULL)
		pDoc = OpenDocumentFile(SrcFile);
	if (pDoc == NULL)
	{
		SrcPathFile = m_WorkingDir;
		SrcPathFile += SrcFile;
		pDoc = OpenDocumentFile(SrcPathFile);
	}	

	//try to get the open document
	if (pDoc)
	{
		POSITION pos = pDoc->GetFirstViewPosition();
		if (pos)
		{
			CDbgRemoteView* pVw = (CDbgRemoteView*) pDoc->GetNextView(pos);
			if (pVw)
			{
				pVw->SetCurrentLine(nLineNumber);
				pVw->SetFocus();
			}
		}
	}
} // ShowSourceLine

void CDbgRemoteApp::UpdateWatch(int nItem)
{
	if (m_pMainWnd)
	{
		CWatchBar* pWatchWnd = ((CMainFrame*)m_pMainWnd)->pGetWatchBar();

		assert(pWatchWnd);

		int WatchStart = 0;
		int WatchEnd;
		if (nItem > -1)
		{
			WatchStart = nItem;
			WatchEnd = nItem+1;
		}
		else
			WatchEnd = pWatchWnd->GetWatchCount();

		char WatchBuff[256];
		for (int i = WatchStart; i < WatchEnd; i++)
		{
			sprintf(WatchBuff, "%d, ", i);
			char* pWatch = WatchBuff + strlen(WatchBuff);
			pWatchWnd->GetWatchItem(i, pWatch, 250);

			if (strlen(pWatch) > 0)
				m_DebugSocket.SendMessage(eMSG_EVALUATE, WatchBuff);
		}
	}
}

void CDbgRemoteApp::GetAutowatchItem(const char* cszWatch)
{
	assert(cszWatch && strlen(cszWatch));

	if (m_CurState == eDBGSTATE_BREAK)
	{
		vector<char> vecWatchBuff;
		//for an autowatch, we'll flag it with an index of -1 that will 
		//tell us to update the view when we receive a response.
		vecWatchBuff.resize(strlen(cszWatch) + 64);
		sprintf(&(vecWatchBuff[0]), "%d, %s", eDBGCLIENT_VIEW, cszWatch);
	
		m_DebugSocket.SendMessage(eMSG_EVALNOERR, &(vecWatchBuff[0]));
	}
}

void CDbgRemoteApp::GetAutoComplete(const char* cszExpr, EDebugExecuteClient eClientId)
{
	assert(cszExpr && strlen(cszExpr));

	if (m_CurState == eDBGSTATE_BREAK)
	{
		vector<char> vecWatchBuff;
		//for an autowatch, we'll flag it with an index of -1 that will 
		//tell us to update the view when we receive a response.
		
		vecWatchBuff.resize(strlen(cszExpr) + 128);
		sprintf(&(vecWatchBuff[0]), "%d, dir(%s)", (int)eClientId, cszExpr);
	
		m_DebugSocket.SendMessage(eMSG_EVALNOERR, &(vecWatchBuff[0]));
	}
}

void CDbgRemoteApp::UpdateWatchItem(const char* cszWatchData)
{
	if (m_pMainWnd)
	{
		CWatchBar* pWatchWnd = ((CMainFrame*)m_pMainWnd)->pGetWatchBar();
		assert(pWatchWnd);

		if (strlen(cszWatchData) > 0)
		{
			//message should have the form inx, value eg: "10, 145"
			char* pNext;
			int nItemInx = strtol(cszWatchData, &pNext, 10);
			pNext++;

			if (nItemInx > -1)
			{
				//update the item from the watch window
				pWatchWnd->UpdateWatchItem(nItemInx, pNext);
			}
			else if (nItemInx == eDBGCLIENT_VIEW)
			{
				//-1 is a special id that indicates the eval was from the view
				//in the form of an autowatch.
				CMDIChildWnd *pChild = ((CMainFrame*)m_pMainWnd)->MDIGetActive();
				if (pChild)
				{
					CDbgRemoteView* pVw = (CDbgRemoteView*)pChild->GetActiveView();
					if (pVw)
						pVw->DisplayAutoWatch(pNext);
				}
			}
			else if (nItemInx == eDBGCLIENT_OUTPUT)
			{
				//update the autocomplete eval from the trace window
				CTraceViewBar* pTraceWnd = ((CMainFrame*)m_pMainWnd)->pGetTraceViewBar();
				assert(pTraceWnd);
				pTraceWnd->DisplayAutoComplete(pNext);
			}
		}
		else
		{
			//clear the watch window values
			for (int i = 0; i < pWatchWnd->GetWatchCount(); i++)
				pWatchWnd->UpdateWatchItem(i, "");
		}
	}
} // UpdateWatchItem


void CDbgRemoteApp::UpdateLocals(const char* cszLocalStr)
{
	if (m_pMainWnd)
	{
		CVariableViewBar* pVarWnd = ((CMainFrame*)m_pMainWnd)->pGetVariableBar();
		assert(pVarWnd);
		assert(cszLocalStr);
		pVarWnd->UpdateLocals(cszLocalStr);
	}
}

void CDbgRemoteApp::UpdateGlobals(const char* cszGlobalStr)
{
	if (m_pMainWnd)
	{
		CVariableViewBar* pVarWnd = ((CMainFrame*)m_pMainWnd)->pGetVariableBar();
		assert(pVarWnd);
		assert(cszGlobalStr);
		pVarWnd->UpdateGlobals(cszGlobalStr);
	}
}


void CDbgRemoteApp::AddTrace(const char* cszTraceMsg)
{
	dprintf("Trace: %s", cszTraceMsg);
	if (m_pMainWnd)
	{
		CTraceViewBar* pTraceWnd = ((CMainFrame*)m_pMainWnd)->pGetTraceViewBar();
		assert(pTraceWnd);
		assert(cszTraceMsg);
		assert(strlen(cszTraceMsg) > 0);
		pTraceWnd->AddDebugMessage(cszTraceMsg);
	}
}

void CDbgRemoteApp::AddOutput(const char* cszTraceMsg)
{
	dprintf("Output: %s", cszTraceMsg);
	if (m_pMainWnd)
	{
		CTraceViewBar* pTraceWnd = ((CMainFrame*)m_pMainWnd)->pGetTraceViewBar();
		assert(pTraceWnd);
		assert(cszTraceMsg);
		assert(strlen(cszTraceMsg) > 0);
		pTraceWnd->AddOutputMessage(cszTraceMsg);
	}
}

//#include <dbghelp.h>
//#pragma comment(lib, "Dbghelp.lib")

//void ShowStack( HANDLE hThread, HANDLE hProcess, CONTEXT& c );

struct ProcessInfo
{
	ProcessInfo(HANDLE handle, DWORD id)
		: hProcess(handle), breakpointReceived(false), processID(id)
	{
	}

	bool operator==(DWORD id) const
	{
		return processID == id;
	}

	HANDLE	hProcess;
	bool	breakpointReceived;
	DWORD	processID;
};

unsigned int __stdcall CDbgRemoteApp::AdvancedDebuggerThreadFn(void* pParam)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)pParam;
	assert(pApp);

	if (!pApp->StartDebugProcess(true))
	{
		pApp->OnDbgMsgDone();
		return -1;			
	}
	
	// Child processes get debugged also (unless specifically excluded, and we don't
	// really want to do that) so we have to keep track of multiple processes.
	// See StartDebugProcess() for the details.
	vector<ProcessInfo>	processes;
	DEBUG_EVENT de;
	while ( !pApp->m_bEndDebuggerThread)
	{
		if (!WaitForDebugEvent(&de, DEBUG_EVENT_WAITMS))
		{
			continue;
		}

		// Find the record for the current process.
		vector<ProcessInfo>::iterator activeProcess = find(processes.begin(), processes.end(), de.dwProcessId);
		// Make sure a record was actually found.
		assert(de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT || activeProcess != processes.end());
		// Assume that this event is not an unhandled exceptions to pass back to the application.
		bool unhandledException = false;
		switch (de.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			//dprintf("Creating process %d.\n", de.dwProcessId);
			assert(activeProcess == processes.end());
			processes.push_back(ProcessInfo(de.u.CreateProcessInfo.hProcess, de.dwProcessId));
			break;

		case EXIT_PROCESS_DEBUG_EVENT:
		{
			//dprintf("Destroying process %d.\n", de.dwProcessId);
			processes.erase(activeProcess);
			pApp->m_bEndDebuggerThread = processes.empty();
			break;
		}

		case LOAD_DLL_DEBUG_EVENT:
		{
			char	buffer[1000];
			void* pImageName = de.u.LoadDll.lpImageName;
			bool success = false;
#ifdef _PLATFORM_SDK			
			if (de.u.LoadDll.lpBaseOfDll &&
						GetModuleFileNameEx(pApp->m_DbgProcessInfo.hProcess,
						(HMODULE)de.u.LoadDll.lpBaseOfDll, buffer, sizeof(buffer)) > 0)
			{
				// This way of getting the module name works sometimes - but not always.
				string message = string("Loaded '") + buffer + "'\n";
				pApp->AddTrace(message.c_str());
				success = true;
			}
			else if (pImageName)
#else
			if (pImageName)
#endif
			{
				// This method of getting the module name seems to work more often than
				// GetModuleFileNameEx, but it still doesn't always work. It can fail
				// on early loads.
				char* ptr;
				DWORD bytesToRead = sizeof(ptr);
				DWORD bytesRead;
				if (ReadProcessMemory(pApp->m_DbgProcessInfo.hProcess, pImageName,
					                  &ptr, bytesToRead, &bytesRead) && bytesRead == bytesToRead)
				{
					bytesToRead = sizeof(buffer);
					if (ReadProcessMemory(pApp->m_DbgProcessInfo.hProcess, ptr,
										  buffer, bytesToRead, &bytesRead) && bytesRead == bytesToRead)
					{
						// Make sure the string is null-terminated.
						buffer[sizeof(buffer)-1] = 0;
						char* text = buffer;
						char newBuffer[sizeof(buffer)];
						if (de.u.LoadDll.fUnicode)
						{
							WideCharToMultiByte(CP_ACP,
										0,	// Flags
										(LPCWSTR)buffer,
										-1,	// Length
										newBuffer,
										sizeof(newBuffer),
										0,
										0);
							text = newBuffer;
						}
						string message = string("Loaded '") + text + "'\n";
						pApp->AddTrace(message.c_str());
						success = true;
					}
				}
			}
			if (!success)
			{
				// On my machine the first DLL loaded is ntdll.dll at 0x77F80000
				char buffer[1000];
				snprintfAssert(buffer, NUMELEMENTS(buffer), "Loaded unknown module (0x%08lx).\n", de.u.LoadDll.lpBaseOfDll);
				pApp->AddTrace(buffer);
			}
			break;
		}

		case EXCEPTION_DEBUG_EVENT:
		{
			/*{
				static int MyBrk = 0;
				if (de.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
				{
					if (MyBrk++ < 1)
						break;

					CONTEXT c;
					memset( &c, '\0', sizeof c );
					c.ContextFlags = CONTEXT_FULL;

					// init CONTEXT record so we know where to start the stackwalk
					if ( ! GetThreadContext(pi.hThread, &c ) )
					{
						break;
					}
					
					ShowStack(pi.hThread, pi.hProcess, c);
				}
			}*/
			const char* exceptionName = "Unknown exception";
			// Assume that this exception *is* an unhandled exception.
			unhandledException = true;
			switch (de.u.Exception.ExceptionRecord.ExceptionCode)
			{
				case EXCEPTION_BREAKPOINT:
					// Only the first breakpoint is actually 'expected' - other breakpoints
					// definitely indicate a problem, typically some sort of heap corruption.
					if (!activeProcess->breakpointReceived)
					{
						activeProcess->breakpointReceived = true;
						// The first breakpoint we receive per process is normal and expected.
						unhandledException = false;
					}
					else
					{
						// This will make hitting F12 in the game drop into the HAP
						// debugger, because F12 triggers a breakpoint. This only works under
						// WinNT/2K/XP if you are running the client as a debuggee under HAP,
						// i.e.; when you are using Advanced Debugging Support
						pApp->OnDebugBreaknow();
					}
					exceptionName = "EXCEPTION_BREAKPOINT";
					break;
				// This list was taken from winbase.h, and is generally in that order.
				case EXCEPTION_ACCESS_VIOLATION:		exceptionName = "EXCEPTION_ACCESS_VIOLATION"; break;
				case EXCEPTION_DATATYPE_MISALIGNMENT:	exceptionName = "EXCEPTION_DATATYPE_MISALIGNMENT"; break;
				case EXCEPTION_SINGLE_STEP:				exceptionName = "EXCEPTION_SINGLE_STEP"; break;
				case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:	exceptionName = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"; break;
				case EXCEPTION_FLT_DENORMAL_OPERAND:	exceptionName = "EXCEPTION_FLT_DENORMAL_OPERAND"; break;
				case EXCEPTION_FLT_DIVIDE_BY_ZERO:		exceptionName = "EXCEPTION_FLT_DIVIDE_BY_ZERO"; break;
				case EXCEPTION_FLT_INEXACT_RESULT:		exceptionName = "EXCEPTION_FLT_INEXACT_RESULT"; break;
				case EXCEPTION_FLT_INVALID_OPERATION:	exceptionName = "EXCEPTION_FLT_INVALID_OPERATION"; break;
				case EXCEPTION_FLT_OVERFLOW:			exceptionName = "EXCEPTION_FLT_OVERFLOW"; break;
				case EXCEPTION_FLT_STACK_CHECK:			exceptionName = "EXCEPTION_FLT_STACK_CHECK"; break;
				case EXCEPTION_FLT_UNDERFLOW:			exceptionName = "EXCEPTION_FLT_UNDERFLOW"; break;
				case EXCEPTION_INT_DIVIDE_BY_ZERO:		exceptionName = "EXCEPTION_INT_DIVIDE_BY_ZERO"; break;
				case EXCEPTION_INT_OVERFLOW:			exceptionName = "EXCEPTION_INT_OVERFLOW"; break;
				case EXCEPTION_PRIV_INSTRUCTION:		exceptionName = "EXCEPTION_PRIV_INSTRUCTION"; break;
				case EXCEPTION_IN_PAGE_ERROR:			exceptionName = "EXCEPTION_IN_PAGE_ERROR"; break;
				case EXCEPTION_ILLEGAL_INSTRUCTION:		exceptionName = "EXCEPTION_ILLEGAL_INSTRUCTION"; break;
				case EXCEPTION_NONCONTINUABLE_EXCEPTION: exceptionName = "EXCEPTION_NONCONTINUABLE_EXCEPTION"; break;
				case EXCEPTION_STACK_OVERFLOW :			exceptionName = "EXCEPTION_STACK_OVERFLOW"; break;
				case EXCEPTION_INVALID_DISPOSITION:		exceptionName = "EXCEPTION_INVALID_DISPOSITION"; break;
				case EXCEPTION_GUARD_PAGE:				exceptionName = "EXCEPTION_GUARD_PAGE"; break;
				case EXCEPTION_INVALID_HANDLE:			exceptionName = "EXCEPTION_INVALID_HANDLE"; break;
				//case CONTROL_C_EXIT:					exceptionName = "CONTROL_C_EXIT"; break;
				default:
					//dprintf("Unknown Exception Code 0x%08lx", de.u.Exception.ExceptionRecord.ExceptionCode); 
					break;
			}
			if (unhandledException)
			{
				char	numAsText[20];
				sprintf(numAsText, "0x%08lx", de.u.Exception.ExceptionRecord.ExceptionCode);
				const char* prefix = "Error: Unexpected exception occurred. ";
				if (de.u.Exception.dwFirstChance)
				{
					// First chance exceptions are those that the debugger does not wish to
					// handle but that the application may decide to handle. If the application
					// does not handle this exception it will come back as a second chance
					// exception, and cause the application to terminate.
					prefix = "Warning: first chance exception occurred. ";
				}
				// Display an informative message, then wait for the process to
				// collapse (if it is a second chance exception).
				string message = prefix + string(exceptionName) + " - " + numAsText + "\n";
				if (pApp->m_bEndDebuggerThread)
					pApp->AddTrace(message.c_str());
				
				
				// Special case for breakpoint instructions. If these occur after the initial breakpoint
				// we want to report them, but we don't want to halt the process. Such breakpoints
				// generally imply heap corruption, but we might as well *try* to continue.
				// Also special case for invalid handle exception - let's ignore that also.
				const int acceptableExceptions[] =
				{
					EXCEPTION_BREAKPOINT,
					EXCEPTION_INVALID_HANDLE,
				};
				const int* acceptableEnd = acceptableExceptions + NUMELEMENTS(acceptableExceptions);
				if (find(acceptableExceptions, acceptableEnd, de.u.Exception.ExceptionRecord.ExceptionCode) != acceptableEnd)
					unhandledException = false;
			}
			break;
		}

		case OUTPUT_DEBUG_STRING_EVENT:
			{
				//get the debug string
				SIZE_T BytesRead = 0;
				SIZE_T BytesToRead = de.u.DebugString.nDebugStringLength;
				vector<char> buff;
				buff.resize(BytesToRead+1);
				if (ReadProcessMemory(pApp->m_DbgProcessInfo.hProcess, de.u.DebugString.lpDebugStringData, 
					                  &(buff[0]), BytesToRead, &BytesRead) )
				{
					buff[BytesToRead] = 0;
					const char* prefix = "ODS Received: ";
					buff.insert(buff.begin(), prefix, prefix + strlen(prefix));
					//send it to the trace window
					pApp->AddTrace(&(buff[0]));
				}
			}
			break;
		}
		if (unhandledException)
			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
		else
			ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
	} // while
	dprintf("\nExiting Debug Thread\n");

	CloseHandle(pApp->m_DbgProcessInfo.hThread);
	CloseHandle(pApp->m_DbgProcessInfo.hProcess);
	pApp->m_DbgProcessInfo.hThread = 0;
	pApp->m_DbgProcessInfo.hProcess = 0;

	return 1;

} // dbgThread

bool CDbgRemoteApp::StartDebugProcess(bool bAdvDebug)
{
	DWORD dwOpts = 0;
	if (bAdvDebug)
	{
		// If you set DEBUG_ONLY_THIS_PROCESS (and only that flag) when sub-processes
		// will not be debugged. This is simpler, but means that any OutputDebugString
		// messages from subprocesses (i.e.; applications spawned from Python with
		// system() or popen()) will be lost.
		//dwOpts = DEBUG_ONLY_THIS_PROCESS;
		// If you set DEBUG_PROCESS then sub-processes will also be debugged. This is
		// somewhat more complicated to set up, but it appears to be working nicely.
		dwOpts = DEBUG_PROCESS;
	}
	
	if (! CreateProcess(0, 
		(char *)m_DbgPathFile.c_str(), NULL, NULL, FALSE, dwOpts, NULL, 
		NULL, &m_DbgStartupInfo, &m_DbgProcessInfo) )
	{
		// Changed from LPVOID, to more accurately reflect its use.
		LPTSTR lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		// WARNING/BUGBUG - using MessageBox( NULL means you are creating a parentless
		// window, which can be dangerous. I have replace this with AfxMessageBox.
		// Also, the message is very cryptic because it makes no indication of *what*
		// file is missing.
		string result = string("Error running '") + m_DbgPathFile +
					string("'.\r\n") + lpMsgBuf;
		AfxMessageBox(result.c_str(), MB_OK | MB_ICONINFORMATION);
		//MessageBox( NULL, lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );
		
		return false;
	}

	return true;

} // StartDebugProcess

bool CDbgRemoteApp::StopDebugProcess()
{
	bool bTerminate = false;
	if (m_hDebuggerThread)
	{
		m_bEndDebuggerThread = true;
		if (  WaitForSingleObject(m_hDebuggerThread, DEBUG_EVENT_WAITMS * 40) == WAIT_TIMEOUT)
		{
			dprintf("CDbgRemoteApp::OnDebugStopdebugging - Terminating debugger thread\n");
			TerminateThread(m_hDebuggerThread, -1);
			bTerminate = true;
		}
		CloseHandle(m_hDebuggerThread);
		m_hDebuggerThread = 0;
	}

	if (m_uiMemCheckTimer)
	{
		//stop the timer that is checking the remote processes memory use
		::KillTimer(0, m_uiMemCheckTimer);
		m_uiMemCheckTimer = 0;
	}

	if (m_DbgProcessInfo.hThread)
	{
		BOOL bClose = CloseHandle(m_DbgProcessInfo.hThread);
		assert(bClose);
	}

	if (m_DbgProcessInfo.hProcess)
	{
		DWORD dwProcExit = 0;
		if (GetExitCodeProcess(m_DbgProcessInfo.hProcess, &dwProcExit))
		{
			if (STILL_ACTIVE == dwProcExit)
			{
				if (WaitForSingleObject(m_DbgProcessInfo.hProcess, 1500) == WAIT_TIMEOUT)
				{
					dprintf("Terminating remote process\n");
					AddTrace("\nHAP - terminating remote process (with prejudice)\n");
					TerminateProcess(m_DbgProcessInfo.hProcess, -1);
				}
			}
		}
		BOOL bClose = CloseHandle(m_DbgProcessInfo.hProcess);
		assert(bClose);
	}

	//clear process info struct
	memset(&m_DbgProcessInfo, 0, sizeof(m_DbgProcessInfo));
	
	return bTerminate;
}

void CDbgRemoteApp::OnDebugRundebugger() 
{	
	if (m_UserCfg.GetAutosaveOnDebug())
		OnFileSaveall();

	if (m_Project.ProjectName().size() == 0)
	{
		//no project make sure there is a document to debug
		POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
		if (!DocPos)
			return;
		
		CDbgRemoteDoc* pCurDoc = (CDbgRemoteDoc*)m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		if (!pCurDoc)
			return;
	}
	
	//load the run options
	UpdateDefaultRunOptions();

	if (m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_RELEASE)
		m_DbgPathFile = m_strReleaseClientFile;
	else if ((m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_DEBUG))
		m_DbgPathFile = m_strDebugClientFile;
	else
	{
		AfxMessageBox("No Debug Clients Available");
		return;
	}

	if (m_UserCfg.GetLastPort() != HE_DEBUGGER_PORTNUM)
	{
		char buff[32];
		sprintf(buff, " -p%d", m_UserCfg.GetLastPort());
		m_DbgPathFile += buff;
	}

	if (m_UserCfg.GetAdvancedDebugEnabled())
	{
		assert(m_hDebuggerThread == 0);
		m_bEndDebuggerThread = false;
		unsigned int ta = 0;
		m_hDebuggerThread = (HANDLE)_beginthreadex(0,0,AdvancedDebuggerThreadFn, (void*)this, 0, &ta);
		ConnectLocal();
	}
	else
	{
		if (StartDebugProcess())
			ConnectLocal();
	}

	m_uiMemCheckTimer = ::SetTimer(NULL, 1010, 250, NULL);

} // OnDebugRundebugger

void CDbgRemoteApp::OnUpdateDebugRundebugger(CCmdUI* pCmdUI) 
{
	OnUpdateDebugGo(pCmdUI);
}


void CDbgRemoteApp::OnDebugGo() 
{
	// if we are editing, go means run local
	if (m_CurState == eDBGSTATE_EDIT)
	{
		OnDebugRundebugger();
	}
	else if (m_CurState == eDBGSTATE_BREAK)
	{
		//if we are break, it means continue
		SetDebuggerState(eDBGSTATE_RUNNING);				
		m_DebugSocket.SendMessage(eMSG_STEP, "4");
	}
}

void CDbgRemoteApp::OnUpdateDebugGo(CCmdUI* pCmdUI) 
{
	if (m_Project.BootFile().size() > 0)
	{
		pCmdUI->Enable((m_CurState == eDBGSTATE_BREAK) || (m_CurState == eDBGSTATE_EDIT));
	}
	else
	{
		POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
		pCmdUI->Enable((m_CurState == eDBGSTATE_BREAK) || ((m_CurState == eDBGSTATE_EDIT) && (DocPos)));
	}
}

void CDbgRemoteApp::OnDebugExecute() 
{
	if (m_UserCfg.GetAutosaveOnDebug())
		OnFileSaveall();

	UpdateDefaultRunOptions();
	
	STARTUPINFO si = {0};
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = {0};
	
	CString CommandLine;
	
	//select the right version of python (debug or release)
	if (m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_RELEASE)
		CommandLine = cszPythonRel; //python.exe
	else
		CommandLine = cszPythonDbg; //python_d.exe

	CommandLine += " ";
	CommandLine += m_WorkingDir;
	CommandLine += m_RunScript;
	CommandLine += " ";
	CommandLine += m_RunParameters;

	// For some reason it works badly if we pass the executable name as the
	// first parameter. I don't know why.
	/*if (!CreateProcess(0, 
		(char *)(const char*)CommandLine, NULL, NULL, FALSE, 0, NULL, 
		m_WorkingDir, &si, &pi) )
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );
	}

	CloseHandle(pi.hThread); 
	CloseHandle(pi.hProcess);*/

	RunChildProcess(CommandLine, m_WorkingDir);

}

void CDbgRemoteApp::OnUpdateDebugExecute(CCmdUI* pCmdUI) 
{
	OnUpdateDebugGo(pCmdUI);
}



void CDbgRemoteApp::OnDebugBreaknow() 
{
	m_DebugSocket.SendMessage(eMSG_BREAKNOW, "");
}

void CDbgRemoteApp::OnUpdateDebugBreaknow(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_RUNNING);
}

void CDbgRemoteApp::OnDebugStepin() 
{
	SetDebuggerState(eDBGSTATE_RUNNING);				
	m_DebugSocket.SendMessage(eMSG_STEP, "2");
}

void CDbgRemoteApp::OnUpdateDebugStepin(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_BREAK);
}

void CDbgRemoteApp::OnDebugStepout() 
{
	SetDebuggerState(eDBGSTATE_RUNNING);				
	m_DebugSocket.SendMessage(eMSG_STEP, "3");
}

void CDbgRemoteApp::OnUpdateDebugStepout(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_BREAK);
}

void CDbgRemoteApp::OnDebugStepover() 
{
	SetDebuggerState(eDBGSTATE_RUNNING);				
	m_DebugSocket.SendMessage(eMSG_STEP, "1");
}

void CDbgRemoteApp::OnUpdateDebugStepover(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_BREAK);
}


void CDbgRemoteApp::OnUpdateDebugStopdebugging(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((m_CurState == eDBGSTATE_BREAK) || (m_CurState == eDBGSTATE_RUNNING) || 
		           (m_CurState == eDBGSTATE_CONNECTING) || (m_CurState == eDBGSTATE_EXCEPTION));
}

void CDbgRemoteApp::OnDebugStopdebugging() 
{
	m_DebugSocket.SendMessage(eMSG_BREAKNOW, "");
	m_DebugSocket.SendMessage(eMSG_EXIT, "");

	if (m_CurState == eDBGSTATE_CONNECTING)
	{
		SetDebuggerState(eDBGSTATE_EDIT);
		m_DebugSocket.Close();
	}

	StopDebugProcess();
}





void CDbgRemoteApp::SetDebuggerState(EDebugState NewState)
{
	if (m_CurState == NewState)
		return;

	if ((NewState == eDBGSTATE_EDIT) && (m_uiSockCheckTimer))
	{
		//no need to check the socket any more
		if (m_uiSockCheckTimer)
			::KillTimer(NULL, m_uiSockCheckTimer);
		m_uiSockCheckTimer = 0;
	}

	CMainFrame* pMain = (CMainFrame*)m_pMainWnd;
		

	//implement a simple fsm to handle the various changes the program will undergo
	//depending on the context
	switch (m_CurState)
	{
	case eDBGSTATE_INVALID:
		//we are just starting up - the only valid mode is edit
		assert(NewState == eDBGSTATE_EDIT);
		m_CurState = NewState;
		break;

	case eDBGSTATE_EDIT:
		assert(NewState == eDBGSTATE_CONNECTING);
		m_CurState = NewState;

		break;

	case eDBGSTATE_CONNECTING:
		assert((NewState == eDBGSTATE_INIT) || (NewState == eDBGSTATE_EDIT));
		//show connection status bar
		m_CurState = NewState;

		if ((NewState == eDBGSTATE_INIT) && (m_uiSockCheckTimer == 0))
			m_uiSockCheckTimer = ::SetTimer(NULL, 1010, 500, NULL);

		break;

	case eDBGSTATE_INIT:
		assert((NewState == eDBGSTATE_RUNNING) || (NewState == eDBGSTATE_EDIT));
		if ((NewState == eDBGSTATE_RUNNING) && (m_vecBreakpointList.size() > 0))
			SendBreakpoints();

		m_CurState = NewState;

		break;

	case eDBGSTATE_RUNNING:
		//hide connection status bar
		//show project, trace, locals, watch and call stack windows
		assert((NewState == eDBGSTATE_EDIT) || (NewState == eDBGSTATE_BREAK) || (NewState == eDBGSTATE_EXCEPTION));
		m_CurState = NewState;
		
		//clear the current line indicator
		if (m_pLastCurLineVw)
			m_pLastCurLineVw->SetCurrentLine(-1);

		
		break;

	case eDBGSTATE_BREAK:
		//assert((NewState == eDBGSTATE_RUNNING) || (NewState == eDBGSTATE_EDIT));
		m_CurState = NewState;
		break;

	case eDBGSTATE_EXCEPTION:
		if (NewState != eDBGSTATE_EDIT)
			dprintf("invalid state change %d to %d\n", m_CurState, NewState);

		m_CurState = NewState;
		break;

	default:
		assert(!"Invalid current state");
	}

	
	if (m_CurState == eDBGSTATE_EDIT)
	{
		pMain->SetBarState(eBARSTATE_EDIT);

		StopDebugProcess();
	}
	else
	{
		pMain->SetBarState(eBARSTATE_DEBUG);
	}

	//set the readonly state of the trace window
	CTraceViewBar* pTraceWnd = ((CMainFrame*)m_pMainWnd)->pGetTraceViewBar();
	pTraceWnd->SetReadOnly(m_CurState != eDBGSTATE_BREAK);

	if (m_CurState == eDBGSTATE_CONNECTING)
	{
		if (m_UserCfg.GetClearOutput())
			pTraceWnd->Clear();
	}

	if (m_CurState == eDBGSTATE_BREAK)
	{
		//grab focus
		if (m_UserCfg.GetSetFocusOnBreak())
		{
			if (!m_pMainWnd->SetForegroundWindow())
				dprintf("Unable to set foreground\n");

		}
	}
}


void CDbgRemoteApp::ModifyVariable(const char* cszVarName, const char* cszNewVal)
{
	if (m_CurState == eDBGSTATE_BREAK)
	{
		char* pBuff = new char[strlen(cszVarName) + strlen(cszNewVal) + 2];
		strcpy(pBuff, cszVarName);
		strcat(pBuff, "=");
		strcat(pBuff, cszNewVal);
		m_DebugSocket.SendMessage(eMSG_EXECUTE, pBuff);
		UpdateWatch();
		delete pBuff;
	}
}

void CDbgRemoteApp::RunStatement(const char* cszStmnt)
{
	if (m_CurState == eDBGSTATE_BREAK)
	{
		m_DebugSocket.SendMessage(eMSG_EXECUTE, cszStmnt);
		UpdateWatch();
	}
}
// Function added 10-19-02 Bruce Dickey
void CDbgRemoteApp::RemoveAllBreakpointsByFile(const char* cszFilePath)
{
	char_vector BpMsg;
	
	/*
   char LineNumBuff[12];
	sprintf(LineNumBuff, "%d,", nLineNumber);
	
	int i = 0;
	while(LineNumBuff[i])
   {
      BpMsg.push_back(LineNumBuff[i++]);
   }
   */

//	BpMsg.insert(BpMsg.end(), cszFilePath, cszFilePath + strlen(cszFilePath)+1);

   BOOL bFound;

   while (bFound = FindBreakpointInList(cszFilePath) )
//   while (bFound = FindBreakpointInList(&BpMsg[0]) )
   {
      // Found so remove it 
//	   RemoveBreakpointFromList(&BpMsg[0]);
	   RemoveBreakpointFromList(cszFilePath);

	   BpMsg.insert(BpMsg.begin(), ',');
	   BpMsg.insert(BpMsg.begin(), BREAKPT_CLEAR);
	   
	   m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, &BpMsg[0]);
   }
}


// Function added 10-18-02 Bruce Dickey
void CDbgRemoteApp::RemoveBreakpoint(const char* cszFilePath, int nLineNumber)
{
	char_vector BpMsg;
	
	char LineNumBuff[12];
	sprintf(LineNumBuff, "%d,", nLineNumber);
	
	int i = 0;
	while(LineNumBuff[i])
   {
      BpMsg.push_back(LineNumBuff[i++]);
   }

	BpMsg.insert(BpMsg.end(), cszFilePath, cszFilePath + strlen(cszFilePath)+1);

	BOOL bFound = FindBreakpointInList(&BpMsg[0]);
	if (bFound)                                  
	{
      // Found so remove it 
	   RemoveBreakpointFromList(&BpMsg[0]);

	   BpMsg.insert(BpMsg.begin(), ',');
	   BpMsg.insert(BpMsg.begin(), BREAKPT_CLEAR);
	
	   m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, &BpMsg[0]);
   }
}

// Function added 10-18-02 Bruce Dickey
void CDbgRemoteApp::AddBreakpoint(const char* cszFilePath, int nLineNumber)
{
	char_vector BpMsg;
	
	char LineNumBuff[12];
	sprintf(LineNumBuff, "%d,", nLineNumber);
	
	int i = 0;
	while(LineNumBuff[i])
		BpMsg.push_back(LineNumBuff[i++]);

	BpMsg.insert(BpMsg.end(), cszFilePath, cszFilePath + strlen(cszFilePath)+1);

	BOOL bFound = FindBreakpointInList(&BpMsg[0]);
	if (!bFound)
	{
      // Not found, so add to list

		//stored breakpoints are in the form "bp#, Line#, File[,expression]" - add the bp# first
		//storing the breakpoints allows us to set breakpoints before we connect to 
		//the debug engine
		m_vecBreakpointList.push_back('0');  // store invalid value
		m_vecBreakpointList.push_back(',');
		m_vecBreakpointList.insert(m_vecBreakpointList.end(), BpMsg.begin(), BpMsg.end());		

	   BpMsg.insert(BpMsg.begin(), ',');
	   BpMsg.insert(BpMsg.begin(), BREAKPT_SET);
	
   	m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, &BpMsg[0]);
	}
}
void CDbgRemoteApp::ToggleBreakpoint(const char* cszFilePath, int nLineNumber)
{
	char_vector BpMsg;
	
	char LineNumBuff[12];
	sprintf(LineNumBuff, "%d,", nLineNumber);
	
	int i = 0;
	while(LineNumBuff[i])
		BpMsg.push_back(LineNumBuff[i++]);

	BpMsg.insert(BpMsg.end(), cszFilePath, cszFilePath + strlen(cszFilePath)+1);

	//BOOL bClear = FindBreakpointInList(&BpMsg[0]);   // Bruce Dickey
	//if (bClear == false)                             // Bruce Dickey

	BOOL bFound = FindBreakpointInList(&BpMsg[0]);     // Bruce Dickey
	if (!bFound)                                       // Bruce Dickey
	{
      // Not found, so add to list

		//stored breakpoints are in the form "bp#, Line#, File[,expression]" - add the bp# first
		//storing the breakpoints allows us to set breakpoints before we connect to 
		//the debug engine
		m_vecBreakpointList.push_back('0');  // store invalid value
		m_vecBreakpointList.push_back(',');
		m_vecBreakpointList.insert(m_vecBreakpointList.end(), BpMsg.begin(), BpMsg.end());		
	}
   // Else block added 10-17-02 Bruce Dickey
   else
   {
      // Found so remove it (This is a toggle function). Doing this fixes the list
      // so that CDbgRemoteApp::SendBreakpoints() send the correct updated list.
	   RemoveBreakpointFromList(&BpMsg[0]);
   }

	BpMsg.insert(BpMsg.begin(), ',');
	BpMsg.insert(BpMsg.begin(), BREAKPT_TOGGLE);
	
	m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, &BpMsg[0]);
}
BOOL CDbgRemoteApp::FindBreakpointInList(const char* BpStr)
{
	char_vector::const_iterator pCurBp = m_vecBreakpointList.begin();

	while (pCurBp < m_vecBreakpointList.end())
	{
		if (strstr(&(*pCurBp), BpStr))
		{
			return true;
		}
		
		pCurBp += strlen(&(*pCurBp)) + 1;
	}

	return false;
}

// Function added 10-17-02 Bruce Dickey
void CDbgRemoteApp::RemoveBreakpointFromList(const char* BpStr)
{
	char_vector::iterator i, pCurBp = m_vecBreakpointList.begin();

	while (pCurBp < m_vecBreakpointList.end())
	{
		if (strstr(&(*pCurBp), BpStr))
		{
         while (1)
         {
            i = m_vecBreakpointList.erase(pCurBp);

            if ( *i == '\0')
            {
               m_vecBreakpointList.erase(pCurBp);  // erase the '\0' also
               break;
            }
         }
		}
		
		pCurBp += strlen(&(*pCurBp)) + 1;
	}
}

void CDbgRemoteApp::SendBreakpoints()
{
	char_vector::const_iterator pCurBp = m_vecBreakpointList.begin();

	while (pCurBp < m_vecBreakpointList.end())
	{
		//replace the breakpoint number with the BREAKPT_SET command
		char* pSendBp = strchr(&(*pCurBp), ',');
		if (pSendBp)
		{
			//move past the comma
			pSendBp--;
			*pSendBp = BREAKPT_SET;

			m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, (const char*)pSendBp);
		}

		pCurBp += strlen(&(*pCurBp)) + 1;
	}
}

void CDbgRemoteApp::SaveBreakpointList(char_vector& vecBreakpointList)
{
	m_vecBreakpointList = vecBreakpointList;

	UpdateBreakpointDisplay();

} //SaveBreakpointList

void CDbgRemoteApp::UpdateBreakpointDisplay()
{
	//signal all documents to update their breakpoint display.
	assert(m_pSourceCodeDocTemplate);

	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDbgRemoteDoc* pCurDoc = (CDbgRemoteDoc*)m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		while (pCurDoc)
		{
			pCurDoc->UpdateBreakpoints(&(m_vecBreakpointList[0]));

			if (DocPos)
				pCurDoc = (CDbgRemoteDoc*)m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}	
	}
} // UpdateBreakpointDisplay


void CDbgRemoteApp::UpdateModuleList(const char* cszModuleStr)
{
	if (m_pMainWnd)
	{
		CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
		assert(pProjWnd);
		assert(cszModuleStr);

		pProjWnd->UpdateModules(cszModuleStr);
	}
	
}

void CDbgRemoteApp::OnDebugBreakpointsClearall() 
{
	char ClearMsg[2];
	ClearMsg[0] = BREAKPT_CLEARALL;
	ClearMsg[1] = 0;
	m_vecBreakpointList.resize(0);
	m_vecBreakpointList.push_back(0);
	UpdateBreakpointDisplay();

	m_DebugSocket.SendMessage(eMSG_SETBREAKPOINT, ClearMsg);
}

void CDbgRemoteApp::OnUpdateDebugBreakpointsClearall(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_vecBreakpointList.size() > 0);
}

void CDbgRemoteApp::OnEditFindinfiles() 
{
	FindInFiles("");
}

void CDbgRemoteApp::FindInFiles(const char* cszFindText)
{
	if (m_pFindInFilesDlg == NULL)
	{
		CFileFindDlg* pDlg = new CFileFindDlg(m_pMainWnd);
		m_pFindInFilesDlg = pDlg;
	}

	if (::IsWindow(m_pFindInFilesDlg->m_hWnd) == FALSE)
	{
		CFileFindDlg* pDlg = (CFileFindDlg*)m_pFindInFilesDlg;
		pDlg->Create(m_pMainWnd);	
	}

	if (strlen(cszFindText))
		((CFileFindDlg*)m_pFindInFilesDlg)->m_FindText = cszFindText;

	m_pFindInFilesDlg->ShowWindow(SW_SHOW);
	m_pFindInFilesDlg->SetFocus();
	::SetWindowPos(m_pFindInFilesDlg->m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER);
}


LRESULT CDbgRemoteApp::OnSocketEvent(WPARAM, LPARAM)
{
	if (m_CurState != eDBGSTATE_CONNECTING)
		CheckRead();
	else //(m_CurState == eDBGSTATE_CONNECTING)
		CheckConnect();
	return 0;
}

void __cdecl CDbgRemoteApp::SocketCallbackFn()
{
	//post a message to the main thread to signal that data is waiting.
	::PostThreadMessage(dwAppMainThreadId, UM_SOCKETEVENT,0, 0);
}








void CDbgRemoteApp::HandleDbgMsgInit(EMessageType eMsgType, const char* MsgBuf)
{
	switch (eMsgType)
	{
		case eMSG_INIT:
			((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar()->SetClientInfo(MsgBuf);
			dprintf("INIT received\n");
			SendInit();
			AddTrace("\n\nINIT received - starting new run:\n\n");
			break;

		case eMSG_DONE:
			OnDbgMsgDone();
			break;
		default:
			dprintf("HandleDbgMsgInit - invalid msg received: %d\n", eMsgType);
	}
}

void CDbgRemoteApp::HandleDbgMsgRun(EMessageType eMsgType, const char* MsgBuf)
{
	switch (eMsgType)
	{
		case eMSG_DONE:
			OnDbgMsgDone();
			break;

		case eMSG_EXCEPTION:
				SetDebuggerState(eDBGSTATE_BREAK);
				dprintf("An exception has occured\n");
				AddTrace(&(m_vecMessageBuffer[0]));
				AddTrace("\n");
				break;

		case eMSG_TRACE:
				AddTrace(&(m_vecMessageBuffer[0]));
				break;

		case eMSG_OUTPUT:
				AddOutput(&(m_vecMessageBuffer[0]));
				break;

		case eMSG_BREAKPTLIST:
				SaveBreakpointList(m_vecMessageBuffer);
				break;

		case eMSG_BREAK:
				SetDebuggerState(eDBGSTATE_BREAK);
				ShowCallStack(m_vecMessageBuffer);
				ShowSourceLine(&(m_vecMessageBuffer[0]));
				UpdateWatch();
				break;

		default:
			dprintf("HandleDbgMsgRun - invalid msg received: %d\n", eMsgType);
	}
}

void CDbgRemoteApp::HandleDbgMsgBreak(EMessageType eMsgType, const char* MsgBuf)
{
	switch (eMsgType)
	{
		case eMSG_BREAKPTLIST:
			SaveBreakpointList(m_vecMessageBuffer);
			break;

		case eMSG_TRACE:
			AddTrace(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_OUTPUT:
			AddOutput(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_EVALUATE:
			UpdateWatchItem(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_LOCALS:
			UpdateLocals(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_GLOBALS:
			UpdateGlobals(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_MODULES:
			UpdateModuleList(&(m_vecMessageBuffer[0]));
			break;

		case eMSG_DONE:
			OnDbgMsgDone();
			break;

		default:
			dprintf("HandleDbgMsgBreak - invalid msg received: %d\n", eMsgType);
	}
}

void CDbgRemoteApp::HandleDbgMsgExcept(EMessageType eMsgType, const char* MsgBuf)
{
}

CDocument* CDbgRemoteApp::GetActiveDoc()
{
	CMDIChildWnd* pActiveWnd = ((CMDIFrameWnd*)m_pMainWnd)->MDIGetActive();
	if (pActiveWnd)
	{
		CView* pActiveVw = pActiveWnd->GetActiveView();
		if (pActiveVw)
		{
			return pActiveVw->GetDocument();
		}
	}

	return NULL;
}

void CDbgRemoteApp::UpdateDefaultRunOptions()
{
	if (m_Project.ProjectName().size() > 0)
	{
		m_RunScript = m_Project.BootFile().c_str();
		m_RunParameters = m_Project.RunOptions().c_str();
		
		m_WorkingDir = m_Project.GetWorkingDir().c_str();
		if (m_WorkingDir.GetLength() == 0)
			m_WorkingDir = m_Project.GetProjectPath().c_str();
	}
	
	if ((m_Project.ProjectName().size() == 0) || (m_RunScript.GetLength() == 0))
	{
		CDbgRemoteDoc* pCurDoc = (CDbgRemoteDoc*)GetActiveDoc();		
		if (pCurDoc)
		{
			m_RunScript = pCurDoc->GetFileName();
		}
	}
}


void CDbgRemoteApp::SendInit()
{
	assert(m_RunScript.GetLength() > 0);
	
	if (m_UserCfg.GetBreakOnFirstLine())
	{
		//set the option that indicates we want to break on the first
		//line of python code.
		m_DebugSocket.SendMessage(eMSG_SETOPTION, "BreakFirstLine=1");	
	}

	CUserCfg::EExceptionMode eMode = m_UserCfg.GetExceptionMode();
	//set the option that indicates how we want to handle exceptions
	string strOption = stringprintf("BreakOnException=%d", (int)eMode);
	m_DebugSocket.SendMessage(eMSG_SETOPTION, strOption.c_str());	

	
	vector<char> vecInitMsg;
	const char* pCur = (const char*)m_RunScript;
	assert(pCur);

	vecInitMsg.insert(vecInitMsg.end(), pCur, pCur + strlen(pCur) + 1);

	pCur = (const char*)m_WorkingDir;
	assert(pCur);
	vecInitMsg.insert(vecInitMsg.end(), pCur, pCur + strlen(pCur) + 1);

	pCur = (const char*)m_RunParameters;
	assert(pCur);
	vecInitMsg.insert(vecInitMsg.end(), pCur, pCur + strlen(pCur) + 1);
	
	m_DebugSocket.SendMessageSize(eMSG_INIT, &(vecInitMsg[0]), vecInitMsg.size() );

	SetDebuggerState(eDBGSTATE_RUNNING);
}





void CDbgRemoteApp::OnDbgMsgDone()
{
	m_vecMessageBuffer.resize(0);
	SetDebuggerState(eDBGSTATE_EDIT);
	UpdateLocals("");
	UpdateGlobals("");
	UpdateWatch();
	ShowCallStack(m_vecMessageBuffer);
	ShowSourceLine("");

	assert(m_pMainWnd);
	assert(((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar());
	
	((CMainFrame*)m_pMainWnd)->pGetConnectStatusBar()->SetStatus(eDBGSTATE_EDIT, "");
	m_DebugSocket.Close();
	
}

void CDbgRemoteApp::OnDebugRunOptions() 
{
	CRunOptionsDlg dlg;
	
	UpdateDefaultRunOptions();

	dlg.m_RunScript = m_RunScript;
	
	dlg.m_RunParameters = m_RunParameters;
	dlg.m_WorkingDir = m_WorkingDir;
	if (dlg.DoModal() == IDOK)
	{
		m_RunParameters = dlg.m_RunParameters;
		m_RunScript = dlg.m_RunScript;
		m_WorkingDir = dlg.m_WorkingDir;
	}
}

void CDbgRemoteApp::OnUpdateDebugRunOptions(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_CurState == eDBGSTATE_EDIT);
}

void CDbgRemoteApp::DisplayFileLine(const char* cszFilePath, int nLineNumber)
{
	if (strlen(cszFilePath))
	{
		CString temp;
		temp.Format("%s, %d", cszFilePath, nLineNumber);
		ShowSourceLine((const char*)temp);
	}
	else
	{
		//no filename was specified, assume its the active view.
		CMDIChildWnd *pChild = ((CMainFrame*)m_pMainWnd)->MDIGetActive();
		if (pChild)
		{
			CDbgRemoteView* pVw = (CDbgRemoteView*)pChild->GetActiveView();
			if (pVw)
				pVw->SetCurrentLine(nLineNumber);
		}
		
	}
}

void CDbgRemoteApp::AddWatchItem(const char* cszWatch)
{
	CWatchBar* pWatchWnd = ((CMainFrame*)m_pMainWnd)->pGetWatchBar();

	assert(pWatchWnd);
	pWatchWnd->AddWatchItem(cszWatch);
}

void CDbgRemoteApp::OnHelpPythonHelp() 
{
	const char* cszHelpFile = m_UserCfg.GetPythonHelpFile();
	if (strlen(cszHelpFile) > 0)
	{
		CString HelpPath;
			
		//if the help file has just a relative path, get the module path to make a full path
		if (strchr(cszHelpFile, ':') == 0)
		{
			char* pBuff = HelpPath.GetBufferSetLength(_MAX_PATH);
			if (GetModuleFileName(0, pBuff, _MAX_PATH) > 0)
			{
				char* slash = strrchr(pBuff, '\\');
				if (slash)
					slash[1] = 0;
			}
			HelpPath.ReleaseBuffer();
		}
		
		HelpPath += cszHelpFile;

#ifdef _PLATFORM_SDK
		::HtmlHelp(0, HelpPath, HH_DISPLAY_TOC, 0);
#else
		//just use shell execute to open the python help file
		SHELLEXECUTEINFO ShExecInfo;

		ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		ShExecInfo.fMask = NULL;
		ShExecInfo.hwnd = NULL;
		ShExecInfo.lpVerb = NULL;
		ShExecInfo.lpFile = (const char*)HelpPath;
		ShExecInfo.lpParameters = NULL;
		ShExecInfo.lpDirectory = NULL;
		ShExecInfo.nShow = SW_SHOW;
		ShExecInfo.hInstApp = NULL;

		ShellExecuteEx(&ShExecInfo);
#endif
	}
}

void CDbgRemoteApp::OnFileSaveall() 
{
	assert(m_pSourceCodeDocTemplate);

	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDocument* pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		while (pCurDoc)
		{
			if (pCurDoc->IsModified())
			{
				POSITION VwPos = pCurDoc->GetFirstViewPosition();
				assert(VwPos);
				CView* pVw = pCurDoc->GetNextView(VwPos);
				assert(pVw);

				pVw->SendMessage(WM_COMMAND, MAKELPARAM(ID_FILE_SAVE, 0), NULL);
			}

			if (DocPos)
				pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}
	}
}

void CDbgRemoteApp::OnToolsOptions() 
{
	COptionsDlg dlg;
	CUserCfg::EExceptionMode eOldMode = m_UserCfg.GetExceptionMode();
	dlg.DoModal();

	if ( (eOldMode != m_UserCfg.GetExceptionMode()) &&
		 (m_CurState != eDBGSTATE_EDIT) )
	{		
		//set the option that indicates how we want to handle exceptions
		string strOption = stringprintf("BreakOnException=%d", (int)m_UserCfg.GetExceptionMode());
		m_DebugSocket.SendMessage(eMSG_SETOPTION, strOption.c_str());	
	}
}

void CDbgRemoteApp::OnFileNewProject() 
{
	//const char* Filter = "Project Files (*.pdp)|*.pdp|All Files (*.*)|*.*||";
	//CFileDialog dlg(FALSE, "", NULL, OFN_HIDEREADONLY, Filter, m_pMainWnd);
	//dlg.m_ofn.lpstrTitle = "Project Filename";
	


	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_pMainWnd->m_hWnd; 
	ofn.lpstrFilter = "Hap Project files (*.hap)\0*.hap\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1; 

	//vector<char>filenm;
	//filenm.resize(MAX_PATH);
	//memset(&(filenm[0]), 0, filenm.size());
	char filenm[MAX_PATH+1] = {0};

	//vector<char> filetitle;
	//filetitle.resize(MAX_PATH);
	//memset(&(filetitle[0]), 0, filetitle.size());
	char filetitle[MAX_PATH+1] = {0};

	ofn.lpstrFile = filenm;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFileTitle = filetitle; 
	ofn.nMaxFileTitle = MAX_PATH; 
	ofn.lpstrInitialDir = 0; 
	ofn.lpstrTitle = "Project Filename"; 
	ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT; 
	ofn.lpstrDefExt = ""; 
	
	
	if (::GetOpenFileName(&ofn))
	//if (dlg.DoModal() == IDOK)
	{
		//string Filename = dlg.GetPathName();
		string Filename = ofn.lpstrFile;
		m_Project.New();
		m_Project.SetProjectName ("Untitled Project");
		
		m_Project.SaveToFile(Filename);

		CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
		assert(pProjWnd);
		pProjWnd->UpdateProjectDisplay(&m_Project);
		m_UserCfg.AddMruProject(ofn.lpstrFile);
	}
}

void CDbgRemoteApp::OnFileOpenProject() 
{
	//const char* Filter = "Project Files (*.pdp)|*.pdp|All Files (*.*)|*.*||";
	//CFileDialog dlg(TRUE, 0, NULL, OFN_HIDEREADONLY|OFN_FILEMUSTEXIST, Filter, m_pMainWnd);
	//dlg.m_ofn.lpstrTitle = "Project Filename";

	vector<char>filenm;
	filenm.resize(MAX_PATH);
	memset(&(filenm[0]), 0, filenm.size());

	vector<char> filetitle;
	filetitle.resize(MAX_PATH);
	memset(&(filetitle[0]), 0, filetitle.size());
	
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_pMainWnd->m_hWnd; 
	ofn.lpstrFilter = "Hap Project files (*.hap)\0*.hap\0Old Project Files (*.pdp)\0*.pdp\0All Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = &(filenm[0]); 
	ofn.nMaxFile = filenm.size();
	ofn.lpstrFileTitle = &(filetitle[0]); 
	ofn.nMaxFileTitle = filetitle.size(); 
	ofn.lpstrInitialDir = 0; 
	ofn.lpstrTitle = "Project Filename"; 
	ofn.Flags = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST; 
	ofn.lpstrDefExt = ""; 
	
	
	if (::GetOpenFileName(&ofn))
	//if (dlg.DoModal() == IDOK)
	{
		//CString Pathname = dlg.GetPathName();	
		OpenProjectFile(ofn.lpstrFile);

	}
}

void CDbgRemoteApp::OpenProjectFile(const char* cszProjFilename) 
{
	assert(cszProjFilename);
	string tagsName = cszProjFilename;
	
	m_Project.LoadFromFile(tagsName.c_str());

	CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
	assert(pProjWnd);

	pProjWnd->UpdateProjectDisplay(&m_Project);
	m_UserCfg.AddMruProject(tagsName.c_str());

	int fileEnd = tagsName.rfind('.');
	if (fileEnd != tagsName.npos)
	{
		tagsName = tagsName.substr(0, fileEnd);
		tagsName += ".tag";
	}
	m_BrowseTags.ClearTagInfo();
	m_BrowseTags.LoadTagsFile(tagsName.c_str());
}

void CDbgRemoteApp::SaveProjectFile()
{
	string FileNm = m_Project.GetProjectPath();
	FileNm += m_Project.GetProjectFile();
	
	if(	!m_Project.SaveToFile(FileNm) )
	{
		MessageBox( NULL, "Unable to save hap project file! Be sure the file is checked out before adding or removing files", "Warning", MB_OK | MB_ICONERROR );
	}
	//store the project filename actually saved (in case of conversion to new format)
	FileNm = m_Project.GetProjectPath();
	FileNm += m_Project.GetProjectFile();

	m_UserCfg.AddMruProject(FileNm.c_str());
}

void CDbgRemoteApp::OnAddFilesToProject() 
{
	const char* Filter = "Python Files (*.py)|*.py|All Files (*.*)|*.*||";
	CFileDialog dlg(TRUE, "", NULL, OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, Filter, m_pMainWnd);
	dlg.m_ofn.lpstrTitle = "Select Files to Add";

	const int MultiFileBufferSize = 2048;
	char FileBuffer[MultiFileBufferSize];
	memset(FileBuffer, 0, MultiFileBufferSize);
	dlg.m_ofn.lpstrFile = FileBuffer;
	dlg.m_ofn.nMaxFile = MultiFileBufferSize; 

	if (dlg.DoModal() == IDOK)
	{
		POSITION pos = dlg.GetStartPosition();
		while (pos)
		{
			string ProjFile = dlg.GetNextPathName(pos);
			m_Project.AddNewFile (ProjFile);
		}
		
		SaveProjectFile();
		
		CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
		pProjWnd->UpdateProjectDisplay(&m_Project);
	}
}


void CDbgRemoteApp::OnAddDocumentsToProject() 
{
	CDocSelectDlg dlg;
	assert(m_pSourceCodeDocTemplate);

	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDocument* pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		while (pCurDoc)
		{
			dlg.m_aDocNames.Add(pCurDoc->GetPathName());		

			if (DocPos)
				pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}
	}

	if (dlg.m_aDocNames.GetSize())
	{
		if (dlg.DoModal() == IDOK)
		{
			for (int i = 0; i < dlg.m_aDocNames.GetSize(); i++)
				m_Project.AddNewFile ((const char*)dlg.m_aDocNames[i]);

			SaveProjectFile();
			
			CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
			assert(pProjWnd);
			pProjWnd->UpdateProjectDisplay(&m_Project);
		}
	}
}
void CDbgRemoteApp::OnProjectSettings() 
{
	CProjectSettingsDlg dlg;
	string ProjName = m_Project.ProjectName();
	dlg.m_ProjectName = ProjName.c_str();
	dlg.m_RunParams = m_Project.RunOptions().c_str();
	dlg.m_RemotePath = m_Project.GetRemotePath().c_str();
	dlg.m_WorkingDir = m_Project.GetWorkingDir().c_str();

	if (dlg.DoModal() == IDOK)
	{
		m_Project.SetProjectName((const char*)dlg.m_ProjectName);
		m_Project.SetRunOptions((const char*)dlg.m_RunParams);
		m_Project.SetWorkingDir((const char*)dlg.m_WorkingDir);
		m_Project.SetRemotePath((const char*)dlg.m_RemotePath);

		SaveProjectFile();

		CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
		pProjWnd->UpdateProjectSettings(&m_Project);
	}
}


void CDbgRemoteApp::OnUpdateAddFilesToProject(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_Project.ProjectName().size() > 0);
}

void CDbgRemoteApp::OnUpdateProjectSettings(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_Project.ProjectName().size() > 0);
}
void CDbgRemoteApp::OnUpdateAddDocumentsToProject(CCmdUI* pCmdUI) 
{
	assert(m_pSourceCodeDocTemplate);

	pCmdUI->Enable( (m_Project.ProjectName().size() > 0) &&
		            (m_pSourceCodeDocTemplate->GetFirstDocPosition()) );
}

void CDbgRemoteApp::OnFileOpen()
{
	// prompt the user (with all document templates)
	CString newName;
	CFileDialog dlgFile(TRUE);

	CString title;
	VERIFY(title.LoadString(AFX_IDS_OPENFILE));

	dlgFile.m_ofn.Flags |= OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

	CString strFilter;
	CString strDefault;
	
	if (m_pSourceCodeDocTemplate)
	{
		CString FiltStr;
		CString ExtStr;
		m_pSourceCodeDocTemplate->GetDocString(FiltStr, CDocTemplate::filterName);
		m_pSourceCodeDocTemplate->GetDocString(ExtStr, CDocTemplate::filterExt);
		if ((FiltStr.GetLength() > 0) && (ExtStr.GetLength() > 0))
		{
			strFilter += FiltStr;
			strFilter += (TCHAR)'\0';   // next string please
			strFilter += "*";
			strFilter += ExtStr;
			strFilter += (TCHAR)'\0';   // next string please
		}
	}

	strFilter += "Text Files(*.txt)";
	strFilter += '\0';
	strFilter += "*.txt";
	strFilter += '\0';

	strFilter += "Batch Files(*.bat)";
	strFilter += '\0';
	strFilter += "*.bat";
	strFilter += '\0';

	strFilter += "XML Files(*.xml)";
	strFilter += '\0';
	strFilter += "*.xml";
	strFilter += '\0';

	strFilter += "IDL Files(*.idl)";
	strFilter += '\0';
	strFilter += "*.idl";
	strFilter += '\0';

	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	strFilter += allFilter;
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string
	dlgFile.m_ofn.nMaxCustFilter++;

	dlgFile.m_ofn.lpstrFilter = strFilter;
	dlgFile.m_ofn.lpstrTitle = title;
	dlgFile.m_ofn.lpstrFile = newName.GetBuffer(_MAX_PATH*10);

	dlgFile.m_ofn.Flags |= OFN_ALLOWMULTISELECT;

	int nResult = dlgFile.DoModal();
	newName.ReleaseBuffer();
	if (nResult != IDOK)
		return;

	POSITION pos = dlgFile.GetStartPosition();
	while (pos)
	{
		CString NewFile = dlgFile.GetNextPathName(pos);
		if (NewFile.GetLength() > 0)
			OpenDocumentFile(NewFile);
		else
			break;
	}

	// if returns NULL, the user has already been alerted
}


CDocument* CDbgRemoteApp::OpenDocumentFile(LPCTSTR lpszFileName) 
{
	assert(lpszFileName);

	//check to see if its a project file instead of a python file
	char * pExt = strrchr(lpszFileName, '.');
	if (pExt)
	{
		if ((strnicmp(pExt, ".pdp", strlen(".pdp")) == 0) || (strnicmp(pExt, ".hap", strlen(".hap")) == 0))
		{
			string projFile = lpszFileName;
			if (projFile[projFile.length() - 1] == '"')
				projFile[projFile.length() - 1] = 0;

			OpenProjectFile(projFile.c_str());

			return 0;
		}
	}

	string strFilename = lpszFileName;

	CDocument* pDoc = CWinApp::OpenDocumentFile(lpszFileName);
	if (pDoc)
	{
		POSITION pos = pDoc->GetFirstViewPosition();
		CView* pFirstView = pDoc->GetNextView( pos );
		if (pFirstView)
			pFirstView->SetFocus();
	}
	return pDoc;
}


void CDbgRemoteApp::OnUpdateFileMruProject1(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(0, pCmdUI);

	CMenu* pSubMenu = pCmdUI->m_pSubMenu;

	if (pSubMenu)
	{
		int MruCount = m_UserCfg.GetMruProjectCount();
		if (MruCount > 0)
		{
			
			for (int i = pSubMenu->GetMenuItemCount(); i < m_UserCfg.GetMruProjectCount(); i++)
			{

				const char* cszProjName = m_UserCfg.GetMruProject(i);
				assert(cszProjName);
				
				if ( ! pSubMenu->AppendMenu(MF_STRING, ID_FILE_MRU_PROJECT1+i, cszProjName) )
						dprintf("Append Failed\n");

			}
		}
	}
}

void CDbgRemoteApp::OnUpdateFileMruProject2(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(1, pCmdUI);
}
void CDbgRemoteApp::OnUpdateFileMruProject3(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(2, pCmdUI);
}
void CDbgRemoteApp::OnUpdateFileMruProject4(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(3, pCmdUI);
}
void CDbgRemoteApp::OnUpdateFileMruProject5(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(4, pCmdUI);
}
void CDbgRemoteApp::OnUpdateFileMruProject6(CCmdUI* pCmdUI)
{
	UpdateProjectMruMenu(5, pCmdUI);
}

void CDbgRemoteApp::UpdateProjectMruMenu(int nMruInx, CCmdUI* pCmdUI)
{
	const char* cszProjName = m_UserCfg.GetMruProject(nMruInx);
	if (cszProjName)
	{
		pCmdUI->SetText(cszProjName);
		pCmdUI->Enable(TRUE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CDbgRemoteApp::OnFileMruProject(UINT nID)
{
	int nMruInx = nID - ID_FILE_MRU_PROJECT1;
	assert(nMruInx < MAX_MRU_PROJECTS);
	if (nMruInx < MAX_MRU_PROJECTS)
	{
		const char* cszProjName = m_UserCfg.GetMruProject(nMruInx);
		if (cszProjName)
			OpenProjectFile(cszProjName);
	}
}


void CDbgRemoteApp::OnConfigureSourceControl() 
{
	vector<char>filenm;
	filenm.resize(MAX_PATH);
	strncpy(&(filenm[0]), m_UserCfg.GetSccDll(), MAX_PATH);

	vector<char> filetitle;
	filetitle.resize(MAX_PATH);
	memset(&(filetitle[0]), 0, filetitle.size());
	
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_pMainWnd->m_hWnd; 
	ofn.hInstance = 0; 
	ofn.lpstrFilter = "SCC DLL (*.dll)\0*.dll\0\0";
	ofn.lpstrCustomFilter = 0; 
	ofn.nMaxCustFilter = 0; 
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = &(filenm[0]); 
	ofn.nMaxFile = filenm.size();
	ofn.lpstrFileTitle = &(filetitle[0]); 
	ofn.nMaxFileTitle = filetitle.size(); 
	ofn.lpstrInitialDir = 0; 
	ofn.lpstrTitle = "Version Control Interface"; 
	ofn.Flags = OFN_FILEMUSTEXIST|OFN_HIDEREADONLY; 
	ofn.nFileOffset = 0; 
	ofn.nFileExtension = 0; 
	ofn.lpstrDefExt = ""; 
	ofn.lCustData = 0; 
	ofn.lpfnHook = 0; 
	ofn.lpTemplateName = 0;
	
	if (::GetOpenFileName(&ofn))
	{
		if (m_pSourceControl->Open(&(filenm[0])) )
		{
			m_UserCfg.SetSccDll(&(filenm[0]));
		}
		else
			AfxMessageBox("Invalid Scc Dll specified");
	}

} // OnConfigureSourceControl

void CDbgRemoteApp::OnUpdateConfigureSourceControl(CCmdUI* pCmdUI) 
{
}

void CDbgRemoteApp::OnSourceControlRefresh() 
{
	CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
	assert(pProjWnd);
	pProjWnd->UpdateProjectDisplay(&m_Project);
}

void CDbgRemoteApp::OnUpdateSourceControlRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pSourceControl->IsOpen());
}

void CDbgRemoteApp::OnWindowCloseAll() 
{
	CloseAllDocuments(FALSE);
}

void CDbgRemoteApp::OnToolsConfigure() 
{
	CToolConfigDlg dlg;
	dlg.DoModal();
}

void CDbgRemoteApp::OnToolsPythonInteractive() 
{
	CDbgRemoteDoc* pCurDoc = (CDbgRemoteDoc*)GetActiveDoc();		
	CString CurPath;
	const char* cszCurPath = 0;
	if (pCurDoc)
	{
		CurPath = pCurDoc->GetFileName();
		int PathEndInx = CurPath.ReverseFind('\\');
		if (PathEndInx > -1)
			CurPath = CurPath.Left(PathEndInx);		
		cszCurPath = (const char*) CurPath;
	}

	STARTUPINFO si = {0};

	PROCESS_INFORMATION pi = {0};
	
	CString CommandLine;
	if (m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_RELEASE)
		CommandLine = cszPythonRel; //"Python.exe"
	else 
		CommandLine = cszPythonDbg; //"Python_d.exe"

	
	// For some reason it works badly if we pass the executable name as the
	// first parameter. I don't know why.
	if (!CreateProcess(0, 
		(char *)(const char*)CommandLine, NULL, NULL, FALSE, 0, NULL, 
		cszCurPath, &si, &pi) )
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );
	}

	CloseHandle(pi.hThread); 
	CloseHandle(pi.hProcess);
}

void CDbgRemoteApp::OnUpdateConfigureToolsMenu(CCmdUI* pCmdUI) 
{
	CMenu* pSubMenu = pCmdUI->m_pSubMenu;

	if (pSubMenu)
	{
		int ToolCount = m_UserCfg.GetToolsCount();
		if (ToolCount > 0)
		{
			string toolMenuItem;
			int curToolInx = 0;
			for (; curToolInx < ToolCount; curToolInx++)
			{
				m_UserCfg.GetToolMenuStr(curToolInx, toolMenuItem);
				BOOL bMod = pSubMenu->ModifyMenu(ID_TOOLS_TOOLS_TOOLSMENU1+curToolInx, MF_BYCOMMAND|MF_STRING, 
					                             ID_TOOLS_TOOLS_TOOLSMENU1+curToolInx, toolMenuItem.c_str ());
				if (!bMod)
					break;
			}

			for (; curToolInx < ToolCount; curToolInx++)
			{				
				m_UserCfg.GetToolMenuStr(curToolInx, toolMenuItem);
				if ( ! pSubMenu->AppendMenu(MF_STRING, ID_TOOLS_TOOLS_TOOLSMENU1+curToolInx, toolMenuItem.c_str()) )
						dprintf("Append Failed\n");
			}
		}
	}
}


void CDbgRemoteApp::UpdateToolsMenu(CCmdUI* pCmdUI)
{
	int nToolInx = pCmdUI->m_nID - ID_TOOLS_TOOLS_TOOLSMENU1;

	string strToolName;
	if (m_UserCfg.GetToolMenuStr(nToolInx, strToolName) && strToolName.length())
	{
		pCmdUI->SetText(strToolName.c_str());
		pCmdUI->Enable(TRUE);
	}
	else
		pCmdUI->Enable(FALSE);
}

void CDbgRemoteApp::OnToolMenuTool(UINT nID)
{
	int nToolInx = nID - ID_TOOLS_TOOLS_TOOLSMENU1;
	assert(nToolInx < MAX_TOOLS);
	if (nToolInx < MAX_TOOLS)
	{
		RunTool(nToolInx);
	}
}

void CDbgRemoteApp::RunTool(int ToolInx)
{
	string Args = m_UserCfg.GetToolArgs(ToolInx);
	const char* cszArgs = 0;
	if (Args.length())
		cszArgs = Args.c_str();

	string Cmd = m_UserCfg.GetToolCmd(ToolInx);
	const char* cszCmd = 0;
	if (Cmd.length())
		cszCmd = Cmd.c_str();

	STARTUPINFO si = {0};
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	if (! CreateProcess(cszCmd, (char*)cszArgs, 0,  0, FALSE, 0, NULL, m_UserCfg.GetToolDir(ToolInx), &si, &pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );
	}
	CloseHandle(pi.hThread); 
	CloseHandle(pi.hProcess);
}


void CDbgRemoteApp::RunChildProcess(const char* cszCmdLine, const char* cszWorkDir)
{
	static CHapRedirect s_redir;
	
	if (s_redir.IsChildRunning())
		return;

	s_redir.StartChildProcess(cszCmdLine, TRUE);
}







void FormatGle(int err)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	dprintf("Err: %d - %s\n", err, (LPCTSTR)lpMsgBuf);
	// Free the buffer.
	LocalFree( lpMsgBuf );
}



/*
#define gle (GetLastError())
#define lenof(a) (sizeof(a) / sizeof((a)[0]))
#define MAXNAMELEN 1024 // max name length for found symbols
#define IMGSYMLEN ( sizeof IMAGEHLP_SYMBOL )
#define TTBUFLEN 65536 // for a temp buffer


void ShowStack( HANDLE hThread, HANDLE hProcess, CONTEXT& c )
{
	// normally, call ImageNtHeader() and use machine info from PE header
	DWORD imageType = IMAGE_FILE_MACHINE_I386;
	//HANDLE hProcess = GetCurrentProcess(); // hProcess normally comes from outside
	int frameNum; // counts walked frames
	DWORD offsetFromSymbol; // tells us how far from the symbol we were
	DWORD symOptions; // symbol handler settings
	IMAGEHLP_SYMBOL *pSym = (IMAGEHLP_SYMBOL *) malloc( IMGSYMLEN + MAXNAMELEN );
	char undName[MAXNAMELEN]; // undecorated name
	char undFullName[MAXNAMELEN]; // undecorated name with all shenanigans
	IMAGEHLP_MODULE Module;
	IMAGEHLP_LINE Line;
	std::string symSearchPath;
	char *tt = 0, *p;

	STACKFRAME s; // in/out stackframe
	memset( &s, '\0', sizeof s );

	// NOTE: normally, the exe directory and the current directory should be taken
	// from the target process. The current dir would be gotten through injection
	// of a remote thread; the exe fir through either ToolHelp32 or PSAPI.

	tt = new char[TTBUFLEN]; // this is a _sample_. you can do the error checking yourself.

	// build symbol search path from:
	symSearchPath = "";
	// current directory
	if ( GetCurrentDirectory( TTBUFLEN, tt ) )
		symSearchPath += tt + std::string( ";" );
	// dir with executable
	if ( GetModuleFileName( 0, tt, TTBUFLEN ) )
	{
		for ( p = tt + strlen( tt ) - 1; p >= tt; -- p )
		{
			// locate the rightmost path separator
			if ( *p == '\\' || *p == '/' || *p == ':' )
				break;
		}
		// if we found one, p is pointing at it; if not, tt only contains
		// an exe name (no path), and p points before its first byte
		if ( p != tt ) // path sep found?
		{
			if ( *p == ':' ) // we leave colons in place
				++ p;
			*p = '\0'; // eliminate the exe name and last path sep
			symSearchPath += tt + std::string( ";" );
		}
	}
	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariable( "_NT_ALTERNATE_SYMBOL_PATH", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );
	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariable( "SYSTEMROOT", tt, TTBUFLEN ) )
		symSearchPath += tt + std::string( ";" );

	if ( symSearchPath.size() > 0 ) // if we added anything, we have a trailing semicolon
		symSearchPath = symSearchPath.substr( 0, symSearchPath.size() - 1 );

	dprintf( "symbols path: %s\n", symSearchPath.c_str() );

	// why oh why does SymInitialize() want a writeable string?
	strncpy( tt, symSearchPath.c_str(), TTBUFLEN );
	tt[TTBUFLEN - 1] = '\0'; // if strncpy() overruns, it doesn't add the null terminator

	// init symbol handler stuff (SymInitialize())
	if ( ! SymInitialize( hProcess, tt, true ) )
	{
		dprintf( "SymInitialize(): gle = %lu\n", gle );
		goto cleanup;
	}

	// SymGetOptions()
	symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions &= ~SYMOPT_UNDNAME;
	SymSetOptions( symOptions ); // SymSetOptions()

	// Enumerate modules and tell imagehlp.dll about them.
	// On NT, this is not necessary, but it won't hurt.
//	enumAndLoadModuleSymbols( hProcess, GetCurrentProcessId() );

	// init STACKFRAME for first call
	// Notes: AddrModeFlat is just an assumption. I hate VDM debugging.
	// Notes: will have to be #ifdef-ed for Alphas; MIPSes are dead anyway,
	// and good riddance.
	s.AddrPC.Offset = c.Eip;
	s.AddrPC.Mode = AddrModeFlat;
	s.AddrFrame.Offset = c.Ebp;
	s.AddrFrame.Mode = AddrModeFlat;

	memset( pSym, '\0', IMGSYMLEN + MAXNAMELEN );
	pSym->SizeOfStruct = IMGSYMLEN;
	pSym->MaxNameLength = MAXNAMELEN;

	memset( &Line, '\0', sizeof Line );
	Line.SizeOfStruct = sizeof Line;

	memset( &Module, '\0', sizeof Module );
	Module.SizeOfStruct = sizeof Module;

	offsetFromSymbol = 0;

	int err;

	dprintf( "\n--# FV EIP----- RetAddr- FramePtr StackPtr Symbol\n" );
	for ( frameNum = 0; ; ++ frameNum )
	{
		// get next stack frame (StackWalk(), SymFunctionTableAccess(), SymGetModuleBase())
		// if this returns ERROR_INVALID_ADDRESS (487) or ERROR_NOACCESS (998), you can
		// assume that either you are done, or that the stack is so hosed that the next
		// deeper frame could not be found.
		if ( ! StackWalk( imageType, hProcess, hThread, &s, &c, NULL,
			SymFunctionTableAccess, SymGetModuleBase, NULL ) )
			break;

		// display its contents
		dprintf( "\n%3d %c%c %08lx %08lx %08lx %08lx ",
			frameNum, s.Far? 'F': '.', s.Virtual? 'V': '.',
			s.AddrPC.Offset, s.AddrReturn.Offset,
			s.AddrFrame.Offset, s.AddrStack.Offset );

		if ( s.AddrPC.Offset == 0 )
		{
			dprintf( "(-nosymbols- PC == 0)\n" );
		}
		else
		{ // we seem to have a valid PC
			// show procedure info (SymGetSymFromAddr())
			if ( ! SymGetSymFromAddr( hProcess, s.AddrPC.Offset, &offsetFromSymbol, pSym ) )
			{
				err = GetLastError();
				if ( err != 487 )
				{
					dprintf( "SymGetSymFromAddr(): gle = %lu", err );
					FormatGle(err);
				}
				
			}
			else
			{
				// UnDecorateSymbolName()
				UnDecorateSymbolName( pSym->Name, undName, MAXNAMELEN, UNDNAME_NAME_ONLY );
				UnDecorateSymbolName( pSym->Name, undFullName, MAXNAMELEN, UNDNAME_COMPLETE );
				dprintf( "%s", undName );
				if ( offsetFromSymbol != 0 )
					dprintf( " %+ld bytes", (long) offsetFromSymbol );
				dprintf("\n");
				dprintf( "    Sig:  %s\n", pSym->Name );
				dprintf( "    Decl: %s\n", undFullName );
			}

			// show line number info, NT5.0-method (SymGetLineFromAddr())
			//if ( pSGLFA != NULL )
			{ // yes, we have SymGetLineFromAddr()
				if ( ! SymGetLineFromAddr( hProcess, s.AddrPC.Offset, &offsetFromSymbol, &Line ) )
				{
					if ( gle != 487 )
						dprintf( "SymGetLineFromAddr(): gle = %lu\n", gle );
				}
				else
				{
					dprintf( "    Line: %s(%lu) %+ld bytes\n",
						Line.FileName, Line.LineNumber, offsetFromSymbol );
				}
			} // yes, we have SymGetLineFromAddr()

			// show module info (SymGetModuleInfo())
			if ( ! SymGetModuleInfo( hProcess, s.AddrPC.Offset, &Module ) )
			{
				dprintf( "SymGetModuleInfo): gle = %lu\n", gle );
			}
			else
			{ // got module info OK
				char ty[80];
				switch ( Module.SymType )
				{
				case SymNone:
					strcpy( ty, "-nosymbols-" );
					break;
				case SymCoff:
					strcpy( ty, "COFF" );
					break;
				case SymCv:
					strcpy( ty, "CV" );
					break;
				case SymPdb:
					strcpy( ty, "PDB" );
					break;
				case SymExport:
					strcpy( ty, "-exported-" );
					break;
				case SymDeferred:
					strcpy( ty, "-deferred-" );
					break;
				case SymSym:
					strcpy( ty, "SYM" );
					break;
				default:
					//_snprintf( ty, sizeof ty, "symtype=%ld", (long) Module.SymType );
					break;
				}

				dprintf( "    Mod:  %s[%s], base: %08lxh\n",
					Module.ModuleName, Module.ImageName, Module.BaseOfImage );
				dprintf( "    Sym:  type: %s, file: %s\n",
					ty, Module.LoadedImageName );
			} // got module info OK
		} // we seem to have a valid PC

		// no return address means no deeper stackframe
		if ( s.AddrReturn.Offset == 0 )
		{
			// avoid misunderstandings in the printf() following the loop
			SetLastError( 0 );
			break;
		}

	} // for ( frameNum )

	if ( gle != 0 )
		dprintf( "\nStackWalk(): gle = %lu\n", gle );

cleanup:
	//ResumeThread( hThread );
	// de-init symbol handler etc. (SymCleanup())
	SymCleanup( hProcess );
	//free( pSym );
	//delete [] tt;
}	
*/
void CDbgRemoteApp::OnDebugPyverDebug() 
{
	m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_DEBUG);
}

void CDbgRemoteApp::OnUpdateDebugPyverDebug(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((m_CurState == eDBGSTATE_EDIT) && (m_bDebugClientAvailable));
	pCmdUI->SetCheck(m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_DEBUG);
}

void CDbgRemoteApp::OnDebugPyverRel() 
{
	m_UserCfg.SetPythonVersion(CUserCfg::ePYVER_RELEASE);
}

void CDbgRemoteApp::OnUpdateDebugPyverRel(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((m_CurState == eDBGSTATE_EDIT) && (m_bReleaseClientAvailable));
	pCmdUI->SetCheck(m_UserCfg.GetPythonVersion() == CUserCfg::ePYVER_RELEASE);
}



void CDbgRemoteApp::OnFileCloseProject() 
{
	m_Project.New();
	CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
	assert(pProjWnd);
	pProjWnd->UpdateProjectDisplay(&m_Project);
}

void CDbgRemoteApp::OnUpdateFileCloseProject(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_Project.ProjectName().size() > 0);
}

void CDbgRemoteApp::ReloadDocumentFile(const char* cszFilename)
{
	CDocument* pDoc = GetDocumentFile(cszFilename);
	if (pDoc)
		pDoc->OnOpenDocument(cszFilename);
}

void CDbgRemoteApp::SaveDocumentFile(const char* cszFilename)
{
	CDocument* pDoc = GetDocumentFile(cszFilename);
	if (pDoc)
		pDoc->OnOpenDocument(cszFilename);
}



CDocument* CDbgRemoteApp::GetDocumentFile(const char* cszFilename)
{
	//clear the current indicator
	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDocument* pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		while (pCurDoc)
		{
			if (pCurDoc->GetPathName().Compare(cszFilename) == 0)
				return pCurDoc;

			if (DocPos)
				pCurDoc = m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}
	}
	return 0;
}

BOOL CDbgRemoteApp::PreTranslateMessage(MSG* pMsg) 
{
	//dispatch the custom messages and timer messages that are intended to be
	//handled by the app - vc7 doesn't like the CWinApp's message map to include
	//ON_MESSAGE entries...
	if (0 == pMsg->hwnd)
	{
		if (UM_SOCKETEVENT == pMsg->message)
		{
			OnSocketEvent(0,0);
		}
		else if (WM_TIMER == pMsg->message)
		{
			OnAppTimer(pMsg->wParam, pMsg->lParam);
		}
	}
	return CWinApp::PreTranslateMessage(pMsg);
}

EDebugState CDbgRemoteApp::GetDebugState()const
{
	return m_CurState;
}

void CDbgRemoteApp::OnFileModified()
{
	if (m_bIgnoreFileChanges)
		return;

	m_bIgnoreFileChanges = true;
	//check the project file
	if (m_Project.ProjectName().size() > 0)
	{
		CString projName = m_Project.ProjectName().c_str();				
		if (m_Project.HasChangedOnDisk())
		{
			if (m_Project.HasChanged())
			{
				dprintf("Project file has changed\n");
				CFileModifiedDlg dlg;
				dlg.m_Filename = projName;
				int ret = dlg.DoModal();
				if (ret == IDOK)
				{
					//reload the project file
					OpenProjectFile(projName);
				}
				else
				{
					//save the project file as	
					SaveProjectFile();
				}
			}
			else
			{
				OpenProjectFile(projName);
			}
		}
	}



	assert(m_pSourceCodeDocTemplate);
	
	//check the documents to see if any of them have changed on disk
	POSITION DocPos = m_pSourceCodeDocTemplate->GetFirstDocPosition();
	if (DocPos)
	{
		CDbgRemoteDoc* pCurDoc = (CDbgRemoteDoc*)m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
		CString DocName;
		while (pCurDoc)
		{
			if (pCurDoc->HasChangedOnDisk())
			{
				dprintf("Document file has changed %s\n", pCurDoc->GetFileName());
				if (pCurDoc->IsModified())
				{
					CFileModifiedDlg dlg;
					dlg.m_Filename = pCurDoc->GetFileName();
					int ret = dlg.DoModal();
					if (ret == IDOK)
					{
						//reload the project file
						pCurDoc->OnOpenDocument(pCurDoc->GetFileName());	
					}
					else
					{
						//save the document (as)
						pCurDoc->DoSave(0);	
					}
				}
				else
				{
					pCurDoc->OnOpenDocument(pCurDoc->GetFileName());
				}
				
				UpdateFileStatus(pCurDoc->GetFileName());
			}

			if (DocPos)
				pCurDoc = (CDbgRemoteDoc*)m_pSourceCodeDocTemplate->GetNextDoc(DocPos);
			else
				pCurDoc = NULL;
		}
	}
	m_bIgnoreFileChanges = false;
} // OnFileModified

void CDbgRemoteApp::IgnoreFileChanges(bool bIgnore)
{
	m_bIgnoreFileChanges = bIgnore;
}

void CDbgRemoteApp::UpdateFileStatus(const char* cszFilename)
{
	CProjectViewBar* pProjWnd = ((CMainFrame*)m_pMainWnd)->pGetProjectViewBar();
	assert(pProjWnd);

	//pProjWnd->UpdateProjectDisplay(&m_Project);
	pProjWnd->UpdateFileStatus(cszFilename, &m_Project);
}
BOOL CDbgRemoteApp::SaveAllModified()
{
	if (m_CurState != eDBGSTATE_EDIT)
	{
		if (AfxMessageBox("A Debugging session is in progress - Exit HAP?", MB_OKCANCEL) == IDCANCEL)
		{
			return FALSE;
		}
		StopDebugProcess();
	}
	return CWinApp::SaveAllModified();
}


const CTags* CDbgRemoteApp::GetBrowseTags()const
{
	return &m_BrowseTags;
}
void CDbgRemoteApp::OnProjectBuildtags()
{
	string tagFile = m_Project.GetProjectPath();
	tagFile += m_Project.GetProjectFile();

	int fileEnd = tagFile.rfind('.');
	if (fileEnd > -1)
		tagFile = tagFile.substr(0, fileEnd);
	tagFile += ".tag";

	
	//BUGBUG - this should ultimately go through the project and build the tags based on
	//the files used in the project.  For now we'll just use the project path and pythonpath
	string paths;
	paths += m_Project.GetProjectPath();
	paths += "*.py";

	DWORD dwSize = GetEnvironmentVariable("PYTHONPATH", 0, 0);
	if (dwSize > 0)
	{
		string buff;
		buff.resize(dwSize+1);
		GetEnvironmentVariable("PYTHONPATH", &(buff[0]), dwSize);
		
		int pathSep = buff.find(';');
		while (pathSep > -1)
		{
			paths += " ";
			paths += buff.substr(0, pathSep);
			paths += "\\*.py";
			
			buff = buff.substr(pathSep+1);
			pathSep = buff.find(';');
		}
		if (buff.length() > 0)
		{
			paths += " ";
			paths += buff;
			paths  += "\\*.py";
		}
	}
	
	if ( m_BrowseTags.BuildTagsFile(tagFile.c_str(), paths.c_str()) )
	{
		m_BrowseTags.ClearTagInfo();
		if (m_BrowseTags.LoadTagsFile(tagFile.c_str()) )
			AfxMessageBox("New Tags File Loaded");
		else
			AfxMessageBox("Error building tags file");		
	}
	else
		AfxMessageBox("Wait for tags file to complete timed out");
}

void CDbgRemoteApp::OnUpdateProjectBuildtags(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_Project.ProjectName().size() > 0);
}
