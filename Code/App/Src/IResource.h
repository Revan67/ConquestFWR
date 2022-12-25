#ifndef IRESOURCE_H
#define IRESOURCE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              IResource.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

		                       Resource management

*/
//--------------------------------------------------------------------------//


#ifndef DACOM_H
#include <DACOM.h>
#endif


//------------------------------
// messages
//------------------------------

#define RESMSG_RES_UNOWNED		0x00000001
#define RESMSG_RES_LOST			0x00000002
#define RESMSG_RES_CLOSING		0x00000003


#define RES_PRIORITY_LOW	 	0x40000000
#define RES_PRIORITY_MEDIUM		0x80000000
#define RES_PRIORITY_HIGH		0xC0000000

#define RES_PRIORITY_TOOLBAR	(RES_PRIORITY_MEDIUM + 10)

struct DACOM_NO_VTABLE IResourceClient : public IDAComponent
{
	DEFMETHOD_(BOOL32,Notify) (struct IResource *res, U32 message, void *parm=0) = 0;


};


//--------------------------------------------------------------------------//


struct DACOM_NO_VTABLE IResource : public IDAComponent
{
	DEFMETHOD_(BOOL32,GetOwnership) (struct IResourceClient *res, U32 priority=RES_PRIORITY_MEDIUM) = 0;

	DEFMETHOD_(BOOL32,ReleaseOwnership) (struct IResourceClient *res) = 0;

	DEFMETHOD(SetDefaultState) (void) = 0;		// set to state when there are no owners
};


#endif

//--------------------------------------------------------------------------//
//-------------------------End IResource.h----------------------------------//
//--------------------------------------------------------------------------//
