//--------------------------------------------------------------------------//
//                                                                          //
//                                Damage.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/App/Src/Damage.h 2     1/10/00 12:06a Rmarr $
*/			    
//------------------------------- #INCLUDES --------------------------------//
//--------------------------------------------------------------------------//

#ifndef DAMAGE_H
#define DAMAGE_H

#include <TextureCoord.h>

#define POLYS_PER_HIT 48
#define NUM_SHIELD_MAPS 100

struct ShieldMap
{
	Vector v[POLYS_PER_HIT][3];
	TC_UVCOORD t[POLYS_PER_HIT][3];
	int poly_cnt;
	SINGLE timer;
};

extern struct ShieldMap shieldMapBank[NUM_SHIELD_MAPS];

ShieldMap * GetShieldHitMap();

#endif
//---------------------------------------------------------------------------
//---------------------------End Damage.h------------------------------------
//---------------------------------------------------------------------------
