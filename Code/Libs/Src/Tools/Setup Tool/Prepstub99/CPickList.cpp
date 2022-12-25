/**************************************************************
* CPickList.cpp: Recent File List  (picklist)
*
* Chris N. Haddan
* October 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "windows.h"
#include "CPickList.hpp"
#include <list>
#include "ctype.h"
#include "resource.h"
#include "myassert.h"
using namespace std ;

extern HINSTANCE g_hInst;
extern string GetPathFromFileName (string s);
extern string GetFileNameFromPath (string s);


DWORD GetKey (char szKey[], char szValue[], DWORD def)
{	
	UINT dwSize = sizeof (DWORD);
	HKEY hkey;
	DWORD dwTemp = 0;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_ALL_ACCESS,  (HKEY FAR*)&hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx (hkey, szValue, NULL, NULL, (LPBYTE)&dwTemp, (LPDWORD)&dwSize) == ERROR_SUCCESS)
		{
			RegCloseKey (hkey);
			return (dwTemp);
		}
	}

	return (def);
}

bool GetKey (char szKey[], char szValue[], char *szString, int strLen)
{	
	HKEY hkey;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_ALL_ACCESS,  (HKEY FAR*)&hkey) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx (hkey, szValue, NULL, NULL, (LPBYTE)szString, (LPDWORD)&strLen) == ERROR_SUCCESS)
		{
			RegCloseKey (hkey);
			return true;
		}
		else
		{
			RegCloseKey (hkey);
			return false;
		}
	}
	else
	{
		RegCloseKey (hkey);
		return false;
	}
}

void SetKey (char szKey[], char szValue [], DWORD value)
{
	HKEY hkey;
	DWORD dwResult;

	RegCreateKeyEx (HKEY_CURRENT_USER, szKey, 0, "", REG_OPTION_NON_VOLATILE, 
					KEY_ALL_ACCESS, NULL, &hkey, &dwResult);
	RegSetValueEx(hkey, szValue, 0, REG_DWORD, (const unsigned char *)&value, 
					sizeof (DWORD));

	RegCloseKey (hkey);
}


void SetKey (char szKey[], char szValue [], char *szString)
{
	DWORD dwResult, dwErr;
	HKEY hkey;

	dwErr = RegCreateKeyEx (HKEY_CURRENT_USER, szKey, 0, "", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &dwResult);

	if (dwErr==ERROR_SUCCESS)
	{
		RegSetValueEx(hkey, szValue, 0, REG_SZ, (const unsigned char *)szString, lstrlen (szString));
		RegCloseKey (hkey);
	}
}

void DeleteKey (char szKey[], char szValue[])
{
	HKEY hkey;
	if (RegOpenKeyEx(HKEY_CURRENT_USER, szKey, 0, KEY_ALL_ACCESS,  (HKEY FAR*)&hkey) == ERROR_SUCCESS)
	{
		DWORD dwErr=RegDeleteValue (hkey, szValue);
	}
	RegCloseKey (hkey);
}


HMENU FindSubMenu (HMENU hMenu, int id)
{
	HMENU hSubMenu;

	for (int i=0; i<GetMenuItemCount (hMenu); i++)
	{
		hSubMenu = GetSubMenu (hMenu, i);
				
		if (GetMenuState (hSubMenu, id, MF_BYCOMMAND)==0xFFFFFFFF)
		{
			continue;
		}
		else
		{
			return hSubMenu;
		}
	}
	return NULL;
}



bool CPickList::Create (char *szKey, int nMaxItems)
{
	// alloc an array of ptr's to strings
	m_PickList = (char **) malloc (sizeof(char *) * nMaxItems);

	if (m_PickList == NULL) 
	{
		return false;
	}
	
	m_nMaxItems = nMaxItems;
	m_pszKey = (char *) malloc (sizeof(char) * lstrlen (szKey)+1);

	if (m_pszKey == NULL) 
	{
		return false;
	}

	for (int i=0;i<nMaxItems;i++)
	{
		m_PickList[i] = NULL;
	}

	lstrcpy (m_pszKey, szKey);
	return true;
}


CPickList::CPickList ()
{
	m_RecentMenu = NULL;
	m_pszKey = NULL;
	m_PickList = NULL;
	m_nMaxItems = 0;
}


CPickList::~CPickList ()
{
	if (m_PickList)
	{
		for (int i=0; i< m_nMaxItems; i++)
		{
			if (m_PickList[i])
			{
				free (m_PickList[i]);
				m_PickList[i]=NULL;
			}
		}
		free (m_PickList);
		m_PickList=NULL;
	}

	m_nMaxItems = 0;
	
	if (m_pszKey)
	{
		free (m_pszKey);
		m_pszKey=NULL;
	}

	// remove the recent file menu
	if (IsMenu (m_RecentMenu))
	{
		DestroyMenu (m_RecentMenu);
		m_RecentMenu = NULL;
	}
}


bool CPickList::Read (void)
{
	char szValue[MAX_PATH*2];
	char szItem[12];

	for (int i=0; i < m_nMaxItems;i++)
	{
		wsprintf (szItem, "file%d", i);

		if (GetKey (m_pszKey, szItem, szValue, sizeof (szValue)))
		{
			m_PickList[i] = (char *) malloc (sizeof (szValue));
			lstrcpy (m_PickList[i], szValue);
		}
		else
		{
			m_PickList[i] = NULL;
		}
	}
	return true;
}


bool CPickList::Write (void)
{
	char szItem[12];

	for (int i=0; i< m_nMaxItems;i++)
	{
		if (m_PickList[i]!=NULL)
		{
			wsprintf (szItem, "file%d", i);
			SetKey (m_pszKey, szItem, m_PickList[i]);
		}
		else
		{
			wsprintf (szItem, "file%d", i);
			DeleteKey (m_pszKey, szItem);
		}
	}
	return true;
}


void CPickList::AddItem (char *szItem)
{
	char *pItem = NULL;
	int iPos;

	// search through the pick list and see if the items already exists
	for (iPos=0; iPos< m_nMaxItems; iPos++)
	{
		if (m_PickList[iPos]!=NULL)
		{
			if (strcmpi (szItem, m_PickList[iPos])==0)
			{
				pItem = m_PickList[iPos];
				break;
			}
		}
	}

	// If the item was not found in list, then we need to allocate and store 
	// the string. If there was a string in the last list entry, we free it and 
	// shift the list down by one.
	if (pItem == NULL)
	{
		pItem = (char *) malloc (lstrlen (szItem)+1);
		lstrcpy (pItem, szItem);

		if (m_PickList[m_nMaxItems-1])
		{
			free (m_PickList[m_nMaxItems-1]);
			m_PickList[m_nMaxItems-1]=NULL;
		}
	
		for (int i=m_nMaxItems-1; i>0; i--)
		{
			m_PickList[i]=m_PickList[i-1];
		}
	}
	else
	{
		// Shift the list down one item, covering the items old position
		for (int i=iPos; i>0; i--)
		{
			m_PickList[i]=m_PickList[i-1];
		}
	}

	// insert the the item at the beginning of the list.
	m_PickList[0] = pItem;
}


void CPickList::RemoveItem (char *szItem)
{
	for (int iPos=0; iPos< m_nMaxItems; iPos++)
	{
		if (m_PickList[iPos]!=NULL)
		{
			if (strcmpi (szItem, m_PickList[iPos])==0)
			{
				free (m_PickList[iPos]);
				m_PickList[iPos]=NULL;

				// shift the list up
				for (int i=iPos; i< m_nMaxItems-1; i++)
				{
					m_PickList[i]=m_PickList[i+1];
				}
				m_PickList[m_nMaxItems-1]=NULL;
				break;
			}
		}
	}
}

void CPickList::Clear (void)
{
	for (int iPos=0; iPos < m_nMaxItems; iPos++)
	{
		if (m_PickList[iPos]!=NULL)
		{
			free (m_PickList[iPos]);
			m_PickList[iPos]=NULL;
		}
	}
}


char *CPickList::GetItem (int nItem)
{
	return m_PickList[nItem];
}

	
bool CPickList::UpdateMenu (HWND hWnd, int nBaseMenuID, int nRecentFileMenuID)
{
	HMENU hHistoryMenu = CreatePopupMenu();
	HMENU hMenu = GetMenu (hWnd);
	MENUITEMINFO mii;
	char szCurrentDirectory[MAX_PATH];
	char szItem[MAX_PATH*2];
	char szFileName[MAX_PATH];
	string sFile, sPath, sFullPath;


	ASSERT (hHistoryMenu);
	ASSERT (hMenu);

	GetCurrentDirectory (sizeof (szCurrentDirectory), szCurrentDirectory);
	
	// create the Recent Files menu.

	int iCount = 0;

	for (int i=0; i<m_nMaxItems;i++)
	{
		if (m_PickList[i] != NULL)
		{
			iCount++;
			sFullPath = m_PickList[i];
			sPath = GetPathFromFileName (sFullPath);
			if (strcmpi (sPath.c_str(), szCurrentDirectory)==0)
			{
				sFile = GetFileNameFromPath (sFullPath);
				lstrcpy (szFileName, sFile.c_str());
			}
			else
			{
				lstrcpy (szFileName, m_PickList[i]);
			}

			if (i<9)
			{
				wsprintf (szItem, "&%d  %s", i+1, szFileName);
			}
			else
			if (i==9)
			{
				wsprintf (szItem, "1&0  %s", szFileName);
			}
			else
			{
				lstrcpy (szItem, szFileName);
			}

			int ret = AppendMenu (hHistoryMenu, MF_STRING, nBaseMenuID+i, szItem);
			ASSERT (ret!=0);
		}
	}


	// find which drop down menu has our recent files item
	HMENU hSubMenu = FindSubMenu (hMenu, nRecentFileMenuID);

	ASSERT (hSubMenu);

	if (hSubMenu == NULL)
	{
		if (IsMenu (hHistoryMenu))
		{
			DestroyMenu (hHistoryMenu);
		}
		return false;
	}

	if (IsMenu (m_RecentMenu))
	{
		DestroyMenu (m_RecentMenu);
	}

	ZeroMemory (&mii, sizeof (MENUITEMINFO));
	GetMenuItemInfo (hSubMenu, nRecentFileMenuID, false, &mii);
	mii.cbSize = sizeof (MENUITEMINFO);

	if (iCount == 0)
	{
		// there are no picklist items, so destroy the menu we created.
		DestroyMenu (hHistoryMenu);

		// load a default 'empty' menu
		mii.hSubMenu = LoadMenu (g_hInst, MAKEINTRESOURCE(IDM_EMPTY_RECENTLIST));
	}
	else
	{
		mii.hSubMenu = hHistoryMenu;
	}

	// attach the new menu
	mii.fMask = MIIM_SUBMENU;
	if (0==SetMenuItemInfo (hSubMenu, nRecentFileMenuID, false, &mii))
	{
		ASSERT (FALSE);
		return false;
	}

	// keep track of the menu we just created
	m_RecentMenu = mii.hSubMenu;

	// redraw the menu
	if (!DrawMenuBar (hWnd))
	{
		ASSERT (FALSE);	
		return false;
	}

	return true;
}
