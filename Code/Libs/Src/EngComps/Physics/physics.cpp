//
// PHYSICS.CPP - RigidBodyPhysics component main source file.
//

//
#pragma warning( disable : 4786 )


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "dacom.h"
#include "tcomponent.h"
#include "da_heap_utility.h"
#include "fdump.h"
#include "filesys.h"
#include "ICamera.h"
#include "IProfileParser_Utility.h"

//

#include "Tfuncs.h"

//

#include "fileutil.h"
#include "rigid.h"
#include "arch.h"
#include "instance.h"
#include "stack.h"
#include "rk.h"

//

HINSTANCE	hInstance;	// DLL instance handle
ICOManager *DACOM;		// Handle to component manager

const C8 *interface_name = "IPhysics";  // Interface name used for registration     
const char *CLSID_Physics = "Physics";

//

int		RigidBodyPhysics::buffer_length	= 0;
float * RigidBodyPhysics::buffer1		= NULL;
float * RigidBodyPhysics::buffer2		= NULL;

CollisionStats RigidBodyPhysics::collision_stats;

//

BOOL COMAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	IComponentFactory *server;

	switch (fdwReason)
	{
	//
	// DLL_PROCESS_ATTACH: Create object server component and register it 
	// with DACOM manager
	//
		case DLL_PROCESS_ATTACH:

			hInstance = hinstDLL;

			DA_HEAP_ACQUIRE_HEAP(HEAP);
			DA_HEAP_DEFINE_HEAP_MESSAGE(hinstDLL);

			server = new DAComponentFactory2<DAComponentAggregate<RigidBodyPhysics>, SYSCONSUMERDESC> (interface_name);

			if (server == NULL)
			{
				break;
			}

			DACOM = DACOM_Acquire();

		//
		// Register at object-renderer priority
		//
			if (DACOM != NULL)
			{
				DACOM->RegisterComponent(server, interface_name, DACOM_NORMAL_PRIORITY);
			}

			server->Release();
			break;

	//
	// DLL_PROCESS_DETACH: Release DACOM manager instance
	//
		case DLL_PROCESS_DETACH:

			if (DACOM != NULL)
			{
				DACOM->Release();
				DACOM = NULL;
			}
			break;
	}

	return TRUE;
}

//
// HERE BEGINS THE implementation of RigidBodyPhysics.
//

//

void FreeStdMeshes(void);

//

RigidBodyPhysics::~RigidBodyPhysics(void)
{
// CLEAN UP EVERYTHING.

	if (buffer1)
	{
		delete [] buffer1;
		buffer1 = NULL;
	}
	if (buffer2)
	{
		delete [] buffer2;
		buffer2 = NULL;
	}
	buffer_length = 0;

//	FreeStdMeshes();

	RKShutdown();

	if (solver)
	{
		solver->Release();
		solver=0;
	}
}

//


GENRESULT COMAPI RigidBodyPhysics::Initialize(void)
{
	GENRESULT result = GR_OK;

	// Get outer IEngine container if present.
	if (get_base()->QueryInterface(IID_IEngine, (void **) &engine) == GR_OK)
	{
		get_base()->Release();
	}

	//
	// Create ODESolver. We could use multiple solvers, e.g. Euler for simple
	// unconnected objects, RK4 for constraints, etc.
	//
	// If we can be guaranteed a high enough update rate, the Trapezoidal solver 
	// should serve us just fine for all purposes.
	//
	DAODESOLVERDESC desc = "Trapezoidal";

	if (solver == 0 && DACOM->CreateInstance(&desc, (void **) &solver) != GR_OK)
	{
		result = GR_INTERFACE_UNSUPPORTED;
	}

	RKInit();

	time = 0.0;
	use_forces			= true;
	Instance::use_forces = true;
	collision_friction	= false;
	joint_dynamics		= false;

	min_dt = 1.0/10;

	buffer_length = 0;
	buffer1 = buffer2 = NULL;

	memset(&collision_stats, 0, sizeof(collision_stats));

	num_dynamic = num_nondynamic = num_fixed = 0;


	opt_get_u32( DACOM, NULL, CLSID_Physics, "DynamicState",
		DS_NONDYNAMIC, (unsigned long*)&default_dyn_state );

	return result;
}

//

