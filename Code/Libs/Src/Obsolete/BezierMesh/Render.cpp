//
// NURB rendering code.
//

#include "BezierMesh.h"									   
#include "lightman.h"
#include "Engine.h"
#include "ILight.h"
#include "ICamera.h"
#include "rendpipeline.h"
#include "FDUMP.h"
#include "matrix4.h"
#include "TextureCoord.h"
#include "inv_sqrt.h"
#include "bezier_curve.h"

const ISQRT isqrt;

#ifdef _DEBUG
#define DEBUG_ASSERT(exp) ASSERT(exp)
#else
#define DEBUG_ASSERT(exp) ((void)0)
#endif

#pragma warning( error : 4701 ) // variable may be used without having been initialized
#pragma warning( push, 3 )

#define IRP_CALL( call )	\
	if( BATCH ) {			\
		BATCH->call ;		\
	}						\
	else if( PIPE ) {		\
		PIPE->call ;		\
	}

inline Vector GetFaceNormal(const Vector & v1, const Vector & v2, const Vector & v3)
{
	Vector norm ( cross_product(v1 - v2, v3 - v2) );

	//norm.normalize();
	//return norm;
	return norm * isqrt.InvSqrt( norm.magnitude_squared() );
}

inline Vector GetFaceCross(const Vector & v1, const Vector & v2, const Vector & v3)
{
	return cross_product(v1 - v2, v3 - v2);
}

const tri_direction[3][3] = {{0,2,1},{1,0,2},{2,1,0}};

inline int GetTriSplitDirection(const float err[3])
{
	if( err[1] >= err[0] && err[1] >= err[2] )
	{
		return 0;// |
	}else
	if( err[2] >= err[0] && err[2] >= err[1] )
	{
		return 1;// /
	}
	else
	{
		return 2;// \ 
	}
}

// TODO: split each patch evenly; then only stitch original edges between patches
void BEZIERMESH::SplitPatches( const PatchGroup & group, const float tolerance )
{
//sub_div_cnt = 1;
	
	patch_list_index = 0;

	for(int src_pid = 0; src_pid < group.patch_cnt; src_pid++)
	{
		const int old_pl_index = patch_list_index;

		patch_list[old_pl_index] = group.patch_list[src_pid];

		Vector normal; float D;
		float err[3];
		GetPatchError(old_pl_index, err, normal, D);

		patch_depth[old_pl_index] = 0;
		patch_alt_tri_style[old_pl_index] = false;

		patch_corners[old_pl_index][0] =
		patch_corners[old_pl_index][2] =
		patch_corners[old_pl_index][4] = 0;
		patch_corners[old_pl_index][1] =
		patch_corners[old_pl_index][3] =
		patch_corners[old_pl_index][5] = EDGE_LENGTH - 1;

		if( sub_div_cnt == 0 )
		{
			patch_list_index++;
			continue;
		}

		int children = 1;

		switch( patch_list[old_pl_index].type )
		{
			case 3:
			{
				bool keep_splitting = (err[0] > tolerance || err[1] > tolerance || err[2] > tolerance);
			
				//while( patch_depth[old_pl_index] < sub_div_cnt )
				while( keep_splitting )
				{
					patch_depth[old_pl_index]++;
					for(int j = 0; j < children; j++)
					{
						const int out_index = old_pl_index + children + 3 * j;
						patch_depth[old_pl_index + j] = patch_depth[old_pl_index]; // other depths are set by split
				
						SplitPatch3( old_pl_index + j, out_index, split_direction[old_pl_index + j][0], src_pid );
						SplitPatch3( old_pl_index + j, out_index + 1, split_direction[old_pl_index + j][1], src_pid );
						SplitPatch3( out_index, out_index + 2, split_direction[old_pl_index + j][2], src_pid );
					}

					children *= 4;

					keep_splitting = false;
					if( patch_depth[old_pl_index] < sub_div_cnt )
					{
						for(j = 0; j < children; j++)
						{
							GetPatchError(old_pl_index + j, err, normal, D); // also sets split direction
							if( !keep_splitting &&
								(err[0] > tolerance || err[1] > tolerance || err[2] > tolerance) )
							{
								keep_splitting = true;
							}
						}
					}
				}

				patch_list_index += children;
			}
			break;

			case 4:
			{
				// 0 vertical; 1 horizontal
				bool keep_splitting = (err[0] > tolerance || err[1] > tolerance);
				
				//while( patch_depth[old_pl_index] < sub_div_cnt )
				while( keep_splitting )
				{
					patch_depth[old_pl_index]++;
					for(int j = 0; j < children; j++)
					{
						const int out_index = old_pl_index + children + 3 * j;
						patch_depth[old_pl_index + j] = patch_depth[old_pl_index]; // other depths are set by split

						SplitPatch4Vertical( old_pl_index + j, out_index, src_pid );
						SplitPatch4Horizontal( old_pl_index + j, out_index + 1, src_pid );
						SplitPatch4Horizontal( out_index, out_index + 2, src_pid );
						//patch_alt_tri_style[out_index  ] =
						//patch_alt_tri_style[out_index+1] =
						//patch_alt_tri_style[out_index+2] = false;
					}

					children *= 4;
					
					keep_splitting = false;
					if( patch_depth[old_pl_index] < sub_div_cnt )
					{
						for(j = 0; j < children; j++)
						{
							GetPatchError(old_pl_index + j, err, normal, D);
							if(err[0] > tolerance || err[1] > tolerance)
							{
								keep_splitting = true;
								break;
							}
						}
					}
				}

				patch_list_index += children;
			}
			break;

			default:
				ASSERT(0);
		}
	}
}

// this is how MAX calculates 5th order interior points from a 4th order patch
#if 0
void BEZIERMESH::CalcInteriorPoints( const BezierPatch & patch )
{
	DEBUG_ASSERT(patch.type == 3);

	scratch_xyz[patch.interior[0]] =
		/*(4.0f/3.0f) **/(scratch_xyz[patch.vec[0]] + scratch_xyz[patch.vec[8]]) -
		/*(5.0f/3.0f) **/  scratch_xyz[patch.v[0]];

	scratch_xyz[patch.interior[1]] =
		/*(4.0f/3.0f) **/ (scratch_xyz[patch.vec[2]] + scratch_xyz[patch.vec[3]]) -
		/*(5.0f/3.0f) **/  scratch_xyz[patch.v[1]];

	scratch_xyz[patch.interior[2]] =
		/*(4.0f/3.0f) **/ (scratch_xyz[patch.vec[5]] + scratch_xyz[patch.vec[6]]) -
		/*(5.0f/3.0f) **/  scratch_xyz[patch.v[2]];
}
#endif

#ifndef _DEBUG
void BEZIERMESH::CheckEdges( const PatchGroup & group ) {}
#else
void BEZIERMESH::CheckEdges( const PatchGroup & group )
{
	for(int eid = 0; eid < edge_list_index; eid++)
	{
		const StitchEdge & edge = edge_list[eid];

		BezierPatch *p1 = &(group.patch_list[edge.pid1]);
		BezierPatch *p2 = &(group.patch_list[edge.pid2]);

		if( edge.depth1 == 0 )
		{
			for(int i = 0; i < EDGE_LENGTH; i++)
			{
				ASSERT( edge.el1[i] == -1 );
			}
		}
		else
		{
			ASSERT( edge.el1[0] >= 0 );
			ASSERT( edge.el1[EDGE_LENGTH-1] >= 0 );

			const int step1 = (EDGE_LENGTH - 1) >> edge.depth1;
		
			for(int i = 0; i < EDGE_LENGTH-1; i += step1)
			{
				ASSERT( edge.el1[i] >= 0 );
				for(int j = 1; j < step1; j++)
				{
					ASSERT( edge.el1[i+j] == -1 );
				}
			}
		}

		if( edge.depth2 == 0 )
		{
			for(int i = 0; i < EDGE_LENGTH; i++)
			{
				ASSERT( edge.el2[i] == -1 );
			}
		}
		else
		{
			ASSERT( edge.el2[0] >= 0 );
			ASSERT( edge.el2[EDGE_LENGTH-1] >= 0 );

			const int step2 = (EDGE_LENGTH - 1) >> edge.depth2;
		
			for(int i = 0; i < EDGE_LENGTH-1; i += step2)
			{
				ASSERT( edge.el2[i] >= 0 );
				for(int j = 1; j < step2; j++)
				{
					ASSERT( edge.el2[i+j] == -1 );
				}
			}
		}
	}
}
#endif

