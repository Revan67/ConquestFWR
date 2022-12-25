/**************************************************************************
* 
* CPrepDoc.cpp
* 
* Created 3/24/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/
#include "CPrepDoc.hpp"
#include "windowsx.h"
#include <process.h>
#include "conio.h"
#include "hotsetuprc.h"
extern BOOL g_bCommandLine;

CPrepDoc::CPrepDoc (void)
{
	m_pszSetupExe		= NULL;
	m_pszSetupDll		= NULL;
	m_pszDropPath		= NULL;
	m_pszProgramPath	= NULL;
	m_pszSourcePath		= NULL;
	m_pszScriptName		= NULL;
	m_pszReparentPath	= NULL;
	m_pszAppName		= NULL;
	m_pszCabFileName	= NULL;
	m_pszUninstallExe	= NULL;
	m_pszTrialExe		= NULL;
	m_pszMakeCabExe		= NULL;
	m_pszBinarySaveAsName=NULL;
	m_pSetupDoc			= NULL;
	m_pFileList			= NULL;
	m_pFileHist			= NULL;
	m_pFileRule			= NULL;
	m_pStringList		= NULL;
	m_hWnd				= NULL;
	m_bListDirty		= false;
	m_bDocLoaded		= false;
	m_bShowComments		= true;
	m_bShowRegular		= true;
	m_bShowInternal		= true;
	m_bShowColor		= true;
	m_bShowProperty		= false;
	m_nStringID			= BASE_DYNAMIC_STRING_ID;
	m_dwLangID			= 0;
	m_bEnableLocalization = false;
	m_dwPrepStubBuildType = PBT_UPDATE;

	m_bReplicateFileTree	= true;
	m_bInjectStaticStrings	= false;
	m_bInjectDynamicStrings = false;
	m_bInjectBinaryBlob		= true;

	m_bIgnoreStrings		= true;
	m_bLoadScriptStrings	= false;
	m_bLoadDllStrings		= false;
	m_bUpdateStaticStrings	= false;
	m_bUpdateFileList		= true;
	m_bPreventScriptBuildNumberUpdate = false;

	m_dwLastBuildType		= PBT_UNKNOWN;
	m_bReparent = true;
	m_nMaxDisk = DISK_NOT_SPECIFIED;
	lstrcpy (m_szScriptBuildNumber, "00.00.00.0000");
	lstrcpy (m_szInjectBuildNumber, "00.00.00.0000");
}


void CPrepDoc::SetStatusBarText (int i, char *pszText)
{
	if (!g_bCommandLine)
	{
		SendMessage (m_StatusWindow, SB_SETTEXT, (WPARAM) i | 0, (LPARAM)(LPSTR) pszText);
	}
}


bool CPrepDoc::SaveDoc ()
{
	char szScriptName[MAX_PATH*2];

	if (m_bPreventScriptBuildNumberUpdate == true)
	{
		m_bPreventScriptBuildNumberUpdate = false;
	}
	else
	{
		if (IsListDirty())
		{
			IncrementScriptBuildNumber();
		}
	}

	StripAppSettings();
	WriteAppSettings();
	NukeListView();
	FillListView();

	lstrcpy (szScriptName, GetScriptName());

	if (szScriptName)
	{
		if (GetSetupDoc()->WriteTextFile (szScriptName))
		{
			SetListDirtyState (false);
			UpdateWindowText();
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if (FileSaveDialog (g_hAppWnd, (char *)&szScriptName, STR_FILE_FILTER_SCRIPT))
		{
			if (GetSetupDoc()->WriteTextFile (szScriptName))
			{
				SetListDirtyState (false);
				UpdateWindowText();
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}


bool CPrepDoc::CheckForUnsavedDoc()
{
	if (IsListDirty())
	{
		switch (Alert( g_hAppWnd, MB_YESNOCANCEL | MB_ICONEXCLAMATION, STR_SAVECHANGES, GetScriptName()))
		{
			case IDYES:
					if (!ValidateCommands())
					{
						if (Alert( g_hAppWnd, MB_OKCANCEL | MB_ICONEXCLAMATION, STR_COMMANDWITHNOFLAGS, "") == IDCANCEL)
							break;
					}
				SaveDoc();
				break;
			case IDCANCEL:
				return false;
		}
	}
	return true;
}


bool CPrepDoc::CloseDoc(bool bIgonoreUI)
{
	if (!bIgonoreUI)
	{
		m_CommandListView->DeleteAllItems();

		InvalidateRect (GetCommandListView()->GetHwnd(), NULL, true);
		InvalidateRect (GetCommandListView()->GetLvHwnd(), NULL, true);
	}

	if (m_pszAppName)
	{
		free (m_pszAppName);
		m_pszAppName = NULL;
	}

	if (m_pszProgramPath)
	{
		free (m_pszProgramPath);
		m_pszProgramPath=NULL;
	}

	if (m_pszSetupExe)
	{
		free (m_pszSetupExe);
		m_pszSetupExe = NULL;
	}

	if (m_pszSetupDll)
	{
		free (m_pszSetupDll);
		m_pszSetupDll = NULL;
	}

	if (m_pszDropPath)
	{
		free (m_pszDropPath);
		m_pszDropPath = NULL;
	}

	if (m_pszSourcePath)
	{
		free (m_pszSourcePath);
		m_pszSourcePath	= NULL;
	}

	if (m_pszScriptName)
	{
		free (m_pszScriptName);
		m_pszScriptName	= NULL;
	}

	if (m_pszReparentPath)
	{
		free (m_pszReparentPath);
		m_pszReparentPath = NULL;
	}

	if (m_pszCabFileName)
	{
		free (m_pszCabFileName);
		m_pszCabFileName = NULL;
	}

	if (m_pszBinarySaveAsName)
	{
		free (m_pszBinarySaveAsName);
		m_pszBinarySaveAsName = NULL;
	}

	if (m_pszUninstallExe)
	{
		free (m_pszUninstallExe);
		m_pszUninstallExe = NULL;
	}

	if (m_pszTrialExe)
	{
		free (m_pszTrialExe);
		m_pszTrialExe = NULL;
	}

	if (m_pszMakeCabExe)
	{
		free (m_pszMakeCabExe);
		m_pszMakeCabExe = NULL;
	}

	if (m_pSetupDoc)
	{
		delete m_pSetupDoc;
		m_pSetupDoc	= NULL;
	}

	if (m_pFileList)
	{
		delete m_pFileList;
		m_pFileList	= NULL;
	}

	if (m_pFileHist)
	{
		delete m_pFileHist;
		m_pFileHist	= NULL;
	}

	if (m_pFileRule)
	{
		delete m_pFileRule;
		m_pFileRule	= NULL;
	}

	if (m_pStringList)
	{
		delete m_pStringList;
		m_pStringList = NULL;
	}
	
	m_bListDirty		= false;
	m_bDocLoaded		= false;
	m_bShowComments		= true;
	m_bShowRegular		= true;
	m_bShowInternal		= true;
	m_bShowColor		= true;
	m_bShowProperty		= false;
	m_bEnableLocalization = false;
	m_nStringID			= BASE_DYNAMIC_STRING_ID;
	m_dwLangID			= 0;
	m_dwPrepStubBuildType = PBT_UPDATE;

	m_bReplicateFileTree = true;
	m_bInjectStaticStrings = false;
	m_bInjectDynamicStrings = false;
	m_bInjectBinaryBlob = true;

	m_bIgnoreStrings		= true;
	m_bLoadScriptStrings	= false;
	m_bLoadDllStrings		= false;
	m_bUpdateStaticStrings	= false;
	m_bUpdateFileList		= true;
	m_dwLastBuildType		= PBT_UNKNOWN;
	m_nMaxDisk				= DISK_NOT_SPECIFIED;
	lstrcpy (m_szScriptBuildNumber, "00.00.00.0000");
	lstrcpy (m_szInjectBuildNumber, "00.00.00.0000");

	return true;
}


bool CPrepDoc::Create (HWND hAppWnd, HINSTANCE hAppInst, char *szCurrentPath)
{

	SetHwnd (hAppWnd);
	SetHinst (hAppInst);
	SetProgramPath (szCurrentPath);

	m_pSetupDoc = new CSetupDoc;

	if (!m_pSetupDoc)
		return false;

	m_pFileList = new CFileList;

	if (!m_pFileList)
		return false;

	m_pFileRule = new CFileRule;

	if (!m_pFileRule)
		return false;

	m_pFileHist = new CFileHist;

	if (!m_pFileHist)
		return false;

	m_pStringList = new CStringList;

	if (!m_pStringList)
		return false;

	m_nStringID	= BASE_DYNAMIC_STRING_ID;

	return true;
}


int CompareByCabFolder (const void *pItem1, const void *pItem2)
{
	CCommand *pc1 = (CCommand *)(*(CCommand **)pItem1);
	CCommand *pc2 = (CCommand *)(*(CCommand **)pItem2);

	// sort first by cab folder #, (actually precopy v.s. regular.) right now.
	// secondary sort by source file name to keep the list in the original order.
	if (pc2->GetCabPreCopy() == pc1->GetCabPreCopy())
	{
		if (pc1->GetSourceName()==NULL && pc2->GetSourceName()==NULL)
			return 0;
		if (pc1->GetSourceName()==NULL)
			return -1;
		if (pc2->GetSourceName()==NULL)
			return 1;
		return (strcmpi (pc1->GetSourceName(), pc2->GetSourceName()));
	}
	else
	{
		// note: if we ever covert this to sorting folder#, rather than 1=precopy & 0=regular,
		// be sure to switch the order of subtraction below to sort in ascending form.
		return (pc2->GetCabPreCopy() - pc1->GetCabPreCopy());
	}
}


bool CPrepDoc::SortDynamicAreaByCabFolder()
{
	// find the start and end of the dynamic file list
	int iStart = GetSetupDoc()->FindFirstToken (TOK_BEGINFILELIST); 
	int iEnd = GetSetupDoc()->FindFirstToken (TOK_ENDFILELIST);

	// if either token not found, bail.
	if (iStart == -1 || iEnd == -1)
	{
		return false;
	}

	// now find the first InstallList token inside the dynamic file list area.
	iStart = GetSetupDoc()->FindFirstToken (TOK_INSTALLLIST, iStart, iEnd);

	// if there is no InstallList, the list is empty and we have no work to do.
	if (iStart == -1)
	{
		return true;
	}

	iEnd--;

	GetSetupDoc()->SortCommandsInRange (iStart, iEnd, CompareByCabFolder);

	return true;
}

bool CPrepDoc::RecreateHist()
{
	delete m_pFileHist;
	m_pFileHist = NULL;

	m_pFileHist = new CFileHist;

	if (!m_pFileHist)
		return false;

	return true;
}

bool CPrepDoc::RecreateRules()
{
	delete m_pFileRule;
	m_pFileRule = NULL;

	m_pFileRule = new CFileRule;

	if (!m_pFileRule)
		return false;

	return true;
}


BOOL CPrepDoc::CreateProgressDlg()
{
	m_ProgressDlg = new CProgressDialog;
	return (m_ProgressDlg->Create (m_hInst, MAKEINTRESOURCE(IDD_PROGRESS), m_hWnd));
}


BOOL CPrepDoc::CreateProgressDlg(char *szLogFile)
{
	m_ProgressDlg = new CProgressDialog;
	return (m_ProgressDlg->Create (szLogFile));
}


CPrepDoc::~CPrepDoc (void)
{
	if (m_pszSetupExe)
	{
		free (m_pszSetupExe);
		m_pszSetupExe		= NULL;
	}

	if (m_pszSetupDll)
	{
		free (m_pszSetupDll);
		m_pszSetupDll		= NULL;
	}

	if (m_pszDropPath)
	{
		free (m_pszDropPath);
		m_pszDropPath		= NULL;
	}

	if (m_pszProgramPath)
	{
		free (m_pszProgramPath);
		m_pszProgramPath	= NULL;
	}

	if (m_pszSourcePath)
	{
		free (m_pszSourcePath);
		m_pszSourcePath		= NULL;
	}

	if (m_pszScriptName)
	{
		free (m_pszScriptName);
		m_pszScriptName		= NULL;
	}

	if (m_pszReparentPath)
	{
		free (m_pszReparentPath);
		m_pszReparentPath	= NULL;
	}

	if (m_pszAppName)
	{
		free (m_pszAppName);
		m_pszAppName		= NULL;
	}

	if (m_pszCabFileName)
	{
		free (m_pszCabFileName);
		m_pszCabFileName = NULL;
	}

	if (m_pszUninstallExe)
	{
		free (m_pszUninstallExe);
		m_pszUninstallExe = NULL;
	}

	if (m_pszTrialExe)
	{
		free (m_pszTrialExe);
		m_pszTrialExe = NULL;
	}

	if (m_pszMakeCabExe)
	{
		free (m_pszMakeCabExe);
		m_pszMakeCabExe = NULL;
	}

	if (m_pszBinarySaveAsName)
	{
		free (m_pszBinarySaveAsName);
		m_pszBinarySaveAsName = NULL;
	}	

	if (m_pSetupDoc)
	{
		delete m_pSetupDoc;
		m_pSetupDoc			= NULL;
	}

	if (m_pFileList)
	{
		delete m_pFileList;
		m_pFileList			= NULL;
	}

	if (m_pFileHist)
	{
		delete m_pFileHist;
		m_pFileHist			= NULL;
	}

	if (m_pFileRule)
	{
		delete m_pFileRule;
		m_pFileRule			= NULL;
	}
	
	if (m_DynaTreeView)
	{
		delete m_DynaTreeView;
		m_DynaTreeView = NULL;
	}

	m_CommandListView->RemoveImageList();
	m_CommandListView->Delete();

	if (m_CommandListView)
	{
		delete m_CommandListView;
		m_CommandListView	= NULL;
	}

	if (m_MainTabView)
	{
		delete m_MainTabView;
		m_MainTabView		= NULL;
	}

	if (m_MainAppFrameWnd)
	{
		delete m_MainAppFrameWnd;
		m_MainAppFrameWnd	= NULL;
	}

	m_hWnd				= NULL;
	m_bDocLoaded		= false;
	m_bListDirty		= false;
	m_bShowComments		= true;
	m_bShowRegular		= true;
	m_bShowInternal		= true;
	m_bShowColor		= true;
	m_bShowProperty		= false;
	m_bEnableLocalization = false;
	m_dwPrepStubBuildType = PBT_UPDATE;

	m_bReplicateFileTree = true;
	m_bInjectStaticStrings = false;
	m_bInjectDynamicStrings = false;
	m_bInjectBinaryBlob = true;
	m_bUpdateFileList		= true;

	m_bIgnoreStrings		= true;
	m_bLoadScriptStrings	= false;
	m_bLoadDllStrings		= false;
	m_bUpdateStaticStrings	= false;
	m_dwLastBuildType		= PBT_UNKNOWN;
	m_nMaxDisk				= DISK_NOT_SPECIFIED;
}


bool PathExists (const char *szPath)
{
	// if no file or directory exists, return false
	if (0xFFFFFFFF == GetFileAttributes (szPath))
	{
		return false;
	}

	// something exists with that name, make sure it is a directory
	return (FILE_ATTRIBUTE_DIRECTORY && GetFileAttributes (szPath))?true:false;
}


bool GetNextCommandLineToken (char **pp, char *szToken)
{
	char *p = *pp;

	if (*p==NULL) 
		return false;

	// skip white space
	while (*p && (isspace (*p))) 
	{
		++p;
	}

	while (*p && (!isspace (*p)))
	{
		*(szToken++) = *(p++);
	}
	*szToken = '\0';

	*pp = p;
	return true;
}


bool CPrepDoc::ParseCommandLine (char *szCommandLine)
{
	char szToken[MAX_PATH];

	char *p = szCommandLine;

	if (strcmpi (szCommandLine, "")==0)
	{
		WriteLogFileMessage (STR_CL_USAGE, g_szAppTitle);
		return false;
	}

	// set some default
	SetEnableLocalization (false);
	SetPrepStubBuildType (BUILD_UPDATE_AND_INJECT);

	SetCreateDirNonFatal (false);
	m_bReparent = true;
	// now parse the commandline 
	while (GetNextCommandLineToken (&p, szToken))
	{
		if (szToken[0] == '-' || szToken[0] == '/')
		{
			switch (tolower (szToken[1]))
			{
				case '?':
					WriteLogFileMessage (STR_CL_USAGE, g_szAppTitle);
					WriteLogFileMessage (STR_CL_PRESSANY);
					getch();
					return false;
				case 'f':	// script name
					if (GetNextCommandLineToken (&p, szToken))
					{
						SetScriptName (szToken);
					}
					else
					{
						WriteLogFileMessage (STR_CL_MISSING_FILENAME);
						return false;
					}
					break;
				case 'b': // output binary file
					if (GetNextCommandLineToken (&p, szToken))
					{
						SetBinarySaveAsName (szToken);
					}
					else
					{
						WriteLogFileMessage (STR_CL_MISSING_BINARY);
					}
					break;
				case 'l':  // enable localization
					SetEnableLocalization (true);
					break;

				case 'r': // replicate type of build
					SetPrepStubBuildType (BUILD_REPLICATE);
					break;

				case 'u': // update type of build
					SetPrepStubBuildType (BUILD_UPDATE_AND_INJECT);
					break;

				case 't': // build Trial version
					SetPrepStubBuildType (BUILD_TRIAL);
					break;

				case 'w': // SaveAs Binary Blob
					SetPrepStubBuildType (BUILD_SAVEAS);
					break;

				case 'x': // ignore the reparent option - this is a hack because the core parser has a bug parsing a null string """""" as " 
					m_bReparent = false;
					break;

				case 'n': // treat directory creation failure as non-fatal.
					SetCreateDirNonFatal (true);
					break;

				default:  // unknown option
					WriteLogFileMessage (STR_CL_UNKNOWN_PARAM, szToken);
					break;
			}
		}
		else
		{
			WriteLogFileMessage (STR_CL_UNKNOWN_PARAM, szToken);
			return true;
		}
	}
	return true;
}

void AddFileEntryToDiamondDirective (char *szSource, char *szDest, DWORD dwUserData)
{
	FILE *fp = (FILE *)dwUserData;

	fprintf (fp,"\"%s\" \"%s\"\n", szSource, szDest);
}

#define CONVERTTOFOLDER(n)(int)(!n)


bool CPrepDoc::WriteDiamondDirectiveFile (char *szDirectiveFile)
{
	int nCurrentFolder = 0;
	char szSourceFileName[MAX_PATH*2];
	char szDestFileName[MAX_PATH*2];
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	FILE *fp;
	
	//
	// Delete file, if it exists
	// 

	DeleteFile (szDirectiveFile);


	//
	// Open Directive File for writing
	//

	fp = fopen (szDirectiveFile, "w+");

	if (fp == NULL)
	{
		return false;
	}

	//
	// write the diamond directive file header
	//

	fprintf (fp, ";Diamond Directive File - Generated by %s\n", g_szAppTitle);
	fprintf (fp, ".option explicit\n");
	fprintf (fp, ".set DiskDirectoryTemplate=%s\n", GetDropPath());
	fprintf (fp, ".set RptFileName=nul\n");
	fprintf (fp, ".set InfFileName=nul\n");
	fprintf (fp, ".set MaxDiskSize=999999488\n");
	fprintf (fp, ".set Compress=on\n");
	fprintf (fp, ".set Cabinet=on\n");
	fprintf (fp, ".set CompressionType=LZX\n");
	fprintf (fp, ".set CompressionMemory=21\n");
	fprintf (fp, ".set CabinetNameTemplate=%s\n", GetCabFileName());

	//
	// insert all of the "pre-copy" execute action files.
	//
	ExecuteAddToCabActions (AddFileEntryToDiamondDirective, (DWORD)fp, true);

	nCurrentFolder = 0;

	//
	// Generate the file list
	//
		
	for (i=0; i < nCommands; i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);

		if (pCmd->GetCommandType() == TOK_INSTALLLIST)
		{
			int nFolder = CONVERTTOFOLDER (pCmd->GetCabPreCopy());

			if (nFolder != nCurrentFolder)
			{
				// cab folder changed, see if we are moving forward in folder #
				if (nFolder < nCurrentFolder)
				{
					// found an installlist command with a cab folder number less than the current
					// folder number.  we don't sort static commands, so this is a fatal error.
					fclose (fp);
					Alert (m_hWnd, MB_OK, STR_BADCABFOLDERORDER);
					return false;
				}

				// tell the diamond directive file to bump the cab folder number
				fprintf (fp, ".new folder\n");

				nCurrentFolder = nFolder;
				
			}


			if (GetLastBuildType()==PBT_REPLICATE)
			{
				lstrcpy (szSourceFileName, GetDropPath());
			}
			else
			{
				lstrcpy (szSourceFileName, GetSourcePath());
			}

			ConvertToValidPath (szSourceFileName);

			lstrcat (szSourceFileName, pCmd->GetSourceName());
			ExpandSubstitutedStrings (szSourceFileName);

			lstrcpy (szDestFileName, pCmd->GetSourceName());
			ExpandSubstitutedStrings (szDestFileName);

			// format: source (from harddrive), dest (in cab file)
			fprintf (fp,"\"%s\" \"%s\"\n", szSourceFileName, szDestFileName);
		}
	}

	//
	// insert all of the "regular" execute action files.
	//
	
	ExecuteAddToCabActions (AddFileEntryToDiamondDirective, (DWORD)fp, false);
		
	fclose (fp);
	return true;
}



bool CPrepDoc::ReadAppSettings ()
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	SetBuildFlags (0xFFFFFFFF);
	SetReplicateFileTree (true);

	SetCabFileName ("msgame.cab");
	SetUninstallExe ("uninstall.exe");
	SetTrialExe ("GameTrial.exe");

	for (i=0; i < nCommands; i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->GetCommandType() == TOK_PROPERTY)
		{
			const char *pszProperty = pCmd->GetProperty();
			const char *pszPropertyValue = pCmd->GetPropertyValue();

			// hack: if a string is null, the hotsetup parser returns a " char.
			if (strcmp (pszPropertyValue, "\"")==0)
			{
				pCmd->SetPropertyValue ("");
				pszPropertyValue = pCmd->GetPropertyValue();
			}

			if (strcmpi (pszProperty, "sourcepath")==0)
			{
				SetSourcePath (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "droppath")==0)
			{
				SetDropPath (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "reparentpath")==0)
			{
				SetReparentPath (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "setupexe")==0)
			{
				SetSetupExe (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "setupdll")==0)
			{
				SetSetupDll (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "localizebuild")==0)
			{
				if (*pszPropertyValue == '1') 
					SetEnableLocalization (true);
				else
					SetEnableLocalization (false);
			}
			else
			if (strcmpi (pszProperty, "ReplicateFileTree")==0)
			{
				if (*pszPropertyValue == '1') 
					SetReplicateFileTree (true);
				else
					SetReplicateFileTree (false);
			}
			else
			if (strcmpi (pszProperty, "InjectDynamicStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetInjectDynamicStrings (true);
				else
					SetInjectDynamicStrings (false);
			}
			else
			if (strcmpi (pszProperty, "InjectStaticStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetInjectStaticStrings(true);
				else
					SetInjectStaticStrings(false);
			}
			else
			if (strcmpi (pszProperty, "InjectBinaryBlob")==0)
			{
				if (*pszPropertyValue == '1') 
					SetInjectBinaryBlob(true);
				else
					SetInjectBinaryBlob (false);
			}
			else
			if (strcmpi (pszProperty, "LoadScriptStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetLoadScriptStrings (true);
				else
					SetLoadScriptStrings (false);
			}
			else
			if (strcmpi (pszProperty, "LoadDllStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetLoadDllStrings (true);
				else
					SetLoadDllStrings (false);
			}
			else
			if (strcmpi (pszProperty, "IgnoreStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetIgnoreStrings (true);
				else
					SetIgnoreStrings (false);
			}
			else
			if (strcmpi (pszProperty, "CabFileName")==0)
			{
				SetCabFileName (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "UninstallExe")==0)
			{
				SetUninstallExe (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "TrialExe")==0)
			{
				SetTrialExe (pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "MakeCabExe")==0)
			{
				SetMakeCabExe (pszPropertyValue);
			}
			/*
			else
			if (strcmpi (pszProperty, "UpdateStaticStrings")==0)
			{
				if (*pszPropertyValue == '1') 
					SetUpdateStaticStrings (true);
				else
					SetUpdateStaticStrings (false);
			}*/
			else
			if (strcmpi (pszProperty, "verification_flags")==0)
			{
				DWORD dwBuildFlags;
				sscanf (pszPropertyValue, "%x", &dwBuildFlags);
				SetBuildFlags (dwBuildFlags);
			}
			else
			if (strcmpi (pszProperty, "last_build_type")==0)
			{
				DWORD dwLastBuildType;
				sscanf (pszPropertyValue, "%x", &dwLastBuildType);
				SetLastBuildType (dwLastBuildType);
			}
			else
			if (strcmpi (pszProperty, "Script_Version")==0)
			{
				lstrcpy (m_szScriptBuildNumber, pszPropertyValue);
			}
			else
			if (strcmpi (pszProperty, "Inject_Version")==0)
			{
				lstrcpy (m_szInjectBuildNumber, pszPropertyValue);
			}

		}
	}
	return true;
}