BOOL32 COMAPI RigidBodyPhysics::create_archetype(ARCHETYPE_INDEX index, IFileSystem * parent)
{
	ASSERT( parent != NULL );

	RigidBody *archetype = NULL;

	if (index != INVALID_ARCHETYPE_INDEX)
	{
		IFileSystem * rb;

		// "Rigid body" directory is optional.
		//
		if( (rb = OpenDirectory("Rigid body", parent)) == NULL ) {
			rb = parent;
			rb->AddRef();
		}

		archetype = new RigidBody;
		ASSERT( archetype );

		archetype->DEBUG_index = index;
		archetype->load(rb);
		archetype->set_dynamic( default_dyn_state );

		Archetypes.insert( index, archetype );

		rb->Release();
	}

	return archetype != NULL;
}

//

BaseExtent * copy_extent(BaseExtent * x)
{
	BaseExtent * result = NULL;
	switch (x->type)
	{
		case ET_LINE_SEGMENT:
			result = new LineSegmentExtent(((LineSegmentExtent *) x)->segment);
			break;
		case ET_INFINITE_PLANE:
			result = new PlaneExtent(((PlaneExtent *) x)->plane);
			break;
		case ET_SPHERE:
			result = new SphereExtent(((SphereExtent *) x)->sphere);
			break;
		case ET_CYLINDER:
			result = new CylinderExtent(((CylinderExtent *) x)->cylinder);
			break;
		case ET_BOX:
			result = new BoxExtent(((BoxExtent *) x)->box);
			break;
		case ET_TUBE:
			result = new TubeExtent(((TubeExtent *) x)->tube);
			break;
		case ET_CONVEX_MESH:
		{
			CollisionMesh * mesh = new CollisionMesh;
			mesh->copy(((ConvexMeshExtent *) x)->mesh);
			result = new ConvexMeshExtent(mesh);
			break;
		}
		case ET_GENERAL_MESH:
		{
			CollisionMesh * mesh = new CollisionMesh;
			mesh->copy(((MeshExtent *) x)->mesh);
			result = new MeshExtent(mesh);
			break;
		}
	}

	//

	if( x->name ) {
		result->name = strdup( x->name );
	}
	else {
		result->name = NULL;
	}

	result->xform = x->xform;
	
	//

	return result;
}

//
// RECURSIVE FUNCTION.
//
void copy_tree(BaseExtent * dst, BaseExtent * src)
{
	if (src->child)
	{
		dst->child = copy_extent(src->child);
		copy_tree(dst->child, src->child);
	}

	src = src->next;
	while (src)
	{
		dst->next = copy_extent(src);
		dst = dst->next;

		copy_tree(dst, src);
		src = src->next;
	}
}

//

void COMAPI	RigidBodyPhysics::duplicate_archetype(ARCHETYPE_INDEX new_arch, ARCHETYPE_INDEX old_arch)
{
	RigidBody * new_rb = new RigidBody;
	RigidBody * old_rb = GetArchetype( old_arch );

	*new_rb = *old_rb;
	new_rb->DEBUG_index = new_arch;
	new_rb->extents = NULL;

	DBGTRACE12("PHYSICS::duplicate_archetype() Old=#%d, New=%d\n", old_arch, new_arch);

// Must copy extent tree.
	if( old_rb->extents ) {
		new_rb->extents = copy_extent( old_rb->extents );
		copy_tree( new_rb->extents, old_rb->extents );
	}

	Archetypes.insert( new_arch, new_rb );
}

//

void COMAPI RigidBodyPhysics::destroy_archetype(ARCHETYPE_INDEX index)
{
	ASSERT( index != INVALID_ARCHETYPE_INDEX );

	arch_map::iterator arch;

	if( (arch = Archetypes.find( index )) != Archetypes.end() ) {
		delete arch->second;
		Archetypes.erase( index );
	}
}

//

GENRESULT RigidBodyPhysics::get_mass (ARCHETYPE_INDEX idx, SINGLE& ret) const
{
	GENRESULT gr = GR_INVALID_PARMS;

	Archetype *archetype;

	if( (archetype = GetArchetype( idx )) != NULL ) {
		gr = GR_OK;
		ret = archetype->mass;
	}

	return gr;
}

//

