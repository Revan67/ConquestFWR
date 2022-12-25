//////////////////////////////////////////////////////////////////////////////
//  File:    ProjectViewBar.cpp
//
//  Purpose: This is a dockable window that displays the current project (if any)
//  it also displays the current modules loaded by the python debugger.
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
#include "ProjectViewBar.h"
#include "PdpManager.h"
#include "FilePropDlg.h"
#include "FileGroup.h"
#include "GroupPropertiesDlg.h"
#include "SourceControlInterface.h"
#include "FindInProjectDlg.h"


#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_MODULE_TREE	1022
#define ID_PROJECT_TREE 1023



const int TABINX_PROJECT = 0;
const int TABINX_MODULES = 1;


const int IMAGEINX_PROJ_NOTVCD = 0;
const int IMAGEINX_PROJ_CHEKDIN = 2;
const int IMAGEINX_PROJ_CHEKDOUT = 4;

const int IMAGEINX_PYFILE_NOTVCD = 6;
const int IMAGEINX_PYFILE_CHEKDIN = 7;
const int IMAGEINX_PYFILE_CHEKDOUT = 8;
const int IMAGEINX_PYFILE_SYNC = 9;
const int IMAGEINX_PYFILE_CHEKSYNC = 10;

const int IMAGEINX_XMLFILE_NOTVCD = 11;
const int IMAGEINX_XMLPYFILE_CHEKDIN = 12;
const int IMAGEINX_XMLPYFILE_CHEKDOUT = 13;
const int IMAGEINX_XMLPYFILE_SYNC = 14;
const int IMAGEINX_XMLPYFILE_CHEKSYNC = 15;

const int IMAGEINX_UNKFILE_NOTVCD = 16;
const int IMAGEINX_UNKPYFILE_CHEKDIN = 17;
const int IMAGEINX_UNKPYFILE_CHEKDOUT = 18;
const int IMAGEINX_UNKPYFILE_SYNC = 19;
const int IMAGEINX_UNKPYFILE_CHEKSYNC = 20;

const int IMAGEINX_BATFILE_NOTVCD = 21;
const int IMAGEINX_BATPYFILE_CHEKDIN = 22;
const int IMAGEINX_BATPYFILE_CHEKDOUT = 23;
const int IMAGEINX_BATPYFILE_SYNC = 24;
const int IMAGEINX_BATPYFILE_CHEKSYNC = 25;

/////////////////////////////////////////////////////////////////////////////
// CProjectViewBar

CProjectViewBar::CProjectViewBar()
{
}

CProjectViewBar::~CProjectViewBar()
{
}


BEGIN_MESSAGE_MAP(CProjectViewBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CProjectViewBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_COMMAND(ID_FILEPOPUP_SETASBOOTFILE, OnFilepopupSetBootFile)
	ON_COMMAND(ID_FILEPOPUP_PROPERTIES, OnFilepopupProperties)
	ON_COMMAND(ID_FILEPOPUP_ADDTOVCS, OnFilepopupAddtoVcs)
	ON_COMMAND(ID_FILEPOPUP_CHECKIN, OnFilepopupCheckin)
	ON_COMMAND(ID_FILEPOPUP_CHECKOUT, OnFilepopupCheckout)
	ON_COMMAND(ID_FILEPOPUP_UNDOCHECKOUT, OnFilepopupUndocheckout)
	ON_COMMAND(ID_FILEPOPUP_GROUP, OnFilepopupSetGroup)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_FILEPOPUP_EXPLORER, OnFilepopupExplorer)
	ON_COMMAND(ID_FILEPOPUP_CMDPROMPT, OnFilepopupCmdprompt)
	ON_COMMAND(ID_FILEPOPUP_REMOVEFROMPROJECT, OnRemoveFromProject)
	ON_COMMAND(ID_FILEPOPUP_DIFFFILE, OnFilepopupDiffFile)
	ON_COMMAND(ID_FILEPOPUP_SYNCFILE, OnFilepopupSyncFile)
	ON_COMMAND(ID_FILEPOPUP_FINDINPROJECT, OnFilepopupFindinProject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CProjectViewBar message handlers
int CProjectViewBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (! m_ModuleTree.Create(WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_DISABLEDRAGDROP|TVS_SHOWSELALWAYS, CRect(0,0,10,10), this, ID_MODULE_TREE) )	
		return -1;

	if (! m_ProjectTree.Create(WS_CHILD|WS_VISIBLE|TVS_HASLINES|TVS_HASBUTTONS|TVS_SHOWSELALWAYS, CRect(0,0,10,10), this, ID_PROJECT_TREE) )
		return -1;

		

	m_FileIcons.Create(IDB_FILEIMAGELIST_BMP, 24, 0, RGB(255,255,255));
	m_ProjectTree.SetImageList(&m_FileIcons, TVSIL_NORMAL);

	m_Tab.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|TCS_TABS|TCS_BOTTOM|TCS_RIGHTJUSTIFY, CRect(0,0,10,10), this, 1011);
	m_Tab.SetMinTabWidth(0);
	m_Tab.InsertItem(TCIF_TEXT, TABINX_PROJECT, "Project", 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, TABINX_MODULES, "Modules", 0, 0);
	//m_Tab.InsertItem(TCIF_TEXT, TABINX_CLASSES, "Class", 0, 0);

	UpdateStyles();
	
	

	if (!m_TabFont.CreatePointFont(75, "Tahoma"))
        return -1;
	m_Tab.SetFont(&m_TabFont);

	m_pDragImgList = 0;
	m_bDragging = false;

	ShowProject();

    return 0;
}

void CProjectViewBar::OnSize(UINT nType, int cx, int cy) 
{
    CSizingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);
	
	m_Tab.MoveWindow(rc);
	m_Tab.AdjustRect(FALSE, &rc);
	m_ModuleTree.MoveWindow(rc);
	m_ProjectTree.MoveWindow(rc);
}

