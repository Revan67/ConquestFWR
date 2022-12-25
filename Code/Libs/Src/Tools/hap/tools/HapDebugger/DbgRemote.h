//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemote.h
//
//  Purpose: main header file for the DbgRemote application
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
#if !defined(AFX_DBGREMOTE_H__DB66BED5_1512_440C_8660_8AE702DB01DC__INCLUDED_)
#define AFX_DBGREMOTE_H__DB66BED5_1512_440C_8660_8AE702DB01DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "DbgNetLib/HeDbgSocket.h"
#include "UserCfg.h"
#include "PdpManager.h"
#include "tags.h"

class CDbgRemoteView;
class CSourceControlInterface;


#define UM_SOCKETEVENT	WM_USER + 1010


/////////////////////////////////////////////////////////////////////////////
// CDbgRemoteApp:
// See DbgRemote.cpp for the implementation of this class
//

//the current state of the debugger
enum EDebugState
{
	eDBGSTATE_INVALID = -1,
	eDBGSTATE_EDIT,
	eDBGSTATE_CONNECTING,
	eDBGSTATE_INIT,
	eDBGSTATE_RUNNING,
	eDBGSTATE_BREAK,
	eDBGSTATE_EXCEPTION,
};


//flags used when excuting (or evaluating) client code in the debugger
enum EDebugExecuteClient
{
	eDBGCLIENT_VIEW = -1,		//the current view
	eDBGCLIENT_OUTPUT = -2,		//the output window (pane)
};

typedef vector<char>	char_vector;

class CDbgRemoteApp : public CWinApp
{
public:
	CDbgRemoteApp();
	void CheckConnect();
	void CheckRead();

	void ModifyVariable(const char* cszVarName, const char* cszNewVal);
	void RunStatement(const char* cszStmnt);
	void UpdateWatch(int nItem = -1);
	void ToggleBreakpoint(const char* cszFilePath, int nLineNumber);
	void DisplayFileLine(const char* cszFilePath, int nLineNumber);

	EDebugState GetDebugState()const;
	
	static void __cdecl SocketCallbackFn();

	CUserCfg* pGetUserCfg() {return &m_UserCfg;}
	const char* GetDebugScript()const {return m_RunScript;}

	void AddWatchItem(const char* cszWatch);
	void GetAutowatchItem(const char* cszWatch);
	void GetAutoComplete(const char* cszExpr, EDebugExecuteClient acClientId);
	void AddTrace(const char* cszTraceMsg);
	void AddOutput(const char* cszOutputMsg);

	const char_vector* GetBreakpointList()const {return &m_vecBreakpointList;}
	CPdpManager* pGetProject() {return &m_Project;}
	void SaveProjectFile();

	CSourceControlInterface* GetSci()	{return m_pSourceControl;}

	void ReloadDocumentFile(const char* cszFilename);
	void SaveDocumentFile(const char* cszFilename);
	CDocument* GetDocumentFile(const char* cszFilename);
	void OnFileModified();

	void IgnoreFileChanges(bool bIgnore);
	void FindInFiles(const char* cszFindText);
	void UpdateFileStatus(const char* cszFilename);

	const CTags* GetBrowseTags()const;

	void AddBreakpoint(const char* cszFilePath, int nLineNumber);     // Added 10-18-02 Bruce Dickey
	void RemoveAllBreakpointsByFile(const char* cszFilePath);         // Added 10-19-02 Bruce Dickey
	void RemoveBreakpoint(const char* cszFilePath, int nLineNumber);  // Added 10-18-02 Bruce Dickey

	void RunChildProcess(const char* cszCmdLine, const char* cszWorkDir);
	
	
protected:
	//socket connecting this to the debug process
	CHeDbgSocket m_DebugSocket;
	//BOOL m_bSocketConnected;
	
	//user options (saved to registry)
	CUserCfg m_UserCfg;	

	//current state of the debugger
	EDebugState m_CurState;

	//pointer to doc template for source code files
	CMultiDocTemplate* m_pSourceCodeDocTemplate;

	//pointer to the last view which displayed the current line indicator
	//used to clear the current line indicator
	CDbgRemoteView* m_pLastCurLineVw;
	
	//buffer for storing the latest message from the socket
	char_vector m_vecMessageBuffer;

	//buffer for storing the current list of breakpoints
	char_vector m_vecBreakpointList;

	//parameters related to the script being debugged:
	CString m_RunScript;			//name (and path) of the script
	CString m_RunParameters;		//parameters (space delimited)
	CString m_WorkingDir;			//working directory

	//timer used to check the socket state
	UINT m_uiSockCheckTimer;

	//pointer to the find in files (modeless) dialog
	CWnd* m_pFindInFilesDlg;
	
	//the project manager object
	CPdpManager m_Project;

	//pointer to the source control interface object (virt base class)
	CSourceControlInterface* m_pSourceControl;

	bool m_bDebugClientAvailable;
	string m_strDebugClientFile;

	bool m_bReleaseClientAvailable;
	string m_strReleaseClientFile;

	bool m_bIgnoreFileChanges;

	UINT m_uiMemCheckTimer;

	CTags m_BrowseTags;

protected:
	//the thread function that watched the debug process for debugging events of interest
	static unsigned int __stdcall AdvancedDebuggerThreadFn(void* pParam);

	//a flag to signal the debugger thread to end
	bool m_bEndDebuggerThread;
	
	//handle to the thread (in case it needs terminating)
	HANDLE m_hDebuggerThread;

	//process information about the debugged process
	STARTUPINFO m_DbgStartupInfo;
	PROCESS_INFORMATION m_DbgProcessInfo;
	
