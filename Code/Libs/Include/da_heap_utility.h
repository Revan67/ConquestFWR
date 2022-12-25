// da_heap_utility.h
//
//
//
//

#ifndef __da_heap_utility_h__
#define __da_heap_utility_h__

//

#if defined(DA_HEAP_ENABLED)

//

#if defined(DA_HEAP_VERBOSE)
#pragma message( "DA heap is enabled" )
#endif

//

#include "heapobj.h"

//

inline void da_heap_set_message( HINSTANCE hInstance )
{
	S32 dwLen;
	C8  buffer[260];

	dwLen = GetModuleFileName( hInstance, buffer, sizeof(buffer) );

	while( dwLen > 0 ) {
		if( buffer[dwLen] == '\\' ) {
			dwLen++;
			break;
		}
		dwLen--;
	}

	SetDefaultHeapMsg( buffer+dwLen );
}

//

#if defined(DA_HEAP_PRINT_ENABLED)

//

static int __stdcall da_heap_mark_allocated_cb( struct IHeap *heap, void *ptr, U32 ptr_flags, void * )
{
	heap->SetBlockOwner( ptr, 0xFFFFFFFF );

	return 1;
}

//

inline void da_heap_mark_allocted( IHeap *heap )
{
	if( heap->EnumerateBlocks( (IHEAP_ENUM_PROC)da_heap_mark_allocated_cb ) == 0 ) {
		GENERAL_TRACE_1("HEAP: Enumerate failed!\n");
	}
}

//

static int __stdcall da_heap_print_allocated_cb( struct IHeap *heap, void *ptr, U32 ptr_flags, void * )
{
	if( !(ptr_flags & DAHEAPFLAG_ALLOCATED_BLOCK) ) {
		return 1;
	}

	const char *msg = heap->GetBlockMessage( ptr );
	U32 owner = heap->GetBlockOwner( ptr );
	U32 size = heap->GetBlockSize( ptr );

	if( owner == 0xFFFFFFFF ) {
		// marked block
		return 1;
	}

	GENERAL_TRACE_1( TEMPSTR( "HEAP: Allocated block at %08X (%4d bytes) (%08X) %s %s\n",
							  ptr,
							  size,
							  owner,
							  (msg)? msg : "",
							  (ptr_flags & DAHEAPFLAG_CORRUPTED_BLOCK)?"[CORRUPTED]":"[OK]" ) );

	return 1;
}

//

inline void da_heap_print( IHeap *heap )
{
	GENERAL_TRACE_1( TEMPSTR( "HEAP: Largest Block=%d, HeapSize=%d.\n", heap->GetLargestBlock(), heap->GetHeapSize() ) );

	if( heap->GetLargestBlock() == heap->GetHeapSize() ) {
		GENERAL_TRACE_1("HEAP: Heap is empty.\n");
	}
	else {
		GENERAL_TRACE_1("HEAP: Heap is NOT empty.\n");
	}

	if( heap->EnumerateBlocks( (IHEAP_ENUM_PROC)da_heap_print_allocated_cb ) == 0 ) {
		GENERAL_TRACE_1("HEAP: Enumerate failed!\n");
	}
}

//

#define DA_HEAP_PRINT(heapptr) \
	da_heap_print( heapptr );

#define DA_HEAP_MARK_ALLOCATED(heapptr) \
	da_heap_mark_allocted( heapptr );

//

#else

//

#define DA_HEAP_PRINT(heapptr) 
#define DA_HEAP_MARK_ALLOCATED(heapptr) 

//

#endif // DA_HEAP_PRINT_ENABALED

//

#define DA_HEAP_ACQUIRE_HEAP(dstptr) \
	dstptr = HEAP_Acquire();

#define DA_HEAP_DEFINE_NEW_OPERATOR(classname)	\
	void * classname::operator new( size_t size ) \
	{ \
		return HEAP->ClearAllocateMemory( size, #classname ); \
	} \
	void classname::operator delete( void *ptr ) \
	{ \
		HEAP->FreeMemory( ptr ); \
	} 

#define DA_HEAP_DEFINE_HEAP_MESSAGE(hinstance) \
	da_heap_set_message( hinstance );

#define DA_HEAP_MALLOC( heap, size, tag )	(HEAP)->AllocateMemory( (size), (tag) )

#define DA_HEAP_CALLOC( heap, size, tag )	(HEAP)->ClearAllocateMemory( (size), (tag) )

#else

#if defined(DA_HEAP_VERBOSE)
#pragma message( "DA heap is disabled" )
#endif

#include <malloc.h>

#define DA_HEAP_MARK_ALLOCATED(heapptr) 
#define DA_HEAP_PRINT(heapptr) 

#define DA_HEAP_ACQUIRE_HEAP(dstptr) 

// NOTE: the clearing behavior of the new operator is necessary
// NOTE: for some components, hence let it in there.
//
#define DA_HEAP_DEFINE_NEW_OPERATOR(classname)	\
	void * classname::operator new( size_t size ) { return calloc( size, 1 ); } \
	void classname::operator delete( void *ptr ) { free( ptr ); }

#define DA_HEAP_DEFINE_HEAP_MESSAGE(hinstance) 

#define DA_HEAP_MALLOC( heap, size, tag )	malloc( (size) )

#define DA_HEAP_CALLOC( heap, size, tag )	calloc( (size), 

#endif


#endif // EOF