BOOL32 COMAPI RigidBodyPhysics::create_instance(INSTANCE_INDEX index, ARCHETYPE_INDEX arch)
{
	ASSERT( index != INVALID_INSTANCE_INDEX );
	ASSERT( arch != INVALID_ARCHETYPE_INDEX );

	BOOL32 result = FALSE;

	Archetype *archetype;

	if( (archetype = GetArchetype( arch )) != NULL ) {

		Instance * instance = new Instance( this, arch, index, archetype->get_dynamic() );

		if (instance)
		{
			if( archetype->extents && archetype->extents->type == ET_SPHERE ) {
//				engine->set_centered_radius( index, ((SphereExtent*)archetype->extents)->sphere.radius,
//													-(archetype->arm) + archetype->extents->xform.translation);
				engine->set_instance_bounding_sphere( index, EN_DONT_RECURSE,
													  ((SphereExtent*)archetype->extents)->sphere.radius,
													  -(archetype->arm) + archetype->extents->xform.translation);
			}

			result = TRUE;

			if( instance->dynamic == DS_DYNAMIC ) {
				dynamic_list.insert(index);
				num_dynamic++;
			}
			else {
				// instances are never FIXED by default.
				num_nondynamic++;
			}

			Instances.insert( index, instance );
		}
	}

	return result;
}

//

void COMAPI RigidBodyPhysics::destroy_instance(INSTANCE_INDEX index)
{
	Instance *instance;

	if( (instance = GetInstance( index )) != NULL ) {

		if( instance->dynamic == DS_DYNAMIC ) {
			dynamic_list.remove(index);
			num_dynamic--;
		}
		else if( instance->dynamic == DS_NONDYNAMIC ) {
			num_nondynamic--;
		}
		else {
			num_fixed--;
		}

		delete instance;
		Instances.erase( index );
	}
}

//

void RigidBodyPhysics::compute_motion(SINGLE time_step)
{
	solver->solve(this, time_step);
}

//
// Make physics update at fixed minimum timestep.
//
//const SINGLE update_step = 1.0 / 30.0;

// DEBUG

Vector HackPoint1, HackPoint2, HackNormal, HackDV, HackV1, HackV2;
bool HackPointsValid = false;

GENRESULT RigidBodyPhysics::init(SYSCONSUMERDESC * info)
{
	return GR_OK;
}

//

GENRESULT COMAPI RigidBodyPhysics::query_archetype_interface( ARCHETYPE_INDEX arch_index, const char *iid, IDAComponent **out_iif ) 
{
	RigidBody *body;

	if( (body = GetArchetype( arch_index )) != NULL ) {
		return body->QueryInterface( iid, (void**)out_iif );
	}

	return GR_GENERIC;
}

//

GENRESULT COMAPI RigidBodyPhysics::query_instance_interface( INSTANCE_INDEX inst_index, const char *iid, IDAComponent **out_iif ) 
{
	Instance *instance;

	if( (instance = GetInstance( inst_index )) != NULL ) {
		return instance->QueryInterface( iid, (void**)out_iif );
	}

	return GR_GENERIC;
}

//

enum vis_state COMAPI RigidBodyPhysics::render_instance( struct ICamera *camera, INSTANCE_INDEX inst_index, float lod_fraction, U32 flags, const Transform *modifier_transform ) 
{
	return VS_UNKNOWN ;
}


//

void COMAPI RigidBodyPhysics::update_instance(INSTANCE_INDEX instance, SINGLE dt)
{
	Instance * inst = GetInstance(instance);
	if (inst && inst->dynamic == DS_DYNAMIC)
	{
		inst->sync_data_from_engine(engine);

		float time_left = dt;
		while (time_left > 0.0)
		{
			dt = Tmin(time_left, min_dt);

		// update position.
			inst->x += inst->v * dt;

		// update velocity.
			Quaternion qw(inst->w);
			Quaternion qdot = qw * inst->q * 0.5;
			inst->q.w += qdot.w * dt;
			inst->q.x += qdot.x * dt;
			inst->q.y += qdot.y * dt;
			inst->q.z += qdot.z * dt;
			inst->q.normalize();

		// if not using forces, ignore momenta, velocities don't change.
			if (use_forces)
			{
				inst->p += (inst->F_applied + inst->F_internal) * dt;
				inst->L += (inst->T_applied + inst->T_internal) * dt;

				inst->R = inst->q;

				Matrix I = inst->R * inst->Ibody * inst->R.get_transpose();
				inst->Iinv = I.get_inverse();

				inst->v = inst->p / inst->mass;
				inst->w = inst->Iinv * inst->L;
			}

			time_left -= dt;
		}

		if (use_forces)
		{
			inst->zero_applied_force_and_torque();
		}
		else
		{
		// be sure to sync rotation matrix with quaternion:
			inst->R = inst->q;

		// recompute inverse inertia tensor, angular momentum. THIS IS NECESSARY
		// IF IMPULSES ARE BEING USED.
			Matrix I = inst->R * inst->Ibody * inst->R.get_transpose();
			inst->Iinv = I.get_inverse();

			inst->L = I * inst->w;
		}

		inst->sync_data_to_engine(engine);
	}
}


