// TextureCoord.h
//
//
//


#ifndef TextureCoord_H
#define TextureCoord_H

//

//

typedef struct TC_UVCOORD
{
	float u;
	float v;

	TC_UVCOORD(void) {}

	TC_UVCOORD( const float _u, const float _v )
	{
		u = _u;
		v = _v;
	}

	//
	// operators
	//
	inline const TC_UVCOORD & operator += (const TC_UVCOORD & tc)
	{
		u += tc.u;
		v += tc.v;
		return *this;
	}

	//
	// friend functions
	//
	inline friend TC_UVCOORD operator * (const TC_UVCOORD & tc, const SINGLE s)
	{
		return TC_UVCOORD(s * tc.u, s * tc.v);
	}

	inline friend TC_UVCOORD operator * (const SINGLE s, const TC_UVCOORD & tc)
	{
		return TC_UVCOORD(s * tc.u, s * tc.v);
	}

	inline friend TC_UVCOORD operator + (const TC_UVCOORD & tc1, const TC_UVCOORD & tc2)
	{
		return TC_UVCOORD(tc1.u + tc2.u, tc1.v + tc2.v);
	}

	inline bool equal(const TC_UVCOORD & tc, const SINGLE tolerance) const
	{
		return ( fabs(u - tc.u) <= tolerance &&
				 fabs(v - tc.v) <= tolerance );
	}

} TexCoord;

//

enum TC_COORD
{
	TC_U =0,
	TC_V =1
};

// Defines how texture coordinates outside of the 
// interval [0,1) are treated.
//
typedef U32 TC_ADDRMODE;
const TC_ADDRMODE TC_ADDR_REPEAT	=0;
const TC_ADDRMODE TC_ADDR_MIRROR	=1;
const TC_ADDRMODE TC_ADDR_CLAMP		=2;
const TC_ADDRMODE TC_ADDR_BORDER	=3;

// Defines how texture coordinates are generated
// and interpolated.
//
typedef U32 TC_WRAPMODE;

const TC_WRAPMODE TC_WRAP_PLANAR	=0;	// Planar in eye space.
const TC_WRAPMODE TC_WRAP_CYL_U		=1;	// Cylindrical along U axis.
const TC_WRAPMODE TC_WRAP_CYL_V		=2;	// Cylindrical along V axis.
const TC_WRAPMODE TC_WRAP_SPHERICAL	=3;	// Spherical in U and V.
const TC_WRAPMODE TC_WRAP_UV_0		=4;	// First object defined map.
const TC_WRAPMODE TC_WRAP_UV_1		=5;	// Second object defined map.

//

#define ADDRESS_MASK_U	0x03
#define ADDRESS_MASK_V	0x0C
#define ADDRESS_SHIFT_U	0x00
#define ADDRESS_SHIFT_V	0x02

#define WRAP_MASK		0xF0
#define WRAP_SHIFT		0x04

//
// the below functions pack and unpack texture mode flags into a U32
//

inline TC_ADDRMODE GET_TC_ADDRESS_MODE( const U32 tflags, const TC_COORD which )
{
	switch( which )
	{
		case TC_U:
			return (tflags & ADDRESS_MASK_U) >> ADDRESS_SHIFT_U;
		case TC_V:
			return (tflags & ADDRESS_MASK_V) >> ADDRESS_SHIFT_V;
		default:
			return 0;
	}
}

inline void SET_TC_ADDRESS_MODE( U32 & tflags, const TC_ADDRMODE mode, const TC_COORD which )
{ 
	switch( which )
	{
		case TC_U:
			tflags = (tflags & ~ADDRESS_MASK_U) | ( ADDRESS_MASK_U & (mode << ADDRESS_SHIFT_U) );
		case TC_V:
			tflags = (tflags & ~ADDRESS_MASK_V) | ( ADDRESS_MASK_V & (mode << ADDRESS_SHIFT_V) );
	}
}

inline TC_WRAPMODE GET_TC_WRAP_MODE( const U32 tflags )
{
	return (tflags & WRAP_MASK) >> WRAP_SHIFT;
}

inline void SET_TC_WRAP_MODE( U32 & tflags, const TC_WRAPMODE mode )
{
	tflags = (tflags & ~WRAP_MASK) | ( WRAP_MASK & (mode << WRAP_SHIFT) );
}

//

#ifndef EXPORT_3DB // ignore the below for the exporters

struct TC_UVGENERATORCONTEXT
{
	Vector P;			// vertex surface point
	Vector N;			// vertex normal at P
	Vector L;			// vertex Lo - P
	Vector V;			// vertex Vo - P

	Vector Nf;			// face normal

	struct Material *M;	// facegroup material

	Vector Lo;			// object light pos in object space
	Vector Vo;			// object viewer pos in object space

	class Transform *Tov;	// object to view
	class Transform *Tow;	// object to world

	U32  SpecularMode;	// 0=none, 1=per-vertex, 2=textured

	// out
	float u,v;
	U8 r,g,b;
};

typedef bool (*TC_UVGENERATOR)( TC_UVGENERATORCONTEXT *render_context, void *user_context );

#endif

//

#endif // EOF

