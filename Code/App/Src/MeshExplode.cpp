// $Header: /Conquest/App/Src/MeshExplode.cpp 32    10/26/00 3:36p Jasony $

#include "pch.h"
#include <Globals.h>

#include "CQTrace.h"
#include "fdump.h"
#include "TSmartPointer.h"

#include <Engine.h>
#include <Engine.h>
#include <Renderer.h>
#include <IRigidBody.h>
#include <IRigidBodyState.h>
#include <IExtentContainer.h>

#include "MeshExplode.h"
#include "MeshRender.h"

//

#include "ArchHolder.h"

//

#ifndef FAILED
#define FAILED(x) ((x) != GR_OK)
#endif


//

const float MIN_RB_BOX_DIM = 10.0f;
const SINGLE VOLUME_CHANGE_PCT = 0.0f;


SINGLE GetMass (MeshInfo *mi)
{
	float bbox[6];
	mi->GetBoundingBox(bbox);

	SINGLE dx,dy,dz;

	dx = __max( bbox[BBOX_MAX_X] - bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = __max( bbox[BBOX_MAX_Y] - bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = __max( bbox[BBOX_MAX_Z] - bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );

//	CQASSERT(5e-6*dx*dy*dz < 12000);

	return 5e-6*dx*dy*dz;
}

//RELATIVE CENTER OF MASS
Vector GetCenterOfMass (MeshInfo *mi)
{
	Vector result(0,0,0);
	float bbox[6];
	memset(bbox,0,sizeof(bbox));
//	if (REND->get_archetype_bounding_box(hArch,1.0,bbox) != 0)
//	{
		mi->GetBoundingBox(bbox);
		result.set((bbox[BBOX_MAX_X]+bbox[BBOX_MIN_X])*0.5,(bbox[BBOX_MAX_Y]+bbox[BBOX_MIN_Y])*0.5,(bbox[BBOX_MAX_Z]+bbox[BBOX_MIN_Z])*0.5);
//	}
	
	CQASSERT(result.magnitude() < 5000);
	return result;
}

//RELATIVE CENTER OF MASS
/*Vector GetCenterOfMass (INSTANCE_INDEX index)
{
	HARCH hArch;
	hArch = index;
	Vector result(0,0,0);
	float bbox[6];
	if (REND->get_archetype_bounding_box(hArch,1.0,bbox) != 0)
	{
		result.set((bbox[BBOX_MAX_X]+bbox[BBOX_MIN_X])*0.5,(bbox[BBOX_MAX_Y]+bbox[BBOX_MIN_Y])*0.5,(bbox[BBOX_MAX_Z]+bbox[BBOX_MIN_Z])*0.5);
	}
	
	CQASSERT(result.magnitude() < 5000);
	return result;
}*/

struct ArchNode
{
	ArchNode* next;
	HARCH hArch;		// idx;

	ArchNode (ARCHETYPE_INDEX i = INVALID_ARCHETYPE_INDEX)
	{
		if (i != INVALID_ARCHETYPE_INDEX)
		{
			hArch.setArchetype(i);
			ENGINE->hold_archetype(i);
		}
		next = NULL;
	}
};

//  This is done **sooo** much

inline void get_instance_pstate( INSTANCE_INDEX inst_index, 
								 ARCHETYPE_INDEX arch_index, 
								 Transform &T,
								 Vector &v, 
								 Vector &w, 
								 float &mass)//, 
							//	 float &density )
{
//	float bbox[6];

	T = ENGINE->get_transform( inst_index );

//	mass = dummy_get_mass( inst_index );
//	CQASSERT(mass < 25000.0);

/*	REND->get_archetype_bounding_box( arch_index, 1.0f, bbox );

	float dx = __max( bbox[BBOX_MAX_X] - bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	float dy = __max( bbox[BBOX_MAX_Y] - bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	float dz = __max( bbox[BBOX_MAX_Z] - bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );*/

//	float volume = dx * dy * dz;
//	density = mass / (volume + VOLUME_CHANGE_PCT / 100.0f * volume);
		
	v = ENGINE->get_velocity( inst_index );
	w = ENGINE->get_angular_velocity( inst_index );
}

//

inline void add_instance_random_impulse( INSTANCE_INDEX inst_index, Transform &T, Vector &along, float strength, const Vector &delta, const PHYS_CHUNK &phys)
{
	Vector impulse;

	impulse = along;

	SINGLE mag = impulse.magnitude();
	if( mag == 0.0 ) {
		//this may be lame, but it's crash safe
		impulse.set( 50.0, rand()%100, rand()%100 );
		if (rand()%2)
			impulse *= -1;
		mag = impulse.magnitude();
	}

	SINGLE speed = (500.0/phys.mass);
	speed = __min(speed,50.0f);
	SINGLE factor = __min(speed*strength,1500.0f);
	impulse *= factor/mag;

	Vector vel;
	vel = ENGINE->get_velocity(inst_index);
	ENGINE->set_velocity(inst_index,vel+impulse);

	//Sean's clever math thing
	if (vel.z < 1e-6)
		vel.z = 0.01;
	
	Vector ang;
	vel.normalize();
	U32 a=rand()%100-50;
	U32 b=rand()%100-50;
	if (b==0)
		b = 1;
	SINGLE c = -(a*vel.x+b*vel.y)/vel.z;
	ang.set(a,b,c);
	ang.normalize();

	ang *= 0.5*speed*(2+rand()%10);
	ENGINE->set_angular_velocity(inst_index,ang);
}

#if 0
void copy_archetype_rigid_body( ARCHETYPE_INDEX src_arch_index,
								ARCHETYPE_INDEX dst_arch_index,
								Vector &out_delta,INSTANCE_INDEX id)

{
	ASSERT( src_arch_index != INVALID_ARCHETYPE_INDEX );
	ASSERT( dst_arch_index != INVALID_ARCHETYPE_INDEX );
	
	IRigidBodyState *src_irb=0, *dst_irb;
	IExtentContainer *dst_iec;
	
	if (id != INVALID_INSTANCE_INDEX)
		ENGINE->query_instance_interface( id, IID_IRigidBodyState, (IDAComponent**)&src_irb ) ;

	if (src_irb == 0)
		ENGINE->query_archetype_interface( src_arch_index, IID_IRigidBody, (IDAComponent**)&src_irb ) ;
	
	
	ENGINE->query_archetype_interface( dst_arch_index, IID_IRigidBody, (IDAComponent**)&dst_irb ) ;
	ENGINE->query_archetype_interface( dst_arch_index, IID_IExtentContainer, (IDAComponent**)&dst_iec ) ;

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
	

	REND->get_archetype_bounding_box( src_arch_index, 1.0f, src_bbox );
		
	dx = __max( src_bbox[BBOX_MAX_X] - src_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = __max( src_bbox[BBOX_MAX_Y] - src_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = __max( src_bbox[BBOX_MAX_Z] - src_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );
	
	volume = dx * dy * dz;
	SINGLE o_mass;
	o_mass = src_irb->get_mass();

	CQASSERT(o_mass < 25000);
	density = o_mass / (volume + VOLUME_CHANGE_PCT / 100.0f * volume);
		

	REND->get_archetype_bounding_box( dst_arch_index, 1.0f, dst_bbox );

	dx = out_delta.x = __max( dst_bbox[BBOX_MAX_X] - dst_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = out_delta.y = __max( dst_bbox[BBOX_MAX_Y] - dst_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = out_delta.z = __max( dst_bbox[BBOX_MAX_Z] - dst_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );
		
	mass = density * dx * dy * dz ;

	//this is gonna have to be out because of objects whose children outweigh them.
	//CQASSERT(mass-o_mass < 1e-4);

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
#endif
/*
void copy_phys_chunk(ARCHETYPE_INDEX src_arch_index,
								ARCHETYPE_INDEX dst_arch_index,
								PHYS_CHUNK *chunk)
{
	float dst_bbox[6];

	REND->get_archetype_bounding_box( dst_arch_index, 1.0f, dst_bbox );
	
	// calc center of mass
	//
	chunk->arm.x = -(dst_bbox[BBOX_MAX_X] + dst_bbox[BBOX_MIN_X]) / 2.0;
	chunk->arm.y = -(dst_bbox[BBOX_MAX_Y] + dst_bbox[BBOX_MIN_Y]) / 2.0;
	chunk->arm.z = -(dst_bbox[BBOX_MAX_Z] + dst_bbox[BBOX_MIN_Z]) / 2.0;

	SINGLE dx,dy,dz;

	dx = __max( dst_bbox[BBOX_MAX_X] - dst_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = __max( dst_bbox[BBOX_MAX_Y] - dst_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = __max( dst_bbox[BBOX_MAX_Z] - dst_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );

	chunk->mass = 1e-5*dx*dy*dz;
}*/

void create_phys_chunk(MeshInfo *mi,PHYS_CHUNK *chunk)
{
	float dst_bbox[6];
	memset(dst_bbox,0,sizeof(dst_bbox));

	// calc center of mass
	//
	mi->GetBoundingBox(dst_bbox);
	chunk->arm.x = -(dst_bbox[BBOX_MAX_X] + dst_bbox[BBOX_MIN_X]) / 2.0;
	chunk->arm.y = -(dst_bbox[BBOX_MAX_Y] + dst_bbox[BBOX_MIN_Y]) / 2.0;
	chunk->arm.z = -(dst_bbox[BBOX_MAX_Z] + dst_bbox[BBOX_MIN_Z]) / 2.0;

	SINGLE dx,dy,dz;

	dx = __max( dst_bbox[BBOX_MAX_X] - dst_bbox[BBOX_MIN_X], MIN_RB_BOX_DIM );
	dy = __max( dst_bbox[BBOX_MAX_Y] - dst_bbox[BBOX_MIN_Y], MIN_RB_BOX_DIM );
	dz = __max( dst_bbox[BBOX_MAX_Z] - dst_bbox[BBOX_MIN_Z], MIN_RB_BOX_DIM );

	chunk->mass = 1e-5*dx*dy*dz;
	//chunk->mass = 1e-5*mi->radius*mi->radius*mi->radius;
}
//--------------------------------------------------------------
// Disconnect parent, and children, and send it all flying
//--------------------------------------------------------------
/*static S32 bustOffChildren( INSTANCE_INDEX inst_index, const Vector &icm, SINGLE strength, INSTANCE_INDEX *out_inst_indices, S32 max_num_out_indices,struct PHYS_CHUNK * phys)
{
	S32 num_out_indices_used = 0;

	INSTANCE_INDEX parent, child;
//	JOINT_INDEX j;
	const JointInfo *jnt;
	Vector p, impulse;
//	float mass;

	// disconnect from parent
	//
	if( (parent = ENGINE->get_instance_parent( inst_index )) != INVALID_INSTANCE_INDEX ) {
		
		//j = MODEL->find_joint( parent, inst_index );
		jnt = ENGINE->get_joint_info(inst_index);
		p = ENGINE->get_transform( parent ) * jnt->parent_point;
		ENGINE->destroy_joint( parent, inst_index );
		
		impulse = p - icm;
		if( !impulse.magnitude() ) {
			impulse.set(rand(),rand(),rand());
		}
		impulse.normalize();
		impulse *= strength;
		
//		PHYSICS->add_impulse_at_point( parent, impulse, p );
	}

	// disconnect children
	//
	child = -1;
	while( (child = ENGINE->get_instance_child_next( inst_index, EN_DONT_RECURSE, child )) != INVALID_INSTANCE_INDEX ) {
		
		//only do this if this is part of the mesh
		Mesh *mesh = REND->get_instance_mesh(child);
		if (mesh)
		{
			//	j = MODEL->find_joint( inst_index, child );
			jnt = ENGINE->get_joint_info(child);
			
			Transform trans = ENGINE->get_transform( child);
			
			if (jnt->type == JT_FIXED)
			{
				Vector ppos = ENGINE->get_position(inst_index);
				p = jnt->rel_position;
				p += ppos;
			}
			else
			{
				Vector pt;
				pt = jnt->child_point;
				p = trans * pt;
			}
			
			Vector cm = GetCenterOfMass(child);  //relative
			cm = trans * cm;
			impulse = (cm-p);
			
			//PHYSICS->add_impulse_at_point( child, impulse, p );
			//Transform T = ENGINE->get_transform(child);
			//add_instance_random_impulse(child,T,impulse,mass*(strength/10000.0),p);
			
			//try to calculate a rotation that will vary based on mass, but not go crazy
			SINGLE stren = 0.2;//4e-12*strength*(12000.0-mass);
			
			//tweak force
			S32 tweak = rand()%10-5;
			
			Vector vel(1+rand()%99,rand()%100,rand()%100);
			vel.normalize();
			SINGLE mag = impulse.magnitude();
			if (mag != 0.0)
				impulse /= mag;
			vel = vel+impulse;
			
			vel *= mag*(0.5+0.05*tweak)*stren;
			ENGINE->set_velocity(child,vel);
			//	CQASSERT(vel.magnitude() < 2500);
			
			//Sean's clever math thing
			if (vel.z < 1e-6)
				vel.z = 0.01;
			
			Vector ang;
			vel.normalize();
			U32 a=rand()%100-50;
			U32 b=rand()%100-50;
			if (b==0)
				b =1;
			SINGLE c = -(a*vel.x+b*vel.y)/vel.z;
			ang.set(a,b,c);
			ang.normalize();
			
			//tweak force
			tweak = rand()%10-5;
			
			ang *= 20*(1+0.1*tweak)*stren;
			
			ENGINE->set_angular_velocity(child,ang);
			
			if( num_out_indices_used < max_num_out_indices ) {
				out_inst_indices[num_out_indices_used] = child;
				phys[num_out_indices_used].arm = -GetCenterOfMass(child);
				phys[num_out_indices_used].mass = GetMass(child);
				num_out_indices_used++;
			}
		}
	}
	
	for (int i=0;i<num_out_indices_used;i++)
		ENGINE->destroy_joint( inst_index, out_inst_indices[i] );

	return num_out_indices_used;
}*/

//--------------------------------------------------------------
// Disconnect parent, and children, and send it all flying
//--------------------------------------------------------------
static S32 bustOffChildren( MeshChain *src_mc, const Vector &icm, SINGLE strength, 
						   MeshChain **out_mc, struct PHYS_CHUNK * phys,int num_array_entries)
{
	S32 num_out_indices_used = 0;

//	JOINT_INDEX j;
	Vector p, impulse;
//	float mass;

	// disconnect from parent  -temporarily commented?  am I really going to bust myself off my parent?
	//
/*	INSTANCE_INDEX inst_index = src_mc->mi[0]->instanceIndex;
	if( (parent = ENGINE->get_instance_parent( inst_index )) != INVALID_INSTANCE_INDEX ) {
		
		//j = MODEL->find_joint( parent, inst_index );
		jnt = ENGINE->get_joint_info(inst_index);
		p = ENGINE->get_transform( parent ) * jnt->parent_point;
		ENGINE->destroy_joint( parent, inst_index );
		
		impulse = p - icm;
		if( !impulse.magnitude() ) {
			impulse.set(rand(),rand(),rand());
		}
		impulse.normalize();
		impulse *= strength;
		
//		PHYSICS->add_impulse_at_point( parent, impulse, p );
	}*/

	// disconnect children
	//

	Transform parentTrans=ENGINE->get_transform(src_mc->mi[0]->instanceIndex);
	for (int cnt=1;cnt<src_mc->numChildren;cnt++)
	{
		INSTANCE_INDEX child = src_mc->mi[cnt]->instanceIndex;
		INSTANCE_INDEX parentID = ENGINE->get_instance_parent(child);
		if (parentID == src_mc->mi[0]->instanceIndex || parentID == -1) //if I am one of the first children of the parent
		{
			if (num_out_indices_used < num_array_entries)
			{
				
				//jnt = ENGINE->get_joint_info(child);
				
				Transform trans = ENGINE->get_transform( child);
				
				/*	if (jnt->type == JT_FIXED)
				{
				Vector ppos = ENGINE->get_position(inst_index);
				p = jnt->rel_position;
				p += ppos;
				}
				else
				{
				Vector pt;
				pt = jnt->child_point;
				p = trans * pt;
			}*/
				
				//Vector cm = GetCenterOfMass(child);  //relative
				//cm = trans * cm;
				
				impulse = (trans.translation-parentTrans.translation);
				/*if( !impulse.magnitude() ) {
				impulse.set(rand(),rand(),rand());
				}
				impulse.normalize();
				impulse *= strength;*/
				
				//PHYSICS->add_impulse_at_point( child, impulse, p );
				//Transform T = ENGINE->get_transform(child);
				//add_instance_random_impulse(child,T,impulse,mass*(strength/10000.0),p);
				
				//try to calculate a rotation that will vary based on mass, but not go crazy
				SINGLE stren = 0.2;//4e-12*strength*(12000.0-mass);
				
				//tweak force
				S32 tweak = rand()%10-5;
				
				Vector vel(1+rand()%99,rand()%100,rand()%100);
				vel.normalize();
				SINGLE mag = impulse.magnitude();
				if (mag != 0.0)
					impulse /= mag;
				vel = vel+impulse;
				
				vel *= mag*(0.5+0.05*tweak)*stren;
				ENGINE->set_velocity(child,vel);
				//	CQASSERT(vel.magnitude() < 2500);
				
				//Sean's clever math thing
				if (vel.z < 1e-6)
					vel.z = 0.01;
				
				Vector ang;
				vel.normalize();
				U32 a=rand()%100-50;
				U32 b=rand()%100-50;
				if (b==0)
					b =1;
				SINGLE c = -(a*vel.x+b*vel.y)/vel.z;
				ang.set(a,b,c);
				ang.normalize();
				
				//tweak force
				tweak = rand()%10-5;
				
				ang *= 20*(1+0.1*tweak)*stren;
				
				ENGINE->set_angular_velocity(child,ang);
				
				//if we have gotten here, we are a first child of the parent
				CQASSERT(src_mc->mi[cnt]);
				
				out_mc[num_out_indices_used] = new MeshChain;
				out_mc[num_out_indices_used]->bOwnsChildren = true;
				if (src_mc->mi[cnt]->parent)
				{
					//we are part of a meshinfotree
					//fill out out_mc with all my children
					out_mc[num_out_indices_used]->numChildren = src_mc->mi[cnt]->parent->ListChildren(out_mc[num_out_indices_used]->mi);
					//take everyone out of the mesh info tree
					for (int q=0;q<out_mc[num_out_indices_used]->numChildren;q++)
					{
						out_mc[num_out_indices_used]->mi[q]->parent->LoseOwnershipOfMeshInfo();
						out_mc[num_out_indices_used]->mi[q]->parent = 0;
					}
				}
				else
				{
					//we are not part of a meshinfotree
					//so do this instead of a ListChildren
					out_mc[num_out_indices_used]->mi[0] = src_mc->mi[cnt];
					out_mc[num_out_indices_used]->numChildren = 1;
				}
				
				out_mc[num_out_indices_used]->bOwnsChildren = true;
				phys[num_out_indices_used].arm = -GetCenterOfMass(src_mc->mi[cnt]);
				phys[num_out_indices_used].mass = GetMass(src_mc->mi[cnt]);
				//take this MeshInfo out of the MeshChain
				src_mc->mi[cnt] = 0;
				num_out_indices_used++;
			}
			else  //not enough slots
			{
				//these don't need to be detached but their MeshInfo's need to be cleaned up

				if (src_mc->mi[cnt]->parent)
				{
					//IMeshInfoTree *temp_tree;
					
					//we are part of a meshinfotree
			//		src_mc->mi[cnt]->parent->DetachChild(child,&temp_tree);  //this can't work right now
					//DestroyMeshInfoTree(temp_tree);

					//why do I have to do this?  because I can't figure out who my real parent is
					//"parent" just means "treeNode"
				/*	MeshChain temp_mc;
					temp_mc.numChildren = src_mc->mi[cnt]->parent->ListChildren(temp_mc.mi);
					//take everyone out of the mesh info tree
					for (int q=0;q<temp_mc.numChildren;q++)
					{
						temp_mc.mi[q]->parent->LoseOwnershipOfMeshInfo();
						temp_mc.mi[q]->parent = 0;
						delete temp_mc.mi[q];
					}*/

					//just do me
					src_mc->mi[cnt]->parent->LoseOwnershipOfMeshInfo();
					src_mc->mi[cnt]->parent = 0;
					delete src_mc->mi[cnt];
					
				}
				else
				{
					//	CQASSERT(src_mc->mi[cnt]->bOwnsInstance);  //if not, who owns me?

					//ok the idea now is that if I am owned by a mesh tree, and I am here, i need to be
					//detached and disposed of
					INSTANCE_INDEX parentID = ENGINE->get_instance_parent(src_mc->mi[cnt]->instanceIndex);
					if (parentID != -1)
						ENGINE->destroy_joint(parentID,src_mc->mi[cnt]->instanceIndex );
					src_mc->mi[cnt]->bOwnsInstance = true;

					delete src_mc->mi[cnt];
				}
				//we can null them just for fun
				src_mc->mi[cnt] = 0;
			}
		}
	}

	src_mc->numChildren = 1;
	
	//detach every last child from its parent object
	for (int i=0;i<num_out_indices_used;i++)
		for (int j=0;j<out_mc[i]->numChildren;j++)
		{
			INSTANCE_INDEX parentID = ENGINE->get_instance_parent(out_mc[i]->mi[j]->instanceIndex);
			if (parentID != -1)
				ENGINE->destroy_joint(parentID,out_mc[i]->mi[j]->instanceIndex );
			out_mc[i]->mi[j]->bOwnsInstance = true;
		}

	return num_out_indices_used;
}
//-------------------------------------------------------------------------
// Snap off children and store in array
//-------------------------------------------------------------------------

static S32 detachChildren( INSTANCE_INDEX index, JointInfo *joints, INSTANCE_INDEX *child_indices,S32 num_joints )
{
	S32 result = 0;

	INSTANCE_INDEX child = ENGINE->get_instance_child_next( index ,0,INVALID_INSTANCE_INDEX);	

	while( child != INVALID_INSTANCE_INDEX ) {

	//	JOINT_INDEX j = MODEL->find_joint( index, child );
		const JointInfo * jnt = ENGINE->get_joint_info( child );
		JointInfo _jnt = *jnt;
		
		//This is ok cause we're gonna "start over" below
		ENGINE->destroy_joint( index, child );
		
		if( result < num_joints ) {
			memcpy( &joints[result], &_jnt, sizeof(JointInfo) );
			child_indices[result] = child;
			result++;
		}
			
		//start enumeration over
		child = ENGINE->get_instance_child_next( index,0, -1);
	}
	
	return result;
}

//
#if 0
BOOL32 SplitInstance( INSTANCE_INDEX inst_index, const Vector& normal, SINGLE d, INSTANCE_INDEX *out_i0, INSTANCE_INDEX *out_i1, PHYS_CHUNK *phys0,PHYS_CHUNK *phys1)
{
//	BOOL32 result = 0;
	JointInfo joints[16];
	INSTANCE_INDEX child_indices[16];
	U32 num_joints;
	Vector r;
	float dot;
	Vector delta;

	HARCH arch_index;
	HARCH new_arch_index_0 ;
	HARCH new_arch_index_1 ;
	INSTANCE_INDEX new_inst_index_0 = INVALID_INSTANCE_INDEX;
	INSTANCE_INDEX new_inst_index_1 = INVALID_INSTANCE_INDEX;

	Vector vel = ENGINE->get_velocity( inst_index );
	Vector ang = ENGINE->get_angular_velocity( inst_index );
	Vector com = GetCenterOfMass( inst_index );
	Transform trans ( ENGINE->get_transform( inst_index ) );

	if( (arch_index.setArchetype(ENGINE->get_instance_archetype( inst_index ))) == INVALID_ARCHETYPE_INDEX ) {
		return FALSE;
	}	

	new_arch_index_0.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));
	new_arch_index_1.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	if( REND->split_archetype( arch_index, normal, d, new_arch_index_0, new_arch_index_1, SA_SPLIT_JAGGED, inst_index ) == 0) {
		ENGINE->release_archetype( new_arch_index_0 );
		ENGINE->release_archetype( new_arch_index_1 );

		return FALSE;
	}

	// detach all children
	//
	num_joints = detachChildren( inst_index, joints, child_indices, 16 );

	// create and setup new instance 0
	//
//	copy_archetype_rigid_body( arch_index, new_arch_index_0, delta, inst_index );
	copy_phys_chunk( arch_index, new_arch_index_0,phys0);

	if( (new_inst_index_0 = ENGINE->create_instance2( new_arch_index_0, NULL )) != INVALID_INSTANCE_INDEX ) {
		ENGINE->set_transform( new_inst_index_0, trans );
		r = GetCenterOfMass( new_inst_index_0 ) - com;
		ENGINE->set_velocity( new_inst_index_0, vel + cross_product( ang, r ) );
		ENGINE->set_angular_velocity( new_inst_index_0, ang );
	}

	// create and setup new instance 1
	//
//	copy_archetype_rigid_body( arch_index, new_arch_index_1, delta, inst_index );
	copy_phys_chunk( arch_index, new_arch_index_1,phys1);

	if( (new_inst_index_1 = ENGINE->create_instance2( new_arch_index_1, NULL )) != INVALID_INSTANCE_INDEX ) {
		ENGINE->set_transform( new_inst_index_1, trans );
		r = GetCenterOfMass( new_inst_index_1 ) - com;
		ENGINE->set_velocity( new_inst_index_1, vel + cross_product( ang, r ) );
		ENGINE->set_angular_velocity( new_inst_index_1, ang );
	}
					
	// reconnect children to the correct pieces
	//
	for( U32 i=0; i<num_joints; i++ ) {
		dot = dot_product( normal, joints[i].parent_point ) + d;
		if( dot > 0.0f ) {
			ENGINE->create_joint(new_inst_index_0, child_indices[i], &joints[i] );
		}
		else {
			ENGINE->create_joint(new_inst_index_1, child_indices[i], &joints[i]);
		}
	}

	*out_i0 = new_inst_index_0;
	*out_i1 = new_inst_index_1;

	return TRUE;

}
#endif
//
//I hereby define normal to point in the direction of 1
#if 1
BOOL32 SplitInstance( INSTANCE_INDEX index,IMeshInfoTree *tree, const Vector& normal, SINGLE d, IMeshInfoTree **out_tree0, IMeshInfoTree **out_tree1, PHYS_CHUNK *phys0,PHYS_CHUNK *phys1)
{
	MeshInfo *mi = tree->GetMeshInfo();
//	BOOL32 result = 0;
	JointInfo joints[16];
	INSTANCE_INDEX child_indices[16];
	//MeshInfo *mi_children[16];
	U32 num_joints;
	Vector r;
	float dot;
	Vector delta;

	HARCH arch_index;
//	HARCH new_arch_index_0 ;
//	HARCH new_arch_index_1 ;
	INSTANCE_INDEX new_inst_index_0 = INVALID_INSTANCE_INDEX;
	INSTANCE_INDEX new_inst_index_1 = INVALID_INSTANCE_INDEX;

	Vector vel = ENGINE->get_velocity( index );
	Vector ang = ENGINE->get_angular_velocity( index );

	Vector com = GetCenterOfMass( mi );
	Transform trans ( ENGINE->get_transform( index ) );

	if( (arch_index.setArchetype(ENGINE->get_instance_archetype( index ))) == INVALID_ARCHETYPE_INDEX ) {
		return FALSE;
	}	

/*	new_arch_index_0.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));
	new_arch_index_1.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );*/


	// detach all children
	//
	num_joints = detachChildren( index, joints, child_indices, 16 );

	// create and setup new instance 0
	//
//	copy_archetype_rigid_body( arch_index, new_arch_index_0, delta, mi.instanceIndex );
//	copy_phys_chunk( arch_index, new_arch_index_0,phys0);

	new_inst_index_0 = ENGINE->create_instance2( arch_index, NULL );
	CQASSERT(new_inst_index_0 != INVALID_INSTANCE_INDEX);

	// create and setup new instance 1
	//
//	copy_archetype_rigid_body( arch_index, new_arch_index_1, delta, mi.instanceIndex );
//	copy_phys_chunk( arch_index, new_arch_index_1,phys1);

	new_inst_index_1 = ENGINE->create_instance2( arch_index, NULL );
	CQASSERT(new_inst_index_1 != INVALID_INSTANCE_INDEX);


//	int num_mi_children;
	IMeshInfoTree *mi_children[MAX_CHILDS];
//	IMeshInfoTree *mi_children_by_id[MAX_CHILDS];

//	num_mi_children = tree->ListFirstChildren(mi_children);

	/*U32 i;
	for (i=0;i<num_joints;i++)
	{
		int j;
		for (j=0;j<num_mi_children;j++)
		{
			if (mi_children[j] && mi_children[j]->instanceIndex == child_indices[i])
			{
				mi_children_by_id[i] = mi_children[j];
			}
		}
	}*/

	*out_tree0 = CreateMeshInfoTree(new_inst_index_0);
	*out_tree1 = CreateMeshInfoTree(new_inst_index_1);
					
	// reconnect children to the correct pieces
	//
	for(U32 i=0; i<num_joints; i++ ) {
		dot = dot_product( normal, joints[i].parent_point ) + d;
		
		if( dot > 0.0f ) {
			ENGINE->create_joint(new_inst_index_0, child_indices[i], &joints[i] );
			tree->DetachChild(child_indices[i],&mi_children[i]);
			(*out_tree0)->AttachChild(new_inst_index_0,mi_children[i]);
		}
		else {
			ENGINE->create_joint(new_inst_index_1, child_indices[i], &joints[i]);
			tree->DetachChild(child_indices[i],&mi_children[i]);
			(*out_tree1)->AttachChild(new_inst_index_1,mi_children[i]);
		}
	}

	MeshInfo *out_mi0 = (*out_tree0)->GetMeshInfo();
	MeshInfo *out_mi1 = (*out_tree1)->GetMeshInfo();

	SplitMeshInfo(*mi,*out_mi0,*out_mi1,d,normal);
	out_mi0->CalculateSphere();
	out_mi1->CalculateSphere();

	create_phys_chunk(out_mi0,phys0);
	create_phys_chunk(out_mi1,phys1);

	ENGINE->set_transform( new_inst_index_0, trans );
	r = GetCenterOfMass( out_mi0 ) - com;
	ENGINE->set_velocity( new_inst_index_0, vel + cross_product( ang, r ) );
	ENGINE->set_angular_velocity( new_inst_index_0, ang );

	ENGINE->set_transform( new_inst_index_1, trans );
	r = GetCenterOfMass( out_mi1 ) - com;
	ENGINE->set_velocity( new_inst_index_1, vel + cross_product( ang, r ) );
	ENGINE->set_angular_velocity( new_inst_index_1, ang );

	return TRUE;

}
#endif

static ArchNode * bustArchetype( INSTANCE_INDEX inst_index, ARCHETYPE_INDEX arch_index, S32 num_chunks )
{
	if( num_chunks <=1 ) {
		return new ArchNode( arch_index );
	}

	HARCH new_arch_index_0;
	HARCH new_arch_index_1;
	ArchNode *node, *tnode;
	Vector n, centroid;
	float dot;

	new_arch_index_0.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));
	new_arch_index_1.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	n.x = -1.0 + rand() * 2.0 / RAND_MAX;
	n.y = -1.0 + rand() * 2.0 / RAND_MAX;
	n.z = -1.0 + rand() * 2.0 / RAND_MAX;
	n.normalize();

	if( REND->get_archetype_centroid( arch_index, 1.0f, centroid ) ) {
			
		dot = -dot_product( centroid, n );

		if( REND->split_archetype( arch_index, n, dot, new_arch_index_0, new_arch_index_1, SA_SPLIT_NONE, inst_index ) ) {

			node = bustArchetype(INVALID_INSTANCE_INDEX, new_arch_index_0, num_chunks / 2 );

			ASSERT( node != NULL );

			for( tnode=node; tnode && tnode->next; tnode=tnode->next );
						
			tnode->next = bustArchetype( INVALID_INSTANCE_INDEX, new_arch_index_1, num_chunks / 2 );
				
			ASSERT( tnode->next );

			return node;
		}
	}

	// when all else fails
	//
	return new ArchNode( arch_index );
}

#if 1
//
static int bustMeshInfo( MeshInfo *src_mi, MeshChain **out_mc, S32 num_chunks )
{
	if( num_chunks <=1 ) {
		out_mc[0] = new MeshChain;
		out_mc[0]->numChildren = 1;
		out_mc[0]->mi[0] = src_mi;
		return 1;
	}

	int pieces=0;
	Vector n, centroid;
	float dot;

	n.x = -1.0 + rand() * 2.0 / RAND_MAX;
	n.y = -1.0 + rand() * 2.0 / RAND_MAX;
	n.z = -1.0 + rand() * 2.0 / RAND_MAX;
	n.normalize();
	
	centroid = src_mi->sphere_center;
	
	dot = -dot_product( centroid, n );
	
	MeshInfo *m0,*m1;
	m0 = new MeshInfo;
	m1 = new MeshInfo;
	SplitMeshInfo(*src_mi,*m0,*m1,dot,n);
	
	HARCH tempArch(src_mi->instanceIndex);
	m0->instanceIndex = ENGINE->create_instance2(tempArch,NULL);
	m1->instanceIndex = ENGINE->create_instance2(tempArch,NULL);
	m0->bOwnsInstance = true;
	m1->bOwnsInstance = true;
	m0->radius = src_mi->radius; //temp
	m1->radius = src_mi->radius;
	m0->sphere_center = src_mi->sphere_center;
	m1->sphere_center = src_mi->sphere_center;

	if (1) //if split worked ok - no check yet for that
	{
		int temp;
		temp = bustMeshInfo( m0, &out_mc[pieces], num_chunks / 2);
		if (temp != 1)
		{
			//there has got to be a better way
			delete m0;
		}
		pieces += temp;
		temp = bustMeshInfo( m1, &out_mc[pieces], num_chunks / 2);
		if (temp !=  1)
		{
			delete m1;
		}
		pieces += temp;

		return pieces;
	}

	out_mc[0] = new MeshChain;
	out_mc[0]->numChildren = 1;
	out_mc[0]->mi[0] = src_mi;
	return 1;
}
#endif
//////
/*
S32 ExplodeInstance ( INSTANCE_INDEX inst_index, SINGLE strength, U32 num_chunks, U32 num_array_entries, INSTANCE_INDEX* chunks, struct PHYS_CHUNK * phys)
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	U32 output_spot;
	HARCH arch_index;
	INSTANCE_INDEX new_inst_index;
	ArchNode *list, *node, *next;
	Vector icm, iv, iw, cm, offset, v, impulse, vert;
	float mass;
	Vector delta;
	Transform xform(false);

	//overloaded operator
	arch_index = inst_index ;

	ASSERT( arch_index != INVALID_ARCHETYPE_INDEX );

	if( (list = bustArchetype(inst_index, arch_index, num_chunks & ~(1) )) == NULL )
		return 0;

	REND->get_archetype_centroid( arch_index, 1.0f, icm );

	output_spot = bustOffChildren( inst_index, icm, strength, chunks, num_array_entries, phys);

	get_instance_pstate( inst_index, arch_index, xform, iv, iw, mass);//, density );

	// traverse list of archetypes and create appropriate instances
	//
	for( node=list; node; node=next ) {

		if( output_spot < num_array_entries ) {
			REND->get_archetype_centroid( node->hArch, 1.0f, cm );
			offset = cm - icm;	// offset is in object space here

			//world space now
			offset = xform.rotate(offset);

//			copy_archetype_rigid_body( arch_index, node->hArch, delta,inst_index );
			copy_phys_chunk( arch_index, node->hArch,&phys[output_spot]);

			chunks[output_spot] = new_inst_index = ENGINE->create_instance2( node->hArch, NULL );

			CQASSERT( INVALID_INSTANCE_INDEX != new_inst_index );


			ENGINE->set_transform( new_inst_index, xform );
			
			ENGINE->set_velocity( new_inst_index, iv + cross_product( iw, offset ) );

			ENGINE->set_angular_velocity( new_inst_index, iw );
			
			add_instance_random_impulse( new_inst_index, xform, offset, strength, delta ,phys[output_spot]);

			output_spot++;
		}

		next = node->next;
		delete node;
	}

	return output_spot;
}
*/
S32 ExplodeInstance ( IMeshInfoTree *tree, SINGLE strength, U32 num_chunks, U32 num_array_entries, 
					MeshChain ** chunks, struct PHYS_CHUNK * phys)
{
	MeshChain src_mc;
	src_mc.numChildren = tree->ListChildren(src_mc.mi);

	S32 result = ExplodeInstance(&src_mc, strength, num_chunks,num_array_entries, 
					chunks,phys);

	tree->LoseOwnershipOfMeshInfo();

	DestroyMeshInfoTree(tree);

	return result;
}

S32 ExplodeInstance ( MeshChain *mc, SINGLE strength, U32 num_chunks, U32 num_array_entries, 
					MeshChain ** chunks, struct PHYS_CHUNK * phys)
{
	INSTANCE_INDEX inst_index = mc->mi[0]->instanceIndex;
	HARCH dummyArch(inst_index);
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );

	U32 output_spot;
	Vector icm, iv, iw, cm, offset, v, impulse, vert;

	iv = ENGINE->get_velocity( inst_index );
	iw = ENGINE->get_angular_velocity( inst_index );

//	float mass;
	Vector delta;
	Transform xform(false);

	icm = mc->mi[0]->sphere_center;

	output_spot = bustMeshInfo(mc->mi[0],chunks,num_chunks & ~(1));
	CQASSERT(output_spot <= num_chunks);

	xform = ENGINE->get_transform(mc->mi[0]->instanceIndex);

	U32 i;
	for (i=0;i<output_spot;i++)
	{
		ENGINE->set_transform(chunks[i]->mi[0]->instanceIndex,xform);
	}

	output_spot += bustOffChildren( mc, icm, strength, &chunks[output_spot], phys, num_array_entries-output_spot);

	//we're done with the source.  kill it.  If this chain is actually a tree, the others will be
	//destroyed in the calling function
	//this will also destroy all the engine instances
	//what if it's not though?
	mc->mi[0]->bOwnsInstance = true;
	delete mc->mi[0];

	for (i=0;i<output_spot;i++)
	{
		chunks[i]->mi[0]->CalculateSphere();

		cm = chunks[i]->mi[0]->sphere_center;  //should be centroid

		offset = cm - icm;	// offset is in object space here

		//world space now
		offset = xform.rotate(offset);

		create_phys_chunk( chunks[i]->mi[0],&phys[i]);

	//	chunks[i] = new_inst_index = ENGINE->create_instance2( dummyArch, NULL );

	//	CQASSERT( new_inst_index != INVALID_INSTANCE_INDEX);


		ENGINE->set_transform( chunks[i]->mi[0]->instanceIndex , xform );
			
		ENGINE->set_velocity( chunks[i]->mi[0]->instanceIndex, iv + cross_product( iw, offset ) );

		ENGINE->set_angular_velocity( chunks[i]->mi[0]->instanceIndex, iw );
			
		add_instance_random_impulse( chunks[i]->mi[0]->instanceIndex, xform, offset, strength, delta ,phys[i]);
	}

	return output_spot;
}
//

struct StepBustLeaf
{
private:
	HARCH idx;
public:
	INSTANCE_INDEX i_idx;
	U8 level;
	bool terminal:1;
	bool split:1;
	StepBustLeaf *next;
	
	//

	StepBustLeaf( INSTANCE_INDEX _ii, ARCHETYPE_INDEX _ai, U8 _level, bool _terminal, bool _split, StepBustLeaf *_next )
	{
		i_idx = _ii;
		idx.setArchetype(_ai);
		ENGINE->hold_archetype(_ai);
		level = _level;
		terminal = _terminal;
		split = _split;
		next = _next;
	}
	
	ARCHETYPE_INDEX GetArchetypeIndex()
	{
		return idx;
	};
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

BOOL32 StepBustArchetype( StepBustInfo *info )
{
	ARCHETYPE_INDEX arch_index;
	HARCH new_arch_index_0;
	HARCH new_arch_index_1;
	Vector n, centroid;
	float dot;
	U8 level;
	StepBustLeaf *leafPos, *new0, *new1;

	for( leafPos=info->leafList; leafPos && (leafPos->terminal || leafPos->split); leafPos=leafPos->next );

	if( leafPos == NULL ) {
		return TRUE;
	}

	arch_index = leafPos->GetArchetypeIndex();
	new_arch_index_0.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));
	new_arch_index_1.setArchetype(ENGINE->duplicate_archetype( arch_index, NULL ));

	ASSERT( new_arch_index_0 != INVALID_ARCHETYPE_INDEX );
	ASSERT( new_arch_index_1 != INVALID_ARCHETYPE_INDEX );

	n.x = -1.0 + rand() * 2.0 / RAND_MAX;
	n.y = -1.0 + rand() * 2.0 / RAND_MAX;
	n.z = -1.0 + rand() * 2.0 / RAND_MAX;
	n.normalize();

	if( REND->get_archetype_centroid( arch_index, 1.0f, centroid ) ) {
		
		dot = -dot_product( centroid, n );

		if (REND->split_archetype( arch_index, n, dot, new_arch_index_0, new_arch_index_1, SA_KEEP_NONE, leafPos->i_idx ) ) {

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
		}
	}

	return FALSE;
}

//
/*
HEXPLODE StepExplodeInstance ( INSTANCE_INDEX inst_index, SINGLE strength, S32 num_chunks )
{
	ASSERT( inst_index != INVALID_INSTANCE_INDEX );
	
	StepBustInfo *info = new StepBustInfo;
	
	info->levels = 3;
	//everbody takes responsibility for their own archetype indices
	ARCHETYPE_INDEX bob = ENGINE->get_instance_archetype( inst_index );
	info->leafList = new StepBustLeaf( inst_index, bob, 0, FALSE, 0, NULL );
	ENGINE->release_archetype(bob);
	info->strength = strength;
	info->instIndex = inst_index;

	return (HEXPLODE)info;
}

//

BOOL32 ContinueExplodeInstance ( HEXPLODE info, S32 *num_fragments, INSTANCE_INDEX *chunks, PHYS_CHUNK *phys,U16 num_array_entries, BOOL32 execute )
{
	ASSERT( info != 0 );

	if( !execute ) {
		return StepBustArchetype( info ) ;
	}

	INSTANCE_INDEX new_inst_index;
	HARCH arch_index;
	Vector icm, iv, iw, cm, offset, v, impulse, vert;
	float mass;
	Vector delta;
	Transform xform(false);
	StepBustLeaf *list, *node;

	while( StepBustArchetype(  info ) == 0 ) {}

	if( (list = info->leafList) == NULL ) {
		return TRUE;
	}

	icm = GetCenterOfMass( info->instIndex );

	*num_fragments = bustOffChildren( info->instIndex, icm, info->strength, chunks, num_array_entries,phys );

	arch_index=info->instIndex;

	get_instance_pstate( info->instIndex, arch_index, xform, iv, iw, mass);//, density );

	for( node=list; node; node=node->next ) {
		
		if( *num_fragments == num_array_entries ) {
			break;
		}
		if( !node->terminal ) {
			continue;
		}

		REND->get_archetype_centroid( node->GetArchetypeIndex(), 1.0f, cm );
		cm = xform*cm;
		offset = cm - icm;	// offset is in world space..

		//CQASSERT(node->GetArchetypeIndex() == ENGINE->get_instance_archetype(new_inst_index));
		//ENGINE->release_archetype(new_inst_index);

//		copy_archetype_rigid_body( arch_index, node->GetArchetypeIndex(), delta,info->instIndex );
		copy_phys_chunk(arch_index, node->GetArchetypeIndex(), &phys[*num_fragments]);

		new_inst_index = chunks[*num_fragments] = ENGINE->create_instance2( node->GetArchetypeIndex(), NULL );
		
		ASSERT( INVALID_INSTANCE_INDEX != new_inst_index );

		ENGINE->set_transform( new_inst_index, xform );
		Vector vel = iv + cross_product( iw, offset );
		ENGINE->set_velocity( new_inst_index, vel);
		ENGINE->set_angular_velocity( new_inst_index, iw );
		
	//	CQASSERT(vel.magnitude() < 2550);

	//	CQASSERT(dummy_get_mass(new_inst_index) < 25000.0);
		
		add_instance_random_impulse( new_inst_index, xform, offset, info->strength, delta, phys[*num_fragments] );

		(*num_fragments)++;
	}

	return TRUE;
}

//

void CloseExplodeHandle( HEXPLODE info )
{
	StepBustLeaf *node, *next;

	for( node=info->leafList; node; node=next ) {
		next = node->next;
		delete node;
	}
							
	delete info;
}*/