bool CPrepDoc::NewDoc()
{
	m_pSetupDoc->SetCommandPtr(0);
	InsertComment ("Start of Dynamic File List");
	InsertComment ("End of Dynamic File List");
	return true;
}


bool CPrepDoc::InsertNewline ()
{
	CCommand *pCmd;
	
	pCmd = new CCommand;

	if (!pCmd)
		return false;

	pCmd->SetCommandType (TOK_NEWLINE);
	pCmd->SetCommandID (-1);
	
	m_pSetupDoc->InsertCommand (pCmd);
	return true;
}


bool CPrepDoc::InsertToken (ETOKEN eToken)
{
	CCommand *pCmd;
	
	pCmd = new CCommand;

	if (!pCmd)
		return false;

	pCmd->SetCommandType (eToken);
	pCmd->SetCommandID (-1);
	
	m_pSetupDoc->InsertCommand (pCmd);
	return true;
}


bool CPrepDoc::InsertProperty (char *szProperty, char *szValue)
{
	CCommand *pCmd;
	
	if (szProperty == NULL || szValue == NULL)
	{
		return false;
	}

	pCmd = new CCommand;

	if (!pCmd)
	{
		return false;
	}

	pCmd->SetProperty (szProperty);
	pCmd->SetPropertyValue (szValue);
	pCmd->SetCommandType (TOK_PROPERTY);
	pCmd->SetCommandID (-1);
	
	m_pSetupDoc->InsertCommand (pCmd);
	
	InsertNewline();
	return true;
}


bool CPrepDoc::InsertComment (char *szComment)
{
	CCommand *pCmd = new CCommand;

	if (!pCmd)
		return false;

	pCmd->SetComment (szComment);
	pCmd->SetCommandType (TOK_COMMENT);
	pCmd->SetCommandID (-1);
	
	m_pSetupDoc->InsertCommand (pCmd);
	
	InsertNewline();
	return true;
}


bool CPrepDoc::WriteAppSettings ()
{
	char szTempStr[12];
	char szBuildNumber[16];

	LoadString (g_hAppInst, STR_BUILD_NUMBER, szBuildNumber, 15);

	// set the command pointer to beginning of list
	m_pSetupDoc->SetCommandPtr(0);
	
	InsertProperty ("PrepStub99_Version",	szBuildNumber);
	InsertProperty ("Script_Version",		m_szScriptBuildNumber);
	InsertProperty ("Inject_Version",		m_szInjectBuildNumber);
	InsertProperty ("SourcePath",			GetSourcePath());
	InsertProperty ("DropPath",				GetDropPath());
	InsertProperty ("ReparentPath",			GetReparentPath());
	InsertProperty ("SetupExe",				GetSetupExe());
	InsertProperty ("SetupDll",				GetSetupDll());
	InsertProperty ("ReplicateFileTree",	IsReplicateFileTree()?"1":"0");
	InsertProperty ("InjectDynamicStrings", IsInjectDynamicStrings()?"1":"0");
	InsertProperty ("InjectStaticStrings",	IsInjectStaticStrings()?"1":"0");
	InsertProperty ("InjectBinaryBlob",		IsInjectBinaryBlob()?"1":"0");
	InsertProperty ("LoadScriptStrings",	IsLoadScriptStrings	()?"1":"0");
	InsertProperty ("LoadDllStrings",		IsLoadDllStrings ()?"1":"0");
	InsertProperty ("IgnoreStrings",		IsIgnoreStrings	()?"1":"0");
	InsertProperty ("CabFileName",			GetCabFileName());
	InsertProperty ("TrialExe",				GetTrialExe());
	InsertProperty ("MakeCabExe",			GetMakeCabExe());
	InsertProperty ("UninstallExe",			GetUninstallExe());
	InsertProperty ("UpdateFileList",		IsUpdateFileList()?"1":"0");

	wsprintf (szTempStr, "%x", GetBuildFlags());
	InsertProperty ("Verification_Flags", szTempStr);

	wsprintf (szTempStr, "%x", GetLastBuildType());
	InsertProperty ("Last_Build_Type", szTempStr);

	return true;
}


bool CPrepDoc::StripAppSettings ()
{
	CCommand *pCmd;

	int nCommands = m_pSetupDoc->GetNumCommands();
	int i = 0;
		
	while (i < nCommands)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);

		if (pCmd->GetCommandType() == TOK_PROPERTY)
		{
			m_pSetupDoc->SetCommandPtr (i);
			m_pSetupDoc->DeleteCommand();
			nCommands = nCommands - 1;

			pCmd = m_pSetupDoc->GetNthCommand (i);

			if (pCmd->GetCommandType() == TOK_NEWLINE)
			{
				m_pSetupDoc->SetCommandPtr (i);
				m_pSetupDoc->DeleteCommand();
				nCommands = nCommands - 1;
			}
		}
		else
		{
			++i;
		}
	}
	return true;
}


void ExpandSlashes (TCHAR *szIn, TCHAR *szOut)
{
	TCHAR *p = szIn, *p2 = szOut;
	
	while (*p)
	{
		if (*p=='\\') 
		{
			*(p2++)=*p;
			*(p2++)=*p;
		}
		else
		*(p2++)=*p;
		++p;
	}
	*p2='\0';
}


bool CPrepDoc::WriteStringsToRC3 (char *szRC3File, int nStartID, int nEndID, int nBaseID)
{
	DWORD dwBytes;
	char szBuffer[MAX_PATH*2], szBufferOut[MAX_PATH*4];
	int nStringID;
	HANDLE hFile;

	hFile = CreateFile (szRC3File, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		return false;
	}

	wsprintf (szBuffer, "//\r\n//\r\n// This file was automatically generated by Prepstub99\r\n//\r\n//\r\n\r\n");
	WriteFile (hFile, &szBuffer, lstrlen (szBuffer), &dwBytes, NULL);

	wsprintf (szBuffer, "STRINGTABLE DISCARDABLE\r\nBEGIN\r\n");
	WriteFile (hFile, &szBuffer, lstrlen (szBuffer), &dwBytes, NULL);

	for (int i=0; i < GetStringList()->GetListCount(); i++)
	{
		nStringID = GetStringList()->GetStringID (i);
		
		if (nStringID >= nStartID && nStringID <= nEndID)
		{
			wsprintf (szBuffer, "\t%d  \"%s\"\r\n", GetStringList()->GetStringID(i)-nStartID+nBaseID, GetStringList()->GetStringValue(i));
			ExpandSlashes (szBuffer, szBufferOut);
			WriteFile (hFile, &szBufferOut, lstrlen (szBufferOut), &dwBytes, NULL);
		}
	}

	wsprintf (szBuffer, "END\r\n");
	WriteFile (hFile, &szBuffer, lstrlen (szBuffer), &dwBytes, NULL);

	CloseHandle (hFile);
	return true;
}

