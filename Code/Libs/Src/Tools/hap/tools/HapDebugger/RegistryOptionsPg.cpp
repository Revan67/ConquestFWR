//////////////////////////////////////////////////////////////////////////////
//  File:    RegistryOptionsPg.cpp
//
//  Purpose: This is a page of the settings dialog - it allows you to set a 
//  number of registry related options (file associations etc)
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
#include "dbgremote.h"
#include "RegistryOptionsPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegistryOptionsPg property page

IMPLEMENT_DYNCREATE(CRegistryOptionsPg, CPropertyPage)

CRegistryOptionsPg::CRegistryOptionsPg() : CPropertyPage(CRegistryOptionsPg::IDD)
{
	//{{AFX_DATA_INIT(CRegistryOptionsPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRegistryOptionsPg::~CRegistryOptionsPg()
{
}

void CRegistryOptionsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRegistryOptionsPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL CRegistryOptionsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	if ((AssurePythonEntries() == false) || (AssureDbgEntries() == false))
	{
		GetDlgItem(IDC_PYFILE_GRP)->EnableWindow(FALSE);
		GetDlgItem(IDC_OPENWITH_BTN)->EnableWindow(FALSE);
		GetDlgItem(IDC_SETEDITOR_BTN)->EnableWindow(FALSE);
	}

	//disable buttons if already set
	/*if (OpenWithSet())
		GetDlgItem(IDC_OPENWITH_BTN)->EnableWindow(FALSE);

	if (EditorSet())
		GetDlgItem(IDC_SETEDITOR_BTN)->EnableWindow(FALSE);
*/

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(CRegistryOptionsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CRegistryOptionsPg)
	ON_BN_CLICKED(IDC_OPENWITH_BTN, OnOpenWithBtn)
	ON_BN_CLICKED(IDC_SETEDITOR_BTN, OnSetEditorBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegistryOptionsPg message handlers

void CRegistryOptionsPg::OnOpenWithBtn() 
{
	TCHAR ProgPath[_MAX_PATH];
	if (0 == GetModuleFileName(AfxGetInstanceHandle(), ProgPath, _MAX_PATH) )
	{
		return;
	}

	HKEY hLocMachKey;
	if (ERROR_SUCCESS != RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hLocMachKey))
		return;

	/*set the value:
	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Python.File\Shell\openas\Command 
		= "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" "%1""
	*/
	HKEY hTempKey;
	const char* cszPyShellCommand = "SOFTWARE\\Classes\\Python.File\\Shell\\openas\\Command";
	DWORD Err = RegOpenKeyEx(hLocMachKey, cszPyShellCommand, 0, KEY_ALL_ACCESS, &hTempKey);
	DWORD dwCreate;
	if (Err != ERROR_SUCCESS)
	{
		//create the key
		Err = RegCreateKeyEx(hLocMachKey,  cszPyShellCommand, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	
	if (ERROR_SUCCESS == Err)
	{
		//build the value "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" "%1""
		CString Temp;
		Temp += '"';
		Temp += ProgPath;
		Temp += '"';
		Temp += " \"%1\"\"";
		//dprintf(Temp);

		RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
		
		RegCloseKey(hTempKey);
		GetDlgItem(IDC_OPENWITH_BTN)->EnableWindow(FALSE);
	}
	RegCloseKey(hLocMachKey);
} //OnOpenWithBtn

void CRegistryOptionsPg::OnSetEditorBtn() 
{
	TCHAR ProgPath[_MAX_PATH];
	if (0 == GetModuleFileName(AfxGetInstanceHandle(), ProgPath, _MAX_PATH) )
	{
		return;
	}

	HKEY hLocMachKey;
	if (ERROR_SUCCESS != RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hLocMachKey))
		return;

	/*set the value:
	
	HKEY_LOCAL_MACHINE\
		= "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" /open "%1"
	*/

	HKEY hTempKey;
	const char* cszPyOpenAsCommand = "SOFTWARE\\Classes\\Python.File\\Shell\\Edit\\Command";
	DWORD Err = RegOpenKeyEx(hLocMachKey, cszPyOpenAsCommand, 0, KEY_ALL_ACCESS, &hTempKey);
	DWORD dwCreate;
	if (Err != ERROR_SUCCESS)
	{
		//create the key
		Err = RegCreateKeyEx(hLocMachKey,  cszPyOpenAsCommand, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	
	if (ERROR_SUCCESS == Err)
	{
		//build the value "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" /open "%1"
		CString Temp;
		Temp += '"';
		Temp += ProgPath;
		Temp += "\" /open \"%1\"\"";
		//dprintf(Temp);

		RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
		
		RegCloseKey(hTempKey);
		
		GetDlgItem(IDC_SETEDITOR_BTN)->EnableWindow(FALSE);
	}

	//set the default icon
	const char* cszDefaultIcon = "SOFTWARE\\Classes\\Python.File\\DefaultIcon";
	Err = RegOpenKeyEx(hLocMachKey, cszDefaultIcon, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		//create the key
		Err = RegCreateKeyEx(hLocMachKey,  cszDefaultIcon, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	
	if (ERROR_SUCCESS == Err)
	{
		//build the value "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" /open "%1"
		CString Temp;
		Temp += ProgPath;
		Temp += ",2";
		//dprintf(Temp);

		RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
		
		RegCloseKey(hTempKey);
	}

	RegCloseKey(hLocMachKey);

} // OnSetEditorBtn








bool CRegistryOptionsPg::AssurePythonEntries()
{
	HKEY hLocMachKey;
	if (ERROR_SUCCESS != RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hLocMachKey))
		return false;
	
	//verify
	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\.py		= "Python.File"
	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\.py\"Content Type" = "text/plain"
	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Python.File = Python File

	const char* cszPyClass = "SOFTWARE\\Classes\\.py";
	HKEY hTempKey;

	LONG Err = RegOpenKeyEx(hLocMachKey, cszPyClass, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		RegCloseKey(hLocMachKey);
		return false;
	}

	DWORD dwType;
	unsigned char DataBuffer[1024];
	DWORD dwSize = 1024;
	
	
	if (ERROR_SUCCESS != RegQueryValueEx(hTempKey, "", 0, &dwType, DataBuffer, &dwSize) )
	{
		//should be "Python.File"
		dprintf("Query Value Failed\n");
		RegCloseKey(hTempKey);
		RegCloseKey(hLocMachKey);
		return false;
	}

	if (ERROR_SUCCESS != RegQueryValueEx(hTempKey, "Content Type", 0, &dwType, DataBuffer, &dwSize) )
	{
		//should be "text/plain"
		dprintf("Query Value Failed\n");
		RegCloseKey(hTempKey);
		RegCloseKey(hLocMachKey);
		return false;
	}

	RegCloseKey(hTempKey);


	const char* cszPyClassFile = "SOFTWARE\\Classes\\Python.File";
	Err = RegOpenKeyEx(hLocMachKey, cszPyClassFile, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		RegCloseKey(hLocMachKey);
		return false;
	}

	
	dwSize = 1024;
	if (ERROR_SUCCESS != RegQueryValueEx(hTempKey, "", 0, &dwType, DataBuffer, &dwSize))
	{
		//should be "Python File"
		dprintf("Query Value Failed\n");
		RegCloseKey(hTempKey);
		RegCloseKey(hLocMachKey);
		return false;
	}

	RegCloseKey(hTempKey);

	
	RegCloseKey(hLocMachKey);
	return true;

} // AssurePythonEntries

bool CRegistryOptionsPg::AssureDbgEntries()
{
	TCHAR ProgPath[_MAX_PATH];
	if (0 == GetModuleFileName(AfxGetInstanceHandle(), ProgPath, _MAX_PATH) )
	{
		return false;
	}

	TCHAR AppName[_MAX_PATH];
	strcpy(AppName, AfxGetAppName());
	strcat(AppName, ".exe");

	dprintf(ProgPath);
	dprintf("\n");
	dprintf(AppName);
	dprintf("\n");
	

	HKEY hLocMachKey;
	if (ERROR_SUCCESS != RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hLocMachKey))
		return false;
		
	/*verify:

	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Applications\DbgRemote.exe\Shell\open\command
		= C:\Work\Python\PAGEDEBUGGER\BinDebug\DBGREMOTE.EXE "%1"
	
	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Applications\DbgRemote.exe\Shell\open\ddeexec
		= [open("%1")]

	HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Applications\DbgRemote.exe\Shell\open\ddeexec\Application
		= DBGREMOTE	

	*/
	
	const char* cszAppRegStr  = "SOFTWARE\\Classes\\Applications\\";
	const char* cszAppOpenCmd = "\\Shell\\open\\command";
	const char* cszAppDdeCmd  = "\\Shell\\open\\ddeexec";
	const char* szAppDdeApp   = "\\Shell\\open\\ddeexec\\Application";
	
	CString RegStr = cszAppRegStr;
	RegStr += AppName;
	RegStr += cszAppOpenCmd;

	HKEY hTempKey;
	DWORD dwCreate;

	DWORD Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}

	//build the value "C:\Work\Python\PageDebugger\BinDebug\DbgRemote.exe" /open "%1"
	CString Temp;
	Temp += '"';
	Temp += ProgPath;
	Temp += "\" /open \"%1\"\"";
	
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}


	RegStr = cszAppRegStr;
	RegStr += AppName;
	RegStr += cszAppDdeCmd;
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	
	const char* cszDdeCmd = "[open(\"%1\")]";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)cszDdeCmd, strlen(cszDdeCmd)+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}


	RegStr = cszAppRegStr;
	RegStr += AppName;
	RegStr += szAppDdeApp;
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}

	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	
	const char* cszApp = "HapDebugger.exe";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)cszApp, strlen(cszApp)+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}


	

	RegStr = "SOFTWARE\\Classes\\.hap";
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	const char* cszProj = "HapDebugger.proj";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)cszProj, strlen(cszProj)+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}

	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\HapDebugger.proj
	//=Hap Project
	RegStr = "SOFTWARE\\Classes\\HapDebugger.proj";
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	cszProj = "Hap Project";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)cszProj, strlen(cszProj)+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	
	
	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\HapDebugger.proj\shell\open\command
	//=c:\work\Systems\dev\bin\HAPDEB~1.EXE "%1"
	RegStr = "SOFTWARE\\Classes\\HapDebugger.proj\\shell\\open\\command";
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	Temp = '"';
	Temp += ProgPath;
	Temp += "\" /open \"%1\"\"";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}

	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\HapDebugger.proj\shell\open\ddeexec
	//[open("%1")]
	RegStr = "SOFTWARE\\Classes\\HapDebugger.proj\\shell\\open\\ddeexec";
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	const char* cszdde = "[open(\"%1\")]";
	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)cszdde, strlen(cszdde)+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}



	RegStr = "SOFTWARE\\Classes\\HapDebugger.proj\\DefaultIcon";
	Err = RegOpenKeyEx(hLocMachKey, RegStr, 0, KEY_ALL_ACCESS, &hTempKey);
	if (Err != ERROR_SUCCESS)
	{
		Err = RegCreateKeyEx(hLocMachKey,  RegStr, 0,  NULL, 
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hTempKey, &dwCreate);
	}
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}
	Temp;
	Temp = ProgPath;
	Temp += ",3";

	Err = RegSetValueEx(hTempKey, "", 0, REG_SZ, (const unsigned char*)((const char*)Temp), Temp.GetLength()+1);
	RegCloseKey(hTempKey);
	if (Err != ERROR_SUCCESS)
	{	
		RegCloseKey(hLocMachKey);
		return false;
	}


	//HKEY_LOCAL_MACHINE\SOFTWARE\Classes\HapDebugger.proj\shell\open\ddeexec\Application
	//HAPDEB~1

	

	RegCloseKey(hLocMachKey);
	return true;

} // AssureDbgEntries

bool CRegistryOptionsPg::OpenWithSet()
{
	return false;
}

bool CRegistryOptionsPg::EditorSet()
{
	return false;
}