//

extern CollisionMesh BoxP;

extern char work[];
void COMAPI RigidBodyPhysics::update(SINGLE time_step)
{
	if (dynamic_list.get_count() == 0)
	{
		return;
	}
//
// First thing we have to do is go ask the Engine for everybody's current 
// position and orientation.
//

	Link<INSTANCE_INDEX> * node = dynamic_list.get_head();
	while (node)
	{
		Instance * instance = GetInstance( node->obj );
		ASSERT( instance );
		instance->sync_data_from_engine(engine);
		node = node->next;
	}

//
// Make sure physics updates at desired rate or better.
// 
	SINGLE start_time = time;

	SINGLE time_left = time_step;
	SINGLE dt;
	while (time_left > 0.0)
	{
		dt = Tmin(time_left, min_dt);

	// Do forward dynamics computation and integration. When this function
	// returns, all objects are in their new state.

		compute_motion(dt);

		time_left -= dt;
		start_time += dt;
	}

// Notify IEngine of new positions and orientations.

	node = dynamic_list.get_head();
	while (node)
	{
		Instance * instance = GetInstance( node->obj );
		ASSERT( instance );
		instance->sync_data_to_engine(engine);

		// update child velocities
		int child_index = -1;
		while ((child_index = engine->get_instance_child_next(node->obj, 0, child_index)) != -1)
		{
			Instance * child_inst = GetInstance( child_index );
			if (child_inst)
				child_inst->sync_velocity_to_engine(engine);
		}

		node = node->next;
	}

// Clear out external forces and torques for new frame.
	node = dynamic_list.get_head();
	while (node)
	{
		Instance * instance = GetInstance( node->obj );
		ASSERT( instance );
		instance->zero_applied_force_and_torque();
		node = node->next;
	}
}

//
// IPhysics methods.
//
SINGLE COMAPI RigidBodyPhysics::get_mass(INSTANCE_INDEX object) const
{
	SINGLE result;
	Instance * instance = GetInstance(object);
	if (instance)
	{
		result = instance->mass;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_mass() - instance has no physics data.\n");
		result = -1.0;
	}

	return result;
}

//
// RETURN LOCATION OF CENTER OF MASS IN ABSOLUTE WORLD COORDINATES.
//
Vector COMAPI RigidBodyPhysics::get_center_of_mass(INSTANCE_INDEX object) const
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		return instance->x;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_center_of_mass() - instance has no physics data.\n");
		return Vector(0,0,0);
	}
}

//

Vector COMAPI RigidBodyPhysics::get_velocity(INSTANCE_INDEX object) const
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		return instance->v;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_velocity() - instance has no physics data.\n");
		return Vector(0,0,0);
	}
}

//

Vector COMAPI RigidBodyPhysics::get_momentum(INSTANCE_INDEX object) const
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		return instance->p;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_momentum() - instance has no physics data.\n");
		return Vector(0,0,0);
	}
}

//

Vector COMAPI RigidBodyPhysics::get_angular_velocity(INSTANCE_INDEX object) const
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		return instance->w;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_angular_velocity() - instance has no physics data.\n");
		return Vector(0,0,0);
	}
}

//

Vector COMAPI RigidBodyPhysics::get_angular_momentum(INSTANCE_INDEX object) const
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		return instance->L;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_angular_momentum() - instance has no physics data.\n");
		return Vector(0,0,0);
	}
}

//

