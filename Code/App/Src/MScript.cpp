//--------------------------------------------------------------------------//
//                                                                          //
//                                MScript.cpp                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MScript.cpp 236   1/09/02 12:18p Tmauer $
*/			    
//--------------------------------------------------------------------------//

#include "pch.h"
#include <stdio.h>
#include <globals.h>
#include <MGlobals.h>
#include <MScript.h>
#include <CQTrace.h>
#include "Menu.h"
#include "UserDefaults.h"
#include <MPartRef.h>
#include <MGroupRef.h>
#include "ObjList.h"
#include "IObject.h"
#include "MPart.h"
#include "IMovieCamera.h"
#include "IObjectGenerator.h"
#include "ITrigger.h"
#include "ITeletype.h"
#include "SoundManager.h"
#include "CommPacket.h"
#include "FogOfWar.h"
#include "MScroll.h"
#include "ILineManager.h"
#include <DSpaceship.h>
#include "Mission.h"
#include "BaseHotRect.h"
#include "Ijumpgate.h"
#include "Ijumpplat.h"
#include "sysmap.h"
#include "common.h"
#include "OpAgent.h"
#include "IWeapon.h"
#include "DPlatform.h"
#include "IPlanet.h"
#include "IBanker.h"
#include "IAttack.h"
#include "Sector.h"
#include "IUnbornMeshList.h"
#include "IMissionActor.h"
#include "ObjMap.h"
#include "Ifabricator.h"
#include "ObjMapIterator.h"
#include "INugget.h"
#include "terrainmap.h"
#include "IGameProgress.h"
#include "IBriefing.h"
#include "Hotkeys.h"
#include "DResearch.h"
#include "UnitComm.h"
#include "MusicManager.h"
#include "Frame.h"
#include "IScriptObject.h"
#include <DCQGame.h>
#include "IWormholeBlast.h"
#include "ISubtitle.h"
#include "IAdmiral.h"
#include "Scripting.h"
#include "IInterfaceManager.h"

#include "IShapeLoader.h"
#include "DrawAgent.h"

#include "Resource.h"
#include <FileSys.h>
#include <ViewCnst.h>
#include <TSmartpointer.h>
#include <StringTable.h>
#include <SaveLoad.h>

#include <malloc.h>

#define MISSION_OVER_TELETYPE_MIN_X_POS 80
#define MISSION_OVER_TELETYPE_MAX_X_POS 560
#define MISSION_OVER_TELETYPE_MIN_Y_POS 300
#define MISSION_OVER_TELETYPE_MAX_Y_POS 330

static DWORD lastExtended;

//--------------------------------------------------------------------------//
//
struct MCachedPart
{
	U32 dwMissionID;
	IBaseObject::MDATA mdata;
	OBJPTR<IBaseObject> obj;

	MCachedPart()
	{
	}

	MCachedPart(MCachedPart &other)
	{
		dwMissionID = other.dwMissionID;
		memcpy(&mdata,&(other.mdata),sizeof(IBaseObject::MDATA));
		obj = other.obj;
	}

	MCachedPart & operator = (MCachedPart &other)
	{
		dwMissionID = other.dwMissionID;
		memcpy(&mdata,&(other.mdata),sizeof(IBaseObject::MDATA));
		obj = other.obj;
	}

	MCachedPart & operator = (U32 _dwMissionID)
	{
		dwMissionID = _dwMissionID;
		OBJLIST->FindObject(_dwMissionID, NONSYSVOLATILEPTR, obj);
		if (obj==0 || obj->GetMissionData(mdata)==0)
		{
			mdata.pSaveData = 0;
			mdata.pInitData = 0;
			obj = 0;
		}
		
		return *this;
	}


	bool isValid (void) const
	{
		return (obj.Ptr() != 0);
	}
};
#define MPARTREF_CACHESIZE 4
static MCachedPart partCache[MPARTREF_CACHESIZE];
//--------------------------------------------------------------------------//
//
static void clearCachedParts (void)   // called on close
{
	int i;

	for (i = 0; i < MPARTREF_CACHESIZE; i++)
	{
		partCache[i].dwMissionID = 0;
		partCache[i].mdata.pSaveData = 0;
		partCache[i].mdata.pInitData = 0;
		partCache[i].obj = 0;
	}
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
MCachedPart & MPartRef::getPartFromCache (void) const
{
	//
	// see if part is in the cache
	//
	if (index >= MPARTREF_CACHESIZE || partCache[index].dwMissionID != dwMissionID)
	{
		//
		// else it's not in the cache, add it to the cache
		//
		index = rand() % MPARTREF_CACHESIZE;
		partCache[index] = dwMissionID;
		pSave = partCache[index].mdata.pSaveData;
		pInit = partCache[index].mdata.pInitData;
	}

	return partCache[index];
}
//--------------------------------------------------------------------------//
//
const MISSION_DATA * MPartRef::GetInitData (void) const 
{
	MCachedPart & part = getPartFromCache();

	return part.mdata.pInitData;
}
//--------------------------------------------------------------------------//
//
const MISSION_SAVELOAD * MPartRef::operator -> (void) const
{
	MCachedPart & part = getPartFromCache();

	return part.mdata.pSaveData;
}
//--------------------------------------------------------------------------//
//
bool MPartRef::isValid (void) const
{
	MCachedPart & part = getPartFromCache();
	return (part.obj.Ptr() != 0);
}
//--------------------------------------------------------------------------//
//
struct PROGNODE
{
	PROGNODE * pNext;
	const CQSCRIPTENTRY * pScript;
	CQBaseProgram * pProgram;

	void * operator new (size_t size)
	{
		return calloc(size, 1);
	}
	void operator delete (void *ptr)
	{
		::free(ptr);
	}

	~PROGNODE (void)
	{
		if (pProgram)
			pProgram->Destroy();
	}
};
//--------------------------------------------------------------------------//

static const CQSCRIPTENTRY * g_pScripts;
static HANDLE g_hOldSymbols, g_hNewSymbols;
static const char * g_scriptDataType;
static void * g_pScriptData;
static U32 g_scriptDataSize;
static void * g_preprocessData;
static U32 g_preprocessSize;
static HINSTANCE g_hScriptDLL, g_hLocalDLL;
static PROGNODE * g_pPrograms;
static PROGNODE * g_pProgramsEnd;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
static void enableMenuItems (bool bEnable)
{
	if (bEnable)
	{
		MENU->EnableMenuItem(IDM_START_PROGRAM, MF_BYCOMMAND|MF_ENABLED);
		MENU->EnableMenuItem(IDM_KILL_PROGRAM, MF_BYCOMMAND|MF_ENABLED);

		HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
		MENUITEMINFO minfo;

		memset(&minfo, 0, sizeof(minfo));
		minfo.cbSize = sizeof(minfo);
		minfo.fMask = MIIM_ID | MIIM_TYPE;
		minfo.fType = MFT_STRING;
		minfo.wID = IDS_VIEWMISSIONDATA;
		minfo.dwTypeData = const_cast<char *>(_localLoadString(IDS_VIEWMISSIONDATA));
		minfo.cch = strlen(minfo.dwTypeData);
		InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);

		minfo.wID = IDS_VIEWPROGRAMDATA;
		minfo.dwTypeData = const_cast<char *>(_localLoadString(IDS_VIEWPROGRAMDATA));
		minfo.cch = strlen(minfo.dwTypeData);
		InsertMenuItem(hMenu, 0x7FFE, 1, &minfo);
	}
	else
	if (MENU)
	{
		MENU->EnableMenuItem(IDM_START_PROGRAM, MF_BYCOMMAND|MF_GRAYED);
		MENU->EnableMenuItem(IDM_KILL_PROGRAM, MF_BYCOMMAND|MF_GRAYED);
		HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
		RemoveMenu(hMenu, IDS_VIEWMISSIONDATA, MF_BYCOMMAND);
		RemoveMenu(hMenu, IDS_VIEWPROGRAMDATA, MF_BYCOMMAND);
	}
}

//--------------------------------------------------------------------------//
//		MSCRIPT functions...
//--------------------------------------------------------------------------//

void MScript::RegisterScriptProgram (CQSCRIPTENTRY * pTable)
{
	// add new node to the beginning of the list
	pTable->pNext = g_pScripts;
	g_pScripts = pTable;
}
//--------------------------------------------------------------------------//
//
void MScript::SetScriptData (const CQSCRIPTDATADESC & desc)
{
	COMPTR<IViewConstructor2> parser;

	CQASSERT(g_hNewSymbols == 0);
		
	g_preprocessData = desc.preprocessData;
	g_preprocessSize = desc.preprocessSize;
	g_scriptDataType = desc.typeName;
	g_pScriptData    = desc.data;
	g_scriptDataSize = desc.typeSize;
	g_hLocalDLL		 = desc.hLocal;

	if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
	{
		char * ptr2 = (char *) malloc(desc.preprocessSize + 1);
		memcpy(ptr2, desc.preprocessData, desc.preprocessSize);
		ptr2[desc.preprocessSize] = 0;		// null terminated string

		g_hNewSymbols = parser->ParseNewMemory(ptr2);
		::free(ptr2);
	}
}
//--------------------------------------------------------------------------//
//
static void saveParseData (IFileSystem * outFile)
{
#ifdef _DEBUG
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "\\Script\\ParseData";
	DWORD dwWritten;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc, file) == GR_OK)
		file->WriteFile(0, g_preprocessData, g_preprocessSize, &dwWritten, 0);