HBRUSH CProjectViewBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CProjectViewBar::OnClose() 
{
	CSizingControlBar::OnClose();
	if (m_pDragImgList)
		delete m_pDragImgList;
	m_pDragImgList = 0;
}


void CProjectViewBar::UpdateModules(const char* cszModuleList)
{
	m_ModuleTree.LockWindowUpdate();
	m_ModuleTree.DeleteAllItems();
	m_Filenames.RemoveAll();
	
	if (cszModuleList)
	{
		const char* pCur = cszModuleList;
		while (strlen(pCur) > 0)
		{
			CString ModuleName = pCur;
			//dprintf("Module: %s\n", pCur);

			int i = ModuleName.Find(',');
			if (i > -1)
			{
				int nItemInx = m_Filenames.Add( ModuleName.Right(ModuleName.GetLength() - i - 1) );
				if (m_Filenames[nItemInx].Right(4).CompareNoCase(".pyc") == 0)
				{
					m_Filenames[nItemInx] = m_Filenames[nItemInx].Left(m_Filenames[nItemInx].GetLength() - 1);
				}
				
				
				
				ModuleName = ModuleName.Left(i);
				
				
				HTREEITEM hNewItem = m_ModuleTree.InsertItem((const char*)ModuleName, TVI_ROOT, TVI_LAST);
				m_ModuleTree.SetItemData(hNewItem, nItemInx);
			}
			pCur += strlen(pCur) + 1;

		}
	}
	m_ModuleTree.SortChildren(NULL);
	m_ModuleTree.UnlockWindowUpdate();
}



