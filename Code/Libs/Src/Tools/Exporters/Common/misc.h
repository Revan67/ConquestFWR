#ifndef __MISC_H
#define __MISC_H

#include <stdlib.h>
#include <assert.h>
#include <memory.h>

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF // 2147483647
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define __TODO1(x)           #x
#define __TODO2(x)        __TODO1(x)
#define TODO(desc) message(__FILE__ "("  __TODO2(__LINE__) "): TODO: " #desc)

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned long U32;

#if defined _DEBUG && !defined SGI
inline void Dbreak(void)
{ 
	//if( IsDebuggerPresent() ) DebugBreak();
	__asm int 3 
}
#else
inline void Dbreak(void) {}
#endif

//inline void Free( void*& pt ) { if(pt != NULL) {free(pt); pt = NULL;} }
#define Free(X) {  if((X) != NULL){ free((X)); (X) = NULL; } }
extern void* Malloc(const size_t size);
extern void* Realloc(void *ptr, const size_t size);

extern int Winprint(const char *format, ...);
#define exit(X) { Winprint("Exiting: File: %s  Line: %d  w/ Code: %d\n", __FILE__, __LINE__, (X)); Dbreak(); exit(X); }

void MemSwap(void *a, void *b, const int size);
void StripExtension(char *name);
void StripPath(char *name);

inline void Swap32(void * const a, void * const b)
{	
	if(a != b)
	{
		*(unsigned long*)a ^= *(unsigned long*)b;
		*(unsigned long*)b ^= *(unsigned long*)a;
		*(unsigned long*)a ^= *(unsigned long*)b;
	}
}

inline void Swap16(void * const a, void * const b)
{	
	if(a != b)
	{
		*(unsigned short*)a ^= *(unsigned short*)b;
		*(unsigned short*)b ^= *(unsigned short*)a;
		*(unsigned short*)a ^= *(unsigned short*)b;
	}
}

inline void Swap8(void * const a, void * const b)
{	
	if(a != b)
	{
		*(unsigned char*)a ^= *(unsigned char*)b;
		*(unsigned char*)b ^= *(unsigned char*)a;
		*(unsigned char*)a ^= *(unsigned char*)b; 
	}
}

inline void Swap96(void * const a, void * const b)
{	
	Swap32((unsigned char*)a    , (unsigned char*)b    );
	Swap32((unsigned char*)a + 4, (unsigned char*)b + 4);
	Swap32((unsigned char*)a + 8, (unsigned char*)b + 8);
}

#ifdef SGI
#ifndef bool
typedef unsigned char bool;
#endif

#ifndef true
#define true ((bool)1)
#endif

#ifndef false
#define false ((bool)0)
#endif
#endif

void TrapFpu(bool on);

template<class _Ty> inline const _Ty& _MAX(const _Ty& _X, const _Ty& _Y)
{ return (_X < _Y ? _Y : _X); }
		
template<class _Ty> inline const _Ty& _MIN(const _Ty& _X, const _Ty& _Y)
{ return (_Y < _X ? _Y : _X); }

#ifndef __max
#define __max _MAX
#endif
#ifndef __min
#define __min _MIN
#endif

inline void Clamp01(float * const x)
{
	if( *x > 1.0f )
	{
		*x = 1.0f;
	}else
	if( *x < 0.0f )
	{
		*x = 0.0f;
	}
}

inline float Clamp01(const float x)
{
	return _MIN( _MAX( x, 0.0f ), 1.0f );
}

inline long my_rand( void )
{
	static long l = 0L;
	l = l * 1664525L + 1013904223L;
	return l;
}

inline long randomize( const long l )
{
	return l * 1664525L + 1013904223L;
}

inline bool is_float( const float f )
{
	const unsigned long l = *(unsigned long*)&f;

	return ( ((l>>23) & 255) != 255 );
	//return ( (l & 0x7F800000) != 255<<23 );
}

inline bool is_float( const double d ) // frexp()
{
#ifdef SGI
	"verify this"
#endif
	const unsigned long l = *((unsigned long *)&d + 1);

	return ( ((l>>20) & 2047) != 2047 );
}

#endif
