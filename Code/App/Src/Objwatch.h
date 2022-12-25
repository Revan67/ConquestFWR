#ifndef OBJWATCH_H
#define OBJWATCH_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              OBJWATCH.H                                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Jasony $

    $Header: /Conquest/App/Src/Objwatch.h 10    10/18/00 1:42a Jasony $
*/			    
//-------------------------------------------------------------------
/*
						DESCRIPTION OF OBJECT WATCHERS

	The RegisterWatcher() installs a "watch" on the dword address passed as 'lplpObject'.
	If an object gets deleted whose address matches the address contained at 'lplpObject',
	the address stored at 'lplpObject' is set to NULL. This prevents dangling pointers to 
	objects that no longer exist.
	
	Any object that can have watchers pointed toward it must call UnRegisterWatchersForObject()
	in its destructor. All watchers that point to the object will be set to NULL.


*/

#ifndef US_TYPEDEFS
#include <Typedefs.h>
#endif

#ifndef CQTRACE_H
#include "CQTrace.h"
#endif

#ifndef IOBJECT_H
#include "IObject.h"
#endif

struct IObject;
struct IBaseObject;

#undef CQEXTERN
#ifdef BUILD_TRIM
#define CQEXTERN __declspec(dllexport)
#else
#define CQEXTERN __declspec(dllimport)
#endif

//-------------------------------------------------------------------
//-------------------------------------------------------------------
template <class Type>
class OBJPTR
{
	friend CQEXTERN void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID, IBaseObject * targetObject, U32 offset);
	friend CQEXTERN void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID);
	friend CQEXTERN void __fastcall UnregisterNonSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask);

//protected:	// Changed by Ryan to facilitate compiling in "FINAL" configuration.
public:
	OBJPTR<Type>     * next;
	OBJPTR<Type>     * prev;
	IBaseObject		 * ptr;
	U32                offset;
	U32				   playerID;
public:

	OBJPTR (void)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
	}

	OBJPTR (const OBJPTR<Type> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,new_ptr.offset);		// move to the correct list
	}

	~OBJPTR (void)
	{
		UninitObjectPointer(*this);
	}

	OBJPTR<Type> & operator = (const OBJPTR<Type> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,new_ptr.offset);		// move to the correct list
		return *this;
	}

	OBJPTR & operator = (int i)
	{
		CQASSERT(i==0);	// can only assign NULL!
		UninitObjectPointer(*this);
		return *this;
	}

	operator OBJPTR<IBaseObject> & (void)
	{
		return *((OBJPTR<IBaseObject> *) this);
	}

	operator Type * (void)
	{
		verify();
	 	return (Type *) (((C8 *)ptr) + offset);
	}

	operator const Type * (void) const
	{
		verify();
	 	return (const Type *) (((C8 *)ptr) + offset);
	}

	Type * operator -> (void) const
	{
		verify();
	 	return (Type *) (((C8 *)ptr) + offset);
	}

	bool operator == (const int i) const
	{
	 	return (ptr == (IBaseObject *)i);
	}

	bool operator != (const int i) const
	{
	 	return (ptr != (IBaseObject *)i);
	}

	bool operator == (const OBJPTR<Type> & cmp) const
	{
	 	return (ptr == cmp.ptr);
	}

	bool operator != (const OBJPTR<Type> & cmp) const
	{
	 	return (ptr != cmp.ptr);
	}

	// maintains current volativity
	void * QueryInterface(OBJID objid, OBJPTR<IBaseObject> & pInterface)
	{
		return ptr->QueryInterface(objid, pInterface, playerID);
	}

	// verify that instance is allocated on the stack
	void verify (void) const 
	{
#ifdef _DEBUG
		if (ptr && playerID == TOTALLYVOLATILEPTR)
		{
			U32 stack;

			__asm mov stack, esp

			CQASSERT((U32(this) - stack < 8196) && "INVALID USE OF VOLATILE OBJPTR");
		}
		if(next)
		{
			CQASSERT(next->ptr == ptr || ptr == NULL  || next->ptr == NULL); 
			CQASSERT(next->prev == this);
		}
		if(prev)
		{
			CQASSERT(prev->ptr == ptr || ptr == NULL  || prev->ptr == NULL);
			CQASSERT(prev->next == this);
		}
#endif
	}

	IBaseObject * Ptr() const
	{
		return ptr;
	}

	void Null()
	{
		UninitObjectPointer(*this);
	}
};
//-------------------------------------------------------------------
//
template <> class OBJPTR<IBaseObject>
{
	friend CQEXTERN void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID, IBaseObject * targetObject, U32 offset);
	friend CQEXTERN void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID);
	friend CQEXTERN void __fastcall UnregisterNonSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask);

//protected:	// Changed by Ryan to facilitate compiling in "FINAL" configuration.
public:
	OBJPTR<IBaseObject>     * next;
	OBJPTR<IBaseObject>     * prev;
	IBaseObject		 * ptr;
	U32                offset;
	U32				   playerID;