// tables for index remapping depending on the split direction
const int L0[3][1] = {{0}, {1}, {2}};
const int L1[3][2] = {{0,8}, {3,2}, {6,5}};
const int L2[3][4] = {{1,0,7}, {4,1,1}, {7,2,4}};
const int L3[3][4] = {{2,1,2,6}, {5,2,0,0}, {8,0,1,3}};
const int L4[3][8] = {{1,3,4,5,2}, {2,6,7,8,0}, {0,0,1,2,1}};
const int UL[3][4] = {{0,1,2},{1,2,0},{2,0,1}};

int BEZIERMESH::SplitPatch3( const int id0, const int id1, const int direction, const int src_pid )
{
	const BezierPatch patch ( patch_list[id0] );
	DEBUG_ASSERT( patch.type == 3 );

	patch_depth[id1] = patch_depth[id0];

	BezierPatch & patch1 = patch_list[id0];
	BezierPatch & patch2 = patch_list[id1];

	const int * const l0 = L0[direction];
	const int * const l1 = L1[direction];
	const int * const l2 = L2[direction];
	const int * const l3 = L3[direction];
	const int * const l4 = L4[direction];
	const int * const ul = UL[direction];

	patch1.type =
	patch2.type = patch.type;

	// UV
	patch1.tv[ul[0]] =
	patch2.tv[ul[0]] = patch.tv[ul[0]];

	patch1.tv[ul[1]] = patch.tv[ul[1]];
	patch2.tv[ul[2]] = patch.tv[ul[2]];

	scratch_uv[scratch_uv_index] = 
		.5f * (scratch_uv[patch.tv[ul[1]]] + scratch_uv[patch.tv[ul[2]]]);
	patch1.tv[ul[2]] =
	patch2.tv[ul[1]] = scratch_uv_index;
	scratch_uv_index++;

	// XYZ
	//0
	patch1.v[l0[0]] =
	patch2.v[l0[0]] = patch.v[l0[0]];

	//1
	patch1.vec[l1[0]] = patch.vec[l1[0]];
	patch1.vec[l1[1]] =
	patch2.vec[l1[0]] = scratch_xyz_index;
	patch2.vec[l1[1]] = patch.vec[l1[1]];
	scratch_xyz[scratch_xyz_index] =
		.5f * ( scratch_xyz[patch.vec[l1[0]]] + scratch_xyz[patch.vec[l1[1]]] );
	scratch_xyz_index++;

	//2
	patch1.vec[l2[0]] = patch.vec[l2[0]];
	patch1.interior[l2[1]] = scratch_xyz_index;
	patch1.vec[l2[2]] = 
	patch2.vec[l2[0]] = scratch_xyz_index + 1;
	patch2.interior[l2[1]] = scratch_xyz_index + 2;
	patch2.vec[l2[2]] = patch.vec[l2[2]];

	SplitBezCurve3(
		scratch_xyz[patch.vec[l2[0]]], scratch_xyz[patch.interior[l2[1]]], scratch_xyz[patch.vec[l2[2]]],
		scratch_xyz[patch1.vec[l2[0]]],  scratch_xyz[patch1.interior[l2[1]]],  scratch_xyz[patch1.vec[l2[2]]],
		scratch_xyz[patch2.vec[l2[0]]],  scratch_xyz[patch2.interior[l2[1]]],  scratch_xyz[patch2.vec[l2[2]]] );

	scratch_xyz_index += 3;

	//3
	patch1.vec[l3[0]] = patch.vec[l3[0]];
	patch1.interior[l3[1]] = scratch_xyz_index;
	patch1.interior[l3[2]] = scratch_xyz_index + 1;
	patch1.vec[l3[3]] = 
	patch2.vec[l3[0]] = scratch_xyz_index + 2;
	patch2.interior[l3[1]] = scratch_xyz_index + 3;
	patch2.interior[l3[2]] = scratch_xyz_index + 4;
	patch2.vec[l3[3]] = patch.vec[l3[3]];

	SplitBezCurve4(
		scratch_xyz[patch.vec[l3[0]]], scratch_xyz[patch.interior[l3[1]]], scratch_xyz[patch.interior[l3[2]]], scratch_xyz[patch.vec[l3[3]]],
		scratch_xyz[patch1.vec[l3[0]]],  scratch_xyz[patch1.interior[l3[1]]], scratch_xyz[patch1.interior[l3[2]]],  scratch_xyz[patch1.vec[l3[3]]],
		scratch_xyz[patch2.vec[l3[0]]],  scratch_xyz[patch2.interior[l3[1]]], scratch_xyz[patch2.interior[l3[2]]],  scratch_xyz[patch2.vec[l3[3]]] );

	scratch_xyz_index += 5;

	//4
	patch1.v[l4[0]] = patch.v[l4[0]];
	patch1.vec[l4[1]] = scratch_xyz_index;
	patch1.vec[l4[2]] = scratch_xyz_index + 1;
	patch1.vec[l4[3]] = scratch_xyz_index + 2;
	patch1.v[l4[4]] = 
	patch2.v[l4[0]] = scratch_xyz_index + 3;
	patch2.vec[l4[1]] = scratch_xyz_index + 4;
	patch2.vec[l4[2]] = scratch_xyz_index + 5;
	patch2.vec[l4[3]] = scratch_xyz_index + 6;
	patch2.v[l4[4]] = patch.v[l4[4]];

	SplitBezCurve5(
		scratch_xyz[patch.v[l4[0]]], scratch_xyz[patch.vec[l4[1]]], scratch_xyz[patch.vec[l4[2]]], scratch_xyz[patch.vec[l4[3]]], scratch_xyz[patch.v[l4[4]]],
		scratch_xyz[patch1.v[l4[0]]],  scratch_xyz[patch1.vec[l4[1]]], scratch_xyz[patch1.vec[l4[2]]], scratch_xyz[patch1.vec[l4[3]]],  scratch_xyz[patch1.v[l4[4]]],
		scratch_xyz[patch2.v[l4[0]]],  scratch_xyz[patch2.vec[l4[1]]], scratch_xyz[patch2.vec[l4[2]]], scratch_xyz[patch2.vec[l4[3]]],  scratch_xyz[patch2.v[l4[4]]]);

	scratch_xyz_index += 7;

	// EDGES
	if( direction == 0 ) // |  patch 1 left; patch 2 right
	{
		patch1.edges[0] = patch.edges[0];
		patch1.edges[1] = patch.edges[1];
		patch1.edges[2] = -1;

		patch2.edges[0] = -1;
		patch2.edges[1] = patch.edges[1];
		patch2.edges[2] = patch.edges[2];

		// split
		const int v_idx1 = patch_corners[id0][2];
		const int v_idx2 = (patch_corners[id0][2] + patch_corners[id0][3]) / 2;
		const int v_idx3 = patch_corners[id0][3];

		patch_corners[id0][2] = v_idx1;
		patch_corners[id0][3] =
		patch_corners[id1][2] = v_idx2;
		patch_corners[id1][3] = v_idx3;

		//copy
		patch_corners[id1][0] = patch_corners[id0][0];
		patch_corners[id1][1] = patch_corners[id0][1];
		patch_corners[id1][4] = patch_corners[id0][4];
		patch_corners[id1][5] = patch_corners[id0][5];

		// update bottom edge
		if( patch.edges[1] != -1 )
		{
			StitchEdge & edge = edge_list[patch.edges[1]];
			DEBUG_ASSERT(edge.pid1 == src_pid || edge.pid2 == src_pid);
			if( edge.pid1 == src_pid )
			{
				edge.el1[v_idx1] = patch1.v[1];
				edge.el1[v_idx2] = patch1.v[2];
				edge.el1[v_idx3] = patch2.v[2];
				
				edge.depth1 = patch_depth[id1];
			}
			else
			{
				edge.el2[v_idx1] = patch1.v[1];
				edge.el2[v_idx2] = patch1.v[2];
				edge.el2[v_idx3] = patch2.v[2];

				edge.depth2 = patch_depth[id1];
			}
		}

	}else
	if( direction == 1 ) // /	patch 1 bottom; patch 2 top
	{
		patch1.edges[0] = -1;
		patch1.edges[1] = patch.edges[1];
		patch1.edges[2] = patch.edges[2];

		patch2.edges[0] = patch.edges[0];
		patch2.edges[1] = -1;
		patch2.edges[2] = patch.edges[2];

		// split
		const int v_idx1 = patch_corners[id0][4];
		const int v_idx2 = (patch_corners[id0][4] + patch_corners[id0][5]) / 2;
		const int v_idx3 = patch_corners[id0][5];

		patch_corners[id0][4] = v_idx1;
		patch_corners[id0][5] =
		patch_corners[id1][4] = v_idx2;
		patch_corners[id1][5] = v_idx3;

		//copy
		patch_corners[id1][0] = patch_corners[id0][0];
		patch_corners[id1][1] = patch_corners[id0][1];
		patch_corners[id1][2] = patch_corners[id0][2];
		patch_corners[id1][3] = patch_corners[id0][3];

		// update right edge
		if( patch.edges[2] != -1 )
		{
			StitchEdge & edge = edge_list[patch.edges[2]];
			DEBUG_ASSERT(edge.pid1 == src_pid || edge.pid2 == src_pid);
			if( edge.pid1 == src_pid )
			{
				edge.el1[v_idx1] = patch1.v[0];
				edge.el1[v_idx2] = patch1.v[2];
				edge.el1[v_idx3] = patch2.v[2];
				
				edge.depth1 = patch_depth[id1];
			}
			else
			{
				edge.el2[v_idx1] = patch1.v[0];
				edge.el2[v_idx2] = patch1.v[2];
				edge.el2[v_idx3] = patch2.v[2];

				edge.depth2 = patch_depth[id1];
			}
		}
	}
	else				// \   patch 1 top; patch 2 bottom
	{
		patch1.edges[0] = patch.edges[0];
		patch1.edges[1] = -1;
		patch1.edges[2] = patch.edges[2];

		patch2.edges[0] = patch.edges[0];
		patch2.edges[1] = patch.edges[1];
		patch2.edges[2] = -1;

		// split
		const int v_idx1 = patch_corners[id0][0];
		const int v_idx2 = (patch_corners[id0][0] + patch_corners[id0][1]) / 2;
		const int v_idx3 = patch_corners[id0][1];

		patch_corners[id0][0] = v_idx1;
		patch_corners[id0][1] =
		patch_corners[id1][0] = v_idx2;
		patch_corners[id1][1] = v_idx3;

		//copy
		patch_corners[id1][2] = patch_corners[id0][2];
		patch_corners[id1][3] = patch_corners[id0][3];
		patch_corners[id1][4] = patch_corners[id0][4];
		patch_corners[id1][5] = patch_corners[id0][5];

		// update left edge
		if( patch.edges[0] != -1 )
		{
			StitchEdge & edge = edge_list[patch.edges[0]];
			DEBUG_ASSERT(edge.pid1 == src_pid || edge.pid2 == src_pid);
			if( edge.pid1 == src_pid )
			{
				edge.el1[v_idx1] = patch1.v[0];
				edge.el1[v_idx2] = patch1.v[1];
				edge.el1[v_idx3] = patch2.v[1];
				
				edge.depth1 = patch_depth[id1];
			}
			else
			{
				edge.el2[v_idx1] = patch1.v[0];
				edge.el2[v_idx2] = patch1.v[1];
				edge.el2[v_idx3] = patch2.v[1];

				edge.depth2 = patch_depth[id1];
			}
		}
	}

	return 2;
}

