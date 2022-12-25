#ifndef PARTICLESAVESTRUCTS_H
#define PARTICLESAVESTRUCTS_H
//--------------------------------------------------------------------------//
//                                                                          //
//                              ParticleSavestructs.h                       //
//                                                                          //
//               COPYRIGHT (C) 2003 By Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*

   $Author: Tmauer $
*/


/*

*/
//--------------------------------------------------------------------------//

//#define PARTICLE_SAVE_VERSION 1
// Version one had a different EncodedTransform Struct

//#define PARTICLE_SAVE_VERSION 2
// Vesion had no max particles in the PointEmmiter
 
//#define PARTICLE_SAVE_VERSION 3
// Version had no space type in PointEmmiter

//#define PARTICLE_SAVE_VERSION 4
// Version had no filter on target shaper

#define PARTICLE_SAVE_VERSION 5

struct ParticleHeader
{
	ParticleEffectType type;
	U32 version;
	U32 size;
	char effectName[32];
};

#endif
