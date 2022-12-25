//////////////////////////////////////////////////////////////////////////////
//  File:    UserCfg.cpp
//
//  Purpose: This file implements a class which provides storage for user
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

// UserCfg.cpp: implementation of the CUserCfg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DbgRemote.h"
#include "UserCfg.h"
#include "scintilla\scintilla.h"
#include "scintilla\scilexer.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define REGKEY_PROJECTLIST  "Recent Project List"
#define REGKEY_TOOLSLIST    "Tools"
#define REGVAL_CONNECTNAMES	"Connect Names"
#define REGVAL_CONNECTTYPE  "Connect Type"
#define REGVAL_LASTPORT     "Port"

#define REGVAL_FILEFINDWHAT "File Find What"
#define REGVAL_FILEFINDTYPE "File Find Type"
#define REGVAL_FILEFINDPATH "File Find Path"
#define REGVAL_TEXTFINDWHAT "Text Find What"
#define REGVAL_TEXTREPLACE  "Text Replace With"
#define REGVAL_TABWIDTH		"Tab Width"
#define REGVAL_USETABS		"Use Tabs"
#define REGVAL_CARETCOLOR	"Caret Color"
#define REGVAL_OPENLASTPROJ "Open Last Project"
#define REGVAL_LASTPROJECT  "Project %d"
#define REGVAL_FOLDONOPEN   "Fold On Open"
#define REGVAL_PYHELPFILE	"Python Help"
#define REGVAL_ADVDEBUGOK	"Advanced Debug"
#define REGVAL_PYTHONVER	"Python Version"
#define REGVAL_TOOLCMD		"Tool Cmd"
#define REGVAL_TOOLDIR		"Tool Dir"
#define REGVAL_TOOLARG		"Tool Arg"
#define REGVAL_SCCDLL		"SCC Library"
#define REGVAL_XMLEXTERN	"OpenXml Extern"
#define REGVAL_AUTOWATCH	"Auto Watch"
#define REGVAL_LASTSELTAB   "Last Sel Tab"

#define REGVAL_WATCHPOP		"Watch Popup"
#define REGVAL_BREAKFIRSTLN "Break First Line"
//#define REGVAL_BREAKEXCEPT  "Break On Exception"
#define REGVAL_EXCEPTIONMODE "Exception Mode"
#define REGVAL_LASTDOCOPTS	"Last Doc Opts Type"
#define REGVAL_CLEAROUTPUT	"Clear Output"

CUserCfg::CUserCfg()
{
	m_nLastConnectType = 0;
	m_nLastPort = HE_DEBUGGER_PORTNUM;
	LoadStyleDefaults();
	m_nEditorTabWidth = 4;
	m_EditorCaretColor = RGB(0,0,0);
	m_bOpenLastProjAtStartup = false;
	m_bFoldDocOnOpen = false;
	m_strPythonHelpFile = "python.chm";
	m_bSetFocusOnBreak = true;
	m_bAdvancedDebugEnabled = false;
	m_ePythonVersion = ePYVER_RELEASE;
	m_strSccWrapperDll = "";
	m_bOpenXmlFilesExternal = false;
	m_bAutoWatch = true;
	m_OutputWndSelTab = 0;

	m_eWatchPopupType = eWATCHPOP_SAFE;
	m_bBreakOnFirstLine = false;
	//m_bBreakOnAnyException = false;
	m_eExceptionHandlingMode = eEXCEPTMODE_IGNORE;
	m_lastDocOptsType = 0;
	m_bClearOutput = true;
	m_bUseTabs = true;
}

CUserCfg::~CUserCfg()
{
	
}



