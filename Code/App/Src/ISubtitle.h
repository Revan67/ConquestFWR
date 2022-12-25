#ifndef ISUBTITLE_H
#define ISUBTITLE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               ISubtitle.h                                //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /Conquest/App/Src/ISubtitle.h 2     7/06/01 11:08a Tmauer $
*/
//--------------------------------------------------------------------------//
 
#ifndef DACOM_H
#include <DACOM.h>
#endif


struct ISubtitle : IDAComponent
{
	virtual void NewSubtitle(wchar_t * subtitle,U32 soundHandle) = 0;

	virtual void NewBriefingSubtitle(wchar_t * subtitle,U32 soundHandle) = 0;
};

#endif