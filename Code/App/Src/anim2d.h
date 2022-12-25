#ifndef ANIM2D_H
#define ANIM2D_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                  Anim2D.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/Src/anim2d.h 17    7/28/00 11:42p Rmarr $
*/			    
//--------------------------------------------------------------------------//

//#include <Vector.h>

//#include <Engine.h>
//#include <StdDAT.h>
//#include <TComponent.h>
#include <TSmartPointer.h>

#include "SuperTrans.h"

#ifndef HEAPOBJ_H
#include <heapobj.h>
#endif


struct AnimFrame
{
	U32 texture;
	SINGLE x0;
	SINGLE y0;
	SINGLE x1;
	SINGLE y1;
};

#ifndef RGBA_DEFINED
struct RGBA
{
	U8 r;
	U8 g;
	U8 b;
	U8 a;
};
#endif

//#define MAX_ANIM 3

struct AnimArchetype
{
	ARCHETYPE_INDEX id;
	U32 *tex;
	SINGLE delay;
	U8 texCnt;

//	unsigned int frame_cnt_per[MAX_ANIM];
//	unsigned int start_frame[MAX_ANIM];
	unsigned int frame_cnt;
	AnimFrame* frames;

	SINGLE capture_rate;

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	AnimArchetype (void);

	void release ();

	void load (IFileSystem* fs);

	~AnimArchetype (void);
};

struct AnimInstance
{
	SINGLE x_meters;
	SINGLE y_meters;
	SINGLE cosA,sinA;
	SINGLE pivx,pivy;

	SINGLE delay;

	SINGLE rate,totalAnimTime;

	Vector pos;
//	TRANSFORM transform;
	Vector x_normal;
	Vector y_normal;
	RGBA color;

	const AnimArchetype* archetype;
	SINGLE t;

	bool alwaysFront:1;
	bool loop:1;


	void Init (const AnimArchetype* _archetype);

	void * operator new (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "AnimInstance");
	}

	void * operator new[] (size_t size)
	{
		return HEAP->ClearAllocateMemory(size, "AnimInstance");
	}

	void   operator delete (void *ptr)
	{
		HEAP->FreeMemory(ptr);
	}

	void SetPosition(const Vector &_pos)
	{
//		transform.set_position(pos);
		pos = _pos;
	}

	inline Vector GetPosition (void) const
	{
		//return transform.get_position();
		return pos;
	}

	void Randomize();

	void SetColor(U8 r,U8 g,U8 b,U8 a);

	void SetRotation(SINGLE angle,SINGLE pivx=-1.0,SINGLE pivy=-1.0);
	
	void ForceFront(bool ff);

	void SetWidth(SINGLE _width);

	void Restart();

	BOOL32 update (SINGLE dt);

	const AnimFrame* retrieve_current_frame ();

	void retrieve_frame_ext (const AnimFrame **frame1,const AnimFrame **frame2,SINGLE *share);

	void SetTime (SINGLE time);

//private:
//		SINGLE width;
};



struct ArchNode
{
	ArchNode* next;
	ARCHETYPE_INDEX idx;

	ArchNode (ARCHETYPE_INDEX i = INVALID_ARCHETYPE_INDEX) : idx (i), next (NULL)
	{
	}
};

struct DACOM_NO_VTABLE IAnim2D : public IDAComponent
{
	virtual GENRESULT COMAPI Initialize (void) = 0;

	virtual AnimArchetype * create_archetype (char *fileName) = 0;

	virtual AnimArchetype * create_archetype (struct IFileSystem *parent) = 0;

	virtual void start_batch (AnimArchetype *arch) = 0;

	virtual void start_batch (U32 texID) = 0;

	virtual void end_batch (void) = 0;

	virtual void COMAPI render_instance (AnimInstance *inst,const Transform * const _pos=0) = 0;

	virtual void COMAPI render_instance (ICamera *camera,AnimInstance *inst,const Transform * const _pos=0) = 0;

	virtual void COMAPI render_smooth (AnimInstance *inst,const Transform * const _pos=0) = 0;

	virtual void render(AnimInstance *inst,const Transform * const _trans=0) = 0;
};


	
//----------------------------------------------------------------------------------
//----------------------------END Anim2D.h------------------------------------------
//----------------------------------------------------------------------------------
#endif