// into left and right
int BEZIERMESH::SplitPatch4Vertical( const int id1, const int id2, const int src_pid )
{
	const BezierPatch patch ( patch_list[id1] );
	DEBUG_ASSERT( patch.type == 4 );

	patch_depth[id2] = patch_depth[id1];

	BezierPatch & patch1 = patch_list[id1];
	BezierPatch & patch2 = patch_list[id2];

	patch1.type =
	patch2.type = patch.type;

	// UV
	patch1.tv[0] = patch.tv[0];
	patch1.tv[1] = patch.tv[1];

	scratch_uv[scratch_uv_index] = 
		.5f * (scratch_uv[patch.tv[1]] + scratch_uv[patch.tv[2]]);
	patch1.tv[2] =
	patch2.tv[1] = scratch_uv_index;
	scratch_uv_index++;

	scratch_uv[scratch_uv_index] = 
		.5f * (scratch_uv[patch.tv[0]] + scratch_uv[patch.tv[3]]);
	patch1.tv[3] =
	patch2.tv[0] = scratch_uv_index;
	scratch_uv_index++;

	patch2.tv[2] = patch.tv[2];
	patch2.tv[3] = patch.tv[3];


	// XYZ
	patch1.v[0] = patch.v[0];
	patch1.vec[7] = scratch_xyz_index;
	patch1.vec[6] = scratch_xyz_index + 1;
	patch1.v[3] = 
	patch2.v[0] = scratch_xyz_index + 2;
	patch2.vec[7] = scratch_xyz_index + 3;
	patch2.vec[6] = scratch_xyz_index + 4;
	patch2.v[3] = patch.v[3];

	SplitBezCurve4(
		scratch_xyz[patch.v[0]], scratch_xyz[patch.vec[7]], scratch_xyz[patch.vec[6]], scratch_xyz[patch.v[3]],
		scratch_xyz[patch1.v[0]],  scratch_xyz[patch1.vec[7]],  scratch_xyz[patch1.vec[6]],  scratch_xyz[patch1.v[3]],
		scratch_xyz[patch2.v[0]],  scratch_xyz[patch2.vec[7]],  scratch_xyz[patch2.vec[6]],  scratch_xyz[patch2.v[3]] );

	scratch_xyz_index += 5;

	patch1.vec[0] = patch.vec[0];
	patch1.interior[0] = scratch_xyz_index;
	patch1.interior[3] = scratch_xyz_index + 1;
	patch1.vec[5] = 
	patch2.vec[0] = scratch_xyz_index + 2;
	patch2.interior[0] = scratch_xyz_index + 3;
	patch2.interior[3] = scratch_xyz_index + 4;
	patch2.vec[5] = patch.vec[5];

	SplitBezCurve4(
		scratch_xyz[patch.vec[0]], scratch_xyz[patch.interior[0]], scratch_xyz[patch.interior[3]], scratch_xyz[patch.vec[5]],
		scratch_xyz[patch1.vec[0]],  scratch_xyz[patch1.interior[0]],  scratch_xyz[patch1.interior[3]],  scratch_xyz[patch1.vec[5]],
		scratch_xyz[patch2.vec[0]],  scratch_xyz[patch2.interior[0]],  scratch_xyz[patch2.interior[3]],  scratch_xyz[patch2.vec[5]] );

	scratch_xyz_index += 5;

	patch1.vec[1] = patch.vec[1];
	patch1.interior[1] = scratch_xyz_index;
	patch1.interior[2] = scratch_xyz_index + 1;
	patch1.vec[4] = 
	patch2.vec[1] = scratch_xyz_index + 2;
	patch2.interior[1] = scratch_xyz_index + 3;
	patch2.interior[2] = scratch_xyz_index + 4;
	patch2.vec[4] = patch.vec[4];

	SplitBezCurve4(
		scratch_xyz[patch.vec[1]], scratch_xyz[patch.interior[1]], scratch_xyz[patch.interior[2]], scratch_xyz[patch.vec[4]],
		scratch_xyz[patch1.vec[1]],  scratch_xyz[patch1.interior[1]],  scratch_xyz[patch1.interior[2]],  scratch_xyz[patch1.vec[4]],
		scratch_xyz[patch2.vec[1]],  scratch_xyz[patch2.interior[1]],  scratch_xyz[patch2.interior[2]],  scratch_xyz[patch2.vec[4]] );

	scratch_xyz_index += 5;

	patch1.v[1] = patch.v[1];
	patch1.vec[2] = scratch_xyz_index;
	patch1.vec[3] = scratch_xyz_index + 1;
	patch1.v[2] = 
	patch2.v[1] = scratch_xyz_index + 2;
	patch2.vec[2] = scratch_xyz_index + 3;
	patch2.vec[3] = scratch_xyz_index + 4;
	patch2.v[2] = patch.v[2];

	SplitBezCurve4(
		scratch_xyz[patch.v[1]], scratch_xyz[patch.vec[2]], scratch_xyz[patch.vec[3]], scratch_xyz[patch.v[2]],
		scratch_xyz[patch1.v[1]],  scratch_xyz[patch1.vec[2]],  scratch_xyz[patch1.vec[3]],  scratch_xyz[patch1.v[2]],
		scratch_xyz[patch2.v[1]],  scratch_xyz[patch2.vec[2]],  scratch_xyz[patch2.vec[3]],  scratch_xyz[patch2.v[2]] );

	scratch_xyz_index += 5;

	// EDGES
	patch1.edges[0] = patch.edges[0];
	patch1.edges[1] = patch.edges[1];
	patch1.edges[2] = -1;
	patch1.edges[3] = patch.edges[3];

	patch2.edges[0] = -1;
	patch2.edges[1] = patch.edges[1];
	patch2.edges[2] = patch.edges[2];
	patch2.edges[3] = patch.edges[3];

	const int v_idx1 = patch_corners[id1][0];
	const int v_idx2 = (patch_corners[id1][0] + patch_corners[id1][1]) / 2;
	const int v_idx3 = patch_corners[id1][1];

	// split
	patch_corners[id1][0] = v_idx1;
	patch_corners[id1][1] =
	patch_corners[id2][0] = v_idx2;
	patch_corners[id2][1] = v_idx3;

	// copy
	patch_corners[id2][2] = patch_corners[id1][2];
	patch_corners[id2][3] = patch_corners[id1][3];


	// update top edge
	if( patch.edges[3] != -1 )
	{
		StitchEdge & edge1 = edge_list[patch.edges[3]];
		DEBUG_ASSERT(edge1.pid1 == src_pid || edge1.pid2 == src_pid);
		if( edge1.pid1 == src_pid )
		{
			edge1.el1[v_idx1] = patch1.v[0];
			edge1.el1[v_idx2] = patch1.v[3];
			edge1.el1[v_idx3] = patch2.v[3];
			
			edge1.depth1 = patch_depth[id1];
		}
		else
		{
			edge1.el2[v_idx1] = patch1.v[0];
			edge1.el2[v_idx2] = patch1.v[3];
			edge1.el2[v_idx3] = patch2.v[3];

			edge1.depth2 = patch_depth[id1];
		}
	}
	
	// udate bottom edge
	if( patch.edges[1] != -1 )
	{
		StitchEdge & edge2 = edge_list[patch.edges[1]];
		DEBUG_ASSERT(edge2.pid1 == src_pid || edge2.pid2 == src_pid);
		if( edge2.pid1 == src_pid )
		{
			edge2.el1[v_idx1] = patch1.v[1];
			edge2.el1[v_idx2] = patch1.v[2];
			edge2.el1[v_idx3] = patch2.v[2];

			edge2.depth1 = patch_depth[id1];
		}
		else
		{
			edge2.el2[v_idx1] = patch1.v[1];
			edge2.el2[v_idx2] = patch1.v[2];
			edge2.el2[v_idx3] = patch2.v[2];

			edge2.depth2 = patch_depth[id1];
		}
	}

	return 2;
}

