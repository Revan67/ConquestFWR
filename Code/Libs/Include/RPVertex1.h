// RPVertex1.h
//
//
//


#ifndef __RPVertex_h__
#define __RPVertex_h__

#pragma warning( push )
#pragma warning( disable : 4201 )		// disable WARNING: nonstandard extension used : nameless struct/union


//

typedef struct RPVertex1
{
	Vector			pos;
	union {
		struct {
		unsigned char	b, g, r, a;
		};
		unsigned int color;
	};
	float			u, v;
} RPVertex;


#define D3DFVF_RPVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

//

#pragma warning( pop ) 

#endif // EOF
