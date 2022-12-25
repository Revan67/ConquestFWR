// $Header: /Libs/Dev/Src/EngOps/explode.cpp 11    10/27/99 9:41p Pbleisch $

#include <stdlib.h>

#include "fdump.h"
#include "TSmartPointer.h"

#include "Engine.h"
#include "Model.h"
#include "Physics.h"
#include "Engine.h"
#include "Renderer.h"
#include "IRigidBody.h"
#include "IRigidBodyState.h"
#include "IExtentContainer.h"

#include "Explode.h"

//

#include "ArchHolder.h"

//

#ifndef FAILED
#define FAILED(x) ((x) != GR_OK)
#endif

//

IEngine *HARCH::ENGINE = NULL;

//

struct ArchNode
{
	ArchNode* next;
	HARCH hArch;		// idx;

	ArchNode (ARCHETYPE_INDEX i = INVALID_ARCHETYPE_INDEX)
	{
		if (i != INVALID_ARCHETYPE_INDEX)
		{
			hArch.setArchetype(i);
			HARCH::ENGINE->hold_archetype(i);
		}
		next = NULL;
	}
};

//

const float MIN_RB_BOX_DIM = 1.0f;
const SINGLE VOLUME_CHANGE_PCT = 0.0f;

//  This is done **sooo** much