void CUserCfg::SaveUserCfg()
{
	HKEY hRegKey = AfxGetApp()->GetAppRegistryKey();
	if (hRegKey)
	{
		StoreMultiSz(hRegKey, REGVAL_CONNECTNAMES, m_vecLastConnectNames);
		
		RegSetValueEx(hRegKey, REGVAL_CONNECTTYPE, 0, REG_DWORD, (unsigned char*)&m_nLastConnectType, sizeof(m_nLastConnectType));
		RegSetValueEx(hRegKey, REGVAL_LASTPORT, 0, REG_DWORD, (unsigned char*)&m_nLastPort, sizeof(m_nLastPort));
		RegSetValueEx(hRegKey, REGVAL_TABWIDTH, 0, REG_DWORD, (unsigned char*)&m_nEditorTabWidth, sizeof(m_nEditorTabWidth));
		RegSetValueEx(hRegKey, REGVAL_USETABS, 0, REG_DWORD, (unsigned char*)&m_bUseTabs, sizeof(m_bUseTabs));
		RegSetValueEx(hRegKey, REGVAL_CARETCOLOR, 0, REG_DWORD, (unsigned char*)&m_EditorCaretColor, sizeof(m_EditorCaretColor));
		RegSetValueEx(hRegKey, REGVAL_OPENLASTPROJ, 0, REG_DWORD, (unsigned char*)&m_bOpenLastProjAtStartup, sizeof(m_bOpenLastProjAtStartup));
		RegSetValueEx(hRegKey, REGVAL_FOLDONOPEN, 0, REG_DWORD, (unsigned char*)&m_bFoldDocOnOpen, sizeof(m_bFoldDocOnOpen));
		RegSetValueEx(hRegKey, REGVAL_ADVDEBUGOK, 0, REG_DWORD, (unsigned char*)&m_bAdvancedDebugEnabled, sizeof(m_bAdvancedDebugEnabled));
		RegSetValueEx(hRegKey, REGVAL_PYTHONVER, 0, REG_DWORD, (unsigned char*)&m_ePythonVersion, sizeof(m_ePythonVersion));
		RegSetValueEx(hRegKey, REGVAL_XMLEXTERN, 0, REG_DWORD, (unsigned char*)&m_bOpenXmlFilesExternal, sizeof(m_bOpenXmlFilesExternal));
		RegSetValueEx(hRegKey, REGVAL_AUTOWATCH, 0, REG_DWORD, (unsigned char*)&m_bAutoWatch, sizeof(m_bAutoWatch));
		RegSetValueEx(hRegKey, REGVAL_LASTSELTAB, 0, REG_DWORD, (unsigned char*)&m_OutputWndSelTab, sizeof(m_OutputWndSelTab));
		RegSetValueEx(hRegKey, REGVAL_WATCHPOP, 0, REG_DWORD, (unsigned char*)&m_eWatchPopupType, sizeof(m_eWatchPopupType));
		RegSetValueEx(hRegKey, REGVAL_BREAKFIRSTLN, 0, REG_DWORD, (unsigned char*)&m_bBreakOnFirstLine, sizeof(m_bBreakOnFirstLine));
		RegSetValueEx(hRegKey, REGVAL_EXCEPTIONMODE, 0, REG_DWORD, (unsigned char*)&m_eExceptionHandlingMode, sizeof(m_eExceptionHandlingMode));
		RegSetValueEx(hRegKey, REGVAL_EXCEPTIONMODE, 0, REG_DWORD, (unsigned char*)&m_eExceptionHandlingMode, sizeof(m_eExceptionHandlingMode));
		RegSetValueEx(hRegKey, REGVAL_LASTDOCOPTS, 0, REG_DWORD, (unsigned char*)&m_lastDocOptsType, sizeof(m_lastDocOptsType));
		RegSetValueEx(hRegKey, REGVAL_CLEAROUTPUT, 0, REG_DWORD, (unsigned char*)&m_bClearOutput, sizeof(m_bClearOutput));
		
		

		const char* szHelpFile = m_strPythonHelpFile.c_str();
		RegSetValueEx(hRegKey, REGVAL_PYHELPFILE, 0, REG_SZ, (unsigned char*)szHelpFile, strlen(szHelpFile)+1);

		const char* szSccDll = m_strSccWrapperDll.c_str();
		RegSetValueEx(hRegKey, REGVAL_SCCDLL, 0, REG_SZ, (unsigned char*)szSccDll, strlen(szSccDll)+1);

		StoreMultiSz(hRegKey, REGVAL_FILEFINDWHAT, m_vecFileFindWhat);
		StoreMultiSz(hRegKey, REGVAL_FILEFINDTYPE, m_vecFileFindTypes);
		StoreMultiSz(hRegKey, REGVAL_FILEFINDPATH, m_vecFileFindPath);

		StoreMultiSz(hRegKey, REGVAL_TEXTFINDWHAT, m_vecTextFind);
		StoreMultiSz(hRegKey, REGVAL_TEXTREPLACE, m_vecTextReplaceWith);
		
		StoreStyles(hRegKey);
		StoreMRUProjects(hRegKey);
		StoreTools(hRegKey);

		RegCloseKey(hRegKey);
	}
}