bool CPrepDoc::WriteStringGroupToResource (HANDLE hUpdateResource, int nStartID, int nEndID, int nBaseID)
{
	#define STRING_BLOCK_SIZE	16
	char szUnicodeString[MAX_PATH*2];
	char buffer[((MAX_PATH*4) + 2) * STRING_BLOCK_SIZE];	// space for 16 unicode strings and a WORD length identifier
	char *pBuffer = buffer;
	int nCount = 0;
	int nFirstStringIndex = -1;
	int nLastStringIndex = -1;
	int nStringID = 0;
	BOOL result;
	WORD wLen;

	ASSERT (0 != GetLangID());

	// count the strings in the range and find the first and last string index
	for (int i=0; i < GetStringList()->GetListCount(); i++)
	{
		nStringID = GetStringList()->GetStringID (i);
		
		if (GetStringList()->GetStringID (i) >= nStartID && GetStringList()->GetStringID (i) <= nEndID)
		{
			if (nFirstStringIndex == -1)
			{
				nFirstStringIndex = i;
			}

			nLastStringIndex = i;

			++nCount;
		}
	}

	// clear the resource buffer
	ZeroMemory (buffer, sizeof (buffer));

	// if we found at least 1 string, write it to the resource in block format
	if (nCount > 0)
	{
		int nFirstBlock		= ((nBaseID - nStartID + GetStringList()->GetStringID (nFirstStringIndex)) / 16) ;
		int nLastBlock		= ((nBaseID - nStartID + GetStringList()->GetStringID (nLastStringIndex))  / 16) ;
		int nStringCount	= (nLastBlock - nFirstBlock + 1) * 16;
		int nCurrentString	= nFirstStringIndex;
		int nString			= 0;
		int nBlock			= nFirstBlock;

		// walk through the block of strings, checking to see if we have a valid string along the way.
		while (nString < nStringCount)
		{
			int nNextValidStringID = nBaseID - nStartID + GetStringList()->GetStringID(nCurrentString);
			int nCurrentStringID = (nString + ((nFirstBlock) * 16));

			// see if the next valid stringID is less than the string id we are currently writing, if it is,
			// then we don't have any strings to write, so we pad with NULL.
			//
			// if the next valid string is -1, then we have run out of strings, so just pad with null 
			//
			if (nNextValidStringID == nBaseID - nStartID -1 || nCurrentStringID < nNextValidStringID)
			{
				// no string at this ID, so write a null char
				*(pBuffer++) = '\0';
				*(pBuffer++) = '\0';
			}
			else
			// if the next valid string is the same as the one we are currently writing, format the string and
			// add it tot he buffer
			if (nCurrentStringID == nNextValidStringID)
			{
				// build string here.

				ZeroMemory (szUnicodeString, sizeof (szUnicodeString));

				// convert the ANSI string to a UNICODE string, placing the result into
				// the szUnicodeString buffer offset by a word
				MultiByteToWideChar (	CP_ACP, 
										MB_PRECOMPOSED, 
										GetStringList()->GetStringValue (nCurrentString),
										-1,
										(LPWSTR) &szUnicodeString + 1,
										sizeof (szUnicodeString));

				// see how long the resulting unicode string is
				wLen = lstrlenW ((LPWSTR) &szUnicodeString + 1);

				// write a string length of type word before the unicode string
				szUnicodeString[0] = LOBYTE (wLen);
				szUnicodeString[1] = HIBYTE (wLen);

				// copy this size+unicode_string into the buffer
				CopyMemory (pBuffer, &szUnicodeString, (wLen * 2) + 2);

				pBuffer += (wLen * 2 ) + 2;

				// increment the index for the next string we want to write
				++nCurrentString;
			}
			// if the string we are trying to write is > than the next valid string, we somehow got out of sync
			// and should have already written the valid string.  This should not happen.
			else
			{
				ASSERT (FALSE);
			}			

			// see if we are at the end of the block, if so, write the string block to the resource
			if ((nString+1) % 16 == 0)
			{
				result = UpdateResource(hUpdateResource,     
					RT_STRING,
					MAKEINTRESOURCE ( ( ( nBaseID + nString - 15) / 16) + 1 ),
					GetLangID(),
					(LPWSTR) &buffer,
					pBuffer - buffer); 

				if (FALSE == result)
				{
					return false;
				}

				//
				//bugbug:: do we need to pad the resource size?
				//
							
				// clear the buffer and reset for the next block
				ZeroMemory (buffer, sizeof (buffer));
				pBuffer = buffer;

				++ nBlock;
			}

			++nString;
		}

		ASSERT (nCount == nCurrentString - nFirstStringIndex);  // did we write as many strings as we originally counted in the range?
	}

	return true;
}


bool CPrepDoc::WriteStringGroupToResource (HANDLE hUpdateResource, CStringList *pStringList, int nStartID, int nEndID, int nBaseID)
{
	#define STRING_BLOCK_SIZE	16
	char szUnicodeString[MAX_PATH*2];
	char buffer[((MAX_PATH*4) + 2) * STRING_BLOCK_SIZE];	// space for 16 unicode strings and a WORD length identifier
	char *pBuffer = buffer;
	int nCount = 0;
	int nFirstStringIndex = -1;
	int nLastStringIndex = -1;
	int nStringID = 0;
	BOOL result;
	WORD wLen;

	ASSERT (0 != GetLangID());

	// count the strings in the range and find the first and last string index
	for (int i=0; i < pStringList->GetListCount(); i++)
	{
		nStringID = pStringList->GetStringID (i);
		
		if (pStringList->GetStringID (i) >= nStartID && pStringList->GetStringID (i) <= nEndID)
		{
			if (nFirstStringIndex == -1)
			{
				nFirstStringIndex = i;
			}

			nLastStringIndex = i;

			++nCount;
		}
	}

	// clear the resource buffer
	ZeroMemory (buffer, sizeof (buffer));

	// if we found at least 1 string, write it to the resource in block format
	if (nCount > 0)
	{
		int nFirstBlock		= ((nBaseID - nStartID + pStringList->GetStringID (nFirstStringIndex)) / 16) ;
		int nLastBlock		= ((nBaseID - nStartID + pStringList->GetStringID (nLastStringIndex))  / 16) ;
		int nStringCount	= (nLastBlock - nFirstBlock + 1) * 16;
		int nCurrentString	= nFirstStringIndex;
		int nString			= 0;
		int nBlock			= nFirstBlock;

		// walk through the block of strings, checking to see if we have a valid string along the way.
		while (nString < nStringCount)
		{
			int nNextValidStringID = nBaseID - nStartID + pStringList->GetStringID(nCurrentString);
			int nCurrentStringID = (nString + ((nFirstBlock) * 16));

			// see if the next valid stringID is less than the string id we are currently writing, if it is,
			// then we don't have any strings to write, so we pad with NULL.
			//
			// if the next valid string is -1, then we have run out of strings, so just pad with null 
			//
			if (nNextValidStringID == nBaseID - nStartID -1 || nCurrentStringID < nNextValidStringID)
			{
				// no string at this ID, so write a null char
				*(pBuffer++) = '\0';
				*(pBuffer++) = '\0';
			}
			else
			// if the next valid string is the same as the one we are currently writing, format the string and
			// add it tot he buffer
			if (nCurrentStringID == nNextValidStringID)
			{
				// build string here.

				ZeroMemory (szUnicodeString, sizeof (szUnicodeString));

				// convert the ANSI string to a UNICODE string, placing the result into
				// the szUnicodeString buffer offset by a word
				MultiByteToWideChar (	CP_ACP, 
										MB_PRECOMPOSED, 
										pStringList->GetStringValue (nCurrentString),
										-1,
										(LPWSTR) &szUnicodeString + 1,
										sizeof (szUnicodeString));

				// see how long the resulting unicode string is
				wLen = lstrlenW ((LPWSTR) &szUnicodeString + 1);

				// write a string length of type word before the unicode string
				szUnicodeString[0] = LOBYTE (wLen);
				szUnicodeString[1] = HIBYTE (wLen);

				// copy this size+unicode_string into the buffer
				CopyMemory (pBuffer, &szUnicodeString, (wLen * 2) + 2);

				pBuffer += (wLen * 2 ) + 2;

				// increment the index for the next string we want to write
				++nCurrentString;
			}
			// if the string we are trying to write is > than the next valid string, we somehow got out of sync
			// and should have already written the valid string.  This should not happen.
			else
			{
				ASSERT (FALSE);
			}			

			// see if we are at the end of the block, if so, write the string block to the resource
			if ((nString+1) % 16 == 0)
			{
				result = UpdateResource(hUpdateResource,     
					RT_STRING,
					MAKEINTRESOURCE ( ( ( nBaseID + nString - 15) / 16) + 1 ),
					GetLangID(),
					(LPWSTR) &buffer,
					pBuffer - buffer); 

				if (FALSE == result)
				{
					return false;
				}

				//
				//bugbug:: do we need to pad the resource size?
				//
							
				// clear the buffer and reset for the next block
				ZeroMemory (buffer, sizeof (buffer));
				pBuffer = buffer;

				++ nBlock;
			}

			++nString;
		}

		ASSERT (nCount == nCurrentString - nFirstStringIndex);  // did we write as many strings as we originally counted in the range?
	}

	return true;
}


bool CPrepDoc::InjectStringTableInResource (const char *szModule)
{
	HANDLE hUpdateRes;  

	hUpdateRes = BeginUpdateResource(szModule, FALSE);  

	if (hUpdateRes == NULL) 
	{
		return false;
	}
	
	if (IsInjectStaticStrings())
	{
		if (!WriteStringGroupToResource (hUpdateRes, 0, 99, BASE_RESOURCE_STATIC_STRING_ID ))
		{
			EndUpdateResource(hUpdateRes, FALSE); 
			return false;
		}
	}

	if (IsInjectDynamicStrings())
	{
		if (!WriteStringGroupToResource (hUpdateRes, 100, 32000, BASE_RESOURCE_DYNAMIC_STRING_ID ))
		{
			EndUpdateResource(hUpdateRes, FALSE);
			return false;	
		}
	}

	if (!EndUpdateResource(hUpdateRes, FALSE)) 
	{
		return false;
	}
	return true;
}


void IncrementBuildNumber (char *szBuild)
{
	SYSTEMTIME time, build;
	int version, buildno;

	GetSystemTime (&time);

	sscanf (szBuild, "%2d.%2d.%2d.%4d", &version, &build.wYear, &build.wMonth, &buildno);
	build.wDay = buildno / 100;
	buildno = buildno - (build.wDay * 100);
	
	if ((build.wYear == time.wYear-1900) && (build.wMonth == time.wMonth) && (build.wDay == time.wDay))
	{
		buildno = (buildno >= 99)?99:buildno+1;
	}
	else
	{
		build.wYear = time.wYear-1900;
		build.wMonth = time.wMonth;
		build.wDay = time.wDay;
		version = 1;
		buildno = 0;
	}

	wsprintf (szBuild, "%02d.%02d.%02d.%04d", version, build.wYear, build.wMonth, build.wDay*100+buildno);
}


void CPrepDoc::IncrementScriptBuildNumber (void)
{
	IncrementBuildNumber (m_szScriptBuildNumber);
}


void CPrepDoc::IncrementInjectBuildNumber (void)
{
	IncrementBuildNumber (m_szInjectBuildNumber);
}


bool CPrepDoc::InjectVersionInfoInStringTable (const char *szModule)
{
	char szString[255];
	HANDLE hUpdateRes;  

	hUpdateRes = BeginUpdateResource(szModule, FALSE);  

	if (hUpdateRes == NULL) 
	{
		return false;
	}
	
	CStringList *pStrings = new CStringList;
	pStrings->Create();

	char szBuildNumber[15];
	LoadString (g_hAppInst, STR_BUILD_NUMBER, szBuildNumber, 15);
	wsprintf (szString, "Prepstub98 [%s]", szBuildNumber);
	pStrings->AddString (0, szString);

	wsprintf (szString, "Script [%s]", m_szScriptBuildNumber);
	pStrings->AddString (1, szString);

	wsprintf (szString, "Inject [%s]", m_szInjectBuildNumber);
	pStrings->AddString (2, szString);

	if (!WriteStringGroupToResource (hUpdateRes, pStrings, 0, 2, PREPSTUB_BUILD_NUMBERS))
	{
		EndUpdateResource(hUpdateRes, FALSE); 
		delete pStrings;
		return false;
	}

	if (!EndUpdateResource(hUpdateRes, FALSE)) 
	{
		delete pStrings;
		return false;
	}

	delete pStrings;
	return true;
}


bool CPrepDoc::InjectResource (const char *szModule, const char *szBinary)
{
	HANDLE hUpdateRes;  
	BOOL result;
	HGLOBAL hFile;
	DWORD cchFile;
	LPSTR lpFileData;

    hFile = ::LoadFile( szBinary, &cchFile);

    if( !hFile)
    {
        GlobalFree( hFile );
        hFile = NULL;
		Alert (m_hWnd, MB_OK, STR_BINOPENFAILED);
        return FALSE;
    }

    lpFileData = (LPSTR)GlobalLock( hFile );

    if( !lpFileData )
    {
        GlobalFree( hFile );
        hFile = NULL;
        Alert( m_hWnd, MB_ICONSTOP | MB_OK, STR_NOMEMORY );
        return FALSE;
    }
	
	hUpdateRes = BeginUpdateResource(szModule, FALSE);  

	if (hUpdateRes == NULL) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}

	ASSERT (0!=GetLangID());

	result = UpdateResource(hUpdateRes,     
			"SETUPBINARY",
             "SETUPDATA",                 
             //MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),       
			 GetLangID(),
             lpFileData,
             cchFile); 

	if (result == FALSE) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}
	
	if (!EndUpdateResource(hUpdateRes, FALSE)) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}

	GlobalUnlock( hFile );
	GlobalFree( hFile );
	hFile = NULL;
    lpFileData = NULL;

	return true;
}

bool CPrepDoc::InjectResourceFromFile (const char *szModule, const char *szType, const char *szName, const char *szBinary)
{
	HANDLE hUpdateRes;  
	BOOL result;
	HGLOBAL hFile;
	DWORD cchFile;
	LPSTR lpFileData;

    hFile = ::LoadFile( szBinary, &cchFile);

    if( !hFile)
    {
        GlobalFree( hFile );
        hFile = NULL;
		Alert (m_hWnd, MB_OK, STR_BINOPENFAILED);
        return FALSE;
    }

    lpFileData = (LPSTR)GlobalLock( hFile );

    if( !lpFileData )
    {
        GlobalFree( hFile );
        hFile = NULL;
        Alert( m_hWnd, MB_ICONSTOP | MB_OK, STR_NOMEMORY );
        return FALSE;
    }
	
	hUpdateRes = BeginUpdateResource(szModule, FALSE);  

	if (hUpdateRes == NULL) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}

	ASSERT (0!=GetLangID());

	result = UpdateResource(hUpdateRes,
			szType,
			szName,
			GetLangID(),
            lpFileData,
            cchFile); 

	if (result == FALSE) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}
	
	if (!EndUpdateResource(hUpdateRes, FALSE)) 
	{
		GlobalUnlock( hFile );
        GlobalFree( hFile );
        hFile = NULL;
		lpFileData = NULL;
		return false;
	}

	GlobalUnlock( hFile );
	GlobalFree( hFile );
	hFile = NULL;
    lpFileData = NULL;

	return true;
}



bool CPrepDoc::ClearFileList ()
{
	if (m_pFileList)
		delete m_pFileList;

	m_pFileList = new CFileList;

	if (!m_pFileList)
		return false;

	return true;
}


bool CPrepDoc::RemoveDynamicFileList ()
{
	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINFILELIST) + 1;
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDFILELIST) - 1;
	
	if (iStart == 0 || iEnd == -2 || iEnd < iStart)
		return false;
	
	if (iStart == iEnd) 
		return true;


	m_pSetupDoc->SetCommandPtr (iStart);

	for (int i=iStart+1;i<=iEnd;i++)
	{
		if (!m_pSetupDoc->DeleteCommand ())
			return false;
	}
	return true;
}

bool CPrepDoc::CreateDynamicStringSection()
{
	m_pSetupDoc->SetCommandPtr (m_pSetupDoc->GetNumCommands());

	if (!InsertNewline())
		return false;

	if (!InsertToken (TOK_BEGINSTRINGLIST))
		return false;

	if (!InsertNewline())
		return false;

	if (!InsertToken (TOK_ENDSTRINGLIST))
		return false;

	if (!InsertNewline())
		return false;

	return true;
}


bool CPrepDoc::CreateStaticStringSection()
{
	m_pSetupDoc->SetCommandPtr (m_pSetupDoc->GetNumCommands());

	if (!InsertNewline())
		return false;

	if (!InsertToken (TOK_BEGINSTATICSTRINGLIST))
		return false;

	if (!InsertNewline())
		return false;

	if (!InsertToken (TOK_ENDSTATICSTRINGLIST))
		return false;

	if (!InsertNewline())
		return false;

	return true;
}


bool CPrepDoc::RemoveDynamicStringList ()
{
	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINSTRINGLIST) + 1;
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDSTRINGLIST) - 1;
	
	if (iStart == 0 || iEnd == -2 || iEnd < iStart)
		return false;
	
	if (iStart == iEnd) 
		return true;

	m_nStringID	= BASE_DYNAMIC_STRING_ID;
	m_pSetupDoc->SetCommandPtr (iStart);

	for (int i=iStart+1;i<=iEnd;i++)
	{
		if (!m_pSetupDoc->DeleteCommand ())
			return false;
	}
	return true;
}


bool CPrepDoc::RemoveStaticStringList ()
{
	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINSTATICSTRINGLIST) + 1;
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDSTATICSTRINGLIST) - 1;
	
	if (iStart == 0 || iEnd == -2 || iEnd < iStart)
		return false;
	
	if (iStart == iEnd) 
		return true;

	m_pSetupDoc->SetCommandPtr (iStart);

	for (int i=iStart+1;i<=iEnd;i++)
	{
		if (!m_pSetupDoc->DeleteCommand ())
			return false;
	}
	return true;
}


