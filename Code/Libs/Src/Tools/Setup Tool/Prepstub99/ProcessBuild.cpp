/**************************************************************************
* 
* ProcessBuild.cpp
* 
* Created 11/17/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/
#include "ProcessBuild.hpp"
#include "CPrepDoc.hpp"
#include "CPickList.hpp"
extern HINSTANCE g_hInst;
extern char	g_szLogFile[_MAX_PATH];
extern char g_szCurrentPath[_MAX_PATH];
extern CPickList	*g_PickList;
extern CPrepDoc	*g_PrepDoc;
extern HWND g_hwnd;
extern BOOL g_bCommandLine;
extern void EnableMenuOptions (DWORD dwState);
extern void SetViewMenuState (void);

bool ProcessBuildCommand (HWND hWnd, DWORD dwBuildType)
{
	string sSourceFiles;	
	char szCurrentDirectory[MAX_PATH];
	char szSetupExe[MAX_PATH];
	char szCabFile[MAX_PATH];
	char szTrialExe[MAX_PATH];
	char szTempFileName[MAX_PATH*2];
	char szTempPath[MAX_PATH];

	if (dwBuildType == BUILD_SETTINGS)
	{
		g_PrepDoc->SetListDirtyState (true);
		g_PrepDoc->StripAppSettings();
		g_PrepDoc->WriteAppSettings();
		g_PrepDoc->UpdateListView();  
		return true;
	}

	if (!g_bCommandLine)
	{
		g_PrepDoc->NukeListView();
		EbuYield();
	}

	// bump the Inject version number

	g_PrepDoc->IncrementScriptBuildNumber();
	g_PrepDoc->IncrementInjectBuildNumber();
	g_PrepDoc->SetPreventBuildNumberUpdate (true);
	g_PrepDoc->SetListDirtyState (true);
	UpdateWindowText ();

	if (!g_bCommandLine)
	{
		g_PrepDoc->CreateProgressDlg();
	}

	g_PrepDoc->GetProgressDlg()->SetWindowText ("Working...");

	// make sure the user specified the location of MakeCab before processing anything

	if (dwBuildType == BUILD_TRIAL)
	{
		if (g_PrepDoc->GetMakeCabExe() == NULL || strcmp (g_PrepDoc->GetMakeCabExe(), "")==0 || !DoesFileExist (g_PrepDoc->GetMakeCabExe()))
		{
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_NOMAKECABEXE);
			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;
		}
	}

	// Pre-copy the SetupEnu and Uninstall files into the source path so they get added to the file list.

	if (dwBuildType == BUILD_TRIAL)
	{
		char szSetupDll[MAX_PATH*2];

		// copy the setupenu dll

		lstrcpy (szSetupDll, g_PrepDoc->GetSourcePath());
		ConvertToValidPath (szSetupDll);
		string sSetupDll = GetFileNameFromPath (g_PrepDoc->GetSetupDll());
		lstrcat (szSetupDll, sSetupDll.c_str());

		// only copy setupenu.dll if src and dest are different files 
		if (lstrcmpi (g_PrepDoc->GetSetupDll(), szSetupDll)!=0)
		{
			if (!CopyFile (g_PrepDoc->GetSetupDll(), szSetupDll, FALSE))
			{
				g_PrepDoc->FillListView();
				Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CANNOTCOPYTODLL, g_PrepDoc->GetSetupDll(), szSetupDll);
				DeleteFile (szTempFileName);
				g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Failed.", "");
				return 0;
			}
		}

		// copy the clean copy of setup to the source dir, renaming to "uninstall.exe"

		lstrcpy (szSetupExe, g_PrepDoc->GetSourcePath());
		ConvertToValidPath (szSetupExe);
		lstrcat (szSetupExe, g_PrepDoc->GetUninstallExe());

		/*if (DoesFileExist (szSetupExe))
		{
			if (Alert( g_hAppWnd, MB_YESNO | MB_ICONEXCLAMATION, STR_UNINSTALLEXEEXISTS, szSetupExe)== IDNO)
			{
				g_PrepDoc->FillListView();
				DeleteFile (szTempFileName);
				g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Failed.", "");
				return 0;
			}
		}*/

		if (!CopyFile (g_PrepDoc->GetSetupExe(), szSetupExe, FALSE))
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CANNOTCOPYTOUNINSTALL, g_PrepDoc->GetSetupExe(), szSetupExe);
			DeleteFile (szTempFileName);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Failed.", "");
			return 0;
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Update File List (includes saving history, etc)
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	
	// - always update the file list on a trial version build.
	// - never update the file list if we are an ISO style build and the user is now SaveAs'ing or Injecting. (the file list CANNOT change in this case, because we are not re-replicating)
	if ((dwBuildType == BUILD_TRIAL) || (dwBuildType == BUILD_UPDATE_AND_INJECT) || (g_PrepDoc->IsUpdateFileList() && !(g_PrepDoc->GetLastBuildType() == PBT_REPLICATE && (dwBuildType == BUILD_INJECT || dwBuildType == BUILD_SAVEAS)) ))
	{
		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Processing history information...");
		g_PrepDoc->RecreateHist();

		if (!g_PrepDoc->LoadHistory())
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_LOADHISTORY_FAILED);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;
		}

		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Processing file and string lists...");

		if (!g_PrepDoc->RemoveDynamicFileList())
		{	
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_NODYNSECTION, g_szAppTitle);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;
		}

		if (!g_PrepDoc->RemoveDynamicStringList())
		{
			if (!g_PrepDoc->CreateDynamicStringSection())
			{
				g_PrepDoc->FillListView();
				Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CREATEDYNSTRINGSECTIONFAILED);
				g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
				return false;
			}
		}

		if (!g_PrepDoc->ClearFileList())
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CLEARFILELIST, g_szAppTitle);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;			
		}

		if (!g_PrepDoc->ClearDynamicStringList ())
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CLEARDYNSTRINGLISTFAILED, g_szAppTitle);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;
		}

		if (!g_PrepDoc->ReadFileList())
		{
			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("File List Update Failed.", "");
			return false;
		}

		if (!g_PrepDoc->ProcessFilelistForMultiDisk())
		{
			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Multi-Disk Processing failed.", "");
			return false;
		}

		// 
		// For trial version builds, sort the dynamic file list area by cab folder #.
		//
		if (dwBuildType == BUILD_TRIAL)
		{
			g_PrepDoc->SortDynamicAreaByCabFolder();
		}

		//
		// The script changed, so update the max Disk #
		//

		g_PrepDoc->FindMaxMultiDisk();

		if (!g_PrepDoc->UpdateStringList())
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_UPDATESTRINGLISTFAILED, g_szAppTitle);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
			return false;
		}
								
		g_PrepDoc->GetSetupDoc()->ReNumberCommands();

		g_PrepDoc->StripAppSettings();
		g_PrepDoc->WriteAppSettings();
		g_PrepDoc->SetListDirtyState (true);

		// if the user is just updating the file list, we now have a
		// non ISO compliant type of build. Reset the last build type
		// so future commands get processed correctly
		if (dwBuildType==BUILD_UPDATEFILELIST || dwBuildType == BUILD_UPDATE_AND_INJECT)
		{
			g_PrepDoc->SetLastBuildType (PBT_UPDATE);
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Load static strings from DLL
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	if ( (dwBuildType != BUILD_UPDATEFILELIST) && g_PrepDoc->IsLoadDllStrings() )
	{
		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Loading Static Strings from DLL");
		if (!g_PrepDoc->LoadStaticStringsFromResourceDll())
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_LOADSTATICSTRINGSFAILED);
			g_PrepDoc->GetProgressDlg()->SetCurrentTask("Load Static Strings from Resource DLL failed.");
			g_PrepDoc->GetProgressDlg()->SetCurrentTask("");
			g_PrepDoc->GetProgressDlg()->SetCurrentTask("Write Failed.");
			g_PrepDoc->GetProgressDlg()->SetCurrentTask("");
			g_PrepDoc->GetProgressDlg()->SetCurrentTask("Complete.");
			g_PrepDoc->GetProgressDlg()->SetStatusText("");
			g_PrepDoc->GetProgressDlg()->ClearCancelRequest();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait();
			return false;
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Replicate the file tree
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	if (dwBuildType == BUILD_REPLICATE && g_PrepDoc->IsReplicateFileTree())
	{
		GetCurrentDirectory (MAX_PATH, szCurrentDirectory);

		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Preparing to replicate...");
		if (!g_PrepDoc->CreateDirs(hWnd))
		{
			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Build Cancelled.");
			g_PrepDoc->GetProgressDlg()->SetStatusText("");
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait();

			SetCurrentDirectory (szCurrentDirectory);
			return 0;
		}

		if (!IsDirectoryWritable (g_PrepDoc->GetDropPath()))
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_DROPDIRNOTWRITABLE, g_PrepDoc->GetDropPath());
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Cancelled.", "");
			SetCurrentDirectory (szCurrentDirectory);
			return 0;
		}	

		if (!g_PrepDoc->ReplicateFileList(hWnd))
		{	
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_REPLICATEFAILED);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Cancelled.", "");
			SetCurrentDirectory (szCurrentDirectory);
			return 0;
		}			
		
		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Replicating user files...");
		g_PrepDoc->ExecuteReplicateActions();

		// replicate will change the current directory, so set it back here
		// to what the user had before the replicate.
		SetCurrentDirectory (szCurrentDirectory);
		Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_ADDFILESMESSAGE);
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Choose the file to save the binary output
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	if (dwBuildType == BUILD_REPLICATE || dwBuildType == BUILD_INJECT || dwBuildType == BUILD_TRIAL || dwBuildType == BUILD_UPDATE_AND_INJECT)
	{
		// Create a temp file to save the binary
		GetTempPath (sizeof (szTempPath), szTempPath);
		GetTempFileName (szTempPath, "PS9", 0, szTempFileName);
	}
	else
	if (dwBuildType == BUILD_SAVEAS)
	{
		// Get the name the user selected in the SaveAs dialog
		if (g_bCommandLine)
		{
			if (g_PrepDoc->GetBinarySaveAsName()==NULL || strcmpi (g_PrepDoc->GetBinarySaveAsName(), "")==0)
			{
				Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CL_MISSING_BINARY, g_szAppTitle);
				g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Failed.", "");
				return false;
			}
			else
			{
				lstrcpy (szTempFileName, g_PrepDoc->GetBinarySaveAsName());
			}
		}
		else
		{
			g_PrepDoc->GetSaveAsFileName(szTempFileName);
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Choose the source path to get file version information
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	if (dwBuildType == BUILD_REPLICATE)
	{
		sSourceFiles = g_PrepDoc->GetDropPath();
	}
	else
	if (dwBuildType == BUILD_SAVEAS || dwBuildType == BUILD_INJECT || dwBuildType == BUILD_TRIAL || dwBuildType == BUILD_UPDATE_AND_INJECT)
	{
		if (g_PrepDoc->GetLastBuildType() == PBT_UNKNOWN)
		{
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_UNKNOWNBUILDTYPE, g_szAppTitle, g_szAppTitle);

			// Prepstub does not know what type of build list is currently
			// loaded. In this case we just default to a LFN type of build
			// and warn the user they might need to update or replicate to
			// make inject (or saveAs) work.

			sSourceFiles = g_PrepDoc->GetSourcePath();
		}
		else
		{
			if (g_PrepDoc->GetLastBuildType()==PBT_REPLICATE)
			{
				sSourceFiles = g_PrepDoc->GetDropPath();
			}
			else
			{
				sSourceFiles = g_PrepDoc->GetSourcePath();
			}
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	//		Write the binary blob (and optionally inject)
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	//
	// If this is a Trial Version build, we want to make two inject passes so injected file sizes
	// are correct in the uninstall.exe and trialname.exe.    Uninstall.exe and SetupEnd.dll are 
	// not initially sized correctly because no inject has taken place. After pass 1, they are
	// injected. When sizing info is collected on the 2nd pass, it is correct. This info is then
	// re-injected into Uninstall.exe (and Trial.exe indirectly) 
	//

	int nPasses = (dwBuildType == BUILD_TRIAL)?2:1;
	
	for (int nPassNumber=0;nPassNumber < nPasses; nPassNumber++)
	{
		if (dwBuildType != BUILD_UPDATEFILELIST)
		{
			g_PrepDoc->SetListDirtyState (true);

			ConvertToValidPath (sSourceFiles);

			g_PrepDoc->GetSetupDoc()->ReNumberCommands();
			g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Creating binary blob...");

			if (!g_PrepDoc->WriteSetupScript (szTempFileName, sSourceFiles.c_str()))
			{
				Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_BINWRITEFAILED, g_szAppTitle);

				if (dwBuildType != BUILD_SAVEAS)
				{
					DeleteFile (szTempFileName);
				}
				g_PrepDoc->FillListView();
				g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Build Cancelled.", "");
				return 0;
			}

			// (optionally) inject binary blob and strings

			if (dwBuildType == BUILD_REPLICATE || dwBuildType == BUILD_INJECT || dwBuildType == BUILD_TRIAL || dwBuildType == BUILD_UPDATE_AND_INJECT)
			{
				if (g_PrepDoc->IsInjectBinaryBlob())
				{
					g_PrepDoc->GetProgressDlg()->SetStatusText ("Injecting binary blob.");


					// determine where we are injecting.  If it is a trial, then we actually inject the
					// uninstall.exe file, which is then later renamed to the trial setup exe name after
					// the cab is created and injected.  Otherwise, just inject the source setup.exe

					if (dwBuildType == BUILD_TRIAL)
					{
						lstrcpy (szSetupExe, g_PrepDoc->GetSourcePath());
						ConvertToValidPath (szSetupExe);
						lstrcat (szSetupExe, g_PrepDoc->GetUninstallExe());
					}
					else  
					{
						lstrcpy (szSetupExe, g_PrepDoc->GetSetupExe());
					}

					if (!g_PrepDoc->InjectResource (szSetupExe, szTempFileName))
					{
						g_PrepDoc->FillListView();
						Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_SETUPUPDATEFAILED, g_szAppTitle);
						DeleteFile (szTempFileName);
						g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
						return 0;
					}
					else
					{
						g_PrepDoc->InjectVersionInfoInStringTable (szSetupExe);
					}
					
				}

				if (g_PrepDoc->IsInjectDynamicStrings() || g_PrepDoc->IsInjectStaticStrings())
				{
					g_PrepDoc->GetProgressDlg()->SetStatusText ("Injecting string table.");
					if (!g_PrepDoc->InjectStringTableInResource (g_PrepDoc->GetSetupDll()))
					{
						Alert (g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_SETUPUPDATEFAILED, g_szAppTitle);		
					}
					else
					{
						g_PrepDoc->InjectVersionInfoInStringTable (g_PrepDoc->GetSetupDll());
					}
				}
			}

			if (dwBuildType != BUILD_SAVEAS)
			{
				DeleteFile (szTempFileName);
			}
		}
	}

	if (dwBuildType == BUILD_TRIAL)
	{
		char szDirectiveTemp[MAX_PATH];
		GetTempPath (sizeof (szTempPath), szTempPath);
		GetTempFileName (szTempPath, "PMK", 0, szDirectiveTemp);

		char szLogTemp[MAX_PATH];
		GetTempPath (sizeof (szTempPath), szTempPath);
		GetTempFileName (szTempPath, "PML", 0, szLogTemp);


		g_PrepDoc->GetProgressDlg()->SetCurrentTask ("Creating Cab File... Please Wait.");
		g_PrepDoc->GetProgressDlg()->SetStatusText ("Generating Diamond Directive File.");
		
		if (!g_PrepDoc->WriteDiamondDirectiveFile(szDirectiveTemp))
		{
			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
			return 0;
		}

		char szCommandLine[MAX_PATH];
		wsprintf (szCommandLine, "/c %s /f %s > %s", g_PrepDoc->GetMakeCabExe(), szDirectiveTemp, szLogTemp);


		g_PrepDoc->GetProgressDlg()->SetStatusText ("Creating the Cab File.");
		if (0!=ShellExecuteAndWait ("cmd.exe", szCommandLine, SW_HIDE))
		{
			g_PrepDoc->GetProgressDlg()->SetCurrentTask ("MakeCab failed.");
			if (Alert( g_hAppWnd, MB_YESNO | MB_ICONEXCLAMATION, STR_MAKECABFAILED, szSetupExe)== IDYES)
			{
				ShellExecute ("notepad.exe", szLogTemp, SW_SHOW);
			}

			g_PrepDoc->FillListView();
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
			return 0;
		}
		else
		{
			g_PrepDoc->GetProgressDlg()->SetCurrentTask ("MakeCab success.");
			DeleteFile (szLogTemp);
		}

		g_PrepDoc->GetProgressDlg()->SetStatusText ("");
		DeleteFile (szDirectiveTemp);

		lstrcpy (szSetupExe, g_PrepDoc->GetSourcePath());
		ConvertToValidPath (szSetupExe);
		lstrcat (szSetupExe, g_PrepDoc->GetUninstallExe());

		lstrcpy (szTrialExe, g_PrepDoc->GetDropPath());
		ConvertToValidPath (szTrialExe);
		lstrcat (szTrialExe, g_PrepDoc->GetTrialExe());

		lstrcpy (szCabFile, g_PrepDoc->GetDropPath());
		ConvertToValidPath (szCabFile);
		lstrcat (szCabFile, g_PrepDoc->GetCabFileName());


		if (!DoesFileExist (szCabFile))
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CABNOTFOUND, szCabFile);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
			return 0;
		}

		// do exist and error checking here...

		if (!CopyFile (szSetupExe, szTrialExe, FALSE))
		{
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CANTCOPYTRIALEXE, szTrialExe);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
			return 0;
		}
		
		// inject the cab file, completing the final TrialVersion executable.
		if (!g_PrepDoc->InjectResourceFromFile (szTrialExe,"CABFILE", "IDR_CABFILE", szCabFile))
		{
			DeleteFile (szCabFile);
			g_PrepDoc->FillListView();
			Alert( g_PrepDoc->GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CANTINJECTCAB, szTrialExe);
			g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Update Failed.", "");
			return 0;
		}

		DeleteFile (szCabFile);
	}

	g_PrepDoc->FillListView();
	g_PrepDoc->GetProgressDlg()->SetCompleteAndWait("Complete.", "");

	return true;
}


