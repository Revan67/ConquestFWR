/**************************************************************
* CFileList.hpp: a file list class
*
* Chris N. Haddan
* Feb 18th, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __CFILELIST_H
#define __CFILELIST_H

#include <windows.h>
#include <list>
#include <string>
#include "ctype.h"
#include <algorithm>
#include "typeinfo.h"
#include "winnls.h"
#include "util.h"

#define FE_DIR			1
#define FE_FILE			2

using namespace std ;

void ConvertToValidPath (string& szPath);


bool WildCardCompare (const string& sString, const string& sWild);

#define TRACE(exp) (OutputDebugString(exp))

class CListEntry {
	public:
		CListEntry(){};
		~CListEntry(){};
		virtual string GetLongName ()=0;
		virtual string GetShortName ()=0;
		virtual BYTE GetType ()=0;
		virtual void AttachUserData (DWORD dwUserData)
			{	m_dwUserData = dwUserData; }
		virtual DWORD GetUserData ()
			{ return m_dwUserData; }
	private:
		DWORD m_dwUserData;
};

class CFileEntry : public CListEntry {
	public:
		CFileEntry () ;
		CFileEntry (const string szFile);
		CFileEntry (const string szFileLong, const string szFileShort);
		~CFileEntry () ;
		virtual string GetLongName ();
		virtual string GetShortName ();
		virtual BYTE GetType () { return FE_FILE; }
	private:
		string m_szLongName;
		string m_szShortName;
};

typedef list <CListEntry *> ENTRYLIST;

class CDirectoryEntry : public CListEntry {
	public:
		CDirectoryEntry ();
		~CDirectoryEntry ();
		bool Dump ();
		bool Add (CListEntry *p_dir);
		CDirectoryEntry (string szDirectory);
		CDirectoryEntry (string szDirectoryLong, string szDirectoryShort);
		CDirectoryEntry *FindFirstDir(string szDirectory);
		CDirectoryEntry *GetParent () { return m_pdeParent; };
		
		string GetLongPath ();
		string GetShortPath ();
		virtual string GetLongName ();
		virtual string GetShortName ();
		virtual BYTE GetType () { return FE_DIR; };
		void SetParent (CDirectoryEntry *p_dir) { m_pdeParent = p_dir; };
		ENTRYLIST* GetDirectoryList() {return &m_FileList; };
	private:
		string m_szLongName;
		string m_szShortName;
		ENTRYLIST m_FileList;
		CDirectoryEntry *m_pdeParent;
};

class CFileList {
	public:
		bool CreateDirectory (const string szDirectory);
		bool CreateDirectory (const string szDirectoryLong, const string szDirectoryShort);
		bool CreatePath (const string szPathname);
		bool CreatePath (const string szPathname, const string szShortPath);
		bool CreateFile (const string szFile);
		bool CreateFile (const string szFileLong, const string szFileShort);
		bool SetDirectory (CDirectoryEntry *p_Directory);
		bool SetDirectory (const string szPathname);
		CDirectoryEntry *GetCurrentDirectory (void) { return (m_pdeCurrent); };
		CDirectoryEntry *GetRootDirectory (void) { return (m_pdeRoot); };
		CDirectoryEntry *FindFirstDir(const string szDirectory);
		CFileEntry *GetCurrentFile (void) { return (m_pfeCurrent); };
		CFileList();
		~CFileList();
		bool ReadFileList (const string szRootPath, const string szWildCard, bool bRecursive);
		void ProcessFile (const string szDirectory, WIN32_FIND_DATA *fdInfo);
		bool ScanFiles (const string szDirectory, const string szWildCard);
		void ShowStats (void) { printf ("There are %d files in %d directories\n",m_dwNumberFiles, m_dwNumberDirectories); };
		int GetFileCount () { return m_dwNumberFiles; };
		int GetDirectoryCount () { return m_dwNumberDirectories; };
	private:
		string m_szRootPath;
		CDirectoryEntry *m_pdeRoot;
		CDirectoryEntry *m_pdeCurrent;
		CFileEntry		*m_pfeCurrent;
		DWORD m_dwNumberFiles;
		DWORD m_dwNumberDirectories;
};

#endif