public:

	OBJPTR (void)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
	}

	OBJPTR (const OBJPTR<IBaseObject> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,0);		// move to the correct list
	}

	~OBJPTR (void)
	{
		UninitObjectPointer(*this);
	}

	OBJPTR<IBaseObject> & operator = (const OBJPTR<IBaseObject> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,0);		// move to the correct list
		return *this;
	}

	OBJPTR & operator = (int i)
	{
		CQASSERT(i==0);	// can only assign NULL!
		UninitObjectPointer(*this);
		return *this;
	}

	/*
	operator OBJPTR<IBaseObject> & (void)
	{
		return *((OBJPTR<IBaseObject> *) this);
	}
	*/

	operator IBaseObject * (void)
	{
		verify();
	 	return ptr;
	}

	operator const IBaseObject * (void) const
	{
		verify();
	 	return (const IBaseObject *) ptr;
	}

	IBaseObject * operator -> (void) const
	{
		verify();
	 	return ptr;
	}

	bool operator == (const int i) const
	{
	 	return (ptr == (IBaseObject *)i);
	}

	bool operator != (const int i) const
	{
	 	return (ptr != (IBaseObject *)i);
	}

	bool operator == (const OBJPTR<IBaseObject> & cmp) const
	{
	 	return (ptr == cmp.ptr);
	}

	bool operator != (const OBJPTR<IBaseObject> & cmp) const
	{
	 	return (ptr != cmp.ptr);
	}

	// maintains current volativity
	void * QueryInterface(OBJID objid, OBJPTR<IBaseObject> & pInterface)
	{
		return ptr->QueryInterface(objid, pInterface, playerID);
	}

	// verify that instance is allocated on the stack
	void verify (void) const 
	{
#ifdef _DEBUG
		if (ptr && playerID == TOTALLYVOLATILEPTR)
		{
			U32 stack;

			__asm mov stack, esp

			CQASSERT((U32(this) - stack < 8196) && "INVALID USE OF VOLATILE OBJPTR");
		}
		if(next)
		{
			CQASSERT(next->ptr == ptr || ptr == NULL  || next->ptr == NULL); 
			CQASSERT(next->prev == this);
		}
		if(prev)
		{
			CQASSERT(prev->ptr == ptr || ptr == NULL  || prev->ptr == NULL);
			CQASSERT(prev->next == this);
		}
#endif
	}
	IBaseObject * Ptr() const
	{
		return ptr;
	}
	void Null()
	{
		UninitObjectPointer(*this);
		ptr = NULL;
		offset = 0;
	}
};
//-------------------------------------------------------------------
//
template <class Type, enum OBJID objid>
class OBJPTR2
{
	friend CQEXTERN void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID, IBaseObject * targetObject, U32 offset);
	friend CQEXTERN void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID);
	friend CQEXTERN void __fastcall UnregisterNonSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask);
//protected:	// Changed by Ryan to facilitate compiling in "FINAL" configuration.
public:
	OBJPTR2<Type,objid>     * next;
	OBJPTR2<Type,objid>     * prev;
	IBaseObject		 * ptr;
	U32                offset;
	U32				   playerID;
