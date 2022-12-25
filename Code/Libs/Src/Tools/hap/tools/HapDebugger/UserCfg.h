//////////////////////////////////////////////////////////////////////////////
//  File:    UserCfg.h
//
//  Purpose: This file defines a class which provides storage for user
//  configurable options - it stores and retrieves this information from the
//  registry.
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

// UserCfg.h: interface for the CUserCfg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USERCFG_H__B1CF4E43_5820_49C6_B9CB_A469BF93B9E6__INCLUDED_)
#define AFX_USERCFG_H__B1CF4E43_5820_49C6_B9CB_A469BF93B9E6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000




const int MAX_MRU_PROJECTS = 10;
const int MAX_TOOLS = 6;


const char kGeneralStyles[] = "General";
const int HAPSTYLE_PROJECTVW_PROJTREE = 0;
const int HAPSTYLE_PROJECTVW_MODLIST  = 1;
const int HAPSTYLE_STACKVW            = 2;
const int HAPSTYLE_TRACEVW_ERR        = 3;
const int HAPSTYLE_TRACEVW_OUT        = 4;
const int HAPSTYLE_TRACEVW_MRG        = 5;



class CUserCfg  
{
public:

	struct SEditorStyle
	{
		char Languagename[LF_FACESIZE];
		int  StyleId;
		char StyleName[LF_FACESIZE];

		char Facename[LF_FACESIZE];
		bool Bold;
		bool Italic;
		bool Underline;
		int  PointSize;

		COLORREF ForeCol;
		COLORREF BackCol;
	};

	CUserCfg();
	virtual ~CUserCfg();

	void SaveUserCfg();
	void LoadUserCfg();

	int nGetLastConnectType()const;
	void SetLastConnectType(int nLastType);

	const std::vector<string>* GetLastConnectNames()const;
	void AddLastConnectName(const char* cszNewName);

	int GetLastPort()const;
	void SetLastPort(int nPort);

	void AddFileFindWhat(const char* cszNewWhat);
	void AddFileFindTypes(const char* cszNewType);
	void AddFileFindPath(const char* cszNewPath);
	const std::vector<string>* GetFileFindWhat()const;
	const std::vector<string>* GetFileFindTypes()const;
	const std::vector<string>* GetFileFindPaths()const;

	void AddTextFindWhat(const char* cszNewWhat);
	void AddTextReplaceWith(const char* cszNewReplace);
	const std::vector<string>* GetTextFindWhat()const;
	const std::vector<string>* GetTextReplaceWith()const;

	bool GetAutosaveOnDebug()const {return true;}

	bool GetEditorStyle(const char* cszLanguage, unsigned int StyleId, SEditorStyle* pStyle)const;
	void SetEditorStyle(const char* cszLanguage, unsigned int StyleId, const SEditorStyle* pStyle);
	const SEditorStyle* GetEditorStyle(unsigned int StyleIndex)const;
	int GetEditorTabWidth()const;
	void SetEditorTabWidth(int TabWd);
	COLORREF GetEditorCaretColor()const;
	void SetEditorCaretColor(COLORREF NewCol);
	bool GetEditorUseTabs()const;
	void SetEditorUseTabs(bool bUse);

	void AddMruProject(const char* cszProjName);
	int GetMruProjectCount()const;
	const char* GetMruProject(unsigned int nProjInx)const;
	bool GetOpenLastProj()const;
	void SetOpenLastProj(bool bOpen);
	bool GetFoldOnOpen()const;
	void SetFoldOnOpen(bool bFold);
	const char* GetPythonHelpFile()const;
	void SetPythonHelpFile(const char* cszHelpFile);
	bool GetSetFocusOnBreak()const;
	void SetSetFocusOnBreak(bool bSet);

	int GetToolsCount()const;
	const char* GetToolName(unsigned int nToolInx)const;
	const char* GetToolCmd(unsigned int nToolInx)const;
	const char* GetToolDir(unsigned int nToolInx)const;
	const char* GetToolArgs(unsigned int nToolInx)const;
	bool GetToolMenuStr(unsigned int nToolInx, string& strMenuItem)const;
	void AddTool(const char* ToolName, const char* ToolCmd, const char* ToolDir, const char* ToolArgs);
	void DelTool(unsigned int nToolInx);

	bool GetAdvancedDebugEnabled()const;
	void SetAdvancedDebugEnabled(bool bEnable);

	bool GetOpenXmlFilesExternal()const;
	void SetOpenXmlFilesExternal(bool bOpenExternal);

	bool GetAutoWatch()const;
	void SetAutoWatch(bool bWatch);

	enum EPythonVer
	{
		ePYVER_INVALID = -1,
		ePYVER_RELEASE,
		ePYVER_DEBUG,

		ePYVER_NUMVERS
	};
	
	EPythonVer GetPythonVersion()const;
	void SetPythonVersion(EPythonVer ver);

