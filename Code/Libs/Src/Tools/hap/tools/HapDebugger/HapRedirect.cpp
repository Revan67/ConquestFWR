//////////////////////////////////////////////////////////////////////////////
//  File:    HapRedirect.cpp
//
//  Purpose: 
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
#include "HapRedirect.h"
#include "DbgRemote.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHapRedirect 

void CHapRedirect::OnChildStarted(LPCSTR lpszCmdLine) 
{
#ifdef _DEBUG
	((CDbgRemoteApp*)AfxGetApp())->AddOutput("starting: ");
	((CDbgRemoteApp*)AfxGetApp())->AddOutput(lpszCmdLine);
	((CDbgRemoteApp*)AfxGetApp())->AddOutput("\n");
#endif 
}
void CHapRedirect::OnChildStdOutWrite(LPCSTR lpszOutput) 
{
	((CDbgRemoteApp*)AfxGetApp())->AddOutput(lpszOutput);
}
void CHapRedirect::OnChildStdErrWrite(LPCSTR lpszOutput) 
{
	((CDbgRemoteApp*)AfxGetApp())->AddTrace(lpszOutput);
}
void CHapRedirect::OnChildTerminate() 
{
#ifdef _DEBUG
	((CDbgRemoteApp*)AfxGetApp())->AddTrace("child process complete\n");
#endif
}
