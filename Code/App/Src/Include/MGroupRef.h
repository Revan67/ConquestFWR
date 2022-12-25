#ifndef MGROUPREF_H
#define MGROUPREF_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                MGroupRef.h                               //
//                                                                          //
//                  COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/MGroupRef.h 1     11/07/00 11:04a Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef MPARTREF_H
#include <MPartRef.h>
#endif

#define MAX_SELECTED_UNITS 22		// also defined in globals.h

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//


#ifdef _ADB
#error MGroupRef cannot be used in a save structure!
#else
//---------------------------------------------------------------------------
//
struct MGroupRef
{
private:

	U32 numObjects;
	U32 objectIDs[MAX_SELECTED_UNITS];

public:

	MGroupRef (void)
	{
		numObjects = 0;
	}

	MGroupRef & operator += (const MPartRef & part)
	{
		if (numObjects < MAX_SELECTED_UNITS)
		{
			objectIDs[numObjects++] = part.dwMissionID;
		}
		return *this;
	}
	
	operator int (void) const
	{
		return numObjects;
	}

	void reset (void)
	{
		numObjects = 0;
	}

	// verify that instance is allocated on the stack
	void verify (void) const 
	{
#ifdef _DEBUG
#ifdef CQASSERT
		U32 stack;

		__asm mov stack, esp

		CQASSERT((U32(this) - stack < 8196) && "INVALID USE OF MGroupRef");
#endif
#endif
	}

	friend struct MScript;
};
#endif  // end !_ADB

//---------------------------------------------------------------------------
//----------------------------END MGroupRef.h---------------------------------
//---------------------------------------------------------------------------
#endif