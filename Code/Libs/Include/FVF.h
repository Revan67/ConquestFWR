// FVF.h
//
//
//

#ifndef FVF_H
#define FVF_H

// helpers to determine size of flexible vertex format buffers.
//

#include "typedefs.h"

static U32 FVF_POSITION_SIZE[8] = { 0,3*4,4*4,0,0,0,0,0 };

static U32 FVF_OTHER_SIZE[16]	= { 0,		//				|				|				|	
									3*4,	//				|				|				| _NORMAL
									1*4,	//				|				|	_RESERVED1 	| 
									4*4,	//				|				|	_RESERVED1	| _NORMAL
									1*4,	//				|	_DIFFUSE	|				|
									4*4,	//				|	_DIFFUSE	|				| _NORMAL
									2*4,	//				|	_DIFFUSE	|   _RESERVED1	|
									5*4,	//				|	_DIFFUSE	|   _RESERVED1	| _NORMAL
									1*4,	// _SPECULAR	|				|				|
									4*4,	// _SPECULAR	|				|				| _NORMAL	
									2*4,	// _SPECULAR	|				|	_RESERVED1	|
									5*4,	// _SPECULAR	|				|	_RESERVED1	| _NORMAL
									2*4,	// _SPECULAR	|	_DIFFUSE	|				|
									5*4,	// _SPECULAR	|	_DIFFUSE	|				| _NORMAL
									3*4,	// _SPECULAR	|	_DIFFUSE	|	_RESERVED1	| 
									6*4		// _SPECULAR	|	_DIFFUSE	|	_RESERVED1	| _NORMAL
								};

#define FVF_SIZEOF_POSITION(fvf)	FVF_POSITION_SIZE[((fvf)&D3DFVF_POSITION_MASK)>>1]
#define FVF_SIZEOF_OTHER(fvf)		FVF_OTHER_SIZE[((fvf)&0xF0)>>4]
#define FVF_SIZEOF_TEXCOORDS(fvf)	((((fvf)&D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) * sizeof(float) * 2)
#define FVF_SIZEOF_VERT(fvf)		(FVF_SIZEOF_POSITION(fvf) + FVF_SIZEOF_OTHER(fvf) + FVF_SIZEOF_TEXCOORDS(fvf))

#define FVF_COLOR_OFS(fvf)			( FVF_SIZEOF_POSITION(fvf) + ((fvf)&D3DFVF_NORMAL)? 3*sizeof(float) : 0 )
#define FVF_TEXCOORD_U0_OFS(fvf)	(FVF_SIZEOF_OTHER(fvf) + FVF_SIZEOF_POSITION(fvf))

struct D3DVERTEX
{
	SINGLE nx;
    SINGLE ny;
    SINGLE nz;
    SINGLE tu;
    SINGLE tv;
    SINGLE x ;    
	SINGLE y;
    SINGLE z ;
};

#endif
