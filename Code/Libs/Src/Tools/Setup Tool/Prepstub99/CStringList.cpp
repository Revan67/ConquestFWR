/**************************************************************
* CStringList.cpp: String List Classs
*
* Chris N. Haddan
* August 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CStringList.hpp"


extern void  _cdecl DS (char *str, ...);

CStringList::CStringList()
{
	ZeroMemory (this, sizeof (CStringList));
}


CStringList::~CStringList()
{
	int i;

	if (m_StringList)
	{
		for (i=0; i < m_cStringItems; i++)
		{
			if (m_StringList[i])
			{
				if (m_StringList[i]->pszStringValue)
				{
					free (m_StringList[i]->pszStringValue);
				}
				free (m_StringList[i]);
			}
		}
		free (m_StringList);
	}
}


bool CStringList::InsertString (int nPos, int nStringID, const char *pszStringValue)
{
	if (nPos < 0 || nPos > m_cStringItems)
		return false;

	// initially add the string to the end of the list.
	if (!AddString (nStringID, pszStringValue))
	{
		return false;
	}

	// get a pointer to the item
	STRINGITEM *pNew = GetStringItem (m_cStringItems - 1);

	if (pNew == NULL)
	{
		return false;
	}

	// shift the list forward from the insertion point
	for (int i=m_cStringItems-1;i>nPos;i--)
	{
		m_StringList[i] = m_StringList[i-1];
	}

	// set the insertion point to the newly added record.
	m_StringList[nPos] = pNew;

	return true;
}


bool CStringList::AddString (int nStringID, const char *pszStringValue)
{
	if (m_cStringItems == m_cMaxStringItem)
	{
		m_cMaxStringItem += ALLOC_STRING;

		if (m_StringList)
		{
			m_StringList = (STRINGITEM **)realloc (m_StringList, sizeof (STRINGITEM *) * m_cMaxStringItem);	
		}
		else
		{
			m_StringList = (STRINGITEM **)malloc (sizeof (STRINGITEM *) * m_cMaxStringItem);
		}
	}

	if (m_StringList == NULL)
		return false;

	m_StringList [m_cStringItems] = (STRINGITEM *)malloc (sizeof (STRINGITEM));
	ZeroMemory (m_StringList [m_cStringItems], sizeof (STRINGITEM));

	if (m_StringList [m_cStringItems])
	{
		m_StringList [m_cStringItems]->nStringID = nStringID;

		// the following line is reported as leaking memory. this is a false warning.
		// it only does it once, and it is on the first item of the 2nd reallocated set.
		m_StringList [m_cStringItems]->pszStringValue = (char *)malloc (lstrlen (pszStringValue) + 1);

		if (m_StringList [m_cStringItems]->pszStringValue)
			lstrcpy (m_StringList [m_cStringItems]->pszStringValue, pszStringValue);

		++m_cStringItems;

		return true;
	}
	else
		return false;
}


bool CStringList::DeleteString (int nPos)
{
	if (nPos < 0 || nPos >= m_cStringItems)
		return false;

	// delete the string item
	if (m_StringList[nPos])
	{
		free (m_StringList[nPos]);
	}

	// shift the list pointers backward one item onto the delete position
	for (int i=nPos;i<m_cStringItems;i++)
	{
		m_StringList[i] = m_StringList[i+1];
	}

	// decrement the list count
	--m_cStringItems;

	return true;
}


STRINGITEM *CStringList::GetStringItem (int nString)
{
	if (nString < 0 || nString >= m_cStringItems)
		return NULL;

	return m_StringList[nString];
}


int CStringList::FindStringID (int nStringID)
{
	for (int i=0; i<m_cStringItems; i++)
	{
		if (nStringID == m_StringList[i]->nStringID)
			return i;
	}
	return -1;
}


int CStringList::FindInsertionPoint (int nStringID)
{
	for (int i=0; i<m_cStringItems; i++)
	{
		if (m_StringList[i]->nStringID >= nStringID)
			return i;
	}
	return i;
}


char *CStringList::GetStringValue (int nPos)
{
	ASSERT (-1 != nPos);
	ASSERT (nPos < m_cStringItems);
	return m_StringList[nPos]->pszStringValue;
}


int CStringList::GetStringID (int nPos)
{
	if (-1==nPos || nPos >= m_cStringItems)
		return -1;

	return m_StringList[nPos]->nStringID;
}

void CStringList::Dump (void)
{
	DS ("StringDump\n");
	DS ("Number of Strings: %d\n", m_cStringItems);	
	DS ("Max Strings: %d\n\n", m_cMaxStringItem);
	DS ("List:\n");

	for (int i=0; i<m_cStringItems; i++)
	{
		DS ("\tString %d='%s'\n", m_StringList[i]->nStringID, m_StringList[i]->pszStringValue);
	}
	DS ("End List.\n");
}


int CStringList::GetStringCountForRange (int nLow, int nHigh)
{
	int nCount = 0;

	for (int i=0; i < GetListCount(); i++)
	{
		if (GetStringID (i) >= nLow && GetStringID (i) <= nHigh)
		{	
			++nCount;
		}
	}
	return nCount;
}