bool CPrepDoc::CreateUserInterface ()
{

	// Create a main frame container for the application.
	// This container will hold three sub frames. 
	//
	// -------------------------------- 
	// |                              |
	// |  Toolbar frame  (fixed size) |
	// |                              |
	// |------------------------------|
	// |                              |
	// |                              |
	// |  TabView frame (dynamic size)|
	// |                              |
	// |                              |
	// |------------------------------|
	// |                              |
	// |  Status frame (fixed size)   |
	// |------------------------------|
	//
	m_MainAppFrameWnd = new CFrameWnd();
	
	if (!m_MainAppFrameWnd->Create (m_hInst, m_hWnd, 0, 0, 0, 0, true))
		return false;

	// 
	// Create the Tab View
	//
	m_MainTabView = (CTabView *) new CTabView;
	m_MainTabView->Create (m_hInst, m_hWnd, 0, 0, 0, 0);

	//
	// Add the tabs to the Tab View
	//
	m_MainTabView->AddTab ("Command List");
	m_MainTabView->AddTab ("String Table");
//	m_MainTabView->AddTab ("Dynamic File List");

//	m_MainTabView->AddTab ("Dynamic File List");
//	m_MainTabView->AddTab ("All Files");
	
	//
	// Create the Status Window
	//

	m_StatusWindow = CreateWindow (STATUSCLASSNAME, 
		"",
		WS_CHILD | SBARS_SIZEGRIP ,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_hAppWnd, NULL,
		g_hAppInst,
		NULL);

	int nWidths[5] = {300,350,450, 600,-1};
	SendMessage (m_StatusWindow, SB_SETPARTS, (WPARAM) 5, (LPARAM) nWidths);
	ShowWindow (m_StatusWindow, SW_SHOW);

	char szBuildNumber[15];
	LoadString (g_hAppInst, STR_BUILD_NUMBER, szBuildNumber, 15);
	SetStatusBarText (4, szBuildNumber);			
			


//	m_ToolBarWindow = CreateToolBar (g_hAppWnd);

	m_ToolBarWindow = CreateWindow ("Static", 
		"",
		WS_CHILD | SBARS_SIZEGRIP ,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_hAppWnd, NULL,
		g_hAppInst,
		NULL);


	ShowWindow (m_ToolBarWindow, SW_SHOW);
	
	RECT rect;

	GetWindowRect (m_StatusWindow, &rect);

	//
	// Create the 3 sub frames
	//

	// Toolbar frame (fixed)
	m_MainAppFrameWnd->AddFrame (true, 0); // 0 for now, until we have a toolbar

	// TabView frame (dynamic size)
	m_MainAppFrameWnd->AddFrame (false, 1);

	// Status frame (fixed)
	m_MainAppFrameWnd->AddFrame (true, rect.bottom-rect.top);

	// Attach the windows into the frame structure
	m_MainAppFrameWnd->AttachFrame (m_ToolBarWindow, 0);
	m_MainAppFrameWnd->AttachFrame (m_MainTabView->GetHwnd(), 1);    
	m_MainAppFrameWnd->AttachFrame (m_StatusWindow, 2);


	// create list view
	if (!CreateListViews())
		return false;
	
	m_MainTabView->AttachTab (m_CommandListView->GetHwnd(), 0); 
	//m_MainTabView->AttachTab (m_StringListView->GetHwnd(), 1);

	// temp testing hack to show list view in main view pane
	m_MainAppFrameWnd->AttachFrame (m_CommandListView->GetHwnd(), 1);
	
	// create the tree view

	m_DynaTreeView = new CDynamicFileListTreeView;
	
	if (!m_DynaTreeView->Create (m_hWnd, 0, 0, 0, 0))
		return false;

	m_DynaTreeView->AddItem ("Level 1");
	m_DynaTreeView->AddItem ("Level 2");
	m_DynaTreeView->AddItem ("Level 3");
	m_DynaTreeView->AddItem ("Level 4");

//	m_MainTabView->AttachTab (m_DynaTreeView->GetHwnd(), 1);

	/*
	GetClientRect (g_hAppWnd, &rect);

	g_Splitters[0] = new CSplitterWnd;

	

	if (!g_Splitters[0]->Create (g_hAppInst, g_hAppWnd, rect, 60, CS_VERTICAL))
		MessageBox (NULL, "SplitterWnd failed", "bioya", MB_OK);
	
	g_Splitters[0]->AttachPanel (g_TreeView->GetHwnd(),0);	

	HWND hwndTemp = CreateWindow ("Static", 
		"Static 1",
		WS_CHILD | WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_hAppWnd, NULL,
		g_hAppInst,
		NULL);

	g_Splitters[0]->AttachPanel (hwndTemp, 1);

	g_Splitters[1] = new CSplitterWnd;
	g_Splitters[1]->Create (g_hAppInst, g_Splitters[0]->GetHwnd(), rect, 60, CS_HORIZONTAL);



	HWND hwndTemp2 = CreateWindow ("Static", 
		"Static 2",
		WS_CHILD | WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_hAppWnd, NULL,
		g_hAppInst,
		NULL);

	HWND hwndTemp3 = CreateWindow ("Static", 
		"Static 3",
		WS_CHILD | WS_BORDER,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		g_hAppWnd, NULL,
		g_hAppInst,
		NULL);

	g_Splitters[1]->AttachPanel (hwndTemp2,0);	

	g_Splitters[1]->AttachPanel (hwndTemp3,1);


	g_TabView->AttachTab (g_Splitters[0]->GetHwnd(), 1);
	g_TabView->AttachTab (g_Splitters[1]->GetHwnd(), 2);

	ShowWindow (hwndTemp, SW_SHOW);
	ShowWindow (hwndTemp2, SW_SHOW);
	ShowWindow (hwndTemp3, SW_SHOW);

	ShowWindow (hwndStatus, SW_SHOW);
	ShowWindow (hwndTemp5, SW_SHOW);

	g_TabView->SetTab (0);

  */
	m_MainTabView->SetTab (0);
	return true;
}


bool CPrepDoc::CreateListViews()
{
	m_CommandListView = new CCommandListView;

	if (!m_CommandListView->Create(m_hInst, m_hWnd))
			return FALSE;

	m_CommandListView->CreateImageList();
	m_CommandListView->AddImageList();

	m_CommandListView->AddColumn ("Param 8",200);
	m_CommandListView->AddColumn ("Param 7",200);
	m_CommandListView->AddColumn ("Param 6",200);
	m_CommandListView->AddColumn ("Param 5",200);
	m_CommandListView->AddColumn ("Param 4",200);
	m_CommandListView->AddColumn ("Param 3",200);
	m_CommandListView->AddColumn ("Param 2",200);
	m_CommandListView->AddColumn ("Param 1",200);

	m_CommandListView->AddColumn ("OS",100);
	m_CommandListView->AddColumn ("Group",55);
	m_CommandListView->AddColumn ("Command",152);
	m_CommandListView->AddColumn ("ID", 30);


	/*m_StringListView = new CCommandListView;

	if (!m_StringListView->Create(m_hInst, m_hWnd))
			return FALSE;

	m_StringListView->CreateImageList();
	m_StringListView->AddImageList();
	m_StringListView->AddColumn ("String Value",200);
	m_StringListView->AddColumn ("String No.",200);
	m_StringListView->AddColumn ("Command",100);
	m_StringListView->AddColumn ("ID", 30);
*/


	return true;
}


void upstring (string& s)
{
	string::iterator p  = s.begin();

	while (p != s.end() )
	{
		*p = toupper (*p);
		p++;
	}
}


void upstring (char *s)
{
	char *p=s;

	while (*p)
	{
		*p = toupper (*p);
		p++;
	}
}


bool CPrepDoc::FillListView()
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	if (g_bCommandLine)
	{
		return true;
	}

	for (i=0;i<nCommands;i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);

		switch (pCmd->GetCommandType ())
		{
			case TOK_COMMENT:
				if (IsShowComments())
				{
					m_CommandListView->AddItem (pCmd);
				}
				break;
			case TOK_PROPERTY:
				if (IsShowProperty ())
				{
					m_CommandListView->AddItem (pCmd);
				}
				break;
			case TOK_BEGINFILELIST:
			case TOK_ENDFILELIST:
			case TOK_RULE:
			case TOK_BEGINSTRINGLIST:
			case TOK_ENDSTRINGLIST:
			case TOK_STRINGVAR:
			case TOK_ACTION:
				if (IsShowInternal())
				{
					m_CommandListView->AddItem (pCmd);
				}
				break;

			default:
				if ((pCmd->GetCommandType () != TOK_NEWLINE) & IsShowRegular())
				{
					m_CommandListView->AddItem (pCmd);
				}
				break;
		}
	}
	return true;
}


void CPrepDoc::UpdateListView ()
{
	if (g_bCommandLine)
	{
		return;
	}

	NukeListView();
	FillListView();
	GetCommandListView()->Refresh();
}


bool CPrepDoc::ValidateCommands()
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;
	DWORD dwFlags;

	for (i=0;i<nCommands;i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->IsValidToken())
		{
			dwFlags = pCmd->GetBuildFlags();
			if (dwFlags == 0)
			{
				return false;
			}
		}
	}
	return true;
}


bool CPrepDoc::NukeListView()
{
	if (g_bCommandLine)
	{
		return true;
	}

	m_CommandListView->DeleteAllItems();
	return true;
}


bool CPrepDoc::ReadFileList (void)
{
	int iPos = m_pSetupDoc->FindFirstToken (TOK_BEGINFILELIST);

	if (iPos == -1)
	{
		Alert (m_hWnd, MB_OK, STR_NO_UPDATE_FILELIST, g_szAppTitle, Keywords[TOK_BEGINFILELIST].pszKeyword);
		return false;
	}
	else
	{
		SetListDirtyState (true);

		m_pSetupDoc->SetCommandPtr (iPos + 2);
		m_pFileList->SetDirectory (m_pFileList->GetRootDirectory());
		if (!SetCurrentDirectory (GetSourcePath()))
		{
			Alert (m_hWnd, MB_OK, STR_SOURCEDIRINVALID, GetSourcePath(), g_szAppTitle);
			return false;
		}
		else
		{
			GetProgressDlg()->SetCurrentTask ("Reading file list...");
			m_pFileList->ReadFileList ("", "*.*", true);
			m_pFileList->SetDirectory (m_pFileList->GetRootDirectory());
			
			GetProgressDlg()->SetStatusText ("");
			GetProgressDlg()->SetRange (0, m_pFileList->GetDirectoryCount() + m_pFileList->GetFileCount());
			GetProgressDlg()->SetPos (0);

			AddDir (m_pFileList->GetRootDirectory());

			GetProgressDlg()->SetPos (0);
			GetProgressDlg()->SetStatusText ("");
			EbuYield();

			m_pFileList->SetDirectory (m_pFileList->GetRootDirectory());
			AddFile (m_pFileList->GetRootDirectory());
		}
		GetProgressDlg()->SetStatusText ("");
	}

	SetCurrentDirectory (m_pszProgramPath);
	return true;
}


bool CPrepDoc::UpdateStringList ()
{
	// find the string list section
	int iPos = m_pSetupDoc->FindFirstToken (TOK_BEGINSTRINGLIST);

	if (iPos == -1)
	{
		Alert (m_hWnd, MB_OK, STR_NO_UPDATE_STRINGLIST, g_szAppTitle, Keywords[TOK_BEGINSTRINGLIST].pszKeyword);
		return false;
	}
	else
	{
		SetListDirtyState (true);
		m_pSetupDoc->SetCommandPtr (iPos+2);
		for (int i=0; i < GetStringList()->GetListCount();i++)
		{
			if (GetStringList()->GetStringID(i) >= 100)
			{
				CCommand *pCmd = new CCommand;
				pCmd->SetCommandType (TOK_STRINGVAR);
				pCmd->SetCommandID (-2);
				pCmd->SetStringID (GetStringList()->GetStringID(i));
				pCmd->SetStringValue (GetStringList()->GetStringValue(i));

				m_pSetupDoc->InsertCommand (pCmd);
				InsertNewline();
			}
		}

	}
	return true;
}

bool CPrepDoc::UpdateStaticStringList ()
{
	// find the string list section
	int iPos = m_pSetupDoc->FindFirstToken (TOK_BEGINSTATICSTRINGLIST);

	if (iPos == -1)
	{
		Alert (m_hWnd, MB_OK, STR_NO_UPDATE_STATICSTRINGLIST, g_szAppTitle, Keywords[TOK_BEGINSTATICSTRINGLIST].pszKeyword);
		return false;
	}
	else
	{
		SetListDirtyState (true);
		m_pSetupDoc->SetCommandPtr (iPos+2);
		for (int i=0; i < GetStringList()->GetListCount();i++)
		{
			if (GetStringList()->GetStringID(i) < 100)
			{
				CCommand *pCmd = new CCommand;
				pCmd->SetCommandType (TOK_STRINGVAR);
				pCmd->SetCommandID (-2);
				pCmd->SetStringID (GetStringList()->GetStringID(i));
				pCmd->SetStringValue (GetStringList()->GetStringValue(i));

				m_pSetupDoc->InsertCommand (pCmd);
				InsertNewline();
			}
		}

	}
	return true;
}



bool CPrepDoc::ClearDynamicStringList ()
{
	CStringList *sl = GetStringList();
	int i=0;

	// walk the string list and remove any string that is dyanmic
	while (i < sl->GetListCount ())
	{
		if (sl->GetStringID(i) >= BASE_DYNAMIC_STRING_ID)
		{
			sl->DeleteString (i);
		}
		else
		{
			i++;
		}
	}

	return true;
}


bool CPrepDoc::ClearStaticStringList ()
{
	CStringList *sl = GetStringList();
	int i=0;

	// walk the string list and remove any string that is dyanmic
	while (i < sl->GetListCount ())
	{
		if (sl->GetStringID(i) < BASE_DYNAMIC_STRING_ID)
		{
			sl->DeleteString (i);
		}
		else
		{
			i++;
		}
	}

	return true;
}


void StrToUpper (string& s)
{
	string::iterator p=s.begin();

	while (p!=s.end())
	{
		*p = toupper (*p);
		p++;
	}
}


