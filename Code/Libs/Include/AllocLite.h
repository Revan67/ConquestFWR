#ifndef __ALLOCLITE_H
#define __ALLOCLITE_H

#include <xmemory>
#include "fdump.h"

//EMAURER attempt to minimize STL calls to new and delete.

//EMAURER since the MS std::list and std::map implementations don't 
//use their allocators in a manner consistent with the allocator
//interface def'n, this allocator cannot be general purpose.

//**D*A*N*G*E*R** complete hacks follow. use with caution.

//*********THIS ALLOCATOR MAKES ASSUMPTIONS ABOUT ITS USER!!!!!!!!!!

template<class T> class AllocLite : public std::allocator<T>
{
	public:

	//assume that the pointer points to something that is at least big
	//enough to be cast to a _NextOverlay.
	void deallocate (void _FARQ* p, size_type _N)
	{
		((_NextOverlay*)p)->next = Head;
		Head = (_NextOverlay*)p;		
	}

	//EMAURER allocate an object of size _N bytes
	char _FARQ *_Charalloc(size_type _N)
	{
		char _FARQ* result;

		//EMAURER insure that all allocations allocate objects of the
		//same size. otherwise, this scheme isn't going to work.
		if (object_size)
		{
			ASSERT (_N == object_size);
		}
		else
		{
			object_size = _N;
		}

		//EMAURER either pull an object from the list, or new one.
		if (Head)
		{
			result = (char _FARQ*)Head;
			Head = Head->next;
		}
		else
			result = (char _FARQ*) operator new (_N);

		return result;
	}

	AllocLite (void)
	{
		ASSERT (heap_ref >= 0);
		heap_ref++;
	}

	~AllocLite (void)
	{
		ASSERT (heap_ref > 0);

		//EMAURER shut down the show.

		if (!--heap_ref)
			shrink ();
	}

	const AllocLite& operator = (const AllocLite& that)
	{
		ASSERT (heap_ref >= 0);
		heap_ref++;
		return *this;
	}

	AllocLite (const AllocLite&)
	{
		ASSERT (heap_ref >= 0);
		heap_ref++;
	}

	static void shrink (void)
	{
		_NextOverlay* cur = Head;

		while (cur)
		{
			_NextOverlay* n = cur->next;
			operator delete (cur);
			cur = n;
		}

		Head = NULL;
	}
		
	protected:

		typedef std::allocator<T> BaseClass;

		struct _NextOverlay
		{
			struct _NextOverlay* next;
		};

		static _NextOverlay* Head;
		static int heap_ref;
		static size_type object_size;
};

//EMAURER 'qualifier' should be the class name of the specialization of AllocatorLite
#define ALLOCLITE_GLOBAL(qualifier) qualifier::_NextOverlay* qualifier::Head = NULL; \
	int qualifier::heap_ref = 0; qualifier::size_type qualifier::object_size = 0;

#endif