void CUserCfg::LoadUserCfg()
{
	HKEY hRegKey = AfxGetApp()->GetAppRegistryKey();
	if (hRegKey)
	{
		LoadMultiSz(hRegKey, REGVAL_CONNECTNAMES, m_vecLastConnectNames);

		DWORD dwSize = sizeof(m_nLastConnectType);
		DWORD dwType = REG_DWORD;

		RegQueryValueEx(hRegKey, REGVAL_CONNECTTYPE, 0, &dwType, (unsigned char*)&m_nLastConnectType, &dwSize);
		
		dwSize = sizeof(m_nLastPort);
		RegQueryValueEx(hRegKey, REGVAL_LASTPORT, 0, &dwType, (unsigned char*)&m_nLastPort, &dwSize);

		dwSize = sizeof(m_bUseTabs);
		RegQueryValueEx(hRegKey, REGVAL_TABWIDTH, 0, &dwType, (unsigned char*)&m_nEditorTabWidth, &dwSize);

		dwSize = sizeof(m_nEditorTabWidth);
		RegQueryValueEx(hRegKey, REGVAL_USETABS, 0, &dwType, (unsigned char*)&m_bUseTabs, &dwSize);
		
		dwSize = sizeof(m_EditorCaretColor);
		RegQueryValueEx(hRegKey, REGVAL_CARETCOLOR, 0, &dwType, (unsigned char*)&m_EditorCaretColor, &dwSize);

		dwSize = sizeof(m_bOpenLastProjAtStartup);
		RegQueryValueEx(hRegKey, REGVAL_OPENLASTPROJ, 0, &dwType, (unsigned char*)&m_bOpenLastProjAtStartup, &dwSize);

		dwSize = sizeof(m_bAdvancedDebugEnabled);
		RegQueryValueEx(hRegKey, REGVAL_FOLDONOPEN, 0, &dwType, (unsigned char*)&m_bFoldDocOnOpen, &dwSize);

		dwSize = sizeof(m_bFoldDocOnOpen);
		RegQueryValueEx(hRegKey, REGVAL_ADVDEBUGOK, 0, &dwType, (unsigned char*)&m_bAdvancedDebugEnabled, &dwSize);

		dwSize = sizeof(m_ePythonVersion);
		RegQueryValueEx(hRegKey, REGVAL_PYTHONVER, 0, &dwType, (unsigned char*)&m_ePythonVersion, &dwSize);

		dwSize = sizeof(m_bOpenXmlFilesExternal);
		RegQueryValueEx(hRegKey, REGVAL_XMLEXTERN, 0, &dwType, (unsigned char*)&m_bOpenXmlFilesExternal, &dwSize);

		dwSize = sizeof(m_bAutoWatch);
		RegQueryValueEx(hRegKey, REGVAL_AUTOWATCH, 0, &dwType, (unsigned char*)&m_bAutoWatch, &dwSize);

		dwSize = sizeof(m_OutputWndSelTab);
		RegQueryValueEx(hRegKey, REGVAL_LASTSELTAB, 0, &dwType, (unsigned char*)&m_OutputWndSelTab, &dwSize);

		dwSize = sizeof(m_eWatchPopupType);
		RegQueryValueEx(hRegKey, REGVAL_WATCHPOP, 0, &dwType, (unsigned char*)&m_eWatchPopupType, &dwSize);
		
		dwSize = sizeof(m_bBreakOnFirstLine);
		RegQueryValueEx(hRegKey, REGVAL_BREAKFIRSTLN, 0, &dwType, (unsigned char*)&m_bBreakOnFirstLine, &dwSize);
		
		dwSize = sizeof(m_eExceptionHandlingMode);
		RegQueryValueEx(hRegKey, REGVAL_EXCEPTIONMODE, 0, &dwType, (unsigned char*)&m_eExceptionHandlingMode, &dwSize);

		dwSize = sizeof(m_lastDocOptsType);
		RegQueryValueEx(hRegKey, REGVAL_LASTDOCOPTS, 0, &dwType, (unsigned char*)&m_lastDocOptsType, &dwSize);

		dwSize = sizeof(m_bClearOutput);
		RegQueryValueEx(hRegKey, REGVAL_CLEAROUTPUT, 0, &dwType, (unsigned char*)&m_bClearOutput, &dwSize);


		dwSize = MAX_PATH;
		char regStrBuff[MAX_PATH+1];
		if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, REGVAL_PYHELPFILE, 0, &dwType, (unsigned char*)regStrBuff, &dwSize))			                 
			m_strPythonHelpFile = regStrBuff;

		dwSize = MAX_PATH;
		if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, REGVAL_SCCDLL, 0, &dwType, (unsigned char*)regStrBuff, &dwSize))
			m_strSccWrapperDll = regStrBuff;

		LoadMultiSz(hRegKey, REGVAL_FILEFINDWHAT, m_vecFileFindWhat);
		LoadMultiSz(hRegKey, REGVAL_FILEFINDTYPE, m_vecFileFindTypes);
		LoadMultiSz(hRegKey, REGVAL_FILEFINDPATH, m_vecFileFindPath);

		LoadMultiSz(hRegKey, REGVAL_TEXTFINDWHAT, m_vecTextFind);
		LoadMultiSz(hRegKey, REGVAL_TEXTREPLACE, m_vecTextReplaceWith);
		

		LoadStyles(hRegKey);
		LoadMRUProjects(hRegKey);
		LoadTools(hRegKey);

		RegCloseKey(hRegKey);
	}
}

int CUserCfg::nGetLastConnectType()const
{
	return m_nLastConnectType;
}

void CUserCfg::SetLastConnectType(int nLastType)
{
	m_nLastConnectType = nLastType;
}

const vector<string>* CUserCfg::GetLastConnectNames()const
{
	return &m_vecLastConnectNames;
}


void CUserCfg::AddLastConnectName(const char* cszNewName)
{
	AddStringToMultiSz(cszNewName, m_vecLastConnectNames);

} // AddLastConnectName

void CUserCfg::AddFileFindWhat(const char* cszNewWhat)
{
	AddStringToMultiSz(cszNewWhat, m_vecFileFindWhat);
}