	const char* GetSccDll()const;
	void SetSccDll(const char* cszSccDll);

	int GetOutputWndLastSelTab()const;
	void SetOutputWndLastSelTab(int nTab);

	enum EWatchPopupType
	{
		eWATCHPOP_OFF,
		eWATCHPOP_SAFE,
		eWATCHPOP_SCARY
	};

	EWatchPopupType GetWatchPopupType()const;
	void SetWatchPopupType(EWatchPopupType newType);

	bool GetBreakOnFirstLine()const;
	void SetBreakOnFirstLine(bool bBreak);

	enum EExceptionMode
	{
		eEXCEPTMODE_BREAKALL = 0,
		eEXCEPTMODE_BREAKUNHAND,
		eEXCEPTMODE_IGNORE,
	};
	EExceptionMode GetExceptionMode()const;
	void SetExceptionMode(EExceptionMode newMode);

	int GetLastDocOptsType()const;
	void SetLastDocOptsType(int newType);

	bool GetClearOutput()const;
	void SetClearOutput(bool bClear);

	bool GetMdiTabs()const;
	void SetMdiTabs();

private:
	//no copying
	CUserCfg(const CUserCfg&);
	CUserCfg& operator=(const CUserCfg&);

	static void StoreMultiSz(HKEY hRegKey, const char* cszKeyName, std::vector<string>& rvecMultiSz);
	static void LoadMultiSz(HKEY hRegKey, const char* cszKeyName, std::vector<string>& rvecMultiSz);
	static void AddStringToMultiSz(const char* cszNewString, std::vector<string>& rvecMultiSz);

	void StoreStyles(HKEY hRegKey);
	void LoadStyles(HKEY hRegKey);
	void LoadStyleDefaults();
	static void StoreStyle(HKEY hRegKey, const SEditorStyle* pStyle);

	void LoadMRUProjects(HKEY hRegKey);
	void StoreMRUProjects(HKEY hRegKey);

	void LoadTools(HKEY hRegKey);
	void StoreTools(HKEY hRegKey);
private:
	int m_nLastConnectType;
	std::vector<string> m_vecLastConnectNames;

	int m_nLastPort;

	std::vector<string> m_vecFileFindWhat;
	std::vector<string> m_vecFileFindTypes;
	std::vector<string> m_vecFileFindPath;

	std::vector<string> m_vecTextFind;
	std::vector<string> m_vecTextReplaceWith;


	int m_nEditorTabWidth;
	bool m_bUseTabs;
	COLORREF m_EditorCaretColor;
	std::vector<SEditorStyle> m_Styles;
	typedef std::vector<SEditorStyle>::iterator StyleItr;

	std::vector<string> m_vecMruProjects;
	bool m_bOpenLastProjAtStartup;
	bool m_bFoldDocOnOpen;
	bool m_bSetFocusOnBreak;

	bool m_bAdvancedDebugEnabled;
		
	std::string m_strPythonHelpFile;

	struct STool
	{
		string strToolName;
		string strToolCmd;
		string strToolDir;
		string strToolArgs;
	};
	std::vector<STool> m_vecTools;

	EPythonVer m_ePythonVersion;
	string m_strSccWrapperDll;
	bool m_bOpenXmlFilesExternal;
	bool m_bAutoWatch;

	//which tab of the output pane (error, output or both) was last selected
	int  m_OutputWndSelTab;

	//which document type was last selected in the editor doc type pull down (in the editor options page)
	int  m_lastDocOptsType;

	//enables/disables the watch popups
	EWatchPopupType m_eWatchPopupType;

	//true will make the debugger break when it enters the python script
	bool m_bBreakOnFirstLine;
	//true will break on any exception raised (even caught ones)
	//bool m_bBreakOnAnyException;
	EExceptionMode m_eExceptionHandlingMode;

	//true will clear the output window when starting debug.
	bool m_bClearOutput;
};


inline bool CUserCfg::GetSetFocusOnBreak()const
{
	return m_bSetFocusOnBreak;
}

inline void CUserCfg::SetSetFocusOnBreak(bool bSet)
{
	m_bSetFocusOnBreak = bSet;
}

inline bool CUserCfg::GetFoldOnOpen()const
{
	return m_bFoldDocOnOpen;
}

inline void CUserCfg::SetFoldOnOpen(bool bFold)
{
	m_bFoldDocOnOpen = bFold;
}

inline const std::vector<string>* CUserCfg::GetFileFindWhat()const
{
	return &m_vecFileFindWhat;
}

inline const std::vector<string>* CUserCfg::GetFileFindTypes()const
{
	return &m_vecFileFindTypes;
}

inline const std::vector<string>* CUserCfg::GetFileFindPaths()const
{
	return &m_vecFileFindPath;
}

inline int CUserCfg::GetLastPort()const
{
	return m_nLastPort;
}
inline void CUserCfg::SetLastPort(int nPort)
{
	m_nLastPort = nPort;
}

