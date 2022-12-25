/**************************************************************
* CStringList.hpp: String List Class
*
* Chris N. Haddan
* August 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CSTRINGLIST_H
#define __CSTRINGLIST_H

#include "windows.h"
#include "RegExMatch.hpp"
#include "myassert.h"
#include <list>
#include <algorithm>

using namespace std ;


#define ALLOC_STRING 50


typedef struct tagSTRINGITEM
{
	int		nStringID;
	char	*pszStringValue;
} STRINGITEM;


class CStringList
{
	public:
		CStringList();
		~CStringList();
		bool Create(){ return true;};
		bool AddString (int nStringID, const char *pszStringValue);
		bool DeleteString (int nPos);
		bool InsertString (int nPos, int nStringID, const char *pszStringValue);
		int GetStringCountForRange (int nLow, int nHigh);
		int FindInsertionPoint (int nStringID);
		int FindStringID (int nStringID);
		char *GetStringValue (int nPos);
		int GetStringID (int nPos);
		STRINGITEM *GetStringItem (int nString);
		
		int	GetListCount () { return m_cStringItems; };
		void Dump (void);

	private:
		STRINGITEM **m_StringList;
		int m_cMaxStringItem;
		int m_cStringItems;
};

#endif