void CUserCfg::AddFileFindTypes(const char* cszNewType)
{
	AddStringToMultiSz(cszNewType, m_vecFileFindTypes);
}

void CUserCfg::AddFileFindPath(const char* cszNewPath)
{
	AddStringToMultiSz(cszNewPath, m_vecFileFindPath);
}

void CUserCfg::AddTextFindWhat(const char* cszNewWhat)
{
	AddStringToMultiSz(cszNewWhat, m_vecTextFind);
}

void CUserCfg::AddTextReplaceWith(const char* cszNewReplace)
{
	AddStringToMultiSz(cszNewReplace, m_vecTextReplaceWith);
}


void CUserCfg::StoreMultiSz(HKEY hRegKey, const char* cszKeyName, vector<string>& rvecMultiString)
{
	assert(cszKeyName);
	
	if (rvecMultiString.size() > 0)
	{
		//store the vector of strings into a single vector corresponding to the multisz format(str\0str\0\0)
		vector<char> vecMultiSz;
		for (vector<char>::size_type i = 0; i < rvecMultiString.size(); i++)
		{
			assert(rvecMultiString[i].size() > 0);
			vecMultiSz.insert(vecMultiSz.end(), rvecMultiString[i].begin(), rvecMultiString[i].end());
			vecMultiSz.push_back(0);
		}
		//add the second last terminating null
		vecMultiSz.push_back(0);

		//store the multisz into the registry
		const unsigned char* pMultiSz = (const unsigned char*)&(vecMultiSz[0]);
		RegSetValueEx(hRegKey, cszKeyName, 0, REG_MULTI_SZ, pMultiSz, vecMultiSz.size());
	}
	else
		RegDeleteValue(hRegKey, cszKeyName);

} // StoreMultiSz

void CUserCfg::LoadMultiSz(HKEY hRegKey, const char* cszKeyName, vector<string>& rvecMultiString)
{
	assert(cszKeyName);
	
	//get the size needed to store the value
	DWORD dwSize = 0;
	DWORD dwType = 0;
	RegQueryValueEx(hRegKey, cszKeyName, 0, &dwType, NULL, &dwSize);		
	
	//resize the storage
	vector<char> vecMultiSz;
	vecMultiSz.resize(dwSize);

	rvecMultiString.resize(0);
	
	if (dwSize > 0)
	{
		//load the value
		RegQueryValueEx(hRegKey, cszKeyName, 0, &dwType, (unsigned char*)&(vecMultiSz[0]), &dwSize);

		//store each null delimited string into the vector of strings
		char* pCur = &vecMultiSz[0];
		while (pCur && strlen(pCur))
		{
			rvecMultiString.push_back(string(pCur));

			pCur += strlen(pCur)+1;
		}
	}
} // LoadMultiSz

void CUserCfg::AddStringToMultiSz(const char* cszNewString, vector<string>& rvecMultiString)
{
	assert(cszNewString);

	if (strlen(cszNewString) == 0)
		return;
	
	
	//look through the existing vector, if the new string is in the array, remove it
	for (vector<string>::size_type i = 0; i < rvecMultiString.size(); i++)
	{
		if (rvecMultiString[i].compare(cszNewString) == 0)
		{
			rvecMultiString.erase(rvecMultiString.begin() + i);
			break;
		}
	}

	//if there are more than 10 strings, clip the first one
	if (rvecMultiString.size() > 10)
	{
		rvecMultiString.erase(rvecMultiString.begin());
	}
	
	//add the new string to the end
	rvecMultiString.push_back(cszNewString);

} // AddStringToMultiSz




bool CUserCfg::GetEditorStyle(const char* cszLanguage, unsigned int StyleId, SEditorStyle* pStyle)const
{
	vector<SEditorStyle>::const_iterator pCur;
	for (pCur = m_Styles.begin(); pCur != m_Styles.end(); pCur++)
	{
		if ((strcmp(cszLanguage, pCur->Languagename) == 0) && (pCur->StyleId == StyleId))
		{
			*pStyle = *pCur;
			return true;
		}
	}
	return FALSE;
}

void CUserCfg::SetEditorStyle(const char* cszLanguage, unsigned int StyleId, const SEditorStyle* pStyle)
{
	StyleItr pCur;
	for (pCur = m_Styles.begin(); pCur != m_Styles.end(); pCur++)
	{
		if ((strcmp(cszLanguage, pCur->Languagename) == 0) && (pCur->StyleId == StyleId))
		{
			*pCur = *pStyle;
			return;
		}
	}
	m_Styles.push_back(*pStyle);
}

void CUserCfg::StoreStyles(HKEY hRegKey)
{
	HKEY hStyleKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, "Styles", 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hStyleKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		vector<SEditorStyle>::const_iterator pCur;
		for (pCur = m_Styles.begin(); pCur != m_Styles.end(); pCur++)
		{
			StoreStyle(hStyleKey, &(*pCur));
		}

		RegCloseKey(hStyleKey);
	}
}

