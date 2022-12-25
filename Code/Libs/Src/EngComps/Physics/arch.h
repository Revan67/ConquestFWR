//
//
//

#ifndef ARCH_H
#define ARCH_H

//

#include <stdlib.h>
#include "rbody.h"

//

struct PhysicalArchetype
{
	RigidBody *			body;

//
// Data and methods required by HashPool template
//
	U32					hash_key;	// Hash key for this entry

	PhysicalArchetype *	hash_next;	// Next/prev pointers in each hash bucket,
	PhysicalArchetype *	hash_prev;	// organized for access speed

	PhysicalArchetype *	next;		// Next/prev pointers in allocation list or  
	PhysicalArchetype *	prev;		// free list, depending on entry's status

	S32					index;		// Index of this entry in linear array

//
	PhysicalArchetype(void)
	{
		body = NULL;
	}

//
// Hash function derives 8-bit key from string by
// adding all ASCII character values modulo 256
//
	static inline U32 PhysicalArchetype::hash(const void *object)
	{
#if 1
		U32 sum;

		_asm
		{
			mov ebx,object
			xor eax,eax
			xor edx,edx

			ALIGN 16
chksum:
			mov dl,BYTE PTR [ebx]
			add eax,edx
			inc ebx
			test edx,edx
			jnz chksum

			and eax,0ffh
			mov sum,eax
		}
#else
		U8 sum;
	//
	// Equivalent C version for portability
	//
		U8 *ptr = (U8 *) object;
		sum     = 0;

		while (*ptr)
		{
			sum += *(ptr++);
		}

		sum &= 0xff;

#endif

		return sum;
	}

	inline void shutdown(void) {}
	inline void display(void) {}

//
// String comparison and maintenance functions
//
	inline BOOL32 compare(const void *object)
	{
		return !strcmp(name, (C8 *) object);
	}

	inline void initialize(const void *object)
	{
		strcpy(name, (C8 *) object);
	}

//
// Physical archetype name
//
	C8 name[_MAX_PATH];
};


//

#endif