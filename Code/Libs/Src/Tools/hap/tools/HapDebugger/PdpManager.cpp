//////////////////////////////////////////////////////////////////////////////
//  File:    PdpManager.cpp
//
//  Purpose: Implementation of a class to handle projects - each project is a set
//  of files which may be optionally grouped into folders. The project 
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

// PdpManager.cpp: implementation of the CPdpManager class.
//
//////////////////////////////////////////////////////////////////////



//=============================================================================
// Includes

#include "stdafx.h"
#include "PdpManager.h"
#include "dbgRemote.h"
#include "SourceControlInterface.h"
#include <iostream>
#include <fstream>
#include <algorithm>

#include <expat/expat.h>

//=============================================================================



//=============================================================================
CPdpManager::CPdpManager()
{
	m_pSCI = 0;
	New();
}
//=============================================================================

//=============================================================================
CPdpManager::~CPdpManager()
{
	New();
}
//=============================================================================

//=============================================================================
string CPdpManager::Version()
{
	return "1.0";
}
//=============================================================================

CSourceControlInterface* CPdpManager::GetSci()
{
	return ((CDbgRemoteApp*)AfxGetApp())->GetSci();
}



static void StartElement(void* userData, const char* name, const char** attrs);
static void EndElement(void* userData, const char* name);
static void CharacterDataHandler(void* userData, const XML_Char* s, int len);
static void CommentHandler(void* userData, const XML_Char* data);


void StartElement(void* userData, const char* name, const char** attrs)
{
	((CPdpManager*)userData)->ProjStartElement(name, attrs);
}

void EndElement(void* userData, const char* name)
{
	((CPdpManager*)userData)->ProjEndElement(name);
}

void CharacterDataHandler(void* userData, const XML_Char* s, int len)
{
}

void CommentHandler(void* userData, const XML_Char* data)
{
}

static vector<int> vecGroupStack;
static vector<string> vecGroupNameStack;

void CPdpManager::ProjStartElement(const char* name, const char** attrs)
{
	if (stricmp(name, "options") == 0)
	{
		for (; *attrs; attrs += 2)
		{
			const char* attrName = attrs[0];
			const char* attrValue = attrs[1];
			
			if (stricmp(attrName, "name") == 0)
			{
				m_strProjectName = attrValue;
			}			
			else if (stricmp(attrName, "bootfile") == 0)
			{
				m_strBootFile = attrValue;
			}
			else if (stricmp(attrName, "runargs") == 0)
			{
				m_strRunOptions = attrValue;
			}
			else if (stricmp(attrName, "workdir") == 0)
			{
				m_strWorkingDir = attrValue;
			}
			else if (stricmp(attrName, "remotepath") == 0)
			{
				m_strRemotePath = attrValue;
			}
		}
	}
	else if (stricmp(name, "group") == 0)
	{
		for ( ; *attrs; attrs += 2)
		{
			const char* attrName = attrs[0];
			const char* attrValue = attrs[1];
			
			if (stricmp(attrName, "name") == 0)
			{
				dprintf("loading group: %s\n", attrValue);
				
				if (strlen(attrValue) > 0)
				{
					string groupPath;
					for (vector<string>::iterator itr = vecGroupNameStack.begin(); itr != vecGroupNameStack.end(); itr++)
					{
						groupPath += *itr;
						groupPath += '/';
					}
					groupPath += attrValue;

					//store the 'path' to the group.
					m_GroupList.push_back(groupPath);				
					
					vecGroupStack.push_back(m_GroupList.size() - 1);
					vecGroupNameStack.push_back(attrValue);
				}
			}
		}
	}
	else if (stricmp(name, "file") == 0)
	{
		for ( ; *attrs; attrs += 2)
		{
			const char* attrName = attrs[0];
			const char* attrValue = attrs[1];
			
			if (stricmp(attrName, "path") == 0)
			{
				dprintf("loading file: %s\n", attrValue);
				if (vecGroupStack.size() > 0)
				{
					m_FileList.push_back(SFile(attrValue, vecGroupStack[vecGroupStack.size()-1]));
				}
				else
				{
					m_FileList.push_back(SFile(attrValue, -1));
				}
			}
		}
	}
}