void CUserCfg::LoadStyles(HKEY hRegKey)
{
	HKEY hStyleKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, "Styles", 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hStyleKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		DWORD dwinx = 0;
		vector<char> vecName;
		vecName.resize(MAX_PATH);
		DWORD dwSize = vecName.size();
		FILETIME ft = {0};
		
		int nErr = RegEnumKeyEx(hStyleKey, dwinx, &(vecName[0]), &dwSize, 0, 0,0, &ft);
		while ((ERROR_SUCCESS == nErr) || (ERROR_MORE_DATA == nErr))
		{		
			HKEY hLangKey;
			Err = RegCreateKeyEx(hStyleKey, &(vecName[0]), 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hLangKey, &dwCreate);
			
			int CurStyle = 0;
			char StyleName[64];
			DWORD NameSize = sizeof(StyleName);
			
			DWORD dwType;
			SEditorStyle NewStyle;
			DWORD dwStyleSize = sizeof(SEditorStyle);
			LONG Err = RegEnumValue(hLangKey, CurStyle, StyleName, &NameSize, 0, &dwType, (unsigned char*)&NewStyle, &dwStyleSize);
			
			while (ERROR_SUCCESS == Err )
			{
				SetEditorStyle(NewStyle.Languagename, NewStyle.StyleId, &NewStyle);
				
				CurStyle++;
				NameSize = sizeof(StyleName);
				dwStyleSize = sizeof(SEditorStyle);
			
				Err = RegEnumValue(hLangKey, CurStyle, StyleName, &NameSize, 0, &dwType, (unsigned char*)&NewStyle, &dwStyleSize);
			}
			RegCloseKey(hLangKey);
			
			dwinx++;
			dwSize = vecName.size();
			nErr = RegEnumKeyEx(hStyleKey, dwinx, &(vecName[0]), &dwSize, 0, 0,0, &ft);
		}

		RegCloseKey(hStyleKey);
	}
}


void CUserCfg::StoreStyle(HKEY hRegKey, const SEditorStyle* pStyle)
{
	HKEY hLangKey;
	DWORD dwCreate;
	int Err = RegCreateKeyEx(hRegKey, pStyle->Languagename, 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hLangKey, &dwCreate);
	if (ERROR_SUCCESS == Err )
	{
		string StyleName = stringprintf("Style_%04d", pStyle->StyleId);
				
		RegSetValueEx(hLangKey, StyleName.c_str(), 0, REG_BINARY, (const BYTE*)pStyle, sizeof(SEditorStyle));

		RegCloseKey(hLangKey);
	}
}