// into top and bottom
int BEZIERMESH::SplitPatch4Horizontal( const int id1, const int id2, const int src_pid )
{
	const BezierPatch patch ( patch_list[id1] );
	DEBUG_ASSERT( patch.type == 4 );

	patch_depth[id2] = patch_depth[id1];

	BezierPatch & patch1 = patch_list[id1];
	BezierPatch & patch2 = patch_list[id2];

	patch1.type =
	patch2.type = patch.type;

	// UV
	patch1.tv[0] = patch.tv[0];
	patch1.tv[3] = patch.tv[3];

	scratch_uv[scratch_uv_index] = 
		.5f * (scratch_uv[patch.tv[0]] + scratch_uv[patch.tv[1]]);
	patch1.tv[1] =
	patch2.tv[0] = scratch_uv_index;
	scratch_uv_index++;

	scratch_uv[scratch_uv_index] = 
		.5f * (scratch_uv[patch.tv[2]] + scratch_uv[patch.tv[3]]);
	patch1.tv[2] =
	patch2.tv[3] = scratch_uv_index;
	scratch_uv_index++;

	patch2.tv[1] = patch.tv[1];
	patch2.tv[2] = patch.tv[2];


	// XYZ
	patch1.v[0] = patch.v[0];
	patch1.vec[0] = scratch_xyz_index;
	patch1.vec[1] = scratch_xyz_index + 1;
	patch1.v[1] = 
	patch2.v[0] = scratch_xyz_index + 2;
	patch2.vec[0] = scratch_xyz_index + 3;
	patch2.vec[1] = scratch_xyz_index + 4;
	patch2.v[1] = patch.v[1];

	SplitBezCurve4(
		scratch_xyz[patch.v[0]], scratch_xyz[patch.vec[0]], scratch_xyz[patch.vec[1]], scratch_xyz[patch.v[1]],
		scratch_xyz[patch1.v[0]],  scratch_xyz[patch1.vec[0]],  scratch_xyz[patch1.vec[1]],  scratch_xyz[patch1.v[1]],
		scratch_xyz[patch2.v[0]],  scratch_xyz[patch2.vec[0]],  scratch_xyz[patch2.vec[1]],  scratch_xyz[patch2.v[1]] );

	scratch_xyz_index += 5;

	patch1.vec[7] = patch.vec[7];
	patch1.interior[0] = scratch_xyz_index;
	patch1.interior[1] = scratch_xyz_index + 1;
	patch1.vec[2] = 
	patch2.vec[7] = scratch_xyz_index + 2;
	patch2.interior[0] = scratch_xyz_index + 3;
	patch2.interior[1] = scratch_xyz_index + 4;
	patch2.vec[2] = patch.vec[2];

	SplitBezCurve4(
		scratch_xyz[patch.vec[7]], scratch_xyz[patch.interior[0]], scratch_xyz[patch.interior[1]], scratch_xyz[patch.vec[2]],
		scratch_xyz[patch1.vec[7]],  scratch_xyz[patch1.interior[0]],  scratch_xyz[patch1.interior[1]],  scratch_xyz[patch1.vec[2]],
		scratch_xyz[patch2.vec[7]],  scratch_xyz[patch2.interior[0]],  scratch_xyz[patch2.interior[1]],  scratch_xyz[patch2.vec[2]] );

	scratch_xyz_index += 5;

	patch1.vec[6] = patch.vec[6];
	patch1.interior[3] = scratch_xyz_index;
	patch1.interior[2] = scratch_xyz_index + 1;
	patch1.vec[3] = 
	patch2.vec[6] = scratch_xyz_index + 2;
	patch2.interior[3] = scratch_xyz_index + 3;
	patch2.interior[2] = scratch_xyz_index + 4;
	patch2.vec[3] = patch.vec[3];

	SplitBezCurve4(
		scratch_xyz[patch.vec[6]], scratch_xyz[patch.interior[3]], scratch_xyz[patch.interior[2]], scratch_xyz[patch.vec[3]],
		scratch_xyz[patch1.vec[6]],  scratch_xyz[patch1.interior[3]],  scratch_xyz[patch1.interior[2]],  scratch_xyz[patch1.vec[3]],
		scratch_xyz[patch2.vec[6]],  scratch_xyz[patch2.interior[3]],  scratch_xyz[patch2.interior[2]],  scratch_xyz[patch2.vec[3]] );

	scratch_xyz_index += 5;

	patch1.v[3] = patch.v[3];
	patch1.vec[5] = scratch_xyz_index;
	patch1.vec[4] = scratch_xyz_index + 1;
	patch1.v[2] = 
	patch2.v[3] = scratch_xyz_index + 2;
	patch2.vec[5] = scratch_xyz_index + 3;
	patch2.vec[4] = scratch_xyz_index + 4;
	patch2.v[2] = patch.v[2];

	SplitBezCurve4(
		scratch_xyz[patch.v[3]], scratch_xyz[patch.vec[5]], scratch_xyz[patch.vec[4]], scratch_xyz[patch.v[2]],
		scratch_xyz[patch1.v[3]],  scratch_xyz[patch1.vec[5]],  scratch_xyz[patch1.vec[4]],  scratch_xyz[patch1.v[2]],
		scratch_xyz[patch2.v[3]],  scratch_xyz[patch2.vec[5]],  scratch_xyz[patch2.vec[4]],  scratch_xyz[patch2.v[2]] );

	scratch_xyz_index += 5;

	// EDGES
	patch1.edges[0] = patch.edges[0];
	patch1.edges[1] = -1;
	patch1.edges[2] = patch.edges[2];
	patch1.edges[3] = patch.edges[3];

	patch2.edges[0] = patch.edges[0];
	patch2.edges[1] = patch.edges[1];
	patch2.edges[2] = patch.edges[2];
	patch2.edges[3] = -1;

	const int v_idx1 = patch_corners[id1][2];
	const int v_idx2 = (patch_corners[id1][2] + patch_corners[id1][3]) / 2;
	const int v_idx3 = patch_corners[id1][3];

	// split
	patch_corners[id1][2] = v_idx1;
	patch_corners[id1][3] =
	patch_corners[id2][2] = v_idx2;
	patch_corners[id2][3] = v_idx3;

	// copy
	patch_corners[id2][0] = patch_corners[id1][0];
	patch_corners[id2][1] = patch_corners[id1][1];

	// update left edge
	if( patch.edges[0] != -1 )
	{
		StitchEdge & edge1 = edge_list[patch.edges[0]];
		DEBUG_ASSERT(edge1.pid1 == src_pid || edge1.pid2 == src_pid);
		if( edge1.pid1 == src_pid )
		{
			edge1.el1[v_idx1] = patch1.v[0];
			edge1.el1[v_idx2] = patch1.v[1];
			edge1.el1[v_idx3] = patch2.v[1];

			edge1.depth1 = patch_depth[id1];
		}
		else
		{
			edge1.el2[v_idx1] = patch1.v[0];
			edge1.el2[v_idx2] = patch1.v[1];
			edge1.el2[v_idx3] = patch2.v[1];

			edge1.depth2 = patch_depth[id1];
		}
	}
	
	// udate right edge
	if( patch.edges[2] != -1 )
	{
		StitchEdge & edge2 = edge_list[patch.edges[2]];
		DEBUG_ASSERT(edge2.pid1 == src_pid || edge2.pid2 == src_pid);
		if( edge2.pid1 == src_pid )
		{
			edge2.el1[v_idx1] = patch1.v[3];
			edge2.el1[v_idx2] = patch1.v[2];
			edge2.el1[v_idx3] = patch2.v[2];

			edge2.depth1 = patch_depth[id1];
		}
		else
		{
			edge2.el2[v_idx1] = patch1.v[3];
			edge2.el2[v_idx2] = patch1.v[2];
			edge2.el2[v_idx3] = patch2.v[2];

			edge2.depth2 = patch_depth[id1];
		}
	}

	return 2;
}