void CPdpManager::ProjEndElement(const char* name)
{
	if (stricmp(name, "group") == 0)
	{
		vecGroupStack.pop_back();
		vecGroupNameStack.pop_back();
	}
}


//=============================================================================
bool CPdpManager::LoadFromXml(const string& fromFile )
{
	//read the file into a buffer
	ifstream fProj(fromFile.c_str());
	if (!fProj.is_open())
		return false;

	fProj.seekg( 0, ios_base::end );
	int fileSize = fProj.tellg();
	if (fileSize <= 0)
		return false;
	
	fProj.seekg( 0, ios_base::beg);
	vector<char> fileBuff;
	fileBuff.resize(fileSize);
	fProj.read(&(fileBuff[0]), fileSize);
	fProj.close();
	
	//parse the file's xml
	XML_Parser m_parser;
	m_parser = XML_ParserCreate(0);
	XML_SetUserData(m_parser, this);
	XML_SetStartElementHandler(m_parser, StartElement);
	XML_SetEndElementHandler(m_parser, EndElement);
//	XML_SetCharacterDataHandler(m_parser, CharacterDataHandler);
//	XML_SetCommentHandler(m_parser, CommentHandler);


	int rtn = XML_Parse(m_parser, &(fileBuff[0]), fileBuff.size(), true);

	if (!rtn)
	{
		dprintf("Error parsing:\n");
		dprintf(XML_ErrorString(XML_GetErrorCode(m_parser)));
		dprintf(" (line: %d, col: %d)\n", XML_GetCurrentLineNumber(m_parser), XML_GetCurrentColumnNumber(m_parser));
	}


//	XML_SetCommentHandler(m_parser, 0);
//	XML_SetCharacterDataHandler(m_parser, 0);
//	XML_SetEndElementHandler(m_parser, 0);
	XML_SetStartElementHandler(m_parser, 0);
	XML_SetUserData(m_parser, 0);
	XML_ParserFree(m_parser);


	return (rtn == 0);
}

void CPdpManager::SaveOptions(ofstream& fProj)
{
	fProj << "\t<options bootfile = \"";
	fProj << m_strBootFile.c_str();
	fProj << "\"/>\n";
	fProj << "\t<options runargs = \"";
	fProj << m_strRunOptions.c_str();
	fProj << "\"/>\n";
	fProj << "\t<options workdir = \"";
	fProj << m_strWorkingDir.c_str();
	fProj << "\"/>\n";
	fProj << "\t<options remotepath = \"";
	fProj << m_strRemotePath.c_str();
	fProj << "\"/>\n";
}
bool CPdpManager::LoadOptionsFile(const std::string& projFile)
{
	string opt = projFile;
	int fileEnd = projFile.rfind('.');
	if (fileEnd != -1)
		opt = opt.substr(0, fileEnd);	
	opt += ".opt";

	return LoadFromXml(opt);
}

bool CPdpManager::SaveOptionsFile(const std::string& projFile)
{
	string opt = projFile;
	int fileEnd = projFile.rfind('.');
	if (fileEnd != -1)
		opt = opt.substr(0, fileEnd);	
	opt += ".opt";
		
	ofstream fOpts(opt.c_str());
	if (!fOpts.is_open())
		return false;

	fOpts << "<?xml version=\"1.0\" ?>\n";
	fOpts << "<project>\n";

	SaveOptions(fOpts);

	fOpts << "</project>\n";

	fOpts.flush();
	fOpts.close();
	return true;
}
		

