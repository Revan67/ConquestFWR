/**************************************************************
* CPrepDoc.hpp: PrepStub98 Document class
*
* Chris N. Haddan
* March 23rd, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CPREPDOC_H
#define __CPREPDOC_H

#define BASE_DYNAMIC_STRING_ID				100		// implies 0-99 are reserved for static strings
#define MAX_DYNAMIC_STRINGS					400
#define BASE_RESOURCE_STATIC_STRING_ID		500 
#define BASE_RESOURCE_DYNAMIC_STRING_ID		600 

// build types
#define PBT_UNKNOWN			0
#define	PBT_UPDATE			1
#define	PBT_REPLICATE		2
#define PBT_TRIAL			3

#include "windows.h"
#include "CFileList.hpp"
#include "CFileRule.hpp"
#include "CFileHist.hpp"
#include "CStringList.hpp"
#include "CListView.hpp"
#include "CTreeView.hpp"
#include "CFrameWnd.hpp"
#include "CCommandListView.hpp"
#include "EditCommand.hpp"
#include "CTabView.hpp"
#include "CProgressDlg.hpp"
#include "SetupDoc.h"
#include "Command.h"
#include "resource.h"
#include "assert.h"
#include "script.h"
#include "util.h"
#include "string.h"
#include "diskinfo.h"
#include <tchar.h>


extern KEYWORD Keywords[];
extern HINSTANCE g_hAppInst;
extern HWND g_hAppWnd;
extern char	g_szAppTitle[256];
extern BOOL g_bDiskKeywordsFound;
extern BOOL g_bDiskPaths;
extern int g_nCurrentDiskID;
extern void UpdateWindowText();
extern BOOL	g_fFileError;
extern HCURSOR g_hAppCursor;
void MakeStringISO (char *szString);


typedef void (* ADDTOCABPROC)(char *, char*, DWORD);

class CPrepDoc
{
	public:
		CPrepDoc ();
		~CPrepDoc ();
		void DumpBuildParameters();
		bool WriteDiamondDirectiveFile (char *szDirectiveFile);
		bool SortDynamicAreaByCabFolder (void);
		bool ExecuteAddToCabActions (ADDTOCABPROC fnCallback, DWORD dwUserData, bool bPrecopy);
		bool ProcessFiles (ADDTOCABPROC fnCallback, CDirectoryEntry *p_dir, char *szBaseSrcDir, char *szBaseDestDir, DWORD dwUserData);
		bool ExecuteReplicateActions (void);
		void IncrementScriptBuildNumber (void);
		void IncrementInjectBuildNumber (void);
		bool ProcessFilelistForMultiDisk ();
		bool IsScriptMultiDisk();
		int  FindMaxMultiDisk();
		bool WriteStringsToRC3 (char *szRC3File, int nStartID, int nEndID, int nBaseID);
		bool LoadStaticStringsFromResourceDll();
		bool UpdateStaticStringList ();
		bool LoadStringsFromDLL (const char *szModule, int nStart, int nEnd, int nBase);
		BOOL IsVerificationRequired (CCommand *pCmd);
		bool ParseCommandLine (char *szCommandLine);
		bool RecreateRules();
		bool RecreateHist();
		void SetStatusBarText (int i, char *pszText);
		BOOL CreateProgressDlg();
		BOOL CreateProgressDlg(char *szLogFile);
		bool SaveDoc ();
		bool Create (HWND hAppWnd, HINSTANCE hAppInst, char *szCurrentPath);
		bool ReadAppSettings ();
		bool WriteAppSettings ();
		bool StripAppSettings ();
		bool InsertProperty (char *szProperty, char *szValue);
		bool InsertComment  (char *szComment);
		bool LoadSetupScript (const char *szFileName);
		//bool DoAutoNumber(const char *szScriptFile, const char *szTempFileName);
		bool InjectResourceFromFile (const char *szModule, const char *szType, const char *szName, const char *szBinary);
		bool InjectResource (const char *szModule, const char *szBinary);
		bool InjectStringTableInResource (const char *szModule);
		bool WriteStringGroupToResource (HANDLE hUpdateResource, int nStartID, int nEndID, int nBaseID);
		bool WriteStringGroupToResource (HANDLE hUpdateResource, CStringList *pStringList, int nStartID, int nEndID, int nBaseID);
		bool InjectVersionInfoInStringTable (const char *szModule);
		bool ClearFileList ();
		bool ReadFileList ();
		bool AddFile (CDirectoryEntry *pDir);
		bool AddDir  (CDirectoryEntry *pDir);
		bool ReplicateFileList (HWND hWnd);
		bool LoadRules ();
		bool LoadHistory ();
		bool LoadStringList ();
		bool ClearDynamicStringList ();
		bool ClearStaticStringList ();
		bool RemoveDynamicFileList ();
		bool RemoveDynamicStringList ();
		bool CreateStaticStringSection();
		bool RemoveStaticStringList ();
		bool CreateDynamicStringSection();
		bool InsertNewline ();
		bool InsertToken (ETOKEN eToken);
		void OnCommandDisplayInfo (LV_DISPINFO *pLvdi);
		bool CreateListViews();
		bool NewDoc();
		bool ValidateCommands();
		bool CopyFiles (CDirectoryEntry *p_dir, HWND hWnd, bool bCreateDirsOnly);
		bool CopyFiles (CDirectoryEntry *p_dir, char *szBaseSrcDir, char *szBaseDestDir);
		bool CreateDirs (HWND hWnd);
		RULE *CreateRule (CCommand *pCmd);
		HIST *CreateHist (CCommand *pCmd);
		CSetupDoc *GetSetupDoc () { return m_pSetupDoc; };
		CFileList *GetFileList () { return m_pFileList; };
		CFileRule *GetFileRules () { return m_pFileRule; };
		CFileHist *GetFileHist () { return m_pFileHist; };
		CStringList *GetStringList () { return m_pStringList; };
		bool FillListView();
		bool NukeListView();
		bool CreateUserInterface ();
		bool CloseDoc(bool bIgnoreUI);
		bool CheckForUnsavedDoc();
		void UpdateListView ();
		bool UpdateStringList ();
		bool GetFileVersionInfo(const char *szFilePath);
		BOOL WriteSetupScript (const char* pszOutput, const char *szFilePath);
		BOOL GetSourceVersionInfo( CCommand *pCmd, LPCSTR lpszStubPath );
		void ExpandSubstitutedStrings (char *szPath);
		CProgressDialog *GetProgressDlg () { return m_ProgressDlg; };
		void SetPrepStubBuildType (DWORD dwPrepStubBuildType)
			{ m_dwPrepStubBuildType = dwPrepStubBuildType; }
		DWORD GetPrepStubBuildType ()
			{	return m_dwPrepStubBuildType; }
		char* GetReparentPath ()
			{ return m_pszReparentPath; }
		char* GetSourcePath ()
			{ return m_pszSourcePath; }
		char* GetDropPath ()
			{ return m_pszDropPath; }
		char* GetProgramPath ()
			{ return m_pszProgramPath; }
		char* GetSetupExe ()
			{ return m_pszSetupExe; }
		char* GetSetupDll ()
			{ return m_pszSetupDll; }
		char* GetScriptName ()
			{ return m_pszScriptName; }
		char* GetCabFileName ()
			{ return m_pszCabFileName; }
		char* GetUninstallExe()
			{ return m_pszUninstallExe; }
		char* GetTrialExe()
			{ return m_pszTrialExe; }
		char* GetMakeCabExe()
			{ return m_pszMakeCabExe; }
		char* GetBinarySaveAsName()
			{ return m_pszBinarySaveAsName; }


		void SetBinarySaveAsName (const char *pszBinarySaveAsName)
			{
				if (pszBinarySaveAsName == m_pszBinarySaveAsName)
				return;

				if (m_pszBinarySaveAsName != NULL)
				{
					free (m_pszBinarySaveAsName);
					m_pszBinarySaveAsName = NULL;
				}
				m_pszBinarySaveAsName = (char *) malloc (lstrlen (pszBinarySaveAsName)+1);
				lstrcpy (m_pszBinarySaveAsName, pszBinarySaveAsName); 
			}



		void SetMakeCabExe (const char *pszMakeCabExe)
			{
				if (pszMakeCabExe == m_pszMakeCabExe)
				return;

				if (m_pszMakeCabExe != NULL)
				{
					free (m_pszMakeCabExe);
					m_pszMakeCabExe = NULL;
				}
				m_pszMakeCabExe = (char *) malloc (lstrlen (pszMakeCabExe)+1);
				lstrcpy (m_pszMakeCabExe, pszMakeCabExe); 
			}


		void SetTrialExe (const char *pszTrialExe)
			{
				if (pszTrialExe == m_pszTrialExe)
				return;

				if (m_pszTrialExe != NULL)
				{
					free (m_pszTrialExe);
					m_pszTrialExe = NULL;
				}
				m_pszTrialExe = (char *) malloc (lstrlen (pszTrialExe)+1);
				lstrcpy (m_pszTrialExe, pszTrialExe); 
			}

		void SetUninstallExe (const char *pszUninstallExe)
			{
				if (pszUninstallExe == m_pszUninstallExe)
				return;

				if (m_pszUninstallExe != NULL)
				{
					free (m_pszUninstallExe);
					m_pszUninstallExe = NULL;
				}
				m_pszUninstallExe = (char *) malloc (lstrlen (pszUninstallExe)+1);
				lstrcpy (m_pszUninstallExe, pszUninstallExe); 
			}


		void SetCabFileName (const char *pszCabFileName)
			{
				if (pszCabFileName == m_pszCabFileName)
				return;

				if (m_pszCabFileName != NULL)
				{
					free (m_pszCabFileName);
					m_pszCabFileName = NULL;
				}
				m_pszCabFileName = (char *) malloc (lstrlen (pszCabFileName)+1);
				lstrcpy (m_pszCabFileName, pszCabFileName); 
			}

		void SetScriptName (const char *pszScriptName)
			{	
				if (pszScriptName == m_pszScriptName)
					return;

				if (m_pszScriptName != NULL)
				{
					free (m_pszScriptName);
					m_pszScriptName = NULL;
				}
				m_pszScriptName = (char *) malloc (lstrlen (pszScriptName)+1);
				lstrcpy (m_pszScriptName, pszScriptName); 
			}
		void SetDropPath (const char *pszDropPath)
			{	
				if (m_pszDropPath != NULL)
				{
					free (m_pszDropPath);
					m_pszDropPath = NULL;
				}
				m_pszDropPath = (char *) malloc (lstrlen (pszDropPath)+1);
				lstrcpy (m_pszDropPath, pszDropPath); 
			}
		void SetProgramPath (const char *pszProgramPath)
			{	
				if (m_pszProgramPath != NULL)
				{
					free (m_pszProgramPath);
					m_pszProgramPath = NULL;
				}
				m_pszProgramPath = (char *) malloc (lstrlen (pszProgramPath)+1);
				lstrcpy (m_pszProgramPath, pszProgramPath); 
			}
		void SetSetupExe (const char *pszSetupExe)
			{	
				if (m_pszSetupExe != NULL)
				{
					free (m_pszSetupExe);
					m_pszSetupExe=NULL;
				}
				m_pszSetupExe = (char *) malloc (lstrlen (pszSetupExe)+1);
				lstrcpy (m_pszSetupExe, pszSetupExe); 
			}
		void SetSetupDll (const char *pszSetupDll)
			{	
				if (m_pszSetupDll != NULL)
				{
					free (m_pszSetupDll);
					m_pszSetupDll=NULL;
				}
				m_pszSetupDll = (char *) malloc (lstrlen (pszSetupDll)+1);
				lstrcpy (m_pszSetupDll, pszSetupDll); 
			}
		void SetSourcePath (const char *pszSourcePath)
			{	
				if (m_pszSourcePath != NULL)
				{
					free (m_pszSourcePath);
					m_pszSourcePath=NULL;
				}
				m_pszSourcePath = (char *) malloc (lstrlen (pszSourcePath)+1);
				lstrcpy (m_pszSourcePath, pszSourcePath); 
			}
		void SetReparentPath (const char *pszReparentPath)
			{	
				if (m_pszReparentPath != NULL)
				{
					free (m_pszReparentPath);
					m_pszReparentPath=NULL;
				}
				m_pszReparentPath = (char *) malloc (lstrlen (pszReparentPath)+1);
				lstrcpy (m_pszReparentPath, pszReparentPath); 
			}
		
		void SetHwnd (HWND hWnd) 
			{ m_hWnd = hWnd; }

		void SetHinst (HINSTANCE hInst) 
			{ m_hInst = hInst; }

		CListView *GetCommandListView ()
			{  return m_CommandListView; }

		CFrameWnd *GetMainFrame ()
			{ return m_MainAppFrameWnd; }

		bool IsListDirty ()
			{ return m_bListDirty; }

		void SetListDirtyState (bool bDirty)
			{ m_bListDirty = bDirty; }

		bool IsDocLoaded ()
			{ return m_bDocLoaded; }
		
		void SetDocLoadedState (bool bDocLoaded)
			{ m_bDocLoaded = bDocLoaded; }

		bool IsShowComments()
			{ return (m_bShowComments); }

		void SetShowCommentsState (bool bShowComments)
			{ m_bShowComments = bShowComments; }

		void SetShowProperty (bool bShowProperty)
			{ m_bShowProperty = bShowProperty; }

		bool IsShowProperty ()
			{ return (m_bShowProperty); }
	 
		bool IsShowInternal()
			{ return (m_bShowInternal); }

		void SetShowInternalState (bool bShowInternal)
			{ m_bShowInternal = bShowInternal; }

		bool IsShowRegular()
			{ return (m_bShowRegular); }

		void SetShowRegularState (bool bShowRegular)
			{ m_bShowRegular = bShowRegular; }

		bool IsShowInColor()
			{ return (m_bShowColor); }

		void SetShowInColorState (bool bShowColor)
			{ m_bShowColor = bShowColor; }

		void SetUseLongFileNamesOnly (bool bUseLongFileNamesOnly)
			{
				m_bUseLongFileNamesOnly = bUseLongFileNamesOnly;
			}

		bool IsUseLongFileNamesOnly ()
			{	return (m_bUseLongFileNamesOnly); }

		bool IsIncludeNoBuildFlagCommands ()
			{ return (m_bIncludeNoBuildFlagCommands); }

		void SetIncludeNoBuildFlagCommands (bool bIncludeNoBuildFlagCommands)
			{	
				m_bIncludeNoBuildFlagCommands = bIncludeNoBuildFlagCommands;
			}

		DWORD GetBuildFlags (void)
			{	return (m_dwBuildFlags); }

		void SetBuildFlags (DWORD dwBuildFlags)
			{
				m_dwBuildFlags = dwBuildFlags;
			}

		void SetLangID (DWORD dwLangID)
			{ m_dwLangID = dwLangID; }

		DWORD GetLangID (void)
			{ return m_dwLangID; }

		void SetEnableLocalization (bool bEnableLocalization)
			{	m_bEnableLocalization = bEnableLocalization; }

		bool IsEnableLocalization (void)
			{ return m_bEnableLocalization; }

		void SetReplicateFileTree (bool bReplicateFileTree)
			{	m_bReplicateFileTree = bReplicateFileTree; }

		bool IsReplicateFileTree (void)
			{ return m_bReplicateFileTree; }

		void SetInjectStaticStrings (bool bInjectStaticStrings)
			{ m_bInjectStaticStrings = bInjectStaticStrings; }

		void SetInjectDynamicStrings (bool bInjectDynamicStrings)
			{ m_bInjectDynamicStrings = bInjectDynamicStrings; }

		void SetInjectBinaryBlob (bool bInjectBinaryBlob)
			{ m_bInjectBinaryBlob = bInjectBinaryBlob; }

		bool IsInjectStaticStrings (void)
			{ return (m_bInjectStaticStrings); }

		bool IsInjectDynamicStrings (void)
			{ return (m_bInjectDynamicStrings); }

		bool IsInjectBinaryBlob (void)
			{ return (m_bInjectBinaryBlob); }

		bool IsLoadScriptStrings (void)
			{ return (m_bLoadScriptStrings); }

		bool IsLoadDllStrings (void)
			{ return (m_bLoadDllStrings); }

		bool IsIgnoreStrings (void)
			{ return (m_bIgnoreStrings); }

		void SetLoadScriptStrings (bool bLoadScriptStrings)
			{ m_bLoadScriptStrings = bLoadScriptStrings; }

		void SetLoadDllStrings (bool bLoadDllStrings)
			{ m_bLoadDllStrings = bLoadDllStrings; }

		void SetIgnoreStrings (bool bIgnoreStrings)
			{ m_bIgnoreStrings = bIgnoreStrings; }

		bool IsReparent ()
			{	return m_bReparent; }
		
		void SetCreateDirNonFatal (bool bCreateDirNonFatal)
			{ m_bCreateDirNonFatal = bCreateDirNonFatal; }

		bool IsCreateDirNonFatal ()
			{ return m_bCreateDirNonFatal; }

		bool IsUpdateFileList ()
			{ return m_bUpdateFileList; }

		void SetUpdateFileList (bool bUpdateFileList)
			{ m_bUpdateFileList = bUpdateFileList; }

		void GetSaveAsFileName(char *szFileName)
			{ lstrcpy (szFileName, (char *)&m_szSaveAsFileName); }

		void SetSaveAsFileName (char *szFileName)
			{ lstrcpy ((char*)&m_szSaveAsFileName, szFileName); }

		void SetLastBuildType (DWORD dwLastBuildType)
			{
				m_dwLastBuildType = dwLastBuildType;
			}

		void SetPreventBuildNumberUpdate (bool bPreventScriptBuildNumberUpdate)
			{
				m_bPreventScriptBuildNumberUpdate = bPreventScriptBuildNumberUpdate;
			}

		DWORD GetLastBuildType ()
			{ return m_dwLastBuildType; }

	private:
		char m_szScriptBuildNumber[14];
		char m_szInjectBuildNumber[14];
		char m_szSaveAsFileName[MAX_PATH*2];
		char *m_pszSetupExe;		// location of "setup.exe"
		char *m_pszSetupDll;		// location of "setupenu.dll"
		char *m_pszDropPath;		// destination path for replication
		char *m_pszProgramPath;		// the directory we start in.
		char *m_pszSourcePath;		// path to build source files
		char *m_pszScriptName;		// current setup script
		char *m_pszReparentPath;	// reparent directory
		char *m_pszAppName;			// name of the application
		char *m_pszBinarySaveAsName;// commandline save as binary filename

		char *m_pszCabFileName;		// name of cabinet file used in a Trial Version Setup
		char *m_pszUninstallExe;
		char *m_pszTrialExe;
		char *m_pszMakeCabExe;
		HWND m_hWnd;
		HINSTANCE m_hInst;

		CSetupDoc	*m_pSetupDoc;
		CFileList	*m_pFileList;
		CFileRule	*m_pFileRule;		// user defined regular expression rules.
		CFileHist	*m_pFileHist;		// previous dynamic filelist
		CStringList	*m_pStringList;		// String table

		CCommandListView	*m_CommandListView;
		CCommandListView	*m_StringListView;
		CFrameWnd	*m_MainAppFrameWnd;
		CTabView	*m_MainTabView;
		CTreeView	*m_DynaTreeView;

		CProgressDialog *m_ProgressDlg;
				
		HWND		m_StatusWindow;
		HWND		m_ToolBarWindow;

		bool	m_bListDirty;
		bool	m_bDocLoaded;

		bool	m_bShowComments;
		bool	m_bShowRegular;
		bool	m_bShowInternal;
		bool	m_bShowColor;
		bool	m_bShowProperty;

		bool	m_bUseLongFileNamesOnly;
		bool	m_bEnableLocalization;

		bool	m_bIncludeNoBuildFlagCommands;
		DWORD	m_dwBuildFlags;
		DWORD	m_dwLangID;

		int		m_nStringID;
		DWORD	m_dwPrepStubBuildType;

		bool	m_bReplicateFileTree;
		bool	m_bInjectStaticStrings;
		bool	m_bInjectDynamicStrings;
		bool	m_bInjectBinaryBlob;

		bool	m_bLoadScriptStrings;
		bool	m_bLoadDllStrings;
		bool	m_bIgnoreStrings;
		bool	m_bUpdateStaticStrings;
		bool	m_bReparent;
		bool	m_bUpdateFileList;

		bool	m_bCreateDirNonFatal; 

		DWORD	m_dwLastBuildType;
		int		m_nMaxDisk;

		bool	m_bPreventScriptBuildNumberUpdate;
};

// this should be moved out of here when it because a class we use.
class CDynamicFileListTreeView : public CTreeView
{
	public:
		CDynamicFileListTreeView () : CTreeView (){};
	private:
		virtual LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

string GetPathFromFileName (string s);
string GetFileNameFromPath (string s);

void ConvertToValidPath (char *szPath);

#endif