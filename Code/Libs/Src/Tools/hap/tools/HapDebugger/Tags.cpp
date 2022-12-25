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
#include "tags.h"
#include <fstream>
#include <string>
using namespace std;

CTags::CTags()
{
}

CTags::~CTags()
{
}

bool CTags::LoadTagsFile(const char* cszTagsFile)
{
	m_tagFile = cszTagsFile;
	
	return true;
}

void CTags::ClearTagInfo()
{
	
}

bool CTags::HasData()const
{
	return (m_tagFile.length() > 0);
}

bool CTags::FindTagInfo(const char* cszTag, vector<string>& files, vector<int>& lines)const
{
	

	ifstream fTags(m_tagFile.c_str());
	
	if (fTags.is_open())
	{
		string readStr;
		getline(fTags, readStr, '\n');
		while (readStr.length() > 0)
		{
			if (readStr[0] !='!')
			{
				//tag	file	line;"	type	inheritance
				string tag;

				int tagEnd = readStr.find('\t');
				int fileEnd = readStr.find('\t', tagEnd+1);
				tag = readStr.substr(0, tagEnd);
				if (stricmp(tag.c_str(), cszTag) == 0)
				{
					files.push_back(readStr.substr(tagEnd+1, fileEnd-tagEnd-1));
					lines.push_back(strtol(readStr.c_str() + fileEnd+1, 0, 10));
				}
				else 
					if (lines.size() > 0)
						//since the tags are grouped in the file alphabetically, once we find a non-match
						//after we've found matches, we're done.
						break;
			}
			readStr.erase();
			getline(fTags, readStr, '\n');
		}
		return (lines.size() > 0);
	}
	return false;
}

bool CTags::BuildTagsFile(const char* cszTagFile, const char* cszPaths)
{
	dprintf("Building tags file %s - paths: %s\n", cszTagFile, cszPaths);

	string tagFile = cszTagFile;
	string workingDir = cszTagFile;
	
	int PathEnd = tagFile.rfind('\\');
	if (PathEnd == -1)
	{
		PathEnd = tagFile.rfind('/');
	}
	if (PathEnd > -1)
	{
		workingDir = tagFile.substr(0, PathEnd);
		tagFile = tagFile.substr(PathEnd+1);
	}
	
	
	STARTUPINFO si = {0};
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = {0};
	
	string CommandLine;
	CommandLine = "ctags.exe -n -R --tag-relative=yes -f ";
	CommandLine += tagFile;
	CommandLine += " ";
	CommandLine += cszPaths;
	
	
	// For some reason it works badly if we pass the executable name as the
	// first parameter. I don't know why.
	if (!CreateProcess(0, 
		(char *)CommandLine.c_str(), NULL, NULL, FALSE, 0, NULL, 
		workingDir.c_str(), &si, &pi) )
	{
		LPVOID lpMsgBuf;
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		);
		// Process any inserts in lpMsgBuf.
		// ...
		// Display the string.
		MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
		// Free the buffer.
		LocalFree( lpMsgBuf );

		return false;
	}

	
	int nRetWait =  WaitForSingleObject(pi.hProcess,10000);

	CloseHandle(pi.hThread); 
	CloseHandle(pi.hProcess); 
	
	return (nRetWait != WAIT_TIMEOUT);
}