bool CPrepDoc::AddFile (CDirectoryEntry *pDir)
{
	#define MAX_DIGITS 2
	int nPrefixOfs = lstrlen (DISKPREFIX);
	static int cFile=0;
	string buffer;
	ENTRYLIST::iterator i;
	static int iLevel=0;
	CCommand *pCmd;
	char szFile [MAX_PATH*2];
	char szString [MAX_PATH*2];
	bool bExcludeFile = false;
	bool bUseHistory = false;

	for (i=pDir->GetDirectoryList()->begin(); i != pDir->GetDirectoryList()->end(); ++i)
	{

		++cFile;
		EbuYield();

		bExcludeFile = false;
		bUseHistory = false;
		if ((*i)->GetType() == FE_FILE)	
		{
			buffer = pDir->GetLongPath();
			buffer += (*i)->GetLongName();

			GetProgressDlg()->SetStatusText (buffer.c_str());
			GetProgressDlg()->SetPos (cFile);

			pCmd = new CCommand;

			pCmd->SetCommandType (TOK_INSTALLLIST);
			pCmd->SetCommandID (-2);

			if ( strnicmp (buffer.c_str(), DISKPREFIX, nPrefixOfs) == 0)
			{
				// we might have a multi disk structure here.
				char szNum[MAX_DIGITS];
				char *pszNum = szNum;
				const char *pszDir = buffer.c_str();

				// look for the end of the directory name or end of string
				while (*pszDir && *pszDir != '\\') 
				{
					pszDir++; 
				}

				if (*pszDir != '\0')
				{
					// we know there is a directory that has the prefix "DISKPREFIX", but
					// we don't know if it is of the multidisk format DISK##

					const char *pszPath = buffer.c_str() + nPrefixOfs;

					// read digits of "disk" number, up to MAX_DIGITS
					while (*pszPath && isdigit (*pszPath) && (pszNum-szNum < MAX_DIGITS))
					{
						*(pszNum++) = *(pszPath++);
					}

					*pszNum = '\0';

					// if the user specified too many or no digits, just treat it as a filename
					
					if (!isdigit (*pszPath) && *szNum != '\0')
					{
						// we have a properly formatted Multidisk directory.
						pCmd->SetDiskId (atoi (szNum)-1);
						lstrcpy (szFile, pszPath+1);
						buffer = szFile;

						// back propagate the disk ID to the file list
						static_cast<CFileEntry *>(*i)->AttachUserData (pCmd->GetDiskId());
					}
				}
			}

			pCmd->SetSourceName(buffer.c_str());
			pCmd->SetDestName(buffer.c_str());
			pCmd->SetUninstallFileFlag();

			StrToUpper (buffer);

			RULE *pRule;
			int iRule = -1;
			int iRuleCount = 0;

			char *pszPattern;
			bool bDeparent = false;
			bool bLocalize = false;

			while ((iRule = GetFileRules()->FindRule (iRule + 1, buffer.c_str())) != -1)
			{
				pRule = GetFileRules()->GetRule (iRule);

				if (pRule)
				{
					switch (pRule->dwAction)
					{
						case RULE_DEPARENT_DIR:
							pszPattern = pRule->pszPattern;

							if (pRule->dwOSFlags == 0 && pRule->cGroup == 0 && pRule->dwInstallFlags == 0)
							{
								// if the user didn't specify any flags, then we want to apply any history,
								// or program defaults later.
								bUseHistory = true;
							}
							else
							{
								pCmd->SetBuildFlags (pRule->dwOSFlags);
								pCmd->SetGroup (pRule->cGroup);
								pCmd->SetInstallFlags (pRule->dwInstallFlags);
							}

							// set a flag and defer til later since we may get multiple rules
							// applied to the same file. in this case it is important to 
							// process them in a certain order.
							bDeparent = true;
						break;

						case RULE_LOCALIZE_STRING:

							if (pRule->dwOSFlags == 0 && pRule->cGroup == 0 && pRule->dwInstallFlags == 0)
							{
								// if the user didn't specify any flags, then we want to apply any history,
								// or program defaults later.
								bUseHistory = true;
							}
							else
							{
								pCmd->SetBuildFlags (pRule->dwOSFlags);
								pCmd->SetGroup (pRule->cGroup);
								pCmd->SetInstallFlags (pRule->dwInstallFlags);
							}

							bLocalize = true;
							// set a flag and defer til later. (see description in RULE_DEPARENT_DIR)
							break;

						case RULE_EXCLUDE:
							bExcludeFile = true;
							break;

						case RULE_DEFAULT:
							pCmd->SetBuildFlags (pRule->dwOSFlags);
							pCmd->SetGroup (pRule->cGroup);
							pCmd->SetInstallFlags (pRule->dwInstallFlags);
							break;
					}
					++iRuleCount;
				}
			}

			//
			// deparent if necessary
			//

			if (bDeparent)
			{
				int len = lstrlen (pszPattern);
				char szFile[MAX_PATH*2];
				lstrcpy (szFile, buffer.c_str());
				
				if (szFile[len-1] == '\\')
				{
					memmove (szFile, szFile+len,lstrlen (szFile)-len+1);
					buffer = szFile;
					pCmd->SetDestName (buffer.c_str());
				}
			}

			//
			// localize
			//

			if (bLocalize)
			{
				string sPath = GetPathFromFileName (buffer);
				string sFile = GetFileNameFromPath (buffer);

				sPath = "%APPPATH\\" + sPath;

				ConvertToValidPath (sPath);

				GetStringList()->InsertString (GetStringList()->FindInsertionPoint (m_nStringID), m_nStringID, sFile.c_str()); 

				wsprintf (szString, "%%STRING%d", m_nStringID++);
				
				buffer = sPath + szString;

				pCmd->SetDestName (buffer.c_str());
			}

			if (iRuleCount == 0 || bUseHistory)
			{
				HIST *pHist = GetFileHist()->FindHist (pCmd->GetSourceName());

				if (pHist)
				{
					pCmd->SetBuildFlags (pHist->dwOSFlags);
					pCmd->SetGroup (pHist->cGroup);
					pCmd->SetInstallFlags (pHist->dwInstallFlags);
				}
				else
				{
					//bugbug::no rule found. set program defaults here....
					// using temp hard coded values for now.
					pCmd->SetUninstallFileFlag();
					pCmd->SetAppDirFlag();
					pCmd->SetBuildFlags (OS_WIN95 | OS_WIN98 | OS_NT40 | OS_NT50);
					pCmd->SetGroup (0x1);
					//pCmd->SetDiskId(0);
				}
			}
	
			if (!bExcludeFile)
			{
				m_pSetupDoc->InsertCommand (pCmd);
				InsertNewline();
			}
		}

		if (!bExcludeFile)
		{
			if ((*i)->GetType() == FE_DIR) 
			{
				AddFile (static_cast<CDirectoryEntry *>(*i));
			}
		}
	}
	return true;
}


bool CPrepDoc::AddDir (CDirectoryEntry *pDir)
{
	#define MAX_DIGITS 2
	int nPrefixOfs = lstrlen (DISKPREFIX);
	static int cDir = 0;
	string buffer;
	ENTRYLIST::iterator i;
	static int iLevel=0;
	CCommand *pCmd;
	bool bExcludeDirectory = false;
	bool bUseHistory = false;
	char szDir [MAX_PATH*2];

	for (i = pDir->GetDirectoryList()->begin(); i != pDir->GetDirectoryList()->end(); ++i)
	{
		++cDir;
		EbuYield();
		bExcludeDirectory = false;
		bUseHistory = false;

		if ((*i)->GetType() == FE_DIR)	
		{
			buffer = pDir->GetLongPath();
			buffer += (*i)->GetLongName();

			GetProgressDlg()->SetStatusText (buffer.c_str());
			GetProgressDlg()->SetPos(cDir);


			if ( strnicmp (buffer.c_str(), DISKPREFIX, nPrefixOfs) == 0)
			{
				// we might have a multi disk structure here.

				char szNum[MAX_DIGITS];
				char *pszNum = szNum;
				const char *pszDir = buffer.c_str();

				// look for the end of the directory name or end of string
				while (*pszDir && *pszDir != '\\') 
				{
					pszDir++; 
				}

				// we know there is a directory that has the prefix "DISKPREFIX", but
				// we don't know if it is of the multidisk format DISK##

				const char *pszPath = buffer.c_str() + nPrefixOfs;

				// read digits of "disk" number, up to MAX_DIGITS
				while (*pszPath && isdigit (*pszPath) && (pszNum-szNum < MAX_DIGITS))
				{
					*(pszNum++) = *(pszPath++);
				}

				*pszNum = '\0';

				// if the user specified too many or no digits, just treat it as a dir
				if (isdigit (*pszPath) || *szNum == '\0')
				{
					lstrcpy (szDir, buffer.c_str());
					buffer = szDir;
				}
				else
				{
					// we have a properly formatted Multidisk directory.
					if (*pszPath == '\\')
					{
						lstrcpy (szDir, pszPath+1);
						buffer = szDir;
					}
					else
					{
						AddDir (static_cast<CDirectoryEntry *>(*i));
						continue;
					}
				}
			}

			pCmd = new CCommand;
			
			pCmd->SetCommandType (TOK_MKDIR);
			pCmd->SetCommandID (-2);
			pCmd->SetMkDirValue (buffer.c_str());
			pCmd->SetUninstallFileFlag();
			pCmd->SetAppDirFlag();

			StrToUpper (buffer);

			RULE *pRule;
			int iRule = -1;
			int iRuleCount = 0;
			bool bDeparent = false;
			char *pszPattern;

			while ((iRule = GetFileRules()->FindRule (iRule+1, buffer.c_str())) != -1)
			{
				pRule = GetFileRules()->GetRule (iRule);

				if (pRule)
				{
					switch (pRule->dwAction)
					{
						case RULE_DEPARENT_DIR:
							if (pRule->dwOSFlags == 0 && pRule->cGroup == 0 && pRule->dwInstallFlags == 0)
							{
								// if the user didn't specify any flags, then we want to apply any history,
								// or program defaults later.
								bUseHistory = true;
							}
							else
							{
								pCmd->SetBuildFlags (pRule->dwOSFlags);
								pCmd->SetDirGroup (pRule->cGroup);
								pCmd->SetInstallFlags (pRule->dwInstallFlags);
							}
							pszPattern = pRule->pszPattern;
							bDeparent = true;
						break;
						case RULE_DEFAULT:
							pCmd->SetBuildFlags (pRule->dwOSFlags);
							pCmd->SetDirGroup (pRule->cGroup);
							pCmd->SetInstallFlags (pRule->dwInstallFlags);
							break;
						case RULE_EXCLUDE:
							bExcludeDirectory = true;
							break;
					}
					++iRuleCount;
				}
			}

			if (iRuleCount == 0 || bUseHistory)
			{
				HIST *pHist = GetFileHist()->FindHist (buffer.c_str());
				if (pHist)
				{
					pCmd->SetBuildFlags (pHist->dwOSFlags);
					pCmd->SetDirGroup (pHist->cGroup);
					pCmd->SetInstallFlags (pHist->dwInstallFlags);
				}
				else
				{
					//bugbug::	no rule found. set program defaults here....
					//			using temp hard coded values for now.
					pCmd->SetBuildFlags (OS_WIN95 | OS_WIN98 | OS_NT40 | OS_NT50);
					pCmd->SetUninstallFileFlag ();
					pCmd->SetDirGroup (0x1);
				}
			}

			if (!bExcludeDirectory)
			{
				if (bDeparent) 
				{
					int len = lstrlen (pszPattern);
					char szFile[MAX_PATH*2];
					lstrcpy (szFile, buffer.c_str());
					
					if (szFile[len-1] == '\\' || szFile[len-1] == '\0')
					{
						int newlen = lstrlen (szFile)-len+1;
						memmove (szFile, szFile+len, newlen);
						szFile[newlen]='\0';
						buffer = szFile;
						pCmd->SetMkDirValue (buffer.c_str());
					}
				}

				if (!buffer.empty())
				{
					m_pSetupDoc->InsertCommand (pCmd);
					InsertNewline();
				}
				else
				{
					delete pCmd;
					pCmd=NULL;
				}
				
				AddDir (static_cast<CDirectoryEntry *>(*i));
			}
			else
			{
				delete pCmd;
				pCmd=NULL;
			}
		}
	}
	return true;
}


bool CPrepDoc::IsScriptMultiDisk()
{
	for (int i=0; i < m_pSetupDoc->GetNumCommands(); i++)
	{
		if (m_pSetupDoc->GetNthCommand(i)->GetDiskId() != -1)
		{
			return true;
		}
	}
	return false;
}

int CPrepDoc::FindMaxMultiDisk()
{
	int nMax = -1;

	for (int i=0; i < m_pSetupDoc->GetNumCommands(); i++)
	{
		CCommand *pCmd = m_pSetupDoc->GetNthCommand(i);

		if (pCmd->GetDiskId() != -1)
		{
			if (pCmd->GetDiskId() > nMax)
			{
				nMax = pCmd->GetDiskId();
			}
		}
	}
	m_nMaxDisk = nMax;
	return nMax;
}


bool CPrepDoc::ProcessFilelistForMultiDisk ()
{
	CCommand *pCmd;
	bool bUnique;
	bool bEndOfDisk;
	int nCurrentDisk;
	int iFile;
	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINFILELIST) + 1; 
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDFILELIST) - 1;
	
	if (iStart == 0 || iEnd == -2 || iEnd < iStart)
		return false;
	
	if (iStart == iEnd) 
		return true;

	// walk the 'static' section before the dynamic file list, and see if the
	// current diskID gets bumped up from Disk1 (id=0).

	nCurrentDisk = 0;

	for (iFile=0;iFile<iStart;iFile++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (iFile);
		if (pCmd->GetDiskId() > nCurrentDisk) 
		{
			nCurrentDisk = pCmd->GetDiskId();
		}
	}

	for (iFile=iStart+1; iFile<=iEnd; iFile++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (iFile);

		//
		// check for disk change
		//
		if (pCmd->GetDiskId() != DISK_NOT_SPECIFIED)
		{
			if (pCmd->GetDiskId() == nCurrentDisk)
			{
				// Not a disk change, still on current disk. In the end we only 
				// want to show changes, so set diskID back to NOT_SPECIFIED
				
				pCmd->SetDiskId(DISK_NOT_SPECIFIED);
			}
			else
			{
				// disk change.
				if (pCmd->GetDiskId() != DISK_NOT_SPECIFIED)
				{
					nCurrentDisk = pCmd->GetDiskId();
				}

				// check to see if this transition file is unqiue across all disks
	
				int iFirst = iFile;
				
				bEndOfDisk = false;

				do 
				{
					bUnique = true;

					for (int iCompareFile=0; iCompareFile < m_pSetupDoc->GetNumCommands(); iCompareFile++)
					{
						if (m_pSetupDoc->GetNthCommand(iCompareFile)->GetCommandType() == TOK_INSTALLLIST)
						{
							if (stricmp (m_pSetupDoc->GetNthCommand(iFile)->GetSourceName(), m_pSetupDoc->GetNthCommand(iCompareFile)->GetSourceName()) == 0)
							{
								if (iFile != iCompareFile)
								{
									bUnique = false;
									break;
								}
							}
						}
					}

					// if file was not unique, search for the next valid InstallList command in the 
					// current disk.

					if (!bUnique)
					{
						do 
						{
							if ((m_pSetupDoc->GetNthCommand(iFile+1)->GetDiskId() != DISK_NOT_SPECIFIED) &&
							(m_pSetupDoc->GetNthCommand(iFile+1)->GetDiskId() != nCurrentDisk))
							{
								bEndOfDisk = true;
							}

							m_pSetupDoc->GetNthCommand (iFile)->SetDiskId(DISK_NOT_SPECIFIED);

							iFile++;

						} while (!bEndOfDisk && (iFile <= iEnd) && (m_pSetupDoc->GetNthCommand(iFile)->GetCommandType() != TOK_INSTALLLIST));
					}
				} while (!bEndOfDisk && !bUnique && (iFile <= iEnd));

				if (!bUnique)
				{
					Alert (m_hWnd, MB_OK, STR_MULTIDISK_FILE_NOT_UNIQUE);
					return false;
				}
				else
				{
					if (iFirst != iFile)
					{
						// swap the unique file to the top 
						CCommand *pCmdTemp = m_pSetupDoc->GetNthCommand(iFirst);
						m_pSetupDoc->SetNthCommand (iFirst,  m_pSetupDoc->GetNthCommand (iFile));
						m_pSetupDoc->SetNthCommand (iFile, pCmdTemp);
						m_pSetupDoc->GetNthCommand(iFirst)->SetDiskId( nCurrentDisk );
					}
				}
				
			}
			
		}
	}
	return true;
}


typedef struct tagThreadData
{
	const char *pszScriptName;
	CSetupDoc *csd;
} THREADDATA;



unsigned int __stdcall BackgroundReadSetupScript (void *ptd)
{
	THREADDATA *td = (THREADDATA *)ptd;

	td->csd->EnableExcelWorkaround();

	BOOL bSuccess = td->csd->OpenFile (td->pszScriptName);
	if (bSuccess)
	{
		td->csd->ReNumberCommands();
		return true;
	}
	else
	{
		return false;
	}
}


bool CPrepDoc::LoadSetupScript (const char *szScriptName)
{
	DWORD dwExitCode;
	UINT ThreadID;
	THREADDATA td;

	if (NULL == szScriptName)
	{
		return false;
	}

	SetScriptName (szScriptName);

	// package up some information to send to the LoadScript thread

	td.csd = m_pSetupDoc;
	td.pszScriptName = szScriptName;

	SetStatusBarText (0, "Loading...");
	
	// start a thread to load the script
	HANDLE hThread = (HANDLE) _beginthreadex (NULL, 0, BackgroundReadSetupScript, (void *)&td, 0, &ThreadID);

	while (WAIT_OBJECT_0 != WaitForSingleObject (hThread, 0))
	{
		if (!g_bCommandLine)
		{
			EbuYield();
		}

		Sleep (20);
	}

	GetExitCodeThread (hThread, &dwExitCode);
	CloseHandle (hThread);
	
	return (dwExitCode?true:false);
}


unsigned int __stdcall BackgroundWriteSetupScript (void *ptd)
{
	THREADDATA *td = (THREADDATA *)ptd;
	BOOL bSuccess = td->csd->Write (td->pszScriptName);
	return bSuccess;
}


BOOL CPrepDoc::WriteSetupScript (const char* pszOutput, const char *szFilePath)
{
	DWORD dwExitCode;
	UINT ThreadID;
	THREADDATA td;

	// g_fFileError is a global that gets triggered if a files does not exist or cannot be opened
	// during the GetVersionInfo scan.  Subsequently during a file WRITE operation, the INSTALL GO
	// token writer will bail if it see this value as true to prevent garbage output.
	
	g_fFileError = FALSE;

	// calculate the file version info for all file related commands.
	if (!GetFileVersionInfo (szFilePath))
	{
		return false;
	}

	if (m_ProgressDlg)
	{
		m_ProgressDlg->SetCurrentTask("Saving file... Please Wait.");
	}


	// package up some information to send to the LoadScript thread

	td.csd = m_pSetupDoc;
	td.pszScriptName = pszOutput;
	
	// bugbug: if a file cannot be read during GetFileVersionInfo, this write will always fail.
	// possibly tell the user before the write that it is going to fail.  Currently
	// if you ignore all of the errors in a scan, it still says file save failed, leading
	// one to think that there is something else wrong.

	// filter out commands -- bugbug: is this still necessary now that we aren't actually filtering anymore?
	GetSetupDoc()->ReNumberCommands ();

	// start a thread to write the script
	HANDLE hThread = (HANDLE) _beginthreadex (NULL, 0, BackgroundWriteSetupScript, (void *)&td, 0, &ThreadID);

	while (WAIT_OBJECT_0 != WaitForSingleObject (hThread, 0))
	{
		EbuYield();
		Sleep (20);
	}

	// make all commands valid again
	GetSetupDoc()->ReNumberCommands ();

	GetExitCodeThread (hThread, &dwExitCode);
	CloseHandle (hThread);

	return (dwExitCode?true:false);
}


