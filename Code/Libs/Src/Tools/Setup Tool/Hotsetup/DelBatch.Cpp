/******************************************************************************
Module name: DelBatch.Cpp
Written by:  Jeffrey Richter and modified by EBUSetup group at Microsoft
Note:        This function works on both Windows 95 and Windows NT.
******************************************************************************/

#include <Windows.h>
#include "hotsetup.h"
#include "registry.h"
#include <tchar.h>

using namespace NGLOBALS;
//
//The name of the temporary batch file
//
#define DELUNSETUPBAT     __TEXT("DelUS.bat")

VOID WINAPI DeleteMyself(BOOL fAboutToReboot)
{
	TCHAR szPathname[MAX_PATH * 2];
	TCHAR szUnsetupPath[_MAX_PATH];

	HANDLE hFile;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	//
	//If we weren't bootstrapped and we're not uninstalling, we're done...
	//
	if (FALSE == GetBootstrapFlag() && FALSE == GetDeleteSetup())
	{
		if (GetResourceInst())
		{
			FreeLibrary(GetResourceInst());
		}

		return;
	}

	//
	//Create a batch file that continuously attempts to delete our executable
	//file.  When the executable no longer exists, remove its containing
	//subdirectory, and then delete the batch file too.
	//

	GetTempPath(sizeof(szPathname), szPathname);
	lstrcat(szPathname, DELUNSETUPBAT);
	
	//
	//Try to create batch file in TEMP directory...
	//Use CreateFile instead of EBUCreateFile because we don't need to ensure
	//that the CDROM is inserted at this point...
	//
	hFile = CreateFile(szPathname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
					   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

	//
	//If can't create in TEMP directory, try windows directory...
	//
	if (INVALID_HANDLE_VALUE == hFile)
	{
		TCHAR *pszPtr = szPathname;

		GetWindowsDirectory(szPathname, sizeof(szPathname));

		//
		//Point to last character in pathname...
		//
		while (*pszPtr)
		{
			pszPtr = CharNext(pszPtr);
		}
		pszPtr = CharPrev(szPathname, pszPtr);

		//
		//If windows dir doesn't already end in a '\', add one...
		//
		if (*pszPtr != '\\')
		{
			lstrcat(szPathname, "\\");
		}

		//
		//Append DelUs.Bat batch file name...
		//
		lstrcat(szPathname, DELUNSETUPBAT);

		//
		//Try to create in Windows directory...
		//Use CreateFile instead of EBUCreateFile because we don't need to ensure
		//that the CDROM is inserted at this point...
		//
		hFile = CreateFile(szPathname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
						   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	}

	if (INVALID_HANDLE_VALUE != hFile)
	{
		TCHAR szOemFile[3072];
		TCHAR szBatFile[3072];
		DWORD dwBytes;
		TCHAR *pszPathPtr;
		
		//
		//Get the full pathname of our executable file.
		//
		GetModuleFileName(NULL, szUnsetupPath, _MAX_PATH);
		
		//
		//Construct the lines for the batch file.
		//
		wsprintf(szBatFile,
				 __TEXT("@echo off\r\n")
				 __TEXT("@cd \\\r\n")
				 __TEXT(":Repeat1\r\n")
				 __TEXT("@del \"%s\"\r\n")
				 __TEXT("@if exist \"%s\" goto Repeat1\r\n"),
				 szUnsetupPath, 
				 szUnsetupPath);

		//
		//If we loaded the setup resource DLL...
		//
		if (GetResourceInst())
		{
			//
			//Get the name of the resource DLL
			//
			GetModuleFileName(GetResourceInst(), szUnsetupPath, sizeof(szUnsetupPath));

			//
			//Free the resource DLL and add batch lines to delete it...
			//
			FreeLibrary(GetResourceInst());
			SetResourceInst(NULL);

			//
			//Construct the lines for the batch file.
			//
			wsprintf(szBatFile + lstrlen(szBatFile),
					 __TEXT(":Repeat2\r\n")
					 __TEXT("@del \"%s\"\r\n")
					 __TEXT("@if exist \"%s\" goto Repeat2\r\n"),
					 szUnsetupPath, 
					 szUnsetupPath);
		}

		//
		//Only remove directory structure if uninstalling app...
		//
		if (TRUE == GetDeleteSetup())
		{
			//
			//Get path where uninstaller was run from...
			//
			lstrcpy(szUnsetupPath, GetSourcePath());

			//
			//Add lines to delete each ancestor level of the path too.  the rmdir command will
			//simply fail if the directory is not empty...
			//
			do
			{
				if(EBUStrlen(szUnsetupPath) == 2 && szUnsetupPath[1] == ':')
					break;
				wsprintf(szBatFile + lstrlen(szBatFile),
						 __TEXT("@rmdir \"%s\"\r\n"), szUnsetupPath);
					
				pszPathPtr = pszGetLast5C(szUnsetupPath);
				if (pszPathPtr)
				{
					*pszPathPtr = '\0';
				}

			} while (pszPathPtr);
		}
		
		//
		//Add a line to delete the batch file...
		//
		wsprintf(szBatFile + lstrlen(szBatFile), __TEXT("@del \"%s\""), szPathname);

		//
		//Ensure that we can delete international characters in paths and filenames also...
		//
		CharToOem(szBatFile, szOemFile);

		//
		//Write the batch file and close it...
		//
		WriteFile(hFile, szOemFile, lstrlen(szOemFile), &dwBytes, NULL);
		FlushFileBuffers(hFile);
		CloseHandle(hFile);

		//
		//Get ready to spawn the batch file we just created.
		//
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		
		//
		//We want its console window to be invisible to the user.
		//
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		//
		//Run command shell with /C (close) switch to remove stray WinoldApp processess...
		//
		if (GetEnvironmentVariable("COMSPEC", szBatFile, sizeof(szBatFile)))
		{
			lstrcat(szBatFile, " /C ");
			lstrcat(szBatFile, szPathname);

			lstrcpy(szPathname, szBatFile);
		}

		//
		//If we're about to reboot, then write the batch file to the RunOnce key so
		//that it is executed after reboot...
		//
		if (fAboutToReboot)
		{
			MyWriteUniversalRegType(HKEY_LOCAL_MACHINE,
									"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce\\EBUSetup",
									szPathname,
									REG_SZ);
		}
		else
		{
			//
			//Set current directory to a known, valid directory other than %APPPATH
			//
			GetWindowsDirectory(szUnsetupPath, sizeof(szUnsetupPath));

			//
			//Spawn the batch file with low-priority and suspended unless we're about to reboot,
			//then make it run at highest priority...
			//
			if (CreateProcess(NULL, 
							  szPathname, 
							  NULL, 
							  NULL, 
							  FALSE,
							  CREATE_SUSPENDED | DETACHED_PROCESS | IDLE_PRIORITY_CLASS, 
							  NULL, 
							  szUnsetupPath, &si, &pi))
			{
				//
				//Lower the batch file's priority even more...
				//
				SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);
				
				//
				//Raise our priority so that we terminate as quickly as possible...
				//
				SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
				SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
				
				//
				//Allow the batch file to run and clean-up our handles.
				//
				CloseHandle(pi.hProcess);
				ResumeThread(pi.hThread);

				//
				//We want to terminate right away now so that we can be deleted...
				//
				CloseHandle(pi.hThread);
			}
		}
	}

	//
	//Set current directory to a known, valid directory other than %APPPATH...
	//
	SetCurrentDirectory(szUnsetupPath);
}