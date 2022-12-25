// Breakpoint.cpp: implementation of the CBreakpoint class.
//
//////////////////////////////////////////////////////////////////////
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
#include "Breakpoint.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileBreakpts::CFileBreakpts()
{
	//make a simple empty string
	SetFilename("");
}

CFileBreakpts::CFileBreakpts(const CFileBreakpts& rBp)
{
	SetFilename(rBp.GetFilename());
	for (set<uint32_t>::const_iterator itr = rBp.m_Linenums.begin(); itr != rBp.m_Linenums.end(); itr++)
		m_Linenums.insert(*itr);
}

CFileBreakpts& CFileBreakpts::operator= (const CFileBreakpts& rBp)
{
	SetFilename(rBp.GetFilename());
	m_Linenums.clear();
	for (set<uint32_t>::const_iterator itr = rBp.m_Linenums.begin(); itr != rBp.m_Linenums.end(); itr++)
		m_Linenums.insert(*itr);
	return *this;
}

CFileBreakpts::~CFileBreakpts()
{

}

void CFileBreakpts::SetFilename(const char* cszFile)
{
	m_vecFilename.clear();
	if (cszFile)
	{
		while (*cszFile)
			m_vecFilename.push_back(*cszFile++);
	}
	
	m_vecFilename.push_back(0);

	char* pPathEnd = strrchr(&(m_vecFilename[0]), '\\');
	if (pPathEnd)
		m_FileNoPath = pPathEnd + 1;
	else
		m_FileNoPath = &(m_vecFilename[0]);
}

void CFileBreakpts::AddLine(uint32_t Line)
{
	m_Linenums.insert(Line);
}

void CFileBreakpts::RemoveLine(uint32_t Line)
{
	m_Linenums.erase(Line);
}

bool CFileBreakpts::CheckLine(uint32_t Line)
{
	return (m_Linenums.find(Line) != m_Linenums.end());
}

int CFileBreakpts::GetLineInx(int LineInx)const
{
	int i = 0;
	for (set<uint32_t>::const_iterator pCur = m_Linenums.begin(); pCur != m_Linenums.end(); pCur++)
	{
		if (LineInx == i++)
			return *pCur;
	}
	return -1;
}