void BEZIERMESH::CalcPatchPlane( const int pid, Vector & normal, float & D )
{
	const BezierPatch & patch = patch_list[pid];
	
	switch(patch.type)
	{
	case 3: // 0 |; 1 /; 2 \ 
		{
			normal = GetFaceNormal(
				scratch_xyz[patch.v[1]],
				scratch_xyz[patch.v[0]],
				scratch_xyz[patch.v[2]] );

			D = dot_product( normal, scratch_xyz[patch.v[0]] );
		}
		break;

	case 4: // 0 vertical; 1 horizontal
		{
			const Vector e0 ( scratch_xyz[patch.v[1]] - scratch_xyz[patch.v[0]] );
			const Vector e1 ( scratch_xyz[patch.v[2]] - scratch_xyz[patch.v[1]] );
			const Vector e2 ( scratch_xyz[patch.v[3]] - scratch_xyz[patch.v[2]] );
			const Vector e3 ( scratch_xyz[patch.v[0]] - scratch_xyz[patch.v[3]] );

			normal =
				cross_product(e0, e1) +
				cross_product(e1, e2) +
				cross_product(e2, e3) +
				cross_product(e3, e0);

			normal *= isqrt.InvSqrt( normal.magnitude_squared() );

			D = dot_product( normal, 
				.25f * (
				scratch_xyz[patch.v[0]] +
				scratch_xyz[patch.v[1]] +
				scratch_xyz[patch.v[2]] +
				scratch_xyz[patch.v[3]]) );
		}
		break;

	default:
		ASSERT(0);
	}
}

void BEZIERMESH::CalcPatchNormals( const int pid )
{
	const BezierPatch & patch = patch_list[pid];

	Vector_4 & p_normals = patch_normals[pid];
	float_4 & p_D = patch_D[pid];
	switch(patch.type)
	{
		case 3:
			p_normals[0] = GetFaceCross(
				scratch_xyz[patch.vec[0]],
				scratch_xyz[patch.v[0]],
				scratch_xyz[patch.vec[8]] );
			p_D[0] = dot_product(p_normals[0], scratch_xyz[patch.v[0]]);

			p_normals[1] = GetFaceCross(
				scratch_xyz[patch.vec[3]],
				scratch_xyz[patch.v[1]],
				scratch_xyz[patch.vec[2]] );
			p_D[1] = dot_product(p_normals[1], scratch_xyz[patch.v[1]]);

			p_normals[2] = GetFaceCross(
				scratch_xyz[patch.vec[6]],
				scratch_xyz[patch.v[2]],
				scratch_xyz[patch.vec[5]] );
			p_D[2] = dot_product(p_normals[2], scratch_xyz[patch.v[2]]);
		break;

		case 4:
			p_normals[0] = GetFaceCross(
				scratch_xyz[patch.vec[0]],
				scratch_xyz[patch.v[0]],
				scratch_xyz[patch.vec[7]] );
			p_D[0] = dot_product(p_normals[0], scratch_xyz[patch.v[0]]);

			p_normals[1] = GetFaceCross(
				scratch_xyz[patch.vec[2]],
				scratch_xyz[patch.v[1]],
				scratch_xyz[patch.vec[1]] );
			p_D[1] = dot_product(p_normals[1], scratch_xyz[patch.v[1]]);

			p_normals[2] = GetFaceCross(
				scratch_xyz[patch.vec[4]],
				scratch_xyz[patch.v[2]],
				scratch_xyz[patch.vec[3]] );
			p_D[2] = dot_product(p_normals[2], scratch_xyz[patch.v[2]]);

			p_normals[3] = GetFaceCross(
				scratch_xyz[patch.vec[6]],
				scratch_xyz[patch.v[3]],
				scratch_xyz[patch.vec[5]] );
			p_D[3] = dot_product(p_normals[3], scratch_xyz[patch.v[3]]);
		break;

		default:
			ASSERT(0);
	}
}

