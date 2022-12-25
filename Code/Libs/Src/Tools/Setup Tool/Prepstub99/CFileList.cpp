/**************************************************************
* CFileList: a file list class
*
* Chris N. Haddan
* Feb 18th, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CFilelist.hpp"


extern void ConvertToValidPath (string& szPath);


bool lessthan_nocase (const string& s, const string& s2)
{
	string::const_iterator p  = s.begin();
	string::const_iterator p2 = s2.begin();

	while (p != s.end() && p2 != s2.end())
	{
		if (toupper (*p) != toupper(*p2)) 
		{
			return (toupper (*p) < toupper(*p2));
		}
		p++;
		p2++;
	}
	return (s.size() < s2.size());
}


bool operator< (CListEntry& a, CListEntry& b) 
{	
	return (lessthan_nocase (a.GetLongName(), b.GetLongName())); 
}


bool operator< (string& a, CListEntry& b) 
{
	return (lessthan_nocase (a, b.GetLongName())); 
}


bool ListEntryLT (CListEntry* a,CListEntry* b)
{
	return (*a < *b);
}


bool StringListEntryLT (string* a,CListEntry* b)
{
	return (*a < *b);
}


bool CFileList::SetDirectory (CDirectoryEntry *p_Directory) 
{ 
	if (!p_Directory) 
	{
		return false;
	}

	m_pdeCurrent = p_Directory; 
	
	return true;
}


bool CFileList::SetDirectory (const string szPathname)
{ 
	CDirectoryEntry *p_dir;
	string::const_iterator p;
	string szDir = "";
	string szPath = szPathname;

	ConvertToValidPath (szPath);
	p = szPath.begin();
	
	if (p != szPath.end())
	{
		if (*p == '\\')
		{
			SetDirectory (GetRootDirectory());
			p++;
		}
	}

	while (p != szPath.end())
	{
		if (*p == '\\')
		{
			if (szDir == "..")
			{
				m_pdeCurrent = GetCurrentDirectory()->GetParent();
			}
			else
			{
				if (p_dir = FindFirstDir (szDir))
				{
					m_pdeCurrent = p_dir;
				}
				else
				{
					return false;
				}
			}
			szDir = "";
		}
		else
		{
			szDir += *p;
		}
		p++;
	}
	return true;
}


CFileList::CFileList ()
{
	m_dwNumberDirectories=0;
	m_pdeRoot		= NULL;
	m_pdeCurrent	= NULL;
	m_szRootPath	= "";
	m_dwNumberFiles	= 0;
	m_pdeRoot		= new CDirectoryEntry ("");
	m_pdeCurrent	= m_pdeRoot;
}


CFileList::~CFileList ()
{
	if (m_pdeRoot) 
	{
		delete m_pdeRoot;
	}

	m_pdeCurrent = NULL;
	m_szRootPath = "";
}


bool CFileList::CreateDirectory (const string szDirectory)
{
	CDirectoryEntry *p_dir;

	if (m_pdeCurrent == NULL)
	{
		return false;
	}

	p_dir = new CDirectoryEntry (szDirectory);

	p_dir->SetParent (GetCurrentDirectory());

	if (!m_pdeCurrent->Add (p_dir))
	{
		delete p_dir;
		return false;
	};
	
	++m_dwNumberDirectories;

	return true;
}


bool CFileList::CreateDirectory (const string szDirectoryLong, const string szDirectoryShort)
{
	CDirectoryEntry *p_dir;

	if (m_pdeCurrent == NULL)
	{
		return false;
	}

	p_dir = new CDirectoryEntry (szDirectoryLong, szDirectoryShort);

	p_dir->SetParent (GetCurrentDirectory());

	if (!m_pdeCurrent->Add (p_dir))
	{
		delete p_dir;
		return false;
	};
	++m_dwNumberDirectories;
	return true;
}


bool CFileList::CreatePath (const string szPathname)
{
	CDirectoryEntry *p_CurrentDir = GetCurrentDirectory();
	string::const_iterator p;

	string szPath = szPathname;
	string szDir = "";

	ConvertToValidPath (szPath);
	p = szPath.begin();

	if (p!=szPath.end())
	{
		if (*p=='\\')
		{
			if (!SetDirectory (GetRootDirectory()))
			{
				return false;
			}
			p++;
		}
	}

	while (p != szPath.end())
	{
		if (*p == '\\')
		{
			string szSearch =szDir;
			ConvertToValidPath (szSearch);
			if (!SetDirectory (szSearch))
			{	
				if (!CreateDirectory (szDir)) 
				{
					return false;
				}

				if (!SetDirectory (szSearch))
				{
					return false;
				}
			}
			szDir = "";
		}
		else
		{
			szDir += *p;
		}

		p++;
	}
	if (!SetDirectory (p_CurrentDir)) 
	{
		return false;
	}
	return true;
}


bool CFileList::CreatePath (const string szPathname, const string szShortPathname)
{
	CDirectoryEntry *p_CurrentDir = GetCurrentDirectory();
	string::const_iterator p;
	string::const_iterator p2;

	string szPath = szPathname;
	string szShortPath = szShortPathname;
	string szDir = "";
	string szShortDir = "";

	ConvertToValidPath (szShortPath);
	ConvertToValidPath (szPath);
	p = szPath.begin();
	p2 = szShortPath.begin();

	if (p!=szPath.end())
	{
		if (*p=='\\')
		{
			if (!SetDirectory (GetRootDirectory()))
			{
				return false;
			}
			p++;
		}
	}

	while (p != szPath.end())
	{
		if (*p == '\\')
		{
			// walk thru the short path now to keep in sync with the long path.
			szShortDir = "";
			while (*p2 != '\\' && p2 != szShortPath.end())
			{
				szShortDir += *p2;
				p2++;
			}
			p2++;

			string szSearch =szDir;
			ConvertToValidPath (szSearch);
			if (!SetDirectory (szSearch))
			{	
				if (!CreateDirectory (szDir, szShortDir)) 
				{
					return false;
				}

				szShortDir = "";
			
				if (!SetDirectory (szSearch))
				{
					return false;
				}
			}
			szDir = "";
		}
		else
		szDir += *p;

		p++;
	}
	if (!SetDirectory (p_CurrentDir)) 
	{
		return false;
	}
	return true;
}


bool CFileList::CreateFile (const string szFile)
{
	CFileEntry *p_file;

	if (m_pdeCurrent == NULL)
	{
		return false;
	}

	p_file = new CFileEntry (szFile);

	// update ptr to current file entry.
	m_pfeCurrent = p_file;

	if (!m_pdeCurrent->Add (p_file))
	{
		delete p_file;
		return false;
	};
	++m_dwNumberFiles;
	return true;
}


bool CFileList::CreateFile (const string szFileLong, const string szFileShort)
{
	CFileEntry *p_file;

	if (m_pdeCurrent == NULL)
	{
		return false;
	}

	p_file = new CFileEntry (szFileLong, szFileShort);

	// update ptr to current file entry.
	m_pfeCurrent = p_file;

	if (!m_pdeCurrent->Add (p_file))
	{
		delete p_file;
		return false;
	};
	++m_dwNumberFiles;
	return true;
}


CDirectoryEntry *CDirectoryEntry::FindFirstDir(const string szPathname)
{
	ENTRYLIST::iterator i;
	string szLongName;

	if (szPathname.empty()) return this;

	CDirectoryEntry pe (szPathname);

	i = lower_bound (m_FileList.begin(), m_FileList.end(), &pe, ListEntryLT);

	if (i == m_FileList.end() || (*i)->GetLongName() != szPathname) 
	{
		return NULL;
	}
	else
	{
		return (static_cast<CDirectoryEntry *>(*i));
	}
}


CDirectoryEntry *CFileList::FindFirstDir(const string szPathname)
{
	return (m_pdeCurrent->FindFirstDir (szPathname));
}


string CDirectoryEntry::GetLongPath ()
{
	string szTemp = "";
	string szPath = "";

	CDirectoryEntry *p_dir = this;
	
	while ( p_dir )
	{
		szTemp = p_dir->GetLongName();
		if (szTemp[0] != '.')
		{
			if (p_dir->GetParent())
			{
				szTemp += "\\";
			}

			szTemp += szPath;
			szPath = szTemp;
		}
		p_dir = p_dir->GetParent();
	}
	return szPath;
}


string CDirectoryEntry::GetShortPath ()
{
	string szTemp = "";
	string szPath = "";

	CDirectoryEntry *p_dir = this;
	
	while ( p_dir )
	{
		szTemp = p_dir->GetShortName();
		if (szTemp[0] != '.')
		{
			if (p_dir->GetParent())
			{
				szTemp += "\\";
			}

			szTemp += szPath;

			szPath = szTemp;
		}
		p_dir = p_dir->GetParent();
	}
	return szPath;
}


CFileEntry::CFileEntry (const string szFile)
{
	m_szLongName = szFile;
	m_szShortName = szFile;
}


CFileEntry::CFileEntry (const string szFileLong, const string szFileShort)
{
	m_szLongName = szFileLong;
	m_szShortName = szFileShort;
}


CDirectoryEntry::CDirectoryEntry (const string szDirectory)
{
	m_szLongName = szDirectory;
	m_szShortName = szDirectory;
	m_pdeParent = NULL;
}


CDirectoryEntry::CDirectoryEntry (const string szDirectoryLong, const string szDirectoryShort)
{
	m_szLongName = szDirectoryLong;
	m_szShortName = szDirectoryShort;
	m_pdeParent = NULL;
}


CDirectoryEntry::~CDirectoryEntry ()
{
	ENTRYLIST::iterator i;

	m_szLongName = "";
	m_szShortName = "";

	for (i=m_FileList.begin(); i != m_FileList.end(); ++i)
	{
		if (static_cast<CListEntry *>(*i)->GetType() == FE_FILE) 
		{
			delete (static_cast<CFileEntry *>(*i));
		}
		else
		{
			delete (static_cast<CDirectoryEntry *>(*i));
		}
	}

	if (!m_FileList.empty()) 
	{
		m_FileList.clear();
	}
}


CFileEntry::~CFileEntry ()
{
	m_szLongName = "";
	m_szShortName = "";
}


bool CDirectoryEntry::Add (CListEntry *p_entry)
{
	m_FileList.insert (	lower_bound (	m_FileList.begin(),
										m_FileList.end(), 
										p_entry, 
										ListEntryLT ), 
						p_entry);
	
	return true;
}


string CDirectoryEntry::GetLongName ()
{
	return (m_szLongName);
}


string CDirectoryEntry::GetShortName ()
{
	return (m_szShortName);
}


string CFileEntry::GetLongName ()
{
	return (m_szLongName);
}


string CFileEntry::GetShortName ()
{
	return (m_szShortName);
}


bool CDirectoryEntry::Dump ()
{
	string buffer;
	ENTRYLIST::iterator i;
	static int iLevel=0;

	for (i=m_FileList.begin(); i != m_FileList.end(); ++i)
	{
		for (int iCount=0; iCount < iLevel; iCount++) 
		{
			printf ("  ");
		}
		
		if ((*i)->GetType() == FE_FILE)	
		{
			printf ("<file>");
		}
		else
		{
			printf ("<dir>");
		}
			
		buffer = (*i)->GetLongName();

		printf ("%s\n",buffer.c_str());

		if ((*i)->GetType() == FE_DIR) 
		{
			++iLevel;
			static_cast<CDirectoryEntry *>(*i)->Dump();
			--iLevel;
		}
	}
	return true;
}


bool CFileList::ReadFileList (string szRootPath, string szWildCard, bool bRecursive)
{
	WIN32_FIND_DATA fdInfo;
	HANDLE hFile;
	string szSearchSpec;
	string szNewPath;

	if (!ScanFiles (szRootPath, szWildCard)) 
	{
		if (!bRecursive)
			return false;
	}

	if (!bRecursive) 
	{
		return true;
	}

	szSearchSpec = szRootPath;

	ConvertToValidPath (szSearchSpec);
	
	szSearchSpec += "*.*";

	hFile = FindFirstFile (szSearchSpec.c_str(), &fdInfo);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	// we can ignore this first file because it is always the current directory "."	

	while (FindNextFile (hFile, &fdInfo))
	{
		EbuYield();
		if (!(fdInfo.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
		{
 			if (fdInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (fdInfo.cFileName[0] != '.')
				{
					if (fdInfo.cFileName[1] != '.')
					{
						szNewPath = szRootPath;
						ConvertToValidPath (szNewPath);
						szNewPath += fdInfo.cFileName;
						ConvertToValidPath (szNewPath);
	
						if (!ReadFileList (szNewPath, szWildCard, bRecursive)) 
						{
							return false;
						}
					}
				}
			}
		}
	}

	FindClose (hFile);
	return true;
}		


void CFileList::ProcessFile (const string szDirectory, WIN32_FIND_DATA *fdInfo)
{
	if (!(fdInfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (fdInfo->cAlternateFileName[0] != '\0')
		{
			this->CreateFile (fdInfo->cFileName, fdInfo->cAlternateFileName);
		}
		else
		{
			this->CreateFile (fdInfo->cFileName, fdInfo->cFileName);
		}
	}
}


bool CFileList::ScanFiles (const string szDirectory, const string szWildCard)
{
	WIN32_FIND_DATA fdInfo;
	HANDLE hFile;
	string szSearchSpec;

	szSearchSpec = szDirectory;

	ConvertToValidPath (szSearchSpec);
	
	szSearchSpec += szWildCard;
	
	hFile = FindFirstFile (szSearchSpec.c_str(), &fdInfo);

	if (hFile == INVALID_HANDLE_VALUE) 
	{
		return false;
	}

	CDirectoryEntry *p_dir = GetCurrentDirectory();

	if (!CreatePath (szDirectory)) 
	{
		return false;
	}
	
	if (!SetDirectory (szDirectory)) 
	{
		return false;
	}

	ProcessFile (szDirectory, &fdInfo);

	while (FindNextFile (hFile, &fdInfo))
	{
		ProcessFile (szDirectory, &fdInfo);
	}

	SetDirectory (p_dir);
	FindClose (hFile);
	return true;
}