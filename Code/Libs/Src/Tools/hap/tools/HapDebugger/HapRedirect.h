//////////////////////////////////////////////////////////////////////////////
//  File:    HapRedirect.h
//
//  Purpose: 
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
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


#ifndef __HAPREDIRECT_H__
#define __HAPREDIRECT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "redirect.h"



class CHapRedirect : public CRedirect
{
public:

	CHapRedirect() {}
//	virtual ~CHapRedirect();

private:
	//no copying
	CHapRedirect(const CHapRedirect&);
	CHapRedirect& operator=(const CHapRedirect&);

	
public:
	virtual void OnChildStarted(LPCSTR lpszCmdLine);
	virtual void OnChildStdOutWrite(LPCSTR lpszOutput);
	virtual void OnChildStdErrWrite(LPCSTR lpszOutput);
	virtual void OnChildTerminate();

}; //CHapRedirect



#endif // __HAPREDIRECT_H__