void BEZIERMESH::GetPatchError( const int id, float err[3], Vector & normal, float & D )
{
	CalcPatchPlane( id, normal, D );
	
	const BezierPatch & patch = patch_list[id];
	switch(patch.type)
	{
	case 3: // 0 |; 1 /; 2 \ 
		{
			err[0] = Tmax(fabs(dot_product(scratch_xyz[patch.vec[0]], normal) - D),
						  fabs(dot_product(scratch_xyz[patch.vec[2]], normal) - D));

			err[1] = Tmax(fabs(dot_product(scratch_xyz[patch.vec[3]], normal) - D),
						  fabs(dot_product(scratch_xyz[patch.vec[5]], normal) - D));

			err[2] = Tmax(fabs(dot_product(scratch_xyz[patch.vec[6]], normal) - D),
						  fabs(dot_product(scratch_xyz[patch.vec[8]], normal) - D));

			split_direction[id] = tri_direction[GetTriSplitDirection(err)];
		}
		break;

	case 4: // 0 vertical; 1 horizontal
		{
			err[0] = fabs(dot_product(scratch_xyz[patch.vec[7]], normal) - D);
			err[0] = Tmax<float>(err[0], fabs(dot_product(scratch_xyz[patch.vec[6]], normal) - D));
			err[0] = Tmax<float>(err[0], fabs(dot_product(scratch_xyz[patch.vec[2]], normal) - D));
			err[0] = Tmax<float>(err[0], fabs(dot_product(scratch_xyz[patch.vec[3]], normal) - D));

			err[1] = fabs(dot_product(scratch_xyz[patch.vec[0]], normal) - D);
			err[1] = Tmax<float>(err[1], fabs(dot_product(scratch_xyz[patch.vec[1]], normal) - D));
			err[1] = Tmax<float>(err[1], fabs(dot_product(scratch_xyz[patch.vec[4]], normal) - D));
			err[1] = Tmax<float>(err[1], fabs(dot_product(scratch_xyz[patch.vec[5]], normal) - D));
		}
		break;

	default:
		ASSERT(0);
	}
}

// generate faces and RPVertices
void BEZIERMESH::EvalPatches( const PatchGroup & group )
{
	vertex_pool_index = 0;
	index_list_index = 0;

	memset(scratch_xyz_rp_idx, 0xff, scratch_xyz_index * sizeof(int));
	memset(scratch_xyz_uv_idx, 0xff, scratch_xyz_index * sizeof(int));

	// TODO: keep track of duplicate xyz/uv pairs
	for(int pid = 0; pid < patch_list_index; pid++)
	{
		const BezierPatch & patch = patch_list[pid];
		CalcPatchNormals( pid );

		// vertices
		const int v_count = patch.type;
		int idx[4];
		for(int i = 0; i < v_count; i++)
		{
			if( scratch_xyz_rp_idx[ patch.v[i] ] != -1 &&
				scratch_xyz_uv_idx[ patch.v[i] ] == patch.tv[i] )
			{
				idx[i] = scratch_xyz_rp_idx[ patch.v[i] ];

				normal_pool[idx[i]] += patch_normals[pid][i];
				normal_pool_cnt[idx[i]]++;
			}
			else
			{
				RPVertex & v_out = vertex_pool[vertex_pool_index];
				v_out.pos = scratch_xyz[patch.v[i]];
				DEBUG_ASSERT( patch.tv[i] >= 0 && patch.tv[i] < scratch_uv_index );
			
				v_out.u = scratch_uv[patch.tv[i]].u;
				v_out.v = scratch_uv[patch.tv[i]].v;

				//v_out.color = 0xFFFFFFFF; //0xFF808080;

				idx[i] = vertex_pool_index;
				
				scratch_xyz_rp_idx[ patch.v[i] ] = vertex_pool_index;
				scratch_xyz_uv_idx[ patch.v[i] ] = patch.tv[i];

				vertex_pool_index++;

				normal_pool[idx[i]] = patch_normals[pid][i];
				normal_pool_cnt[idx[i]] = 1;
			}
		}

		// faces
		switch(patch.type)
		{
			case 3:
			{
				index_list[index_list_index  ] = idx[0];
				index_list[index_list_index+1] = idx[2];
				index_list[index_list_index+2] = idx[1];

				index_list_index += 3;
			}
			break;

			case 4:
			{
				if(0)// patch_alt_tri_style[pid] )
				{
					index_list[index_list_index  ] = idx[0];
					index_list[index_list_index+1] = idx[3];
					index_list[index_list_index+2] = idx[1];

					index_list[index_list_index+3] = idx[1];
					index_list[index_list_index+4] = idx[3];
					index_list[index_list_index+5] = idx[2];
				}
				else
				{
					index_list[index_list_index  ] = idx[0];
					index_list[index_list_index+1] = idx[2];
					index_list[index_list_index+2] = idx[1];

					index_list[index_list_index+3] = idx[0];
					index_list[index_list_index+4] = idx[3];
					index_list[index_list_index+5] = idx[2];
				}

				index_list_index += 6;
			}
			break;
			default:
				ASSERT(0);
		}
	}

	for(int i = 0; i < vertex_pool_index; i++)
	{
		// face normals were NOT normalized
		//if( normal_pool_cnt[i] > 1 )
		{
			normal_pool[i] *= isqrt.InvSqrt( normal_pool[i].magnitude_squared() );
		}
	}

#if 1
	//CheckEdges(group);

	for(int eid = 0; eid < edge_list_index; eid++)
	{
		StitchEdge & edge = edge_list[eid];

		if(edge.pid2 == -1) // edge only used by one patch
			continue;

		if( edge.depth1 != edge.depth2 )
		{
			DEBUG_ASSERT( edge.el1[0] != -1 || edge.depth1 == 0);
			DEBUG_ASSERT( edge.el1[EDGE_LENGTH-1] != -1 || edge.depth1 == 0);

			DEBUG_ASSERT( edge.el2[0] != -1 || edge.depth2 == 0);
			DEBUG_ASSERT( edge.el2[EDGE_LENGTH-1] != -1 || edge.depth2 == 0);

			int depth1, depth2;
			int *el1, *el2;
			if( edge.depth1 > edge.depth2 )
			{
				depth1 = edge.depth1;
				depth2 = edge.depth2;
				el1 = edge.el1;
				el2 = edge.el2;
			}
			else
			{
				depth1 = edge.depth2;
				depth2 = edge.depth1;
				el1 = edge.el2;
				el2 = edge.el1;
			}

			const int step1 = (EDGE_LENGTH - 1) >> depth1;
			const int step2 = (EDGE_LENGTH - 1) >> depth2;

			// remap indices from scratch_xyz into scratch_xyz_rp_idx
			if( el1[0] != -1 )
			{
				for(int ep = 0; ep < EDGE_LENGTH; ep += step1)
				{
					DEBUG_ASSERT(el1[ep] >= 0 && el1[ep] < scratch_xyz_index);
					el1[ep] = scratch_xyz_rp_idx[ el1[ep] ];
					DEBUG_ASSERT(el1[ep] >= 0 && el1[ep] < scratch_xyz_index);
				}
			}

			if( el2[0] != -1 )
			{
				for(int ep = 0; ep < EDGE_LENGTH; ep += step2)
				{
					DEBUG_ASSERT(el2[ep] >= 0 && el2[ep] < scratch_xyz_index);
					el2[ep] = scratch_xyz_rp_idx[ el2[ep] ];
					DEBUG_ASSERT(el2[ep] >= 0 && el2[ep] < scratch_xyz_index);
				}
			}

			// emit triangles
			if( depth2 > 0 )
			{
				int id2 = step2;
				for(int id1 = 0; id1 < EDGE_LENGTH-1; id1 += step1)
				{
					index_list[index_list_index    ] = el2[id2];
					index_list[index_list_index + 1] = el1[id1];
					index_list[index_list_index + 2] = el1[id1 + step1];

					DEBUG_ASSERT( index_list[index_list_index    ] >= 0 &&
							index_list[index_list_index    ] < vertex_pool_index);
					DEBUG_ASSERT( index_list[index_list_index + 1] >= 0 &&
							index_list[index_list_index + 1] < vertex_pool_index);
					DEBUG_ASSERT( index_list[index_list_index + 2] >= 0 &&
							index_list[index_list_index + 2] < vertex_pool_index);

					index_list_index += 3;

					if( id1 >= id2 + step2 )
					{
						index_list[index_list_index    ] = el2[id2];
						index_list[index_list_index + 1] = el1[id1 + step1];
						index_list[index_list_index + 2] = el2[id2 + step2];

						DEBUG_ASSERT( index_list[index_list_index    ] >= 0 &&
							index_list[index_list_index    ] < vertex_pool_index);
						DEBUG_ASSERT( index_list[index_list_index + 1] >= 0 &&
								index_list[index_list_index + 1] < vertex_pool_index);
						DEBUG_ASSERT( index_list[index_list_index + 2] >= 0 &&
								index_list[index_list_index + 2] < vertex_pool_index);
						
						index_list_index += 3;

						id2 += step2;
					}
				}
			}
			else
			{
				for(int id1 = step1; id1 < EDGE_LENGTH-1; id1 += step1)
				{
					index_list[index_list_index    ] = el1[0];
					index_list[index_list_index + 1] = el1[id1];
					index_list[index_list_index + 2] = el1[id1 + step1];

					DEBUG_ASSERT( index_list[index_list_index    ] >= 0 &&
							index_list[index_list_index    ] < vertex_pool_index);
					DEBUG_ASSERT( index_list[index_list_index + 1] >= 0 &&
							index_list[index_list_index + 1] < vertex_pool_index);
					DEBUG_ASSERT( index_list[index_list_index + 2] >= 0 &&
							index_list[index_list_index + 2] < vertex_pool_index);

					index_list_index += 3;
				}
			}
		}
	}
#endif
}

