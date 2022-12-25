//////////////////////////////////////////////////////////////////////////////
//  File:    AboutDlg.cpp
//
//  Purpose: Implementation of the class that creates the About box.
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
#include "AboutDlg.h"
 

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	m_BuildTime = "";
	m_Copywrite = "";
	m_PythonCopyright = _T("");
	m_PythonVersion = _T("");
	m_ProgVersion = _T("");
	m_PythonBuild = _T("");
	//}}AFX_DATA_INIT

	m_BuildTime = "Built: ";
	m_BuildTime += GetExeLinkDate();
	
	m_Copywrite = "Copyright ";
	m_Copywrite += (char)0xA9;
	m_Copywrite += " Humongous Entertainment 2001";

	m_PythonVersion = "Python Version: ";
//	m_PythonBuild = Py_GetVersion();
//	m_PythonCopyright = Py_GetCopyright();
	

	m_ProgVersion = "Hap Debugger Version 3.0";
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Text(pDX, IDC_BUILDTIME_STC, m_BuildTime);
	DDX_Text(pDX, IDC_COPYWRITE_STC, m_Copywrite);
	DDX_Text(pDX, IDC_PYTHONCOPY_STC, m_PythonCopyright);
	DDX_Text(pDX, IDC_PYTHONVER_STC, m_PythonVersion);
	DDX_Text(pDX, IDC_VERSION_STC, m_ProgVersion);
	DDX_Text(pDX, IDC_PYTHONBUILD_STC, m_PythonBuild);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