bool CPdpManager::SaveToXml( const std::string& toFile)
{
	dprintf("saving to xml format file %s\n", toFile.c_str());

	//save the options file - this will get saved even if the project file is write protected.
	bool bOpts = SaveOptionsFile(toFile);

	ofstream fProj(toFile.c_str());
	if (!fProj.is_open())
	{
		return false;
	}
	
	fProj << "<?xml version=\"1.0\" ?>\n";
	fProj << "<project>\n";
	fProj << "\t<options name = \"";
	fProj << m_strProjectName.c_str();
	fProj << "\"/>\n";	
	
	SaveOptions(fProj);
	
	int lastDepth = -1;
	bool bFirstGroupWritten = false;

	for (GroupListItr grp = m_GroupList.begin(); grp != m_GroupList.end(); grp++)
	{
		lastDepth = 0;
		int depth = 0;
		string grpName = *grp;
		if (grpName.length() == 0)
			continue;
		
		for (unsigned int i = 0; i < grpName.length(); i++)
		{
			if ( grpName[i] == '/')
				depth++;
		}
		dprintf("Group: %s (depth %d)\n", grpName.c_str(), depth);

		if ( (depth <= lastDepth) && (bFirstGroupWritten))
		{
			for (int curDepth = lastDepth; curDepth >= depth; curDepth--)
			{
				for (int i = 0; i < curDepth; i++)
					fProj << "\t";
				fProj << "\t</group>\n";				
			}
		}		
		
		int pathEnd = grpName.rfind('/');
		if (pathEnd > -1)
		{
			grpName = grpName.substr(pathEnd+1);
		}

		lastDepth = depth;


		for (int i = 0; i < depth; i++)
			fProj << "\t";

		fProj << "\t<group name = \"";
		fProj << grpName.c_str();
		fProj << "\">\n";
		bFirstGroupWritten = true;

		int grpIndex = grp - m_GroupList.begin();

		for (FileListItr itr = m_FileList.begin(); itr != m_FileList.end(); itr++)
		{
			if (itr->nGroupInx == grpIndex)
			{
				for (int i = 0; i < depth; i++)
					fProj << "\t";

				fProj << "\t\t<file path = \"";
				fProj << itr->filename.c_str();
				fProj << "\"/>\n";
			}
		}
	}

	for (int curDepth = lastDepth; curDepth >= 0; curDepth--)
	{
		for (int i = 0; i < curDepth; i++)
			fProj << "\t";
		fProj << "\t</group>\n";
	}

	for (FileListItr itr = m_FileList.begin(); itr != m_FileList.end(); itr++)
	{
		if (itr->nGroupInx == -1)
		{
			
			fProj << "\t<file path = \"";
			fProj << itr->filename.c_str();
			fProj << "\"/>\n";
		}
	}
	
	fProj << "</project>";

	fProj.flush();
	fProj.close();

	StoreFileModTime(toFile);
	StorePathFile(toFile);
	
	return true;

} // SaveToXml


bool CPdpManager::LoadFromFile(const string& fromFile )
{
	New();

	if (fromFile.find(".hap") != fromFile.npos)
	{		
		if (LoadFromXml(fromFile))
		{
			SortGroups();
			LoadOptionsFile(fromFile);
			
			StoreFileModTime(fromFile);
			StorePathFile(fromFile);

			return true;
		}
		return false;
	}
			
	fstream file( fromFile.c_str(), ios::in );

	if( file.good() )
	{
		StoreFileModTime(fromFile);
		StorePathFile(fromFile);
		
		string line = "";
		do
		{

			char ch;
			file.read( &ch, 1 );

			if( ch )
			{
				
				if( ch == '\n' )
				{
					if( !ProcessLine( line ) )
					{
						return false;
					}
					line = "";
				}
				else
				{
					line += ch;
				}

			}
			else
			{
				break;
			}

		}
		while( !file.eof() );

		file.close();

	}

	SortGroups();
	


	return true;

}

//=============================================================================


bool CPdpManager::SFileCompare( SFile& elem1, SFile& elem2 )
{
	string fileOnly1 = elem1.filename;
	string fileOnly2 = elem2.filename;
	int fileEnd = fileOnly1.rfind('\\');

	if (fileEnd != -1)
	{
		fileOnly1 = fileOnly1.substr(fileEnd+1);
	}
	
	fileEnd = fileOnly2.rfind('\\');
	if (fileEnd != -1)
	{
		fileOnly2 = fileOnly2.substr(fileEnd+1);
	}
	return (stricmp(fileOnly1.c_str(), fileOnly2.c_str()) == -1);
}

