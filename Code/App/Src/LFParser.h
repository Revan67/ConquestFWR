#ifndef LFPARSER_H
#define LFPARSER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               LFPARSER.H                                 //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/LFParser.h 2     12/11/00 5:59p Jasony $
*/
//--------------------------------------------------------------------------//

#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ILFParser : public IDAComponent
{
	virtual bool Initialize (const char * fileName) = 0;
	
	// returns the number of elements in the array, 
	// -1 on error
	// 0 = IOPENDING
	virtual S32 ParseFile (U32 ** frameArray) = 0;

};


//---------------------------------------------------------------------------//
//-----------------------------End LFParser.h--------------------------------//
//---------------------------------------------------------------------------//
#endif