string GetPathFromFileName (string s)
{
	string::iterator p;
	
	if (s.empty()) return NULL;

	p = s.end();

	while ((p--) > s.begin())
	{
		if (*p == '\\') 
			return s.substr(0, p-s.begin());
	}	
	return "";
}


string GetFileNameFromPath (string s)
{
	string::iterator p;
	
	if (s.empty()) return NULL;

	p = s.end();

	while ((p--) > s.begin())
	{
		if (*p == '\\') 
			return s.substr(p-s.begin()+1, s.end()-p-1);
	}	
	return s;
}


void MakeStringISO (char *szString)
{
	char *p = szString;
	char ch;

	while ((ch=toupper(*(p))))
	{
		if (!((ch >= 'A' && ch <= 'Z') ||
			  (ch >= '0' && ch <= '9') ||
			  (ch == '_')  || 
			  (ch == ':')  || 
			  (ch == '\\') || 
			  (ch == '.')))	 
			*p = '_';
		else
			*p = ch;
		
		++p;
	}
}


void ConvertToValidPath (string& szPath)
{
	if (szPath.length() != 0) 
		if (szPath.at(szPath.length()-1) != '\\')
			szPath = szPath + '\\';
}


void ConvertToValidPath (char *szPath)
{
	int nLen = lstrlen (szPath);
	if (szPath[nLen-1] != '\\')
	{
		szPath[nLen++] = '\\';
		szPath[nLen] = '\0';
	}
}

void  _cdecl DS (char *str, ...)
{
	va_list marker;
	char buffer[128];
	char outstr[1024];
	
	return;
	strcpy (outstr,"");

	va_start (marker, str);
	while (*str != NULL) 
	{
		if (*str != '%')
		{
			strncat (outstr, str, 1);
		}
		else                 
	    {
			switch (*(++str)) {
				case 'l':
					++str;
					wsprintf (buffer, "%ld",va_arg(marker, int));
					strncat (outstr, buffer, strlen (buffer));
					break;
				case 'f':
					//float test;
					//test = va_arg(marker, float);
					sprintf (buffer, "%f", va_arg(marker, double));
					//wsprintf (buffer, "%f",va_arg(marker, float));
					strncat (outstr, buffer, strlen (buffer));
					break;
				case 'd':
					wsprintf (buffer, "%d",va_arg(marker, int));
					strncat (outstr, buffer, strlen (buffer));
					break;
				case 's': 
					if (marker != NULL)
						strcpy (buffer, va_arg(marker, char*));       
					else
						strcpy (buffer, "<null>");
					strncat (outstr, buffer, strlen (buffer));
					break;
				case 'c':      
					strncat (outstr, (char *)&va_arg(marker, int), 1);
					break;
				default: 
					strncat (outstr, str, 1);
					break;
			}
		}   
		str++;
	}
	OutputDebugString (outstr);
}



bool CPrepDoc::ReplicateFileList (HWND hWnd)
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	int nCurrentDisk = 0;
	char szShortFile [MAX_PATH];
	char szSourceShortPath [MAX_PATH];
	char szLongFilePath [MAX_PATH*3];
	char szDisk[12];

	string szFilePath;
	string szSourcePath;
	string szDirectory;

	CCommand *pCmd;

	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINFILELIST) + 1;
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDFILELIST) - 1;

	ASSERT (m_pszDropPath);
	ASSERT (m_pszSourcePath);
	ASSERT (m_pszReparentPath);

	CFileList *flReplicate = new CFileList();

	flReplicate->SetDirectory (flReplicate->GetRootDirectory());
	

	GetProgressDlg()->SetCurrentTask ("Generating ISO 9660 compliant file list...");
	GetProgressDlg()->SetRange (iStart,iEnd);
	GetProgressDlg()->SetGranularity(1);

	for (i=iStart; i <= iEnd; i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);

		// keep the UI updating
		EbuYield();

		// BUGBUG:  question. should we just replicate INSTALLLIST entries?
		//			also, should we only do files inside the dynamic area?
		if (pCmd->GetCommandType() == TOK_INSTALLLIST)
		{
			// get the long filename we want to convert to ISO/Short
			//szFilePath = pCmd->GetDestName();
			szFilePath = pCmd->GetSourceName();

			lstrcpy (szLongFilePath, szFilePath.c_str());
			ExpandSubstitutedStrings (szLongFilePath);
			szFilePath = szLongFilePath;

			// find out where the user is storing the source files
			szSourcePath = m_pszSourcePath;
			ConvertToValidPath (szSourcePath);

			// add on disk# if in multidisk

			if (pCmd->GetDiskId() != DISK_NOT_SPECIFIED)
			{
				nCurrentDisk = pCmd->GetDiskId();
			}

			if (m_nMaxDisk > DISK_NOT_SPECIFIED)
			{
				wsprintf (szDisk, "DISK%d\\", nCurrentDisk+1);
				szSourcePath += szDisk;
			}

			// using source file path and relative path on filename, 
			// get a fully qualified path to the filename.

			szSourcePath += GetPathFromFileName (szFilePath);
			ConvertToValidPath (szSourcePath);

			// Set the default directory to this path

			if (!SetCurrentDirectory (szSourcePath.c_str()))
			{
				ASSERT (FALSE);
				return false;
			}

			// don't convert it since collisions are not resolved
			// Ask windows what the short filename equiv is for our long file name
			if (1) // TSP (0 == GetShortPathName (GetFileNameFromPath(szFilePath).c_str(), szShortFile, MAX_PATH)) 
			{
				// could not get short path	
				lstrcpy (szShortFile, GetFileNameFromPath(szFilePath).c_str()); 
			}

			// switch to the source root path.
			if (!SetCurrentDirectory (m_pszSourcePath))
			{
				ASSERT (FALSE);
				return false;
			}

			// Ask windows what the short pathname is for the given directory
			if (1) // TSP 0 == GetShortPathName (GetPathFromFileName (szFilePath).c_str(), szSourceShortPath, MAX_PATH)) 
			{
				// could not get short path	
				lstrcpy (szSourceShortPath, GetPathFromFileName (szFilePath).c_str());
			}
			
			szDirectory = GetPathFromFileName(szFilePath); 
			
			// convert the SFN directory path to ISO
			// TSP MakeStringISO (szSourceShortPath);

			// convert the SFN filename to ISO
			// TSP MakeStringISO (szShortFile);


			// update the SetupDoc CCommand list with new ISO compliant filename.
			string szNew;
			szNew = GetReparentPath();
			ConvertToValidPath (szNew);
			szNew += szSourceShortPath;
			ConvertToValidPath (szNew);
			szNew += szShortFile;
			pCmd->SetSourceName (szNew.c_str());

			// add the path and file to the replicate file tree
			
			flReplicate->SetDirectory (GetReparentPath());

			//DialogBoxParam (m_hInst, MAKEINTRESOURCE(IDD_VIEWLIST), m_hWnd, (DLGPROC) ViewListDialogProc, (LPARAM)flReplicate);
			// create the path, storing the LFN and SFN path

			upstring (szDirectory);
			upstring (szSourceShortPath);

			flReplicate->CreatePath (szDirectory, szSourceShortPath);

			flReplicate->SetDirectory (szDirectory);
		
			// 
			// After creating a full path, we need to back propagate the current disk
			// number into the file list.  Since we might have just created multiple directories
			// in a full path, rather than just one, we need to walk back up through all parent 
			// directories until we get to the root, setting the disk# along the way.
			//

			CDirectoryEntry *pCurrentDir = flReplicate->GetCurrentDirectory();

			while (pCurrentDir != flReplicate->GetRootDirectory())
			{
				pCurrentDir->AttachUserData(nCurrentDisk+1);
				pCurrentDir = pCurrentDir->GetParent();
			}

			// change into the directory so we can create the file
			//flReplicate->SetDirectory (szDirectory);

			// create the file, storing the LFN and SFN filename
			flReplicate->CreateFile (GetFileNameFromPath(szFilePath), szShortFile);

			// attach the current diskID to the file for later reference during copy phase
			flReplicate->GetCurrentFile()->AttachUserData(nCurrentDisk+1);

			// reset the current filelist dir to the root.
			flReplicate->SetDirectory (flReplicate->GetRootDirectory());
		}
	}

	// Dump the file list and show user. (debug)
	// DialogBoxParam (m_hInst, MAKEINTRESOURCE(IDD_VIEWLIST), m_hWnd, (DLGPROC) ViewListDialogProc, (LPARAM)flReplicate);

	SetLastBuildType (PBT_REPLICATE);

	GetProgressDlg()->SetRange (0,flReplicate->GetFileCount()+flReplicate->GetDirectoryCount());
	GetProgressDlg()->SetGranularity(1);
	GetProgressDlg()->SetPos(0);
	GetProgressDlg()->SetCurrentTask ("Replicating files...");
	EbuYield();

	if (!CopyFiles(flReplicate->GetRootDirectory(), hWnd, true))
	{
		delete flReplicate;
		return false;
	}

	delete flReplicate;
	return true;
}


bool CreateFullPath (const char *szPath)
{
	int nCount = 0;
	char szDir[MAX_PATH*2] = "";
	char *pszDir = szDir;
	const char *pszPath = szPath;

	// 
	// Check to see if we have a UNC path. If we do, skip over the server and share name.
	//
	if (*pszPath && *pszPath == '\\' && *(pszPath+1) == '\\')
	{
		while (*pszPath && nCount < 4)
		{
			if (*pszPath == '\\')
			{
				++nCount;
			}

			*pszDir++ = *pszPath++;
		}
	}
	else
	// see if a driver letter is specified. If so, skip over "X:\"
	if (*pszPath && *(pszPath+1) == ':')
	{
		while (*pszPath != '\\' && *pszPath)
			*pszDir++ = *pszPath++;
	}

	//
	// now walk through the path creating the directories as we go, if needed.
	// if a directory cannot be created, return failure.
	//

	while (*pszPath)
	{
		*pszDir++ = *pszPath++;
		if (*pszPath == '\\' || !*pszPath)
		{
			*pszDir = '\0';
			if (!PathExists (szDir))
			{
				if (!CreateDirectory (szDir, NULL))
				{
					return false;
				}
			}
		}
	}
	return true;
}


bool CPrepDoc::CreateDirs (HWND hWnd)
{
	char szDisk[12];
	int iDisk;
	string str;
	string sFileName;
	string sSource;
	string sDest;
	string sDestPath;
	//int nMaxDisk = FindMaxMultiDisk();

	sDestPath = GetDropPath();
	ConvertToValidPath (sDestPath);

	//
	// Create the drop directory
	//

	if (PathExists (sDestPath.c_str()))
	{
		if (Alert(GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_DROPDIREXISTS, sDestPath.c_str()) == IDNO )
		{
			return false;
		}
	}
	else
	{
		if (!CreateFullPath (sDestPath.c_str()))
		{
			Alert(GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CREATEDROPDIRFAILED, sDestPath.c_str());
			return false;
		}
	}

	//
	// If this is a multidisk set, create all of the disk#'s, and reparent dir's.
	//

	if (m_nMaxDisk != DISK_NOT_SPECIFIED)
	{
		for (iDisk=0; iDisk <= m_nMaxDisk; iDisk++)
		{
			sDestPath = GetDropPath();
			ConvertToValidPath (sDestPath);

			wsprintf (szDisk, "DISK%d\\", iDisk+1);
			sDestPath += szDisk;

			// create the DISK#'s directories

			if (PathExists (sDestPath.c_str()))
			{
				if (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_DISKDIREXISTS, sDestPath.c_str()) == IDNO )
				{
					return false;
				}
			}
			else
			{
				if (!CreateFullPath (sDestPath.c_str()))
				{
					Alert(GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CREATEDISKDIRFAILED, sDestPath.c_str());
					return false;
				}
			}


			// now create the reparent paths within the DISK's (if one was specified)
			if (GetReparentPath() != NULL && *GetReparentPath())
			{
				sDestPath += GetReparentPath();
				ConvertToValidPath(sDestPath);

				if (PathExists (sDestPath.c_str()))
				{
					if (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_REPARENTEXISTS, sDestPath.c_str()) == IDNO )
					{
						return false;
					}
				}
				else
				{
					if (!CreateFullPath (sDestPath.c_str()))
					{
						Alert(GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CREATEREPARENTDIRFAILED, sDestPath.c_str());
						return false;
					}
				}
			}
		}
	}
	else
	{

		//
		// not a multidisk set, just create the single reparent directory.
		//

		if (GetReparentPath() != NULL && *GetReparentPath())
		{
			sDestPath += GetReparentPath();
			ConvertToValidPath(sDestPath);

			if (PathExists (sDestPath.c_str()))
			{
				if (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_REPARENTEXISTS, sDestPath.c_str()) == IDNO )
				{
					return false;
				}
			}
			else
			{
				if (!CreateFullPath (sDestPath.c_str()))
				{
					Alert(GetProgressDlg()->GetHwnd(), MB_OK | MB_ICONEXCLAMATION, STR_CREATEREPARENTDIRFAILED, sDestPath.c_str());
					return false;
				}
			}
		}
	}

	return true;
}


bool CPrepDoc::CopyFiles (CDirectoryEntry *p_dir, HWND hWnd, bool bCreateDirsOnly)
{
	ENTRYLIST::iterator i;
	static int iLevel = 0;

	string str;
	string sFileName;
	string sSource;
	string sDest;
	string sDestPath;
	char buffer[255];

	sDestPath = GetDropPath();
	ConvertToValidPath (sDestPath);
	
	int iItem = 0;

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		GetProgressDlg()->Increment();

		if (GetProgressDlg()->IsCancelRequested())
		{
			if (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_CANCELREPLICATE) == IDYES )
			{
				GetProgressDlg()->SetCurrentTask ("Cancelled by user.");
				GetProgressDlg()->ClearCancelRequest();
				return false;
			}
			GetProgressDlg()->ClearCancelRequest();
		}
		
		EbuYield();

		char szDisk[12];
		sFileName = ((*i)->GetLongName());
		if ((*i)->GetType() == FE_FILE)	
		{
			sSource = GetSourcePath();

			if (m_nMaxDisk > DISK_NOT_SPECIFIED)
			{
				wsprintf (szDisk,"DISK%d\\", static_cast<CFileEntry *>(*i)->GetUserData());
				sSource += szDisk;
			}

			sSource += p_dir->GetLongPath() + (*i)->GetLongName();
		
			sDest = sDestPath;

			if (m_nMaxDisk > DISK_NOT_SPECIFIED)
			{
				sDest += szDisk;
			}

			sDest += GetReparentPath();
			ConvertToValidPath (sDest);
			sDest += p_dir->GetShortPath() + (*i)->GetShortName();
			
			wsprintf (buffer, "Copying: %s to %s", sSource.c_str(), sDest.c_str());
			GetProgressDlg()->SetStatusText (buffer);

		COPY_FILE:
			if (!CopyFile (sSource.c_str(), sDest.c_str(),  TRUE))
			{
				
				switch (Alert( GetProgressDlg()->GetHwnd(), MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, STR_CANNOTCOPYFILE, sSource.c_str(), sDest.c_str()))
				{
					case IDOK:
						GetProgressDlg()->SetCurrentTask ("Aborting replicate due to file copy error.");
						// in the commandline case, we just want to fail here
						return false;
					case IDABORT:
						GetProgressDlg()->SetCurrentTask ("Aborted.");
						return false;

					case IDRETRY:
						goto COPY_FILE;

					case IDIGNORE:
						break;
				}
			}
		}
		else
		{
			wsprintf (szDisk,"DISK%d\\", static_cast<CDirectoryEntry *>(*i)->GetUserData());

			sDest = sDestPath;

			if (m_nMaxDisk > DISK_NOT_SPECIFIED)
			{
				sDest += szDisk;
			}

			sDest += GetReparentPath();
			ConvertToValidPath (sDest);
				
			sDest += (static_cast<CDirectoryEntry *>(*i)->GetShortPath());
			ConvertToValidPath (sDest);
			wsprintf (buffer, "Creating Directory: %s", sDest.c_str());
			GetProgressDlg()->SetStatusText (buffer);

			if (!CreateDirectory (sDest.c_str(), NULL))
			{
				switch (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_CANNOTCREATEDIR, sDest.c_str()))
				{
					case IDNO:
						GetProgressDlg()->SetCurrentTask ("Cancelled by user.");
						return false;
					case IDOK:
						if (false==IsCreateDirNonFatal())
						{
							GetProgressDlg()->SetCurrentTask ("Aborting build due to directory creation error.");
							return false;
						}
						else
						{
							WriteLogFileMessage ("***** Warning! *****: Previous CreateDirectory failed, but was ignored due to commandline override.\n");
						}
						break;
				}

			}
		}
		
		if ((*i)->GetType() == FE_DIR)	
		{
			if (!CopyFiles (static_cast<CDirectoryEntry *>(*i), hWnd, bCreateDirsOnly))
			{
				return false;
			}
		}
	}
	return true;
}




