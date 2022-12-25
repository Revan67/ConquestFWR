//--------------------------------------------------------------------------//
//                                                                          //
//                               ArchList.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2003 by Fever Pitch Studios, INC.         //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /EffectEd/Src/ArchList.cpp 2     10/14/03 4:06p Tmauer $
*/			    
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "globals.h"

#include "ArchList.h"
#include "startup.h"

#include <DACOM.h>
#include <TSmartPointer.h>
#include <TComponent.h>
#include <FileSys.h>

#include <DBaseData.h>

#include <stdlib.h>

//--------------------------------------------------------------------------//
//
struct ARCHNODE 
{
	struct ARCHNODE * prev, * next;
	U32 usage;
	ARCHDATATYPE * archDataType;
	HANDLE hArchetype;

#ifndef FINAL_RELEASE
	
	struct NAMENODE
	{
		NAMENODE * pNext;
		const char * szName;
	} * nameList;

	~ARCHNODE (void)
	{
		reset();
	}

	void reset (void)		// flush the names list
	{
		NAMENODE * node = nameList;

		while (node)
		{
			nameList = node->pNext;
			delete node;
			node = nameList;
		}
	}

	void addName (const char * name)
	{
		if (name)
		{
			NAMENODE * node = new NAMENODE;
			node->pNext = nameList;
			nameList = node;
			node->szName = name;
		}
	}

	void removeName (const char * name)
	{
		if (name)
		{
			NAMENODE * node = nameList, *prev=0;

			while (node)
			{
	//			if (strcmp(name, node->szName) == 0)
				if (name == node->szName)
				{
					if (prev)
						prev->pNext = node->pNext;
					else
						nameList = node->pNext;
					delete node;
					break;
				}

				prev = node;
				node = node->pNext;
			}
		}
	}

#endif