BOOL CProjectViewBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pNm = (NMHDR*) lParam;
	if(pNm->code == TCN_SELCHANGE)
	{
		switch (m_Tab.GetCurSel())
		{
		case TABINX_PROJECT:
			ShowProject();
			break;
		case TABINX_MODULES:
			ShowModules();
			break;
		}
	}
	
	else if (pNm->idFrom == ID_MODULE_TREE)
	{
		if (pNm->code == NM_DBLCLK)
		{
			HTREEITEM hCurItem = m_ModuleTree.GetSelectedItem();
			if (hCurItem)
			{
				int FilenameInx = m_ModuleTree.GetItemData(hCurItem);
				if (FilenameInx >= 0)
				{
					if (m_Filenames[FilenameInx].Right(3).CompareNoCase(".py") == 0)
					{
						AfxGetApp()->OpenDocumentFile(m_Filenames[FilenameInx]);
					}
				}
			}
		}
		
	}
	else if (pNm->idFrom == ID_PROJECT_TREE)
	{
		if (pNm->code == NM_DBLCLK)
		{
			HTREEITEM hCurItem = m_ProjectTree.GetSelectedItem();
			if (hCurItem)
			{
				int FilenameInx = m_ProjectTree.GetItemData(hCurItem);
				if (FilenameInx != -1)
				{
					string Filename = GetFilename(FilenameInx);
					
					//since we're using xml files so much, it might be better to open them
					//in explorer
					if (((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg()->GetOpenXmlFilesExternal() && 
						(Filename.find(".xml") != string::npos))
					{
						SHELLEXECUTEINFO ShExecInfo;

						ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
						ShExecInfo.fMask = NULL;
						ShExecInfo.hwnd = NULL;
						ShExecInfo.lpVerb = NULL;
						ShExecInfo.lpFile = Filename.c_str();
						ShExecInfo.lpParameters = NULL;
						ShExecInfo.lpDirectory = NULL;
						ShExecInfo.nShow = SW_SHOW;
						ShExecInfo.hInstApp = NULL;

						ShellExecuteEx(&ShExecInfo);
					}
					else
					{	
						AfxGetApp()->OpenDocumentFile(Filename.c_str());
					}
				}
			}
		}
		else if (pNm->code == NM_RCLICK)
		{
			ShowProjectFileMenu();
		}
		else if (pNm->code == TVN_BEGINDRAG)
		{
			OnProjectBeginDrag((LPNMTREEVIEW) lParam);
		}
		else if (pNm->code == TVN_KEYDOWN)
		{
			OnProjectKeyDown((LPNMTVKEYDOWN) lParam);
		}
	}

	return CSizingControlBar::OnNotify(wParam, lParam, pResult);
}

BOOL CProjectViewBar::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{

	return CSizingControlBar::OnChildNotify(message, wParam, lParam, pLResult);
}


CString CProjectViewBar::GetFilePathName(const char* cszFilename)const
{

	for (int i = 0; i < m_Filenames.GetSize(); i++)
	{
		int nPathEnd = m_Filenames[i].ReverseFind('\\');
		if (nPathEnd < 0)
			nPathEnd = 0;

		if (m_Filenames[i].Find(cszFilename, nPathEnd) >= 0)
			return m_Filenames[i];
	}
	return "";
}

void CProjectViewBar::UpdateProjectDisplay(CPdpManager* pProject)
{
	if (pProject->GetProjectFile().length() == 0)
	{
		m_ProjectTree.DeleteAllItems();
		return;
	}

	m_vecProjectItems.resize(0);
	int FileCount = pProject->FileCount();
	CSourceControlInterface* pSci = pProject->GetSci();
	
	string BootFile = pProject->BootFile();
	string Path = pProject->GetProjectPath();
	string PathFile;

	vector<char> absPath;
	absPath.resize(1024);
	for (int i = 0; i < FileCount; i++)
	{
		SProjectItem NewItem;
		NewItem.strFilename = pProject->GetFileName(i);
		string FilePath = pProject->GetFilePath(i);
		if (!pSci)
		{
			NewItem.status.bControlled = false;
		}
		else
		{
			NewItem.status = pSci->GetFileStatus(FilePath.c_str());
		}

		NewItem.bBootFile = (BootFile == NewItem.strFilename);
		NewItem.strGroup = pProject->GetFileGroup(NewItem.strFilename);
		NewItem.nInx = i;

		m_vecProjectItems.push_back(NewItem);
	}

	//sort(m_vecProjectItems.begin(), m_vecProjectItems.end());

	//get a list of the groups that are expanded so we can re-expand them.
	set<string> vecExpandedNodes;
	HTREEITEM hRoot = m_ProjectTree.GetRootItem();
	GetExpandedNodes(hRoot, vecExpandedNodes);

	m_ProjectTree.LockWindowUpdate();
	m_ProjectTree.DeleteAllItems();
	
	string ProjectName = pProject->ProjectName();
	
	string ProjPathFile = pProject->GetProjectPath();
	ProjPathFile += pProject->GetProjectFile();

	m_ProjectFileStatus = pSci->GetFileStatus(ProjPathFile.c_str());
	
	// Root node
	HTREEITEM hProjRoot = m_ProjectTree.InsertItem(ProjectName.c_str(), IMAGEINX_PROJ_NOTVCD, IMAGEINX_PROJ_NOTVCD+1, TVI_ROOT, TVI_LAST);
	m_ProjectTree.SetItemData(hProjRoot, -1);

	UpdateProjectItemStatus();
	
	// Group nodes
	std::map<std::string,HTREEITEM> groupMap;
	for( int i = 0; i < pProject->GroupCount(); ++i )
	{
		std::string group = pProject->GetGroupName(i);
		if (group.length() > 0)
		{

			int pos = group.find( '/', 0 );
			if( pos >= 0 )
			{

				HTREEITEM hLastNode = hProjRoot;
				pos = 0;
				while( pos >= 0 )
				{
					int new_pos = group.find( '/', (pos + 1) );

					std::string name = group.substr( ((pos == 0) ? pos : (pos + 1)),(((new_pos < 0) ? (group.size() - 1) : new_pos) - pos));
					std::string mapKey = ((new_pos < 0) ? group : group.substr(0,new_pos));
					std::string parent = group.substr(0,pos);

					HTREEITEM parentNode;
					if( parent == "" )
					{
						parentNode = hProjRoot;
					}
					else
					{
						parentNode = groupMap[parent];
					}

					if( !groupMap[mapKey] )
					{	
						HTREEITEM hGroup = m_ProjectTree.InsertItem(name.c_str(), IMAGEINX_PROJ_NOTVCD, IMAGEINX_PROJ_NOTVCD+1, parentNode, TVI_LAST);
						groupMap[mapKey] = hGroup;
						m_ProjectTree.SetItemData(hGroup, -1);
						hLastNode = hGroup;
					}
					else
					{
						hLastNode = groupMap[mapKey];
					}
					
					if( new_pos < 0 )
					{
						break;
					}
					else
					{
						pos = new_pos;
					}

				}

			}
			else
			{

				if( !groupMap[group] )
				{
					HTREEITEM hGroup = m_ProjectTree.InsertItem(group.c_str(), IMAGEINX_PROJ_NOTVCD, IMAGEINX_PROJ_NOTVCD+1, hProjRoot, TVI_LAST);
					groupMap[group] = hGroup;
					m_ProjectTree.SetItemData(hGroup, -1);
				}

			}

		}
	}

	for (int i = 0; i < FileCount; i++)
	{
		string Display = m_vecProjectItems[i].strFilename;
		
		//strip the path off of the filename
		int FilenmInx = Display.find_last_of("\\/");
		if (FilenmInx >= 0)
		{
			Display.erase(Display.begin(), Display.begin()+FilenmInx+1);
		}
		
		// determine which group to put the file in
		HTREEITEM parent = 0;
		if( m_vecProjectItems[i].strGroup != "" )
		{
			parent = groupMap[m_vecProjectItems[i].strGroup];
		}
		else
		{
			parent = hProjRoot;
		}
		
		
		HTREEITEM hItem = m_ProjectTree.InsertItem(Display.c_str(), IMAGEINX_UNKFILE_NOTVCD, IMAGEINX_UNKFILE_NOTVCD, parent, TVI_LAST);
		m_ProjectTree.SetItemData(hItem, i);

		UpdateFileItemStatus(hItem);
	}

	//expand the project node
	m_ProjectTree.Expand(hProjRoot, TVE_EXPAND);
	ExpandNodes(hProjRoot, vecExpandedNodes);

	m_ProjectTree.UnlockWindowUpdate();
}


void CProjectViewBar::UpdateProjectSettings(CPdpManager* pProject)
{
	HTREEITEM hRoot = m_ProjectTree.GetRootItem();
	if (hRoot)
	{
		//update the project name
		string ProjName = pProject->ProjectName();
		m_ProjectTree.SetItemText(hRoot, ProjName.c_str());

		
		//update the boot file indicator (bold style)
		string BootFile = pProject->BootFile();
		HTREEITEM hCurChild = m_ProjectTree.GetChildItem(hRoot);
		while (hCurChild)
		{
			int ItemInx = m_ProjectTree.GetItemData(hCurChild);
			
			if (ItemInx > -1)
			{
				if (m_vecProjectItems[ItemInx].bBootFile)
					m_ProjectTree.SetItemState(hCurChild, 0, TVIS_BOLD);

				m_vecProjectItems[ItemInx].bBootFile = (BootFile == m_vecProjectItems[ItemInx].strFilename);
				if (m_vecProjectItems[ItemInx].bBootFile)
				{
					m_ProjectTree.SetItemState(hCurChild, TVIS_BOLD, TVIS_BOLD);
				}
			}

			hCurChild = m_ProjectTree.GetNextSiblingItem(hCurChild);
		}
	}
} // UpdateProjectSettings

void CProjectViewBar::UpdateFileStatus(const char* cszFile, CPdpManager* pProject)
{
	string srchStr = cszFile;
	int pathEnd = srchStr.rfind('\\');
	if (pathEnd > -1)
	{
		srchStr = srchStr.substr(pathEnd+1);
	}
	HTREEITEM hSrch = m_ProjectTree.GetRootItem();
	HTREEITEM hMatch = FindFileInTree(hSrch, srchStr.c_str());
	if (hMatch)
	{	
		int nInx = m_ProjectTree.GetItemData(hMatch);
		CSourceControlInterface* pSci = pProject->GetSci();
		if ((nInx > -1) && pSci)
		{	
			string AddFile = GetFilename(nInx);
			m_vecProjectItems[nInx].status = pSci->GetFileStatus(AddFile.c_str());
			UpdateFileItemStatus(hMatch);	
		}
	}
}



void CProjectViewBar::ShowModules()
{
	m_ModuleTree.ShowWindow(SW_SHOW);
	m_ProjectTree.ShowWindow(SW_HIDE);
}

void CProjectViewBar::ShowProject()
{
	m_ProjectTree.ShowWindow(SW_SHOW);
	m_ModuleTree.ShowWindow(SW_HIDE);
}

string m_SelectedFilename;

void CProjectViewBar::ShowProjectFileMenu()
{
	HTREEITEM hCurItem = m_ProjectTree.GetDropHilightItem();
	if (hCurItem == NULL)
	{
		hCurItem = m_ProjectTree.GetSelectedItem();
		if (hCurItem == NULL)
			return;
	}

	m_SelectedFilename = "";

	m_ProjectTree.Select(hCurItem, TVGN_CARET);

	int nInx = m_ProjectTree.GetItemData(hCurItem);
	
	RECT rectSel;
	m_ProjectTree.GetItemRect(hCurItem, &rectSel, TRUE);
	ClientToScreen(&rectSel);

	rectSel.top += 10;
	rectSel.left += 10;
	 
	// Load the menu resource. 	
	CMenu menu;
	menu.LoadMenu(IDR_FILEPOPUP);

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	CMenu* pSubMenu = menu.GetSubMenu(0); 

	//enable or disable menu items based on whats selected
	//NOTE if this gets any more complicated it should be converted to use
	//seperate menus...
	if (nInx == -1)
	{		
		//project or folder so disable file only stuff
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_SETASBOOTFILE, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_GROUP, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_REMOVEFROMPROJECT, MF_BYCOMMAND|MF_GRAYED);		
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_EXPLORER, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_CMDPROMPT, MF_BYCOMMAND|MF_GRAYED);
				
		//BUGBUG - this could be enabled for the project file..
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_DIFFFILE, MF_BYCOMMAND|MF_GRAYED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_SYNCFILE, MF_BYCOMMAND|MF_GRAYED);

		//if the project file is under vcs control, disable the add command
		if (m_ProjectFileStatus.bControlled)
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_ADDTOVCS, MF_BYCOMMAND|MF_GRAYED);
		else
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_ADDTOVCS, MF_BYCOMMAND|MF_ENABLED);

		//if the project file is checked out
		if (m_ProjectFileStatus.bOpenForEdit)
		{
			//disable check out
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKOUT, MF_BYCOMMAND|MF_GRAYED);
			//enable check in and undo checkout
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_UNDOCHECKOUT, MF_BYCOMMAND|MF_ENABLED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_ENABLED);
		}
		else
		{
			//otherwise enable check out
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKOUT, MF_BYCOMMAND|MF_ENABLED);
			//disable check in and undo checkout
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_UNDOCHECKOUT, MF_BYCOMMAND|MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_GRAYED);
		}
	}
	else
	{
		//file
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_SETASBOOTFILE, MF_BYCOMMAND|MF_ENABLED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_GROUP, MF_BYCOMMAND|MF_ENABLED);
		pSubMenu->EnableMenuItem(ID_FILEPOPUP_REMOVEFROMPROJECT, MF_BYCOMMAND|MF_ENABLED);

		m_SelectedFilename = m_vecProjectItems[nInx].strFilename;
		
		if (m_vecProjectItems[nInx].status.bControlled)
		{
			if (m_vecProjectItems[nInx].status.bOpenForEdit)
			{
				//disable check out
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKOUT, MF_BYCOMMAND|MF_GRAYED);
				//enable check in
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_ENABLED);
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_UNDOCHECKOUT, MF_BYCOMMAND|MF_ENABLED);
				//enable diff
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_DIFFFILE, MF_BYCOMMAND|MF_ENABLED);
				//disable sync (must resync from sc)
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_SYNCFILE, MF_BYCOMMAND|MF_GRAYED);
			}
			else
			{
				//disable check in
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_GRAYED);
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_UNDOCHECKOUT, MF_BYCOMMAND|MF_GRAYED);

				//enable check out
				pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKOUT, MF_BYCOMMAND|MF_ENABLED);

				if (m_vecProjectItems[nInx].status.bNewerInSc)
				{
					//disable diff
					pSubMenu->EnableMenuItem(ID_FILEPOPUP_DIFFFILE, MF_BYCOMMAND|MF_GRAYED);
					//enable sync (since the file isn't checked out)
					pSubMenu->EnableMenuItem(ID_FILEPOPUP_SYNCFILE, MF_BYCOMMAND|MF_ENABLED);
				}
				else
				{
					//no sense in diffing - should match 
					pSubMenu->EnableMenuItem(ID_FILEPOPUP_DIFFFILE, MF_BYCOMMAND|MF_GRAYED);
					//no sense in syncing - should match 
					pSubMenu->EnableMenuItem(ID_FILEPOPUP_SYNCFILE, MF_BYCOMMAND|MF_GRAYED);
				}
			}
			
			//disable add to vcs
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_ADDTOVCS, MF_BYCOMMAND|MF_GRAYED);
		}
		else
		{
			//gray checkin/checkout
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKIN, MF_BYCOMMAND|MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_CHECKOUT, MF_BYCOMMAND|MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_UNDOCHECKOUT, MF_BYCOMMAND|MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_DIFFFILE, MF_BYCOMMAND|MF_GRAYED);
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_SYNCFILE, MF_BYCOMMAND|MF_GRAYED);

			//enable add to vcs
			pSubMenu->EnableMenuItem(ID_FILEPOPUP_ADDTOVCS, MF_BYCOMMAND|MF_ENABLED);

			

		}
	}

	// Display the shortcut menu. Track the right mouse 
	// button. 

	pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
			rectSel.left, rectSel.top, this); 

} // ShowProjectFileMenu