DWORD ShellExecuteAndWait (char *szProgram, char *szParams, int nShow)
{
	DWORD dwExitCode;

	SHELLEXECUTEINFO shlx;

	memset(&shlx,0,sizeof(SHELLEXECUTEINFO));

	shlx.cbSize = sizeof(SHELLEXECUTEINFO);
	shlx.fMask = SEE_MASK_NOCLOSEPROCESS; 
	shlx.lpFile= szProgram;
	shlx.lpParameters = szParams;
	shlx.nShow=nShow;

	ShellExecuteEx(&shlx);

	// .hInstApp returns a value of < 32 if an error is encountered
	if ((int)shlx.hInstApp < 32)
	{
		return 1;
	}

	//
	// wait for the process to terminate. While waiting, keep the
	// message loop pumping. Keep in mind that this function could therefore
	// be re-entered.
	//

	while (WaitForSingleObject (shlx.hProcess, 10))
	{
		EbuYield();
	}

	GetExitCodeProcess (shlx.hProcess, &dwExitCode);
  
	CloseHandle (shlx.hProcess);

	// return the exit code from the process
	return dwExitCode;
}


bool ShellExecute (char *szProgram, char *szParams, int nShow)
{
	SHELLEXECUTEINFO shlx;

	memset(&shlx,0,sizeof(SHELLEXECUTEINFO));

	shlx.cbSize = sizeof(SHELLEXECUTEINFO);
	shlx.fMask = SEE_MASK_NOCLOSEPROCESS; 
	shlx.lpFile= szProgram;
	shlx.lpParameters = szParams;
	shlx.nShow=nShow;

	ShellExecuteEx(&shlx);

	// .hInstApp returns a value of < 32 if an error is encountered
	if ((int)shlx.hInstApp < 32)
	{
		return 1;
	}

	return 0;
}