void CPdpManager::SortGroups()
{
	//here is a little hack to sort the groups into the correct order so that they
	//will save out properly.

	GroupList gListSort = m_GroupList;
	sort(gListSort.begin(), gListSort.end());
	for (int newIdx = 0; newIdx < (int)gListSort.size(); newIdx++)
	{
		if (gListSort[newIdx] != m_GroupList[newIdx])
		{
			int oldIdx = 0;
			for (oldIdx = 0; oldIdx < (int)m_GroupList.size(); oldIdx++)
			{
				if (m_GroupList[oldIdx] == gListSort[newIdx])
				{
					dprintf("%s - old: %d, new: %d\n", m_GroupList[oldIdx].c_str(), oldIdx, newIdx);
					break;
				}
			}
			assert(oldIdx != gListSort.size());

			for (FileListItr itr = m_FileList.begin(); itr != m_FileList.end(); itr++)
			{
				if (itr->nGroupInx == oldIdx)
				{
					//set the new index to the negative value so it doesn't get re-remapped
					itr->nGroupInx = -newIdx;
				}
			}
		}
	}
	m_GroupList = gListSort;

	//fix the negative indexes.
	for (FileListItr itr = m_FileList.begin(); itr != m_FileList.end(); itr++)
	{
		if (itr->nGroupInx < 0)
			itr->nGroupInx = -itr->nGroupInx;
	}

	sort(m_FileList.begin(), m_FileList.end(), CPdpManager::SFileCompare);

	for (FileListItr itr = m_FileList.begin(); itr != m_FileList.end(); itr++)
	{
		dprintf(itr->filename.c_str());
		dprintf("\n");
	}
		
}



//=============================================================================
bool CPdpManager::ProcessLine(const string& line )
{

	int pos = 0;
	std::string file = "";
	std::string group = "";
	bool unique = false;

	enum eCommand
	{
		kNone,
		kVersion,
		kProject,
		kFiles,
		kBootFile,
		kRunOpts,
		kGrouping,
		kWorkingDir,
		kRemotePath,
	}

	static currentCommand = kNone;
	
	FileList::iterator it;

	if( line != "" )
	{
	
		string sub = line.substr( 1, (line.size() - 1) );

		switch( line[0] )
		{	
		case '!':
			if( m_bFileIdentifierFound )
			{
				if( sub == "VERSION" )
				{
					currentCommand = kVersion;
				}
				else if( sub == "PROJECT" )
				{
					currentCommand = kProject;
				}
				else if( sub == "FILES" )
				{
					currentCommand = kFiles;
				}
				else if( sub == "BOOT_FILE" )
				{
					currentCommand = kBootFile;
				}
				else if( sub == "RUN_OPTS" )
				{
					currentCommand = kRunOpts;
				}
				else if( sub == "GROUPING" )
				{
					currentCommand = kGrouping;
				}
				else if( sub == "WORKING_DIR" )
				{
					currentCommand = kWorkingDir;
				}
				else if( sub == "REMOTE_PATH" )
				{
					currentCommand = kRemotePath;
				}
				else
				{
					dprintf("Invalid command: %s\n", line.c_str());
					return false;
				}
			}
			else
			{
				if( sub == "PAGE_DEBUGGER_PROJECT_FILE" )
				{
					m_bFileIdentifierFound = true;
					return true;
				}
				else
				{
					dprintf("Invalid file format (no PAGE_DEBUGGER_PROJECT_FILE directive)\n");
					return false;
				}
			}
			break;

		case '=':
			if( m_bFileIdentifierFound )
			{
				switch( currentCommand )
				{
				case kVersion:
					if( sub != Version() )
					{
						dprintf("Invalid file version\n");
						return false;
					}
					break;

				case kProject:
					m_strProjectName = sub;
					break;

				case kFiles:
				{
					m_FileList.push_back(SFile(sub));
					break;
				}

				case kBootFile:
					m_strBootFile = "";
					for( it = m_FileList.begin(); it != m_FileList.end(); ++it )
					{
						if( sub == (it->filename) )
						{
							m_strBootFile = sub;
							break;
						}
					}

					break;

				case kRunOpts:
					m_strRunOptions = sub;
					break;

				case kGrouping:
					pos = sub.find("->");
					file = sub.substr( 0, pos );
					group = sub.substr( (pos + 2), sub.size() );
					//m_GroupingMap[file] = group;
					SetFileGroup(file, group);

					/*unique = true;
					for( GroupList::iterator it = m_GroupList.begin(); it != m_GroupList.end(); ++it )
					{
						if( group == (*it) )
						{
							unique = false;
						}
					}

					if( unique )
					{
						m_GroupList.push_back( group );
					}*/
					
					break;
				case kWorkingDir:
					m_strWorkingDir = sub;
					break;

				case kRemotePath:
					m_strRemotePath = sub;
					break;

				default:
					dprintf("Invalid current command: %d\n", currentCommand);
					return false;
				}
			}
			else
			{
				dprintf("Value specified before command: %d\n", currentCommand);
				assert(0);
				return false;
			}
			break;

		case ';':
			currentCommand = kNone;
			break;

		default:
			dprintf("Invalid command character\n");
			return false;
		}

	}
		
	return true;

}
//=============================================================================

