// $Header: /Libs/Dev/Include/explode.h 2     11/05/99 5:57p Rmarr $

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
	$Header: /Libs/Dev/Include/explode.h 2     11/05/99 5:57p Rmarr $
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

#include "Engine.h"

S32 ExplodeInstance (IEngine* eng,
						INSTANCE_INDEX index, 
						SINGLE strength, 
						S32 num_chunks, 
						U32 num_array_entries,
						INSTANCE_INDEX* chunks);

BOOL32 SplitInstance (IEngine* eng,
					   INSTANCE_INDEX idx,
					   const Vector& normal,
					   SINGLE d,
					   INSTANCE_INDEX *r0,
					   INSTANCE_INDEX *r1);

struct StepBustInfo;
typedef StepBustInfo *HEXPLODE;

HEXPLODE StepExplodeInstance (IEngine* eng,
						INSTANCE_INDEX index, 
						SINGLE strength, 
						S32 num_chunks);

BOOL32 ContinueExplodeInstance (IEngine *eng,
								HEXPLODE info,
								S32 *num_fragments,
								INSTANCE_INDEX *chunks,
								U16 num_array_entries,
								BOOL32 execute);

void CloseExplodeHandle(IEngine *eng,HEXPLODE info);

#endif