bool LoadScript(char *szScriptName)
{
	HCURSOR hCursorOld;

	if (!DoesFileExist (szScriptName))
	{
		Alert( g_hwnd, MB_ICONSTOP | MB_OK, STR_FILENOTFOUND, (LPCSTR)szScriptName );
		if (!g_bCommandLine)
		{
			g_PickList->RemoveItem (szScriptName);
			g_PickList->UpdateMenu (g_hAppWnd,  BASE_PICKLIST_ITEM, IDM_RECENTFILES);
			g_PickList->Write();
			EnableMenuOptions (MO_DOC_CLOSED);
			UpdateWindowText();
		}
		return false;
	}

	g_PrepDoc->SetScriptName (szScriptName);
		
	if (!g_bCommandLine)
	{
		// update the pick list
		g_PickList->AddItem (szScriptName);
		g_PickList->UpdateMenu (g_hAppWnd,  BASE_PICKLIST_ITEM, IDM_RECENTFILES);
		g_PickList->Write ();
	
		// put the UI into a loading state.
		g_PrepDoc->SetStatusBarText (0, "Loading...");
		UpdateWindowText();
		EnableWindow (g_hAppWnd, FALSE);
		g_hAppCursor = LoadCursor (NULL, IDC_WAIT);
		hCursorOld = SetCursor (g_hAppCursor);
	}

	if (!g_PrepDoc->LoadSetupScript (szScriptName))
	{
		if (!g_bCommandLine)
		{
			EbuYield();
			Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADSCRIPT, g_szAppTitle, szScriptName);
			g_hAppCursor = NULL;
			SetCursor (hCursorOld);
			EnableWindow (g_hAppWnd, TRUE);
			g_PrepDoc->SetStatusBarText (0, "Ready");
			g_PrepDoc->CloseDoc(CD_HANDLE_UI); 
			UpdateWindowText();
		}
		else
		{
			Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADSCRIPT, g_szAppTitle, szScriptName);
			g_PrepDoc->CloseDoc(CD_IGNORE_UI); 
		}
		return false;
	}

	g_PrepDoc->SetScriptName (szScriptName);
	g_PrepDoc->SetDocLoadedState (true);


	if (!g_PrepDoc->LoadStringList())
	{
		if (!g_bCommandLine)
		{
			EbuYield();
		
			g_hAppCursor = NULL;
			SetCursor (hCursorOld);
			EnableWindow (g_hAppWnd, TRUE);
			g_PrepDoc->SetStatusBarText (0, "Ready");
			g_PrepDoc->CloseDoc(CD_HANDLE_UI); 
		}
		else
		{
			g_PrepDoc->CloseDoc(CD_IGNORE_UI); 
		}

		Alert (g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_LOADSTRINGLIST_FAILED,  g_szAppTitle);
		return false;
	}

	if (!g_PrepDoc->LoadRules ())
	{
		if (!g_bCommandLine)
		{
			EbuYield();
			g_hAppCursor = NULL;
			SetCursor (hCursorOld);
			EnableWindow (g_hAppWnd, TRUE);
			g_PrepDoc->SetStatusBarText (0, "Ready");
			g_PrepDoc->CloseDoc(CD_HANDLE_UI); 
		}
		else
		{
			g_PrepDoc->CloseDoc(CD_IGNORE_UI); 
		}

		Alert (g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_LOADRULES_FAILED,  g_szAppTitle);
		return false;
	}

	if (!g_PrepDoc->LoadHistory ())
	{
		if (!g_bCommandLine)
		{
			EbuYield();
			g_hAppCursor = NULL;
			SetCursor (hCursorOld);
			EnableWindow (g_hAppWnd, TRUE);
			g_PrepDoc->SetStatusBarText (0, "Ready");
			g_PrepDoc->CloseDoc(CD_HANDLE_UI); 
		}
		else
		{
			g_PrepDoc->CloseDoc(CD_IGNORE_UI); 
		}

		Alert (g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_LOADHISTORY_FAILED,  g_szAppTitle);
		return false;
	}

	g_PrepDoc->ReadAppSettings ();

	if (!g_bCommandLine)
	{
		g_PrepDoc->SetStatusBarText (0, "Wait...");
		EbuYield();
		g_PrepDoc->FillListView();
		HMENU hMenu = GetMenu (g_hAppWnd);						
		EnableMenuOptions (MO_DOC_OPEN);
		g_hAppCursor = NULL;
		SetCursor (hCursorOld);
		EnableWindow (g_hAppWnd, TRUE);
		EbuYield();
		SetViewMenuState ();
		g_PrepDoc->SetStatusBarText (0, "Ready");
	}

	return true;
}