//=============================================================================
bool CPdpManager::SaveToFile(const string &toFile )
{
	string xml = toFile;
	int fileEnd = toFile.rfind('.');
	if (fileEnd != -1)
		xml = xml.substr(0, fileEnd);	
	xml += ".hap";
	return SaveToXml(xml);
		
/*	const string separator = ";-------------------------------------------------------------------------------";

	fstream file( toFile.c_str(), ios::out );

	if( file.good() )
	{
		StorePathFile(toFile);
		

		file 
			<< separator << endl
			<< "; " << toFile << endl
			<< separator << endl << endl
			<< separator << endl
			<< "; Required first command - indicates a valid .pdp file" << endl
			<< "!PAGE_DEBUGGER_PROJECT_FILE" << endl << endl
			<< separator << endl
			<< "; File format version" << endl
			<< "!VERSION" << endl
			<< "=" << Version() << endl << endl
			<< separator << endl
			<< "; Project name" << endl
			<< "!PROJECT" << endl
			<< "=" << m_strProjectName << endl << endl
			<< separator << endl
			<< "; Working Directory" << endl
			<< "!WORKING_DIR" << endl
			<< "=" << m_strWorkingDir << endl << endl
			<< separator << endl
			<< "; Remote Path" << endl
			<< "!REMOTE_PATH" << endl
			<< "=" << m_strRemotePath << endl << endl;

		file
			<< separator << endl
			<< "; Project files" << endl
			<< "!FILES" << endl;
		for( FileList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it )
		{
			file << "=" << (it->filename) << endl;
		}
		file << endl;

		for( FileList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it )
		{
			if (it->nGroupInx > -1)
			{
				file
					<< separator << endl
					<< "; File grouping" << endl
					<< "!GROUPING" << endl
					<< "=" << (it->filename) << "->" << m_GroupList[it->nGroupInx] << endl << endl;
			}
		}

		file
			<< separator << endl
			<< "; Project boot file" << endl
			<< "!BOOT_FILE" << endl
			<< "=" << m_strBootFile << endl << endl
			<< separator << endl
			<< "; Commandline run options" << endl
			<< "!RUN_OPTS" << endl
			<< "=" << m_strRunOptions << endl << endl;

		file.close();
		StoreFileModTime(toFile);
	}
	else
	{
		return false;
	}
	
	return true;
*/
}
//=============================================================================

