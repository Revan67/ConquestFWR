#ifndef RENDPIPELINE_H
#include "RendPipeline.h"
#endif
//
extern unsigned char TheBigBuffer[];
extern U32 TheBigPointer;

struct BMPOOLJR
{
#define BM_P_F_ENABLE	(1<<0)
#define BM_P_F_EMPTY	(1<<1)

	unsigned char	*pool;
	U32				 pool_used_cnt;			// index
	U32				 old_used_cnt;
	U32				 pool_size;
	U32				 pool_flags;
	bool			 bRendered:1;
	bool			 bLastRendered:1;
	bool			 bAssigned:1;

	//debug only
//	struct BATCHEDMATERIAL *owner;
//	int				source;
	//


public:	// Interface

	BMPOOLJR( )
	{
//		name = "";

		pool = NULL;
		old_used_cnt = pool_used_cnt = 0;
		pool_size = 0;
		pool_flags = BM_P_F_EMPTY;

//		owner = (BATCHEDMATERIAL *)0xffffffff;

		bRendered = bLastRendered = bAssigned = 0;
//		source = 0;

	}

	//

	~BMPOOLJR()
	{
		delete[] pool;
		pool = NULL;
	}

	void set_size( U32 new_pool_size );

	bool is_empty( );

	void reallocate ();

	//now will return an offset from the start of it's pool, 0xFFFFFFFF is fail
	U32 allocate( U32 size, bool set_non_empty=false );

	void set_pool_used_cnt( U32 cnt);

	void free_all();

	void update( bool bInternal );

};
//

