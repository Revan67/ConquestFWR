//////////////////////////////////////////////////////////////////////////////
//  File:    SourceControlInterface.cpp
//
//  Purpose: This is a class which implements a generic (currently unimplemented)
//  interface to a source control provider.
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

// SourceControlInterface.cpp: implementation of the CSourceControlInterface class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dbgremote.h"
#include "SourceControlInterface.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


CSourceControlInterface::CSourceControlInterface()
{
	m_hWrapMod = 0;
	SetNotifyFn = 0;
	GetFileStatusFn = 0;
	CheckFileOutFn = 0;
	RevertFileFn = 0;
	CheckFileInFn = 0;
	AddFileFn = 0;
	DiffFileFn = 0;
	SyncFileFn = 0;

	m_hWndNotify = 0;
}

CSourceControlInterface::~CSourceControlInterface()
{
	if (m_hWrapMod)
		FreeLibrary(m_hWrapMod);
}

bool CSourceControlInterface::IsOpen()
{
	return (m_hWrapMod != 0);
}

bool CSourceControlInterface::Open(const char* cszSourceControlPath, HWND hwndNotify)
{
	if (m_hWrapMod)
		FreeLibrary(m_hWrapMod);

	m_hWrapMod = LoadLibrary(cszSourceControlPath);

	m_hWndNotify = hwndNotify;
	
	if (m_hWrapMod)
	{
		SetNotifyFn = (TFnSetNotifyWnd)GetProcAddress(m_hWrapMod, FNNAME_SETNOTIFYHWND);
		GetFileStatusFn = (TFnGetFileStatus)GetProcAddress(m_hWrapMod, FNNAME_FILESTATUS);
		CheckFileOutFn = (TFnCheckFileOut)GetProcAddress(m_hWrapMod, FNNAME_CHECKOUT);
		RevertFileFn = (TFnRevertFile)GetProcAddress(m_hWrapMod, FNNAME_REVERT);
		CheckFileInFn = (TFnCheckFileIn)GetProcAddress(m_hWrapMod, FNNAME_CHECKIN);
		AddFileFn = (TFnAddFile)GetProcAddress(m_hWrapMod, FNNAME_ADD);
		DiffFileFn = (TFnDiffFile)GetProcAddress(m_hWrapMod, FNNAME_DIFF);
		SyncFileFn = (TFnSyncFile)GetProcAddress(m_hWrapMod, FNNAME_SYNC);
		
		if (GetFileStatusFn && CheckFileOutFn && RevertFileFn && 
			CheckFileInFn && AddFileFn && DiffFileFn && SyncFileFn && SetNotifyFn)
		{
			SetNotifyFn(hwndNotify);
			return true;
		}
		
		FreeLibrary(m_hWrapMod);
		m_hWrapMod = 0;
	}
	return false;
}

bool CSourceControlInterface::CheckFileIn(const char* cszFile)
{
	if (CheckFileInFn)
	{
		CheckFileInFn(cszFile);
		return true;
	}

	return false;
}

bool CSourceControlInterface::CheckFileOut(const char* cszFile)
{
	if (CheckFileOutFn)
	{
		CheckFileOutFn(cszFile);
		return true;
	}
	return false;
}
bool CSourceControlInterface::UndoCheckOut(const char* cszFile)
{
	if (RevertFileFn)
	{
		RevertFileFn(cszFile);
		return true;
	}
	return false;
}

bool CSourceControlInterface::AddFile(const char* cszFile)
{
	if (AddFileFn)
	{
		AddFileFn(cszFile);
		return true;
	}
	
	return false;
}

SScFileStatus CSourceControlInterface::GetFileStatus(const char* cszFile)
{
	SScFileStatus fs;
	fs.bControlled = false;

	if (GetFileStatusFn)
	{
		GetFileStatusFn(cszFile, &fs);
	}

	return fs;
}

bool CSourceControlInterface::DiffFile(const char* cszFile)
{
	if (DiffFileFn)
	{
		DiffFileFn(cszFile);
		return true;
	}
	
	return false;	
}

bool CSourceControlInterface::SyncFile(const char* cszFile)
{
	if (SyncFileFn)
	{
		SyncFileFn(cszFile);
		return true;
	}
	
	return false;	
}
