#ifndef ISPLAYERAI_H
#define ISPLAYERAI_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ISPlayerAI.cpp                            //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ISPlayerAI.h 14    6/30/00 11:44a Ahunter $
*/			    
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//----------------------------------------------------------------------------
//

struct DACOM_NO_VTABLE ISPlayerAI : public IDAComponent
{
	virtual void Update (void) = 0;

	virtual void Activate(bool bOnOff) = 0;

	virtual void SetStrategicTarget (IBaseObject *obj, U32 range, U32 systemID = 0) = 0;

	virtual void LaunchOffensive(enum UNIT_STANCE stance) = 0;

	virtual void SetPersonality (const struct AIPersonality & settings) = 0;

	virtual BOOL32 Load (struct IFileSystem * inFile) = 0;

	virtual BOOL32 Save (struct IFileSystem * outFile) = 0;

	virtual void ResolveResolveAssociations (void) = 0;

	virtual const char * getSaveLoadName (void) const = 0;

	static GENRESULT CreateInstance (const char * szPlayerAIType, U32 playerID, ISPlayerAI ** pplayerAI);

	// create a random playerAI for that race
	static GENRESULT CreateInstance (enum M_RACE race, U32 playerID, ISPlayerAI ** pplayerAI);
};


//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//
//---------------------------END ISPlayerAI.h---------------------------------//
//---------------------------------------------------------------------------//
#endif
