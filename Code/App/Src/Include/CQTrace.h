#ifndef CQTRACE_H
#define CQTRACE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                CQTrace.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Include/CQTrace.h 11    10/03/00 10:11p Jasony $
*/			    
//---------------------------------------------------------------------------
// Trace output / error handling for Conquest
//---------------------------------------------------------------------------

#ifndef ERRORKIND_DEFINED
#define ERRORKIND_DEFINED
// Standard error kinds, used by the libraries
#define ERR_LIB_BASE  65536

enum ErrorKind
{
	ERR_MISSION,
	ERR_NETWORK,
	ERR_RULES,
	ERR_GENERAL = ERR_LIB_BASE,  // Catch-all error kind, unclassified
	ERR_FILE,                    // File related errors
	ERR_MEMORY,                  // Memory related errors
	ERR_CORRUPT,                 // Corrupted data structure, bad data
	ERR_ASSERT,                  // failed assertion
	ERR_BADPARAM,                // bad parameters to function call (programmer error)
	ERR_PERFORMANCE,              // something could be done better
	ERR_PARSER				     // Used by parser in Docuview.dll		
};
#else
#error ErrorKind already defined!?
#endif


#ifndef FDUMP_H
#include <FDump.h>
#endif

#ifndef CQIMAGE_H
#include "..\\CQImage.h"
#endif

//----------------------------------------------------------------------------
//
typedef int (__cdecl * DA_ERROR_HANDLER) (ErrorCode code, const C8 *fmt, ...);

#ifdef FINAL_RELEASE
#define SILENCE_TRACE
#endif

#ifdef SILENCE_TRACE

#define CQTRACE10(exp) ((void)0)
#define CQTRACE11(exp,p1) ((void)0)
#define CQTRACE12(exp,p1,p2) ((void)0)
#define CQTRACE13(exp,p1,p2,p3) ((void)0)
#define CQTRACE14(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACE20(exp) ((void)0)
#define CQTRACE21(exp,p1) ((void)0)
#define CQTRACE22(exp,p1,p2) ((void)0)
#define CQTRACE23(exp,p1,p2,p3) ((void)0)
#define CQTRACE24(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACE30(exp) ((void)0)
#define CQTRACE31(exp,p1) ((void)0)
#define CQTRACE32(exp,p1,p2) ((void)0)
#define CQTRACE33(exp,p1,p2,p3) ((void)0) 
#define CQTRACE34(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACE40(exp)	((void)0)
#define CQTRACE41(exp,p1) ((void)0)
#define CQTRACE42(exp,p1,p2) ((void)0)
#define CQTRACE43(exp,p1,p2,p3) ((void)0)
#define CQTRACE44(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACE50(exp) ((void)0)
#define CQTRACE51(exp,p1) ((void)0)
#define CQTRACE52(exp,p1,p2) ((void)0)
#define CQTRACE53(exp,p1,p2,p3) ((void)0)
#define CQTRACE54(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACEM0(exp) ((void)0)
#define CQTRACEM1(exp,p1) ((void)0)
#define CQTRACEM2(exp,p1,p2) ((void)0)
#define CQTRACEM3(exp,p1,p2,p3) ((void)0)
#define CQTRACEM4(exp,p1,p2,p3,p4) ((void)0)

#define CQTRACER0(exp) ((void)0)
#define CQTRACER1(exp,p1) ((void)0)
#define CQTRACER2(exp,p1,p2) ((void)0)
#define CQTRACER3(exp,p1,p2,p3) ((void)0)
#define CQTRACER4(exp,p1,p2,p3,p4) ((void)0)


#define CQFILENOTFOUND(p1) ((void)0)

#define CQASSERT(exp) ((void)0)

#define CQASSERT1(exp,msg,p1) ((void)0)

#define CQASSERT2(exp,msg,p1,p2) ((void)0)

#define CQASSERT3(exp,msg,p1,p2,p3) ((void)0)

#define CQASSERT4(exp,msg,p1,p2,p3,p4) ((void)0)

#define CQBOMB0(exp) ((void)0)

#define CQBOMB1(exp,p1) ((void)0)

#define CQBOMB2(exp,p1,p2) ((void)0)

#define CQBOMB3(exp,p1,p2,p3) ((void)0)

#define CQBOMB4(exp,p1,p2,p3,p4) ((void)0)

#define CQERROR0(exp) ((void)0)

#define CQERROR1(exp,p1) ((void)0)

#define CQERROR2(exp,p1,p2) ((void)0)

#define CQERROR3(exp,p1,p2,p3) ((void)0)

#define CQERROR4(exp,p1,p2,p3,p4) ((void)0)


#else  // !FINAL_RELEASE

