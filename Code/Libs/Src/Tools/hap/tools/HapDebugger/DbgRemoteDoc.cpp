//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemoteDoc.cpp
//
//  Purpose: implementation of the CDbgRemoteDoc class - which holds a single 
//  python source file.
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
#include "DbgRemoteView.h"
#include "RunOptionsDlg.h"
#include "FileModifiedDlg.h"
#include "FilePropdlg.H"

#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
//	MFC message maps for CDbgRemoteDoc
//-----------------------------------------------------------------------------

IMPLEMENT_DYNCREATE(CDbgRemoteDoc, CDocument)

BEGIN_MESSAGE_MAP(CDbgRemoteDoc, CDocument)
	//{{AFX_MSG_MAP(CDbgRemoteDoc)
	ON_COMMAND(ID_DEBUG_TESTCOMPILE, OnDebugTestCompile)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
//	CDbgRemoteDoc implementation
//-----------------------------------------------------------------------------

CDbgRemoteDoc::CDbgRemoteDoc()
{
	m_pSciDoc = 0;
}

CDbgRemoteDoc::~CDbgRemoteDoc()
{
}

void* CDbgRemoteDoc::GetSciDocPtr()
{
	return m_pSciDoc;
}
void CDbgRemoteDoc::SetSciDocPtr(void* pDoc)
{
	m_pSciDoc = pDoc;
}

bool CDbgRemoteDoc::ReadOnly() const
{
	if (m_strFileName.GetLength())
	{
		DWORD dwFileAttributes = ::GetFileAttributes(m_strFileName);

		if (dwFileAttributes != -1)
		{
			return (dwFileAttributes & FILE_ATTRIBUTE_READONLY);
		}
	}

	return false;
}

void CDbgRemoteDoc::ToggleBreakpoint(int nLine)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	pApp->ToggleBreakpoint(GetPathName(), nLine);
}

void CDbgRemoteDoc::UpdateBreakpoints(const char* cszBrkptList)
{
	POSITION pos = GetFirstViewPosition();
	assert(pos);
	CDbgRemoteView* pVw = (CDbgRemoteView*) GetNextView(pos);
	assert(pVw);
	pVw->ClearBreakpoints();

	const char* pCur = cszBrkptList;
	//number, linenumber,filename[,expression]
	while (strlen(pCur))
	{
		char* pNext;
		int BpNum = strtol(pCur, &pNext, 10);
		if (pNext)
		{
			pNext++;
			int Line = strtol(pNext, &pNext, 10);
			if (pNext)
			{
				pNext++;
				if (GetPathName().CompareNoCase(pNext) == 0)
				{
					pVw->SetBreakpoint(Line);
				}
			}
		}
		
		pCur += strlen(pCur)+1;
	}
}

bool CDbgRemoteDoc::GetEditorText(char_vector& fileData) 
{
	// connect to view and get the editor buffer

	POSITION pos = GetFirstViewPosition();

	if (0 == pos)
	{
		dprintf("Warning: unable to get editor data - no views found\n");
		return false;
	}

	CDbgRemoteView* pView = reinterpret_cast<CDbgRemoteView*>(GetNextView(pos));
	pView->GetEditorText(fileData);

	return true;
}

#ifdef _DEBUG

void CDbgRemoteDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDbgRemoteDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}

#endif //_DEBUG

//-----------------------------------------------------------------------------
//	CDocument overrides
//-----------------------------------------------------------------------------


BOOL CDbgRemoteDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	LPARAM updateHint = HINT_DOC_OPENED;
	if (lpszPathName)
	{
		bool bReload = (stricmp(GetPathName(), lpszPathName) == 0);
		if (bReload)
		{
			//we are reloading the file, save breakpoints, current position and bookmarks
			updateHint = HINT_DOC_REOPENED;
		}
	}


	// open the file
	ifstream ifs(lpszPathName, ios::binary|ios::in);

	if (!ifs)
	{
		//don't display an "Unable to open" message here

		//ostringstream oss;
		//oss << "Unable to open '" << lpszPathName << "' for reading.";
		//AfxMessageBox(oss.str().c_str());
		return false;
	}

	StoreFileModTime(lpszPathName);

	//update the document name
	m_strFileName = lpszPathName;
	
	ifs.seekg(0, ios_base::end);
	int fileLength = ifs.tellg();
	ifs.seekg(0, ios_base::beg);

	// read file data into a vector

	char_vector fileData;
	fileData.resize(fileLength + 1);

	ifs.read(&fileData[0], fileLength);

	int readLength = ifs.gcount();

	// terminate vector with a null character and signal views to update,
	// passing this data as a standard C string - this is out of the ordinary
	// for Doc/View architecture, but since the Scintilla control takes care
	// of the data management, there's not a lot for the Doc to do

	fileData[readLength] = '\0';
	UpdateAllViews(0, updateHint, reinterpret_cast<CObject*>(&fileData[0]));

	//clear the modified flag

	SetModifiedFlag(false);

	return true;
}