public:

	OBJPTR2 (void)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
		verify();
	}

	OBJPTR2 (IObject * obj)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
		verify();
		if (obj)
			obj->QueryInterface(objid, *this, TOTALLYVOLATILEPTR);
	}

	~OBJPTR2 (void)
	{
		UninitObjectPointer(*this);
	}

	OBJPTR2<Type,objid> & operator = (const OBJPTR2<Type,objid> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,new_ptr.offset);		// move to the correct list
		return *this;
	}

	OBJPTR2 & operator = (IObject * obj)
	{
		UninitObjectPointer(*this);
		if (obj)
			obj->QueryInterface(objid, *this, playerID);
		return *this;
	}

	operator OBJPTR<IBaseObject> & (void)
	{
		return *((OBJPTR<IBaseObject> *) this);
	}

	operator Type * (void)
	{
	 	return (Type *) (((C8 *)ptr) + offset);
	}

	operator const Type * (void) const
	{
	 	return (const Type *) (((C8 *)ptr) + offset);
	}

	Type * operator -> (void) const
	{
	 	return (Type *) (((C8 *)ptr) + offset);
	}

	bool operator == (const int i) const
	{
	 	return (ptr == (IBaseObject *)i);
	}

	bool operator != (const int i) const
	{
	 	return (ptr != (IBaseObject *)i);
	}

	bool operator == (const OBJPTR2<Type,objid> & cmp) const
	{
	 	return (ptr == cmp.ptr);
	}

	bool operator != (const OBJPTR2<Type,objid> & cmp) const
	{
	 	return (ptr != cmp.ptr);
	}

	// maintains current volativity
	void * QueryInterface(OBJID _objid, OBJPTR<IBaseObject> & pInterface)
	{
		return ptr->QueryInterface(_objid, pInterface, playerID);
	}

	// verify that instance is allocated on the stack
	void verify (void) const 
	{
#ifdef _DEBUG
		U32 stack;

		__asm mov stack, esp

		CQASSERT((U32(this) - stack < 8196) && "INVALID USE OF VOLATILE OBJPTR2");
		if(next)
		{
			CQASSERT(next->ptr == ptr || ptr == NULL  || next->ptr == NULL); 
			CQASSERT(next->prev == this);
		}
		if(prev)
		{
			CQASSERT(prev->ptr == ptr || ptr == NULL  || prev->ptr == NULL);
			CQASSERT(prev->next == this);
		}
#endif
	}
	IBaseObject * Ptr() const
	{
		return ptr;
	}
	void Null()
	{
		UninitObjectPointer(*this);
		ptr = NULL;
		offset = 0;
	}
};
//-------------------------------------------------------------------
//
template <> class OBJPTR2<IBaseObject,IBaseObjectID>
{
	friend CQEXTERN void __fastcall InitObjectPointer (OBJPTR<IBaseObject> & instance, U32 playerID, IBaseObject * targetObject, U32 offset);
	friend CQEXTERN void __fastcall UninitObjectPointer (OBJPTR<IBaseObject> & instance);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayer (IBaseObject *lpObject, U32 playerID);
	friend CQEXTERN void __fastcall UnregisterNonSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterSystemVolatileWatchersForObject (IBaseObject *lpObject);
	friend CQEXTERN void __fastcall UnregisterWatchersForObjectForPlayerMask (IBaseObject *lpObject, U32 playerMask);
//protected:	// Changed by Ryan to facilitate compiling in "FINAL" configuration.
public:
	OBJPTR2<IBaseObject,IBaseObjectID>     * next;
	OBJPTR2<IBaseObject,IBaseObjectID>     * prev;
	IBaseObject		 * ptr;
	U32                offset;
	U32				   playerID;
public:

	OBJPTR2 (void)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
		verify();
	}

	OBJPTR2 (IObject * obj)
	{
		next = prev = 0;
		ptr = 0;
		offset = 0;
		playerID = TOTALLYVOLATILEPTR;
		verify();
		if (obj)
			obj->QueryInterface(IBaseObjectID, *this, TOTALLYVOLATILEPTR);
	}

	~OBJPTR2 (void)
	{
		UninitObjectPointer(*this);
	}

	OBJPTR2<IBaseObject,IBaseObjectID> & operator = (const OBJPTR2<IBaseObject,IBaseObjectID> & new_ptr)
	{
		InitObjectPointer(*this, new_ptr.playerID,new_ptr.ptr,0);		// move to the correct list
		return *this;
	}

	OBJPTR2 & operator = (IObject * obj)
	{
		UninitObjectPointer(*this);
		if (obj)
			obj->QueryInterface(IBaseObjectID, *this, playerID);
		return *this;
	}

	operator OBJPTR<IBaseObject> & (void)
	{
		return *((OBJPTR<IBaseObject> *) this);
	}

	operator IBaseObject * (void)
	{
	 	return ptr;
	}

	operator const IBaseObject * (void) const
	{
	 	return (const IBaseObject *) ptr;
	}

	IBaseObject * operator -> (void) const
	{
	 	return ptr;
	}

	bool operator == (const int i) const
	{
	 	return (ptr == (IBaseObject *)i);
	}

	bool operator != (const int i) const
	{
	 	return (ptr != (IBaseObject *)i);
	}

	bool operator == (const OBJPTR2<IBaseObject,IBaseObjectID> & cmp) const
	{
	 	return (ptr == cmp.ptr);
	}

	bool operator != (const OBJPTR2<IBaseObject,IBaseObjectID> & cmp) const
	{
	 	return (ptr != cmp.ptr);
	}

	// maintains current volativity
	void * QueryInterface(OBJID _objid, OBJPTR<IBaseObject> & pInterface)
	{
		return ptr->QueryInterface(_objid, pInterface, playerID);
	}

	// verify that instance is allocated on the stack
	void verify (void) const 
	{
#ifdef _DEBUG
		U32 stack;

		__asm mov stack, esp

		CQASSERT((U32(this) - stack < 8196) && "INVALID USE OF VOLATILE OBJPTR2");
		if(next)
		{
			CQASSERT(next->ptr == ptr || ptr == NULL  || next->ptr == NULL); 
			CQASSERT(next->prev == this);
		}
		if(prev)
		{
			CQASSERT(prev->ptr == ptr || ptr == NULL  || prev->ptr == NULL);
			CQASSERT(prev->next == this);
		}
#endif
	}
	IBaseObject * Ptr() const
	{
		return ptr;
	}
	void Null()
	{
		UninitObjectPointer(*this);
		ptr = NULL;
		offset = 0;
	}
};

#define VOLPTR(x) OBJPTR2<x,x##ID>
//-------------------------------------------------------------------
//



#endif