void COMAPI RigidBodyPhysics::set_mass(INSTANCE_INDEX object, SINGLE mass)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->set_mass(mass);
	}
	else
	{
		GENERAL_WARNING("IPhysics::set_mass() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::set_inertia_tensor(INSTANCE_INDEX object, const Matrix & tensor)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->set_inertial_tensor(tensor);
	}
	else
	{
		GENERAL_WARNING("IPhysics::set_inertia_tensor() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::set_local_center_of_mass(INSTANCE_INDEX object, const Vector & center)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->set_center_of_mass(center);
	}
	else
	{
		GENERAL_WARNING("IPhysics::set_local_center_of_mass() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::set_velocity(INSTANCE_INDEX object, const Vector & vel)
{
	engine->set_velocity(object,vel);

	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
//		instance->set_velocity(vel);
	}
	else
	{
		GENERAL_WARNING("IPhysics::set_velocity() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::set_angular_velocity(INSTANCE_INDEX object, const Vector & ang)
{
	engine->set_angular_velocity(object,ang);

	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
	}
	else
	{
		GENERAL_WARNING("IPhysics::set_angular_velocity() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::add_force(INSTANCE_INDEX object, const Vector & force)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->F_applied += force;
	}
	else
	{
		GENERAL_WARNING("IPhysics::add_force() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::add_force_at_point(INSTANCE_INDEX object, const Vector & force, const Vector & absolute)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->F_applied += force;

	// Compute torque introduced by force.
		Vector r = absolute - instance->x;
		Vector torque = cross_product(r, force);
		instance->T_applied += torque;
	}
	else
	{
		GENERAL_WARNING("IPhysics::add_force_at_point() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::add_torque(INSTANCE_INDEX object, const Vector & torque)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->T_applied += torque;
	}
	else
	{
		GENERAL_WARNING("IPhysics::add_torque() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::add_impulse(INSTANCE_INDEX object, const Vector & impulse)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->set_momentum(instance->p + impulse);
		instance->sync_data_to_engine(engine);
	}
	else
	{
		GENERAL_WARNING("IPhysics::add_impulse() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::add_impulse_at_point(INSTANCE_INDEX object, const Vector & impulse, const Vector & absolute)
{
	Instance * instance = GetInstance(object);
	if (instance)
	{
		instance->sync_data_from_engine(engine);
		instance->set_momentum(instance->p + impulse);

		Vector r = absolute - instance->x;
		instance->set_angular_momentum(instance->L + cross_product(r, impulse));
		instance->sync_data_to_engine(engine);
	}
	else
	{
		GENERAL_WARNING("IPhysics::add_impulse_at_point() - instance has no physics data.\n");
	}
}

//

void COMAPI RigidBodyPhysics::set_collision_callback(const CollisionCallback func)
{
	collision_func = func;
}

//
// IODE methods.
//
S32 RigidBodyPhysics::get_y_length(void)
{
	S32 len = 0;

	Link<INSTANCE_INDEX> * node = dynamic_list.get_head();
	while (node)
	{
		INSTANCE_INDEX idx = node->obj;

	//
	// Use full state of dynamic root objects.
	//
		if (engine->get_instance_parent(idx) == INVALID_INSTANCE_INDEX)
		{
			Instance * instance = GetInstance( node->obj );
			ASSERT( instance );
			
			len += instance->get_state_length();

		//
		// Add joint states for compound objects.
		//
			if (joint_dynamics)
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(idx, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					Instance * child = GetInstance(jnt);
					if (child && child->dynamic == DS_DYNAMIC)
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								len += joint->get_state_vector_size() << 1;
								break;
							case JT_DAMPED_SPRING:
								len += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(idx, 0, jnt);
				}
			}
		}

		node = node->next;
	}

//
// Need to add joint states for non-dynamic root objects as well.
// It would be nice to have a list of non-dynamic objects instead of 
// traversing all instances.
//
	if (joint_dynamics)
	{
		INSTANCE_INDEX root = -1;
		while ((root = engine->get_root_instance_next(root)) != INVALID_INSTANCE_INDEX)
		{
			Instance * inst = GetInstance(root);
			if (inst && (inst->dynamic != DS_DYNAMIC))
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(root, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					Instance * child = GetInstance(jnt);
					if (child->dynamic == DS_DYNAMIC)
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								len += joint->get_state_vector_size() << 1;
								break;
							case JT_DAMPED_SPRING:
								len += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(root, 0, jnt);
				}
			}
		}
	}

	return len;
}

//

void RigidBodyPhysics::get_y(SINGLE * dst, SINGLE t)
{
	Link<INSTANCE_INDEX> * node = dynamic_list.get_head();
	while (node)
	{
		INSTANCE_INDEX idx = node->obj;

		if (engine->get_instance_parent(idx) == INVALID_INSTANCE_INDEX)
		{
			Instance * instance = GetInstance( node->obj );
			ASSERT( instance );
			
			instance->get_state_vector(dst);
			dst += instance->get_state_length();

			if (joint_dynamics)
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(idx, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					float joint_q;

					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								engine->get_joint_state( jnt, IE_JST_BASIC, &joint_q );
								*(dst++) = joint_q;
								*(dst++) = child->q_dot;
								break;
							case JT_DAMPED_SPRING:
								child->get_state_vector(dst);
								dst += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(idx, 0, jnt);
				}
			}
		}
		node = node->next;
	}

	if (joint_dynamics)
	{
		INSTANCE_INDEX root = INVALID_INSTANCE_INDEX;
		while ((root = engine->get_root_instance_next(root)) != INVALID_INSTANCE_INDEX)
		{
			Instance * inst = GetInstance(root);
			if (inst && (inst->dynamic != DS_DYNAMIC))
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(root, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					float joint_q;

					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								engine->get_joint_state( jnt, IE_JST_BASIC, &joint_q );
								*(dst++) = joint_q;
								*(dst++) = child->q_dot;
								break;
							case JT_DAMPED_SPRING:
								child->get_state_vector(dst);
								dst += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(root, 0, jnt);
				}
			}
		}
	}
}

//

void RigidBodyPhysics::get_dydt(SINGLE * dst, SINGLE * y, SINGLE t)
{
	set_y(y);

//
// Zero out internal forces and torques, which we'll recompute in this
// function.
//
	Link<INSTANCE_INDEX> * node = dynamic_list.get_head();

	while (node)
	{
		Instance * instance = GetInstance( node->obj );
		ASSERT( instance );
		instance->zero_internal_force_and_torque();
		node = node->next;
	}

//
// Compute forces from constraints, springs, etc.
//
	compute_internal_forces();

//
// Get derivative vector of loose objects.
//
	node = dynamic_list.get_head();
	while (node)
	{
		INSTANCE_INDEX idx = node->obj;
		if (engine->get_instance_parent(idx) == INVALID_INSTANCE_INDEX)
		{
			Instance * instance = GetInstance( node->obj );
			ASSERT( instance );
			instance->get_derivative_vector(dst);
			dst += instance->get_state_length();

			if (joint_dynamics)
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(idx, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								*(dst++) = child->q_dot;
								*(dst++) = child->q_dot_dot;
								break;
							case JT_DAMPED_SPRING:
								child->get_derivative_vector(dst);
								dst += child->get_state_length();
								break;
						}
					}

					jnt = engine->get_instance_child_next(idx, 0, jnt);
				}
			}
		}

		node = node->next;
	}

	if (joint_dynamics)
	{
		INSTANCE_INDEX root = INVALID_INSTANCE_INDEX;
		while ((root = engine->get_root_instance_next(root)) != INVALID_INSTANCE_INDEX)
		{
			Instance * inst = GetInstance(root);
			if (inst && (inst->dynamic != DS_DYNAMIC))
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(root, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);
					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								*(dst++) = child->q_dot;
								*(dst++) = child->q_dot_dot;
								break;
							case JT_DAMPED_SPRING:
								child->get_derivative_vector(dst);
								dst += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(root, 0, jnt);
				}
			}
		}
	}
}

// this is now done by engine
void RigidBodyPhysics::compute_child_velocity( INSTANCE_INDEX joint_index )
{
	ASSERT( joint_index != INVALID_INSTANCE_INDEX );

	const JointInfo* joint = engine->get_joint_info(joint_index);

	Instance * obj1 = GetInstance(engine->get_instance_parent(joint_index));
	Instance * obj2 = GetInstance(joint_index);

// Non-physical objects can be linked together. Make sure we two physical objects here.
	if (obj1 && obj2)
	{
		Matrix Rp = obj1->get_orientation();
		Matrix Rc = obj2->get_orientation();

		Vector r = obj2->x - obj1->x;

	// Compute base velocity of child's C.M. ignoring any joint motion.
		Vector cvel = obj1->v + cross_product(obj1->w, r);

		switch (joint->type)
		{
			case JT_FIXED:
				obj2->set_angular_velocity(obj1->w);
				obj2->set_velocity(cvel);
				break;

			case JT_REVOLUTE:
			{
				float joint_q_dot;
				Vector axis = Rp * joint->axis;
				Vector d = Rc * joint->child_point;

				engine->get_joint_state( joint_index, IE_JST_FIRST_DERIVATIVE, &joint_q_dot );

			// Angular velocity w = parent's w + joint rotation velocity.
				obj2->set_angular_velocity(obj1->w + axis * joint_q_dot);

			// Velocity v = rvel + joint rotation coriolis term.
			// NOTE vector "d" is reversed from ABM definition, hence backwards cross product.
				obj2->set_velocity(cvel + cross_product(d, axis) * joint_q_dot);
				break;
			}

			case JT_PRISMATIC:
			{
				float joint_q_dot;
				Vector axis = Rp * joint->axis;

				engine->get_joint_state( joint_index, IE_JST_FIRST_DERIVATIVE, &joint_q_dot );

			// Angular velocity w = parent's w.
				obj2->set_angular_velocity(obj1->w);

			// Velocity v = rvel + joint translation velocity.
				obj2->set_velocity(cvel + axis * joint_q_dot);

				break;
			}

			case JT_CYLINDRICAL:
			{
				float joint_foo_dot[2];

				Vector axis = Rp * joint->axis;
				Vector d = Rc * joint->child_point;

				engine->get_joint_state( joint_index, IE_JST_FIRST_DERIVATIVE, joint_foo_dot );

				Vector w = obj1->w + axis * joint_foo_dot[1];
				Vector v = cvel + cross_product(axis, d) * joint_foo_dot[1] + axis * joint_foo_dot[0];

				obj2->set_angular_velocity(w);
				obj2->set_velocity(v);

				break;
			}
		}
	}
}

//

void RigidBodyPhysics::set_y(SINGLE * src)
{
	Link<INSTANCE_INDEX> * node = dynamic_list.get_head();
	while (node)
	{
		INSTANCE_INDEX idx = node->obj;

		if (engine->get_instance_parent(idx) == INVALID_INSTANCE_INDEX)
		{
			Instance * instance = GetInstance( node->obj );
			ASSERT( instance );
			instance->set_state_vector(src);
			src += instance->get_state_length();

			if (joint_dynamics)
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(idx, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo * joint = engine->get_joint_info(jnt);
					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								engine->set_joint_state( jnt, IE_JST_BASIC, &src[0] );
								engine->set_joint_state( jnt, IE_JST_FIRST_DERIVATIVE, &src[1] );
								child->q_dot = src[1];
								src += 2;
								break;
							case JT_DAMPED_SPRING:
								child->set_state_vector(src);
								src += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(idx, 0, jnt);
				}
			}
		}

		node = node->next;
	}

	if (joint_dynamics)
	{
		INSTANCE_INDEX root = INVALID_INSTANCE_INDEX;
		while ((root = engine->get_root_instance_next(root)) != INVALID_INSTANCE_INDEX)
		{
			Instance * inst = GetInstance(root);
			if (inst && (inst->dynamic != DS_DYNAMIC))
			{
				JOINT_INDEX jnt = engine->get_instance_child_next(root, 0, INVALID_INSTANCE_INDEX);
				while (jnt != -1)
				{
					const JointInfo* joint = engine->get_joint_info(jnt);

					Instance * child = GetInstance(jnt);
					if (child && (child->dynamic == DS_DYNAMIC))
					{
						switch (joint->type)
						{
							case JT_REVOLUTE:
							case JT_PRISMATIC:
								engine->set_joint_state( jnt, IE_JST_BASIC, &src[0] );
								engine->set_joint_state( jnt, IE_JST_FIRST_DERIVATIVE, &src[1] );
								child->q_dot = src[1];
								src += 2;
								break;
							case JT_DAMPED_SPRING:
								child->set_state_vector(src);
								src += child->get_state_length();
								break;
						}
					}
					jnt = engine->get_instance_child_next(root, 0, jnt);
				}
			}
		}
	}
}

//

SINGLE RigidBodyPhysics::get_t(void) const
{
	return time;
}

//

void RigidBodyPhysics::set_t(SINGLE t)
{
	time = t;
}

//

void COMAPI RigidBodyPhysics::set_dynamic(INSTANCE_INDEX idx, DynamicState state)
{
	Instance * instance = GetInstance(idx);
	if (instance)
	{
		if (instance->dynamic != state)
		{
			bool was_in_list = (instance->dynamic == DS_DYNAMIC);

			switch (instance->dynamic)
			{
			case DS_DYNAMIC:
				num_dynamic--;
				break;
			case DS_NONDYNAMIC:
				num_nondynamic--;
				break;
			case DS_FIXED:
				num_fixed--;
				break;
			}

			instance->dynamic = state;

			bool should_be_in_list = (state == DS_DYNAMIC);

			if (was_in_list && !should_be_in_list)
			{
				dynamic_list.remove(idx);
			}
			else if (!was_in_list && should_be_in_list)
			{
				dynamic_list.insert(idx);
			}

			if (state == DS_FIXED)
			{
				instance->set_position(engine->get_position(idx));
				instance->set_orientation(engine->get_orientation(idx));
			}

			switch (instance->dynamic)
			{
			case DS_DYNAMIC:
				num_dynamic++;
				break;
			case DS_NONDYNAMIC:
				num_nondynamic++;
				break;
			case DS_FIXED:
				num_fixed++;
				break;
			}
		}
	}
	else
	{
		// this can happen for lights, and cameras
		GENERAL_TRACE_1("IPhysics::set_dynamic() - instance has no physics data.\n");
	}
}

//

DynamicState COMAPI RigidBodyPhysics::get_dynamic(INSTANCE_INDEX idx) const
{
	DynamicState result;
	Instance * instance = GetInstance(idx);
	if (instance)
	{
		result = instance->dynamic;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_dynamic() - instance has no physics data.\n");
		result = DS_FIXED;
	}
	return result;
}

//

HANDLE COMAPI RigidBodyPhysics::add_force_element(ForceElement * fe)
{
	forces.insert(fe);
	return (HANDLE) forces.get_tail();
}

//

void COMAPI RigidBodyPhysics::remove_force_element(HANDLE fe_handle)
{
	forces.remove((Link<ForceElement *> *) fe_handle);
}

//

void COMAPI RigidBodyPhysics::enable(PhyState state)
{
	switch (state)
	{
		case PS_USE_FORCES:
			use_forces = true;
			Instance::use_forces = true;
			break;
		case PS_COLLISION_FRICTION:
			collision_friction = true;
			break;
		case PS_JOINT_DYNAMICS:
			joint_dynamics = true;
			break;
	}
}

//

void COMAPI RigidBodyPhysics::disable(PhyState state)
{
	switch (state)
	{
		case PS_USE_FORCES:
			use_forces = false;
			Instance::use_forces = false;
			break;
		case PS_COLLISION_FRICTION:
			collision_friction = false;
			break;
		case PS_JOINT_DYNAMICS:
			joint_dynamics = false;
			break;
	}
}

//

bool COMAPI RigidBodyPhysics::is_enabled(PhyState state)
{
	bool result = false;

	switch (state)
	{
		case PS_USE_FORCES:
			result = use_forces;
			break;
		case PS_COLLISION_FRICTION:
			result = collision_friction;
			break;
		case PS_JOINT_DYNAMICS:
			result = joint_dynamics;
			break;
	}

	return result;
}

//

bool COMAPI RigidBodyPhysics::get_extent(const BaseExtent ** extent, INSTANCE_INDEX object)
{
	ASSERT( extent );
	bool result = false;
	Instance * instance = GetInstance(object);
	if (instance)
	{
		RigidBody * body = GetArchetype(instance->body_idx);
		if (body && body->extents)
		{
			*extent = body->extents;
			result = true;
		}
	}
	else
	{
		*extent = NULL;
	}

	return result;
}

//

bool COMAPI RigidBodyPhysics::get_archetype_extent(const BaseExtent ** extent, ARCHETYPE_INDEX arch)
{
	bool result = false;
	
	if (arch != INVALID_ARCHETYPE_INDEX)
	{
		RigidBody * rb = GetArchetype(arch);
		if (rb)
		{
			*extent = rb->extents;
			result = true;
		}
		else
		{
			GENERAL_WARNING("IPhysics::get_archetype_extent() - archetype has no physics data.\n");
		}
	}

	return result;
}

//

void COMAPI RigidBodyPhysics::set_joint_driver(JOINT_INDEX jidx, IJointDriver * jd)
{
	if (jidx != INVALID_JOINT_INDEX)
	{
		joint_drivers[jidx] = jd;
	}
}

//

Vector COMAPI RigidBodyPhysics::get_local_center_of_mass(INSTANCE_INDEX idx) const
{
	Instance * instance = GetInstance(idx);
	if (instance)
	{
		RigidBody * body = GetArchetype(instance->body_idx);
		if (body)
		{
			return -body->arm;
		}
		else
		{
			GENERAL_WARNING("IPhysics::get_archetype_extent() - instance has no archetype.\n");
			return Vector(0, 0, 0);
		}
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_local_center_of_mass() - instance has no physics data.\n");
		return Vector(0, 0, 0);
	}
}

//

const Matrix * COMAPI RigidBodyPhysics::get_inertia_tensor(INSTANCE_INDEX idx) const
{
	const Matrix * result = NULL;

	Instance * instance = GetInstance(idx);
	if (instance)
	{
		result = &instance->Ibody;
	}
	else
	{
		GENERAL_WARNING("IPhysics::get_inertia_tensor() - instance has no physics data.\n");
	}
	return result;
}

//

void COMAPI RigidBodyPhysics::get_stats(PhyInstanceStats & stats) const
{
	stats.num_dynamic = num_dynamic;
	stats.num_nondynamic = num_nondynamic;
	stats.num_fixed = num_fixed;
}


bool COMAPI RigidBodyPhysics::is_valid (INSTANCE_INDEX idx) const
{
	Instance * instance = GetInstance(idx);
	return (instance!=0);
}
//