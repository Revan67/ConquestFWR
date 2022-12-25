#ifndef MPARTREF_H
#define MPARTREF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                MPartRef.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/MPartRef.h 6     11/07/00 11:03a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//



//---------------------------------------------------------------------------
//
#undef CQEXTERN
#ifdef BUILD_MISSION
#define CQEXTERN __declspec(dllexport)
#define MEXTERN  __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#define MEXTERN  __declspec(dllimport)
#endif

#ifndef _ADB
//---------------------------------------------------------------------------
//
struct MPartRef
{
private:
	mutable U32 index;
	U32 dwMissionID;
	// the following are for the debugger only!
	mutable const struct MISSION_SAVELOAD * pSave;
	mutable const struct MISSION_DATA * pInit;	
	// end debugging members
	struct MCachedPart & getPartFromCache (void) const;		// internal use only

public:

	MPartRef (void)
	{
		pInit = 0;
		pSave = 0;
		index = 0;
		dwMissionID = 0;
	}

	MEXTERN const MISSION_DATA * GetInitData (void) const;
	
	MEXTERN const MISSION_SAVELOAD * operator -> (void) const;

	MEXTERN bool isValid (void) const;

	operator bool (void) const
	{
		return isValid();
	}

	bool operator == (MPartRef & other) const 
	{
		return dwMissionID == other.dwMissionID;
	}

	bool operator  != (MPartRef & other) const 
	{
		return dwMissionID != other.dwMissionID;
	}

	friend struct MScript;
	friend struct MGroupRef;
};

#else

struct MPartRef
{
	U32 index;
	__hexview U32 dwMissionID;
	// the following are for the debugger only!
	__readonly struct MISSION_SAVELOAD * pSave;
	__readonly struct MISSION_DATA * pInit;	
};

#endif  // _ADB


//---------------------------------------------------------------------------
//----------------------------END MPartRef.h---------------------------------
//---------------------------------------------------------------------------
#endif
