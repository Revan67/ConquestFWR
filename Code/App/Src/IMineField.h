#ifndef IMINEFIELD_H
#define IMINEFIELD_H

//--------------------------------------------------------------------------//
//                                                                          //
//                             Minefield.h                                //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/IMineField.h 9     12/14/99 9:59p Tmauer $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef IOBJECT_H
#include "IObject.h"
#endif

//--------------------------------------------------------------------------//

struct _NO_VTABLE IMinefield : IObject
{
	virtual void InitMineField (GRIDVECTOR _gridPos, U32 _systemID) = 0 ;

	virtual void AddLayerRef () = 0;

	virtual U32 GetLayerRef () = 0;

	virtual void SubLayerRef () = 0;

	virtual void AddMine (Vector & pos,Vector & velocity) = 0;

	virtual void SetMineNumber (U32 mineNumber) = 0;

	virtual U32 GetMineNumber () = 0;

	virtual U32 MaxMineNumber () = 0;

	virtual BOOL32 AtPosition(GRIDVECTOR _gridPos) = 0;
};
//--------------------------------------------------------------------------//

#endif