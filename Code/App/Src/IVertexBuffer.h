#ifndef IVERTEXBUFFER_H
#define IVERTEXBUFFER_H
//--------------------------------------------------------------------------//
//                                                                          //
//                           IVertexBuffer.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*

    $Author: Rmarr $

    $Header: /Conquest/App/Src/IVertexBuffer.h 2     8/15/00 2:22p Rmarr $
*/			    
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct _NO_VTABLE IVertexBufferOwner
{
	virtual void RestoreVertexBuffers() = 0;

	IVertexBufferOwner *next,*prev;
};

struct ICQ_VB_Manager
{
	virtual void Add (IVertexBufferOwner *obj) = 0;
	virtual void Delete (IVertexBufferOwner *obj) = 0;
};


//----------------------------------------------------------------------------------
//-------------------------END IVertexBuffer.h--------------------------------------
//----------------------------------------------------------------------------------
#endif