int ProcessCommandLine (char *szCommandLine)
{
	char szBuildNumber[15];
	char szBuildMessage[30];
	g_bCommandLine = TRUE;

	if (DoesFileExist (g_szLogFile))
	{
		DeleteFile (g_szLogFile);
	}

	GetCurrentDirectory (MAX_PATH, g_szCurrentPath);
	
	lstrcpy (g_szLogFile, g_szCurrentPath);
	MakePathCompliant (g_szLogFile);
	lstrcat (g_szLogFile, "PrepStub99.out");

	WORD wOS;

	g_bCommandLine = TRUE;

	GetCurrentDirectory (MAX_PATH, g_szCurrentPath);

	// set the global window/instance handle for older core prepstub code
	g_hwnd = g_hAppWnd;
	g_hInst = g_hAppInst;

	LoadString( GetModuleHandle (NULL), STR_APPTITLE, g_szAppTitle, sizeof(g_szAppTitle) );

	// Make sure we are running on NT, if not let the app run, but warn user
	// they will not be able to use all features.

	LoadString (g_hAppInst, STR_BUILD_NUMBER, szBuildNumber, 15);
	
	wsprintf (szBuildMessage, "%s - [%s]", g_szAppTitle, szBuildNumber);
	WriteLogFileMessage (szBuildMessage);

	// write the intro header.

	WriteLogFileMessage (STR_CL_HEADER);

	// warn if the user is on something other than NT
	wOS = GetCurrentOperatingSystem();

	if (wOS != OS_NT40 && wOS != OS_NT50)
	{
		Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_WINNT_REQUIRED);
	}
	
	// create our main container.
	g_PrepDoc = new CPrepDoc();

	if (!g_PrepDoc->Create(GetDesktopWindow(), GetModuleHandle(NULL), g_szCurrentPath)) 
	{
		return 1;
	}

	g_PrepDoc->SetUseLongFileNamesOnly (true);

	// default to injecting into english lang resource
	g_PrepDoc->SetLangID (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));


	if (0 == g_PrepDoc->ParseCommandLine (szCommandLine))
	{
		g_PrepDoc->CloseDoc(true);
		return 0;
	}

	g_PrepDoc->DumpBuildParameters ();

	// redirect all of the progress dialogs to the log file.
	g_PrepDoc->CreateProgressDlg (g_szLogFile);

	
	if (g_PrepDoc->GetScriptName()==NULL || !LoadScript (g_PrepDoc->GetScriptName()))
	{
		WriteLogFileMessage ("ERROR: Could not load script file.");
		return 1;
	}
	else
	{
		WriteLogFileMessage ("Script file loaded.");
	}

	g_PrepDoc->ReadAppSettings ();
	g_PrepDoc->SetDocLoadedState (true);


	if (!ProcessBuildCommand (NULL, g_PrepDoc->GetPrepStubBuildType()))
	{
		WriteLogFileMessage ("Build Process Failed.\n");

	}

	g_PrepDoc->CloseDoc(true);

	WriteLogFileMessage ("Complete.");

	return 0;
}