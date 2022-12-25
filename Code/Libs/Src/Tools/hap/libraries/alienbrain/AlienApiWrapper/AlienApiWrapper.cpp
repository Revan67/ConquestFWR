// P4ApiWrapper.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "AlienApiWrapper.h"
#include <cassert>

CNxNSmartIntegrator* m_pSmartIntegrator = 0;
HWND m_hInfoWnd = 0;



BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			m_pSmartIntegrator = new CNxNSmartIntegrator();
			if (!m_pSmartIntegrator)
			{
				assert(0);
				return FALSE;
			}
			if (!m_pSmartIntegrator->InitInstance())
			{
				return FALSE;
			}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			m_pSmartIntegrator->ExitInstance();
			delete m_pSmartIntegrator;
			m_pSmartIntegrator = 0;
			break;
    }
    return TRUE;
}







static CNxNFile* GetFileNode(const CNxNString& sFile)
{
	CNxNString sNamespacePath;

	// map the local file path to a concerning namespace path, successfully mapped?
	if (m_pSmartIntegrator->MapManagedPath(sFile, sNamespacePath)) {

		// successfully mapped, get the concerning node
		CNxNFile* pFile = (CNxNFile*)m_pSmartIntegrator->GetManagedNode(sNamespacePath, CNxNFile::GetType());

		// valid file? => return it!
		if (pFile) {
			return pFile;
		}
	}

	// if we reach this pointer, it was not found or the name was invalid
	return NULL;
}

static CNxNDbNode* GetFolderOrProjectNode(const CNxNString& sPath)
{
	CNxNString sNamespacePath;

	// map the local file path to a concerning namespace path, successfully mapped?
	if (m_pSmartIntegrator->MapManagedPath(sPath, sNamespacePath)) {

		// get the dbnode, which can be a file, a folder or a project
		CNxNDbNode* pDbNode = (CNxNDbNode*)m_pSmartIntegrator->GetManagedNode(sNamespacePath, CNxNDbNode::GetType());

		// valid database node?
		if (pDbNode) {
			// valid dbnode, now check if it's a folder or a project
			if ((pDbNode->IsOfType(CNxNFolder::GetType())) || 
				(pDbNode->IsOfType(CNxNProject::GetType())) ) {
				// yep, return the pointer!
				return pDbNode;
			}
		}
	}

	// if we reach this pointer, it was not found or the name was invalid
	return NULL;
}



int SetNotifyHwnd(HWND hwndNotify)
{
	m_hInfoWnd = hwndNotify;
	if (m_pSmartIntegrator)
		m_pSmartIntegrator->SetParentWindow((long)hwndNotify);
	return 0;
}

int GetFileStatus(const char* cszFilename, SScFileStatus* pFs)
{
	assert(cszFilename && strlen(cszFilename));
	dprintf ("GetFileStatus: %s", cszFilename);
	if (m_pSmartIntegrator->FileIsManageable(cszFilename)) 
	{
		// might be a standard file, allow open, but don't continue in here.
		// file does NOT exist in database?
		if (m_pSmartIntegrator->FileExistsInDatabase(cszFilename)) 
		{
			// file checked out by another user?
			pFs->bControlled = true;
			pFs->bOpenByOther = m_pSmartIntegrator->FileCheckedOutByAnotherUser(cszFilename);
			pFs->bOpenForEdit = m_pSmartIntegrator->FileCheckedOutByUser(cszFilename);
			pFs->bNewerInSc = m_pSmartIntegrator->NewerFileOnServer(cszFilename);
		}
		else
		{
			dprintf ("FileExistsInDatabase returned false");
			char buff[1024];
			sprintf(buff, "FileExistsInDatabase returned false: %s\n", cszFilename);
			::SendMessage(m_hInfoWnd, NM_SENDERROR, (WPARAM)buff, 0);

			pFs->bControlled = false;
		}
	}
	else
	{
		dprintf ("FileIsManageable returned false");
		
		char buff[1024];
		sprintf(buff, "FileIsManageable returned false: %s\n", cszFilename);
		::SendMessage(m_hInfoWnd, NM_SENDERROR, (WPARAM)buff, 0);
		pFs->bControlled = false;
	}

	
	return 1;
}

int CheckFileOut(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	// file not checked out by the user and not checked out by another user?
	if (!m_pSmartIntegrator->FileCheckedOutByUser(cszFilename) &&
		!m_pSmartIntegrator->FileCheckedOutByAnotherUser(cszFilename)) {
		// yes! => check out makes sense

		CNxNFile* pFile = GetFileNode(cszFilename);

		// valid file?
		if (pFile) {
			// try to check out the file
			if (pFile->CheckOut()) 
			{
				return 1;
			}

			// if this happens the user might have pressed cancel, so
			// try to get the latest version of that file
			pFile->GetLatest(false);
		}
	}
	return 0;
}

int RevertFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	// file not checked out by the user
	if (m_pSmartIntegrator->FileCheckedOutByUser(cszFilename) ) 
	{
		CNxNFile* pFile = GetFileNode(cszFilename);

		// valid file?
		if (pFile) 
		{
			// uncheck out the file
			if (pFile->UndoCheckOut(true, false)) 
			{
				return 1;
			}
		}
	}
	return 0;

}

int CheckFileIn(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	// file not checked out by the user and checked out by another user?
	if (m_pSmartIntegrator->FileCheckedOutByUser(cszFilename) &&
		!m_pSmartIntegrator->FileCheckedOutByAnotherUser(cszFilename)) {
		// yes! => check in makes sense
	
		CNxNString sNamespacePath;

		// map the file name to a concerning namespace path
		if (m_pSmartIntegrator->MapManagedPath(cszFilename, sNamespacePath)) {
			CNxNFile* pFile = (CNxNFile*)m_pSmartIntegrator->GetManagedNode(sNamespacePath, CNxNFile::GetType());

			// valid file?
			if (pFile) 
			{
				// try to check in the file, but keep it checked out
				bool bKeepCheckedOut = false;
				if (pFile->CheckIn(true, _STR(""), _STR(""), bKeepCheckedOut)) 
				{
					return 1;
				}
			}
		}
	}

	return 0;
}

int AddFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	CNxNString sLocalFolderPath = m_pSmartIntegrator->GetPathOnly(cszFilename);

	// get the pointer to the node, below which we import the file
	// the node either be a project or a folder
	CNxNDbNode* pImportNode = GetFolderOrProjectNode(sLocalFolderPath);
	
	// valid import node?
	if (!pImportNode) 
	{
		return 0;
	}

	// import the current file below the found importnode.
	if (pImportNode->ImportDbNode(cszFilename)) 
	{
		return 1;
	}

	// import was either canceled or failed
	return 0;
}

int DiffFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	string Cmd = "diff";

	

	return 1;
}

int SyncFile(const char* cszFilename)
{
	assert(cszFilename && strlen(cszFilename));
	
	CNxNString sNamespacePath;
	CNxNFile* pFile = GetFileNode(cszFilename);

	// valid file?
	if (pFile) {
		// try to get the latest file
		if (pFile->GetLatest(false)) 
		{
			return 1;
		}
	}

	// either invalid pointer or the get latest failed.
	return 0;
}