#ifndef IGAMEPROGRESS_H
#define IGAMEPROGRESS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IGameProgress.h                             //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IGameProgress.h 7     9/01/00 9:19p Sbarton $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IGameProgress : IDAComponent
{
	virtual U32  GetMissionsCompleted (void) = 0;

	virtual void SetMissionCompleted (U32 missionID) = 0;

	virtual void SetMissionsSeen (U32 missionID) = 0;

	virtual U32  GetMissionsSeen (void) = 0;

	virtual U32  GetMoviesSeen (void) = 0;
	
	virtual void SetMovieSeen (U32 movieID) = 0;

	virtual void ReInitialize (void) = 0;

	virtual void ForcedIntroMovie (void) = 0;

	virtual void SetTempMissionsCompleted (U32 missionID) = 0;
};

#endif