void CProjectViewBar::OnFilepopupSetBootFile() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);

	assert(m_SelectedFilename.size() > 0);

	pProject->SetBootFile(m_SelectedFilename);

	((CDbgRemoteApp*)AfxGetApp())->SaveProjectFile();

	UpdateProjectSettings(pProject);
}

void CProjectViewBar::OnFilepopupExplorer() 
{
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;

	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);	
	string FilePath;// = pProject->GetProjectPath();

	char CmdLine[512];

	HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
	if (hSelItem)
	{
		int nInx = m_ProjectTree.GetItemData(hSelItem);
		if (nInx > -1)
		{
			//FilePath += m_vecProjectItems[nInx].strFilename;
			FilePath = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);
			int nPathEnd = FilePath.rfind('\\');
			if (nPathEnd != -1)
				FilePath = FilePath.substr(0, nPathEnd);
		}
	}
	strcpy(CmdLine, "explorer.exe /e,");
	strcat(CmdLine, FilePath.c_str());

	CreateProcess(0, 
		CmdLine, NULL, NULL, FALSE, 0, NULL, 
		FilePath.c_str(), &si, &pi);
	
}

void CProjectViewBar::OnFilepopupCmdprompt() 
{
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;

	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);	
	string FilePath;// = pProject->GetProjectPath();

	HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
	if (hSelItem)
	{
		int nInx = m_ProjectTree.GetItemData(hSelItem);
		if (nInx > -1)
		{
			//FilePath += m_vecProjectItems[nInx].strFilename;
			FilePath = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);
			int nPathEnd = FilePath.rfind('\\');
			if (nPathEnd != -1)
				FilePath = FilePath.substr(0, nPathEnd);
		}
	}


	CreateProcess(0, 
		"cmd.exe", NULL, NULL, FALSE, 0, NULL, 
		FilePath.c_str(), &si, &pi);
	
}