//=============================================================================
void CPdpManager::New()
{
	m_HasChanged = false;
	m_strProjectName = "";
	m_strBootFile = "";
	m_strRunOptions = "";

	m_strPdpFilename = "";
	m_strProjectPath = "";

	m_FileList.clear();
	//m_GroupingMap.clear();
	m_GroupList.clear();

	m_bFileIdentifierFound = false;
	if (m_pSCI)
		delete m_pSCI;
	m_pSCI = 0;
}
//=============================================================================

//=============================================================================
bool CPdpManager::HasChanged()const
{
	return m_HasChanged;
}
//=============================================================================

//=============================================================================
const string& CPdpManager::ProjectName()const
{
	return m_strProjectName;
}
//=============================================================================

//=============================================================================
bool CPdpManager::SetProjectName( const string &newName )
{
	m_strProjectName = newName;
	m_HasChanged = true;

	return true;

}
//=============================================================================

//=============================================================================
int CPdpManager::FileCount()const 
{
	return m_FileList.size();
}
//=============================================================================

//=============================================================================
const string& CPdpManager::GetFileName(unsigned int idx )const
{

	if( (idx >= 0) && (idx < m_FileList.size()) )
	{
		return m_FileList[idx].filename;
	}

	static string Empty("");
	return Empty;

}
//=============================================================================
//=============================================================================
std::string CPdpManager::GetFilePath(unsigned  int idx )const
{
	string Path = GetProjectPath();
	string PathFile;

	vector<char> absPath;
	Path += GetFileName(idx);

	absPath.resize(MAX_PATH+1);
	if (0 == _fullpath(&(absPath[0]), Path.c_str(),absPath.size()))
		return "";

	return &(absPath[0]);
}
//=============================================================================

//=============================================================================
bool CPdpManager::AddNewFile( const string &newFile, int groupinx/* = -1*/)
{
	string FiletoAdd = newFile;
	
	unsigned int iDiff = 0;
	for (iDiff = 0; iDiff < FiletoAdd.size(), iDiff < m_strProjectPath.size(); iDiff++)
	{
		if (toupper(newFile[iDiff]) != toupper(m_strProjectPath[iDiff]))
		{
			break;
		}
	}
	if (iDiff == 0)
	{
		//assume FiletoAdd is already a relative path
	}
	else if (iDiff == m_strProjectPath.size())
	{
		//strip path.size chars off to make it relative
		FiletoAdd.erase(FiletoAdd.begin(), FiletoAdd.begin()+m_strProjectPath.size());
	}
	else
	{
		//iDiff is the index of the first differing char

		//back up to the previous directory
		while ((iDiff >= 0) && (FiletoAdd[iDiff] != '\\'))
		{
			iDiff--;
		}

		//erase the part that matches, the rest is a relative path
		FiletoAdd.erase(FiletoAdd.begin(), FiletoAdd.begin()+iDiff+1);

		//count the number of dir levels left in path
		int nLevels = 0;
		while (++iDiff < m_strProjectPath.size())
		{
			if (m_strProjectPath[iDiff] == '\\')
				nLevels++;
		}
		
		const string strPathUp("..\\");
		for (int i = 0; i < nLevels; i++)
		{
			FiletoAdd.insert(FiletoAdd.begin(), strPathUp.begin(), strPathUp.end());
		}

	}

	dprintf("Adding File: %s\n", FiletoAdd.c_str());

	FileList::iterator pCurFile; 
	for (pCurFile = m_FileList.begin(); pCurFile != m_FileList.end(); pCurFile++)
	{
		if (stricmp(pCurFile->filename.c_str(), FiletoAdd.c_str()) == 0)
		{
			//file is allready in the project
			return true;
		}
	}
	SFile NewFile;
	NewFile.filename = FiletoAdd;
	NewFile.nGroupInx = groupinx;
	m_FileList.push_back(NewFile);
	m_HasChanged = true;

	return true;
}
//=============================================================================

//=============================================================================
const string& CPdpManager::BootFile()const
{
	return m_strBootFile;
}
//=============================================================================

