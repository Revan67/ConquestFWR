//////////////////////////////////////////////////////////////////////////////
//  File:    GroupPropertiesDlg.cpp
//
//  Purpose: A dialog box that allows you to change the name of a file group
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

// GroupPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "GroupPropertiesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupPropertiesDlg dialog


CGroupPropertiesDlg::CGroupPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGroupPropertiesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGroupPropertiesDlg)
	m_GroupName = _T("");
	//}}AFX_DATA_INIT
}


void CGroupPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupPropertiesDlg)
	DDX_Text(pDX, IDC_GROUPNAME_EDT, m_GroupName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGroupPropertiesDlg, CDialog)
	//{{AFX_MSG_MAP(CGroupPropertiesDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupPropertiesDlg message handlers
