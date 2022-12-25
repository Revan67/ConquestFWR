#ifndef ISPLITDIR_H
#define ISPLITDIR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ISplitDir.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/ISplitDir.h 1     7/18/01 2:44p Tmauer $
*/			    
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
#ifndef DACOM_H
#include <DACOM.h>
#endif

//--------------------------------------------------------------------------//
//
struct DACOM_NO_VTABLE ISplitDir: IDAComponent
{
	virtual void AddDir (IFileSystem * fileSys) = 0;
	
	virtual void Empty () = 0;

	virtual void CreateInstance(DACOMDESC *descriptor, void **instance) = 0;

	virtual ISearchPath * GetSearchPath() = 0;
};

#endif