bool CPrepDoc::CopyFiles (CDirectoryEntry *p_dir, char *szBaseSrcDir, char *szBaseDestDir)
{
	char szCurrentDir[MAX_PATH*2];
	ENTRYLIST::iterator i;

	string sFileName;
	string sSource;
	string sDest;
	string sDestPath;
	char buffer[255];

	// save the current directory
	GetCurrentDirectory (sizeof (szCurrentDir), szCurrentDir);

	// set the current directory to the base dir since the file list is relative to it.
	SetCurrentDirectory (szBaseSrcDir);

	sDestPath = szBaseDestDir;
	ConvertToValidPath (sDestPath);

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		EbuYield();

		sFileName = ((*i)->GetLongName());

		if ((*i)->GetType() == FE_FILE)	
		{
			sSource  = szBaseSrcDir; 
			ConvertToValidPath (sSource);
			sSource += p_dir->GetLongPath() + (*i)->GetLongName();
		
			sDest   = sDestPath;
			sDest  += p_dir->GetLongPath() + (*i)->GetLongName();
			
			wsprintf (buffer, "Replicating: %s to %s", sSource.c_str(), sDest.c_str());
			GetProgressDlg()->SetStatusText (buffer);
	
		COPY_FILE:
			if (!CopyFile (sSource.c_str(), sDest.c_str(),  TRUE))
			{
				switch (Alert( GetProgressDlg()->GetHwnd(), MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, STR_CANNOTCOPYFILE, sSource.c_str(), sDest.c_str()))
				{
					case IDOK:
						GetProgressDlg()->SetCurrentTask ("Aborting replicate due to file copy error.");
						// in the commandline case, we just want to fail here
						return false;
					case IDABORT:
						GetProgressDlg()->SetCurrentTask ("Aborted.");
						return false;

					case IDRETRY:
						goto COPY_FILE;

					case IDIGNORE:
						break;
				}
			}
		}
		else
		{
			sDest = sDestPath;
			sDest += (static_cast<CDirectoryEntry *>(*i)->GetShortPath());
			ConvertToValidPath (sDest);
			wsprintf (buffer, "Creating Directory: %s", sDest.c_str());
			GetProgressDlg()->SetStatusText (buffer);
			
			if (!CreateFullPath (sDest.c_str()))
			{
				switch (Alert( GetProgressDlg()->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_CANNOTCREATEDIR, sDest.c_str()))
				{
					case IDNO:
						GetProgressDlg()->SetCurrentTask ("Cancelled by user.");
						return false;
					case IDOK:
						if (false==IsCreateDirNonFatal())
						{
							GetProgressDlg()->SetCurrentTask ("Aborting build due to directory creation error.");
							return false;
						}
						else
						{
							WriteLogFileMessage ("***** Warning! *****: Previous CreateDirectory failed, but was ignored due to commandline override.\n");
						}
						break;
				}
			}
		}
		
		if ((*i)->GetType() == FE_DIR)	
		{
			if (!CopyFiles (static_cast<CDirectoryEntry *>(*i), szBaseSrcDir, szBaseDestDir))
			{
				return false;
			}
		}
	}
	return true;
}


bool CPrepDoc::ProcessFiles (ADDTOCABPROC fnCallback, CDirectoryEntry *p_dir, char *szBaseSrcDir, char *szBaseDestDir, DWORD dwUserData)
{
	ENTRYLIST::iterator i;
	string sFileName;
	string sSource;
	string sDest;
	string sDestPath;

	sDestPath = szBaseDestDir;
	ConvertToValidPath (sDestPath);

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		EbuYield();

		sFileName = ((*i)->GetLongName());

		if ((*i)->GetType() == FE_FILE)	
		{
			sSource  = szBaseSrcDir; 
			ConvertToValidPath (sSource);
			sSource += p_dir->GetLongPath() + (*i)->GetLongName();
		
			sDest   = sDestPath;
			sDest  += p_dir->GetLongPath() + (*i)->GetLongName();

			// call the user callback with the source and dest filename.
			(fnCallback)((char *)sSource.c_str(), (char *)sDest.c_str(), dwUserData);
		}
		
		if ((*i)->GetType() == FE_DIR)	
		{
			if (!ProcessFiles (fnCallback, static_cast<CDirectoryEntry *>(*i), szBaseSrcDir, szBaseDestDir, dwUserData))
			{
				return false;
			}
		}
	}
	return true;
}



DWORD ParseRuleAction (CCommand *pCmd)
{
	const char *pszRuleAction = pCmd->GetRuleAction();

	if (strcmpi (pszRuleAction, "default")==0)
		return RULE_DEFAULT;

	if (strcmpi (pszRuleAction, "exclude")==0)
		return RULE_EXCLUDE;

	if (strcmpi (pszRuleAction, "localize")==0)
		return RULE_LOCALIZE_STRING;

	if (strcmpi (pszRuleAction, "deparent")==0)
		return RULE_DEPARENT_DIR;

	Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_UNKNOWNRULEACTION, pszRuleAction);
	return 0;
}



RULE *CPrepDoc::CreateRule (CCommand *pCmd)
{
	char szBuffer[MAX_PATH*4];
	RULE *rule = new RULE;

	rule->dwOSFlags = pCmd->GetBuildFlags();
	rule->dwInstallFlags = pCmd->GetInstallFlags();
	rule->cGroup = pCmd->GetGroup();
	lstrcpy (szBuffer, pCmd->GetRulePattern());

	rule->dwAction = ParseRuleAction (pCmd);

	if (rule->dwAction == RULE_DEPARENT_DIR)
	{
		int len = lstrlen(szBuffer);

		// to make the Deparent rule work correctly we need the pattern format to
		// be DIRECTORY*, the following tests will fix up rule if
		// the user tries to specify ending \'s or wildcard * chars.
		// when comparing the files, prepstub will do a rough test on the directory
		// and then verify that it is DIRECTORY\ not just a directory that starts 
		// with the word DIRECTORY.
		
		if (szBuffer[len-1] == '*' && szBuffer[len-2] == '\\')
		{
			szBuffer[len-2] = '*';
			szBuffer[len-1] = '\0';
		}
		else
		if (szBuffer[len-1] == '\\')
		{
			szBuffer[len-1] = '*';
		}
		else
		{
			lstrcat (szBuffer, "*");
		}
	}

	ExpandSubstitutedStrings (szBuffer);
	
	rule->pszPattern = (char *)malloc (lstrlen (szBuffer)+1);
	lstrcpy (rule->pszPattern, szBuffer);
	_strupr (rule->pszPattern);

	return rule;
}

bool CPrepDoc::ExecuteAddToCabActions (ADDTOCABPROC fnCallback, DWORD dwUserData, bool bPrecopy)
{
	char szCurrentDir[MAX_PATH*2];
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	GetCurrentDirectory (sizeof (szCurrentDir),szCurrentDir);

	for (i=0; i < nCommands; i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);

		if ((pCmd->GetCommandType() == TOK_ACTION) && (pCmd->GetCabPreCopy() == bPrecopy))
		{
			if (strcmpi (pCmd->GetActionCommand(), "AddToCab")==0)
			{
				// param format: Replicate Wildcard [recurse]

				if (pCmd->GetActionParam1() == NULL)
				{
					Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADACTIONSYNTAX, pCmd->GetActionCommand());
					continue;
				}
				
				CFileList *FileList = new CFileList();
				FileList->SetDirectory (FileList->GetRootDirectory());
				SetCurrentDirectory (GetSourcePath());
				FileList->ReadFileList ("", pCmd->GetActionParam1(), pCmd->GetActionRecurseFlag());
				FileList->SetDirectory (FileList->GetRootDirectory());
				ProcessFiles (fnCallback, FileList->GetCurrentDirectory(), GetSourcePath(), "", dwUserData);
				delete FileList;
			}
			else
			if (strcmpi (pCmd->GetActionCommand(), "AddToCabExternal")==0)
			{
				if (pCmd->GetActionParam1() == NULL || pCmd->GetActionParam2() == NULL || pCmd->GetActionParam3()==NULL)
				{
					Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADACTIONSYNTAX, pCmd->GetActionCommand());
					continue;
				}

				// param format: ExternalReplicate Src Dest Wildcard [recurse]
				CFileList *FileList = new CFileList();
				FileList->SetDirectory (FileList->GetRootDirectory());
				string sSrc = pCmd->GetActionParam1();
				string sDest;
				ConvertToValidPath (sSrc);

				// change into the source directory so we can do everything relative.
				SetCurrentDirectory (pCmd->GetActionParam1());

				// bugbug: validate the parms here and pop up a warning.

				FileList->ReadFileList ("", pCmd->GetActionParam3(), pCmd->GetActionRecurseFlag());
				FileList->SetDirectory (FileList->GetRootDirectory());
				
				sDest = pCmd->GetActionParam2();
				ConvertToValidPath (sDest);

				// now that the file list is generated, process the files. ProcessFiles calls the user callback
				// on each file added.
				ProcessFiles (fnCallback, FileList->GetCurrentDirectory(), (char*)sSrc.c_str(), (char*)sDest.c_str(), dwUserData);
				delete FileList;
			}
		}
	}

	SetCurrentDirectory (szCurrentDir);
	return true;
}

bool CPrepDoc::ExecuteReplicateActions (void)
{
	char szCurrentDir[MAX_PATH*2];
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	GetCurrentDirectory (sizeof (szCurrentDir),szCurrentDir);

	for (i=0; i < nCommands; i++)
	{
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->GetCommandType() == TOK_ACTION)
		{
			if (strcmpi (pCmd->GetActionCommand(), "replicate")==0)
			{
				// param format: Replicate Wildcard [recurse]

				if (pCmd->GetActionParam1() == NULL)
				{
					Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADACTIONSYNTAX, pCmd->GetActionCommand());
					continue;
				}

				CFileList *FileList = new CFileList();
				FileList->SetDirectory (FileList->GetRootDirectory());
				SetCurrentDirectory (GetSourcePath());
				FileList->ReadFileList ("", pCmd->GetActionParam1(), pCmd->GetActionRecurseFlag());
				FileList->SetDirectory (FileList->GetRootDirectory());
				CopyFiles (FileList->GetCurrentDirectory(), GetSourcePath(), GetDropPath());
				delete FileList;
			}
			else
			if (strcmpi (pCmd->GetActionCommand(), "externalreplicate")==0)
			{
				// param format: ExternalReplicate Src Dest Wildcard [recurse]

				if (pCmd->GetActionParam1() == NULL || pCmd->GetActionParam2() == NULL || pCmd->GetActionParam3()==NULL)
				{
					Alert( g_hAppWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADACTIONSYNTAX, pCmd->GetActionCommand());
					continue;
				}

				CFileList *FileList = new CFileList();
				FileList->SetDirectory (FileList->GetRootDirectory());
				string sSrc = pCmd->GetActionParam1(), sDest = GetDropPath();

				ConvertToValidPath (sSrc);
				ConvertToValidPath (sDest);

				SetCurrentDirectory (pCmd->GetActionParam1());

				FileList->ReadFileList ("", pCmd->GetActionParam3(), pCmd->GetActionRecurseFlag());
				FileList->SetDirectory (FileList->GetRootDirectory());
				
				sDest += pCmd->GetActionParam2();
				ConvertToValidPath (sDest);

				CreateFullPath (sDest.c_str());
				CopyFiles (FileList->GetCurrentDirectory(), (char*)sSrc.c_str(), (char*)sDest.c_str());
				delete FileList;
			}
		}
	}

	SetCurrentDirectory (szCurrentDir);
	return true;
}


bool CPrepDoc::LoadRules ()
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	for (i=0; i < nCommands; i++)
	{
		EbuYield();
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->GetCommandType() == TOK_RULE)
		{
			if (!GetFileRules()->AttachRule (CreateRule (pCmd)))
			{
				return false;
			}
		}
	}
	return true;
}


HIST *CPrepDoc::CreateHist (CCommand *pCmd)
{
	HIST *hist = new HIST;

	hist->dwOSFlags = pCmd->GetBuildFlags();
	
	if (pCmd->GetCommandType() == TOK_MKDIR)
	{
		hist->cGroup = pCmd->GetDirGroup();
		hist->pszPattern = (char *)malloc (lstrlen (pCmd->GetDirName())+1);
		hist->dwInstallFlags = pCmd->GetInstallFlags();
		lstrcpy (hist->pszPattern, pCmd->GetDirName());
		_strupr (hist->pszPattern);
	}
	else
	if (pCmd->GetCommandType() == TOK_INSTALLLIST)
	{
		hist->cGroup = pCmd->GetGroup();
		hist->pszPattern = (char *)malloc (lstrlen (pCmd->GetSourceName())+1);
		hist->dwInstallFlags = pCmd->GetInstallFlags();
		lstrcpy (hist->pszPattern, pCmd->GetSourceName());
		_strupr (hist->pszPattern);
	}
	else
	{
		Alert( m_hWnd, MB_OK | MB_ICONEXCLAMATION, "CPrepDoc::CreateHist(), Error: Unknown Token");
		return NULL;
	}

	return hist;
}


bool CPrepDoc::LoadHistory ()
{
	CCommand *pCmd;

	// load previous file history info from the dynamic file list area.

	int iStart = m_pSetupDoc->FindFirstToken (TOK_BEGINFILELIST) + 1;
	int iEnd = m_pSetupDoc->FindFirstToken (TOK_ENDFILELIST) - 1;
	
	if (iStart == -1 || iEnd == -1)
		return false;

	for (int i=iStart;i<=iEnd;i++)
	{
		EbuYield();
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->GetCommandType() == TOK_INSTALLLIST || pCmd->GetCommandType() == TOK_MKDIR)
		{
			if (!GetFileHist()->AttachHist (CreateHist (pCmd)))
			{
				return false;
			}
		}
	}
	return true;
}


void CPrepDoc::ExpandSubstitutedStrings (char *pszPathname)
{
	char szTemp[MAX_PATH*2];
	char *pszPath = pszPathname;
	char *pszTmp = szTemp;

	while (*pszPath)
	{
		if (*pszPath == '%')
		{
			if (strnicmp ("%APPPATH", pszPath, 8) == 0)
			{
				// eat the apppath token
				pszPath +=8;

				// eat the \ if there is one. 
				if (*pszPath == '\\')
				{
					pszPath += 1;
				}
			}
			else
			if (strnicmp ("%STRING", pszPath, 7) == 0)
			{
				char szNum[10];
				char *pszNum = szNum;
				pszPath += 7;

				while (isdigit (*pszPath))
				{
					*(pszNum++) = *(pszPath++);
				}

				*pszNum = '\0';

				ASSERT (szNum);

				int nStringID = atoi (szNum);
				int nPos = GetStringList()->FindStringID(nStringID);

				if (-1 == nPos)
				{
					// string was not found
					char pszStringValue[14];
					wsprintf (pszStringValue, "STRING%%%d", nStringID);
					CopyMemory (pszTmp, pszStringValue, lstrlen (pszStringValue));
					pszTmp += lstrlen (pszStringValue);
					Alert( m_hWnd, MB_ICONSTOP | MB_OK, STR_STRINGNOTFOUND, nStringID);
				}
				else
				{
					char *pszStringValue = GetStringList()->GetStringValue (nPos);
					CopyMemory (pszTmp, pszStringValue, lstrlen (pszStringValue));
					pszTmp += lstrlen (pszStringValue);
				}
			}
			else
			{
				Alert( m_hWnd, MB_ICONSTOP | MB_OK, STR_BADSUBSTSTRING, pszPath);
				// We did not expect any other % substituted strings to appear in a source filename.
				// Feel free to add your expansion above.
				ASSERT (FALSE);	
			}
		}
		else
		{
			*(pszTmp++) = *(pszPath++);
		}
	}
	*pszTmp = '\0';

	lstrcpy (pszPathname, szTemp);
}