vis_state BEZIERMESH::render(ICamera * camera, INSTANCE_INDEX index, U32 flags, const Transform *tr,
						   float lod_fraction)
{
	ASSERT(camera);

	vis_state result = VS_UNKNOWN;

	BezierInstance * object = &instances[index];

	const BezierMesh & bez_mesh = *(object->patch);
	bez_mesh.last_face_cnt = 0;
	bez_mesh.last_vertex_cnt = 0;

	
	Transform world_to_view ( camera->get_inverse_transform() );

	Transform object_to_world(false);
	if(tr != NULL) 
	{
		object_to_world = *tr * ENG->get_transform(index);
	}
	else 
	{
		object_to_world = ENG->get_transform(index);
	}

	bool zero_D;
	Vector svec;
	if( tr ) 
	{	
		svec = Vector(tr->get_i().magnitude(), tr->get_j().magnitude(), tr->get_k().magnitude());
		zero_D = (svec.x < 1.0 || svec.y < 1.0 || svec.z < 1.0);
	}
	else
	{
		svec = Vector(1, 1, 1);
		zero_D = false;
	}

	float max_scale = Tmax(svec.x, Tmax(svec.y, svec.z));

	//
	// Don't bother with objects outside the view frustum.
	//	
	// sphere center in world transformed to view
	Vector view_pos (world_to_view.rotate_translate( object_to_world * bez_mesh.sphere_center));

	result = camera->object_visibility(view_pos, bez_mesh.radius * max_scale);
	if( result == VS_NOT_VISIBLE || result == VS_SUB_PIXEL) 
	{
		return result;
	}

	const Vector cam_pos ( camera->get_position() );
	const Vector cam_pos_in_object_space ( object_to_world.inverse_rotate_translate(cam_pos) );
	

	// Take out any scale that might have been in tr for lighting purposes
	// otherwise normals get scaled and so do dot products
	Transform light_world_to_object(false);
	if( tr ) 
	{
		light_world_to_object = object_to_world;
		light_world_to_object.set_i( light_world_to_object.get_i().normalize() );
		light_world_to_object.set_j( light_world_to_object.get_j().normalize() );
		light_world_to_object.set_k( light_world_to_object.get_k().normalize() );
		light_world_to_object = light_world_to_object.get_inverse(); 
	}
	else 
	{
		light_world_to_object = object_to_world.get_inverse();
	}

	Transform object_to_view ( world_to_view * object_to_world );

	IRP_CALL( set_modelview( object_to_view ) )


	const Vector centroid_in_camera ( object_to_view * bez_mesh.centroid );
	const float pixel_length_at_object = pixel_error * (-centroid_in_camera.z) /
			( camera->get_znear() * camera->get_hpc() );

	// check buffers
	const int max_patches = bez_mesh.patch_cnt * pow(4, sub_div_cnt);
	int i_cnt = 6 * max_patches + 30 * bez_mesh.edge_cnt; // accounts for cracks
	verify_lists(i_cnt);

	int v_cnt = bez_mesh.vertex_cnt + (16 * 4 * max_patches) / 3;
	verify_pools(v_cnt);

	verify_patch_lists( max_patches );

	verify_edge_lists( bez_mesh.edge_cnt );
	edge_list_index = bez_mesh.edge_cnt;

	memcpy( scratch_xyz, bez_mesh.vertices, bez_mesh.vertex_cnt * sizeof(Vector));
	if( bez_mesh.uv_cnt )
	{
		memcpy( scratch_uv, bez_mesh.uv_list, bez_mesh.uv_cnt * sizeof(TexCoord));
	}
	else
	{
		scratch_uv[0].u =
		scratch_uv[0].v = 0.0f;
	}

	bez_mesh.last_face_cnt = 0;
	bez_mesh.last_vertex_cnt = 0;
	const TexCoord *uv_list = bez_mesh.uv_list;
	const Vector *vertices = bez_mesh.vertices;
	for(int i = 0; i < bez_mesh.group_cnt; i++)
	{
		for(int eid = 0; eid < bez_mesh.edge_cnt; eid++)
		{
			StitchEdge & edge = edge_list[eid];
			edge.pid1 = bez_mesh.edges[eid].patch1;
			edge.pid2 = bez_mesh.edges[eid].patch2;
			memset(edge.el1, 0xff, EDGE_LENGTH * sizeof(int));
			memset(edge.el2, 0xff, EDGE_LENGTH * sizeof(int));
			edge.depth1 =
			edge.depth2 = 0;
		}

		const PatchGroup & group = bez_mesh.patch_groups[i];
		
		Material & mat = bez_mesh.material_list[group.mtl_id];
		// Set common state for all passes
		//
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_TEXCOORDINDEX,	0 ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAOP,		D3DTOP_MODULATE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG1,		D3DTA_TEXTURE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ALPHAARG2,		D3DTA_DIFFUSE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLOROP,		D3DTOP_MODULATE ))
		IRP_CALL( set_texture_stage_state( 1, D3DTSS_COLOROP,		D3DTOP_DISABLE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG1,		D3DTA_TEXTURE ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_COLORARG2,		D3DTA_DIFFUSE ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MINFILTER,		D3DTEXF_LINEAR ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MAGFILTER,		D3DTEXF_LINEAR ))
		IRP_CALL( set_sampler_state( 0, D3DSAMP_MIPFILTER,		D3DTEXF_POINT ))

		IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))

		IRP_CALL( set_texture_stage_texture( 0, mat.texture_id ))
		//IRP_CALL( set_render_state( D3DRS_TEXTUREHANDLE, mat.texture_id ))

		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSU,
			MAT_GET_ADDR_MODE(mat.texture_flags,0) ))
		IRP_CALL( set_texture_stage_state( 0, D3DTSS_ADDRESSV,
			MAT_GET_ADDR_MODE(mat.texture_flags,1) ))
		
		bool opaque;
		if (mat.transparency < 255)
		{
			opaque = false;
		}
		else if (mat.texture_id && mat.texture_flags & TF_F_HAS_ALPHA) //mat.texture_alpha_bits)
		{
			opaque = false;
		}
		else
		{
			opaque = true;
		}

		if (!opaque)
		{
			IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, TRUE ) )
			IRP_CALL( set_render_state( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA ) )
			IRP_CALL( set_render_state( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA ) )
		}
		else
		{
			IRP_CALL( set_render_state( D3DRS_ALPHABLENDENABLE, FALSE ) )
		}

		// can't cull because stitch faces have random winding order
		//IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_CCW ))
		IRP_CALL ( set_render_state( D3DRS_CULLMODE, D3DCULL_NONE ) )

		scratch_xyz_index = bez_mesh.vertex_cnt;
		scratch_uv_index = Tmax(1, bez_mesh.uv_cnt); // one for non mapped patches

//TrapFpu( true );
		patch_list_index = 0;
		SplitPatches( group, pixel_length_at_object );

		vertex_pool_index = 0;
		index_list_index = 0;
		EvalPatches( group );