void CProjectViewBar::OnFilepopupProperties() 
{
	HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
	if (hSelItem)
	{
		int nInx = m_ProjectTree.GetItemData(hSelItem);
		if (nInx > -1)
		{
			CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
			assert(pProject);	

			string FilePath = GetFilename(nInx);

			CFilePropDlg dlg;
			dlg.ShowFileProperties(FilePath.c_str());
		}
		else
		{
			
			if (m_ProjectTree.GetRootItem() == hSelItem)
			{
				//project node selected
				((CDbgRemoteApp*)AfxGetApp())->OnProjectSettings();
			}
			else
			{
				//a group is selected
				CGroupPropertiesDlg dlg;
				dlg.m_GroupName = m_ProjectTree.GetItemText(hSelItem);
				string OldGroupName = dlg.m_GroupName;
				
				if (dlg.DoModal() == IDOK)
				{	
					string NewGroupName = dlg.m_GroupName;

					CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
					assert(pProject);
					for (int i = 0; i < pProject->GroupCount(); i++)
					{
						if (pProject->GetGroupName(i) == OldGroupName)
						{
							pProject->SetGroupName(i, NewGroupName);
							break;
						}
					}
					UpdateProjectDisplay(pProject);
				}
			}
		}
	}
}

void CProjectViewBar::OnFilepopupAddtoVcs() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				string AddFile = GetFilename(nInx);

				((CDbgRemoteApp*)AfxGetApp())->SaveDocumentFile(AddFile.c_str());		
				
				pSci->AddFile(AddFile.c_str());

				m_vecProjectItems[nInx].status = pSci->GetFileStatus(AddFile.c_str());
				
				UpdateFileItemStatus(hSelItem);
			}
			else
			{
				//project file
			}
		}
	}
}

void CProjectViewBar::OnFilepopupCheckin() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				//string CheckFile = GetFilename(nInx);
				string CheckFile = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);

				((CDbgRemoteApp*)AfxGetApp())->SaveDocumentFile(CheckFile.c_str());			
				
				pSci->CheckFileIn(CheckFile.c_str());

				m_vecProjectItems[nInx].status = pSci->GetFileStatus(CheckFile.c_str());
				
				UpdateFileItemStatus(hSelItem);
			}
			else
			{
				//check in project file
			}
		}
	}
}

//quick wildcard comparison code taken from www.codeproject.com
int wildcmp(const char* wild, const char* string)
// -------------------------------------------------------------------
{
	if(*wild == *string)
		return '\0' == *string || wildcmp(++wild, ++string);

	if('\0' == *string)
		return '*' == *wild && wildcmp(++wild, string);

	switch(*wild)
	{
		case '?':
			return wildcmp(++wild, ++string);

		case '*':
			wild++;

			if('\0' == *wild)
				return 1;

			while(*string != '\0')
				if(wildcmp(wild, string++))
					return 1;

		default:
			return 0;
	}
}

