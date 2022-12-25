// $Header: /Conquest/App/Src/MeshExplode.h 10    10/01/00 12:08p Rmarr $

#ifndef EXPLODE_H
#define EXPLODE_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               Explode.h                                  //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Conquest/App/Src/MeshExplode.h 10    10/01/00 12:08p Rmarr $
*/			    
//--------------------------------------------------------------------------//

//
//
// int ExplodeInstance(IEngine*, INSTANCE_INDEX instance, SINGLE strength, int num_chunks, int num_array_entries, INSTANCE_INDEX * chunks);
//
// INPUT:	
//			instance:	Instance index of object to explode.
//			strength:	Explosion strength. An impulse of this magnitude will
//						be applied to each chunk created by the explosion.
//			num_chunks:	Desired number of chunks to create. 
//			num_array_entries: length of 'chunks.'  should be larger than 'num_chunks'
//			chunks:		Pointer to array of INSTANCE_INDEX's which will be 
//						filled in with the new chunk instances created as well as
//						disconnected child objects. 
//
// OUTPUT:	
//			int:		The number of chunks valid entries in 'chunks.'
//
// NOTES:
//			It is the caller's responsibility to remove 'instance' from the engine database.
//			explode_instance () only creates objects NO INSTANCES ARE DELETED.
//			When explode_instance() returns, new instances have been created for 
//			each chunk (via IEngine->create_instance()). It's up to the app to do 
//			something with these chunk instances.

//

struct PHYS_CHUNK
{
	Vector arm;
	SINGLE mass;
};

#include "Engine.h"
#ifndef MESHRENDER_H
#include "MeshRender.h"
#endif
/*
S32 ExplodeInstance (	INSTANCE_INDEX index, 
						SINGLE strength, 
						U32 num_chunks, 
						U32 num_array_entries,
						INSTANCE_INDEX* chunks,
						PHYS_CHUNK *phys);
*/
S32 ExplodeInstance( IMeshInfoTree *tree, SINGLE strength, U32 num_chunks, U32 num_array_entries, 
					MeshChain ** chunks, struct PHYS_CHUNK * phys);

S32 ExplodeInstance( MeshChain *mc, SINGLE strength, U32 num_chunks, U32 num_array_entries, 
					MeshChain ** chunks, struct PHYS_CHUNK * phys);
/*
BOOL32 SplitInstance (   INSTANCE_INDEX idx,
					   const Vector& normal,
					   SINGLE d,
					   INSTANCE_INDEX *r0,
					   INSTANCE_INDEX *r1,
					   PHYS_CHUNK *phys0,
					   PHYS_CHUNK *phys1);
*/
BOOL32 SplitInstance( INSTANCE_INDEX index,IMeshInfoTree *tree, const Vector& normal, SINGLE d, IMeshInfoTree **out_tree0, IMeshInfoTree **out_tree1, PHYS_CHUNK *phys0,PHYS_CHUNK *phys1);

struct StepBustInfo;
typedef StepBustInfo *HEXPLODE;
/*
HEXPLODE StepExplodeInstance (	INSTANCE_INDEX index, 
						SINGLE strength, 
						S32 num_chunks);

BOOL32 ContinueExplodeInstance (HEXPLODE info,
								S32 *num_fragments,
								INSTANCE_INDEX *chunks,
								PHYS_CHUNK *phys,
								U16 num_array_entries,
								BOOL32 execute);

void CloseExplodeHandle(HEXPLODE info);
*/
SINGLE GetMass (MeshInfo *mi);
Vector GetCenterOfMass (MeshInfo *mi);

#endif
