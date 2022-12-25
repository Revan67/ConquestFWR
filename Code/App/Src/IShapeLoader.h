#ifndef ISHAPELOADER_H
#define ISHAPELOADER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IShapeLoader.h                              //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IShapeLoader.h 2     4/07/99 11:39a Jasony $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE IShapeLoader : IDAComponent
{
	virtual GENRESULT CreateDrawAgent (U32 subImage, struct IDrawAgent ** drawAgent, RECT * pRect=0) = 0;

	virtual GENRESULT CreateImageReader (U32 subImage, struct IImageReader ** imageReader) = 0;
};

#endif