//////////////////////////////////////////////////////////////////////////////
//  File:    PdpManager.h
//
//  Purpose: Definition of a class to handle projects - each project is a set
//  of files which may be optionally grouped into folders.  The project 
//  files have a .pdp extension (python debugger project).
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

#if !defined(AFX_PDPMANAGER_H__76CD904F_50E7_4FF1_A50E_7449A13F26FD__INCLUDED_)
#define AFX_PDPMANAGER_H__76CD904F_50E7_4FF1_A50E_7449A13F26FD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



//=============================================================================
// Includes
//=============================================================================

//forward declare generic source interface
class CSourceControlInterface;




class CPdpManager  
{
public: // construction/destruction
	
	CPdpManager();
	virtual ~CPdpManager();

public: // methods

	std::string Version();
	
	std::string CurrentFile();
	
	bool LoadFromXml(const std::string& fromFile );
	bool SaveToXml( const std::string& toFile);

	bool LoadFromFile( const std::string& fromFile );
	bool SaveToFile( const std::string& toFile );

	void New();
	bool HasChanged()const;

	CSourceControlInterface* GetSci();
	bool HasChangedOnDisk()const;

private:

	bool ProcessLine( const std::string &line );
	void StorePathFile(const std::string &filepath);
	void StoreFileModTime(const std::string &filepath);

public: // data access

	const std::string& ProjectName()const;
	bool SetProjectName( const std::string &newName );

	int FileCount()const;
	const std::string& GetFileName(unsigned  int idx )const;
	std::string GetFilePath(unsigned  int idx )const;
	bool AddNewFile( const std::string& newFile, int groupinx = -1);
	bool RemoveFile( const std::string& file );
	const std::string& GetFileGroup( const std::string& );
	bool SetFileGroup( const std::string&, const std::string& group );
	int GroupCount() const;
	const std::string& GetGroupName(unsigned  int idx ) const;
	void SetGroupName(unsigned int idx, const std::string& newGroupName);
	void RemoveGroup(const std::string& group );
	void RemoveGroup(unsigned int idx);

	const std::string& BootFile()const;
	bool SetBootFile( const std::string& newFile );

	const std::string& RunOptions()const;
	bool SetRunOptions( const std::string& newOpts );

	const std::string& GetProjectPath()const	{return m_strProjectPath;}
	const std::string& GetProjectFile()const	{return m_strPdpFilename;}

	const std::string& GetWorkingDir()const;
	void SetWorkingDir(const std::string& strNewDir);
	const std::string& GetRemotePath()const;
	void SetRemotePath(const std::string& strNewPath);
	
	void ProjStartElement(const char* name, const char** attrs);
	void ProjEndElement(const char* name);

private:
	void SaveOptions(ofstream& fProj);
	bool SaveOptionsFile(const std::string& projFile);
	bool LoadOptionsFile(const std::string& projFile);
	int FindGroup(const std::string& grpName);
	void SortGroups();

private: // members
	
	//filename and path of the project
	std::string m_strPdpFilename;
	std::string m_strProjectPath;		//should end with a '\'
	bool m_HasChanged;

	//name of the project
	std::string m_strProjectName;

	//list of files in the project and the grouping map
	struct SFile
	{
		string filename;	//filename
		int nGroupInx;		//index of the group that the file is a member of
		
		SFile()
		{
			filename = "";
			nGroupInx = -1;
		}

		SFile(const string& file, int nGroup = -1)
		{	
			nGroupInx = nGroup;
			filename = file;
		}
	};

	typedef std::vector<SFile> FileList;
	typedef std::vector<SFile>::iterator FileListItr;
	FileList m_FileList;

	typedef std::vector<std::string> GroupList;
	typedef std::vector<std::string>::iterator GroupListItr;
	GroupList m_GroupList;

	//filename of the boot file
	std::string m_strBootFile;

	//run options specified
	std::string m_strRunOptions;

	//working directory
	std::string m_strWorkingDir;

	//remote path (path to project dir on remote machine)
	std::string m_strRemotePath;

	bool m_bFileIdentifierFound;


	CSourceControlInterface* m_pSCI;

	FILETIME m_LastModifiedTime;

	static bool SFileCompare( SFile& elem1, SFile& elem2 );

}; // CPdpManager





#endif // !defined(AFX_PDPMANAGER_H__76CD904F_50E7_4FF1_A50E_7449A13F26FD__INCLUDED_)