void CDbgRemoteDoc::OnFileSave() 
{	
	if (HasChangedOnDisk())
	{
		CFileModifiedDlg dlg;
		dlg.m_Filename = GetPathName();
		int ret = dlg.DoModal();
		if (ret == IDOK)
			//reload the document
			OnOpenDocument(GetPathName());
		else 
			//save as
			OnFileSaveAs();
	}
	else
	{
		DWORD dwAttrib = GetFileAttributes(GetPathName());
		if (dwAttrib & FILE_ATTRIBUTE_READONLY)
		{
			// we do not have read-write access or the file does not (now) exist
			OnFileSaveAs();
		}
		else
		{
		
			CWaitCursor wait;

			if (!OnSaveDocument(GetPathName()))
			{
				/*// be sure to delete the file
				TRY
				{
					CFile::Remove(newName);
				}
				CATCH_ALL(e)
				{
					TRACE(traceAppMsg, 0, "Warning: failed to delete file after failed SaveAs.\n");
					DELETE_EXCEPTION(e);
				}
				END_CATCH_ALL*/
			}
		}

	}
}


void CDbgRemoteDoc::OnFileSaveAs()
{
	CString strFilter = "Python Files (*.py)";
	strFilter += '\0';
	strFilter += "*.py";
	strFilter += '\0';

	strFilter += "XML Files (*.xml)";
	strFilter += '\0';
	strFilter += "*.xml";
	strFilter += '\0';

	strFilter += "Text Files (*.txt)";
	strFilter += '\0';
	strFilter += "*.txt";
	strFilter += '\0';

	
	// append the "*.*" all files filter
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
	strFilter += allFilter;
	strFilter += (TCHAR)'\0';   // next string please
	strFilter += _T("*.*");
	strFilter += (TCHAR)'\0';   // last string

	vector<char>filenm;
	filenm.resize(MAX_PATH);
	memset(&(filenm[0]), 0, filenm.size());

	vector<char> filetitle;
	filetitle.resize(MAX_PATH);
	memset(&(filetitle[0]), 0, filetitle.size());
	
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = AfxGetMainWnd()->m_hWnd; 
	ofn.lpstrFilter = strFilter;
	ofn.nFilterIndex = 1; 
	ofn.lpstrFile = &(filenm[0]); 
	ofn.nMaxFile = filenm.size();
	ofn.lpstrFileTitle = &(filetitle[0]); 
	ofn.nMaxFileTitle = filetitle.size(); 
	ofn.lpstrInitialDir = 0; 
	ofn.lpstrTitle = "Save File As"; 
	ofn.Flags = OFN_HIDEREADONLY|OFN_FILEMUSTEXIST; 
	ofn.lpstrDefExt = ""; 

	string defPath = GetPathName();
	if (defPath.length() > 0)
	{
		ofn.lpstrInitialDir = defPath.c_str();
		strncpy(&(filenm[0]), defPath.c_str(), MAX_PATH);

		int extInx = defPath.rfind('.');
		if (extInx != defPath.npos)
		{
			string ext = defPath.substr(extInx);
			ext = strlwr( (char*)ext.c_str());
			if (ext == ".py")
			{
				ofn.nFilterIndex = 1;
			}
			else if (ext == ".xml")
			{
				ofn.nFilterIndex = 2;
			}
			else if (ext == ".txt")
			{
				ofn.nFilterIndex = 3;
			}
			else
			{
				ofn.nFilterIndex = 4;
			}
		}
	}
	else
	{
		//BUGBUG - set a default path based on the project or last saved file.
	}
	
	
	if (::GetSaveFileName(&ofn))
	{
		OnSaveDocument(ofn.lpstrFile);
	}
}

BOOL CDbgRemoteDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	if ((!lpszPathName) || (strlen(lpszPathName) == 0))
		lpszPathName = GetPathName();


	// connect to the view and get the editor buffer
	char_vector fileData;

	if (!GetEditorText(fileData))
	{
		return false;
	}
	
	// open the file
	ofstream ofs(lpszPathName, ios::binary|ios::out);

	if (!ofs)
	{
		ostringstream oss;
		oss << "Unable to open '" << lpszPathName << "' for writing.";
		AfxMessageBox(oss.str().c_str());
		return false;
	}

	((CDbgRemoteApp*)AfxGetApp())->IgnoreFileChanges(true);

	vector<char>::iterator itr = fileData.begin();
	
	while (itr != fileData.end())
	{
		//remove any null characters
		if (*itr == 0x00)
		{
			itr = fileData.erase(itr);
		}
		else
			itr++;
	}
	
	// write the editor buffer to disk
	if (!ofs.write(&fileData[0], fileData.size()))
	{
		((CDbgRemoteApp*)AfxGetApp())->IgnoreFileChanges(false);
		return false;
	}
	ofs.flush();
	ofs.close();

	// let the view know that it needs to update the save position in the undo buffer
	UpdateAllViews(0, HINT_DOC_SAVED);

	// clear modified flag
	m_strFileName = lpszPathName;
	SetPathName(lpszPathName);
	SetModifiedFlag(false);
	StoreFileModTime(lpszPathName);

	((CDbgRemoteApp*)AfxGetApp())->IgnoreFileChanges(false);
	return true;	

} // OnSaveDocument