//=============================================================================
bool CPdpManager::SetBootFile( const string &newFile )
{

	m_strBootFile = "";
	
	for( FileList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it )
	{
		if( it->filename == newFile )
		{
			m_strBootFile = newFile;
			m_HasChanged = true;
			break;
		}
	}

	if( m_strBootFile == "" )
	{
		return false;
	}

	return true;

}
//=============================================================================

//=============================================================================
const string& CPdpManager::RunOptions()const
{
	return m_strRunOptions;
}
//=============================================================================

//=============================================================================
bool CPdpManager::SetRunOptions( const string &newOpts )
{

	m_strRunOptions = newOpts;
	m_HasChanged = true;
	return true;

}
//=============================================================================

//=============================================================================
void CPdpManager::StorePathFile(const std::string &filepath)
{
	int nPathEnd = filepath.find_last_of("\\");
	if (nPathEnd != -1)
	{
		m_strPdpFilename = "";
		m_strPdpFilename.append(filepath.begin()+nPathEnd+1, filepath.end());
		m_strProjectPath = "";
		m_strProjectPath.append(filepath.begin(), filepath.begin()+nPathEnd+1);
	}
	else
	{
		m_strPdpFilename = filepath;
		m_strProjectPath = "";
	}
}
//=============================================================================

//=============================================================================
bool CPdpManager::RemoveFile( const std::string& file )
{
	for (FileList::iterator pCur = m_FileList.begin(); pCur != m_FileList.end(); pCur++)
	{
		if (pCur->filename == file)
		{
			m_FileList.erase(pCur);
			return true;
		}
	}
	return false;

}
//=============================================================================

//=============================================================================
const std::string& CPdpManager::GetFileGroup( const std::string& file )
{
	for (FileList::iterator pCur = m_FileList.begin(); pCur != m_FileList.end(); pCur++)
	{
		if (pCur->filename == file)
		{
			if ( (pCur->nGroupInx > -1) && (pCur->nGroupInx < int(m_GroupList.size())) ) 
			{
				return m_GroupList[pCur->nGroupInx];
			}
		}
	}
	static string Empty = "";
	return Empty;
}
//=============================================================================

int CPdpManager::FindGroup(const std::string& grpName)
{
	for (int i = 0; i != m_GroupList.size(); i++)
	{
		if (m_GroupList[i] == grpName)
		{
			return i;
		}
	}
	return -1;
}

//=============================================================================
bool CPdpManager::SetFileGroup( const std::string& file, const std::string& group )
{

/*	std::string oldGroup = m_GroupingMap[file];

	if( oldGroup != "" )
	{

		int occurs = 0;
		for( FileList::iterator it = m_FileList.begin(); it != m_FileList.end(); ++it )
		{
			if( oldGroup == GetFileGroup((*it)) )
			{
				++occurs;
			}
		}

		if( occurs == 1 )
		{
			for( GroupList::iterator it = m_GroupList.begin(); it != m_GroupList.end(); ++it )
			{
				if( oldGroup == (*it) )
				{
					m_GroupList.erase( it );
					break;
				}
			}
		}

	}

	m_GroupingMap[file] = group;

	if( group != "" )
	{

		bool unique = true;
		for( GroupList::iterator it = m_GroupList.begin(); it != m_GroupList.end(); ++it )
		{
			if( group == (*it) )
			{
				unique = false;
			}
		}

		if( unique )
		{
			m_GroupList.push_back( group );
		}

	}	*/

	//find the index of the group
	
	int GroupInx = -1;
	if ("" != group)
	{
		GroupInx = FindGroup(group);
	
		if (-1 == GroupInx)
		{
			//it is a new group name, add it.
			
			//make sure that the group path is all there.
			string curPath = "";
			while (curPath.length() < group.length())
			{
				int nPath = group.find('/', curPath.length()+1);
				if (nPath != curPath.npos)
				{
					curPath = group.substr(0, nPath);
					int pathInx = FindGroup(curPath);
					
					if (pathInx == -1)
					{
						dprintf("adding group %s\n", curPath.c_str());
						m_GroupList.push_back(curPath);
					}
				}
				else
				{
					dprintf("adding group %s\n", group.c_str());
					m_GroupList.push_back(group);
					GroupInx = m_GroupList.size()-1;
					break;
				}
			}			
		}
	}
	
	for (FileList::iterator pCurFile = m_FileList.begin(); pCurFile != m_FileList.end(); pCurFile++)
	{
		if (pCurFile->filename == file)
		{
			pCurFile->nGroupInx = GroupInx;
			return true;
		}
	}

	//it is a new file - add it
	return AddNewFile(file, GroupInx);
}
//=============================================================================

