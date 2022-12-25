//--------------------------------------------------------------------------//
//                                                                          //
//                               DataList.cpp                               //
//                                                                          //
//                  COPYRIGHT (C) 2004 by Fever Pitch Studios, INC.         //
//                                                                          //
//--------------------------------------------------------------------------//
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "globals.h"

#include "DataList.h"
#include "startup.h"
#include "cqTrace.h"

#include <EventSys.h>
#include <DACOM.h>
#include <TSmartPointer.h>
#include <TConnPoint.h>
#include <TConnContainer.h>
#include <FileSys.h>

#include <DBaseData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------//
//------------------------------DataList Class----------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE DataList : public   IDataList,IEventCallback
{
	U32 eventHandle;		// handles to callback

	ARCHDATA *archData;							// pointer to loaded archetype database info

	//
	//
	//
	U32 creationCounter;		// increment everytime an object is created

	//
	// Interface mapping
	//

	BEGIN_DACOM_MAP_INBOUND(DataList)
	DACOM_INTERFACE_ENTRY(IDataList)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()


	DataList (void);

	~DataList (void);

	/* IDataList methods */
	virtual const char * GetArchName (U32 dwArchetypeID);

	virtual U32 GetArchetypeDataID (const C8 * name);

	virtual	void * GetArchetypeData (const C8 * name);

	virtual void * GetArchetypeData (U32 dwArchetypeID);

	virtual U32 GetArchetypeDataSize (U32 dwArchetypeID);

	virtual void Close (void);

	virtual	BOOL32 EnumerateArchetypeData (struct IArchetypeEnum * enumerator, DWORD context);

	virtual	BOOL32 EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch);


	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* DataList methods */	
	U32 getNumArchetypes (void);

	BOOL32 loadTypesData (const char * databaseName);

	ARCHDATATYPE * getArchDataType (const C8 * name);
};
//--------------------------------------------------------------------------//
//
DataList::DataList (void)
{
	archData = NULL;
}
//--------------------------------------------------------------------------//
//
DataList::~DataList (void)
{
	COMPTR<IDAConnectionPoint> connection;

	if (EVENTSYS && EVENTSYS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		connection->Unadvise(eventHandle);

	free(archData);
	archData = 0;

	if(GAMETYPES == this)
		GAMETYPES = NULL;

	if(GENDATA == this)
		GENDATA = NULL;
}
//-------------------------------------------------------------------
//
const char * DataList::GetArchName (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return dataType->name;
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
void * DataList::GetArchetypeData (const C8 * name)
{
	ARCHDATATYPE * dataType;
		
	if ((dataType = getArchDataType(name)) != 0)
		return dataType->objData;
	else
		return 0;
}
//-------------------------------------------------------------------
//
U32 DataList::GetArchetypeDataID (const C8 * name)
{
	ARCHDATATYPE * dataType;
		
	if ((dataType = getArchDataType(name)) != 0)
		return (U32(dataType) - U32(archData));

	return 0;
}
//-------------------------------------------------------------------
//
void * DataList::GetArchetypeData (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return dataType->objData;
	}
	
	return 0;
}
//-------------------------------------------------------------------
//
U32 DataList::GetArchetypeDataSize (U32 dwArchetypeID)
{
	ARCHDATATYPE * dataType;

	if (dwArchetypeID)
	{
		dataType = (ARCHDATATYPE *) ( U32(archData) + dwArchetypeID );
		return dataType->dataSize;
	}
	
	return 0;
}
//--------------------------------------------------------------------------//
//
void DataList::Close (void)
{
}
//-------------------------------------------------------------------
// receive notifications from event system
//
GENRESULT DataList::Notify (U32 message, void *param)
{
	return GR_OK;
}
//-------------------------------------------------------------------
//
BOOL32 DataList::EnumerateArchetypeData (struct IArchetypeEnum * enumerator, DWORD context)
{
	BOOL32 result = 1;
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if ((result = enumerator->ArchetypeEnum(type->name, type->objData, type->dataSize, context)) == 0)
			break;
		type++;
		i--;
	}

	return result;
}
//-------------------------------------------------------------------
//
BOOL32 DataList::EnumerateArchetypeDataBySubArch (struct IArchetypeEnum * enumerator, const char* subarch)
{
	U32 nStrLen = strlen(subarch);
	BOOL32 result = 1;
	ARCHDATATYPE * type = archData->type;
	U32 i = archData->numArchetypes;

	while (i)
	{
		if (!strncmp(subarch,type->name,nStrLen))
		{
			if ((result = enumerator->ArchetypeEnum(type->name, type->objData, type->dataSize, 0)) == 0)
				break;
		}
		type++;
		i--;
	}

	return result;
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

						strncpy(archData->type[i].name, data.cFileName, sizeof(archData->type[i].name));
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
BOOL32 DataList::loadTypesData (const char * databaseName)
{
	DAFILEDESC fdesc = databaseName;
	COMPTR<IFileSystem> file;
	U32 dataSize=0, numFiles=0, checkSum=0;
	BOOL32 result = 0;

	char buffer[MAX_PATH];

#ifdef _INTERNAL_PATHS
	sprintf(buffer,"Z:\\Shadow\\code\\shared\\db\\%s",databaseName);
	fdesc.lpFileName =  buffer;
#endif
	if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
	{
		sprintf(buffer,"..\\App\\DB\\%s",databaseName);
		fdesc.lpFileName =  buffer;
		if (DACOM->CreateInstance(&fdesc, file) != GR_OK)
		{
			CString msg = CString("File not found (") + CString(databaseName) + CString("\n");
			CQTRACE10("FILE NOT FOUND");
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
ARCHDATATYPE * DataList::getArchDataType (const C8 * name)
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
struct DataListComp : GlobalComponent
{
	DataList * gameTypes;
	DataList * genData;

	virtual void Startup (void)
	{
		GAMETYPES = gameTypes = new DAComponent<DataList>;
		AddToGlobalCleanupList((IDAComponent **) &GAMETYPES);
		if (gameTypes->loadTypesData("GameTypes.db") == 0)
		{
			CQBOMB0("Load failed on GameTypes database.");
		}

		GENDATA = genData = new DAComponent<DataList>;
		AddToGlobalCleanupList((IDAComponent **) &GENDATA);
		if (genData->loadTypesData("GenData.db") == 0)
		{
			CQBOMB0("Load failed on GenData database.");
		}
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (EVENTSYS->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(GAMETYPES, &gameTypes->eventHandle);
		}
	}
};

static DataListComp dataListComp;

//-------------------------------------------------------------------
//-------------------------END DataList.cpp---------------------------
//-------------------------------------------------------------------
