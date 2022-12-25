//---------------------------------------------------------------------------
//
// TYPEDEFS.H = General type definitions for portability
// 
//---------------------------------------------------------------------------

#ifndef US_TYPEDEFS
#define US_TYPEDEFS

typedef signed   long    BOOL32;
typedef unsigned char    U8 ;
typedef unsigned short   U16;
typedef unsigned long    U32;
typedef          char    C8 ;
typedef signed   char    S8 ;
typedef signed   short   S16;
typedef signed   long    S32;

// bool is not defined on SGI
#ifdef SGI
#ifndef bool
#include <inttypes.h>
typedef unsigned char bool;
#endif
#endif

#ifndef true
#define true ((bool)1)
#endif

#ifndef false
#define false ((bool)0)
#endif

#ifdef __cplusplus      // (for H2INC compatibility)
#ifdef SGI
typedef __uint64_t U64;
typedef __int64_t S64;
#else
typedef unsigned __int64 U64;
typedef signed   __int64 S64;
#endif
#endif

#endif // US_TYPEDEFS

//---------------------------------------------------------------------------

#ifndef F_TYPEDEFS
#define F_TYPEDEFS

typedef float           SINGLE;
typedef double          DOUBLE;

#endif // F_TYPEDEFS

//---------------------------------------------------------------------------

#ifndef H_TYPEDEF
#define H_TYPEDEF

typedef union
{
   S32    i;
   U32    u;
   SINGLE f;
   void  *p;
}
HYBRID;

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO  0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE  0
#endif

//
// int example1[1968];
// int example2[1968];
// double example3[1968];
//
// assert(countof(example1) == 1968);
// assert(countof(example2) == 1968);
// assert(countof(example3) == 1968);
//
#define countof(list) (sizeof(list)/sizeof(list[0]))

#endif // H_TYPEDEF

//---------------------------------------------------------------------------