void CDbgRemoteDoc::SetModifiedFlag(BOOL bModified)
{
	// append an asterisk to the file name in the title bar for modified files

	char szFileTitle[_MAX_PATH];
	char szFile[_MAX_FNAME];
	char szExt[_MAX_EXT];

	_splitpath(m_strFileName, 0, 0, szFile, szExt);
	_makepath(szFileTitle, 0, 0, szFile, szExt);

	CString strNewTitle = szFileTitle;

	if (bModified)
	{
		strNewTitle += " *";
	}

	SetTitle(strNewTitle);

	// call the base class to update the modified flag

	CDocument::SetModifiedFlag(bModified);
}

string ApplicationDirectory()
{
	string ModPath;
	ModPath.resize(MAX_PATH);
	if (GetModuleFileName(0, &ModPath[0], MAX_PATH) > 0)
	{
		int nEndSlash = ModPath.rfind('\\');
		if (nEndSlash > -1)
			ModPath.resize(nEndSlash);
	}
	return ModPath;
}


void CDbgRemoteDoc::OnDebugTestCompile() 
{
	OnFileSave();

	CDbgRemoteApp* pApp = ((CDbgRemoteApp*)AfxGetApp());
	CString path = GetPathName();
	path.Replace("\\", "/");
	char buff[1024];
	_snprintf(buff,1024,"python.exe -c \"import py_compile\npy_compile.compile('%s')\"", path);
	buff[1023] = 0;
	pApp->RunChildProcess(buff, "");

	// connect to the view and get the editor buffer

	/*BUGBUG naj
	char_vector editorData;

	if (GetEditorText(editorData))
	{
		// strip carriage returns from editor buffer

		char_vector::const_iterator itSrc = editorData.begin();
		char_vector::iterator itDst = editorData.begin();

		while (itSrc != editorData.end())
		{
			if (*itSrc != '\r')
			{
				*itDst++ = *itSrc;
			}

			++itSrc;
		}

		editorData.resize(itDst - editorData.begin());

		CString PathName = GetPathName();
		


		Py_Initialize();
		
		char* pPath = (char*)((const char*)PathName);
		PySys_SetArgv(1, &pPath);


			
		
		pApp->AddTrace("Compiling File: ");
		pApp->AddTrace(PathName);
		pApp->AddTrace("\n");

		string ErrLog = ApplicationDirectory();
		ErrLog.append("BuildLog.txt");


		FILE* ErrFile = fopen(ErrLog.c_str(), "w+t");
		assert(ErrFile);

		PyObject* pPyStdErr = PyFile_FromFile(ErrFile, "<stderr>", "w", NULL);
		assert(pPyStdErr);
		PySys_SetObject("stderr", pPyStdErr);

		//if (Py_CompileString(m_vectTextBuffer.begin(), (char*)((const char*)PathName), Py_file_input) == NULL)
		if (Py_CompileString(&editorData[0], (char*)((const char*)PathName), Py_file_input) == NULL)
		{
			PyErr_Print();	
			
			fflush(ErrFile);
			fseek(ErrFile, 0, SEEK_END);
			int Size = ftell(ErrFile);
			rewind(ErrFile);
				
			string ErrBuff;
			ErrBuff.resize(Size);
			fread(&ErrBuff[0], 1, Size, ErrFile);

			pApp->AddTrace(ErrBuff.c_str());
		}
		else
		{
			pApp->AddTrace("Compile Complete\n");
		}
		
		Py_Finalize();
		fclose(ErrFile);

  

	}*/
}




void CDbgRemoteDoc::StoreFileModTime(const char* cszFilePath)
{
	HANDLE hFile = CreateFile(cszFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		GetFileTime(hFile, 0, 0, &m_LastModifiedTime);
		CloseHandle(hFile);
	}
	else
	{
		dprintf("CreateFile Failed: %d\n", GetLastError());
	}
}

bool CDbgRemoteDoc::HasChangedOnDisk()const
{
	HANDLE hFile = CreateFile(GetPathName(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		FILETIME ft;
		GetFileTime(hFile, 0, 0, &ft);
		
		bool bChanged = (0 != CompareFileTime(&m_LastModifiedTime, &ft));
		CloseHandle(hFile);
		
		return bChanged;
	}
	else
		dprintf("Unable to check file time: %s\n", GetPathName());

	return false;
}

void CDbgRemoteDoc::ShowProperties(void)
{
	string FilePath = GetPathName();
	CFilePropDlg dlg;

	dlg.ShowFileProperties(FilePath.c_str());
}

// function added 10-18-02 Bruce Dickey
void CDbgRemoteDoc::AddBreakpoint(int nLine)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	pApp->AddBreakpoint(GetPathName(), nLine);
}

// function added 10-18-02 Bruce Dickey
void CDbgRemoteDoc::RemoveBreakpoint(int nLine)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	pApp->RemoveBreakpoint(GetPathName(), nLine);
}

// function added 10-19-02 Bruce Dickey
void CDbgRemoteDoc::RemoveAllBreakpointsByFile(void)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	pApp->RemoveAllBreakpointsByFile(GetPathName());
}