#ifndef NETVECTOR_H
#define NETVECTOR_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               NetVector.h                               //
//                                                                          //
//               COPYRIGHT (C) 1999 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/NetVector.h 2     8/03/99 5:27p Jasony $
*/
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
// structure used to send positions across the network
//
//--------------------------------------------------------------------------//
//
struct NETVECTOR
{
	U32 systemID:6;			// don't really need to send this anymore
	S32 posX:13;			// 19 significant bits, lop off the bottom 6 bits = 13 bits
	S32 posY:13;

	Vector getPosition (void) const
	{
		S32 x, y;

		x = posX << 6;
		y = posY << 6;

		return Vector(x, y, 0);
	}

	void setPosition (const Vector & pos)
	{
		S32 x, y;

		x = pos.x;
		y = pos.y;

		posX = ((x+31) >> 6);
		posY = ((y+31) >> 6);
	}
	
};
//---------------------------------------------------------------------------------//
//--------------------------------End NetVector.h----------------------------------//
//---------------------------------------------------------------------------------//
#endif