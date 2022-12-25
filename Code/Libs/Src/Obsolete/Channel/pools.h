#ifndef POOLS_H
#define POOLS_H

//

#include "stddat.h"

//

using namespace Channel;

struct PoolArchetype
{
	//BEGIN Pool stuff
	PoolArchetype *next;      // Next/prev pointers in allocation list or  
	PoolArchetype *prev;      // free list, depending on entry's status
	S32 index;     // Index of this entry in linear array
	void initialize(const void *object);
	void shutdown(void);
	void display(void);
	//END Pool stuff

	PoolArchetype (void);

	unsigned int ref_cnt;

	Archetype* archetype;
};

struct PoolChannelObj
{
	//BEGIN Pool stuff
	PoolChannelObj* next;
	PoolChannelObj* prev;
	S32 index;
	void initialize(const void *object);
	void shutdown(void);
	void display(void);
	//END Pool stuff

	PoolChannelObj (void);
		
	bool active:1;
	bool updated:1;

	CHANNEL_ARCHETYPE_INDEX archetype_index;
	Object* object;
};

//

typedef Pool <PoolArchetype, 64>		archetype_pool;
typedef Pool <PoolChannelObj, 64>		instance_pool;


//

#endif
