//////////////////////////////////////////////////////////////////////////////
//  File:    FileGroup.cpp
//
//  Purpose: A dialog box which allows you to add files in a project to 
//  a file group (displayed as a folder)
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
#include "FileGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileGroup dialog


CFileGroup::CFileGroup(CWnd* pParent /*=NULL*/)
	: CDialog(CFileGroup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileGroup)
	m_strFileGroup = _T("");
	//}}AFX_DATA_INIT
}


void CFileGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileGroup)
	DDX_CBString(pDX, IDC_GROUP_CBO, m_strFileGroup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileGroup, CDialog)
	//{{AFX_MSG_MAP(CFileGroup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileGroup message handlers

BOOL CFileGroup::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CComboBox* pCbo = (CComboBox*)GetDlgItem(IDC_GROUP_CBO);
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	int GrpCount = pProject->GroupCount();
	for (int i = 0; i < GrpCount; i++)
	{
		CString Group = pProject->GetGroupName(i).c_str();
		if (Group.GetLength() > 0)
			pCbo->AddString(Group);
	}

	UpdateData(TRUE);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
