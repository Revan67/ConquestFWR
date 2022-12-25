//////////////////////////////////////////////////////////////////////////////
//  File:    SourceControlInterface.h
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

#if !defined(AFX_SOURCECONTROLINTERFACE_H__004B55D4_02B5_4D60_A47C_9546B9999961__INCLUDED_)
#define AFX_SOURCECONTROLINTERFACE_H__004B55D4_02B5_4D60_A47C_9546B9999961__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SccGeneric.h"

class CSourceControlInterface  
{
public:
	CSourceControlInterface();
	virtual ~CSourceControlInterface();
	
	virtual bool Open(const char* cszSourceControlPath, HWND hwndNotify = 0);
	virtual bool IsOpen();

	virtual bool CheckFileIn(const char* cszFile);
	virtual bool CheckFileOut(const char* cszFile);
	virtual bool UndoCheckOut(const char* cszFile);
	virtual bool AddFile(const char* cszFile);
	virtual bool DiffFile(const char* cszFile);
	virtual bool SyncFile(const char* cszFile);
	virtual SScFileStatus GetFileStatus(const char* cszFile); 

private:

	HMODULE	m_hWrapMod;
	TFnSetNotifyWnd SetNotifyFn;
	TFnGetFileStatus GetFileStatusFn;
	TFnCheckFileOut CheckFileOutFn;
	TFnRevertFile RevertFileFn;
	TFnCheckFileIn CheckFileInFn;
	TFnAddFile AddFileFn;
	TFnDiffFile DiffFileFn;
	TFnSyncFile SyncFileFn;

	HWND m_hWndNotify;


};

#endif // !defined(AFX_SOURCECONTROLINTERFACE_H__004B55D4_02B5_4D60_A47C_9546B9999961__INCLUDED_)