HTREEITEM CProjectViewBar::FindFileInTree(HTREEITEM hItemStart, const char* cszFilename)
{
	HTREEITEM hItem = hItemStart;
	while (hItem)
	{
		//compare the current item
		if (wildcmp(cszFilename, m_ProjectTree.GetItemText(hItem)) != 0)
			return hItem;

		//get the next item - first test any children
		HTREEITEM hChild = m_ProjectTree.GetChildItem(hItem);
		if (hChild)
			hItem = hChild;
		else
		{
			//no children, try the next sibling
			HTREEITEM hSibling = m_ProjectTree.GetNextSiblingItem(hItem);
			if (hSibling)
				hItem = hSibling;
			else
			{
				//try the parent's next sibling
				HTREEITEM hPar = m_ProjectTree.GetParentItem(hItem);
				if (hPar)
					hItem = m_ProjectTree.GetNextSiblingItem(hPar);
				else
					hItem = 0;
			}
		}
	}
	/*HTREEITEM hChild = m_ProjectTree.GetChildItem(hItemStart);
	while (hChild)
	{
		//compare this node
		if (wildcmp(cszFilename, m_ProjectTree.GetItemText(hChild)) != 0)
			return hChild;

		if (m_ProjectTree.ItemHasChildren(hChild))
		{
			//compare children nodes
			HTREEITEM hNextChild = FindFileInTree(hChild, cszFilename);
			if (hNextChild)
				return hNextChild;
		}
		//move to the next sibling
		hChild = m_ProjectTree.GetNextSiblingItem(hChild);
	}*/
	return 0;
}

void CProjectViewBar::OnFilepopupFindinProject() 
{
	//find a file in the current project (wildcard search)
	static CString findFile;
	CFindInProjectDlg dlg;
	dlg.m_Filename = findFile;
	if (dlg.DoModal() == IDOK)
	{
		findFile = dlg.m_Filename;
		HTREEITEM hSrch = m_ProjectTree.GetSelectedItem();
		if (!hSrch)
			hSrch = m_ProjectTree.GetRootItem();
		HTREEITEM hMatch = FindFileInTree(hSrch, dlg.m_Filename);
		if (hMatch)
		{
			m_ProjectTree.EnsureVisible(hMatch);
			m_ProjectTree.SelectItem(hMatch);
			
			m_ProjectTree.Select(hMatch, TVGN_CARET);
			m_ProjectTree.SetFocus();
		}
	}
}


void CProjectViewBar::UpdateFileItemStatus(HTREEITEM hItem)
{
	int nInx = m_ProjectTree.GetItemData(hItem);
	if (nInx > -1)
	{	
		int FileImgInx = IMAGEINX_UNKFILE_NOTVCD;
		if (m_vecProjectItems[nInx].strFilename.find(".py") != string::npos)
		{
			FileImgInx = IMAGEINX_PYFILE_NOTVCD;
		}
		else if (m_vecProjectItems[nInx].strFilename.find(".xml") != string::npos)
		{
			FileImgInx = IMAGEINX_XMLFILE_NOTVCD;
		}
		else if (m_vecProjectItems[nInx].strFilename.find(".bat") != string::npos)
		{
			FileImgInx = IMAGEINX_BATFILE_NOTVCD;
		}

		if (m_vecProjectItems[nInx].status.bControlled)
		{
			if (m_vecProjectItems[nInx].status.bOpenForEdit)
				FileImgInx += 2;
			else
				FileImgInx += 1;
		}

		if (m_vecProjectItems[nInx].status.bNewerInSc)
		{
			FileImgInx += 2;
		}

		m_ProjectTree.SetItemImage(hItem, FileImgInx, FileImgInx);

		if (m_vecProjectItems[nInx].bBootFile)
		{
			m_ProjectTree.SetItemState(hItem, TVIS_BOLD, TVIS_BOLD);
		}
	}
}

void CProjectViewBar::UpdateProjectItemStatus()
{
	int ProjImageInx = IMAGEINX_PROJ_NOTVCD;
	if (m_ProjectFileStatus.bControlled)
	{
		if (m_ProjectFileStatus.bOpenForEdit)
			ProjImageInx = IMAGEINX_PROJ_CHEKDOUT;
		else
			ProjImageInx = IMAGEINX_PROJ_CHEKDIN;
	}
	
	m_ProjectTree.SetItemImage(m_ProjectTree.GetRootItem(), ProjImageInx, ProjImageInx+1);
}

void CProjectViewBar::OnFilepopupCheckout() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				//string CheckFile = pProject->GetProjectPath();
				//CheckFile += m_vecProjectItems[nInx].strFilename;
				string CheckFile = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);

				pSci->CheckFileOut(CheckFile.c_str());

				m_vecProjectItems[nInx].status = pSci->GetFileStatus(CheckFile.c_str());
				
				UpdateFileItemStatus(hSelItem);
			}
			else
			{
				//check out the project file
				string CheckProj = pProject->GetProjectPath();
				CheckProj += pProject->GetProjectFile();
				pSci->CheckFileOut(CheckProj.c_str());
				m_ProjectFileStatus = pSci->GetFileStatus(CheckProj.c_str());
				
				UpdateProjectItemStatus();
			}
		}
	}
}

void CProjectViewBar::OnFilepopupUndocheckout() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				//string CheckFile = GetFilename(nInx);
				string CheckFile = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);
			
				pSci->UndoCheckOut(CheckFile.c_str());

				m_vecProjectItems[nInx].status = pSci->GetFileStatus(CheckFile.c_str());
				
				UpdateFileItemStatus(hSelItem);
				
				//reload the document file
				((CDbgRemoteApp*)AfxGetApp())->ReloadDocumentFile(CheckFile.c_str());
			}
			else
			{
				//the user has selected the project file (or one of the groups)
				//check out the project file
				string CheckProj = pProject->GetProjectPath();
				CheckProj += pProject->GetProjectFile();
				pSci->UndoCheckOut(CheckProj.c_str());
				m_ProjectFileStatus = pSci->GetFileStatus(CheckProj.c_str());
				
				//reload the project file
				pProject->LoadFromFile(CheckProj.c_str());

				UpdateProjectDisplay(pProject);
			}
		}
	}
}


void CProjectViewBar::OnFilepopupDiffFile() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				//string CheckFile = GetFilename(nInx);
				string CheckFile = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);
			
				pSci->DiffFile(CheckFile.c_str());
			}
		}
	}
}

