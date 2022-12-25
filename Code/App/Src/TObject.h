#ifndef TOBJECT_H
#define TOBJECT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 TObject.h                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

                       Template class for Base Objects

   $Header: /Conquest/App/Src/TObject.h 6     6/12/00 1:43p Rmarr $	
*/
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

#ifndef HEAPOBJ_H
#include <HeapObj.h>
#endif

#ifndef OBJWATCH_H
#include "ObjWatch.h"
#endif

//----------------------------------//

#define cqoffsetofclass(base, derived) ((U32)(static_cast<base*>((derived*)8))-8)
#define cqoffsetofmember(base, member) (size_t)((&(((base *)0)->member))+0)
#define cqsizeofmember(base,member) (size_t)((&(((base *)0)->member))+1)-(size_t)((&(((base *)0)->member))+0)


struct _INTMAP_ENTRY
{
	enum OBJID objid;
	U32 offset;
};

#define BEGIN_MAP_INBOUND(x) public: \
	const static _INTMAP_ENTRY* __stdcall _CQGetEntriesIn() { \
	typedef x _CQMapClass; \
	static const _INTMAP_ENTRY _entries[] = { 

#define BEGIN_MAP_OUTBOUND(x) public: \
	const static _INTMAP_ENTRY* __stdcall _CQGetEntriesOut() { \
	typedef x _CQMapClass; \
	static const _INTMAP_ENTRY _entries[] = { 

#define _INTERFACE_ENTRY(x)\
	{x##ID, \
	cqoffsetofclass(x, _CQMapClass) - cqoffsetofclass(IBaseObject, _CQMapClass) },

#define _INTERFACE_ENTRY_(x,y)\
	{x, \
	cqoffsetofclass(y, _CQMapClass) - cqoffsetofclass(IBaseObject, _CQMapClass) },

#define _INTERFACE_ENTRY_AGGREGATE(x,y)\
	{x, \
	cqoffsetofmember(_CQMapClass, y) },

#define END_MAP()   {IEmptyID, 0}};\
	return _entries;}



#define ObjectImpl _Coi
//----------------------------------//

template <class Base>
struct ObjectImpl : public Base
{
	ObjectImpl (void)
	{
		// verify asssumption made in Objlist.cpp
#ifdef _DEBUG
		CQASSERT(cqoffsetofclass(IBaseObject, Base) == 0);
#endif
	}

	/* IObject methods */

	virtual void * __fastcall QueryInterface (OBJID objid, OBJPTR<IBaseObject> & pInterface, U32 playerID);

	void * operator new    (size_t size);
	void * operator new[] (size_t size);
	void   operator delete (void *ptr);
	void	operator delete[] (void *ptr);	
};

//--------------------------------------------------------------------------//
//
template <class Base>
void * ObjectImpl< Base >::QueryInterface (OBJID objid, OBJPTR<IBaseObject> & pInterface, U32 playerID)
{
	int i;
	const _INTMAP_ENTRY * interfaces = _CQGetEntriesIn();

	for (i = 0; interfaces[i].objid; i++)
	{
		if (interfaces[i].objid == objid)
		{
			InitObjectPointer(pInterface, playerID,this,interfaces[i].offset);
			return pInterface.Ptr();
		}
	}

	pInterface.Null();
	return 0;
}
//--------------------------------------------------------------------------//
//
template <class Base>
void * ObjectImpl< Base >::operator new (size_t size)
{
#ifdef _DEBUG
	void * result;

	if ((result = HEAP->ClearAllocateMemory(size, "Object instance")) != 0)
	{
		DWORD dwAddr;
		__asm
		{
			mov eax, DWORD PTR [EBP+4]
			mov DWORD PTR dwAddr, eax
		}
		HEAP->SetBlockOwner(result, dwAddr);
	}
	return result;
#else
	return HEAP->ClearAllocateMemory(size);
#endif
}
//--------------------------------------------------------------------------//
//
template <class Base>
void * ObjectImpl< Base >::operator new[] (size_t size)
{
#ifdef _DEBUG
	void * result;

	if ((result = HEAP->ClearAllocateMemory(size, "Object instance")) != 0)
	{
		DWORD dwAddr;
		__asm
		{
			mov eax, DWORD PTR [EBP+4]
			mov DWORD PTR dwAddr, eax
		}
		HEAP->SetBlockOwner(result, dwAddr);
	}
	return result;
#else
	return HEAP->ClearAllocateMemory(size);
#endif
}
//--------------------------------------------------------------------------//
//
template <class Base>
void ObjectImpl< Base >::operator delete (void *ptr)
{
	HEAP->FreeMemory(ptr);
}
//--------------------------------------------------------------------------//
//
template <class Base>
void ObjectImpl< Base >::operator delete[] (void *ptr)
{
	HEAP->FreeMemory(ptr);
}


//--------------------------------------------------------------------------//
//----------------------------End TObject.h---------------------------------//
//--------------------------------------------------------------------------//
#endif