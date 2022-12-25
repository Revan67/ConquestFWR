// P4ApiWrapper.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "P4ApiWrapper.h"
#include "P4ClientUser.h"
#include <cassert>

ClientApi* pP4ClientApi;
P4ClientUserWrap* pP4ClientUser;
Error e;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			pP4ClientUser = new P4ClientUserWrap();
			pP4ClientApi = new ClientApi();
			
			pP4ClientApi->Init(&e);
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			if (pP4ClientApi)
			{
				pP4ClientApi->Final( &e );
				e.Abort();
				delete pP4ClientApi;
			}
			if (pP4ClientUser)
				delete pP4ClientUser;
			break;
    }
    return TRUE;
}


P4APIWRAPPER_API int SetNotifyHwnd(HWND hwndNotify)
{
	if (pP4ClientUser)
	{
		pP4ClientUser->m_hwndNotify = hwndNotify;
		return 1;
	}
	return 0;
}

P4APIWRAPPER_API int GetFileStatus(const char* cszFilename, SScFileStatus* pFs)
{
	assert(cszFilename && strlen(cszFilename));
	
	string Cmd = "fstat";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	memset(&(pP4ClientUser->m_fs), 0, sizeof(SScFileStatus));

	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	memcpy(pFs, &(pP4ClientUser->m_fs), sizeof(SScFileStatus));
	return 1;
}

int CheckFileOut(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	string Cmd = "edit";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}

int RevertFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	string Cmd = "revert";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}

int CheckFileIn(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	string Cmd = "submit";
	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}

int AddFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	string Cmd = "add";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}

int DiffFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	string Cmd = "diff";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}

int SyncFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	string Cmd = "sync";

	char* argv[2];
	argv[0] = (char*)cszFilename;
	argv[1] = 0;

	pP4ClientApi->SetArgv(1, argv);
	pP4ClientApi->Run(Cmd.c_str(), pP4ClientUser);

	return 1;
}