#endif
}
//--------------------------------------------------------------------------//
//
static void saveGlobalData (IFileSystem * outFile)
{
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "\\Script\\GlobalData";
	DWORD dwWritten;

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = CREATE_ALWAYS;
	if (outFile->CreateInstance(&fdesc, file) == GR_OK)
		file->WriteFile(0, g_pScriptData, g_scriptDataSize, &dwWritten, 0);
}
//--------------------------------------------------------------------------//
//
static void saveProgramData (IFileSystem * outFile)
{
	outFile->CreateDirectory("Programs");
	if (outFile->SetCurrentDirectory("Programs") != 0)
	{
		COMPTR<IFileSystem> file;
		char buffer[MAX_PATH];
		DAFILEDESC fdesc = buffer;
		PROGNODE * node = g_pPrograms;
		DWORD dwWritten;
		int i=0;

		while (node)
		{
			fdesc.lpImplementation = "DOS";
			fdesc.dwDesiredAccess = GENERIC_READ|GENERIC_WRITE;
			fdesc.dwShareMode = 0;  // no sharing
			fdesc.dwCreationDistribution = CREATE_ALWAYS;
			wsprintf(buffer, "%d-%s", i++, node->pScript->progName);
			if (outFile->CreateInstance(&fdesc, file) == GR_OK)
				file->WriteFile(0, ((char *)node->pProgram) + node->pScript->saveOffset, node->pScript->saveSize, &dwWritten, 0);

			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::Save (IFileSystem * outFile)
{
	outFile->CreateDirectory("\\Script");
	if (outFile->SetCurrentDirectory("\\Script") == 0)
		goto Done;

	RecursiveDelete(outFile);

	if (g_hScriptDLL && g_preprocessData)
	{
		saveParseData(outFile);
		saveGlobalData(outFile);
		saveProgramData(outFile);
	}

	MGlobals::SetLastStreamID(SOUNDMANAGER->GetLastStreamID());
	MGlobals::SetLastTeletypeID(TELETYPE->GetLastTeletypeID());

Done:
	outFile->SetCurrentDirectory("\\");
}
//--------------------------------------------------------------------------//
//
static void appendProgram (CQBaseProgram * program, const CQSCRIPTENTRY * script)
{
	PROGNODE * node = new PROGNODE;
	
	if (g_pProgramsEnd == 0)
		g_pProgramsEnd = g_pPrograms = node;
	else
	{
		g_pProgramsEnd->pNext = node;
		g_pProgramsEnd = node;
	}
	node->pScript = script;
	node->pProgram = program;
}
//--------------------------------------------------------------------------//
//
static void loadParseData (IFileSystem * inFile, IViewConstructor2 * parser)
{
	DAFILEDESC fdesc = "\\Script\\ParseData";
	DWORD len, dwRead;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	C8 * pTemp = 0;

	CQASSERT(g_hOldSymbols==0);
	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
		goto Done;
	len = inFile->GetFileSize(hFile);
	if ((pTemp = (C8 *) malloc (len+1)) == 0)
		goto Done;
	if (inFile->ReadFile(hFile, (void *)pTemp, len, &dwRead, 0) == 0)
		goto Done;
	pTemp[len] = 0;
	g_hOldSymbols = parser->ParseNewMemory(pTemp);
Done:
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);
	::free(pTemp);
}
//--------------------------------------------------------------------------//
// search through program list for a start program
//
static void runStartProgram (bool bBriefing)
{
	const CQSCRIPTENTRY * script = g_pScripts;

	while (script)
	{
		if ((bBriefing==0 && (script->eventFlags & CQPROGFLAG_STARTMISSION)!=0) ||
			(bBriefing!=0 && (script->eventFlags & CQPROGFLAG_STARTBRIEFING)!=0) )
		{
			CQBaseProgram * program = script->factory();
			CQASSERT(program);

			program->Initialize(bBriefing ? CQPROGFLAG_STARTBRIEFING : CQPROGFLAG_STARTMISSION,MPartRef());
			appendProgram(program, script);
		}
		script = script->pNext;
	}
}
//--------------------------------------------------------------------------//
//
static const CQSCRIPTENTRY * findScript (const char * name)
{
	const CQSCRIPTENTRY * result = g_pScripts;

	while (result)
	{
		if (strcmp(result->progName, name) == 0)
			break;
		result = result->pNext;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static void loadProgramData (IFileSystem * inFile, IViewConstructor2 * parser)
{
	DAFILEDESC fdesc = "\\Script\\Programs";
	COMPTR<IFileSystem> pDir;

	if (inFile->CreateInstance(&fdesc, pDir) == GR_OK)
	{
		WIN32_FIND_DATA data;
		HANDLE handle;
		fdesc.lpFileName = data.cFileName;

		if ((handle = pDir->FindFirstFile("?*-*", &data)) != INVALID_HANDLE_VALUE)
		do
		{
			char * ptr = strchr(data.cFileName+1, '-') + 1;
			const CQSCRIPTENTRY * script = findScript(ptr);

			if (script == 0)
			{
				CQERROR1("Can't find script '%s'", ptr);
			}
			else
			{	
				CQBaseProgram * program = script->factory();
				CQASSERT(program);
				// pNewData = address of user data
				void * pNewData = ((char *)program) + script->saveOffset;
				DWORD len, dwRead;
				HANDLE hFile = INVALID_HANDLE_VALUE;
				C8 * pTemp = 0;

				if ((hFile = pDir->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
				{
					CQERROR1("Could not open file '%s'", fdesc.lpFileName);
				}
				else
				{
					len = pDir->GetFileSize(hFile);
					pTemp = (C8 *) malloc (len);
					pDir->ReadFile(hFile, pTemp, len, &dwRead, 0);
					//
					// transmorgriphy data
					//
					if (len > script->saveSize)
						len = script->saveSize;

					if (g_hOldSymbols==0 || g_hNewSymbols==0)
						memcpy(pNewData, pTemp, len);		// simple copy
					else
					{
						SYMBOL hOldType = parser->GetSymbol(g_hOldSymbols, script->saveLoadStruct);
						SYMBOL hNewType = parser->GetSymbol(g_hNewSymbols, script->saveLoadStruct);
					
						if (hOldType==0 || hNewType==0 || parser->IsEqual(hOldType, hNewType))
							memcpy(pNewData, pTemp, len);		// simple copy
						else
							parser->CorrelateSymbol(hOldType, pTemp, hNewType, pNewData);
					}
					pDir->CloseHandle(hFile);
					::free(pTemp);
				}

				appendProgram(program, script);
			}
		} while (pDir->FindNextFile(handle, &data));
	}
}
//--------------------------------------------------------------------------//
//
static void loadGlobalData (IFileSystem * inFile, IViewConstructor2 * parser)
{
	DAFILEDESC fdesc = "\\Script\\GlobalData";
	DWORD len, dwRead;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	C8 * pTemp = 0;

	CQASSERT(g_pScriptData);

	if ((hFile = inFile->OpenChild(&fdesc)) == INVALID_HANDLE_VALUE)
	{
		goto Done;
	}
	len = inFile->GetFileSize(hFile);
	if (len == 0 || (pTemp = (C8 *) malloc (len)) == 0)
		goto Done;
	if (inFile->ReadFile(hFile, (void *)pTemp, len, &dwRead, 0) == 0)
		goto Done;
	//
	// transmorgriphy data
	//
	if (len > g_scriptDataSize)
		len = g_scriptDataSize;

	if (g_hOldSymbols==0 || g_hNewSymbols==0)
		memcpy(g_pScriptData, pTemp, len);		// simple copy
	else
	{
		SYMBOL hOldType = parser->GetSymbol(g_hOldSymbols, g_scriptDataType);
		SYMBOL hNewType = parser->GetSymbol(g_hNewSymbols, g_scriptDataType);
	
		if (hOldType==0 || hNewType==0 || parser->IsEqual(hOldType, hNewType))
			memcpy(g_pScriptData, pTemp, len);		// simple copy
		else
			parser->CorrelateSymbol(hOldType, pTemp, hNewType, g_pScriptData);
	}

Done:
	if (hFile != INVALID_HANDLE_VALUE)
		inFile->CloseHandle(hFile);
	::free(pTemp);
}
//--------------------------------------------------------------------------//
//
static void loadStringTable( IFileSystem & inFile )
{
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "\\";

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = OPEN_EXISTING;

	if( inFile.CreateInstance(&fdesc,file) == GR_OK )
	{
		COMPTR<ISaverLoader> stringTableLoader;
		if( STRINGTABLE->QueryInterface("ISaverLoader",stringTableLoader) == GR_OK )
		{		
			file->SetCurrentDirectory("\\");
			stringTableLoader->Load( *file.ptr );
		}
	}
}
//--------------------------------------------------------------------------//
//
static void loadScripts( IFileSystem & inFile )
{
	COMPTR<IFileSystem> file;
	DAFILEDESC fdesc = "\\";

	fdesc.lpImplementation = "DOS";
	fdesc.dwDesiredAccess = GENERIC_READ;
	fdesc.dwShareMode = 0;  // no sharing
	fdesc.dwCreationDistribution = OPEN_EXISTING;

	if( inFile.CreateInstance(&fdesc,file) == GR_OK )
	{
		COMPTR<ISaverLoader> scriptingLoader;
		if( SCRIPTING->QueryInterface("ISaverLoader",scriptingLoader) == GR_OK )
		{		
			file->SetCurrentDirectory("\\");
			scriptingLoader->Load( *file.ptr );
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::Load (IFileSystem * inFile)
{
	char abuffer[MAX_PATH];
	int len;
	const U32 updateCount = MGlobals::GetUpdateCount();

	SOUNDMANAGER->SetLastStreamID(MGlobals::GetLastStreamID());
	TELETYPE->SetLastTeletypeID(MGlobals::GetLastTeletypeID());
		
	if (SCRIPTSDIR)	// would be NULL in a DEMO build
		SCRIPTSDIR->GetFileName(abuffer, sizeof(abuffer));
	else
		abuffer[0] = 0;
	len = strlen(abuffer);
	if (len && abuffer[len-1] != '\\')
	{
		abuffer[len++] = '\\';
		abuffer[len] = 0;
	}
	if (MGlobals::GetScriptName(abuffer+len, sizeof(abuffer)-len))
	{
		if ((g_hScriptDLL = LoadLibrary(abuffer)) != 0)
		{
			if (g_preprocessData == 0 || g_preprocessSize == 0 || g_scriptDataType == 0 || g_pScriptData == 0 || g_scriptDataSize == 0 || g_hLocalDLL == 0)
			{
				CQERROR1("Invalid script file: %d", abuffer);
				FreeLibrary(g_hScriptDLL);
				g_hScriptDLL = 0;
				g_hLocalDLL = 0;
			}
			else
			{
				COMPTR<IViewConstructor2> parser;

				CQIMAGE->LoadSymTable(g_hScriptDLL);
				enableMenuItems(true);

				if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
				{
					if (inFile != 0)
					{
						loadParseData(inFile, parser);
						if (updateCount > 0)
						{
							loadGlobalData(inFile, parser);
							loadProgramData(inFile, parser);
						}
					}
				}
				else
					CQBOMB0("Parser missing?");
			}
		}
// this happens later (CQE_GAME__ACTIVE)
//		if (updateCount == 0 && DEFAULTS->GetDefaults()->bEditorMode==0)
//			runStartProgram(false);
	}

	// loading string table
	loadStringTable( *inFile );

	// loading new scripting
	loadScripts( *inFile );
}
//--------------------------------------------------------------------------//
//
void MScript::LoadScript (void)
{
	char abuffer[MAX_PATH];
	int len;

	SOUNDMANAGER->SetLastStreamID(MGlobals::GetLastStreamID());
	TELETYPE->SetLastTeletypeID(MGlobals::GetLastTeletypeID());

	SCRIPTSDIR->GetFileName(abuffer, sizeof(abuffer));
	len = strlen(abuffer);
	if (len && abuffer[len-1] != '\\')
	{
		abuffer[len++] = '\\';
		abuffer[len] = 0;
	}
	if (MGlobals::GetScriptName(abuffer+len, sizeof(abuffer)-len))
	{
		if ((g_hScriptDLL = LoadLibrary(abuffer)) != 0)
		{
			if (g_preprocessData == 0 || g_preprocessSize == 0 || g_scriptDataType == 0 || g_pScriptData == 0 || g_scriptDataSize == 0 || g_hLocalDLL == 0)
			{
				CQERROR1("Invalid script file: %d", abuffer);
				FreeLibrary(g_hScriptDLL);
				g_hScriptDLL = 0;
				g_hLocalDLL = 0;
			}
			else
			{
				CQIMAGE->LoadSymTable(g_hScriptDLL);
				enableMenuItems(true);
			}
		}
		runStartProgram(true);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::Close  (void)
{
	// shut down program list
	{
		PROGNODE * node = g_pPrograms;
		while (node)
		{
			g_pPrograms = node->pNext;
			delete node;
			node = g_pPrograms;
		}
		g_pProgramsEnd   = 0;
	}
	
	if (PARSER && (g_hNewSymbols || g_hOldSymbols))
	{
		COMPTR<IViewConstructor2> parser;

		if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
		{
			if (g_hNewSymbols)
				parser->DestroySymbols(g_hNewSymbols);
			g_hNewSymbols = 0;
			if (g_hOldSymbols)
				parser->DestroySymbols(g_hOldSymbols);
			g_hOldSymbols = 0;
		}
	}

	{
		g_preprocessData = 0;
		g_preprocessSize = 0;
		g_scriptDataType = 0;
		g_pScriptData    = 0;
		g_scriptDataSize = 0;
		g_pScripts		 = 0;
		g_hLocalDLL		 = 0;

		if (g_hScriptDLL)
		{
			CQIMAGE->UnloadSymTable(g_hScriptDLL);
			FreeLibrary(g_hScriptDLL);
			g_hScriptDLL = 0;
		}
	}

	enableMenuItems(false);
	if (MGlobals::GetScriptUIControl())
		EnableMouse(true);

	clearCachedParts();
}
//--------------------------------------------------------------------------//
//
void MScript::Update (void)
{
	PROGNODE * node = g_pPrograms, *prev = 0;
	while (node)
	{
		if (node->pProgram->Update() == false)
		{
			// remove this node
			if (prev)
			{
				if ((prev->pNext = node->pNext) == 0)	// removing end of the list
					g_pProgramsEnd = prev;
				delete node;
				node = prev->pNext;
			}
			else  // removing first node
			{
				if ((g_pPrograms = node->pNext) == 0) // removing end of the list
					g_pProgramsEnd = 0;
				delete node;
				node = g_pPrograms;
			}
		}
		else
		{
			prev = node;
			node = node->pNext;
		}
	}
}
//--------------------------------------------------------------------------//
//
static void initializeScriptListWnd (HWND hListbox)
{
	const CQSCRIPTENTRY * node = g_pScripts;

	while (node)
	{
		S32 index = SendMessage(hListbox, LB_ADDSTRING, 0, (LPARAM) node->progName);
		SendMessage(hListbox, LB_SETITEMDATA, index, (LONG) node);
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
static bool startScriptFromList (HWND hListbox)
{
	bool result = 0;
	S32 buffer[32];
	int numItems = SendMessage(hListbox, LB_GETSELITEMS, 32, (LONG)buffer);
	
	while (numItems-- > 0)
	{
		if (buffer[numItems] >= 0)
		{
			const CQSCRIPTENTRY * node = (const CQSCRIPTENTRY *) SendMessage(hListbox, LB_GETITEMDATA, buffer[numItems], 0);

			CQBaseProgram * program = node->factory();
			CQASSERT(program);

			program->Initialize(CQPROGFLAG_EDITOR,MPartRef());
			appendProgram(program, node);
			result = true;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static BOOL runScriptDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd, IDC_LIST1);
			initializeScriptListWnd(hList);
			SetFocus(hList);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			startScriptFromList(GetDlgItem(hwnd, IDC_LIST1));
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	
		case IDC_LIST1:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				if (startScriptFromList(GetDlgItem(hwnd, IDC_LIST1)))
					EndDialog(hwnd, 0);
				break;

			case LBN_SELCHANGE:
				EnableWindow(GetDlgItem(hwnd, IDOK), 1);	// enable button
				break;
			}
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static void initializeProgListWnd (HWND hListbox)
{
	PROGNODE * node = g_pPrograms;

	while (node)
	{
		S32 index = SendMessage(hListbox, LB_ADDSTRING, 0, (LPARAM) node->pScript->progName);
		SendMessage(hListbox, LB_SETITEMDATA, index, (LONG) node);
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
static bool viewProgFromList (HWND hListbox)
{
	bool result = 0;
	S32 buffer[32];
	int numItems = SendMessage(hListbox, LB_GETSELITEMS, 32, (LONG)buffer);
	
	while (numItems-- > 0)
	{
		if (buffer[numItems] >= 0)
		{
			PROGNODE * node = (PROGNODE *) SendMessage(hListbox, LB_GETITEMDATA, buffer[numItems], 0);
			void * pData = ((char *)node->pProgram) + node->pScript->saveOffset;
			COMPTR<IViewConstructor2> parser;

			if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
			{
				SYMBOL hNewType = parser->GetSymbol(g_hNewSymbols, node->pScript->saveLoadStruct);
				CQASSERT(hNewType);
				DEFAULTS->GetScriptData(hNewType, "Program Data", pData, node->pScript->saveSize);
			}

			result = true;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static void killProgFromList (HWND hListbox)
{
	S32 buffer[32];
	int numItems = SendMessage(hListbox, LB_GETSELITEMS, 32, (LONG)buffer);
	
	while (numItems-- > 0)
	{
		if (buffer[numItems] >= 0)
		{
			PROGNODE * kill = (PROGNODE *) SendMessage(hListbox, LB_GETITEMDATA, buffer[numItems], 0);
			PROGNODE * node = g_pPrograms, *prev = 0;
			while (node!=kill)
			{
				prev = node;
				node = node->pNext;
			}
			if (prev)
			{
				if ((prev->pNext = kill->pNext) == 0)
					g_pProgramsEnd = prev;
			}
			else
			{
				if ((g_pPrograms = kill->pNext) == 0)
					g_pProgramsEnd = 0;
			}
			delete kill;
		}
	}

	SendMessage(hListbox, LB_RESETCONTENT, 0, 0);
	initializeProgListWnd(hListbox);
}
//--------------------------------------------------------------------------//
//
static BOOL runViewProgDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd, IDC_LIST1);
			initializeProgListWnd(hList);
			SetFocus(hList);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			viewProgFromList(GetDlgItem(hwnd, IDC_LIST1));
			EndDialog(hwnd, 0);
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	
		case IDC_LIST1:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				if (viewProgFromList(GetDlgItem(hwnd, IDC_LIST1)))
					EndDialog(hwnd, 0);
				break;

			case LBN_SELCHANGE:
				EnableWindow(GetDlgItem(hwnd, IDOK), 1);	// enable button
				break;
			}
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
static BOOL killProgDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hwnd, IDC_LIST1);
			initializeProgListWnd(hList);
			SetFocus(hList);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			killProgFromList(GetDlgItem(hwnd, IDC_LIST1));
			EnableWindow(GetDlgItem(hwnd, IDOK), 0);	// disable button
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			break;
	
		case IDC_LIST1:
			switch (HIWORD(wParam))
			{
			case LBN_DBLCLK:
				killProgFromList(GetDlgItem(hwnd, IDC_LIST1));
				EnableWindow(GetDlgItem(hwnd, IDOK), 0);	// disable button
				break;

			case LBN_SELCHANGE:
				EnableWindow(GetDlgItem(hwnd, IDOK), 1);	// enable button
				break;
			}
			break;
		}
	}

	return result;
}
//--------------------------------------------------------------------------//
//
void MScript::Notify (U32 message, void * param)
{
	MSG * msg = (MSG *) param;

	switch (message)
	{
	case CQE_HOTKEY:
		{
			switch ((U32)param)
			{
			case IDH_SCRIPT_KEY_1:
				{
					MScript::RunProgramsWithEvent(CQPROGFLAG_HOTKEY_1);
//					MScript::EndMissionSplash("VFXShape!!ExitGame",600,true);
				}
				break;
			case IDH_SCRIPT_KEY_2:
				{
					MScript::RunProgramsWithEvent(CQPROGFLAG_HOTKEY_2);
				}
				break;
			case IDH_SCRIPT_KEY_3:
				MScript::RunProgramsWithEvent(CQPROGFLAG_HOTKEY_3);
				break;
			case IDH_SCRIPT_KEY_4:
				MScript::RunProgramsWithEvent(CQPROGFLAG_HOTKEY_4);
				break;
			case IDH_SCRIPT_KEY_5:
				MScript::RunProgramsWithEvent(CQPROGFLAG_HOTKEY_5);
				break;
			}
		}
		break;
	case CQE_GAME_ACTIVE:
		if (param!=0)
		{
			if (MGlobals::GetScriptUIControl())
				EnableMouse(false);
			if (MGlobals::GetUpdateCount() == 0 && DEFAULTS->GetDefaults()->bEditorMode==0)
				runStartProgram(false);
		}
		break;
	
	case WM_COMMAND:
		{
			switch (LOWORD(msg->wParam))
			{
			case IDM_KILL_PROGRAM:
				DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG11), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) killProgDlgProc, 0);
				break;
			case IDM_START_PROGRAM:
				DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG5), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) runScriptDlgProc, 0);
				break;
			case IDS_VIEWMISSIONDATA:
				if (g_pScriptData && g_hNewSymbols)
				{
					COMPTR<IViewConstructor2> parser;

					if (PARSER->QueryInterface("IViewConstructor2", parser) == GR_OK)
					{
						SYMBOL hNewType = parser->GetSymbol(g_hNewSymbols, g_scriptDataType);
						DEFAULTS->GetScriptData(hNewType, "Mission Data", g_pScriptData, g_scriptDataSize);
					}
				}
				break;

			case IDS_VIEWPROGRAMDATA:
				if (g_hNewSymbols)
					DialogBoxParam(hResource, MAKEINTRESOURCE(IDD_DIALOG10), hMainWindow, (int (__stdcall *)(struct HWND__ *,unsigned int,unsigned int,long)) runViewProgDlgProc, 0);
				break;
			}
		}
		break; // end case WM_COMMAND
	} // end switch message
}
//-------------------------------------------------------------------
//
static bool isAdmiral (BASIC_DATA * _data)
{
	BASE_SPACESHIP_DATA	* data  = (BASE_SPACESHIP_DATA	*) _data;
	return (data->objClass == OC_SPACESHIP && data->type == SSC_FLAGSHIP);
}
//--------------------------------------------------------------------------//
//---------------------------Begin scripting methods -----------------------//
//--------------------------------------------------------------------------//
//
void MScript::RunProgramByName (const char * progName, const MPartRef & part)
{
	const CQSCRIPTENTRY * node = g_pScripts;

	while (node)
	{
		if(strcmp(node->progName,progName) == 0)
		{
			CQBaseProgram * program = node->factory();
			CQASSERT(program);

			program->Initialize(CQPROGFLAG_EDITOR,part);
			appendProgram(program, node);
			return;
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::RunProgramByName (const char * progName, U32 dwMissionID)
{
	MScript::RunProgramByName(progName, GetPartByID(dwMissionID));
}
//--------------------------------------------------------------------------//
//
DWORD MScript::GetExtendedEventInfo ()
{
	return lastExtended;
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetExtendedEventPartRef (void)
{
	MPartRef result;
	IBaseObject * obj = OBJLIST->FindObject(lastExtended);
	IBaseObject::MDATA mdata;
	
	if (obj && obj->GetMissionData(mdata))
	{
		result.dwMissionID = mdata.pSaveData->dwMissionID;
		result.index = MPARTREF_CACHESIZE;
		result.pInit = mdata.pInitData;
		result.pSave = const_cast<MISSION_SAVELOAD *>(mdata.pSaveData);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
	/*
	 * Called to trigger all programs of a certain event type
	 */ 
void MScript::RunProgramsWithEvent(U32 eventType, const MPartRef & part, DWORD extendendInfo)
{
	const CQSCRIPTENTRY * node = g_pScripts;

	lastExtended = extendendInfo;
	while (node)
	{
		if(node->eventFlags & eventType)
		{
			CQBaseProgram * program = node->factory();
			CQASSERT(program);

			program->Initialize(eventType,part);
			appendProgram(program, node);
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::RunProgramsWithEvent(U32 eventType, U32 dwMissionID, DWORD extendendInfo)
{
	if(OBJLIST->FindObject(dwMissionID))
		MScript::RunProgramsWithEvent(eventType, MScript::GetPartByID(dwMissionID), extendendInfo);
}
//--------------------------------------------------------------------------//
//
	/*
	 * Called to trigger all programs of a certain event type
	 */ 
void MScript::RunProgramsWithEvent(U32 eventType)
{
	const CQSCRIPTENTRY * node = g_pScripts;

	while (node)
	{
		if(node->eventFlags & eventType)
		{
			CQBaseProgram * program = node->factory();
			CQASSERT(program);

			program->Initialize(eventType,MPartRef());
			appendProgram(program, node);
		}
		node = node->pNext;
	}
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::CreatePart (const char *szArchetype, const MPartRef & location, U32 playerID, const char * partName)
{
	MPartRef result;
	MCachedPart & loc = location.getPartFromCache();
	CQASSERT(loc.isValid());

	PARCHETYPE pArchetype = ARCHLIST->LoadArchetype(szArchetype);
	
	if (pArchetype)
	{
		BASIC_DATA * _data = (BASIC_DATA *) ARCHLIST->GetArchetypeData(pArchetype);
		U32 partID = MGlobals::CreateNewPartID(playerID);
		if (isAdmiral(_data))
			partID |= ADMIRAL_MASK;
		IBaseObject * rtObject = MGlobals::CreateInstance(pArchetype, partID);
		if (rtObject)
		{
			Vector position = loc.obj->GetPosition();
			U32 systemID = loc.obj->GetSystemID();

			OBJPTR<IPhysicalObject> phys;
			IBaseObject::MDATA mdata;

		
			// set initial supplies, hull points
			//
			MPartNC part = rtObject;

			if (part.isValid())
			{
				part->playerID = playerID;
// Since this command is only called form the mission, I don't care about's it's cost...
//				BANKER->UseCommandPt(playerID,part.pInit->resourceCost.commandPt);
				part->hullPoints = part->hullPointsMax;
				if ((part->mObjClass != M_HARVEST) && (part->mObjClass != M_GALIOT) &&(part->mObjClass != M_SIPHON))
					part->supplies   = part->supplyPointsMax;
				rtObject->SetReady(true);
			}

			if (playerID)
			{
				rtObject->SetVisibleToAllies(MGlobals::GetAllyMask(playerID));
				rtObject->UpdateVisibilityFlags();
			}
			
			OBJLIST->AddObject(rtObject);

			VOLPTR(IPhysicalObject) physObj;
			VOLPTR(IMovieCamera) cameraObj;
			physObj = rtObject;
			cameraObj = rtObject;

			VOLPTR(IPlatform) platform;
			
			platform = rtObject;
			if (platform && (!platform->IsDeepSpacePlatform()))		// yes! we are a platform
			{
				if(platform->IsJumpPlatform())
				{
					IBaseObject * bestJumpgate = NULL;
					IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
					SINGLE bestDistance=10e8;
					while(obj)
					{
						if(obj->GetSystemID() == systemID && obj->objClass == OC_JUMPGATE)
						{
							OBJPTR<IJumpGate> jumpgate;
							obj->QueryInterface(IJumpGateID,jumpgate);
							CQASSERT(jumpgate);
							if(!jumpgate->GetPlayerOwner())
							{
								SINGLE newDist = (position-obj->GetPosition()).magnitude_squared();
								if(newDist < bestDistance)
								{
									bestJumpgate = obj;
									bestDistance = newDist;
								}
							}
						}
						obj = obj->nextTarget;
					}
					if(bestJumpgate)
					{
						platform->ParkYourself(bestJumpgate);
					}
					else
					{
						CQASSERT(0 && "Could Not Place JumpPlat");
					}
				}
				else
				{
					//
					// find nearest open slot
					//
					IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
					S32 bestSlot=-1;
					SINGLE bestDistance=10e8;
					OBJPTR<IPlanet> planet, bestPlanet;
					U32 dwPlatformID = platform.Ptr()->GetPartID();

					while (obj)
					{
						if (obj->GetSystemID() == systemID && obj->QueryInterface(IPlanetID, planet)!=0)
						{
							U32 newSlot = planet->FindBestSlot(rtObject->pArchetype,&position);
							if(newSlot)
							{
								SINGLE newDist = (position-planet->GetSlotTransform(newSlot).translation).magnitude();
								if(newDist < bestDistance)
								{
									bestDistance = newDist;
									bestSlot = newSlot;
									bestPlanet = planet;
								}
							}

						} // end obj->GetSystemID() ...
						
						obj = obj->nextTarget;
					} // end while()

					if (bestPlanet && bestDistance < 10000)
					{
						CQASSERT(bestSlot);
						bestSlot = bestPlanet->AllocateBuildSlot(dwPlatformID, bestSlot);
						TRANSFORM trans = bestPlanet->GetSlotTransform(bestSlot);
						position = trans.translation;

						platform->ParkYourself(trans, bestPlanet.Ptr()->GetPartID(), bestSlot);
					}
					
					if(physObj)
					{
						physObj->SetPosition(position, systemID);
						ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
					}
				}
			}
			else if(platform && platform->IsJumpPlatform())
			{
				IBaseObject * jump = NULL;
				SINGLE dist = 0;
				IBaseObject * obj = OBJLIST->GetObjectList();
				while(obj)
				{
					if(obj->objClass == OC_JUMPGATE && obj->GetSystemID() == systemID)
					{
						if(jump)
						{
							SINGLE newDist =  (position-obj->GetPosition()).magnitude_squared();
							if(newDist < dist)
							{
								jump = obj;
								dist = newDist;
							}
						}
						else
						{
							jump = obj;
							dist =  (position-obj->GetPosition()).magnitude_squared();
						}
					}
					obj = obj->next;
				}
				if(jump)
				{
					platform->ParkYourself(jump);
					ENGINE->update_instance(platform.Ptr()->GetObjectIndex(),0,0);
				}
			}
			else if (cameraObj)
			{
				cameraObj->InitCamera();
				physObj->SetSystemID(systemID);
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}
			else if (physObj)
			{
//				RECT rect;
//				getObjectDropRect(rect,position);

				physObj->SetPosition(position, systemID);
				ENGINE->update_instance(physObj.Ptr()->GetObjectIndex(),0,0);
			}

			// the object has to initialize its footprint
			COMPTR<ITerrainMap> map;
			SECTOR->GetTerrainMap(systemID, map);
			if (map)
			{
				rtObject->SetTerrainFootprint(map);
			}

			result.dwMissionID = partID;
			result.index = MPARTREF_CACHESIZE;

			if (rtObject->GetMissionData(mdata))
			{
				result.pInit = mdata.pInitData;
				result.pSave = mdata.pSaveData;
			}
		}
	}

	if (partName != NULL)
	{
		strncpy((char*)result->partName.string, partName, sizeof(result->partName.string));
	}

	return result;
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetPartByName (const char *szPartName)
{
	MPartRef result;
	IBaseObject::MDATA mdata;
	IBaseObject * obj = OBJLIST->GetObjectList();

	bool bObjectFound = false;
	while (obj)
	{
		if (obj->GetMissionData(mdata))
		{
			if (strcmp(szPartName, mdata.pSaveData->partName) == 0)
			{	
				bObjectFound = true;
				result.dwMissionID = mdata.pSaveData->dwMissionID;
				result.index = MPARTREF_CACHESIZE;
				result.pInit = mdata.pInitData;
				result.pSave = mdata.pSaveData;
				break;
			}
		}

		obj = obj->next;
	}

	if(!bObjectFound)
		CQBOMB1("GetPartByName failed on %s",szPartName);
	return result;
}
//--------------------------------------------------------------------------//
//
bool MScript::IsPartValid (const char *szPartName)
{
	IBaseObject::MDATA mdata;
	IBaseObject * obj = OBJLIST->GetObjectList();

	while (obj)
	{
		if (obj->GetMissionData(mdata))
		{
			if (strcmp(szPartName, mdata.pSaveData->partName) == 0)
			{	
				return true;
			}
		}

		obj = obj->next;
	}

	return false;
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetPartByID (U32 dwMissionID)
{
	MPartRef result;
	IBaseObject * obj = OBJLIST->FindObject(dwMissionID);
	IBaseObject::MDATA mdata;
	
	if (obj && obj->GetMissionData(mdata))
	{
		result.dwMissionID = mdata.pSaveData->dwMissionID;
		result.index = MPARTREF_CACHESIZE;
		result.pInit = mdata.pInitData;
		result.pSave = const_cast<MISSION_SAVELOAD *>(mdata.pSaveData);
	}
	else
	{
		CQBOMB1("GetPartByID failed on %x",dwMissionID);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the first MPartRef in the object list
	 */
MPartRef MScript::GetFirstPart (void)
{
	MPartRef result;
	IBaseObject * obj = OBJLIST->GetTargetList();
	IBaseObject::MDATA mdata;
	if(obj && obj->GetMissionData(mdata))
	{
		result.dwMissionID = mdata.pSaveData->dwMissionID;
		result.index = MPARTREF_CACHESIZE;
		result.pInit = mdata.pInitData;
		result.pSave = const_cast<MISSION_SAVELOAD *>(mdata.pSaveData);
	}
	else
		CQASSERT(obj==0);

	return result;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the next MPartRef in the object list or an invalid part if it is the end of the list
	 */
MPartRef MScript::GetNextPart (const MPartRef & part)
{
	MCachedPart & cachPart = part.getPartFromCache();
	MPartRef result;
	IBaseObject * obj = cachPart.obj;
	if(obj)
		obj = obj->nextTarget;
	IBaseObject::MDATA mdata;
	if(obj && obj->GetMissionData(mdata))
	{
		result.dwMissionID = mdata.pSaveData->dwMissionID;
		result.index = MPARTREF_CACHESIZE;
		result.pInit = mdata.pInitData;
		result.pSave = const_cast<MISSION_SAVELOAD *>(mdata.pSaveData);
	}
	else
		CQASSERT(obj==0);	// obj must implement this
	return result;
}

//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the Jump PLAT on the other side of the wormhole for the given Jump PLAT
	 */
MPartRef MScript::GetJumpgateSibling (const MPartRef & part)
{
	MPartRef nullpart;	

	if (!part.isValid() || part->mObjClass != M_JUMPPLAT)
		return nullpart;

	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		OBJPTR<IJumpPlat> jumpplat;
		IBaseObject *sibling;
		object.obj->QueryInterface(IJumpPlatID,jumpplat);
		sibling = jumpplat->GetSibling();

		MPartRef result;
		IBaseObject::MDATA mdata;
		if (sibling && sibling->GetMissionData(mdata))
		{
			result.dwMissionID = mdata.pSaveData->dwMissionID;
			result.index = MPARTREF_CACHESIZE;
			result.pInit = mdata.pInitData;
			result.pSave = mdata.pSaveData;
			return result;
		}
	}
	
	return nullpart;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the System ID which the given Jump PLAT's wormhole takes you to
	 */
U32 MScript::GetJumpgateDestination (const MPartRef & part)
{
	if (!part.isValid() || part->mObjClass != M_JUMPPLAT)
		return 0;

	MPartRef sibling = GetJumpgateSibling(part);

	if (!sibling.isValid())
		return 0;
	
	return sibling->systemID;
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetJumpgateWormhole (const MPartRef & part)
{
	MPartRef nullpart;	

	if (!part.isValid() || part->mObjClass != M_JUMPPLAT)
		return nullpart;

	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		OBJPTR<IJumpPlat> jumpplat;
		IBaseObject *wormhole;
		object.obj->QueryInterface(IJumpPlatID,jumpplat);
		wormhole = jumpplat->GetJumpGate();
		MPartRef result;
		IBaseObject::MDATA mdata;
		if (wormhole && wormhole->GetMissionData(mdata))
		{
			result.dwMissionID = mdata.pSaveData->dwMissionID;
			result.index = MPARTREF_CACHESIZE;
			result.pInit = mdata.pInitData;
			result.pSave = mdata.pSaveData;
			return result;
		}
	}
	return nullpart;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the Jumpgate on the other side of the wormhole for the given Jumpgate
	 */
MPartRef MScript::GetWormholeSibling (const MPartRef & part)
{
	MPartRef nullpart;	

	if (!part.isValid() || part->mObjClass != M_JUMPGATE)
		return nullpart;

	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		IBaseObject *sibling;
		sibling = SECTOR->GetJumpgateDestination(object.obj);
		
		MPartRef result;
		IBaseObject::MDATA mdata;
		if (sibling && sibling->GetMissionData(mdata))
		{
			result.dwMissionID = mdata.pSaveData->dwMissionID;
			result.index = MPARTREF_CACHESIZE;
			result.pInit = mdata.pInitData;
			result.pSave = mdata.pSaveData;
			return result;
		}
	}
	
	return nullpart;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  This Method returns the System ID which the given Jumpgate wormhole takes you to
	 */
U32 MScript::GetWormholeDestination (const MPartRef & part)
{
	if (!part.isValid() || part->mObjClass != M_JUMPGATE)
		return 0;

	MPartRef sibling = GetWormholeSibling(part);

	if (!sibling.isValid())
		return 0;
	
	return sibling->systemID;
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetTowerID( const MPartRef & part )
{
	MCachedPart & object = part.getPartFromCache();
    if(object.obj)
    {
        OBJPTR<IScriptObject> scriptObj;
        object.obj->QueryInterface(IScriptObjectID,scriptObj);
        CQASSERT(scriptObj && "Passed a non-script object to GetTowerID");
        if(OBJLIST->FindObject(scriptObj->GetTowerID()))
            return GetPartByID(scriptObj->GetTowerID());
    }
    return MPartRef();
}
//--------------------------------------------------------------------------//
//
void MScript::SetAllies (U32 playerID, U32 allyID, bool bEnable)
{
	MGlobals::SetAlly(playerID,allyID,bEnable);
	SECTOR->ComputeSupplyForAllPlayers();
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Gets the players view of his ally
	 */
bool MScript::GetAllies (U32 playerID, U32 allyID)
{
	return (MGlobals::GetOneWayAllyMask(playerID) & (0x01 << (allyID-1))) != 0;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  return true if both parties are allied
	 */
bool MScript::AreAllies (U32 playerID1, U32 playerID2)
{
	return MGlobals::AreAllies(playerID1,playerID2);
}
//--------------------------------------------------------------------------//
//
	/*
	 * Returns true if given player has any platforms left
	 */
bool MScript::PlayerHasPlatforms(U32 playerID)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetPlayerID() == playerID && obj->objClass == OC_PLATFORM)
			return true;
		obj = obj->next;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
bool MScript::PlayerHasPlatformsInSystem(U32 playerID, U32 systemID)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetSystemID() == systemID && obj->GetPlayerID() == playerID && obj->objClass == OC_PLATFORM)
			return true;
		obj = obj->next;
	}
	return false;
}
//--------------------------------------------------------------------------//
//
	/*
	 * Returns true if given player has any units left
	 */
bool MScript::PlayerHasUnits(U32 playerID)
{
	IBaseObject * obj = OBJLIST->GetTargetList();
	while(obj)
	{
		if(obj->GetPlayerID() == playerID && (obj->objClass == OC_PLATFORM || obj->objClass == OC_SPACESHIP))
			return true;
		obj = obj->next;
	}
	return false;
}

//--------------------------------------------------------------------------//
//

void MScript::SetPartName (const MPartRef & part, const char * name)
{
	if (name)
		strncpy((char*)part->partName.string, name, sizeof(part->partName.string));
}
//--------------------------------------------------------------------------//
//
void MScript::SetReady (const MPartRef & part, bool bSetting)
{
	if (part.isValid())
	{
		MCachedPart & obj = part.getPartFromCache();
		obj.obj->SetReady(bSetting);
	}
}
//--------------------------------------------------------------------------//
//
struct ResourceCost MScript::GetResourceCost(const char * name)
{
	BASIC_DATA * data = (BASIC_DATA *)ARCHLIST->GetArchetypeData(name);
	if(data)
	{
		if(data->objClass == OC_SPACESHIP)
		{
			BASE_SPACESHIP_DATA * sData = (BASE_SPACESHIP_DATA *)data;
			return sData->missionData.resourceCost;
		}
		else if(data->objClass == OC_PLATFORM)
		{
			BASE_PLATFORM_DATA * sData = (BASE_PLATFORM_DATA *)data;
			return sData->missionData.resourceCost;
		}else if(data->objClass == OC_RESEARCH)
		{
			BASE_RESEARCH_DATA * sData = (BASE_RESEARCH_DATA *)data;
			return sData->cost;
		}
	}
	CQASSERT(0 && "GetResourceCost called on an invalid archetype"); 
	return ResourceCost();
}
//--------------------------------------------------------------------------//
//
void MScript::EnableMouse (bool bEnable)
{
	if (!bEnable)
	{
		if (Frame::g_pFocusFrame == NULL)
		{
			EVENTSYS->Send(CQE_KILL_FOCUS,0);
		}
		else
		{
			Frame::g_pFocusFrame->Notify(CQE_KILL_FOCUS, 0);
		}
	}
	else
	{
		if (Frame::g_pFocusFrame == NULL)
		{
			EVENTSYS->Send(CQE_SET_FOCUS,0);
		}
		else
		{
			Frame::g_pFocusFrame->Notify(CQE_SET_FOCUS, 0);
		}
		
	}

	MSCROLL->SetActive(bEnable);

	MGlobals::SetScriptUIControl(!bEnable);
	CURSOR->EnableCursor(bEnable);
}
//--------------------------------------------------------------------------//
//
bool MScript::IsMouseEnabled (void)
{
	return !MGlobals::GetScriptUIControl();
}
//--------------------------------------------------------------------------//
//
void MScript::EnableBriefingControls (bool _bEnable)
{
//	BOOL32 bEnable = (_bEnable == TRUE);
//	TOOLBAR->Notify(CQE_BRIEFING_CONTROLS, (void*)bEnable);
}
//--------------------------------------------------------------------------//
//
void MScript::EnableRegenMode (bool bEnable)
{
	MGlobals::SetRegenMode(bEnable);
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::SetAvailiableTech (TECHNODE node)
{
	MGlobals::SetTechAvailable(node);
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
struct TECHNODE MScript::GetPlayerTech (U32 playerID)
{
    return MGlobals::GetCurrentTechLevel(playerID);
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::SetPlayerTech (U32 playerID, TECHNODE node)
{
    MGlobals::SetCurrentTechLevel(node,playerID);
}
//--------------------------------------------------------------------------//
//
	/*
	 *	Get a counted string from a stringID
	 *
	 */
wchar_t * MScript::GetCountedString (U32 stringID)
{
	U16 *pData = NULL;
	HRSRC hRes;

	// find address of string resource group (16 in a group)
	if ((hRes = FindResourceEx(g_hLocalDLL , RT_STRING, MAKEINTRESOURCE((stringID/16)+1), findUserLang())) != 0)
	{
		HGLOBAL hGlobal;

		if ((hGlobal = LoadResource(g_hLocalDLL, hRes)) != 0)
		{
			if ((pData = (U16 *) LockResource(hGlobal)) != 0)
			{
				//
				// find the actual string within the group
				//
				int i = stringID % 16;
				U32 numChars;

				while (i-- > 0)
				{
					numChars = *pData++;
					pData += numChars;
				}
				
				numChars = *pData;
			}
		}
	}

	return (wchar_t*)pData;
}

//--------------------------------------------------------------------------//
//
	/*
	 *  End The Mission in Victory
	 */
void MScript::EndMissionVictory(U32 missionCompletedID)
{
	FlushStreams();

	// only set the mission completed flag for this mission if we are playing
	// the Terran Campaign
	if (MGlobals::GetPlayerRace(MGlobals::GetThisPlayer()) == M_TERRAN)
	{
		GAMEPROGRESS->SetMissionCompleted(missionCompletedID);
		GAMEPROGRESS->SetMissionsSeen(missionCompletedID);
	}

	EVENTSYS->Post(CQE_MISSION_PROG_ENDING, (void *) (1 << (MGlobals::GetThisPlayer()-1)));
}
//--------------------------------------------------------------------------//
//
	/*
	 *  End The Mission in Defeat
	 */
void MScript::EndMissionDefeat()
{
	FlushStreams();
	EVENTSYS->Post(CQE_MISSION_PROG_ENDING, 0);
}
//--------------------------------------------------------------------------//
//

void MScript::EndMissionSplash (const char * vfxName,SINGLE speed, bool bAllowExit)
{
	SPLASHINFO info;
	info.bAllowExit = bAllowExit;
	info.speed = speed;
	strncpy( info.vfxName, vfxName, 63 );
	info.vfxName[63] = 0;

//	// todo(aaj-5/6/2004): big hack to get the first splash screen up first
//
//	// do the loading
//	COMPTR<IShapeLoader> loader;
//	COMPTR<IDAComponent> pComp;
//	COMPTR<IDrawAgent> slide;
//
//	GENDATA->CreateInstance(vfxName, pComp);
//	pComp->QueryInterface("IShapeLoader", loader);
//
//	if( loader->CreateDrawAgent(0,slide) == GR_OK )
//	{
//		// start the drawing
//		PIPE->clear_buffers(RP_CLEAR_COLOR_BIT|RP_CLEAR_DEPTH_BIT,0);
//		PIPE->begin_scene();
//
//		// do the drawing
//		if(slide)
//		{
//			slide->Draw(0, 0, 0);
//		}
//
//		// end the drawing
//		PIPE->end_scene();
//		PIPE->swap_buffers();
//	}
//
//	// do the cleanup
//	loader.free();
//	slide.free();

	FlushStreams();
	EVENTSYS->Send(CQE_MISSION_ENDING_SPLASH, &info );
}

//--------------------------------------------------------------------------//
//
void MScript::EnableMovieMode (bool bEnable)
{
	if(bEnable)
	{
		CQFLAGS.bMovieMode = true;
		EVENTSYS->Send(CQE_MOVIE_MODE,(void *)true);
	}
	else
	{
		CQFLAGS.bMovieMode = false;
		EVENTSYS->Send(CQE_MOVIE_MODE,false);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::ChangeCamera (const MPartRef & camera, SINGLE time, S32 flags)
{
	MCachedPart & part = camera.getPartFromCache();
	CAMERAMANAGER->ChangeCamera(part.obj,time,flags);
}
//--------------------------------------------------------------------------//
//
void MScript::MoveCamera (const MPartRef & location, SINGLE time, S32 flags)
{
	if (location.isValid())
	{
		MCachedPart & part = location.getPartFromCache();
		CAMERAMANAGER->MoveCamera(part.obj,time,flags);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::CameraSourceShip (const MPartRef & sourceShip, SINGLE xLoc, SINGLE yLoc, SINGLE zLoc)
{
	if (sourceShip.isValid())
	{
		Vector offset(xLoc,yLoc,zLoc);
		MCachedPart & part = sourceShip.getPartFromCache();
		CAMERAMANAGER->SetSourceShip(part.obj,&offset);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::CameraTargetShip (const MPartRef & targetShip)
{
	if (targetShip.isValid())
	{
		MCachedPart & part = targetShip.getPartFromCache();
		CAMERAMANAGER->SetTargetShip(part.obj);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::ClearCameraQueue (void)
{
	CAMERAMANAGER->ClearQueue();
}
//--------------------------------------------------------------------------//
//
void MScript::SaveCameraPos (void)
{
	CAMERAMANAGER->SaveCameraPos();
}
//--------------------------------------------------------------------------//
//
void MScript::LoadCameraPos (SINGLE time, S32 flags)
{
	CAMERAMANAGER->LoadCameraPos(time, flags);
}
//--------------------------------------------------------------------------//
//
void MScript::SetGenFrequency (const MPartRef & generator, SINGLE mean, SINGLE minDiference)
{
	MCachedPart & part = generator.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<IObjectGenerator> gen;
	part.obj->QueryInterface(IObjectGeneratorID,gen);
	CQASSERT(gen);
	gen->SetGenerationFrequency(mean,minDiference);
}
//--------------------------------------------------------------------------//
//
void MScript::SetGenType (const MPartRef & generator, const char * partType)
{
	MCachedPart & part = generator.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<IObjectGenerator> gen;
	part.obj->QueryInterface(IObjectGeneratorID,gen);
	CQASSERT(gen);
	U32 typeID;
	if(partType && partType[0])
		typeID = ARCHLIST->GetArchetypeDataID(partType);
	else
		typeID = 0;
	gen->SetGenerationType(typeID);
}
//--------------------------------------------------------------------------//
//
void MScript::GenEnable (const MPartRef & generator, bool bEnable)
{
	MCachedPart & part = generator.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<IObjectGenerator> gen;
	part.obj->QueryInterface(IObjectGeneratorID,gen);
	CQASSERT(gen);
	gen->EnableGeneration(bEnable);
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GenForceGeneration (const MPartRef & generator)
{
	MCachedPart & part = generator.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<IObjectGenerator> gen;
	part.obj->QueryInterface(IObjectGeneratorID,gen);
	CQASSERT(gen);
	U32 partID = gen->ForceGeneration();
	return GetPartByID(partID);
}
//--------------------------------------------------------------------------//
//
void MScript::EnableTrigger (const MPartRef & trigger, bool bEnable)
{
	MCachedPart & part = trigger.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<ITrigger> trig;
	part.obj->QueryInterface(ITriggerID,trig);
	CQASSERT(trig);
	trig->EnableTrigger(bEnable);
}
//--------------------------------------------------------------------------//
//
void MScript::SetTriggerFilter (const MPartRef & trigger, U32 number, U32 flags, bool bAddTo)
{
	MCachedPart & part = trigger.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<ITrigger> trig;
	part.obj->QueryInterface(ITriggerID,trig);
	CQASSERT(trig);
	trig->SetFilter(number,flags,bAddTo);
}
//--------------------------------------------------------------------------//
//
void MScript::SetTriggerRange (const MPartRef & trigger, SINGLE range)
{
	MCachedPart & part = trigger.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<ITrigger> trig;
	part.obj->QueryInterface(ITriggerID,trig);
	CQASSERT(trig);
	trig->SetTriggerRange(range);
}
//--------------------------------------------------------------------------//
//
void MScript::SetTriggerProgram (const MPartRef & trigger, const char * progName)
{
	MCachedPart & part = trigger.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<ITrigger> trig;
	part.obj->QueryInterface(ITriggerID,trig);
	CQASSERT(trig);
	trig->SetTriggerProgram(progName);
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::GetLastTriggerObject (const MPartRef & trigger)
{
	MCachedPart & part = trigger.getPartFromCache();
	CQASSERT(part.obj);
	OBJPTR<ITrigger> trig;
	part.obj->QueryInterface(ITriggerID,trig);
	CQASSERT(trig);
	IBaseObject * obj = trig->GetLastTriggerObject();

	MPartRef result;
	IBaseObject::MDATA mdata;

	if (obj && obj->GetMissionData(mdata))
	{
		result.dwMissionID = mdata.pSaveData->dwMissionID;
		result.index = MPARTREF_CACHESIZE;
		result.pInit = mdata.pInitData;
		result.pSave = mdata.pSaveData;
	}

	return result;
}
//--------------------------------------------------------------------------//
//
/* general part methods */
//--------------------------------------------------------------------------//
//
//--------------------------------------------------------------------------//
//
	/*
	 *  Removes a part from the game (no Explosion)
	 */
void MScript::RemovePart(const MPartRef & part)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	OBJLIST->DeferredDestruction(object.dwMissionID);
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Makes an object explode
	 */
void MScript::DestroyPart(const MPartRef & part)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	THEMATRIX->ObjectTerminated(object.dwMissionID,0);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SetHullPoints(const MPartRef & part, U32 amount)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	MPartNC(object.obj)->hullPoints = amount;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SetMaxHullPoints(const MPartRef & part, U32 amount)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	MPartNC(object.obj)->hullPointsMax = amount;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SetSupplies(const MPartRef & part, U32 amount)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	MPartNC(object.obj)->supplies = amount;
	}
}
//--------------------------------------------------------------------------//
//
bool MScript::IsDead(const MPartRef & part)
{
	if(part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		if(object.obj->objClass == OC_PLATFORM)
		{
			OBJPTR<IPlatform> platform;
			object.obj->QueryInterface(IPlatformID,platform);
			return platform->IsReallyDead();
		}
		return false;

	}
	return true;
}
//--------------------------------------------------------------------------//
//
bool MScript::IsDeadToPlayer(const MPartRef & part, U32 playerID)
{
	if(part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		if(object.obj->objClass == OC_PLATFORM)
		{
			return !(object.obj->IsTargetableByPlayer(playerID));
		}
		return false;

	}
	return true;
}
//--------------------------------------------------------------------------//
//
bool MScript::IsVisibleToPlayer (const MPartRef & part, U32 playerID)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	return object.obj->IsVisibleToPlayer(playerID);
	}
	else
		return false;
}
//--------------------------------------------------------------------------//
//
void MScript::SetVisibleToPlayer (const MPartRef & part, U32 playerID)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	object.obj->SetVisibleToPlayer(playerID);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::ClearVisibility(const MPartRef & part)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		object.obj->SetVisibilityFlags(0);
	}
}
//--------------------------------------------------------------------------//
//
bool MScript::IsPlatform (const MPartRef & part)
{
	if (part.isValid())
	{
	    MCachedPart & object = part.getPartFromCache();

	    return object.obj->objClass == OC_PLATFORM;
	}
	else
		return false;
}

//--------------------------------------------------------------------------//
//
bool MScript::IsGunboat (const MPartRef & part)
{
	if (part.isValid())
	{
        if ( MGlobals::IsGunboat( part->mObjClass ) )
        {
            return true;
        }
    }

	return false;
}

//--------------------------------------------------------------------------//
//
bool MScript::IsSelected (const MPartRef & part)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	return object.obj->bSelected != 0;
	}
	else
		return false;
}
//--------------------------------------------------------------------------//
//
void MScript::SelectUnit (const MPartRef & part)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		OBJLIST->FlushSelectedList();
		OBJLIST->FlushHighlightedList();
		OBJLIST->HighlightObject(object.obj);
		OBJLIST->SelectHighlightedObjects();
	}
}

//--------------------------------------------------------------------------//
//
bool MScript::IsSelectedUnique (const MPartRef & part)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	return (object.obj->bSelected && (!(object.obj->nextSelected)));
	}
	else
		return false;
}
//--------------------------------------------------------------------------//
//
void MScript::EnableSelection (const MPartRef & part, bool selectable)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->bUnselectable = !selectable;
		if((!selectable) && object.obj->bSelected)
			OBJLIST->UnselectObject(object.obj);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableJumpCap (const MPartRef & part, bool jumpOk)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->caps.jumpOk = jumpOk;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableMoveCap (const MPartRef & part, bool moveOk)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->caps.moveOk = moveOk;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableAttackCap (const MPartRef & part, bool attackOk)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->caps.attackOk = attackOk;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableSpecialAttackCap (const MPartRef & part, bool attackOk)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->caps.specialAttackOk = attackOk;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableSpecialEOACap (const MPartRef & part, bool attackOk)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->caps.specialEOAOk = attackOk;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::MakeDerelict (MPartRef & part)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();

		if(object.mdata.pInitData->mObjClass != M_JUMPPLAT)
		{
			OBJPTR<IMissionActor> victim;
			object.obj->QueryInterface(IMissionActorID, victim);
			CQASSERT(victim != 0);
					
			U32 newDwTargetID = MGlobals::CreateNewPartID(object.obj->GetPlayerID());
			victim->PrepareTakeover(newDwTargetID, 0);
			if(!THEMATRIX->GetWorkingOp(object.obj))
			{
				THEMATRIX->FlushOpQueueForUnit(object.obj);
				OBJLIST->UnselectObject(object.obj);

				// change the player ID of the victim
				victim->TakeoverSwitchID(newDwTargetID);

				part.dwMissionID = newDwTargetID;
				MCachedPart & object2 = part.getPartFromCache();

				object2.obj->SetReady(false);
				if(object2.obj->objMapNode)
					object2.obj->objMapNode->flags &= ~(OM_TARGETABLE);
				MPartNC part(object2.obj);
				part->bDerelict = true;

				LPSTR strDisabled = const_cast<char *>(_localLoadString(IDS_DISABLED));
				char mystr[100];
				sprintf(mystr,"%.18s (%.10s)",part->partName.string,strDisabled);
				strcpy(part->partName.string, mystr);
				//sprintf(part->partName.string,"%s %s",strDisabled,part->partName.string);

				OBJPTR<IPhysicalObject> phys;
				object2.obj->QueryInterface(IPhysicalObjectID,phys);
				if(phys)
				{
					phys->SetTransform(object2.obj->GetTransform(),object2.obj->GetSystemID());
				}
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::MakeInvincible (MPartRef & part,bool value)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();

		MPartNC part = object.obj;
		part->bInvincible = value;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::MakeNonAutoTarget (MPartRef & part,bool value)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();

		MPartNC part = object.obj;
		part->bNoAutoTarget = value;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::MakeNonDerelict (const MPartRef & part)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->bDerelict = false;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::MakeJumpgateInvisible (const MPartRef & part, bool value)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		VOLPTR(IJumpGate) gate = object.obj;
		if(gate)
		{
			gate->SetJumpgateInvisible(value);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SwitchPlayerID (const MPartRef & part, U32 playerID)
{
	bool bjumpplat=false;
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		IBaseObject * target = object.obj;

		// if the visuals for popping off pods are done, go to next stage
		OBJPTR<IMissionActor> victim;
		target->QueryInterface(IMissionActorID, victim);
		CQASSERT(victim != 0);

		MPartNC part = target;

		// take over the target
		if (!((target->GetPlayerID() == playerID) || (!(MPart(target)->bReady))))
		{
			OBJPTR<IJumpPlat> jumpPlat;
			OBJPTR<IMissionActor> victim2;
			U32 dwTargetID = object.dwMissionID;
			U32 oldJumpID = 0;
			U32 newJumpPlatID = 0;
			if(victim.Ptr()->objClass == OC_PLATFORM)
			{
				victim->QueryInterface(IJumpPlatID,jumpPlat);
				if(jumpPlat)
				{
					bjumpplat = true;
					
					// GetSibling has been known to return NULL
					IBaseObject * sibling = jumpPlat->GetSibling();
					if (sibling)
					{
						sibling->QueryInterface(IMissionActorID,victim2);
						oldJumpID = victim2.Ptr()->GetPartID();
						newJumpPlatID = (oldJumpID& ~0xF) | playerID;
					}
				}
			}

			U32 newDwTargetID = (dwTargetID & ~0xF) | playerID;

			victim->PrepareTakeover(newDwTargetID, 0);
			if(!THEMATRIX->GetWorkingOp(victim.Ptr()))
			{
				THEMATRIX->FlushOpQueueForUnit(victim.Ptr());
				if(victim2)
				{
					victim2->PrepareTakeover(newJumpPlatID,0);
					if(THEMATRIX->GetWorkingOp(victim2.Ptr()))
						return;
					THEMATRIX->FlushOpQueueForUnit(victim2.Ptr());
				}

				OBJLIST->UnselectObject(target);
				if(victim2)
					OBJLIST->UnselectObject(victim2.Ptr());

				// change the player ID of the victim
				victim->TakeoverSwitchID(newDwTargetID);
				if(victim2)
					victim2->TakeoverSwitchID(newJumpPlatID);

				part->playerID = playerID;

				// adding something here to update the objects tech-tree, give me hell if it' wrong -sbarton
				MGlobals::UpgradeMissionObj(OBJLIST->FindObject(newDwTargetID));

				if (bjumpplat)	
				{				
					MPartRef part2;
					IBaseObject* jumpPlat2;
					jumpPlat2 = jumpPlat->GetSibling();
					MPartNC ncpart2=jumpPlat2;
					if(ncpart2.isValid())
					{
						ncpart2->playerID = playerID;
					}
				}
			}

		}
	}
}
//--------------------------------------------------------------------------//
//
bool MScript::WasPart(const MPartRef & oldPart, U32 newPartID)
{
	return (oldPart.dwMissionID&0xFFFFFFF0) == (newPartID &0xFFFFFFF0);
}
//--------------------------------------------------------------------------//
//
void MScript::EnableGenerateAllEvents (const MPartRef & part, bool bEnable)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->bAllEventsOn = bEnable;
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnablePartnameDisplay (const MPartRef & part, bool bEnable)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		MPartNC part(object.obj);
		part->bShowPartName = bEnable;
	}
}
//--------------------------------------------------------------------------//
//
bool MScript::IsIdle(const MPartRef & part)
{
	if (part.isValid())
	{
	MCachedPart & object = part.getPartFromCache();
//	CQASSERT(object.obj);
	return !THEMATRIX->HasPendingOp(object.obj);
	}
	else
		return true;
}
//--------------------------------------------------------------------------//
//
	/*
	 *  returns the distance bettween two objects return -1 if one object does not exist or are in different systems.
	 */
SINGLE MScript::DistanceTo(const MPartRef & part1, const MPartRef & part2)
{
	MCachedPart object1 = part1.getPartFromCache();
	MCachedPart & object2 = part2.getPartFromCache();
	if(object1.obj && object2.obj)
	{
		if(object1.obj->GetSystemID() == object2.obj->GetSystemID())
		{
			return (object1.obj->GetGridPosition()-object2.obj->GetGridPosition());
		}
	}
	return -1;
}
//--------------------------------------------------------------------------//
//
bool MScript::IsFabBuilding (const MPartRef & part,U32 buildID)
{
	if(part.isValid())
	{
		return UNBORNMANAGER->IsFabBuilding(part.dwMissionID,buildID);
	}
	return false;
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetFabBuildID (char * buildType)
{
	return ARCHLIST->GetArchetypeDataID(buildType);
}
//--------------------------------------------------------------------------//
//
	/*
	 *	Documentation goes here!
	 */
void MScript::EnableJumpgate(const MPartRef & part, bool jumpOn)
{
	MCachedPart & object = part.getPartFromCache();
	CQASSERT(object.obj);
	OBJPTR<IJumpGate> jump;
	object.obj->QueryInterface(IJumpGateID,jump);
	jump->SetJumpAllowed(jumpOn);
}
//--------------------------------------------------------------------------//
//
void MScript::ClearPath(U32 systemA, U32 systemB, U32 playerID)
{
	Vector startPos(0,0,0);
	U32 list[16];
	U32 numSystems = SECTOR->GetShortestPath(systemA,systemB,list,0);
	SECTOR->RevealSystem(systemA,playerID);
	for(U32 i = 1; i < numSystems; ++i)
	{
		SECTOR->RevealSystem(list[i],playerID);
		IBaseObject * obj = SECTOR->GetJumpgateTo(list[i-1],list[i],startPos);
		CQASSERT(obj);
		obj->SetVisibleToPlayer(playerID);
		obj->UpdateVisibilityFlags2();
		obj = SECTOR->GetJumpgateDestination(obj);
		obj->SetVisibleToPlayer(playerID);
		obj->UpdateVisibilityFlags2();
	}

}
//--------------------------------------------------------------------------//
//
IBaseObject * searchSystem(GRIDVECTOR basePos, U32 playerID, U32 currentSystemID, bool bVisible, SINGLE & distTo)
{
	Vector pos(0,0,0);
	IBaseObject * closestFound = NULL;

	const U32 allyMask = MGlobals::GetAllyMask(playerID);
	ObjMapIterator it(currentSystemID, pos, 128 * GRIDSIZE, playerID);

	while (it)
	{
		if ((it->flags & OM_UNTOUCHABLE) == 0 && (it->flags & OM_TARGETABLE))
		{
			MPart part = it->obj;

			if (part.isValid() && part->bReady)
			{
				const U32 hisPlayerID = it.GetApparentPlayerID(allyMask);

				// are we enemies
				if (MGlobals::AreAllies(playerID, hisPlayerID) == false)
				{
					if ((bVisible == false) || it->obj->IsVisibleToPlayer(playerID))
					{
						if (closestFound)
						{
							SINGLE newDist = basePos - it->obj->GetGridPosition();
							if(newDist < distTo)
							{
								distTo = newDist;
								closestFound = it->obj;
							}
						}
						else
						{
							distTo = basePos - it->obj->GetGridPosition();
							closestFound = it->obj;
						}
					}
				}
			}
		}
		++it;
	}

	return closestFound;
/*	IBaseObject * closestFound = NULL;
	IBaseObject * search = OBJLIST->GetTargetList();
	while(search)
	{
		if(search->GetSystemID() == currentSystem && search->GetPlayerID() && (search->objMapNode!=0))
		{
			if(!(MGlobals::AreAllies(playerID,search->GetPlayerID())))
			{
				if((!bVisible) || (search->IsVisibleToPlayer(playerID)))
				{
					if(search->objClass & CF_TARGETABLE)
					{
						if(closestFound)
						{
							SINGLE newDist = basePos-search->GetGridPosition();
							if(newDist < distTo)
							{
								distTo = newDist;
								closestFound = search;
							}
						}
						else
						{
							distTo = basePos-search->GetGridPosition();
							closestFound = search;
						}
					}
				}
			}
		}
		search = search->nextTarget;
	}
	return closestFound;
*/
}
//--------------------------------------------------------------------------//
//
MPartRef MScript::FindNearestEnemy(const MPartRef & part,bool bMultiSystem,bool bVisible)
{
	MCachedPart & object = part.getPartFromCache();
	if(!object.obj)
		return MPartRef();
	CQASSERT(object.obj);
	if(object.obj->GetSystemID()& HYPER_SYSTEM_MASK)
		return MPartRef();

	U32 playerID = object.obj->GetPlayerID();
	U32 currentSystem = (object.obj->GetSystemID()& ~HYPER_SYSTEM_MASK);

	IBaseObject * bestFound = NULL;
	SINGLE distTo = 0;
	if(bMultiSystem)
	{
		GRIDVECTOR baseSystem[16];
		SINGLE distlist[16];
		memset(distlist,0,sizeof(SINGLE)*16);
		baseSystem[currentSystem] = object.obj->GetGridPosition();
		distlist[currentSystem] = 0;
		U32 systemsChecked = 0;
		U32 systemsToSearch = (0x01 <<currentSystem);
		while((!bestFound) && systemsToSearch)
		{
			for(U32 i = 0; i <16; ++i)
			{
				if((systemsToSearch>>i) & 0x01)
				{
					SINGLE testDist;
					IBaseObject * test = searchSystem(baseSystem[i],playerID,i,bVisible,testDist);
					if(test)
					{
						testDist += distlist[i];
						if(bestFound)
						{
							if(testDist < distTo)
							{
								distTo = testDist;
								bestFound = test;
							}
						}
						else
						{
							distTo = testDist;
							bestFound = test;
						}
					}
					systemsChecked |= (0x01 << i);
				}
			}
			if(!bestFound)
			{
				systemsToSearch = 0;
				for(U32 i = 0; i <16; ++i)
				{
					if((systemsChecked>>i) & 0x01)
					{
						for(U32 j = 0; j <16; ++j)
						{
							if((!((systemsChecked>>j) & 0x01)) && (!((systemsToSearch>>j) & 0x01)))
							{
								IBaseObject * jump = SECTOR->GetJumpgateTo(i,j,baseSystem[i]);
								if(jump)
								{
									distlist[j] = distlist[i]+(jump->GetGridPosition()-baseSystem[i]);
									baseSystem[j] = (SECTOR->GetJumpgateDestination(jump))->GetGridPosition();
									systemsToSearch |= (0x01 << j);
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		bestFound = searchSystem(object.obj->GetGridPosition(),playerID,currentSystem,bVisible,distTo);
	}
	if(bestFound)
	{
		return GetPartByID(bestFound->GetPartID());
	}
	return MPartRef();
}
//--------------------------------------------------------------------------//
//
	/*
	 *	Documentation goes here!
	 */
U32 MScript::GetAvailableSlots(const MPartRef & part)
{
	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		OBJPTR<IPlanet> planet;
		object.obj->QueryInterface(IPlanetID,planet);
		return planet->GetEmptySlots();
	}
	return 0;
}
//--------------------------------------------------------------------------//
//
bool MScript::IsPlayerOnPlanet (const MPartRef & planet, U32 playerID)
{
	MCachedPart & object = planet.getPartFromCache();
	if(object.obj)
	{
		OBJPTR<IPlanet> planet;
		object.obj->QueryInterface(IPlanetID,planet);
		if(planet)
		{
			for(U32 i = 0; i < 12; ++i)
			{
				U32 slotOwner = planet->GetSlotUser(0x01 << i);
				if(slotOwner && ((MGlobals::GetPlayerFromPartID(slotOwner)) == playerID))
					return true;
			}
		}
	}
	return false;
}

void MScript::TeraformPlanet (const MPartRef & part, const char * planetType, SINGLE changeTime)
{
	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		OBJPTR<IPlanet> planet;
		object.obj->QueryInterface(IPlanetID,planet);
		if(planet)
		{
			SINGLE angle = (((SINGLE)(rand()%10000))/10000.0f)*PI*2;
			Vector hitDir(cos(angle),sin(angle),0);
			planet->TeraformPlanet(planetType,changeTime,hitDir);
		}
	}
}
//--------------------------------------------------------------------------//
//

//--------------------------------------------------------------------------//
//
	/*
	 *	Documentation goes here!
	 */
void MScript::EnableSystem(U32 systemID, bool bEnable, bool bVisible)
{
	U32 i;
	if(bEnable)
	{
		for(i = 1; i <=MAX_PLAYERS;++i)
		{
			if(bVisible)
				SECTOR->SetAlertState(systemID,S_VISIBLE,i);
			else
				SECTOR->SetAlertState(systemID,0,i);
		}
	}
	else
	{
		for(i = 1; i <=MAX_PLAYERS;++i)
		{
			SECTOR->SetAlertState(systemID,S_LOCKED,i);
		}
	}
}

//--------------------------------------------------------------------------//
//
	/*
	 *	Documentation goes here!
	 */

U32 MScript::GetCurrentSystem (void)
{
	return SECTOR->GetCurrentSystem();
}
//--------------------------------------------------------------------------//
//
/* wormhole anim stuff */
//--------------------------------------------------------------------------//
//
U32 MScript::CreateWormBlast(const MPartRef & target, SINGLE gridRadius, U32 playerID, bool suckOut)
{
	U32 result = 0;
	if(target.isValid())
	{
		MCachedPart & targetObj = target.getPartFromCache();

		IBaseObject * obj = ARCHLIST->CreateInstance("WORMEFF!!S_WORM");
		CQASSERT(obj);
		OBJLIST->AddObject(obj);
		OBJPTR<IWormholeBlast> hole1;
		obj->QueryInterface(IWormholeBlastID,hole1,NONSYSVOLATILEPTR);
		CQASSERT(hole1);
		TRANSFORM trans = targetObj.obj->GetTransform();
		hole1->InitWormholeBlast(trans,targetObj.obj->GetSystemID(),gridRadius*GRIDSIZE,playerID,suckOut);
		hole1->SetScriptID(MGlobals::CreateNewPartID(0));
        result = hole1->GetScriptID();
    }
	return result;
}
//--------------------------------------------------------------------------//
//
void MScript::FlashWormBlast(U32 wormID)
{
    IBaseObject * obj = OBJLIST->GetObjectList();
	OBJPTR<IWormholeBlast> hole1;
	while(obj)
	{
		obj->QueryInterface(IWormholeBlastID,hole1,NONSYSVOLATILEPTR);
		if(hole1 && wormID == hole1->GetScriptID())
		{
			break;
		}
		else
			obj = obj->next;
	}
	if(hole1)
	{
		hole1->Flash();
	}
}
//--------------------------------------------------------------------------//
//
void MScript::CloseWormBlast(U32 wormID)
{
    IBaseObject * obj = OBJLIST->GetObjectList();
	OBJPTR<IWormholeBlast> hole1;
	while(obj)
	{
		obj->QueryInterface(IWormholeBlastID,hole1,NONSYSVOLATILEPTR);
		if(hole1 && wormID == hole1->GetScriptID())
		{
			break;
		}
		else
			obj = obj->next;
	}
	if(hole1)
	{
		hole1->CloseUp();
	}
}
//--------------------------------------------------------------------------//
//
/* orders */
//--------------------------------------------------------------------------//
//
void MScript::OrderTeleportTo (const MPartRef & part, const MPartRef & target)
{

	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
		// CQASSERT(object.obj);
		MCachedPart & targetObj = target.getPartFromCache();
		// CQASSERT(targetObj.obj);
		
		OBJPTR<IPhysicalObject> phys;
		object.obj->QueryInterface(IPhysicalObjectID,phys);
		if(phys)
		{
			phys->SetPosition(targetObj.obj->GetPosition(), targetObj.obj->GetSystemID());
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderMoveTo (const MPartRef & part, const MPartRef & target, bool bQueueOn)
{
	if (part.isValid() && target.isValid())
	{
	MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
	//CQASSERT(object.obj);
	MCachedPart & targetObj = target.getPartFromCache();
	//CQASSERT(targetObj.obj);

	USR_PACKET<USRMOVE> packet;

	if(bQueueOn == true)
	{
		packet.userBits = 1;
	}
	else
	{
		packet.userBits = 0;
	}

	packet.objectID[0] = object.obj->GetPartID();

	packet.position.init(targetObj.obj->GetGridPosition(),targetObj.obj->GetSystemID() & ~HYPER_SYSTEM_MASK);
	packet.init(1);

	NETPACKET->Send(HOSTID,0,&packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderUseJumpgate (const MPartRef & part, const MPartRef & jumpgate, bool bQueueOn)
{
	if (part.isValid() && jumpgate.isValid())
	{
		USR_PACKET<USRJUMP> packet;

		if(bQueueOn == true)
		{
			packet.userBits = 1;
		}
		else
		{
			packet.userBits = 0;
		}

		packet.objectID[0] = part.dwMissionID;
		packet.jumpgateID = jumpgate.dwMissionID;
		packet.init(1);

		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderAttack (const MPartRef & part, const MPartRef & target, bool user_command )
{
	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
		//CQASSERT(object.obj);

		MCachedPart & objTarget = target.getPartFromCache();
		//CQASSERT(objTarget.obj);

		USR_PACKET<USRATTACK> packet;
		packet.objectID[0] = object.obj->GetPartID();
		packet.targetID = objTarget.obj->GetPartID();
		packet.destSystemID = 0;		// go to any system
		packet.bUserGenerated = user_command;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderAttackPosition (const MPartRef & part, const MPartRef & target, bool user_command)
{
	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
		//CQASSERT(object.obj);

		MCachedPart & objTarget = target.getPartFromCache();
		//CQASSERT(objTarget.obj);

		USR_PACKET<USRAOEATTACK> packet;
		packet.objectID[0] = object.obj->GetPartID();
		packet.position.init(objTarget.obj->GetPosition(),objTarget.obj->GetSystemID());
		packet.bSpecial = false;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderEscort (const MPartRef & part, const MPartRef & target )
{
	if (part.isValid() && target.isValid())
	{
	    MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
	    MCachedPart & objTarget = target.getPartFromCache();

		USR_PACKET<USRESCORT> packet;
		packet.objectID[0] = object.obj->GetPartID();
		packet.targetID = objTarget.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
    }
}
//--------------------------------------------------------------------------//
//
void MScript::OrderMoveTo (const MGroupRef & group, const MPartRef & target, bool bQueueOn)
{
	if (group > 0 && target.isValid())
	{
		MCachedPart & targetObj = target.getPartFromCache();
		//CQASSERT(targetObj.obj);
		group.verify();

		USR_PACKET<USRMOVE> packet;

		if(bQueueOn == true)
		{
			packet.userBits = 1;
		}
		else
		{
			packet.userBits = 0;
		}

		memcpy(packet.objectID, group.objectIDs, sizeof(packet.objectID));
		packet.position.init(targetObj.obj->GetGridPosition(),targetObj.obj->GetSystemID() & ~HYPER_SYSTEM_MASK);
		packet.init(group.numObjects);

		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderAttack (const MGroupRef & group, const MPartRef & target, bool user_command)
{
	if (group > 0 && target.isValid())
	{
		MCachedPart & objTarget = target.getPartFromCache();
		//CQASSERT(objTarget.obj);
		group.verify();

		USR_PACKET<USRATTACK> packet;
		memcpy(packet.objectID, group.objectIDs, sizeof(packet.objectID));
		packet.targetID = objTarget.obj->GetPartID();
		packet.destSystemID = 0;		// go to any system
		packet.bUserGenerated = user_command;
		packet.init(group.numObjects);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderUseJumpgate (const struct MGroupRef & group, const MPartRef & jumpgate, bool bQueueOn)
{
	if (group > 0 && jumpgate.isValid())
	{
		USR_PACKET<USRJUMP> packet;
		group.verify();

		if(bQueueOn == true)
		{
			packet.userBits = 1;
		}
		else
		{
			packet.userBits = 0;
		}

		memcpy(packet.objectID, group.objectIDs, sizeof(packet.objectID));
		packet.jumpgateID = jumpgate.dwMissionID;
		packet.init(group.numObjects);

		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Supports special attacks, Area of effect, Mimic, and Probe
	 */
void MScript::OrderSpecialAttack (const MPartRef & part, const MPartRef & target)
{
	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
		//CQASSERT(object.obj);

		MCachedPart & objTarget = target.getPartFromCache();
		//CQASSERT(objTarget.obj);

		if(object.mdata.pSaveData->caps.specialAttackOk ||object.mdata.pSaveData->caps.specialTargetPlanetOk)
		{
			USR_PACKET<USRSPATTACK> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.targetID = objTarget.obj->GetPartID();
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		else if(object.mdata.pSaveData->caps.specialEOAOk)
		{
			USR_PACKET<USRAOEATTACK> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.position.init(objTarget.obj->GetGridPosition(),objTarget.obj->GetSystemID() & ~HYPER_SYSTEM_MASK);
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		else if(object.mdata.pSaveData->caps.mimicOk)
		{
			USR_PACKET<USRMIMIC> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.targetID = objTarget.obj->GetPartID();
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		else if(object.mdata.pSaveData->caps.probeOk)
		{
			USR_PACKET<USRPROBE> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.position.init(objTarget.obj->GetGridPosition(),objTarget.obj->GetSystemID() & ~HYPER_SYSTEM_MASK);
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		else if(object.mdata.pSaveData->caps.captureOk)
		{
			USR_PACKET<USRCAPTURE> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.targetID = objTarget.obj->GetPartID();
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderDoWormhole (const MPartRef & part, U32 systemID)
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		USR_PACKET<USRCREATEWORMHOLE> packet;
		packet.objectID[0] = object.obj->GetPartID();
		packet.systemID = systemID;
		packet.init(1);
		NETPACKET->Send(HOSTID, 0, &packet);
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::OrderSpecialAbility (const MPartRef & part)
{
	if (part.isValid())
	{

		MCachedPart & object = part.getPartFromCache();
		//CQASSERT(object.obj);
		if(object.mdata.pSaveData->caps.specialAbilityOk)
		{
			USR_PACKET<USRSPABILITY> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.init(1);
			NETPACKET->Send(HOSTID, 0, &packet);
		}
		else if(object.mdata.pSaveData->caps.cloakOk)
		{
			USR_PACKET<USRCLOAK> packet;
			packet.objectID[0] = object.obj->GetPartID();
			packet.init(1);			
			NETPACKET->Send(HOSTID, 0, &packet);
		}
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::OrderBuildUnit (const MPartRef & part,char * buildName, bool queUnits )
{
	if (part.isValid())
	{
		MCachedPart & object = part.getPartFromCache();
		//CQASSERT(object.obj);
		U32 archID;
		if (buildName[0] && (archID = ARCHLIST->GetArchetypeDataID(buildName)) != 0)
		{
			USR_PACKET<USRBUILD> packet;

            if ( !queUnits )
			    packet.cmd = USRBUILD::ADDIFEMPTY;
            else 
			    packet.cmd = USRBUILD::ADD;

			packet.dwArchetypeID = archID;
			packet.objectID[0] = object.obj->GetPartID();
			packet.init(1);			
			NETPACKET->Send(HOSTID,0,&packet);
		}
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::OrderBuildPlatform (const MPartRef & part,MPartRef & target,char * buildName)
{
	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)
	//	CQASSERT(object.obj);

		MCachedPart & objTarget = target.getPartFromCache();
	//	CQASSERT(objTarget.obj);

		U32 archID;
		if (buildName[0] && (archID = ARCHLIST->GetArchetypeDataID(buildName)) != 0)
		{
			BASE_PLATFORM_DATA * data = (BASE_PLATFORM_DATA *)(ARCHLIST->GetArchetypeData(archID));
			if(data->type == PC_JUMPPLAT)
			{
				USR_PACKET<USRFABJUMP> packet;
				packet.dwArchetypeID = archID;
				packet.jumpgateID = objTarget.obj->GetPartID();
				packet.objectID[0] = object.obj->GetPartID();
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
			else if(data->slotsNeeded)
			{
				USR_PACKET<USRFAB> packet;
				OBJPTR<IPlanet> planet;
				objTarget.obj->QueryInterface(IPlanetID,planet);
				CQASSERT(planet);
				if(object.obj->GetSystemID() == objTarget.obj->GetSystemID())
				{
					Vector pos = object.obj->GetPosition();
					packet.slotID = planet->FindBestSlot(ARCHLIST->LoadArchetype(archID),&pos);
				}
				else
				{
					packet.slotID = planet->FindBestSlot(ARCHLIST->LoadArchetype(archID),NULL);
				}
				packet.dwArchetypeID = archID;
				packet.planetID = objTarget.obj->GetPartID();
				packet.objectID[0] = object.obj->GetPartID();
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
			else
			{
				USR_PACKET<USRFABPOS> packet;
				packet.dwArchetypeID = archID;
				packet.position.init(objTarget.obj->GetGridPosition(),objTarget.obj->GetSystemID());
				packet.objectID[0] = object.obj->GetPartID();
				packet.init(1);
				NETPACKET->Send(HOSTID,0,&packet);
			}
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderHarvest (const MPartRef & part, const MPartRef & target)
{
	if (part.isValid() && target.isValid())
	{
		MCachedPart object = part.getPartFromCache();		// not a reference since we need two parts at same time (jy)

		MCachedPart & objTarget = target.getPartFromCache();

		IBaseObject * closest = NULL;
		GRIDVECTOR vect = objTarget.obj->GetGridPosition();
		ObjMapIterator iter(objTarget.obj->GetSystemID(),vect,64*GRIDSIZE);
		SINGLE dist = 0;
		while(iter)
		{
			if(iter->obj->objClass == OC_NUGGET)
			{
// todo(aaj-5/6/2004): harvesters should see ALL nuggets in this grid area
//				if(iter->obj->IsVisibleToPlayer(object.obj->GetPlayerID()))
				{
					OBJPTR<INugget> nugget;
					iter->obj->QueryInterface(INuggetID,nugget);
					if(nugget->GetSupplies()> 0)
					{
						if(closest)
						{
							SINGLE newDist = vect-iter->obj->GetGridPosition();
							if(newDist < dist)
							{
								dist = newDist;
								closest = iter->obj;
							}
						}
						else
						{
							closest = iter->obj;
							dist = vect-iter->obj->GetGridPosition();
						}
					}
				}
			}
			++iter;
		}
		if(closest)
		{
			USR_PACKET<USRHARVEST> packet;

			packet.objectID[0] = object.obj->GetPartID();
			packet.targetID = closest->GetPartID();
			packet.bAutoSelected = false;
			packet.init(1);
			NETPACKET->Send(HOSTID,0,&packet);			
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::OrderCancel(const MPartRef & part)
{
	MCachedPart & object = part.getPartFromCache();
	if(object.obj)
	{
		USR_PACKET<USRSTOP> packet;
		packet.objectID[0] = object.obj->GetPartID();
		packet.init(1);
		NETPACKET->Send(HOSTID,0,&packet);
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
void MScript::SetStance(const MPartRef & part, UNIT_STANCE stance)
{
	if(part.isValid())
	{
		MCachedPart & objTarget = part.getPartFromCache();
		OBJPTR<IAttack> attack;
		objTarget.obj->QueryInterface(IAttackID,attack);
		if(attack)
		{
			attack->SetUnitStance(stance);
		}
	}
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Documentation goes here!
	 */
UNIT_STANCE MScript::GetStance(const MPartRef & part)
{
	if(part.isValid())
	{
		MCachedPart & objTarget = part.getPartFromCache();
		OBJPTR<IAttack> attack;
		objTarget.obj->QueryInterface(IAttackID,attack);
		if(attack)
		{
			return attack->GetUnitStance();
		}
	}
	return US_ATTACK;
}
//--------------------------------------------------------------------------//
//
void MScript::SetRallyPoint (const MPartRef & platform, const MPartRef & point)
{
	if(platform.isValid() && point.isValid())
	{
		MCachedPart & objPlatform = platform.getPartFromCache();
		if(objPlatform.obj->objClass == OC_PLATFORM)
		{
			MCachedPart & objPoint = point.getPartFromCache();
			NETGRIDVECTOR gridVectorPoint;
			gridVectorPoint.init( objPoint.obj->GetGridPosition(), objPoint.obj->GetSystemID() );
			
			VOLPTR(IPlatform) platform = objPlatform.obj;
			if( platform )
				platform->SetRallyPoint(gridVectorPoint);
		}
	}
}
//--------------------------------------------------------------------------//
//
void MScript::GiveCommandKit (const MPartRef & admiral, char * kitName)
{
	if(admiral.isValid())
	{
		MCachedPart & admiralObj = admiral.getPartFromCache();
		if(MGlobals::IsFlagship(admiralObj.mdata.pInitData->mObjClass))
		{
			VOLPTR(IAdmiral) ad = admiralObj.obj;
			ad->LearnCommandKit(kitName);
		}
	}
}
//--------------------------------------------------------------------------//
//
const char * MScript::GetFormationName(const MPartRef & admiral)
{
	if(admiral.isValid())
	{
		MCachedPart & admiralObj = admiral.getPartFromCache();
		if(MGlobals::IsFlagship(admiralObj.mdata.pInitData->mObjClass))
		{
			VOLPTR(IAdmiral) ad = admiralObj.obj;
			return ARCHLIST->GetArchName(ad->GetFormation());
		}
	}
	return NULL;
}
//--------------------------------------------------------------------------//
//
/* fog of war */
//--------------------------------------------------------------------------//
//
void MScript::ClearHardFog (const MPartRef & target, SINGLE range)
{
	MCachedPart & object = target.getPartFromCache();
	CQASSERT(object.obj);

	OBJLIST->CastVisibleArea(MGlobals::GetThisPlayer(), object.obj->GetSystemID(), object.obj->GetPosition(), object.obj->fieldFlags, range, 0);
}
//--------------------------------------------------------------------------//
//
void MScript::MakeAreaVisible (U32 playerID, const MPartRef & target, SINGLE range)
{
	MCachedPart & object = target.getPartFromCache();
	CQASSERT(object.obj);

	OBJLIST->CastVisibleArea(playerID, object.obj->GetSystemID(), object.obj->GetPosition(), object.obj->fieldFlags, range, 0);
}
//--------------------------------------------------------------------------//
//
void MScript::EnableFogOfWar (bool bEnable)
{
	DEFAULTS->GetDefaults()->fogMode = (bEnable) ? FOGOWAR_NORMAL : FOGOWAR_NONE;
}
//--------------------------------------------------------------------------//
//
SINGLE MScript::GetHardFogCleared(U32 playerID, U32 systemID)
{
	return FOGOFWAR->GetPercentageFogCleared(playerID,systemID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetSpectatorMode (bool bSetting)
{
	DEFAULTS->GetDefaults()->bSpectatorModeOn = bSetting;
	DEFAULTS->GetDefaults()->bVisibilityRulesOff = bSetting;
}
//--------------------------------------------------------------------------//
//
/* game Speed */
//--------------------------------------------------------------------------//
//
void MScript::PauseGame (bool bPause)
{
	CQFLAGS.bRTGamePaused = bPause;
}
//--------------------------------------------------------------------------//
//
void MScript::SetGameSpeed (S32 speed)
{
	DEFAULTS->GetDefaults()->gameSpeed = speed;
}
//--------------------------------------------------------------------------//
//
S32 MScript::GetGameSpeed (void)
{
	return 	DEFAULTS->GetDefaults()->gameSpeed;
}
//--------------------------------------------------------------------------//
//
/* audio video text */
//--------------------------------------------------------------------------//
//
void MScript::BriefingSubtitle(U32 soundHandle, U32 subtitle)
{
	if(subtitle)
	{
		SUBTITLE->NewBriefingSubtitle(GetCountedString(subtitle),soundHandle);
	}
}
//--------------------------------------------------------------------------//
//
bool MScript::IsAudioThere (const char * fileName)
{
	if (MSPEECHDIR && MSPEECHDIR->GetFileAttributes(fileName) != 0xFFFFFFFF)
		return true;
	return false;
}
//--------------------------------------------------------------------------//
U32 MScript::PlayAudio (const char * fileName, U32 subtitle)
{
	U32 handle = SOUNDMANAGER->PlayAudioMessage(fileName);
	if(handle && subtitle)
	{
		SUBTITLE->NewSubtitle(GetCountedString(subtitle),handle);
	}
	return handle;
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayAudio (const char * fileName,const MPartRef & speaker, U32 subtitle)
{
	MCachedPart & object = speaker.getPartFromCache();
	U32 handle = SOUNDMANAGER->PlayAudioMessage(fileName,object.obj);
	if(handle && subtitle)
	{
		SUBTITLE->NewSubtitle(GetCountedString(subtitle),handle);
	}
	return handle;
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayCommAudio (const char * fileName,const MPartRef & speaker, U32 subtitle)
{
	MCachedPart & object = speaker.getPartFromCache();
	U32 handle = SOUNDMANAGER->PlayCommMessage(fileName,NULL,object.obj);
	if(handle && subtitle)
	{
		SUBTITLE->NewSubtitle(GetCountedString(subtitle),handle);
	}
	return handle;
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y,U32 subtitle)
{
	U32 handle = SOUNDMANAGER->PlayAnimatedMessage(filename, animType, x, y);
	if(subtitle)
	{
		SUBTITLE->NewSubtitle(GetCountedString(subtitle),handle);
	}
	return handle;
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayAnimatedMessage (const char * filename, const char * animType, S32 x, S32 y, const MPartRef & speaker,U32 subtitle)
{
	MCachedPart & object = speaker.getPartFromCache();
	U32 handle = SOUNDMANAGER->PlayAnimatedMessage(filename, animType, x, y, object.obj);
	if(subtitle)
	{
		SUBTITLE->NewSubtitle(GetCountedString(subtitle),handle);
	}

	return handle;
}
//--------------------------------------------------------------------------//
//
void MScript::PlayFullScreenVideo (const char * filename)
{
	EnableMovieMode(true);
	CQFLAGS.bGameActive = false;
	MovieScreen(NULL,filename);
	CQFLAGS.bGameActive = true;
	EnableMovieMode(false);
}

//--------------------------------------------------------------------------//
//
void MScript::PlayMusic (const char * fileName)
{
	MUSICMANAGER->PlayMusic(fileName);
}
//--------------------------------------------------------------------------//
//
void MScript::GetMusicFileName (struct M_STRING & string)
{
	MUSICMANAGER->GetMusicFileName(string);
}
//--------------------------------------------------------------------------//
//
void MScript::FlushStreams()
{
	SOUNDMANAGER->FlushStreams();
}

//--------------------------------------------------------------------------//
//
BOOL32 MScript::IsStreamPlaying (U32 movie_handle)
{
	return SOUNDMANAGER->IsPlaying(movie_handle);
}

//--------------------------------------------------------------------------//
//
void MScript::EndStream (U32 movie_handle)
{
	if (movie_handle)
		SOUNDMANAGER->StopPlayback(movie_handle);
}

//--------------------------------------------------------------------------//
//
U32 MScript::PlayTeletype (U32 stringID, U32 left, U32 top, U32 right, U32 bottom, COLORREF color, U32 lifetime, U32 textTime, 
						   bool bMuted, bool bCentered)
{
	RECT rc;
	SetRect(&rc, left, top, right, bottom);

	return TELETYPE->CreateTeletypeDisplay(GetCountedString(stringID), rc, IDS_TELETYPE_FONT, color, lifetime, textTime, bMuted, bCentered);
}

//--------------------------------------------------------------------------//
//
BOOL32 MScript::IsTeletypePlaying (U32 teletypeID)
{
	return TELETYPE->IsPlaying(teletypeID);
}
//--------------------------------------------------------------------------//
//
BOOL32 MScript::IsCustomBriefingAnimationDone (U32 slotID)
{
	CQASSERT(BRIEFING && "Only call briefing functions when in briefing mode");

	if (BRIEFING->IsAnimationPlaying(slotID))
	{
		return TRUE;
	}

	return FALSE;
}
//--------------------------------------------------------------------------//
//
void::MScript::FlushTeletype (void)
{
	TELETYPE->Flush();
}

//--------------------------------------------------------------------------//
//
U32 MScript::GetScriptStringLength( U32 stringID )
{
    return U32( GetCountedString( stringID )[0] );
}

//--------------------------------------------------------------------------//
//
void MScript::DrawLine (U32 x1, U32 y1, U32 x2, U32 y2, COLORREF color, U32 lifetime, U32 travelTime)
{
	x1 = IDEAL2REALX(x1);
	x2 = IDEAL2REALX(x2);
	y1 = IDEAL2REALY(y1);
	y2 = IDEAL2REALY(y2);

	LINEMAN->CreateLineDisplay(x1, y1, x2, y2, color, lifetime, travelTime);
}
//--------------------------------------------------------------------------//
//
void MScript::DrawLine (U32 x, U32 y, const MPartRef & target, COLORREF color, U32 lifetime, U32 travelTime)
{
	x = IDEAL2REALX(x);
	y = IDEAL2REALY(y);

	MCachedPart & object = target.getPartFromCache();
	CQASSERT(object.obj);

	LINEMAN->CreateLineDisplay(x, y, object.obj, color, lifetime, travelTime);
}
//--------------------------------------------------------------------------//
//
void MScript::DrawLine (const MPartRef & object, const MPartRef & target, COLORREF color, U32 lifetime, U32 travelTime)
{
	MCachedPart & obj = object.getPartFromCache();
	CQASSERT(obj.obj);

	MCachedPart & targ = target.getPartFromCache();
	CQASSERT(targ.obj);

	LINEMAN->CreateLineDisplay(obj.obj, targ.obj, color, lifetime, travelTime);
}
//--------------------------------------------------------------------------//
//
void MScript::AlertSector(U32 systemID)
{
	SECTOR->ZoneOn(systemID);
}
//--------------------------------------------------------------------------//
//
	/*
	 *  Highlight an object in the systemMap and optionaly in the sector map
	 *  
	 */
void MScript::AlertObjectInSysMap(const MPartRef & object, bool bSectorToo )
{
	MCachedPart & obj = object.getPartFromCache();
	CQASSERT(obj.obj);
	if(bSectorToo)
		SECTOR->ZoneOn(obj.obj->GetSystemID());
	if(SECTOR->GetCurrentSystem() == obj.obj->GetSystemID())
		SYSMAP->ZoneOn(obj.obj->GetPosition());

}
//--------------------------------------------------------------------------//
//
void MScript::AlertMessage (const MPartRef & object, char * filename)
{
	MCachedPart & obj = object.getPartFromCache();
	CQASSERT(obj.obj);

	if (obj.obj)
	{
    	SOUNDMANAGER->PlayAudioMessage( "MSAlert.wav" );
		
		// load the wide string from the string table and convert it to a counted string
		const wchar_t * string = _localLoadStringW(IDS_TELETYPE_ALERT);
		wchar_t szAlert[256];
		wcscpy(&szAlert[1], string);
		szAlert[0] = wcslen(string);
        
		RECT rc;
		rc.left		= MISSION_OVER_TELETYPE_MIN_X_POS;
		rc.right	= MISSION_OVER_TELETYPE_MAX_X_POS;
		rc.top		= MISSION_OVER_TELETYPE_MIN_Y_POS;
		rc.bottom	= MISSION_OVER_TELETYPE_MAX_Y_POS;

		TELETYPE->CreateTeletypeDisplay( szAlert, rc, IDS_TELETYPE_FONT, RGB( 255,0,0 ), 6000, 1000, false, true );
		
		COMM_MISSION_ALLERT(obj.obj,obj.dwMissionID,filename);
	}
}
//--------------------------------------------------------------------------//
//
U32 MScript::StartAlertAnim (const MPartRef & object)
{
	MCachedPart & obj = object.getPartFromCache();
	CQASSERT(obj.obj);
	if(obj.obj)
	{
		return SYSMAP->CreateMissionAnim(obj.obj->GetSystemID(),obj.obj->GetGridPosition());
	}
	return 0xFFFFFFFF;
}
//--------------------------------------------------------------------------//
//
void MScript::StopAlertAnim (U32 animID)
{
	SYSMAP->StopMissionAnim(animID);
}
//--------------------------------------------------------------------------//
//
void MScript::FlustAlertAnims ()
{
	SYSMAP->FlushMissionAnims();
}
//--------------------------------------------------------------------------//
//
void MScript::StartFlashUI (U32 hotkeyID)
{
	INTERMAN->StartHotkeyFlash(hotkeyID);
}
//--------------------------------------------------------------------------//
//
void MScript::StopFlashUI (U32 hotkeyID)
{
	INTERMAN->StopHotkeyFlash(hotkeyID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetGas(U32 playerID, U32 amount)
{
	MGlobals::SetCurrentGas(playerID,amount);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetGas(U32 playerID)
{
	return MGlobals::GetCurrentGas(playerID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMaxGas (U32 playerID, U32 amount)
{
	MGlobals::SetMaxGas(playerID,amount);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMetal(U32 playerID, U32 amount)
{
	MGlobals::SetCurrentMetal(playerID,amount);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetMetal(U32 playerID)
{
	return MGlobals::GetCurrentGas(playerID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMaxMetal (U32 playerID, U32 amount)
{
	MGlobals::SetMaxMetal(playerID,amount);
}
//--------------------------------------------------------------------------//
//
void MScript::SetCrew(U32 playerID, U32 amount)
{
	MGlobals::SetCurrentCrew(playerID,amount);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetCrew(U32 playerID)
{
	return MGlobals::GetCurrentGas(playerID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMaxCrew (U32 playerID, U32 amount)
{
	MGlobals::SetMaxCrew(playerID,amount);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetTotalCommandPoints(U32 playerID)
{
	return MGlobals::GetCurrentTotalComPts(playerID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetTotalCommandPoints (U32 playerID, U32 newTotal)
{
	MGlobals::SetCurrentTotalComPts(playerID,newTotal);
}
//--------------------------------------------------------------------------//
//
S32 MScript::GetFreeCommandPoints(U32 playerID)
{
	return MGlobals::GetCurrentTotalComPts(playerID) - MGlobals::GetCurrentUsedComPts(playerID);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetUsedCommandPoints(U32 playerID)
{
	return MGlobals::GetCurrentUsedComPts(playerID);
}
//--------------------------------------------------------------------------//
//
U32 MScript::GetMaxCommandPoints (U32 playerID)
{
	return MGlobals::GetMaxControlPoints(playerID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMaxCommandPoitns (U32 playerID, U32 cpMax)
{
	MGlobals::SetMaxControlPoints(playerID,cpMax);
}
//--------------------------------------------------------------------------//
//
void MScript::OpenObjectiveMenu (U32 numObjectives, U32 * objectiveArray)
{
	EVENTSYS->Send(CQE_HOTKEY, (void*)IDH_MISSION_OBJ);

	for (U32 i = 0; i < numObjectives; i++)
	{
		AddToObjectiveList(objectiveArray[i]);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SetMissionName (U32 stringID)
{
	MGlobals::SetMissionName(stringID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMissionDescription (U32 stringID)
{
	MGlobals::SetMissionDescription(stringID);
}
//--------------------------------------------------------------------------//
//
void MScript::SetMissionID (U32 missionID)
{
	MGlobals::SetMissionID(missionID);
}
//--------------------------------------------------------------------------//
//
void MScript::AddToObjectiveList (U32 stringID, bool bSecondary)
{
	MGlobals::AddToObjectiveList(stringID, bSecondary);
}
//--------------------------------------------------------------------------//
//
void MScript::RemoveFromObjectiveList (U32 stringID)
{
	MGlobals::RemoveFromObjectiveList(stringID);
}
//--------------------------------------------------------------------------//
//
void MScript::MarkObjectiveCompleted (U32 stringID)
{
	MGlobals::MarkObjectiveCompleted(stringID);
}
//--------------------------------------------------------------------------//
//
void MScript::MarkObjectiveFailed (U32 stringID)
{
	MGlobals::MarkObjectiveFailed(stringID);
}

//--------------------------------------------------------------------------//
//
bool MScript::IsObjectiveCompleted (U32 stringID)
{
    return MGlobals::IsObjectiveCompleted( stringID );
}

//--------------------------------------------------------------------------//
//
bool MScript::IsObjectiveInList(U32 stringID)
{
    return MGlobals::IsObjectiveInList( stringID );
}

//--------------------------------------------------------------------------//
//
/*
 *	These functions should only be called when the briefing menu is up.  They make sure our briefing stuff get's played
 *	in the right spot on the briefing dialog.
 * 
 */
//--------------------------------------------------------------------------//
//
U32 MScript::PlayBriefingTalkingHead (const CQBRIEFINGITEM & item)
{
	CQASSERT(BRIEFING && "Do not make briefing calls unless the Briefing menu is up!");
	return BRIEFING->PlayAnimatedMessage(item);
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayBriefingAnimation (const CQBRIEFINGITEM & item)
{
	CQASSERT(BRIEFING && "Do not make briefing calls unless the Briefing menu is up!");
	return BRIEFING->PlayAnimation(item);
}
//--------------------------------------------------------------------------//
//
U32 MScript::PlayBriefingTeletype (U32 stringID, COLORREF color, U32 lifeTime, U32 textTime, bool bMuted)
{
	CQASSERT(BRIEFING && "Do not make briefing calls unless the Briefing menu is up!");
	CQASSERT(stringID != 0 && "stringID is zero");

	static CQBRIEFINGTELETYPE tele;
	tele.pString = GetCountedString(stringID);
	tele.color = color;
	tele.lifeTime = lifeTime;
	tele.textTime = textTime;
	tele.bMuted = bMuted;

	return BRIEFING->PlayTeletype(tele);	
}
//--------------------------------------------------------------------------//
//
void MScript::FreeBriefingSlot (S32 slotID)
{
	CQASSERT(BRIEFING && "Do not make briefing calls unless the Briefing menu is up!");

	if (slotID >= 0)
		BRIEFING->FreeSlot((U32)slotID);
	else
	{
		for (U32 x=0; x<4; x++)
			BRIEFING->FreeSlot(x);
	}
}
//--------------------------------------------------------------------------//
// AI CONTROLLING METHODS   (JeffP)
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
void MScript::EnableEnemyAI (U32 playerID, bool bOn, M_RACE race)
{
	MISSION->EnableEnemyAI(playerID, bOn, race);
}
//--------------------------------------------------------------------------//
//
void MScript::EnableEnemyAI (U32 playerID, bool bOn, const char * szPlayerAIType)
{
	MISSION->EnableEnemyAI(playerID, bOn, szPlayerAIType);
}
//--------------------------------------------------------------------------//
//
void MScript::SetEnemyCharacter (U32 playerID, U32 stringNameID)
{
	// playerID has to be between 1 and max players
	CQASSERT(playerID > 0 && playerID < MAX_PLAYERS);
	
	// get the string and pass it on to MGlobals
	wchar_t * pString = GetCountedString(stringNameID);

	if (pString && pString[0])
	{
		U32 nChars = pString[0];
		wchar_t szName[32];
		wcsncpy(szName, &pString[1], sizeof(szName) / sizeof(wchar_t));
		
		if (nChars < 32)
		{
			szName[nChars] = 0;
		}
		else
		{
			szName[31] = 0;
		}

		MGlobals::SetupComputerCharacter(playerID, szName);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::SetEnemyAIRules (U32 playerID, const AIPersonality & rules)
{
	MISSION->SetEnemyAIRules(playerID, rules);
}
//--------------------------------------------------------------------------//
//
void MScript::SetEnemyAITarget (U32 playerID, const MPartRef & object, U32 range, U32 systemID)
{
	if (object.isValid())
	{
		IBaseObject * obj;
		MCachedPart & cachPart = object.getPartFromCache(); 
			
		if (!cachPart.isValid()) return;
		obj = cachPart.obj;
	
		MISSION->SetEnemyAITarget(playerID, obj, range, systemID);
	}
}
//--------------------------------------------------------------------------//
//
void MScript::EnableAIForPart (const MPartRef & object, bool bEnable)
{
	if (object.isValid())
	{
		MCachedPart & cachPart = object.getPartFromCache(); 
		MPartNC part(cachPart.obj);
		part->bNoAIControl = !bEnable;
	}	
}
//--------------------------------------------------------------------------//
//
void MScript::LaunchOffensive (U32 playerID, UNIT_STANCE stance)
{
	MISSION->LaunchOffensive(playerID, stance);
}
//--------------------------------------------------------------------------//
//
void MScript::ClearResources (U32 systemID)
{
	IBaseObject * obj = OBJLIST->GetTargetList();     // nav hazard list
	OBJPTR<IPlanet> planet;

	while (obj)
	{
		if (obj->GetSystemID() == systemID && obj->QueryInterface(IPlanetID, planet)!=0)
		{
			planet->SetCrew(0);		
			planet->SetGas(0);		
			planet->SetMetal(0);		
		} 
		
		obj = obj->nextTarget;
	} 

	NUGGETMANAGER->RemoveNuggetsFromSystem(systemID);

}
//--------------------------------------------------------------------------//
//
void MScript::SetResourcesOnPlanet (MPartRef & planet, S32 gas, S32 metal, S32 crew)
{
	if (planet.isValid())
	{
		OBJPTR<IPlanet> iplanet;
		IBaseObject * obj;
		MCachedPart & cachPart = planet.getPartFromCache(); 
			
		if (!cachPart.isValid()) return;
		obj = cachPart.obj;

		if (obj->QueryInterface(IPlanetID, iplanet) != 0)
		{
			if (crew >= 0)
				iplanet->SetCrew(crew);		
			if (gas >= 0)
				iplanet->SetGas(gas);		
			if (metal >= 0)
				iplanet->SetMetal(metal);		
		}
	}

}
//--------------------------------------------------------------------------//
//
S32 MScript::GetCrewOnPlanet (const MPartRef & planet)
{
	if (planet.isValid())
	{
		OBJPTR<IPlanet> iplanet;
		IBaseObject * obj;
		MCachedPart & cachPart = planet.getPartFromCache(); 
			
		if (!cachPart.isValid()) return 0;
		obj = cachPart.obj;

		if (obj->QueryInterface(IPlanetID, iplanet) != 0)
		{
			return iplanet->GetCrew();	
		}
	}

    return 0;
}
//--------------------------------------------------------------------------//
//
S32 MScript::GetGasOnPlanet (const MPartRef & planet)
{
	if (planet.isValid())
	{
		OBJPTR<IPlanet> iplanet;
		IBaseObject * obj;
		MCachedPart & cachPart = planet.getPartFromCache(); 
			
		if (!cachPart.isValid()) return 0;
		obj = cachPart.obj;

		if (obj->QueryInterface(IPlanetID, iplanet) != 0)
		{
			return iplanet->GetGas();	
		}
	}

    return 0;
}
//--------------------------------------------------------------------------//
//
S32 MScript::GetMetalOnPlanet (const MPartRef & planet)
{
	if (planet.isValid())
	{
		OBJPTR<IPlanet> iplanet;
		IBaseObject * obj;
		MCachedPart & cachPart = planet.getPartFromCache(); 
			
		if (!cachPart.isValid()) return 0;
		obj = cachPart.obj;

		if (obj->QueryInterface(IPlanetID, iplanet) != 0)
		{
			return iplanet->GetMetal();	
		}
	}

    return 0;
}
//--------------------------------------------------------------------------//
//
void MScript::SetColorTable (COLORREF colors[9])
{
	memcpy(COLORTABLE, colors, sizeof(COLORTABLE));		// init the color table
}
//--------------------------------------------------------------------------//
//
void MScript::ResetColorTable (void)
{
	memcpy(COLORTABLE, DEFCOLORTABLE, sizeof(COLORTABLE));		// reset the color table
}


//----------------------------END MScript.cpp-------------------------------//
//--------------------------------------------------------------------------//