void CProjectViewBar::OnFilepopupSyncFile() 
{
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);
	CSourceControlInterface* pSci = pProject->GetSci();
	if (pSci)
	{
		HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
		if (hSelItem)
		{		
			int nInx = m_ProjectTree.GetItemData(hSelItem);
			if (nInx > -1)
			{
				//string CheckFile = GetFilename(nInx);
				string CheckFile = pProject->GetFilePath(m_vecProjectItems[nInx].nInx);
			
				pSci->SyncFile(CheckFile.c_str());

				m_vecProjectItems[nInx].status = pSci->GetFileStatus(CheckFile.c_str());
				
				UpdateFileItemStatus(hSelItem);
				
				//reload the document file
				((CDbgRemoteApp*)AfxGetApp())->ReloadDocumentFile(CheckFile.c_str());
			}
		}
	}
}


void CProjectViewBar::OnFilepopupSetGroup()
{
	CFileGroup dlg;
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	assert(pProject);

	HTREEITEM hSelItem = m_ProjectTree.GetSelectedItem();
	if (hSelItem)
	{
		
		int nInx = m_ProjectTree.GetItemData(hSelItem);
		dlg.m_strFileGroup = pProject->GetFileGroup(m_vecProjectItems[nInx].strFilename).c_str();
		
		if (dlg.DoModal() == IDOK)
		{
			pProject->SetFileGroup(m_vecProjectItems[nInx].strFilename,(const char*)dlg.m_strFileGroup);

			((CDbgRemoteApp*)AfxGetApp())->SaveProjectFile();

			UpdateProjectDisplay(pProject);
		}
	}
}


void CProjectViewBar::OnProjectBeginDrag(NMTREEVIEW* pNmTv)
{
    if (pNmTv->itemNew.lParam < 0)
	{
		//only drag files (not groups)
		return;
	}
	m_nDragInx = pNmTv->itemNew.lParam;
	
	RECT rcItem;        // bounding rectangle of item 
    DWORD dwLevel;      // heading level of item 
    DWORD dwIndent;     // amount that child items are indented 
 
    // Tell the tree-view control to create an image to use 
    // for dragging. 
	if (m_pDragImgList)
		delete m_pDragImgList;

	m_pDragImgList = m_ProjectTree.CreateDragImage(pNmTv->itemNew.hItem);
 
    // Get the bounding rectangle of the item being dragged. 
	m_ProjectTree.GetItemRect(pNmTv->itemNew.hItem, &rcItem, TRUE); 
 
    // Get the heading level and the amount that the child items are 
    // indented. 
    dwLevel = pNmTv->itemNew.lParam; 
    dwIndent = m_ProjectTree.GetIndent();
	
 
    // Start the drag operation.
	m_pDragImgList->BeginDrag(0, CPoint(0,0));
	m_pDragImgList->DragEnter(&m_ProjectTree, CPoint(pNmTv->ptDrag.x, pNmTv->ptDrag.y));

 
    // Hide the mouse pointer, and direct mouse input to the 
    // parent window. 
    ShowCursor(FALSE); 
    SetCapture();
    m_bDragging = true; 
}


void CProjectViewBar::OnMouseMove(UINT nFlags, CPoint point) 
{
	if ((m_bDragging) && ((nFlags & MK_LBUTTON) == 0))
	{
		OnProjectEndDrag();
	}
	else if (m_bDragging) 
	{
		assert(m_pDragImgList);

		// Drag the item to the current position of the mouse pointer. 
		m_pDragImgList->DragMove(point);

		
		//wicked code August 1998
		
		// Find out which item (if any) the cursor is over.
		UINT nFlags;
		HTREEITEM htiTarget = m_ProjectTree.HitTest (point, &nFlags);


		// Highlight the item, or unhighlight all items if the cursor isn't
		// over an item.
		m_pDragImgList->DragShowNolock (FALSE);
		m_ProjectTree.SelectDropTarget (htiTarget);
		m_pDragImgList->DragShowNolock (TRUE);

		
		// Modify the cursor to provide visual feedback to the user.
		// Note: It's important to do this AFTER the call to DragMove.
		::SetCursor (htiTarget == NULL ?
					 AfxGetApp ()->LoadStandardCursor (IDC_NO) :
					 (HCURSOR) ::GetClassLong (m_hWnd, GCL_HCURSOR));

    } 
	
	CSizingControlBar::OnMouseMove(nFlags, point);
}

void CProjectViewBar::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (m_bDragging) 
	{ 
        OnProjectEndDrag();
    }

	CSizingControlBar::OnLButtonUp(nFlags, point);
}


void CProjectViewBar::OnProjectEndDrag()
{
	assert(m_pDragImgList);

	HTREEITEM hItem = m_ProjectTree.GetDropHilightItem();
	
	m_pDragImgList->DragLeave (&m_ProjectTree);
    m_pDragImgList->EndDrag();
	
	ReleaseCapture(); 
    ShowCursor(TRUE); 
    
	delete m_pDragImgList;
	m_pDragImgList = 0;


	if (hItem)
	{
		//if the item is a file, get its parent - it will either be a group or the root
		if (m_ProjectTree.GetItemData(hItem) > -1)
			hItem = m_ProjectTree.GetParentItem(hItem);
		
		string GroupName = "";
		if (m_ProjectTree.GetRootItem() != hItem)
		{
			GroupName = m_ProjectTree.GetItemText(hItem);
		}
		if (m_vecProjectItems[m_nDragInx].strGroup != GroupName)
		{
			assert(-1 < m_nDragInx);

			CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
			assert(pProject);
			
			string groupPath = GetGroupPath(hItem);

			pProject->SetFileGroup(m_vecProjectItems[m_nDragInx].strFilename, groupPath);

			((CDbgRemoteApp*)AfxGetApp())->SaveProjectFile();

			UpdateProjectDisplay(pProject);			
		}
	}

	m_bDragging = false; 
	m_nDragInx = -1;
}


