#ifndef CQGAME_H
#define CQGAME_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 CQGame.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/CQGame.h 26    6/22/01 2:09p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
/*
	CQGAME defines the structure for data that needs to be collected before
	starting a multiplayer game.

 */

#include <DCQGame.h>

//--------------------------------------------------------------------------//
//
struct FULLCQGAME : CQGAME
{
	wchar_t szMapName[MAPNAMESIZE];
	U32     localSlot;			// slot for this player (0 to MAX_PLAYERS-1)

	wchar_t szPlayerNames[MAX_PLAYERS][PLAYERNAMESIZE];
};
//--------------------------------------------------------------------------//
//  interface so that menus can set data items over the net
//
struct DACOM_NO_VTABLE ICQGame : FULLCQGAME
{
	virtual void SetMapName (const wchar_t * szName) = 0;

	virtual void SetType (U32 slot, CQGAMETYPES::TYPE type, bool bUpdate=false) = 0;		// call setStateInfo() if bUpdate is true

	virtual void SetCompChalange (U32 slot, CQGAMETYPES::COMP_CHALANGE compChalange, bool bUpdate = false) = 0; // call setStateInfo() if bUpdate is true

	virtual void SetState (U32 slot, CQGAMETYPES::STATE state, bool bUpdate=false) = 0;	// call setStateInfo() if bUpdate is true

	virtual void SetRace (U32 slot, CQGAMETYPES::RACE race, bool bUpdate=false) = 0;		// call setStateInfo() if bUpdate is true

	virtual void SetColor (U32 slot, CQGAMETYPES::COLOR color, bool bUpdate=false) = 0;	// call setStateInfo() if bUpdate is true

	virtual void SetTeam (U32 slot, CQGAMETYPES::TEAM team, bool bUpdate=false) = 0;		// call setStateInfo() if bUpdate is true

	virtual void SetGameType (CQGAMETYPES::GAMETYPE type) = 0;

	virtual void SetGameSpeed (S8 gameSpeed) = 0;

	virtual void SetMoney (CQGAMETYPES::MONEY money) = 0;

	virtual void SetMapType (CQGAMETYPES::MAPTYPE type) = 0;

	virtual void SetMapTemplateType (CQGAMETYPES::RANDOM_TEMPLATE rndTemp) = 0;

	virtual void SetMapSize (CQGAMETYPES::MAPSIZE size) = 0;

	virtual void SetTerrain (CQGAMETYPES::TERRAIN terrain) = 0;

	virtual void SetUnits (CQGAMETYPES::STARTING_UNITS units) = 0;

	virtual void SetResourceRegen (BOOL32 bRegen) = 0;
		
	virtual void SetSpectatorState (BOOL32 bSpectatorsOn) = 0;

	virtual void SetLockDiplomacy (BOOL32 bLockOn) = 0;

	virtual void SetVisibility (CQGAMETYPES::VISIBILITYMODE fog) = 0;

	virtual void SetNumSystems (U32 _numSystems) = 0;

	virtual void SetHostBusy (BOOL32 _bHostBusy) = 0;

	virtual void SetCommandLimit (CQGAMETYPES::COMMANDLIMIT commandSetting) = 0;

	virtual void SetDifficulty (CQGAMETYPES::DIFFICULTY difficulty, bool bUpdate=true) = 0;

	virtual CQGAMETYPES::DIFFICULTY GetDifficulty (void) = 0;

	virtual void ForceUpdate (void) = 0;
};











#endif