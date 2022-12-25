/**************************************************************
* CPickList.hpp: Recent File List  (picklist)
*
* Chris N. Haddan
* October 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CPICKLIST_H
#define __CPICKLIST_H


class CPickList 
{
	public:
		CPickList();
		~CPickList();
		bool Create (char *szKey, int nMaxItems);
		bool Read();
		bool Write();
		void AddItem(char *szItem);
		void RemoveItem (char *szItem);
		bool UpdateMenu (HWND hWnd, int nBaseMenuID, int nRecentFileMenuID);
		void Clear (void);
		char *GetItem (int nItem);
	private:
		char *m_pszKey;
		char **m_PickList;
		int	 m_nMaxItems;
		HMENU m_RecentMenu;
};


#endif