	ARCHNODE (void)
	{
		memset(this, 0, sizeof(*this));
	}

private:
	// not allowed
	ARCHNODE & operator = (const ARCHNODE & obj)
	{
		return *this;
	}
};
//--------------------------------------------------------------------------//
//------------------------------ArchList Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE ArchList : public   IArchetypeList						   
{
	U32 eventHandle;		// handles to callback

	ARCHNODE *archList;							// pointer to first ARCHNODE instance in archetype list
	ARCHDATA *archData;							// pointer to loaded archetype database info

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(ArchList)
	DACOM_INTERFACE_ENTRY(IArchetypeList)
	END_DACOM_MAP()

	ArchList (void);

	~ArchList (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IArchetypeList methods */
	virtual	void * GetArchetypeData (const C8 * name);

	virtual	BOOL32 EnumerateArchetypeData (struct IArchetypeEnum * enumerator);

	virtual	BOOL32 EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch);

	virtual void Close (void);

	/* ArchList methods */	
	void flushUnusedArchetypes (void);

	U32 getNumArchetypes (void);

	BOOL32 loadTypesData (void);

	ARCHDATATYPE * getArchDataType (const C8 * name);

	void DEBUG_reloadDatabase (void);

	void DEBUG_forceFlush (void);
};
//--------------------------------------------------------------------------//
//
ArchList::ArchList (void)
{
}
//--------------------------------------------------------------------------//
//
ArchList::~ArchList (void)
{
	flushUnusedArchetypes();

	free(archData);
	archData = 0;
	ARCHLIST = NULL;
}
//--------------------------------------------------------------------------//
//
void ArchList::DEBUG_reloadDatabase (void)
{
	if (archList==0)
	{
		loadTypesData();
	}
}
//-------------------------------------------------------------------
//
void * ArchList::GetArchetypeData (const C8 * name)
{
	ARCHDATATYPE * dataType;
		
	if ((dataType = getArchDataType(name)) != 0)
		return dataType->objData;
	else
		return 0;
}
//-------------------------------------------------------------------
//
BOOL32 ArchList::EnumerateArchetypeData (struct IArchetypeEnum * enumerator)
{
	BOOL32 result = 1;
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if ((result = enumerator->ArchetypeEnum(type->name, type->objData, type->dataSize)) == 0)
			break;
		type++;
		i--;
	}

	return result;
}
//-------------------------------------------------------------------
//
BOOL32 ArchList::EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch)
{
	U32 nStrLen = strlen(subarch);
	BOOL32 result = 1;
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if (!strncmp(subarch,type->name,nStrLen))
		{
			if ((result = enumerator->ArchetypeEnum(type->name, type->objData, type->dataSize)) == 0)
				break;
		}
		type++;
		i--;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void ArchList::Close (void)
{
	flushUnusedArchetypes();
}
//-------------------------------------------------------------------
//
void ArchList::flushUnusedArchetypes (void)
{
	ARCHNODE * node = archList;

	while (node)
	{
		if (node->usage == 0)
		{
			node = archList;
		}
		else
			node = node->next;
	}
}
//--------------------------------------------------------------------------//
//
U32 ArchList::getNumArchetypes (void)
{
	ARCHNODE * node = archList;
	U32 result = 0;

	while (node)
	{
		result++;
		node = node->next;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void ArchList::DEBUG_forceFlush (void)
{
	ARCHNODE * node = archList;

	while (node)
	{
		node = node->next;
	}

	archList = 0;
	exit(-1);
}
//--------------------------------------------------------------------------//
// return total numbers of files, and cumulative size of all files
//
static void get_total_bytes (IFileSystem * file, U32 & dataSize, U32 & numFiles)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	DAFILEDESC fdesc=data.cFileName;
	
	if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		fdesc.hFindFirst = handle;
		do
		{
			// make sure this not a silly "." entry
			if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (strcmp(data.cFileName, "Parsed Files"))
					{
						COMPTR<IFileSystem> pNewFile;
						// traverse subdirectory
						if (file->CreateInstance(&fdesc, pNewFile) == GR_OK)
						{
							get_total_bytes(pNewFile, dataSize, numFiles);
						}
					}
				}
				else 
				{	
				 	dataSize += data.nFileSizeLow;
					numFiles++;
				}
			}

		} while (file->FindNextFile(handle, &data));

		file->FindClose(handle);
	}
}
//--------------------------------------------------------------------------//
//
/*
struct ARCHDATATYPE
{
	C8 name[64];
	void * objData;
	U32 dataSize;		// size of data chunk in bytes
};
struct ARCHDATA
{
	U32 numArchetypes;
	ARCHDATATYPE type[];
};
*/
//--------------------------------------------------------------------------//
//
static U32 calcCheckSum (const U8 * buffer, U32 bufferSize, U32 checkSum)
{
	while (bufferSize-- > 0)
	{
		checkSum += buffer[bufferSize];
	}
	return ~checkSum;
}
//--------------------------------------------------------------------------//
//
static void load_bytes (IFileSystem * file, ARCHDATA * archData, U32 & checkSum)
{
	WIN32_FIND_DATA data;
	HANDLE handle;
	DAFILEDESC fdesc=data.cFileName;
	
	if ((handle = file->FindFirstFile("*.*", &data)) != INVALID_HANDLE_VALUE)
	{
		fdesc.hFindFirst = handle;
		do
		{
			// make sure this not a silly "." entry
			if (data.cFileName[0] != '.' || strchr(data.cFileName, '\\') != 0)
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					// traverse subdirectory
					if (strcmp(data.cFileName, "Parsed Files"))
					{
						COMPTR<IFileSystem> pNewFile;
						if (file->CreateInstance(&fdesc, pNewFile) == GR_OK)
						{
							load_bytes(pNewFile, archData, checkSum);
						}
					}
				}
				else 
				{	
					HANDLE hFile;

					if ((hFile = file->OpenChild(&fdesc)) != INVALID_HANDLE_VALUE)
					{
						U32 i = archData->numArchetypes++;
						DWORD dwRead;

						if (i != 0)
							archData->type[i].objData = (BASIC_DATA *) (((U8 *)archData->type[i-1].objData) + archData->type[i-1].dataSize);

						strncpy(archData->type[i].name, data.cFileName, sizeof(archData->type[i].name)-1);
						archData->type[i].dataSize = file->GetFileSize(hFile);
						file->ReadFile(hFile, archData->type[i].objData, archData->type[i].dataSize, &dwRead, 0);
						file->CloseHandle(hFile);
						checkSum = calcCheckSum((U8 *)archData->type[i].objData, dwRead, checkSum);
					}
				}
			}

		} while (file->FindNextFile(handle, &data));

		file->FindClose(handle);
	}

}
//--------------------------------------------------------------------------//
//
BOOL32 ArchList::loadTypesData (void)
{
#if !defined(_XBOX)
	const char* szGameTypesFilename = "GameTypes.db";
	char iniFilename[MAX_PATH];

	DWORD ret = GetPrivateProfileString( "GameTypes", "file", "GameTypes.db", iniFilename, MAX_PATH, ".\\game.ini" );
	if( ret )
	{
		szGameTypesFilename = iniFilename;
	}

#else
	const char* szGameTypesFilename = "D:\\GameTypes.db";
#endif

	DAFILEDESC fdesc = szGameTypesFilename;
	COMPTR<IFileSystem> file;
	U32 dataSize=0, numFiles=0, checkSum=0;
	BOOL32 result = 0;

#ifdef _INTERNAL_PATHS
	fdesc.lpFileName = "Z:\\Shadow\\code\\shared\\db\\GameTypes.db";
#endif
	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
	{
		fdesc.lpFileName = "..\\..\\Shared\\DB\\GameTypes.db";
		if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
		{
			goto Done;
		}
	}
	
	get_total_bytes(file, dataSize, numFiles);
	
	::free(archData);
	archData = (ARCHDATA *) calloc(sizeof(ARCHDATA)+(sizeof(ARCHDATATYPE)*numFiles)+dataSize, 1);

	archData->type[0].objData = (BASIC_DATA *) (((U8 *)archData) + sizeof(ARCHDATA)+(sizeof(ARCHDATATYPE)*numFiles)); // mark beginning of data
	load_bytes(file, archData, checkSum);

	result = 1;
Done:
	return result;
}
//--------------------------------------------------------------------------//
//
ARCHDATATYPE * ArchList::getArchDataType (const C8 * name)
{
	ARCHDATATYPE * result = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if (strcmp(result->name, name) == 0)
			return result;
		result++;
		i--;
	}

	return 0;
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
struct ArchListComp : GlobalComponent
{
	ArchList * list;

	virtual void Startup (void)
	{
		ARCHLIST = list = new DAComponent<ArchList>;
		AddToGlobalCleanupList((IDAComponent **) &ARCHLIST);
	}

	virtual void Initialize (void)
	{
		list->loadTypesData();
	}
};

static ArchListComp archListComp;

//-------------------------------------------------------------------
//-------------------------END ArchList.cpp---------------------------
//-------------------------------------------------------------------