BOOL CPrepDoc::IsVerificationRequired (CCommand *pCmd)
{
	DWORD dwBuildFlags = GetBuildFlags ();
	DWORD dwCommandFlags = pCmd->GetBuildFlags();

	if (0 == (dwCommandFlags & (BLD_TYPEMASK | BLD_LANGMASK)))
	{
		return TRUE;
	}

	if (dwCommandFlags & BLD_RTL)
	{
		if (dwBuildFlags & BLD_RTL)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_OEM)
	{
		if (dwBuildFlags & BLD_OEM)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_DBCS)
	{
		if (dwBuildFlags & BLD_DBCS)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_APP1)
	{
		if (dwBuildFlags & BLD_APP1)
		{
			return TRUE;
		}
	}
	
	if (dwCommandFlags & BLD_APP2)
	{
		if (dwBuildFlags & BLD_APP2)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_APP3)
	{
		if (dwBuildFlags & BLD_APP3)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_USA)
	{
		if (dwBuildFlags & BLD_USA)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_JPN)
	{
		if (dwBuildFlags & BLD_JPN)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_GER)
	{
		if (dwBuildFlags & BLD_GER)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_FRA)
	{
		if (dwBuildFlags & BLD_FRA)
		{
			return TRUE;
		}
	}

	if (dwCommandFlags & BLD_SPA)
	{
		if (dwBuildFlags & BLD_SPA)
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CPrepDoc::GetSourceVersionInfo( CCommand *pCmd, LPCSTR lpszStubPath )
{
    char szPath[_MAX_PATH*4];
    UINT uResult;
    WORD wOS = GetCurrentOperatingSystem();
	FILEINFO fi;
	
    // build file path
    lstrcpy( szPath, lpszStubPath );
    lstrcat( szPath, pCmd->GetSourceName());

	if (false == IsIgnoreStrings())
	{
		if (_tcschr(szPath, '%'))
		{
			ExpandSubstitutedStrings (szPath);		
		}
	}


TryAgain:
    uResult = EBUFileInfo( szPath, &fi/*&m_FileInfo */);
	pCmd->SetFileInfo (&fi);
	
	//If there was an error AND if the first char of the source file was not a %
	//sign (indicating a substitution token) then report an error...

    if( uResult & FI_ERRORMASK )
    { 

		// if the FileInfo call failed and we are not localizing, then see if it has a subst token.
		// If it does, then zero out the file info and proceed as normal. (this is the old EbuPrepStub
		// way of doing things).  We still support this for old script compatibility.

		if (true == IsIgnoreStrings())
		{
			if (_tcschr(pCmd->GetSourceName(), '%'))
			{
				ZeroMemory(&fi, sizeof(FILEINFO));
				pCmd->SetFileInfo (&fi);
				return TRUE;
			}
		}

		// 
		if (!IsVerificationRequired (pCmd))
		{
			ZeroMemory(&fi, sizeof(FILEINFO));
			pCmd->SetFileInfo (&fi);
			return TRUE;
		}

        if( uResult & FI_ERR_NOEXIST )
        {
            int iErr = Alert( m_hWnd, MB_ICONSTOP | MB_ABORTRETRYIGNORE, STR_FILENOTFOUND, (LPCSTR)szPath );
			
			if ( iErr == IDIGNORE )
			{	
				g_fFileError = TRUE;
				return TRUE;
			}

			if ( iErr == IDRETRY )
			{
				goto TryAgain;
			}
        }
        else if( uResult & FI_ERR_CANTOPEN )
        {
            int iErr = Alert( m_hWnd, MB_ICONSTOP | MB_ABORTRETRYIGNORE, STR_CANTOPENSOURCE, (LPCSTR)szPath );
			if ( iErr == IDIGNORE )
			{	g_fFileError = TRUE;
			return TRUE;
			}
			if ( iErr == IDRETRY )
				goto TryAgain;
        }
        else
        {
            Alert( m_hWnd, MB_ICONSTOP | MB_OK, STR_NOMEMORY );
        }
		
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

bool CPrepDoc::GetFileVersionInfo(const char *szFilePath)
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	ETOKEN eToken;
	CCommand *pCmd;
	char szPath[_MAX_PATH];
	char buffer[MAX_PATH*2];
	char szFileName[MAX_PATH*2];
	char szDiskId[12];

	// get file version info for all tokens that have an associated file.

	m_ProgressDlg->SetCurrentTask ("Calculating file sizes...");


	m_ProgressDlg->SetRange (0,nCommands);
	m_ProgressDlg->SetGranularity (1);
	m_ProgressDlg->SetPos (0);
	
	g_nCurrentDiskID = 0;

	//
	// turn multidisk support if we find multidisk tokens in the script.
	//
	g_bDiskPaths = IsScriptMultiDisk()?true:false;

	for (i=0;i<nCommands;i++)
	{
		if (m_ProgressDlg->IsCancelRequested())
		{
			if (IDYES == Alert( m_ProgressDlg->GetHwnd(), MB_YESNO | MB_ICONEXCLAMATION, STR_GENERICABORT, g_szAppTitle))
			{
				m_ProgressDlg->SetCurrentTask ("");
				m_ProgressDlg->SetCurrentTask ("Cancelled by user.");
				m_ProgressDlg->SetStatusText ("");
				return false;
			}
			else
			{
				m_ProgressDlg->ClearCancelRequest();
			}
		}
		pCmd = m_pSetupDoc->GetNthCommand (i);
		eToken = pCmd->GetCommandType();

		m_ProgressDlg->Increment();
		EbuYield();

//		if (pCmd->IsIncludedInBuild (GetBuildFlags()))
		{
			if (eToken == TOK_INSTFONT || eToken == TOK_INSTALL || eToken == TOK_INSTALLLIST)
			{
				lstrcpy (szFileName, pCmd->GetSourceName ());

				if (false == IsIgnoreStrings())
				{
					ExpandSubstitutedStrings (szFileName);
				}

				wsprintf (buffer, "%s", szFileName);
				m_ProgressDlg->SetStatusText (buffer);

				lstrcpy( szPath, szFilePath );

				//
				// Only look in sub-directories when told to do so buy the prepstub user.
				// Otherwise process as before using the stubpath 
				//
			
				if (g_bDiskPaths)
				{
					if (pCmd->GetDiskId() != DISK_NOT_SPECIFIED && pCmd->GetDiskId() != g_nCurrentDiskID)
					{
						g_nCurrentDiskID = pCmd->GetDiskId();
					}
					// Using alternate paths
					lstrcat( szPath, CharNext(&(DISKPATH)));
					lstrcat( szPath, itoa((g_nCurrentDiskID + 1), szDiskId, 10));
					lstrcat(szPath, "\\");
				}
			
				//if (!pCmd->GetSourceVersionInfo( szPath))
				if (!GetSourceVersionInfo( pCmd, szPath ))
				{
					return false;
				}
			}
			else
			{
				//m_ProgressDlg->SetStatusText ("Skipping non-file command");
			}
		}
	}
	m_ProgressDlg->SetStatusText ("");
	return true;
}


bool CPrepDoc::LoadStringList ()
{
	int i, nCommands = m_pSetupDoc->GetNumCommands();
	CCommand *pCmd;

	CStringList *sl = GetStringList();

	// load all of the Strings from the script and place them in a string list.

	for (i=0;i<nCommands;i++)
	{
		EbuYield();
		pCmd = m_pSetupDoc->GetNthCommand (i);
		if (pCmd->GetCommandType() == TOK_STRINGVAR)
		{
			int nPos = sl->FindInsertionPoint (pCmd->GetStringID());

			if (sl->GetStringID(nPos) == pCmd->GetStringID())
			{
				// bugbug: possibly add a ignore all future string errors here.
				//		   if the user has a file with many collisons, it is very annoying to 
				//		   hit the OK button many times.

				// collison. Only allow one occurrence of a single string id.
				Alert( m_hWnd, MB_ICONSTOP, STR_DUPLICATESTRING, pCmd->GetStringID());
				continue;
			}

			// add the string to the list in sort order
			if (!sl->InsertString (nPos, pCmd->GetStringID(), pCmd->GetStringValue()))
			{
				return false;
			}
		}
	}
	return true;
}


bool CPrepDoc::LoadStaticStringsFromResourceDll()
{
	if (NULL != GetSetupDll())
	{
		if (DoesFileExist (GetSetupDll()))
		{
			ClearStaticStringList();

			if (!RemoveStaticStringList ())
			{
				CreateStaticStringSection();
			}

			LoadStringsFromDLL (GetSetupDll(), 500, 599, 0);
			UpdateStaticStringList ();
		//	UpdateListView();
		}
		else
		{
			Alert( m_hWnd, MB_OK | MB_ICONEXCLAMATION, STR_BADSETUPFILE, GetSetupDll());
			return false;
		}

	}
	return true;
}


bool CPrepDoc::LoadStringsFromDLL (const char *szModule, int nStart, int nEnd, int nBase)
{
	char szBuffer[MAX_PATH];

	HINSTANCE hInst = LoadLibrary (szModule);

	if (NULL == hInst)
	{
		return false;
	}

	for (int i=nStart; i<=nEnd; i++)
	{
		if (0 != LoadString (hInst, i, (LPSTR)&szBuffer, sizeof (szBuffer)))
		{
			GetStringList()->InsertString(GetStringList()->FindInsertionPoint (i-nStart+nBase),i-nStart+nBase, szBuffer);
		}
	}

	FreeLibrary (hInst);

	return true;
}


void CPrepDoc::DumpBuildParameters()
{
	// write what kind of build is being processed
	switch (GetPrepStubBuildType())
	{
		case BUILD_UPDATE_AND_INJECT:
			WriteLogFileMessage (STR_CL_BUILD_UPDATEANDINJECT);
			break;
		case BUILD_REPLICATE:
			WriteLogFileMessage (STR_CL_BUILD_REPLICATE);
			break;
		case BUILD_TRIAL:
			WriteLogFileMessage (STR_CL_BUILD_TRIAL);
			break;
		case BUILD_SAVEAS:
			WriteLogFileMessage (STR_CL_BUILD_SAVEASBINARY);
			break;
	}
}


void DumpIt (HWND hwnd, CDirectoryEntry *p_dir)
{
	int nItem;
	string str;
	ENTRYLIST::iterator i;
	static int iLevel = 0;

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		str = "";
			
		for (int iCount=0;iCount<iLevel;iCount++) 
		{
			str += "    ";
		}
			
		if ((*i)->GetType() == FE_FILE)	
		{
			str += "<file>";

//			char szDisk[12];

		//	wsprintf (szDisk, "Disk=%d,", (int)(*i)->GetUserData());

		//	str += szDisk; 

/*			if (static_cast<CFileEntry *>(*i)->GetUserData() != NULL)
			{
				//char buffer[10];
				//wsprintf (buffer, "<ref %d>", 
				//	*(static_cast<int *>((static_cast<CFileEntry *>(*i))->GetUserData()))
				//	);
				//str += buffer;
			}*/
		}
		else
		{
			str += "<dir> ";
		}

		str += ((*i)->GetLongName());

		str += "(" + ((*i)->GetShortName()) + ")";

		str += "<" + p_dir->GetLongPath() + " , " + p_dir->GetShortPath() + ">";

		
		nItem = SendMessage (hwnd, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR) str.c_str());
		SendMessage (GetDlgItem (hwnd,IDC_VIEWLIST), LB_SETCURSEL, (WPARAM)nItem, (LPARAM)0);

		if ((*i)->GetType() == FE_DIR)	
		{
			++iLevel;
			DumpIt (hwnd, static_cast<CDirectoryEntry *>(*i));
			--iLevel;
		}
	}
}


void DumpToFile (FILE *fp, CDirectoryEntry *p_dir)
{
//	int nItem;
	string str;
	ENTRYLIST::iterator i;
	static int iLevel = 0;

	for (int zz=0;zz<iLevel;zz++)
	{
		fprintf (fp, ">");
	}
	fprintf (fp, "[level=%d]\n");


	fprintf  (fp, "---> Beginning dir listing of [lfn=%s] [sfn=%s] [lfp=%s] [sfp=%s]\n", p_dir->GetLongName().c_str(), p_dir->GetShortName().c_str(), p_dir->GetLongPath().c_str(), p_dir->GetShortPath().c_str());

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		str = "";
			
		for (int iCount=0;iCount<iLevel;iCount++) 
		{
			str += " ";
		}

		if ((*i)->GetType() == FE_FILE)	
		{
			str += "<file>";

			//if (static_cast<CFileEntry *>(*i)->GetUserData() != NULL)
			//{
				//char buffer[10];
				//wsprintf (buffer, "<ref %d>", 
				//	*(static_cast<int *>((static_cast<CFileEntry *>(*i))->GetUserData()))
				//	);
				//str += buffer;
			//}
		}
		else
		{
			str += "<dir> ";
		}

		str += "[lfn=" + ((*i)->GetLongName()) + "]";

		str += " sfn=(" + ((*i)->GetShortName()) + ") ";

		str += " <lfp=" + p_dir->GetLongPath() + "> , <sfp=" + p_dir->GetShortPath() + ">\n";

		
		//nItem = SendMessage (hwnd, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR) str.c_str());
		fprintf (fp, str.c_str());
		//SendMessage (GetDlgItem (hwnd,IDC_VIEWLIST), LB_SETCURSEL, (WPARAM)nItem, (LPARAM)0);

		if ((*i)->GetType() == FE_DIR)
		{
			fprintf (fp, "{dropping into a sub directory}\n");
			++iLevel;
			DumpToFile (fp, static_cast<CDirectoryEntry *>(*i));
			--iLevel;
			fprintf (fp, "{popping back from a sub directory}\n");
		}
	}
	fprintf  (fp, "<--- End of dir listing of [lfn=%s] [sfn=%s] [lfp=%s] [sfp=%s]\n", p_dir->GetLongName().c_str(), p_dir->GetShortName().c_str(), p_dir->GetLongPath().c_str(), p_dir->GetShortPath().c_str());

}

void DumpToDebug (CDirectoryEntry *p_dir)
{
//	int nItem;
	string str;
	ENTRYLIST::iterator i;
	static int iLevel = 0;

	for (int zz=0;zz<iLevel;zz++)
	{
		DS (">");
	}
	DS ("[level=%d]\n",iLevel);


	DS ("---> Beginning dir listing of [lfn=%s] [sfn=%s] [lfp=%s] [sfp=%s]\n", p_dir->GetLongName().c_str(), p_dir->GetShortName().c_str(), p_dir->GetLongPath().c_str(), p_dir->GetShortPath().c_str());

	for (i=p_dir->GetDirectoryList()->begin(); i != p_dir->GetDirectoryList()->end(); ++i)
	{
		str = "";
			
		for (int iCount=0;iCount<iLevel;iCount++) 
		{
			str += " ";
		}

		if ((*i)->GetType() == FE_FILE)	
		{
			str += "<file>";

			//if (static_cast<CFileEntry *>(*i)->GetUserData() != NULL)
			{
				
			}
		}
		else
		{
			str += "<dir> ";
		}

		str += "[lfn=" + ((*i)->GetLongName()) + "]";

		str += " sfn=(" + ((*i)->GetShortName()) + ") ";

		str += " <lfp=" + p_dir->GetLongPath() + "> , <sfp=" + p_dir->GetShortPath() + ">\n";

		
		//nItem = SendMessage (hwnd, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)(LPCTSTR) str.c_str());
		DS ("%s",str.c_str());
		
		//SendMessage (GetDlgItem (hwnd,IDC_VIEWLIST), LB_SETCURSEL, (WPARAM)nItem, (LPARAM)0);

		if ((*i)->GetType() == FE_DIR)
		{
			DS ( "{dropping into a sub directory}\n");
			++iLevel;
			DumpToDebug(static_cast<CDirectoryEntry *>(*i));
			--iLevel;
			DS ("{popping back from a sub directory}\n");
		}
	}
	DS ("<--- End of dir listing of [lfn=%s] [sfn=%s] [lfp=%s] [sfp=%s]\n", p_dir->GetLongName().c_str(), p_dir->GetShortName().c_str(), p_dir->GetLongPath().c_str(), p_dir->GetShortPath().c_str());

}


BOOL CALLBACK ViewListDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			{
				CFileList *p_filelist = (CFileList *)lParam;	

				p_filelist->SetDirectory (p_filelist->GetRootDirectory());

				DumpIt (GetDlgItem (hWnd,IDC_VIEWLIST), p_filelist->GetCurrentDirectory());

				FILE *fp;
				fp = fopen ("c:\\dirlist.txt", "w+");
				DumpToFile (fp, p_filelist->GetCurrentDirectory());
				fclose (fp);

				return true;
			}
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDCANCEL:
				case IDOK:
					EndDialog (hWnd, 1);
					break;
				
			}
			return 0;
	}
	return 0;
}


LRESULT CALLBACK CDynamicFileListTreeView::ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_NOTIFY:
			{
				LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
				NM_TREEVIEW *pNm = (NM_TREEVIEW *)lParam;	

				switch (pNm->action)
				{
					case TVN_SELCHANGED:
					{	
						TV_ITEM *tv = &pNm->itemNew;
					}
					break;
				}
			}

		case WM_DESTROY:
			return 0;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}