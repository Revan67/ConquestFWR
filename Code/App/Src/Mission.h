#ifndef MISSION_H
#define MISSION_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Mission.h                                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/Mission.h 52    5/02/01 4:28p Tmauer $
*/			    
//-------------------------------------------------------------------
/*
	Management of the overall game state
*/
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#ifndef DACOM_H
#include <DACOM.h>
#endif

struct DACOM_NO_VTABLE IMission : public IDAComponent
{
	virtual BOOL32 __stdcall New () = 0;

	virtual void __stdcall Close (BOOL32 bForceIt=1) = 0;

	virtual BOOL32 __stdcall Load (const C8 * fileName = 0, struct IComponentFactory * pParent = 0) = 0;
	
	virtual BOOL32 __stdcall Save (const C8 * fileName = 0, struct IComponentFactory * pParent = 0) = 0;

	virtual BOOL32 __stdcall LoadFromDescription (const wchar_t * fileDesc, struct IComponentFactory * pParent = 0) = 0;

	virtual BOOL32 __stdcall SaveByDescription (const wchar_t * fileDesc, struct IComponentFactory * pParent = 0, 
												const char * szOverwrite = 0, U32 saveKey = 0) = 0;

	virtual BOOL32 __stdcall LoadBriefing (const char * fileName) = 0;

	virtual BOOL32 __stdcall QuickSave (const C8 * fileName = 0, struct IComponentFactory * pParent = 0) = 0;

	virtual BOOL32 __stdcall QuickSaveFile (void) = 0;

	virtual BOOL32 __stdcall QuickLoadFile (void) = 0;

	virtual BOOL32 __stdcall SetUnsavedData (void) = 0;

	virtual GENRESULT __stdcall CorrelateSymbol (const C8 *pSymbolName, void *pOldData, void *pNewData) = 0;

	virtual void __stdcall Update (void) = 0;

	virtual void __stdcall SetWindowTitle (void) = 0;

	virtual void __stdcall StartNetDownload (void) = 0;

	virtual void __stdcall CancelNetDownload (void) = 0;

	virtual enum FTSTATUS __stdcall GetDownloadStatus (void) = 0;

	virtual GENRESULT __stdcall GetFileSystem (struct IFileSystem **ppFile) = 0;

	virtual void __stdcall SetFileSystem (struct IFileSystem * pFile) = 0;

	virtual BOOL32 __stdcall Reload (void) = 0;	// reload current file

	virtual BOOL32 __stdcall ReloadMission (void) = 0;

	virtual void __stdcall InitializeNetGame (const struct FULLCQGAME & cqgame, U32 randomSeed) = 0;

	virtual void LoadInterface (void) = 0;

	virtual void __stdcall GenerateMultiplayerMap (const struct FULLCQGAME & cqgame, U32 randomSeed) = 0;

	virtual void __stdcall EndMission (U32 allyMask, bool bNonWinLose = false) = 0;

	virtual U32 __stdcall GetNumAIPlayers (void) = 0;		// for accounting

	virtual void __stdcall SetAIResign (U32 playerID) = 0;	// used by player AI to resign

	virtual U32 __stdcall GetFileDescription (const C8 * fileName, wchar_t * string, U32 numMaxChars) = 0;

	virtual U32 __stdcall GetFileDescription (IFileSystem * fileDir, const C8 * fileName, wchar_t * string, U32 numMaxChars) = 0;

	virtual U32 __stdcall GetFileMaxPlayers (IFileSystem * fileDir, const C8 * fileName, U32 & maxPlayers) = 0;

	virtual U32 __stdcall GetFileMaxPlayers (const C8 * fileName, U32 & maxPlayers) = 0;

	// make a copy of current defaults state
	virtual void __stdcall SetInitialCheatState (void) = 0;

	// AI CONTROLLING METHODS, FOR MISSION SCRIPTING

	virtual void __stdcall EnableEnemyAI (U32 playerID, bool bOn, enum M_RACE race) = 0;

	virtual void __stdcall EnableEnemyAI (U32 playerID, bool bOn, const char * szPlayerAIType) = 0;

	virtual void __stdcall SetEnemyAIRules (U32 playerID, const struct AIPersonality & rules) = 0;

	virtual void __stdcall SetEnemyAITarget (U32 playerID, IBaseObject *obj, U32 range, U32 systemID = 0) = 0;

	virtual void __stdcall LaunchOffensive (U32 playerID, enum UNIT_STANCE stance) = 0;

	virtual bool __stdcall IsSinglePlayerGame (void) = 0;

	// returns TRUE even if there is a coop human player
	virtual bool __stdcall IsComputerControlled (U32 playerID) = 0;

	virtual void __stdcall SetFileDescription(const wchar_t * desc) = 0;

	virtual bool __stdcall IsMouseEnabled (void) = 0;

	virtual void __stdcall CancelProgressAnim (void) = 0;

	// only Menu_nl will call this function!!
	virtual void __stdcall StartProgressAnim (struct IPANIM ** ppAnim) = 0;

	// only called by Objlist::Load()
	virtual void __stdcall SetObjlistLoadProgress (SINGLE percentLoaded) = 0;

	virtual const enum M_RACE __stdcall GetSinglePlayerRace (void) const = 0;

	virtual void __stdcall SetSinglePlayerRace (const enum M_RACE race) = 0;

	virtual const struct CQGAME & GetGameSettings (void) = 0;

	virtual void __stdcall ToggleCheating (void) = 0;

	virtual const bool _stdcall GetCheatsEnabled (void) const = 0;

	// used by pause menu to display the color correctly
	virtual COLORREF __stdcall GetPlayerColorForDPID (U32 dpid) = 0;
};




#endif
