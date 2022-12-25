// Breakpoint.h: interface for the CBreakpoint class.
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

#if !defined(AFX_BREAKPOINT_H__40D779E2_9F7D_4A84_B317_FAA4F99C577C__INCLUDED_)
#define AFX_BREAKPOINT_H__40D779E2_9F7D_4A84_B317_FAA4F99C577C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFileBreakpts
{
public:
	CFileBreakpts();
	CFileBreakpts(const CFileBreakpts&);
	CFileBreakpts& operator=(const CFileBreakpts&);
	
	virtual ~CFileBreakpts();

	const char* GetFilename()const;
	void SetFilename(const char* cszFile);

	bool operator ==(const CFileBreakpts& rBp);
	
	bool operator ==(const char* cszFile);
	
	void AddLine(uint32_t Line);
	void RemoveLine(uint32_t Line);
	bool CheckLine(uint32_t Line);

	int GetLinesCount()const;
	int GetLineInx(int LineInx)const;

	bool CheckBreakpoint(const char* cszFile, uint32_t Line);

private:
	vector<char> m_vecFilename;
	char* m_FileNoPath;
	set<uint32_t> m_Linenums;	
};



inline const char* CFileBreakpts::GetFilename()const
{
	return &(m_vecFilename[0]);
}


inline int CFileBreakpts::GetLinesCount()const
{
	return m_Linenums.size();
}

inline bool CFileBreakpts::CheckBreakpoint(const char* cszFile, uint32_t Line)
{
	assert(m_FileNoPath);
	if (m_Linenums.find(Line) != m_Linenums.end())
	{
		const char* pCur = m_FileNoPath;//GetFilename();
		while (*pCur && *cszFile)
		{
			if (*pCur++ != *cszFile++)
				//the filename didn't match
				return false;
		}
		return true;
	}
	return false;
}

inline bool CFileBreakpts::operator ==(const CFileBreakpts& rBp)
{
	const char* pCur = GetFilename();
	const char* pCmp = rBp.GetFilename();
	while (*pCur && *pCmp)
	{
		if (*pCur++ != *pCmp++)
			return false;
	}
	return (*pCur == *pCmp);	
}

inline bool CFileBreakpts::operator ==(const char* cszFile)
{
	const char* pCur = GetFilename();
	while (*pCur && *cszFile)
	{
		if (*pCur++ != *cszFile++)
			return false;
	}
	return (*pCur == *cszFile);
}




#endif // !defined(AFX_BREAKPOINT_H__40D779E2_9F7D_4A84_B317_FAA4F99C577C__INCLUDED_)
