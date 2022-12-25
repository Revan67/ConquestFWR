//--------------------------------------------------------------------------//
//                                                                          //
//                               RandomGen.cpp                             //
//                                                                          //
//                  COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/RandomGen.cpp 2     4/28/00 11:26p Tmauer $
*/			    
//---------------------------------------------------------------------------

#include <pch.h>
#include "RandomGen.h"

RandomGen::RandomGen(U16 seed)
{
	lastRandomNumber	=0xA2A9;	//that's a prime
	srand(seed);
}	

void RandomGen::srand(U16 seed)
{
	lastRandomNumber	=0xA2A9; 

	while(seed--)
	{
		lastRandomNumber	=lastRandomNumber * 65539 + 3;
		lastRandomNumber	=lastRandomNumber * 1009 + 7;
	}
}

U32	RandomGen::rand(U32 max)
{
	U32	a;
	
	lastRandomNumber	=lastRandomNumber * 65539 + 3;
	
	a	=lastRandomNumber >> 16;
	
	return a % max;
}

//-------------------------------------------------------------------
//-------------------------END ObjList.cpp---------------------------
//-------------------------------------------------------------------