inline void get_instance_pstate( IEngine *engine, 
								 IPhysics *physics, 
								 IRenderer *renderer, 
								 INSTANCE_INDEX inst_index, 
								 ARCHETYPE_INDEX arch_index, 
								 Transform &T,
								 Vector &v, 
								 Vector &w, 
								 float &mass, 
								 float &density )
{
	float bbox[6];

	T = engine->get_transform( inst_index );

	mass = physics->get_mass( inst_index );

	renderer->get_archetype_bounding_box( arch_index, 1.0f, bbox );

	float dx = __max( bbox[BBOX_MAX_X] - bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	float dy = __max( bbox[BBOX_MAX_Y] - bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	float dz = __max( bbox[BBOX_MAX_Z] - bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );

	float volume = dx * dy * dz;
	density = mass / (volume + VOLUME_CHANGE_PCT / 100.0f * volume);
		
	v = physics->get_velocity( inst_index );
	w = physics->get_angular_velocity( inst_index );
}

//

inline void add_instance_random_impulse( IPhysics *physics, INSTANCE_INDEX inst_index, Transform &T, Vector &along, float strength, float dx, float dy, float dz )
{
	Vector vert, impulse;

	impulse = along;

	if( !impulse.magnitude() ) {
		impulse.set( rand(), rand(), rand() );
	}
	impulse.normalize();
	impulse *= strength;

	// Apply impulse at a random point on the chunk.
	vert.x = dx * rand() / float(RAND_MAX) - dx / 2.0;
	vert.y = dy * rand() / float(RAND_MAX) - dy / 2.0;
	vert.z = dz * rand() / float(RAND_MAX) - dz / 2.0;

	physics->add_impulse_at_point( inst_index, impulse, T * vert );
}

//

void copy_archetype_rigid_body( IEngine *engine, 
								IRenderer *renderer, 
								IPhysics *physics,
								ARCHETYPE_INDEX src_arch_index,
								ARCHETYPE_INDEX dst_arch_index,
								float &out_dst_dx,
								float &out_dst_dy,
								float &out_dst_dz )

{
	ASSERT( engine != NULL );
	ASSERT( renderer != NULL ) ;
	ASSERT( physics != NULL );
	ASSERT( src_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( dst_arch_index != INVALID_ARCHETYPE_INDEX );
	
	IRigidBody *src_irb, *dst_irb;
	IExtentContainer *dst_iec;
	
	engine->query_archetype_interface( src_arch_index, IID_IRigidBody, (IDAComponent**)&src_irb ) ;
	engine->query_archetype_interface( dst_arch_index, IID_IRigidBody, (IDAComponent**)&dst_irb ) ;
	engine->query_archetype_interface( dst_arch_index, IID_IExtentContainer, (IDAComponent**)&dst_iec ) ;

	ASSERT( src_irb != NULL ) ;
	ASSERT( dst_irb != NULL ) ;
	ASSERT( dst_iec != NULL ) ;

	float density, volume, dx, dy, dz, mass;
	float src_bbox[6], dst_bbox[6];
	Vector center_of_mass, radius;
	SphereExtent *sphere_extent;
	BoxExtent *box_extent;
	Box box;
	Sphere sphere;
	Matrix Ibody;
	

	renderer->get_archetype_bounding_box( src_arch_index, 1.0f, src_bbox );
		
	dx = __max( src_bbox[BBOX_MAX_X] - src_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = __max( src_bbox[BBOX_MAX_Y] - src_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = __max( src_bbox[BBOX_MAX_Z] - src_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );
	
	volume = dx * dy * dz;
	density = src_irb->get_mass() / (volume + VOLUME_CHANGE_PCT / 100.0f * volume);
		

	renderer->get_archetype_bounding_box( dst_arch_index, 1.0f, dst_bbox );

	dx = out_dst_dx = __max( dst_bbox[BBOX_MAX_X] - dst_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = out_dst_dy = __max( dst_bbox[BBOX_MAX_Y] - dst_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = out_dst_dz = __max( dst_bbox[BBOX_MAX_Z] - dst_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );
		
	mass = density * dx * dy * dz ;

	// calc center of mass
	//
	center_of_mass.x = (dst_bbox[BBOX_MAX_X] + dst_bbox[BBOX_MIN_X]) / 2.0;
	center_of_mass.y = (dst_bbox[BBOX_MAX_Y] + dst_bbox[BBOX_MIN_Y]) / 2.0;
	center_of_mass.z = (dst_bbox[BBOX_MAX_Z] + dst_bbox[BBOX_MIN_Z]) / 2.0;

	// calc extents
	//
	box.half_x = dx / 2.0;
	box.half_y = dy / 2.0;
	box.half_z = dz / 2.0;

	box_extent = new BoxExtent( box );
	box_extent->xform.set_identity();
	box_extent->xform.set_position( center_of_mass );

	radius.set( dst_bbox[BBOX_MAX_X], dst_bbox[BBOX_MAX_Y], dst_bbox[BBOX_MAX_Z] );
	sphere.radius = radius.magnitude();

	sphere_extent = new SphereExtent( sphere );
	sphere_extent->xform.set_identity();
	sphere_extent->xform.set_position( center_of_mass );
	sphere_extent->child = box_extent;

	// calc inertial tensor for box
	//
	float xx = dx * dx;
	float yy = dy * dy;
	float zz = dz * dz;

	float scale = mass / 12.0;
	float Ixx = scale * (yy + zz);
	float Iyy = scale * (xx + zz);
	float Izz = scale * (xx + yy);

	Ibody.zero();
	Ibody.d[0][0] = Ixx;
	Ibody.d[1][1] = Iyy;
	Ibody.d[2][2] = Izz;

	dst_irb->set_mass( mass );
	dst_irb->set_inertial_tensor( Ibody );
	dst_irb->set_center_of_mass_in_object( center_of_mass );

	dst_iec->set_extents_tree( sphere_extent );
	
	src_irb->Release();
	dst_irb->Release();
	dst_iec->Release();
}

//--------------------------------------------------------------
// Disconnect parent, and children, and send it all flying
//--------------------------------------------------------------
static S32 bustOffChildren( IEngine *eng, IPhysics *phy, IModel *model, INSTANCE_INDEX inst_index, const Vector &icm, SINGLE strength, INSTANCE_INDEX *out_inst_indices, S32 max_num_out_indices )
{
	S32 num_out_indices_used = 0;

	INSTANCE_INDEX parent, child;
	JOINT_INDEX j;
	const Joint *jnt;
	Vector p, impulse;
	float mass;

	// disconnect from parent
	//
	if( (parent = model->get_parent( inst_index )) != INVALID_INSTANCE_INDEX ) {
		
		j = model->find_joint( parent, inst_index );
		jnt = model->get_joint(j);
		p = eng->get_transform( parent ) * jnt->parent_point;
		model->disconnect( parent, inst_index );
		
		impulse = p - icm;
		if( !impulse.magnitude() ) {
			impulse.set(rand(),rand(),rand());
		}
		impulse.normalize();
		impulse *= strength;
		
		phy->add_impulse_at_point( parent, impulse, p );
	}

	// disconnect children
	//
	while( (child = model->get_child( inst_index, -1 )) != INVALID_INSTANCE_INDEX ) {

		j = model->find_joint( inst_index, child );
		jnt = model->get_joint(j);
		p = eng->get_transform( child ) * jnt->child_point;
		model->disconnect( inst_index, child );
		
		if( (mass = phy->get_mass( child )) > 0.0f ) {
			
			impulse = (p - icm);
			if( !impulse.magnitude() ) {
				impulse.set(rand(),rand(),rand());
			}
			impulse.normalize();
			impulse *= strength;
			
			phy->add_impulse_at_point( child, impulse, p );
		}
		
		if( num_out_indices_used < max_num_out_indices ) {
			out_inst_indices[num_out_indices_used++] = child;
		}
	}
	
	return num_out_indices_used;
}

//-------------------------------------------------------------------------
// Snap off children and store in array
//-------------------------------------------------------------------------

static S32 detachChildren( INSTANCE_INDEX index, IModel *model, Joint *joints, S32 num_joints )
{
	S32 result = 0;

	INSTANCE_INDEX child = model->get_child( index );
	
	while( child != INVALID_INSTANCE_INDEX ) {

		JOINT_INDEX j = model->find_joint( index, child );
		const Joint * jnt = model->get_joint( j );
		Joint _jnt = *jnt;
		
		model->disconnect( index, child );
		
		if( result < num_joints ) {
			memcpy( &joints[result++], &_jnt, sizeof(Joint) );
		}
			
		child = model->get_child( index, -1 );
	}
	
	return result;
}

//

BOOL32 SplitInstance( IEngine *eng, INSTANCE_INDEX inst_index, const Vector& normal, SINGLE d, INSTANCE_INDEX *out_i0, INSTANCE_INDEX *out_i1 )
{
	COMPTR<IRenderer> rend;
	COMPTR<IModel> model;
	COMPTR<IPhysics> phy;
	
	if( FAILED( eng->QueryInterface( IID_IPhysics, phy ) ) ) {
		return FALSE;
	}

	if( FAILED( eng->QueryInterface( IID_IRenderer, rend ) ) ) {
		return FALSE;
	}

	if( FAILED( eng->QueryInterface( IID_IModel, model ) ) ) {
		return FALSE;
	}

	HARCH::ENGINE = eng;

	BOOL32 result = 0;
	Joint joints[16];
	U32 num_joints;
	Vector r;
	float dot, dx, dy, dz;

	ARCHETYPE_INDEX arch_index;
	ARCHETYPE_INDEX new_arch_index_0 ;
	ARCHETYPE_INDEX new_arch_index_1 ;
	INSTANCE_INDEX new_inst_index_0 = INVALID_INSTANCE_INDEX;
	INSTANCE_INDEX new_inst_index_1 = INVALID_INSTANCE_INDEX;

	Vector vel = phy->get_velocity( inst_index );
	Vector ang = phy->get_angular_velocity( inst_index );
	Vector com = phy->get_center_of_mass( inst_index );
	Transform trans ( eng->get_transform( inst_index ) );

	if( (arch_index = eng->get_archetype( inst_index )) == INVALID_ARCHETYPE_INDEX ) {
		return FALSE;
	}	

	new_arch_index_0 = eng->duplicate_archetype( arch_index, NULL );
	new_arch_index_1 = eng->duplicate_archetype( arch_index, NULL );

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	if( rend->split_archetype( arch_index, normal, d, new_arch_index_0, new_arch_index_0, SA_SPLIT_JAGGED, inst_index ) ) {
		eng->release_archetype( new_arch_index_0 );
		eng->release_archetype( new_arch_index_1 );

		return FALSE;
	}

	// detach all children
	//
	num_joints = detachChildren( inst_index, model, joints, 16 );

	// create and setup new instance 0
	//
	copy_archetype_rigid_body( eng, rend, phy, arch_index, new_arch_index_0, dx, dy, dz );

	if( (new_inst_index_0 = eng->create_instance2( new_arch_index_0, NULL )) != INVALID_INSTANCE_INDEX ) {
		eng->set_transform( new_inst_index_0, trans );
		r = phy->get_center_of_mass( new_inst_index_0 ) - com;
		phy->set_velocity( new_inst_index_0, vel + cross_product( ang, r ) );
		phy->set_angular_velocity( new_inst_index_0, ang );
	}

	// create and setup new instance 1
	//
	copy_archetype_rigid_body( eng, rend, phy, arch_index, new_arch_index_1, dx, dy, dz );

	if( (new_inst_index_1 = eng->create_instance2( new_arch_index_1, NULL )) == INVALID_INSTANCE_INDEX ) {
		eng->set_transform( new_inst_index_1, trans );
		r = phy->get_center_of_mass( new_inst_index_1 ) - com;
		phy->set_velocity( new_inst_index_1, vel + cross_product( ang, r ) );
		phy->set_angular_velocity( new_inst_index_1, ang );
	}
					
	// reconnect children to the correct pieces
	//
	for( U32 i=0; i<num_joints; i++ ) {
		dot = dot_product( normal, joints[i].parent_point ) + d;
		if( dot > 0.0f ) {
			joints[i].parent = new_inst_index_0;
			model->connect( &joints[i] );
		}
		else {
			joints[i].parent = new_inst_index_1;
			model->connect(&joints[i]);
		}
	}
	
	return TRUE;

}

//

static ArchNode * bustArchetype( IEngine *eng, IRenderer *render, INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index, S32 num_chunks )
{
	if( num_chunks <=1 ) {
		return new ArchNode( arch_index );
	}

	ARCHETYPE_INDEX new_arch_index_0;
	ARCHETYPE_INDEX new_arch_index_1;
	ArchNode *node, *tnode;
	Vector n, centroid;
	float dot;

	new_arch_index_0 = eng->duplicate_archetype( arch_index, NULL );
	new_arch_index_1 = eng->duplicate_archetype( arch_index, NULL );

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	n.x = -1.0 + rand() * 2.0 / RAND_MAX;
	n.y = -1.0 + rand() * 2.0 / RAND_MAX;
	n.z = -1.0 + rand() * 2.0 / RAND_MAX;
	n.normalize();

	if( render->get_archetype_centroid( arch_index, 1.0f, centroid ) ) {
			
		dot = -dot_product( centroid, n );

		if( render->split_archetype( arch_index, n, dot, new_arch_index_0, new_arch_index_1, SA_SPLIT_NONE, inst_index ) ) {

			node = bustArchetype( eng, render, INVALID_INSTANCE_INDEX, new_arch_index_0, num_chunks / 2 );

			ASSERT( node != NULL );

			for( tnode=node; tnode && tnode->next; tnode=tnode->next );
						
			tnode->next = bustArchetype( eng, render, INVALID_INSTANCE_INDEX, new_arch_index_1, num_chunks / 2 );
				
			ASSERT( tnode->next );

			return node;
		}
	}

	// when all else fails
	//
	return new ArchNode( arch_index );
}

//

S32 ExplodeInstance( IEngine* eng, INSTANCE_INDEX inst_index, SINGLE strength, S32 num_chunks, U32 num_array_entries, INSTANCE_INDEX* chunks )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( eng != NULL );

	COMPTR<IRenderer> rend;
	COMPTR<IModel> model;
	COMPTR<IPhysics> phy;
	
	if( FAILED( eng->QueryInterface( IID_IPhysics, phy ) ) ) {
		return 0;
	}

	if( FAILED( eng->QueryInterface( IID_IRenderer, rend ) ) ) {
		return FALSE;
	}

	if( FAILED( eng->QueryInterface( IID_IModel, model ) ) ) {
		return 0;
	}

	U32 output_spot;
	ARCHETYPE_INDEX arch_index;
	INSTANCE_INDEX new_inst_index;
	ArchNode *list, *node, *next;
	Vector icm, iv, iw, cm, offset, v, impulse, vert;
	float density, mass, dx, dy, dz;
	Transform xform(false);

	HARCH::ENGINE = eng;

	arch_index = eng->get_archetype( inst_index );

	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	if( (list = bustArchetype( eng, rend, inst_index, arch_index, num_chunks & ~(1) )) == NULL ) {
		eng->release_archetype( arch_index );
		return 0;
	}

	rend->get_archetype_centroid( arch_index, 1.0f, icm );

	output_spot = bustOffChildren( eng, phy, model, inst_index, icm, strength, chunks, num_array_entries );

	get_instance_pstate( eng, phy, rend, inst_index, arch_index, xform, iv, iw, mass, density );

	// traverse list of archetypes and create appropriate instances
	//
	for( node=list; node; node=next ) {

		if( output_spot < num_array_entries ) {

			chunks[output_spot] = new_inst_index = eng->create_instance2( node->hArch, NULL );

			ASSERT( INVALID_INSTANCE_INDEX != new_inst_index );

			rend->get_archetype_centroid( node->hArch, 1.0f, cm );
			offset = cm - icm;	// offset is in world space..

			copy_archetype_rigid_body( eng, rend, phy, arch_index, node->hArch, dx, dy, dz );

			eng->set_transform( new_inst_index, xform );
			phy->set_velocity( new_inst_index, iv + cross_product( iw, offset ) );
			phy->set_angular_velocity( inst_index, iw );
			
			add_instance_random_impulse( phy, new_inst_index, xform, offset, strength, dx, dy, dz );

			output_spot++;
		}

		next = node->next;
		delete node;
	}

	eng->release_archetype( arch_index );
	return output_spot;
}

//

struct StepBustLeaf
{
	ARCHETYPE_INDEX idx;
	INSTANCE_INDEX i_idx;
	U8 level;
	bool terminal:1;
	bool split:1;
	StepBustLeaf *next;
	
	//

	StepBustLeaf( INSTANCE_INDEX _ii, ARCHETYPE_INDEX _ai, U8 _level, bool _terminal, bool _split, StepBustLeaf *_next )
	{
		i_idx = _ii;
		idx = _ai;
		level = _level;
		terminal = _terminal;
		split = _split;
		next = _next;
	}
};

//

struct StepBustInfo
{
	StepBustLeaf *leafList;
	U8 levels;
	SINGLE strength;
	U8 totalChunks;
	INSTANCE_INDEX instIndex;
};

//
//returns true if process is done

BOOL32 StepBustArchetype( IEngine *eng, IRenderer *render, StepBustInfo *info )
{
	ARCHETYPE_INDEX arch_index;
	ARCHETYPE_INDEX new_arch_index_0;
	ARCHETYPE_INDEX new_arch_index_1;
	Vector n, centroid;
	float dot;
	U8 level;
	StepBustLeaf *leafPos, *new0, *new1;

	for( leafPos=info->leafList; leafPos && (leafPos->terminal || leafPos->split); leafPos=leafPos->next );

	if( leafPos == NULL ) {
		return TRUE;
	}

	arch_index = leafPos->idx;
	new_arch_index_0 = eng->duplicate_archetype( arch_index, NULL );
	new_arch_index_1 = eng->duplicate_archetype( arch_index, NULL );

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	n.x = -1.0 + rand() * 2.0 / RAND_MAX;
	n.y = -1.0 + rand() * 2.0 / RAND_MAX;
	n.z = -1.0 + rand() * 2.0 / RAND_MAX;
	n.normalize();

	if( render->get_archetype_centroid( arch_index, 1.0f, centroid ) ) {
		
		dot = -dot_product( centroid, n );

		if (render->split_archetype( arch_index, n, dot, new_arch_index_0, new_arch_index_1, SA_KEEP_NONE, leafPos->i_idx ) ) {

			level = leafPos->level + 1;

			//create new leafs
			new1 = new StepBustLeaf( INVALID_INSTANCE_INDEX, new_arch_index_1, level, (level==info->levels), 0, leafPos->next );
			new0 = new StepBustLeaf( INVALID_INSTANCE_INDEX, new_arch_index_0, level, (level==info->levels), 0, new1 );

			// insert into list
			leafPos->next = new0;
			leafPos->split = 1;
		}
		else {  
			leafPos->terminal = TRUE;
			eng->release_archetype( new_arch_index_0 );
			eng->release_archetype( new_arch_index_1 );
		}
	}

	return FALSE;
}

//

HEXPLODE StepExplodeInstance( IEngine* eng, INSTANCE_INDEX inst_index, SINGLE strength, S32 num_chunks )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	ASSERT( eng != NULL );
	
	StepBustInfo *info = new StepBustInfo;
	
	info->levels = 3;
	info->leafList = new StepBustLeaf( inst_index, eng->get_archetype( inst_index ), 0, FALSE, 0, NULL );
	info->strength = strength;
	info->instIndex = inst_index;

	return (HEXPLODE)info;
}

//

BOOL32 ContinueExplodeInstance( IEngine *eng, HEXPLODE info, S32 *num_fragments, INSTANCE_INDEX *chunks, U16 num_array_entries, BOOL32 execute )
{
	ASSERT( eng != NULL );
	ASSERT( info != 0 );

	COMPTR<IRenderer> rend;
	
	if( FAILED( eng->QueryInterface( IID_IRenderer, rend ) ) ) {
		return FALSE;
	}

	if( !execute ) {
		return StepBustArchetype( eng, rend, info ) ;
	}

	COMPTR<IModel> model;
	COMPTR<IPhysics> phy;
	INSTANCE_INDEX new_inst_index;
	ARCHETYPE_INDEX arch_index;
	Vector icm, iv, iw, cm, offset, v, impulse, vert;
	float density, mass, dx, dy, dz;
	Transform xform(false);
	StepBustLeaf *list, *node;

	while( StepBustArchetype( eng, rend, info ) == 0 ) {}

	if( (list = info->leafList) == NULL ) {
		return TRUE;
	}

	eng->QueryInterface( IID_IPhysics, phy ) ;
	eng->QueryInterface( IID_IModel, model ) ;

	ASSERT( phy != NULL ) ;
	ASSERT( model != NULL );

	icm = phy->get_center_of_mass( info->instIndex );

	*num_fragments = bustOffChildren( eng, phy, model, info->instIndex, icm, info->strength, chunks, num_array_entries );

	arch_index = eng->get_archetype( info->instIndex );

	get_instance_pstate( eng, phy, rend, info->instIndex, arch_index, xform, iv, iw, mass, density );

	for( node=list; node; node=node->next ) {
		
		if( *num_fragments < num_array_entries ) {
			break;
		}
		if( !node->terminal ) {
			continue;
		}

		new_inst_index = chunks[*num_fragments] = eng->create_instance2( node->idx, NULL );
		
		ASSERT( INVALID_INSTANCE_INDEX != new_inst_index );

		rend->get_archetype_centroid( node->idx, 1.0f, cm );
		offset = cm - icm;	// offset is in world space..

		copy_archetype_rigid_body( eng, rend, phy, arch_index, node->idx, dx, dy, dz );

		eng->set_transform( new_inst_index, xform );
		phy->set_velocity( new_inst_index, iv + cross_product( iw, offset ) );
		phy->set_angular_velocity( new_inst_index, iw );
		
		add_instance_random_impulse( phy, new_inst_index, xform, offset, info->strength, dx, dy, dz );

		(*num_fragments)++;
	}

	return TRUE;
}

//

void CloseExplodeHandle( IEngine *eng, HEXPLODE info )
{
	StepBustLeaf *node, *next;

	for( node=info->leafList; node; node=next ) {
		eng->release_archetype( node->i_idx );
		next = node->next;
		delete node;
	}
							
	delete info;
}