//TrapFpu( false );

		const ClampFlags light_clamp = (flags & RF_CLAMP_COLOR) ? CF_COLOR : CF_INTENSITY;
	
		// light
		if( LIGHTMAN )
		{
			LIGHTMAN->light_vertices_strided( light_pool, sizeof(LightRGB),
											  (Vector*)&(vertex_pool[0].pos), sizeof(*vertex_pool),
											  normal_pool, sizeof(Vector),
											  normal_index_pool, sizeof(U32),
											  vertex_pool_index, &light_world_to_object, light_clamp);
		}
		else
		{
			for(int i = 0; i < vertex_pool_index; i++)
			{
				light_pool[i].r =
				light_pool[i].g =
				light_pool[i].b = 255;
			}
		}

		if (mat.flags & MF_EMITTER)
		{
			for(int vid = 0; vid < vertex_pool_index; vid++)
			{
				vertex_pool[vid].r = Tmin(255, mat.emission.r + ((mat.diffuse.r * light_pool[vid].r) >> 8));
				vertex_pool[vid].g = Tmin(255, mat.emission.g + ((mat.diffuse.g * light_pool[vid].g) >> 8));
				vertex_pool[vid].b = Tmin(255, mat.emission.b + ((mat.diffuse.b * light_pool[vid].b) >> 8));
				vertex_pool[vid].a = mat.transparency;
			}
		}
		else
		{
			for(int vid = 0; vid < vertex_pool_index; vid++)
			{
				vertex_pool[vid].r = Tmin(255, (mat.diffuse.r * light_pool[vid].r) >> 8);
				vertex_pool[vid].g = Tmin(255, (mat.diffuse.g * light_pool[vid].g) >> 8);
				vertex_pool[vid].b = Tmin(255, (mat.diffuse.b * light_pool[vid].b) >> 8);
				vertex_pool[vid].a = mat.transparency;
			}
		}

		bez_mesh.last_face_cnt += index_list_index / 3;
		bez_mesh.last_vertex_cnt += vertex_pool_index;

#if 1 // render
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX,
			vertex_pool, vertex_pool_index, index_list, index_list_index, 0) )
#endif

#if 0 // normals
		IRP_CALL( set_texture_stage_texture( 0, 0 ))
		IRP_CALL( set_render_state( D3DRS_ZENABLE,			TRUE ))
		IRP_CALL( set_render_state( D3DRS_ZWRITEENABLE,	TRUE ))
		//IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_ALWAYS ))
		
		const float scale = .20 * vertex_pool[0].pos.magnitude();
		pb.Begin(PB_LINES);
		for(int vid = 0; vid < vertex_pool_index; vid++)
		{
			pb.Color4ub(255, 0, 0, 255);
			Vector normal_end ( vertex_pool[vid].pos + scale * normal_pool[vid] );
			pb.Vertex3fv((float*)&(normal_end.x));

			pb.Color4ub(255, 255, 255, 255);
			pb.Vertex3fv((float*)&(vertex_pool[vid].pos.x));
		}
		pb.End();

		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ))
#endif

#if 0 // wireframe
		IRP_CALL( set_texture_stage_texture( 0, 0 ))
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESSEQUAL ))
		IRP_CALL( set_render_state( D3DRS_FILLMODE, D3DFILL_WIREFRAME ))
		IRP_CALL( draw_indexed_primitive( D3DPT_TRIANGLELIST, D3DFVF_RPVERTEX,
			vertex_pool, vertex_pool_index, index_list, index_list_index, 0) )
		IRP_CALL( set_render_state( D3DRS_ZFUNC, D3DCMP_LESS ))
		IRP_CALL( set_render_state( D3DRS_FILLMODE, D3DFILL_SOLID ))
		IRP_CALL( set_texture_stage_texture( 0, mat.texture_id ))
#endif



#if 0	// control points
		IRP_CALL( set_texture_stage_texture( 0, 0 ))
		for( pid = 0; pid < group.patch_cnt; pid++)
		{
			const BezierPatch & patch = group.patch_list[pid];
#if 0
			pb.Begin( D3DPT_POINTLIST );
			switch(patch.type)
			{
				case 3:
				{
					for(int i = 0; i < 3; i++)
					{
						if(i==0)
							pb.Color3ub(255, 0, 0);
						else
						if(i==1)
							pb.Color3ub(0, 255, 0);
						else
							pb.Color3ub(255, 255, 255);

						// corner vertices
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.v[i]) );
						
						// vectors
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.vec[2*i]) );
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.vec[2*i+1]) );
						
						// interior
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.interior[i]) );
						
						// aux
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.aux_index + 3 * i) );
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.aux_index + 3 * i + 1) );
						pb.Vertex3fv( (float*)(bez_mesh->vertices + patch.aux_index + 3 * i + 2) );
					}
				}
				break;
				case 4:
				{
				
				}
				break;
				default:
					ASSERT(1);
			}
			pb.End();
#endif
		}
		
#endif

	}

	return result;
}

void BEZIERMESH::delete_pools( void )
{
	delete [] vertex_pool;
	vertex_pool = NULL;
	delete [] normal_pool;
	normal_pool = NULL;
	delete [] normal_pool_cnt;
	normal_pool_cnt = NULL;
	delete [] light_pool;
	light_pool = NULL;
	delete [] normal_index_pool;
	normal_index_pool = NULL;
	delete [] scratch_xyz;
	scratch_xyz = NULL;
	delete[] scratch_uv;
	scratch_uv = NULL;
	delete [] scratch_xyz_rp_idx;
	scratch_xyz_rp_idx = NULL;
	delete [] scratch_xyz_uv_idx;
	scratch_xyz_uv_idx = NULL;

	vertex_pool_len = 0;
}

void BEZIERMESH::verify_pools(int size)
{
	if(size > vertex_pool_len)
	{
		delete_pools();
		vertex_pool_len = size;
		vertex_pool = new RPVertex[vertex_pool_len];
		normal_pool = new Vector[vertex_pool_len];
		normal_pool_cnt = new int[vertex_pool_len];
		light_pool = new LightRGB[vertex_pool_len];
		normal_index_pool = new U32[vertex_pool_len];
		for(int i = 0; i < vertex_pool_len; i++)
		{
			normal_index_pool[i] = i;
		}
		scratch_xyz = new Vector[vertex_pool_len];
		scratch_uv = new TexCoord[vertex_pool_len];
		scratch_xyz_rp_idx = new int[vertex_pool_len];
		scratch_xyz_uv_idx = new int[vertex_pool_len];
	}
}

void BEZIERMESH::delete_lists( void )
{
	delete [] index_list;
	index_list = NULL;
	index_list_len = 0;
}

void BEZIERMESH::verify_lists(int size)
{
	if(size > index_list_len)
	{
		delete_lists();
		index_list_len = size;
		index_list = new U16[index_list_len];
	}
}

void BEZIERMESH::delete_patch_lists( void )
{
	delete [] patch_list;
	patch_list = NULL;
	delete [] patch_normals;
	patch_normals = NULL;
	delete [] patch_D;
	patch_D = NULL;
	delete [] patch_corners;
	patch_corners = NULL;
	delete [] patch_depth;
	patch_depth = NULL;
	delete [] patch_alt_tri_style;
	patch_alt_tri_style = NULL;
	delete [] split_direction;
	split_direction = NULL;
	patch_list_len = 0;
}

void BEZIERMESH::verify_patch_lists(int size)
{
	if(size > patch_list_len)
	{
		delete_patch_lists();
		patch_list_len = size;
		patch_list = new BezierPatch[patch_list_len];
		patch_normals = new Vector_4[patch_list_len];
		patch_D = new float_4[patch_list_len];
		patch_corners = new Corner[patch_list_len];
		patch_depth = new int[patch_list_len];
		patch_alt_tri_style = new bool[patch_list_len];
		split_direction = new const int*[patch_list_len];
	}
}

void BEZIERMESH::delete_edge_lists( void )
{
	delete [] edge_list;
	edge_list = NULL;
	edge_list_len = 0;
}

void BEZIERMESH::verify_edge_lists(int size)
{
	if(size > edge_list_len)
	{
		delete_edge_lists();
		edge_list_len = size;
		edge_list = new StitchEdge[edge_list_len];
	}
}

#pragma warning( pop )