	//path and filename of the debugged process
	string m_DbgPathFile;

protected:
	//connect to a debug process on the local machine
	BOOL ConnectLocal();
	//connect to the process on a remote machine
	BOOL ConnectRemote(const char* szRemoteMachine);

	void SendInit();
	void SendBreakpoints();
	void ShowCallStack(char_vector& vecCallStack);
	void ShowSourceLine(const char* SourceInfo);
	void UpdateWatchItem(const char* cszWatchData);
	void UpdateLocals(const char* cszLocalStr);
	void UpdateGlobals(const char* cszGlobalStr);
	void UpdateModuleList(const char* cszModuleStr);
	void UpdateDefaultRunOptions();
	void SaveBreakpointList(char_vector& vecBreakpointList);
	void UpdateBreakpointDisplay();
	
	void SetDebuggerState(EDebugState NewState);
	void OnDbgMsgDone();

	BOOL FindBreakpointInList(const char* BpStr);
	void RemoveBreakpointFromList(const char* BpStr); // Added 10-17-02 Bruce Dickey


	
	CDocument* GetActiveDoc();
	
	void HandleDbgMsgInit(EMessageType eMsgType, const char* MsgBuf);
	void HandleDbgMsgRun(EMessageType eMsgType, const char* MsgBuf);
	void HandleDbgMsgBreak(EMessageType eMsgType, const char* MsgBuf);
	void HandleDbgMsgExcept(EMessageType eMsgType, const char* MsgBuf);


	void OpenProjectFile(const char* cszProjFilename);
	void UpdateProjectMruMenu(int nMruInx, CCmdUI* pCmdUI);
	//void UpdateToolsMenu(int nToolInx, CCmdUI* pCmdUI);
	void RunTool(int ToolInx);
	bool StartDebugProcess(bool bAdvDebug = false);
	bool StopDebugProcess();
	
	
	
public:	
	//{{AFX_VIRTUAL(CDbgRemoteApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL


	//{{AFX_MSG(CDbgRemoteApp)
	afx_msg void OnAppAbout();
	afx_msg void OnDebugConnect();
	afx_msg void OnUpdateDebugConnect(CCmdUI* pCmdUI);
	afx_msg void OnDebugGo();
	afx_msg void OnUpdateDebugGo(CCmdUI* pCmdUI);
	afx_msg void OnDebugBreaknow();
	afx_msg void OnUpdateDebugBreaknow(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDebugStopdebugging(CCmdUI* pCmdUI);
	afx_msg void OnDebugStopdebugging();
	afx_msg void OnDebugRundebugger();
	afx_msg void OnUpdateDebugRundebugger(CCmdUI* pCmdUI);
	afx_msg void OnDebugStepin();
	afx_msg void OnUpdateDebugStepin(CCmdUI* pCmdUI);
	afx_msg void OnDebugStepout();
	afx_msg void OnUpdateDebugStepout(CCmdUI* pCmdUI);
	afx_msg void OnDebugStepover();
	afx_msg void OnUpdateDebugStepover(CCmdUI* pCmdUI);
	afx_msg void OnDebugBreakpointsClearall();
	afx_msg void OnUpdateDebugBreakpointsClearall(CCmdUI* pCmdUI);
	afx_msg void OnEditFindinfiles();
	afx_msg void OnDebugRunOptions();
	afx_msg void OnUpdateDebugRunOptions(CCmdUI* pCmdUI);
	afx_msg void OnHelpPythonHelp();
	afx_msg void OnFileSaveall();
	afx_msg void OnToolsOptions();
	afx_msg void OnFileNewProject();
	afx_msg void OnFileOpenProject();
	afx_msg void OnAddFilesToProject();
	afx_msg void OnUpdateAddFilesToProject(CCmdUI* pCmdUI);
	afx_msg void OnProjectSettings();
	afx_msg void OnUpdateProjectSettings(CCmdUI* pCmdUI);
	afx_msg void OnAddDocumentsToProject();
	afx_msg void OnUpdateAddDocumentsToProject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject1(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject2(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject3(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject4(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject5(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileMruProject6(CCmdUI* pCmdUI);
	afx_msg void OnDebugExecute();
	afx_msg void OnUpdateDebugExecute(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConfigureSourceControl(CCmdUI* pCmdUI);
	afx_msg void OnConfigureSourceControl();
	afx_msg void OnSourceControlRefresh();
	afx_msg void OnUpdateSourceControlRefresh(CCmdUI* pCmdUI);
	afx_msg void OnFileOpen();
	afx_msg void OnWindowCloseAll();
	afx_msg void OnToolsPythonInteractive();
	afx_msg void OnDebugPyverDebug();
	afx_msg void OnUpdateDebugPyverDebug(CCmdUI* pCmdUI);
	afx_msg void OnDebugPyverRel();
	afx_msg void OnUpdateDebugPyverRel(CCmdUI* pCmdUI);
	afx_msg void OnToolsConfigure();
	afx_msg void OnFileCloseProject();
	afx_msg void OnUpdateFileCloseProject(CCmdUI* pCmdUI);
	afx_msg void OnUpdateConfigureToolsMenu(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT OnAppTimer(WPARAM, LPARAM);
	afx_msg LRESULT OnSocketEvent(WPARAM, LPARAM);
	void OnFileMruProject(UINT nID);
	void OnToolMenuTool(UINT nID);
	void UpdateToolsMenu(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
	virtual BOOL SaveAllModified();
	afx_msg void OnProjectBuildtags();
	afx_msg void OnUpdateProjectBuildtags(CCmdUI *pCmdUI);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBGREMOTE_H__DB66BED5_1512_440C_8660_8AE702DB01DC__INCLUDED_)