//=============================================================================
int CPdpManager::GroupCount() const
{
	return m_GroupList.size();
}
//=============================================================================

//=============================================================================
const std::string& CPdpManager::GetGroupName(unsigned int idx ) const
{
	if( (idx >= 0) && (idx < m_GroupList.size()) )
	{
		return m_GroupList[idx];
	}

	static string Empty("");
	return Empty;
}
//=============================================================================
void CPdpManager::SetGroupName(unsigned int idx, const std::string& newGroupName)
{
	assert((idx >= 0) && (idx < m_GroupList.size()));
	
	if (newGroupName == "")
		RemoveGroup(idx);
	else
		m_GroupList[idx] = newGroupName;
}


void CPdpManager::RemoveGroup(const std::string& group )
{
	dprintf("Removing Group: %s\n", group.c_str());
	int GroupInx = -1;
	for (int i = 0; i != m_GroupList.size(); i++)
	{
		if (m_GroupList[i] == group)
		{
			GroupInx = i;
			break;
		}
	}

	RemoveGroup(GroupInx);
}

void CPdpManager::RemoveGroup(unsigned int idx)
{
	if (idx < m_GroupList.size())
	{
		m_GroupList[idx] = "";

		for (FileList::iterator pCurFile = m_FileList.begin(); pCurFile != m_FileList.end(); pCurFile++)
		{
			if (pCurFile->nGroupInx == idx)
			{
				pCurFile->nGroupInx = -1;
			}
		}
	}
}


const std::string& CPdpManager::GetWorkingDir()const		
{
	return m_strWorkingDir;
}
void CPdpManager::SetWorkingDir(const std::string& strNewDir)
{
	m_strWorkingDir = strNewDir;
	if (m_strWorkingDir.length())
	{
		if ((m_strWorkingDir[m_strWorkingDir.length()-1] != '/') && 
			(m_strWorkingDir[m_strWorkingDir.length()-1] != '\\'))
		{
			m_strWorkingDir += "/";
		}
	}
}

const std::string& CPdpManager::GetRemotePath()const		
{
	return m_strRemotePath;
}
void CPdpManager::SetRemotePath(const std::string& strNewPath)
{
	m_strRemotePath = strNewPath;

	if (m_strRemotePath.length())
	{
		if ((m_strRemotePath[m_strRemotePath.length()-1] != '/') && 
			(m_strRemotePath[m_strRemotePath.length()-1] != '\\'))
		{
			m_strRemotePath += "/";
		}
	}
}


void CPdpManager::StoreFileModTime(const std::string &filepath)
{
	HANDLE hFile = CreateFile(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		GetFileTime(hFile, 0, 0, &m_LastModifiedTime);
		CloseHandle(hFile);
	}
}

bool CPdpManager::HasChangedOnDisk()const
{
	string pathfile = m_strProjectPath;
	if ((pathfile.size() > 1) && (pathfile[pathfile.size()-1] != '\\'))
		pathfile += '\\';
	pathfile += m_strPdpFilename;
	
	HANDLE hFile = CreateFile(pathfile.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE != hFile)
	{
		FILETIME ft;
		GetFileTime(hFile, 0, 0, &ft);
		
		bool bChanged = (0 != CompareFileTime(&m_LastModifiedTime, &ft));
		CloseHandle(hFile);
		
		return bChanged;
	}
	return true;
}

