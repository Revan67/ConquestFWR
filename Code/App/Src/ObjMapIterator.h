//--------------------------------------------------------------------------//
//                                                                          //
//                               ObjMapIterator.h                                   //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ObjMapIterator.h 10    9/27/00 11:42a Jasony $
*/			    
//---------------------------------------------------------------------------
#ifndef OBJMAPITERATOR_H
#define OBJMAPITERATOR_H

#ifndef OBJMAP_H
#include "ObjMap.h"
#endif

//---------------------------------------------------------------------------
//

class ObjMapIterator
{
public:
	U32 systemID;
	U32 radius;
	Vector center;

	struct ObjMapNode * current;
	int ref_array[MAX_MAP_REF_ARRAY_SIZE];
	U32 refLoc;
	U32 numRefArray;
	U32 playerID;

	ObjMapIterator (void)
	{
		systemID = 0xFFFFFFFF;
#ifndef FINAL_RELEASE
		CQASSERT(DEBUG_ITERATOR==0);
		DEBUG_ITERATOR = this;
#endif
	}
	
	ObjMapIterator (U32 _systemID,const Vector & _center, U32 _radius,U32 _playerID = 0);
	
	~ObjMapIterator (void)
	{
#ifndef FINAL_RELEASE
		DEBUG_ITERATOR = NULL;
#endif
	}

	void SetArea (U32 _systemID,const Vector & _center, U32 _radius,U32 _playerID = 0);

	void UpdateArea (void);

	void SetFirst (void);

	U32 GetApparentPlayerID (U32 allyMask);
	
	ObjMapNode * Next (void);

	ObjMapNode * operator -> (void)
	{
	 	return current;
	}

	ObjMapNode * operator ++ (void)
	{
		return Next();
	}

#ifndef FINAL_RELEASE
	operator bool (void) const;
#else
	operator bool (void) const
	{
		return (current != 0);
	}
#endif

	void DEBUG_print (void)
	{
	}
};


#endif
//----------------------------------------------------------------------------
//---------------------------END ObjMapIterator.h-------------------------------------
//----------------------------------------------------------------------------