

#ifndef packed_argb_h
#define packed_argb_h

typedef U32 PACKEDARGB;

#define ARGB_MAKE( r, g, b, a )		(((a) & 0xFF)<<24) | (((r) & 0xFF)<<16) | (((g) & 0xFF )<<8) | (((b) & 0xFF )<<0)

#define ARGB_R(packed_argb)			(((packed_argb)>>16) & 0xFF )
#define ARGB_G(packed_argb)			(((packed_argb)>> 8) & 0xFF )
#define ARGB_B(packed_argb)			(((packed_argb)>> 0) & 0xFF )
#define ARGB_A(packed_argb)			(((packed_argb)>>24) & 0xFF )

#endif /* EOF */