inline int CUserCfg::GetEditorTabWidth()const
{
	return m_nEditorTabWidth;
}

inline void CUserCfg::SetEditorTabWidth(int TabWd)
{
	m_nEditorTabWidth = TabWd;
}

inline COLORREF CUserCfg::GetEditorCaretColor()const
{
	return m_EditorCaretColor;
}

inline void CUserCfg::SetEditorCaretColor(COLORREF NewCol)
{
	m_EditorCaretColor = NewCol;
}

inline bool CUserCfg::GetOpenLastProj()const
{
	return m_bOpenLastProjAtStartup;
}
inline void CUserCfg::SetOpenLastProj(bool bOpen)
{
	m_bOpenLastProjAtStartup = bOpen;
}

inline const std::vector<string>* CUserCfg::GetTextFindWhat()const
{
	return &m_vecTextFind;
}

inline const std::vector<string>* CUserCfg::GetTextReplaceWith()const
{
	return &m_vecTextReplaceWith;
}

inline const char* CUserCfg::GetPythonHelpFile()const
{
	return m_strPythonHelpFile.c_str();
}

inline void CUserCfg::SetPythonHelpFile(const char* cszHelpFile)
{
	m_strPythonHelpFile = cszHelpFile;
}

inline bool CUserCfg::GetAdvancedDebugEnabled()const
{
	return m_bAdvancedDebugEnabled;
}

inline void CUserCfg::SetAdvancedDebugEnabled(bool bEnable)
{
	m_bAdvancedDebugEnabled = bEnable;
}

inline CUserCfg::EPythonVer CUserCfg::GetPythonVersion()const
{
	return m_ePythonVersion;
}

inline void CUserCfg::SetPythonVersion(CUserCfg::EPythonVer ver)
{
	m_ePythonVersion = ver;
}

inline const char* CUserCfg::GetSccDll()const
{
	return m_strSccWrapperDll.c_str();
}
inline void CUserCfg::SetSccDll(const char* cszSccDll)
{
	m_strSccWrapperDll = cszSccDll;
}

inline bool CUserCfg::GetOpenXmlFilesExternal()const
{
	return m_bOpenXmlFilesExternal;
}

inline void CUserCfg::SetOpenXmlFilesExternal(bool bOpenExternal)
{
	m_bOpenXmlFilesExternal = bOpenExternal;
}

inline bool CUserCfg::GetAutoWatch()const
{
	return m_bAutoWatch;
}
inline void CUserCfg::SetAutoWatch(bool bWatch)
{
	m_bAutoWatch = bWatch;
}

inline int CUserCfg::GetOutputWndLastSelTab()const
{
	return m_OutputWndSelTab;
}

inline void CUserCfg::SetOutputWndLastSelTab(int nTab)
{
	m_OutputWndSelTab = nTab;
}

inline CUserCfg::EWatchPopupType CUserCfg::GetWatchPopupType()const
{
	return m_eWatchPopupType;
}

inline void CUserCfg::SetWatchPopupType(CUserCfg::EWatchPopupType newType)
{
	m_eWatchPopupType = newType;
}

//inline bool CUserCfg::GetBreakOnAnyException()const
//{
//	return m_bBreakOnAnyException;
//}

//inline void CUserCfg::SetBreakOnAnyException(bool bBreak)
//{
//	m_bBreakOnAnyException = bBreak;/
//}

inline bool CUserCfg::GetBreakOnFirstLine()const
{
	return m_bBreakOnFirstLine;
}
inline void CUserCfg::SetBreakOnFirstLine(bool bBreak)
{
	m_bBreakOnFirstLine = bBreak;
}

inline CUserCfg::EExceptionMode CUserCfg::GetExceptionMode()const
{
	return m_eExceptionHandlingMode;
}

inline void CUserCfg::SetExceptionMode(CUserCfg::EExceptionMode newMode)
{
	m_eExceptionHandlingMode = newMode;
}


inline int CUserCfg::GetLastDocOptsType()const
{
	return m_lastDocOptsType;
}

inline void CUserCfg::SetLastDocOptsType(int newType)
{
	m_lastDocOptsType = newType;
}

inline bool CUserCfg::GetClearOutput()const
{
	return m_bClearOutput;
}

inline void CUserCfg::SetClearOutput(bool bClear)
{
	m_bClearOutput = bClear;
}

inline bool CUserCfg::GetEditorUseTabs()const
{
	return m_bUseTabs;
}

inline void CUserCfg::SetEditorUseTabs(bool bUse)
{
	m_bUseTabs = bUse;
}

inline bool CUserCfg::GetMdiTabs()const
{
	return true;
}
inline void CUserCfg::SetMdiTabs()
{
}

#endif // !defined(AFX_USERCFG_H__B1CF4E43_5820_49C6_B9CB_A469BF93B9E6__INCLUDED_)
