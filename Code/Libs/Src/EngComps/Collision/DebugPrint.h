#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                 DebugPrint.h                             //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Libs/Dev/Src/EngComps/Collision/DebugPrint.h 1     11/16/99 12:34p Kbaird $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef FDUMP_H
#include <FDump.h>
#endif

#ifdef VERBOSE

#define PHYTRACE10(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define PHYTRACE11(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1)
#define PHYTRACE12(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2)
#define PHYTRACE13(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2,p3)
#define PHYTRACE14(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2,p3,p4)

#else

#define PHYTRACE10(exp) ((void)0)
#define PHYTRACE11(exp,p1) ((void)0)
#define PHYTRACE12(exp,p1,p2) ((void)0)
#define PHYTRACE13(exp,p1,p2,p3) ((void)0)
#define PHYTRACE14(exp,p1,p2,p3,p4) ((void)0)

#endif

#ifdef _DEBUG

#define DBGTRACE10(exp) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp)
#define DBGTRACE11(exp,p1) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1)
#define DBGTRACE12(exp,p1,p2) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2)
#define DBGTRACE13(exp,p1,p2,p3) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2,p3)
#define DBGTRACE14(exp,p1,p2,p3,p4) FDUMP(ErrorCode(ERR_GENERAL, SEV_TRACE_1), exp,p1,p2,p3,p4)

#else

#define DBGTRACE10(exp) ((void)0)
#define DBGTRACE11(exp,p1) ((void)0)
#define DBGTRACE12(exp,p1,p2) ((void)0)
#define DBGTRACE13(exp,p1,p2,p3) ((void)0)
#define DBGTRACE14(exp,p1,p2,p3,p4) ((void)0)

#endif


//--------------------------------------------------------------------------//
//---------------------------End DebugPrint.h-------------------------------//
//--------------------------------------------------------------------------//
#endif