#define CQTRACE10(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACE11(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACE12(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACE13(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACE14(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACE20(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_2), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACE21(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_2), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACE22(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_2), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACE23(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_2), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACE24(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_2), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACE30(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_3), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACE31(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_3), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACE32(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_3), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACE33(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_3), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACE34(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_3), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACE40(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACE41(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACE42(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACE43(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACE44(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACE50(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACE51(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACE52(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACE53(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACE54(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_5), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACEM0(exp) FDUMP(ErrorCode(ERR_MISSION, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__)
#define CQTRACEM1(exp,p1) FDUMP(ErrorCode(ERR_MISSION, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1)
#define CQTRACEM2(exp,p1,p2) FDUMP(ErrorCode(ERR_MISSION, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2)
#define CQTRACEM3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_MISSION, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3)
#define CQTRACEM4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_MISSION, SEV_TRACE_4), "%s(%d) : "##exp"\n", __FILE__, __LINE__,p1,p2,p3,p4)

#define CQTRACER0(exp) FDUMP(ErrorCode(ERR_RULES, SEV_TRACE_1), exp)
#define CQTRACER1(exp,p1) FDUMP(ErrorCode(ERR_RULES, SEV_TRACE_1), exp,p1)
#define CQTRACER2(exp,p1,p2) FDUMP(ErrorCode(ERR_RULES, SEV_TRACE_1), exp,p1,p2)
#define CQTRACER3(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_RULES, SEV_TRACE_1), exp,p1,p2,p3)
#define CQTRACER4(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_RULES, SEV_TRACE_1), exp,p1,p2,p3,p4)


#define CQFILENOTFOUND(p1) 	{	\
	if (ICQImage::Error("%s(%d) : File not found: %s", __FILE__, __LINE__, p1)) {\
		__asm int 3	\
	} }

#define CQASSERT(exp)	{	\
	if ( (exp) == 0)	\
	{						\
		if (ICQImage::Assert(#exp, __FILE__, __LINE__))	\
			__asm int 3	\
	} }

#define CQASSERT1(exp,msg,p1)	{	\
	if ( (exp) == 0)	\
	{						\
		if (ICQImage::Bomb("%s(%d) : "#exp##msg, __FILE__, __LINE__, p1))	\
			__asm int 3	\
	} }

#define CQASSERT2(exp,msg,p1,p2)	{	\
	if ( (exp) == 0)	\
	{						\
		if (ICQImage::Bomb("%s(%d) : "#exp##msg, __FILE__, __LINE__, p1, p2))	\
			__asm int 3	\
	} }

#define CQASSERT3(exp,msg,p1,p2,p3)	{	\
	if ( (exp) == 0)	\
	{						\
		if (ICQImage::Bomb("%s(%d) : "#exp##msg, __FILE__, __LINE__, p1, p2, p3))	\
			__asm int 3	\
	} }

#define CQASSERT4(exp,msg,p1,p2,p3,p4)	{	\
	if ( (exp) == 0)	\
	{						\
		if (ICQImage::Bomb("%s(%d) : "#exp##msg, __FILE__, __LINE__, p1, p2, p3, p4))	\
			__asm int 3	\
	} }

#define CQBOMB0(exp)	{	\
	if (ICQImage::Bomb("%s(%d) : "##exp, __FILE__, __LINE__)) { \
		__asm int 3	\
	} }

#define CQBOMB1(exp,p1)	{	\
	if (ICQImage::Bomb("%s(%d) : "##exp, __FILE__, __LINE__, p1)) {\
		__asm int 3	\
	} }

#define CQBOMB2(exp,p1,p2)	{	\
	if (ICQImage::Bomb("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2)) {\
		__asm int 3	\
	} }

#define CQBOMB3(exp,p1,p2,p3)	{	\
	if (ICQImage::Bomb("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2, p3)) {\
		__asm int 3	\
	} }

#define CQBOMB4(exp,p1,p2,p3,p4)	{	\
	if (ICQImage::Bomb("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2, p3, p4)) {\
		__asm int 3	\
	} }

#define CQERROR0(exp)	{	\
	if (ICQImage::Error("%s(%d) : "##exp, __FILE__, __LINE__)) { \
		__asm int 3	\
	} }

#define CQERROR1(exp,p1)	{	\
	if (ICQImage::Error("%s(%d) : "##exp, __FILE__, __LINE__, p1)) {\
		__asm int 3	\
	} }

#define CQERROR2(exp,p1,p2)	{	\
	if (ICQImage::Error("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2)) {\
		__asm int 3	\
	} }

#define CQERROR3(exp,p1,p2,p3)	{	\
	if (ICQImage::Error("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2, p3)) {\
		__asm int 3	\
	} }

#define CQERROR4(exp,p1,p2,p3,p4)	{	\
	if (ICQImage::Error("%s(%d) : "##exp, __FILE__, __LINE__, p1, p2, p3, p4)) {\
		__asm int 3	\
	} }


#endif  // end !FINAL_RELEASE
//-------------------------------------------------------------------------------
//------------------------------END CQTrace.h------------------------------------
//-------------------------------------------------------------------------------
#endif