CUserCfg::SEditorStyle DefaultStyles[] = 
{
	//these are all the python styles
	{"Python\0                        ",STYLE_DEFAULT,     "Default\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_DEFAULT,     "Python Text\0                   ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_COMMENTLINE, "Comment\0                       ", "Courier\0                       ", false, false, false, 12, RGB(128,128,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_NUMBER,      "Number\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_STRING,      "String\0                        ", "Courier\0                       ", false, false, false, 12, RGB(255,0,255), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_CHARACTER,   "Character\0                     ", "Courier\0                       ", false, false, false, 12, RGB(255,0,255), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_WORD,        "Keyword\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,255), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_TRIPLE,      "Triple\0                        ", "Courier\0                       ", false, false, false, 12, RGB(128,128,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_TRIPLEDOUBLE,"Triple Double\0                 ", "Courier\0                       ", false, false, false, 12, RGB(128,128,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_CLASSNAME,   "Class Name\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_DEFNAME,     "Function Name\0                 ", "Courier\0                       ", false, false, false, 12, RGB(0,0,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_OPERATOR,    "Operator\0                      ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_IDENTIFIER,  "Identifier\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,64), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_COMMENTBLOCK,"Comment Block\0                 ", "Courier\0                       ", false, false, false, 12, RGB(128,128,128), RGB(255,255,255)},
	{"Python\0                        ",SCE_P_STRINGEOL,   "String EOL\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",STYLE_LINENUMBER,  "Line Number\0                   ", "Courier\0                       ", false, false, false, 12, RGB(128,128,128), RGB(255,255,255)},
	{"Python\0                        ",STYLE_BRACELIGHT,  "Brace Match\0                   ", "Courier\0                       ", false, false, false, 12, RGB(0,255,255), RGB(255,255,255)},
	{"Python\0                        ",STYLE_BRACEBAD,    "Brace Err\0                     ", "Courier\0                       ", false, false, false, 12, RGB(255,0,0), RGB(255,255,255)},
	{"Python\0                        ",STYLE_CONTROLCHAR, "Control Char\0                  ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",STYLE_INDENTGUIDE, "Indent Guide\0                  ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"Python\0                        ",STYLE_DEFAULT,     "Default\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},

	//C/C++ styles
	{"C/C++\0                         ",STYLE_DEFAULT,               "Default\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_DEFAULT,               "C Text\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENT,               "Comment\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENTLINE,           "Comment Line\0                  ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENTDOC,            "Comment Doc\0                   ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_NUMBER,                "Number\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_WORD,                  "Keyword\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_STRING,                "String\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_CHARACTER,             "Character\0                     ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_UUID,                  "UUID\0                          ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_PREPROCESSOR,          "Preprocessor\0                  ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_OPERATOR,              "Operator\0                      ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_IDENTIFIER,            "Identifier\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_STRINGEOL,             "String EOL\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_VERBATIM,              "Verbatim\0                      ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_REGEX,                 "Regex\0                         ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENTLINEDOC,        "Comment Line Doc\0              ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_WORD2,                 "Word2\0                         ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENTDOCKEYWORD,     "CommentDocKeyword\0             ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"C/C++\0                         ",SCE_C_COMMENTDOCKEYWORDERROR,"CommentDocKwdError\0            ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},

	//Html/Xml styles
	{"html\0                          ",STYLE_DEFAULT,                "Default\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_DEFAULT,                "html code\0                     ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_TAG,                    "Tag\0                           ", "Courier\0                       ", false, false, false, 12, RGB(0,0,255), RGB(255,255,255)},
	{"html\0                          ",SCE_H_TAGUNKNOWN,             "Unknown Tag\0                   ", "Courier\0                       ", false, false, false, 12, RGB(0,0,255), RGB(255,255,255)},
	{"html\0                          ",SCE_H_ATTRIBUTE,              "Attribute\0                     ", "Courier\0                       ", false, false, false, 12, RGB(64,128,128), RGB(255,255,255)},
	{"html\0                          ",SCE_H_ATTRIBUTEUNKNOWN,       "Attribute Unk\0                 ", "Courier\0                       ", false, false, false, 12, RGB(64,128,128), RGB(255,255,255)},
	{"html\0                          ",SCE_H_NUMBER,                 "Number\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_DOUBLESTRING,           "Dbl String\0                    ", "Courier\0                       ", false, false, false, 12, RGB(255,0,255), RGB(255,255,255)},
	{"html\0                          ",SCE_H_SINGLESTRING,           "Sng String\0                    ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_OTHER,                  "Other\0                         ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_COMMENT,                "Comment\0                       ", "Courier\0                       ", false, false, false, 12, RGB(192,192,192), RGB(255,255,255)},
	{"html\0                          ",SCE_H_ENTITY,                 "Entity\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_TAGEND,                 "Tag End\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_XMLSTART,               "Xml Start\0                     ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_XMLEND,                 "Xml End\0                       ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_SCRIPT,                 "Script\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_ASP,                    "Asp\0                           ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_ASPAT,                  "Asp At\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_CDATA,                  "C Data\0                        ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_QUESTION,               "Question\0                      ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_VALUE,                  "Value\0                         ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},
	{"html\0                          ",SCE_H_XCCOMMENT,              "XCComment\0                     ", "Courier\0                       ", false, false, false, 12, RGB(0,0,0), RGB(255,255,255)},

	{"General\0                       ",HAPSTYLE_PROJECTVW_PROJTREE,  "Project Tree\0                  ", "Tahoma\0                        ", false, false, false,  9, RGB(0,0,0), RGB(255,255,255)},
	{"General\0                       ",HAPSTYLE_PROJECTVW_MODLIST,   "Module List\0                   ", "Tahoma\0                        ", false, false, false,  9, RGB(0,0,0), RGB(255,255,255)},
	{"General\0                       ",HAPSTYLE_STACKVW,             "Stack Window\0                  ", "Courier\0                       ", false, false, false, 10, RGB(0,0,0), RGB(255,255,255)},
	{"General\0                       ",HAPSTYLE_TRACEVW_ERR,         "Error Window\0                  ", "Courier\0                       ", false, false, false, 10, RGB(0,0,0), RGB(255,255,255)},
	{"General\0                       ",HAPSTYLE_TRACEVW_OUT,         "Output Window\0                 ", "Courier\0                       ", false, false, false, 10, RGB(0,0,0), RGB(255,255,255)},
	{"General\0                       ",HAPSTYLE_TRACEVW_MRG,         "Merged Window\0                 ", "Courier\0                       ", false, false, false, 10, RGB(0,0,0), RGB(255,255,255)},
};

void CUserCfg::LoadStyleDefaults()
{
	for (int i = 0; i < NUMELEMENTS(DefaultStyles); i++)
	{
		SetEditorStyle(DefaultStyles[i].Languagename, DefaultStyles[i].StyleId, &(DefaultStyles[i]));
	}


	//use some system colors to set a few colors more appropriately
	SEditorStyle style;
	
	GetEditorStyle("General", HAPSTYLE_PROJECTVW_PROJTREE, &style);
	style.BackCol = GetSysColor(COLOR_3DFACE);
	style.ForeCol = GetSysColor(COLOR_BTNTEXT);
	SetEditorStyle("General", HAPSTYLE_PROJECTVW_PROJTREE, &style);

	GetEditorStyle("General", HAPSTYLE_PROJECTVW_MODLIST, &style);
	style.BackCol = GetSysColor(COLOR_3DFACE);
	style.ForeCol = GetSysColor(COLOR_BTNTEXT);
	SetEditorStyle("General", HAPSTYLE_PROJECTVW_MODLIST, &style);

	GetEditorStyle("General", HAPSTYLE_PROJECTVW_MODLIST, &style);
	style.BackCol = GetSysColor(COLOR_3DFACE);
	style.ForeCol = GetSysColor(COLOR_BTNTEXT);
	SetEditorStyle("General", HAPSTYLE_PROJECTVW_MODLIST, &style);
}

const CUserCfg::SEditorStyle* CUserCfg::GetEditorStyle(unsigned int StyleIndex)const
{
	if ((StyleIndex >= 0) && (StyleIndex < m_Styles.size()))
		return &(m_Styles[StyleIndex]);
	return NULL;
}


void CUserCfg::LoadMRUProjects(HKEY hRegKey)
{
	HKEY hProjKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, REGKEY_PROJECTLIST, 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hProjKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		int n = 0;
		char ValName[64];
		char ProjBuff[MAX_PATH];
		DWORD dwType;
		for (int n = 0; n < MAX_MRU_PROJECTS; n++)
		{
			sprintf(ValName, REGVAL_LASTPROJECT, n);
			DWORD dwSize = MAX_PATH;			
			
			if (ERROR_SUCCESS==RegQueryValueEx(hProjKey, ValName, 0, &dwType, (unsigned char*)ProjBuff, &dwSize))
			{
				m_vecMruProjects.push_back(string(ProjBuff));
			}
		}

		RegCloseKey(hProjKey);
	}
}

void CUserCfg::StoreMRUProjects(HKEY hRegKey)
{
	HKEY hProjKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, REGKEY_PROJECTLIST, 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hProjKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		vector<string>::const_iterator pCur;
		int n = 0;
		char buff[64];
		for (pCur = m_vecMruProjects.begin(); pCur != m_vecMruProjects.end(); pCur++)
		{
			sprintf(buff, REGVAL_LASTPROJECT, n++);
			RegSetValueEx(hProjKey, buff, 0, REG_SZ, (const unsigned char*)pCur->c_str(),  pCur->size()+1);
		}

		RegCloseKey(hProjKey);
	}
}

void CUserCfg::AddMruProject(const char* cszProjName)
{
	dprintf("Adding MRU Project: %s\n", cszProjName);
		
	string LocalName(cszProjName);
	
	for (vector<string>::iterator pCur = m_vecMruProjects.begin(); pCur != m_vecMruProjects.end(); pCur++)
	{
		if (pCur->compare(LocalName) == 0)
		{
			m_vecMruProjects.erase(pCur);
			break;
		}
	}

	m_vecMruProjects.insert(m_vecMruProjects.begin(), LocalName);
	
	if (m_vecMruProjects.size() > MAX_MRU_PROJECTS)
	{
		m_vecMruProjects.erase(m_vecMruProjects.begin()+MAX_MRU_PROJECTS, m_vecMruProjects.end());
	}
}
int CUserCfg::GetMruProjectCount()const
{
	return m_vecMruProjects.size();
}

const char* CUserCfg::GetMruProject(unsigned int nProjInx)const
{
	if ((nProjInx >= 0) && (nProjInx < m_vecMruProjects.size()))
	{
		return m_vecMruProjects[nProjInx].c_str();
	}
	return NULL;
}




int CUserCfg::GetToolsCount()const
{
	return m_vecTools.size();
}

const char* CUserCfg::GetToolName(unsigned int nToolInx)const
{
	if ((nToolInx >= 0) && (nToolInx < m_vecTools.size()))
		return (m_vecTools[nToolInx].strToolName.c_str());
	return 0;
}

const char* CUserCfg::GetToolCmd(unsigned int nToolInx)const
{
	if ((nToolInx >= 0) && (nToolInx < m_vecTools.size()))
		return (m_vecTools[nToolInx].strToolCmd.c_str());
	return 0;
}

const char* CUserCfg::GetToolDir(unsigned int nToolInx)const
{
	if ((nToolInx >= 0) && (nToolInx < m_vecTools.size()))
		return (m_vecTools[nToolInx].strToolDir.c_str());
	return 0;
}

const char* CUserCfg::GetToolArgs(unsigned int nToolInx)const
{
	if ((nToolInx >= 0) && (nToolInx < m_vecTools.size()))
		return (m_vecTools[nToolInx].strToolArgs.c_str());
	return 0;
}

bool CUserCfg::GetToolMenuStr(unsigned int nToolInx, string& strMenuItem)const
{
	const char* cszToolName = GetToolName(nToolInx);
	if (cszToolName)
	{
		vector<char> buff;
		buff.resize(256);
		
		_snprintf(&(buff[0]), 255, "&%d %s\tCtrl+Alt+%d",nToolInx+1, cszToolName, nToolInx+1);
		buff[255] = 0;
		strMenuItem = &(buff[0]);
		return true;
	}
	return false;
}

void CUserCfg::AddTool(const char* ToolName, const char* ToolCmd, const char* ToolDir, const char* ToolArgs)
{
	if (GetToolsCount() < MAX_TOOLS)
	{
		STool newTool;
		newTool.strToolName = ToolName;
		newTool.strToolCmd = ToolCmd;
		newTool.strToolDir = ToolDir;
		newTool.strToolArgs = ToolArgs;
		
		m_vecTools.push_back(newTool);
	}
}

void CUserCfg::DelTool(unsigned int nToolInx)
{
	if ((nToolInx >= 0) && (nToolInx < m_vecTools.size()))
	{
		m_vecTools.erase(m_vecTools.begin() + nToolInx);
	}
}




void CUserCfg::LoadTools(HKEY hRegKey)
{
	HKEY hToolsListKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, REGKEY_TOOLSLIST, 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_READ, NULL, &hToolsListKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		DWORD dwIndex = 0;
		FILETIME ft;
		vector<char> vecRegStrBuff;
		DWORD dwSize = 256;
		vecRegStrBuff.resize(dwSize);

		Err = RegEnumKeyEx(hToolsListKey, dwIndex, &(vecRegStrBuff[0]), &dwSize, 0, 0, 0, &ft);
		
		while (ERROR_SUCCESS == Err)
		{
			HKEY hToolKey;
			DWORD dwCreate;
			Err = RegCreateKeyEx(hToolsListKey, &(vecRegStrBuff[0]), 0, 0, REG_OPTION_NON_VOLATILE, 
										KEY_READ, NULL, &hToolKey, &dwCreate);
			if ((ERROR_SUCCESS == Err) && (REG_OPENED_EXISTING_KEY == dwCreate))
			{
				string strName = &(vecRegStrBuff[0]);
				DWORD dwType = 0;
				
				dwSize = 256;	
				if (ERROR_SUCCESS == RegQueryValueEx(hToolKey, REGVAL_TOOLCMD, 0, &dwType, (unsigned char*)&(vecRegStrBuff[0]), &dwSize) )
				{
					string strCmd =  &(vecRegStrBuff[0]);
					
					dwSize = 256;
					if (ERROR_SUCCESS == RegQueryValueEx(hToolKey, REGVAL_TOOLDIR, 0, &dwType, (unsigned char*)&(vecRegStrBuff[0]), &dwSize) )
					{
						string strDir =  &(vecRegStrBuff[0]);
						
						dwSize = 256;
						if (ERROR_SUCCESS == RegQueryValueEx(hToolKey, REGVAL_TOOLARG, 0, &dwType, (unsigned char*)&(vecRegStrBuff[0]), &dwSize) )
						{
							string strArg =  &(vecRegStrBuff[0]);
							AddTool(strName.c_str(), strCmd.c_str(), strDir.c_str(), strArg.c_str());
						}
					}
				}

				RegCloseKey(hToolKey);
			}			
			
			dwIndex++;
			Err = RegEnumKeyEx(hToolsListKey, dwIndex, &(vecRegStrBuff[0]), &dwSize, 0, 0, 0, &ft);
		}
		
		
		RegCloseKey(hToolsListKey);
	}
}

void CUserCfg::StoreTools(HKEY hRegKey)
{
	HKEY hToolKey;
	DWORD dwCreate;
	LONG Err = RegCreateKeyEx(hRegKey, REGKEY_TOOLSLIST, 0, 0, REG_OPTION_NON_VOLATILE, 
								KEY_ALL_ACCESS, NULL, &hToolKey, &dwCreate);
	if (ERROR_SUCCESS == Err)
	{
		vector<STool>::const_iterator pCurTool = m_vecTools.begin();
		int n = 0;
		while (pCurTool != m_vecTools.end())
		{
			HKEY hCurToolKey;
			LONG Err = RegCreateKeyEx(hToolKey, pCurTool->strToolName.c_str(), 0, 0, REG_OPTION_NON_VOLATILE, 
								      KEY_ALL_ACCESS, NULL, &hCurToolKey, &dwCreate);
			if (ERROR_SUCCESS == Err)
			{
				RegSetValueEx(hCurToolKey, REGVAL_TOOLCMD, 0, REG_SZ, (const unsigned char*)pCurTool->strToolCmd.c_str(), pCurTool->strToolCmd.length()+1);
				RegSetValueEx(hCurToolKey, REGVAL_TOOLDIR, 0, REG_SZ, (const unsigned char*)pCurTool->strToolDir.c_str(), pCurTool->strToolDir.length()+1);
				RegSetValueEx(hCurToolKey, REGVAL_TOOLARG, 0, REG_SZ, (const unsigned char*)pCurTool->strToolArgs.c_str(), pCurTool->strToolArgs.length()+1);
			
				RegCloseKey(hCurToolKey);
			}
			pCurTool++;
		}

		RegCloseKey(hToolKey);
	}
}