void CProjectViewBar::OnProjectKeyDown(NMTVKEYDOWN* pKeyDown)
{
	if (pKeyDown->wVKey == VK_DELETE)
	{
		OnRemoveFromProject();
	}
}


void CProjectViewBar::OnRemoveFromProject() 
{
	HTREEITEM hItem = m_ProjectTree.GetSelectedItem();
	if (hItem)
	{
		CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
		
		int ItemData = m_ProjectTree.GetItemData(hItem);
		if (ItemData > -1)
		{
			//remove the selected file from the project
			int nInx = m_ProjectTree.GetItemData(hItem);
			pProject->RemoveFile(m_vecProjectItems[nInx].strFilename);
			((CDbgRemoteApp*)AfxGetApp())->SaveProjectFile();
		}
		else if (m_ProjectTree.GetRootItem() != hItem)
		{
			//remove the group
			if (m_ProjectTree.ItemHasChildren(hItem))
			{
				int msgBox = AfxMessageBox("Remove grouped files as well?", MB_YESNOCANCEL|MB_ICONQUESTION);
				if (msgBox == IDCANCEL)
					return;
				if (msgBox == IDYES)
				{
					HTREEITEM hFile = m_ProjectTree.GetChildItem(hItem);
					while (hFile)
					{
						int nInx = m_ProjectTree.GetItemData(hFile);
						pProject->RemoveFile(m_vecProjectItems[nInx].strFilename);
						hFile = m_ProjectTree.GetNextSiblingItem(hFile);
					}
				}
			}
			string groupPath = GetGroupPath(hItem);

			pProject->RemoveGroup(groupPath.c_str());
			((CDbgRemoteApp*)AfxGetApp())->SaveProjectFile();
		}
		else
			return;

		UpdateProjectDisplay(pProject);
	}	
}

string CProjectViewBar::GetGroupPath(HTREEITEM hItem)
{
	string groupPath = m_ProjectTree.GetItemText(hItem);
	HTREEITEM hPar = m_ProjectTree.GetParentItem(hItem);
	while (hPar != m_ProjectTree.GetRootItem())
	{
		string newPath = m_ProjectTree.GetItemText(hPar);
		newPath += '/';
		groupPath = newPath + groupPath;
		hPar = m_ProjectTree.GetParentItem(hPar);
	}
	return groupPath;
}

string CProjectViewBar::GetFilename(int ProjFileInx)
{
	assert((ProjFileInx >= 0) && (ProjFileInx < int(m_vecProjectItems.size())));
	string Filename = m_vecProjectItems[ProjFileInx].strFilename;
					
	//add the project path to the filename
	CPdpManager* pProject = ((CDbgRemoteApp*)AfxGetApp())->pGetProject();
	string PathFile = pProject->GetProjectPath();
	if (Filename.find(':') == string::npos)
	{
		// No colons found - must be a relative path name.
		PathFile += Filename;
	}
	else
	{
		// Colons found - assume to be an absolute path.
		PathFile = Filename;
	}
	return PathFile;
}


void CProjectViewBar::UpdateStyles()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CUserCfg::SEditorStyle projectStyle;
	CUserCfg::SEditorStyle modulesStyle;

	pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_PROJECTVW_PROJTREE, &projectStyle);
	pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_PROJECTVW_MODLIST, &modulesStyle);

	//to change the color of the project tree
	m_ProjectTree.SetBkColor(projectStyle.BackCol);
	m_ProjectTree.SetTextColor(projectStyle.ForeCol);
	m_ProjectTree.ModifyStyle( TVS_HASLINES, 0, 0 ); 
	m_ProjectTree.ModifyStyle( 0, TVS_HASLINES, 0 );
	
	//to change the color of the module tree
	m_ModuleTree.SetBkColor(modulesStyle.BackCol);
	m_ModuleTree.SetTextColor(modulesStyle.ForeCol);
	m_ModuleTree.ModifyStyle( TVS_HASLINES, 0, 0 ); 
	m_ModuleTree.ModifyStyle( 0, TVS_HASLINES, 0 );

	//BUGBUG - this only creates a font that is the right face and size, it ignores, the other style params
	m_ProjectFont.CreatePointFont(projectStyle.PointSize * 10, projectStyle.Facename);
	m_ProjectFont.DeleteObject();
	m_ProjectTree.SetFont(&m_ProjectFont);

	m_ModulesFont.DeleteObject();
	m_ModulesFont.CreatePointFont(modulesStyle.PointSize * 10, modulesStyle.Facename);
    m_ModuleTree.SetFont(&m_ModulesFont);
	
}


void CProjectViewBar::GetExpandedNodes(HTREEITEM hItem, set<string>& nodes)
{
	HTREEITEM hCur = m_ProjectTree.GetChildItem(hItem);
	while (hCur)
	{
		if (m_ProjectTree.GetItemData(hCur) == -1)
		{
			if (m_ProjectTree.GetItemState(hCur, TVIS_EXPANDED))
			{
				nodes.insert((const char*)m_ProjectTree.GetItemText(hCur));
				
				GetExpandedNodes(hCur, nodes);
			}
		}
		
		hCur = m_ProjectTree.GetNextSiblingItem(hCur);
	}
}

void CProjectViewBar::ExpandNodes(HTREEITEM hItem, set<string>& nodes)
{
	HTREEITEM hCur = m_ProjectTree.GetChildItem(hItem);
	while (hCur)
	{
		if (m_ProjectTree.GetItemData(hCur) == -1)
		{
			string curNodeName = (const char*)m_ProjectTree.GetItemText(hCur);
			if (nodes.find(curNodeName) != nodes.end())
			{
				m_ProjectTree.Expand(hCur, TVE_EXPAND);
				ExpandNodes(hCur, nodes);
			}
		}
		
		hCur = m_ProjectTree.GetNextSiblingItem(hCur);
	}
}



