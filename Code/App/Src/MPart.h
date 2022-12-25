#ifndef MPART_H
#define MPART_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                  MPart.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/MPart.h 12    6/06/00 11:28p Jasony $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//
#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef OBJWATCH_H
#include "ObjWatch.h"
#endif

#ifndef DMBASEDATA_H
#include <DMBaseData.h>
#endif

//---------------------------------------------------------------------------
//
template <class MSaveClass>
class MPartHandle
{
public:
	MSaveClass * DATA;
	const struct MISSION_DATA * pInit;
	IBaseObject * obj;

	MPartHandle (void)
	{
		pInit = 0;
		DATA = 0;
		obj = 0;
		verify();
	}

	MPartHandle (IBaseObject * object)
	{
		verify();
		init(object);
	}

	MPartHandle<MSaveClass> & operator = (const MPartHandle<MSaveClass> & newobj)
	{
		obj = newobj.obj;
		pInit = newobj.pInit;
		DATA = newobj.DATA;
		return *this;
	}

private:
	
	// verify that instance is allocated on the stack
	void verify (void)
	{
#ifdef _DEBUG
		U32 stack;

		__asm mov stack, esp

		CQASSERT(U32(this) - stack < 8196);
#endif
	}
	
	void init (IBaseObject * object)
	{
		pInit = 0;
		DATA = 0;

		if ((obj = object) != 0)
		{
			IBaseObject::MDATA mdata;
			if (object->GetMissionData(mdata))
			{
				pInit = mdata.pInitData;	// success!
				DATA = const_cast<MSaveClass *>(mdata.pSaveData);
			}
			else
			{
				obj = 0;
			}
		}
	}
public:

	~MPartHandle (void)
	{
	}

	MPartHandle & operator = (IBaseObject * object)
	{
		init(object);
		return *this;
	}

	MSaveClass * operator -> (void) const
	{
	 	return DATA;
	}

	bool isValid (void) const
	{
		return (obj!=0);
	}

	operator bool (void) const
	{
		return (obj!=0);
	}
};


// const versions
typedef MPartHandle<const struct MISSION_SAVELOAD> MPart;

// non-const versions, use with care!!!
typedef MPartHandle<struct MISSION_SAVELOAD> MPartNC;


//---------------------------------------------------------------------------
//-------------------------------END MPart.h---------------------------------
//---------------------------------------------------------------------------
#endif
